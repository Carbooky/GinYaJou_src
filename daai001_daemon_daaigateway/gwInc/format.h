#ifndef _FORMAT_H  
#define _FORMAT_H

//#include "..\targetver.h"
#include "ZtcTypedef.h"

#pragma pack(1)     /* set alignment to 1 byte boundary */

#define SYNC_SYMBOL 0x02
#define GID_SENPU 0x31
#define bool_t _Bool

#define INV_MSG_CRASH 2
#define INV_MSG_CRC_ERR 3

#define TRUE 1
#define FALSE 0

#define INV_MAX 10

//#define MODEMDEVICE "/dev/ttyO2"  // wired rj45
#define MODEMDEVICE "/dev/ttyUSB0"  // usb
//#define MODEMDEVICE "/dev/ttyO1"  // wireless 433 or 900
#define INVERTER_BAUDRATE B9600

typedef unsigned char gGpioValue_t;
typedef unsigned char gGpioAddr_t;
typedef unsigned char gInverterAddr_t;
typedef unsigned char gGpioEvent;
typedef void (*alarm_func_t)(gGpioValue_t alarmGpio);
typedef void (*audio_func_t)(void);
typedef void (*ind_func_t)(int event);

typedef struct INV_info{
	uint8_t num;
	uint8_t addr[256];
}scan_inv_info_t;


extern uint8_t rs232_run_flag;

// entire struct is 98 *2 bytes
typedef struct inverter_meta_data_struct{
	//0xb5 ~ 0xbc
	uint16_t State;
	uint16_t Err1;
	uint16_t Err2;
	uint16_t Err3;
	uint16_t Err4;
	uint16_t VpvA;
	uint16_t VpvB;
	uint16_t VpvC;
	
	//0xbd ~ 0xc4
	uint16_t PpvA;
	uint16_t PpvB;
	uint16_t PpvC;
	uint16_t Vac;
	uint16_t Pac;
	uint16_t Iac;
	uint16_t Fac;
	uint16_t TOTAL_POWER_H;
	
	//0xc5 ~ 0xcc
	uint16_t TOTAL_POWER_L;
	uint16_t EpvA_H;
	uint16_t EpvA_L;
	uint16_t EpvB_H;
	uint16_t EpvB_L;
	uint16_t EpvC_H;
	uint16_t EpvC_L;
	uint16_t TIME_ON_TODAY;
	
	//0xcd ~ 0xd4
	uint16_t Ires;
	uint16_t TempHS;
	uint16_t Z;
	uint16_t IR;
	uint16_t TIME_HR_CNT;
	uint16_t TIME_MIN_CNT;
	uint16_t TIME_SEC_CNT;
	uint16_t BridgeRelay_ON_NumH;
	
	//0xd5 ~ 0xdc
	uint16_t BridgeRelay_ON_NumL;
	uint16_t VacTRIP;
	uint16_t FacTRIP;
	uint16_t IdcinjHS;
	uint16_t Eac_L_TODAY;
	uint16_t NA_1;
	uint16_t State_COD_CURR;
	uint16_t NA_2;
	
	//0xdd ~ 0xe4
	uint16_t NA_3;
	uint16_t POWER_ON_NUM;
	uint16_t POWER_ON_NUM_CURR;
	uint16_t IresCar;	
	uint16_t IpvA;
	uint16_t IpvB;
	uint16_t IpvC;
	uint16_t VpvATRIP;
	
	//0xe5 ~ 0xec
	uint16_t VpvBTRIP;
	uint16_t VpvCTRIP;
	uint16_t VpvTRIP_SOUR_CURR;
	uint16_t IDCC_TRIP;
	uint16_t IR_TRIP;
	uint16_t IR_DELTA_TRIP;
	uint16_t IR_RATE_TRIP;
	uint16_t V1650_TRIP;
	
	//0xed ~ 0xf4
	uint16_t DeltaCPU_VPE;
	uint16_t DeltaCPU_Idc;
	uint16_t DeltaCPU_Vac;
	uint16_t DeltaCPU_Fac;
	uint16_t DeltaCPU_Ires;
	uint16_t DeltaCPU_Zac;
	uint16_t RCMA10_DELT_TRIP;
	uint16_t NA_4;
	
	//0xf5 ~ 0xfc
	uint16_t Ires_FAST_TRIP;
	uint16_t Ires_SLOW_TRIP;
	uint16_t Pdc;
	uint16_t VDCbus1;
	uint16_t NA_5;
	uint16_t VDCbus2;
	uint16_t Idcinj;
	uint16_t IdcinjTRIP;
	
	//0xfd ~ 0x104
	uint16_t FAN_POWER_FLG;
	uint16_t PF_COS_THETA_DISP;
	uint16_t VacTImerTRIP;
	uint16_t FacTimeTRIP;	
	uint16_t NA_6;
	uint16_t AutoTest_Flg;
	uint16_t AutoTest_STATE_REG;
	uint16_t AutoTest_VacH_COM;
	
	//0x105 ~ 0x10c
	uint16_t AutoTest_VacL_COM;
	uint16_t AutoTest_FacH_COM;
	uint16_t AutoTest_FacL_COM;
	uint16_t DSP_IDN;
	uint16_t MODEL_NAME;
	uint16_t SN_HIGH;
	uint16_t SN_LOW;
	uint16_t DEVICE_VER;
	
	//0x10d ~ 0x114
	uint16_t VersionW1;
	uint16_t VersionW1_CURR;
	uint16_t Sensor1;
	uint16_t Sensor2;
	uint16_t Sensor3;
	uint16_t Sensor4;
	uint16_t NA_7;
	uint16_t NA_8;

	//0x115 ~ 0x116
	uint16_t NA_9;
	uint16_t NA_10;
}inverter_meta_data_t;


typedef struct inverter_info_struct{

	// the order is allocated according to the order of data received from inverter	
	uint16_t W_0x85;
	
	char brand_name[16];		// 0x0306
	char type_name[16];			// 0x030e
	char sn_name[16];				// 0x0316
	
	uint16_t W_0x17[2];
	uint16_t W_0x12;
	uint16_t W_0x56;
	uint16_t W_0x86;
	uint16_t W_0x83;
	
	inverter_meta_data_t inverter_meta_data;
	
	uint16_t W_0xc1;
	uint16_t W_0xf7;
	uint16_t W_0x01[11];
	uint16_t W_0x0c[11];
	uint16_t W_0x15;
}inverter_info_t;

extern inverter_info_t inverter_info;

typedef struct inverter_msg2host_struct{
	uint8_t state_code;
	uint8_t brand_name[16];
	uint8_t type_name[16];
	uint8_t sn_name[16];
	uint8_t total_hr_h;
	uint8_t total_hr_l;
	uint8_t total_min_h;
	uint8_t total_min_l;
	uint8_t gen_H_h;
	uint8_t gen_H_l;
	uint8_t gen_L_h;
	uint8_t gen_L_l;
	uint8_t pac_h;
	uint8_t pac_l;
	uint8_t pdc_h;
	uint8_t pdc_l;
}inverter_msg2host_t;

typedef struct inverter_msg2host_new_struct{
	uint8_t state_code;
	uint8_t inv_id;
	uint8_t brand_name[16];
	uint8_t type_name[16];
	uint8_t sn_name[16];
	uint16_t total_hr;
	uint16_t total_min;
	uint16_t total_sec;
	uint16_t total_gen_h;
	uint16_t total_gen_l;
	uint16_t pac;
	uint16_t pdc;
	uint16_t total_today;
}inverter_msg2host_new_t;


typedef struct status_ind_struct{
	uint8_t sn;
	uint8_t IP[4];
	uint8_t inv_state;
}status_ind_t;

// each inverter should have its own state and order, 
// states between INV_CMD_STOP and INV_CMD_STATE_END are different from one another 
typedef enum{
	INV_CMD_STATE_INIT=0,
	INV_CMD_STATE_RDY,
	INV_CMD_STATE_TIMEOUT,
	INV_CMD_STATE_ERR,
	INV_CMD_STATE_STOP,
		
	INV_CMD_STATE_W85 = 0x10, // following will be executed one by one till END
	INV_CMD_STATE_BN,
	INV_CMD_STATE_TN,
	INV_CMD_STATE_SN,
	
	INV_CMD_STATE_WB5,
	INV_CMD_STATE_WC4,
	INV_CMD_STATE_WD3,
	INV_CMD_STATE_WE2,
	INV_CMD_STATE_WF1,
	INV_CMD_STATE_W100,
	INV_CMD_STATE_WC1,
	INV_CMD_STATE_WF7,
	INV_CMD_STATE_W01,
	INV_CMD_STATE_W0C,
	INV_CMD_STATE_W15,
	INV_CMD_STATE_END,  // state keeps going the next state until it reach this end
	
	INV_CMD_STATE_SCAN_RUN = 0x101,
	INV_CMD_STATE_SCAN_END,
	// following is useless
	INV_CMD_STATE_W17,
	INV_CMD_STATE_W12,
	INV_CMD_STATE_W56,
	INV_CMD_STATE_W86,
	INV_CMD_STATE_W83	
	// End of useless
	
}inv_cmd_state_t;


typedef enum{
	INV_CMD_EVENT_NORMAL = 0,
	INV_CMD_EVENT_TIMEOUT = 0x2,
	INV_CMD_EVENT_ERR = 0x4,
	INV_CMD_EVENT_STOP = 0x8,
	INV_CMD_EVENT_END = 0x10,
	INV_CMD_EVENT_NOCHANGE = 0x20,
	INV_CMD_EVENT_SCAN = 0x40
}inv_cmd_event_t;

typedef struct _Solar_GPIOReadReq				
{
	uint8_t			GroupId;
	uint8_t			OpCode;
	uint8_t			Length;
	uint8_t			Command;
}solar_GPIOReadReq_t;

typedef struct _Solar_GPIOReadRsp			   	
{
	uint8_t			GroupId;
	uint8_t			OpCode;
	uint8_t			Length;
	uint8_t			ReturnCode;
	uint8_t			Value;
}solar_GPIOReadRsp_t;

struct _Solar_GPIOWriteReq				
{
	uint8_t			GroupId;
	uint8_t			OpCode;
	uint8_t			Length;
	uint8_t			Address;
	uint8_t			Value;
};

typedef struct _Solar_GPIOWriteConf			
{
	uint8_t			GroupId;
	uint8_t			OpCode;
	uint8_t			Length;
	uint8_t			ReturnCode;
}solar_GPIOWriteConf_t;

struct _Solar_EnergyReadReq
{
	uint8_t GroupId;
	uint8_t OpCode;
	uint8_t Length;
	uint8_t Command;
};

typedef struct _Solar_EnergyReadRsp
{
	uint8_t GroupId;
	uint8_t OpCode;
	uint8_t Length;
	inverter_msg2host_t meta_data;
	//inverter_info_t meta_data;
}solar_EnergyReadRsp_t;

typedef struct _Solar_VideoCtrlReq				
{
	uint8_t GroupId;
	uint8_t OpCode;
	uint8_t Length;
	uint8_t Command;
}solar_VideoCtrlReq_t;

typedef struct _Solar_VideoCtrlConf			
{
	uint8_t			GroupId;
	uint8_t			OpCode;
	uint8_t			Length;
	uint8_t			ReturnCode;
}solar_VideoCtrlConf_t;


struct _Solar_StatusInd
{
	uint8_t GroupId;
	uint8_t OpCode;
	uint8_t Length;
	status_ind_t ind_data;
};

struct _Basic
{
	uint8_t GroupId;
	uint8_t OpCode;
	uint8_t Length;	
};

union _UNI_ZTCFRAME
{
	struct _Basic								Solar_Basic;
	struct _Solar_GPIOReadReq				Solar_GPIOReadReq;
	struct _Solar_GPIOReadRsp			   	Solar_GPIOReadRsp;
	struct _Solar_GPIOWriteReq				Solar_GPIOWriteReq;
	struct _Solar_GPIOWriteConf			Solar_GPIOWriteConf;
	struct _Solar_EnergyReadRsp			Solar_EnergyReadRsp;
	struct _Solar_VideoCtrlReq				Solar_VideoCtrlReq;
	struct _Solar_VideoCtrlConf			Solar_VideoCtrlConf;
//	struct _Solar_AudioCtrlReq				Solar_AudioCtrlReq;
//	struct _Solar_AudioCtrlConf			Solar_AudioCtrlConf;
//	struct _Solar_EnergyReadReq			Solar_EnergyReadReq;
//	struct _Solar_EnergyReadRsp			Solar_EnergyReadRsp;
//	struct _Solar_EnergyResetReq			Solar_EnergyResetReq;
//	struct _Solar_EnergyResetRsp			Solar_EnergyResetRsp;
//	struct _Solar_InverterCtrlReq			Solar_InverterCtrlReq;
//	struct _Solar_InverterCtrlRsp			Solar_InverterCtrlRsp;
//	struct _Solar_SysCtrlReq				Solar_SysCtrlReq;
//	struct _Solar_SysCtrlRsp				Solar_SysCtrlRsp;
//	struct _Solar_SysCfgReadReq			Solar_SysCfgReadReq;
//	struct _Solar_SysCfgReadRsp			Solar_SysCfgReadRsp;
	struct _Solar_StatusInd			Solar_StatusInd;
};

#define WB_PKG_ANONYMOUS_SIZE (ZTC_DATAFRAME_BUFSIZE+ZTC_DATAFRAME_NAME_LEN+4)

typedef struct _RAW_PKG{
	uint8_t sync;
	uint8_t len;
	union _UNI_ZTCFRAME uniFrame;
	uint8_t FCS;
}raw_pkg_t;


enum _RAW_PKG_TYPE{
	WB_PKG_TYPE_FIRST=0,

	WB_PKG_TYPE_SOLAR_GPIO_READ_REQ,
	WB_PKG_TYPE_SOLAR_GPIO_READ_RSP,
	WB_PKG_TYPE_SOLAR_GPIO_WRITE_REQ,
	WB_PKG_TYPE_SOLAR_GPIO_WRITE_CONF,

	WB_PKG_TYPE_SOLAR_VIDEO_REQ,
	WB_PKG_TYPE_SOLAR_VIDEO_CONF,
	WB_PKG_TYPE_SOLAR_AUDIO_REQ,
	WB_PKG_TYPE_SOLAR_ADUIO_CONF,

	WB_PKG_TYPE_SOLAR_ENERGYREAD_REQ,
	WB_PKG_TYPE_SOLAR_ENERGYREAD_RSP,
	WB_PKG_TYPE_SOLAR_ENERGYRESET_REQ,
	WB_PKG_TYPE_SOLAR_ENERGYRESET_CONF,
	
	WB_PKG_TYPE_SOLAR_STATUS_IND,
	
	WB_PKG_TYPE_LAST,
};


enum FRAMEFORMAT_LOC{
FRAMEFORMAT_LOC_SYNC=0,
//FRAMEFORMAT_LOC_SN,
//FRAMEFORMAT_LOC_PKT_TYPE,
//FRAMEFORMAT_LOC_LEN_H=3,
//FRAMEFORMAT_LOC_LEN_L,
FRAMEFORMAT_LOC_LEN,
FRAMEFORMAT_LOC_API_ID,
FRAMEFORMAT_LOC_OPCODE,
FRAMEFORMAT_LOC_DATA_LEN,
FRAMEFORMAT_LOC_DATA_CMD_RTN,
//FRAMEFORMAT_LOC_DATAFIELD_START
//FRAMEFORMAT_LOC_LENGTH=
};

enum RETURN_CODE{
	RETURN_CODE_SUCCESS=0,
	RETURN_CODE_ERROR,
	RETURN_CODE_ERROR_WITH
};

enum GROUP_TYPE{
	OPGROUP_GPIO = 0x31,
	OPGROUP_VIDEO,
	OPGROUP_AUDIO,
	OPGROUP_INVERTER,
	OPGROUP_SYSTEM
};

enum CMD_GROUP {
	CMD_GPIOREAD = 0x01,
	CMD_GPIOWRITE,
	CMD_VIDEOCTRL,
	CMD_AUDIOCTRL,
	CMD_ENERGYREAD,
	CMD_ENERGYRESET,
	CMD_INVERTERCTRL,	
	CMD_SYSCTRL,
	CMD_SYSCFGREAD,
	CMD_SEC_ALARM,
	CMD_STATUS
};

enum OPCODE_TYPE {
	OPTYPE_REQ = 0x01,
	OPTYPE_CONF,
	OPTYPE_RSP,
	OPTYPE_IND
};

#define	OPCODE_GPIOREAD_REQ 	 (CMD_GPIOREAD<<4)|OPTYPE_REQ
#define	OPCODE_GPIOREAD_RSP 	 (CMD_GPIOREAD<<4)|OPTYPE_RSP
#define	OPCODE_GPIOWRITE_REQ 	 (CMD_GPIOWRITE<<4)|OPTYPE_REQ
#define	OPCODE_GPIOWRITE_CONF 	 (CMD_GPIOWRITE<<4)|OPTYPE_CONF
#define	OPCODE_SEC_ALARM_IND 	 (CMD_SEC_ALARM<<4)|OPTYPE_IND

#define	OPCODE_VIDEOCTRL_REQ 	 (CMD_VIDEOCTRL<<4)|OPTYPE_REQ
#define	OPCODE_VIDEOCTRL_CONF 	 (CMD_VIDEOCTRL<<4)|OPTYPE_CONF

#define	OPCODE_AUDIOCTRL_REQ 	 (CMD_AUDIOCTRL<<4)|OPTYPE_REQ
#define	OPCODE_AUDIOCTRL_CONF 	 (CMD_AUDIOCTRL<<4)|OPTYPE_CONF

#define	OPCODE_ENERGYREAD_REQ 	 (CMD_ENERGYREAD<<4)|OPTYPE_REQ
#define	OPCODE_ENERGYREAD_RSP 	 (CMD_ENERGYREAD<<4)|OPTYPE_RSP
#define	OPCODE_ENERGYRESET_REQ 	 (CMD_ENERGYRESET<<4)|OPTYPE_REQ
#define	OPCODE_ENERGYRESET_CONF  (CMD_ENERGYRESET<<4)|OPTYPE_CONF
#define	OPCODE_INVERTERCTRL_REQ  (CMD_INVERTERCTRL<<4)|OPTYPE_REQ
#define	OPCODE_INVERTERCTRL_CONF (CMD_INVERTERCTRL<<4)|OPTYPE_CONF

#define	OPCODE_SYSCTRL_REQ 		 (CMD_SYSCTRL<<4)|OPTYPE_REQ
#define	OPCODE_SYSCTRL_CONF 	 (CMD_SYSCTRL<<4)|OPTYPE_CONF
#define	OPCODE_SYSCFGREAD_REQ 	 (CMD_SYSCFGREAD<<4)|OPTYPE_REQ
#define	OPCODE_SYSCFGREAD_RSP 	 (CMD_SYSCFGREAD<<4)|OPTYPE_RSP

#define OPCODE_STATUS_IND		(CMD_STATUS<<4)|OPTYPE_IND

enum CALL_THREAD {
	THREAD_GPIO=1,
	THREAD_VIDEO,
	THREAD_AUDIO,
	THREAD_UART,
	THREAD_MAIN
};

enum VIDEO_CMD {
	VIDEO_CMD_START = 1,
	VIDEO_CMD_STOP,
	VIDEO_CMD_RESTART
};

typedef enum{
	INV_MATRIX_MOTECH = 1,
	INV_MATRIX_DELTA = 2,
	INV_MATRIX_EVERSOLAR = 3,
	INV_MATRIX_ALI = 4
}gInvType_t;




#endif

