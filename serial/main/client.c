#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <esp_log.h>

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/queue.h>
#include <freertos/task.h>

#include <nvs.h>
#include <nvs_flash.h>

#include <esp_bt.h>
#include <esp_bt_main.h>
#include <esp_gap_bt_api.h>
#include <esp_bt_device.h>
#include <esp_spp_api.h>
#include "asc_bt_common.h"

#include "client.h"
#include "serial.h"
#include "utils.h"

// Blocks, delays and waits.
#define WAIT_MAX UINT32_MAX
#define WAIT_COPY ((TickType_t)100 / portTICK_PERIOD_MS)
#define WAIT_WRITE ((TickType_t)100 / portTICK_PERIOD_MS)
#define WAIT_RESPONSE ((TickType_t)1000 / portTICK_PERIOD_MS)

// Event group bits:
#define EVT_CONNECTED (1 << 0)
#define EVT_WRITE_READY (1 << 1)

// Synchronization objects:
QueueHandle_t bt_recv = NULL;
EventGroupHandle_t events = NULL;

// Bluetooth remote device data:
esp_bd_addr_t remote_addr = {0};
uint8_t remote_len = 0;
char remote_name[ESP_BT_GAP_MAX_BDNAME_LEN + 1];

// Bluetooth SPP connection data:
uint32_t con_handle = 0xFFFFFFFF;

// Forward declarations:
BluetoothPacket init_packet();
int test_command(const char *message);
int send(int flags, const char *message, TickType_t wait);

// Callback methods:
void data_client_bt_callback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param);
void data_client_gap_callback(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param);

/*
* Quick and dirty setup of drivers and services we will need for this demo's
* functionality.  We neglect doing any attempt at error handling or recovery.
* Returns 0 if initialization is successful.
*/
int data_client_prepare()
{
    memset(remote_name, 0, ESP_BT_GAP_MAX_BDNAME_LEN + 1);

    if (nvs_flash_init() != ESP_OK)
        return -3;

    bt_recv = xQueueCreate(8, sizeof(BluetoothPacket));
    if (!bt_recv)
        return -2;

    events = xEventGroupCreate();
    if (!events)
        return -1;

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();

    esp_bt_controller_mem_release(ESP_BT_MODE_BLE);

    if (esp_bt_controller_init(&bt_cfg) != ESP_OK)
        return 1;

    if (esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT) != ESP_OK)
        return 2;

    if (esp_bluedroid_init() != ESP_OK)
        return 3;

    if (esp_bluedroid_enable() != ESP_OK)
        return 4;

    if (esp_bt_gap_register_callback(data_client_gap_callback) != ESP_OK)
        return 5;

    if (esp_spp_register_callback(data_client_bt_callback) != ESP_OK)
        return 6;

    if (esp_spp_init(ESP_SPP_MODE_CB) != ESP_OK)
        return 7;

    return 0;
}

// Initializes a BluetoothPacket structure.
BluetoothPacket init_packet()
{
    BluetoothPacket out;
    memset(&out, 0, sizeof(BluetoothPacket));
    return out;
}

// Writes a single packet over the Bluetooth connection.  Does not perform
//	any validation of flags or message contents.  Returns 0 on success.
int send(int A_flags, const char *A_msg, TickType_t A_wait)
{
    // Wait for the channel out to be available.  Terminate on time-out.
    EventBits_t bits_write_result = xEventGroupWaitBits(events, EVT_WRITE_READY, pdTRUE, pdTRUE, A_wait);
    if (!(bits_write_result & EVT_WRITE_READY))
        return 1;

    // Construct the packet from arguments.
    BluetoothPacket pak = init_packet();
    pak.flags = A_flags;
    strncpy(pak.data, A_msg, 59);

    // Send the packet off on its merry way.
    esp_spp_write(con_handle, sizeof(pak), (void *)&pak);

    return 0;
}

int test_command(const char *A_msg)
{
    if (send(BT_FLAG_END, A_msg, WAIT_WRITE))
    {
        serial_out("Failed to send packet.  Write unavailable.");
        return 1;
    }

    int response_complete = 0;
    while (!response_complete)
    {
        BluetoothPacket in = init_packet();
        if (xQueueReceive(bt_recv, &in, WAIT_RESPONSE) != pdTRUE)
        {
            serial_out("Timed out waiting for response..");
            return 2;
        }

        // Minimal packet verification to prevent buffer overruns.
        assert(in.data[59] == '\0');

        // Print out the packet contents.
        serial_out(in.data);

        // Evaluate the packet flags and proceed appropriately.
        if (in.flags & BT_FLAG_ERR)
        {
            serial_out("ERROR");
            // printf("ERROR: %s\n", in.data);
            return 3;
        }

        if (in.flags & BT_FLAG_REQ)
        {
            // Ack requested:
            if (send(BT_FLAG_ACK | BT_FLAG_END, "", WAIT_WRITE))
            {
                serial_out("Failed to send ACK.  Write unavailable.");
                return 1;
            }
        }

        if (in.flags & BT_FLAG_END)
        {
            response_complete = 1;
            serial_out("Response complete.");
        }
    }
    return 0;
}

void worker(void *A_param)
{
    // Worker blocks indefinitely until the connection event bit is set.  Observe
    //	that we need to set up a loop here and check the output bits -- it is possible
    //	that the integer maximum tick count can elapse, and we need to verify that the
    //	bits were set rather than timed out.

    int wait_ready = 0;
    EventBits_t wait_result = 0;
    while (!wait_ready)
    {
        wait_result = xEventGroupWaitBits(events, EVT_CONNECTED, pdFALSE, pdTRUE, UINT32_MAX);
        if (wait_result & EVT_CONNECTED)
        {
            wait_ready = 1;
        }
    }

    // Try out some commands.
    test_command("test_single");
    test_command("test_multi");
    test_command("test_req");
    test_command("poll 64");

    // We're done here -- close the BT connection and terminate.
    esp_spp_disconnect(con_handle);
    vTaskDelete(NULL);
}

void data_client_gap_callback(esp_bt_gap_cb_event_t A_event, esp_bt_gap_cb_param_t *A_param)
{
    switch (A_event)
    {
    case ESP_BT_GAP_DISC_RES_EVT:
    {
        for (int i = 0; i < A_param->disc_res.num_prop; ++i)
        {
            if (A_param->disc_res.prop[i].type == ESP_BT_GAP_DEV_PROP_EIR)
            {
                // Check the EIR property, see if the name is a match for the device
                //	that we're looking for.  We go about this in a slightly roundabout way,
                //	in order to ensure that there are no buffer overruns.
                //
                uint8_t *eir = A_param->disc_res.prop[i].val;

                uint8_t *eir_name = NULL;

                eir_name = esp_bt_gap_resolve_eir_data(eir, ESP_BT_EIR_TYPE_CMPL_LOCAL_NAME, &remote_len);
                if (eir_name == NULL)
                {
                    eir_name = esp_bt_gap_resolve_eir_data(eir, ESP_BT_EIR_TYPE_SHORT_LOCAL_NAME, &remote_len);
                }
                if (eir_name == NULL)
                {
                    // If we can't resolve the EIR as the complete or short name, skip it.
                    continue;
                }

                if (remote_len > ESP_BT_GAP_MAX_BDNAME_LEN)
                    remote_len = ESP_BT_GAP_MAX_BDNAME_LEN;

                memcpy(remote_name, eir_name, remote_len);
                remote_name[remote_len] = '\0';

                // If the name we've found is correct, take the BD address and start
                //	service discovery, while ending device discovery.
                if (remote_len == strlen(BT_DATA_SOURCE_DEVICE) &&
                    !strncmp(remote_name, BT_DATA_SOURCE_DEVICE, remote_len))
                {
                    memcpy(remote_addr, A_param->disc_res.bda, ESP_BD_ADDR_LEN);
                    esp_spp_start_discovery(remote_addr);
                    esp_bt_gap_cancel_discovery();
                    break;
                }
            }
        }
        break;
    }
    default:
        // Do nothing.
        break;
    }
}

void data_client_bt_callback(esp_spp_cb_event_t A_event, esp_spp_cb_param_t *A_param)
{
    // Barebones SPP callback method.  Implements discovery and connection, as well as
    //	monitoring write congestion status.

    switch (A_event)
    {
    case ESP_SPP_INIT_EVT:
        esp_bt_dev_set_device_name(BT_DATA_CLIENT_DEVICE);
        esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
        esp_bt_gap_start_discovery(ESP_BT_INQ_MODE_GENERAL_INQUIRY, 30, 0);
        break;

    case ESP_SPP_DISCOVERY_COMP_EVT:
        // NOTE: We neglect to verify whether the actual named service we're interested in exists.
        //	For this demo, that works fine, but in general we should be verifying that as well.
        if (A_param->disc_comp.status == ESP_SPP_SUCCESS)
        {
            esp_spp_connect(ESP_SPP_SEC_NONE, ESP_SPP_ROLE_MASTER, A_param->disc_comp.scn[0], remote_addr);
        }
        break;

    case ESP_SPP_OPEN_EVT:
        con_handle = A_param->open.handle;
        xEventGroupSetBits(events, EVT_CONNECTED | EVT_WRITE_READY);
        serial_out("Connection opened.");
        break;

    case ESP_SPP_DATA_IND_EVT:
        assert(A_param->data_ind.len == sizeof(BluetoothPacket));

        // Cannot proceed unless there is space on the inbound packet queue to store
        //	this new packet.  This may be problematic if long waits are occuring.
        while (xQueueSend(bt_recv, A_param->data_ind.data, WAIT_COPY) != pdTRUE)
        {
            // Spin.
        }
        break;

    case ESP_SPP_WRITE_EVT:
        if (!A_param->write.cong)
        {
            xEventGroupSetBits(events, EVT_WRITE_READY);
        }
        break;

    case ESP_SPP_CONG_EVT:
        if (!A_param->cong.cong)
        {
            xEventGroupSetBits(events, EVT_WRITE_READY);
        }
        break;

    case ESP_SPP_CLOSE_EVT:
        serial_out("Connection closed.");
        break;

    default:
        break;
    }
}
