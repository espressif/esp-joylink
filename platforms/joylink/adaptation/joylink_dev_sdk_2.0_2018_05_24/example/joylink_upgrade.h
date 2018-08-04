#ifndef JOYLINK_UPGRADE_H_
#define JOYLINK_UPGRADE_H_

#define USER_BIN1           0x00    /**< firmware, user1.bin */
#define USER_BIN2           0x01    /**< firmware, user2.bin */


#define UPGRADE_FLAG_IDLE 0x00
#define UPGRADE_FLAG_START 0x01
#define UPGRADE_FLAG_FINISH 0x02

#define SYSTEM_BIN_NO_MAP_MAX_SECTOR    58
#define SYSTEM_BIN_MAP_512_512_MAX_SECTOR  122
#define SYSTEM_BIN_MAP_1024_1024_MAX_SECTOR 250

extern xTaskHandle *ota_task_handle;

//void fota_event_cb(System_Event_t *event);
void start_FOTA_task(void *pvParameters);
void fota_check_version_task(void *pvParameters);
#endif
