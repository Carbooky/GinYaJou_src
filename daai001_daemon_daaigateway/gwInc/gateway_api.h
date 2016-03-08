#ifndef _SENPU_API_H
#define _SENPU_API_H

#include "format.h"
#include <semaphore.h>
#include "util.h"

#define MAX_NUM_RDNT_GW 4

void *thread_redundant_host_control(void *argu);

int gateway_api_init(void);

typedef struct _AGING_LIST_ELEMENT
{
	bool_t needCheck;
	uint8_t count;
	uint8_t invId;
	uint16_t invType;
}aging_element_t;

typedef struct _AGING_LIST
{
	uint8_t num;
	aging_element_t aging_element[256];
}aging_list_t;

typedef struct _SCAN_CHANNEL
{
	char dev_name[64];
	uint16_t scan_capability;
	sem_t* sem_invScan_p;
	sem_t* sem_waitInvScanFin_p;
	scan_inv_info_t* scan_inv_info_p;
	uint16_t offset;
	aging_list_t aging_list;
	pthread_t thread_update_scan;
	pthread_t thread_inverter_manager;
	eversolar_list_t* list_head;
}scan_channel_info_t;

typedef struct _REDUNDANT_STRUCT
{
	char access_bus_name[32];
	char unique_name[16];
	bool_t enable;
	uint8_t state;
	uint8_t host_level;
	sem_t* ctrl_lock_p;
	sem_t* recv_lock_p;
}redundant_t;

typedef enum
{
	RDNT_STATE_IDLE=0,
	RDNT_STATE_STANDBY,
	RDNT_STATE_CONTEND,
	RDNT_STATE_HOST
}redundant_state_t;

typedef struct _REDUNDANT_STRUCT_REQ
{
	uint16_t prefix;
	char unique_name[16];
	uint8_t host_level;
	uint8_t cmd;
	uint16_t FCS;
}redundant_pkt_req;

typedef struct _REDUNDANT_STRUCT_RSP
{
	uint16_t prefix;
	char unique_name[16];
	uint8_t host_level;
	uint8_t rsp;
	uint16_t FCS;
}redundant_pkt_rsp;

typedef enum{
	RDNT_CMD_HOST=0x10,
	RDNT_CMD_CONTEND=0x20,
	RDNT_RSP_CONTEND=0x40,
	RDNT_RSP_STANDBY=0x80
}rdnt_cmd_t;

/*
typedef struct _SCAN_CHANNEL_BODY
{
	char dev_name[64];
	uint16_t scan_capability;
	sem_t* sem_invScan_p;
	sem_t* sem_waitInvScanFin_p;
	sem_t sem_invScan;
	sem_t sem_waitInvScanFin;
	scan_inv_info_t scan_inv_info;
	pthread_t thread_update_scan;
	pthread_t thread_inverter_manager;
}scan_channel_info_t2;
*/
typedef unsigned char gGpioValue_t;
typedef unsigned char gGpioAddr_t;
typedef unsigned char gInverterAddr_t;
typedef unsigned char gGpioEvent;
typedef void (*alarm_func_t)(gGpioValue_t alarmGpio);
typedef void (*audio_func_t)(void);
typedef void (*ind_func_t)(int event);

typedef enum{
	SNAPSHOT_GET_ONE_FRAME = 0x01,
	SNAPSHOT_TRIGGER_GET_FRAMES = 0x08,
	SNAPSHOT_SET_NUM_OF_FRAMES = 0x11,
	SNAPSHOT_SET_INTERVAL,
	SNAPSHOT_SET_PRI_NAME 
}gSnapShot_t;

typedef enum{
	AUDIO_SET_CAPTURE_VOLUME=0x01,
	AUDIO_SET_PLAYBACK_VOLUME=0x11,
	AUDIO_PLAY_FILE_DEFAULT = 0x80,
	AUDIO_PLAY_FILE_RECORD = 0x81
}gAudioCmd_t;

typedef enum{
	VIDEO_CMD_SET_RESTART = 1,
	VIDEO_CMD_SET_RESOL,
	VIDEO_CMD_SET_CODEC,
	VIDEO_CMD_SET_VIDEO,
	VIDEO_CMD_SET_AUDIO
}gVideoCmd_t;

typedef enum{
	VIDEO_RESOL_720P = 1,
	VIDEO_RESOL_1080P,
	VIDEO_CODEC_H264,
	VIDEO_CODEC_MJPEG,
	VIDEO_CODEC_MPEG4,
	VIDEO_VIDEO_ON,
	VIDEO_VIDEO_OFF,
	VIDEO_AUDIO_ON,
	VIDEO_AUDIO_OFF
}gVideoValue_t;

typedef enum{
	SYS_SET_TIME = 0x11,
	SYS_SET_TIMESTAMP = 0x12,
	SYS_SET_IND_CALLBACK = 0x13,
	SYS_SET_AUD_CALLBACK = 0x14,
	SYS_INV_REBOOT = 0x21,
	SYS_GET_DEV_IP = 0x41,
	SYS_GET_DEV_MAC = 0x42,
	SYS_GET_SCAN_INV = 0x43,
	SYS_GET_CURRENT_TIME = 0x44
}gSysCmd_t;

typedef enum{
	SYS_INFO_INIT = 0x01,
    SYS_INFO_WR_CACHE,
    SYS_INFO_SNAPSHOT_CACHE_TO_BACKUP,
    SYS_INFO_WR_BACKUP,
    SYS_INFO_DEL_CACHECOPY_IN_BACKUP,
    SYS_INFO_RESTORE_BACKUP_TO_CACHE,
    SYS_INFO_BACKUP_FULL
}gBackupState_t;

typedef enum{
	EVNT_WR_INIT = 0x01,
	EVNT_WR_CACHE_FULL,
	EVNT_WR_CACHE_AVAIL,
	EVNT_WR_BACKUP_FULL,
	EVNT_WR_BACKUP_EMPTY,
	EVNT_WR_BACKUP_HAS_DATA,
	EVNT_WR_BACKUP_AVAIL,
	EVNT_WR_JUST_GO
}gBackupEvent_t;

enum{
	gSuccess = 0,
	gTimeOut,
	gIllegalWrite
};

typedef struct{
	uint8_t num;
	uint8_t addr[INV_MAX];
}inv_info_t;

extern char info_3G[128];

extern scan_total_inv_info_t sys_inv_info_all;

extern int senpu_GPIOGet(gGpioValue_t * gpioValue);		// get gpio value
extern int senpu_GPIOSet(gGpioValue_t gpioValue);		// set gpio value
extern int senpu_SnapShotSet(gSnapShot_t cmd, void *params);
extern int senpu_AudioSet(gAudioCmd_t cmd, int value);
extern int senpu_StreamVideoSet(gVideoCmd_t cmd, int value);
extern int senpu_EnergyGet(gInverterAddr_t invAddr, inverter_msg2host_t *data);
extern int senpu_EnergyReSet(void);
extern int senpu_SysCtrlSet(gSysCmd_t cmd, char *params);
extern int senpu_SysCfgGet(gSysCmd_t cmd, void *params);
extern int senpu_SecurityAlarmInd(gGpioAddr_t *alarmGPIO, gGpioEvent *event);
extern int senpu_api_init(void);

// Newest APIs for XML
int getGPIO(char * returnString, int maxLen);
int getEnergy(int index, char * returnString, int maxLen);
int getSysIp(char* returnString, int maxLen);
int getSysMac(char* returnString, int maxLen);
int getSysInv(char* returnString, int maxLen);
//int getSysTimeStampEnable(char * returnString, int maxLen);
int getAudio(char * returnString, int maxLen);
int getStreamVideo(char * returnString, int maxLen);
int getSysTime(char * returnString, int maxLen);
int getSysStatus(char * returnString, int maxLen);

int daai_getSysInv(char* returnString, int maxLen);

int daai_getEnergy(int, char *, int);

int daai_getSensorInfo(char* returnString, int maxLen);

int daai_getSysCtrl(char * returnString, int maxLen);
int daai_setSysCtrl(char * inputString);

int setSysInit_senpu(void * , void*);
int daai_setSysInit(void *);
int daai_setSysDispose();

int setGPIO(char * inputString);
int setSysTimeStampEnable(char * inputString);
int setSnapShot(char * inputString);
int setAudio(char * inputString);
int setStreamVideo(char * inputString);
int setSysTime(char * inputString);
int setSysCtrl(char * inputString);

int getSubSetInvInfo(scan_inv_info_t* motherSet, uint16_t invType, scan_inv_info_t* subSet);

/*
#define USER_LEN 32
#define PASSWORD_LEN 16
#define IP_STR_LEN 20

typedef struct{
	char id[USER_LEN];
	char password[PASSWORD_LEN];
	char remote_ip[IP_STR_LEN];
}login_data_t;

typedef struct{
	char user_id[USER_LEN];
	int	authority;
}get_user_authority_t;
*/
#endif

