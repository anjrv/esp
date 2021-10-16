#ifndef BT_H_
#define BT_H_

#define BT_DATA_SOURCE_DEVICE "ASC_BT_DataSource"
#define BT_DATA_SOURCE_SERVICE "ASC_BT_DataSourceSvc"

/* 
* A primitive data packet definition.
* 
* DATA INVARIANTS:
*  'data' shall be a valid, null terminated C-string.  Under all circumstances, data[59] == '\0'.
* 
*  !((flags & REQ) && (flags & ACK))	:: Cannot be both request and acknowledge
*  !((flags & PART) && (flags & END))	:: Cannot be both message partial and final
*  (flags & (PART | END | ERR))			:: Must be message partial OR final OR error.
*  !((flags & ERR) && (flags & ~ERR))	:: Cannot be error and anything else
*  (flags & ACK) -> (flags & END)		:: Acknowledge messages must be final.
* 
*  DAT flags are "don't care" -> can be high or low under any circumstances.
*/

typedef struct BluetoothPacket
{
    int flags;
    char data[60];
} BluetoothPacket;

#define BT_FLAG_ACK 0x00000001
#define BT_FLAG_REQ 0x00000002
#define BT_FLAG_PART 0x00000004
#define BT_FLAG_END 0x00000008
#define BT_FLAG_DAT 0x00FFFF00
#define BT_FLAG_ERR 0x80000000

#endif
