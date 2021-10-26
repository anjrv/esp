#ifndef CLIENT_H_
#define CLIENT_H_

// Reference: Most of the raw implementation in this file was provided by the bt_client demo.

#define BT_DATA_CLIENT_DEVICE "Kool-Aid"

int data_client_prepare();
int bt_scan_now();
void bt_disconnect();
void append_bt(void *parameters);

char BT_DATA_SOURCE_DEVICE[32];
char BT_DATA_SOURCE_SERVICE[32];

int active_connection;

#endif
