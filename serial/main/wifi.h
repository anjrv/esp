#ifndef WIFI_H_
#define WIFI_H_

int espnow_init(void);
void wifi_init(void);
void espnow_onreceive(const uint8_t *mac, const uint8_t *data, int len);
int wifi_send_locate();
int wifi_send_status();

int added_peers;

#endif
