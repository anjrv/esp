#ifndef CLIENT_H_
#define CLIENT_H_

#define BT_DATA_CLIENT_DEVICE "Kool-Aid"

int data_client_prepare();
int bt_scan_now();
void bt_disconnect();
void append_bt(void *parameters);

char BT_DATA_SOURCE_DEVICE[32];
char BT_DATA_SOURCE_SERVICE[32];

int active_connection;

#endif
