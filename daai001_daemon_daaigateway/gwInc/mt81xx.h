#ifndef _MT81XX_PROT
#define _MT81XX_PROT

#include "ZtcTypedef.h"

enum {
	MT81ADDR_FacH_TRIP=0x01,
	MT81ADDR_FacH_CYCLE,
	MT81ADDR_FacL_TRIP,
	MT81ADDR_FacL_CYCLE,
	MT81ADDR_VacH_TRIP,
	MT81ADDR_VacH_CYCLE,
	MT81ADDR_VacL_TRIP,
	MT81ADDR_VacL_CYCLE,
	MT81ADDR_DELTA_Zac_TRIP=0x09,
	MT81ADDR_Zac_TRIP,
	MT81ADDR_FastIearth_TRIP,
	MT81ADDR_SlowIearth_TRIP,
	MT81ADDR_Riso_TRIP,
	MT81ADDR_Vpv_START,
	MT81ADDR_ONGRID_DELAY,
	MT81ADDR_VacH_LIMIT,
	MT81ADDR_VacH_LIMIT_CYCLE,
	MT81ADDR_Type_No=0x12,
	MT81ADDR_ADDRESS,
	MT81ADDR_BAUDRATE,
	MT81ADDR_LANGUAGE,
	
	MT81ADDR_Start=0x89,
	MT81ADDR_State=0xb5,
	MT81ADDR_Error_Code1,
	MT81ADDR_Error_Code2,
	MT81ADDR_Error_Code3,
	MT81ADDR_Error_Code4,
	MT81ADDR_Vpv_A=0xba,
	MT81ADDR_Vpv_B,
	MT81ADDR_Vpv_C,
	MT81ADDR_Ppv_A,
	MT81ADDR_Ppv_B,
	MT81ADDR_Ppv_C,
	MT81ADDR_Vac=0xc0,
	MT81ADDR_Pac,
	MT81ADDR_Iac,
	MT81ADDR_Fac,
	MT81ADDR_Eac_H=0xc4,
	MT81ADDR_Eac_L,
	MT81ADDR_EpvA_H,
	MT81ADDR_EpvA_L,
	MT81ADDR_EpvB_H,
	MT81ADDR_EpvB_L,
	MT81ADDR_EpvC_H,
	MT81ADDR_EpvC_L,
	MT81ADDR_Ton_today=0xcc,
	MT81ADDR_Ires,
	MT81ADDR_Heatsink_Temp,
	MT81ADDR_Zac,
	MT81ADDR_Riso,
	MT81ADDR_Ton_total_Hr=0xd1,
	MT81ADDR_Ton_total_Min,
	MT81ADDR_Ton_total_Sec,
	MT81ADDR_Relay_Turn_On_Times_H,
	MT81ADDR_Relay_Turn_On_Times_L,
	MT81ADDR_Vac_TRIP,
	MT81ADDR_Fac_TRIP,
	MT81ADDR_MODEL_NAME=0x109,
	MT81ADDR_SN_HIGH,
	MT81ADDR_SN_LOW,
	MT81ADDR_DEVICE_VER,
	MT81ADDR_Version_SEQU,
	MT81ADDR_Version_CURR,
	MT81ADDR_CALI_REQ_FLG=0x39b,
	MT81ADDR_WRITE_EN,
	MT81ADDR_MODEL_EN_KEY=0x3a9
};

//need password 1
enum{
	MT81ADDR_PL1_Type_No=0x12,
	MT81ADDR_PL1_Reset=0x39d
};

//need password 2
enum{
	MT81ADDR_PL2_BridgeRelay_ON_NumH=0x19,
	MT81ADDR_PL2_BridgeRelay_ON_NumL,
	MT81ADDR_PL2_TIME_HR_CNT,
	MT81ADDR_PL2_TIME_MIN_CNT,
	MT81ADDR_PL2_TIME_SEC_CNT,
	MT81ADDR_PL2_Eac_H=0x1e,
	MT81ADDR_PL2_Eac_L,
	MT81ADDR_PL2_EpvA_H=0x21,
	MT81ADDR_PL2_EpvA_L,
	MT81ADDR_PL2_EpvB_H=0x24,
	MT81ADDR_PL2_EpvB_L,
	MT81ADDR_PL2_EpvC_H=0x27,
	MT81ADDR_PL2_EpvC_L,
	MT81ADDR_PL2_Zac_OFF_SW=0x57
};

typedef enum {
	mt81xx_cmd_read = 0x03,
	mt81xx_cmd_write = 0x06,
	mt81xx_cmd_485 = 0x08,
	mt81xx_cmd_6C = 0x6c,
	mt81xx_cmd_AA = 0xaa,
	mt81xx_cmd_0B = 0x0b
}mt81xx_cmd_type_t;

// read 
typedef struct mt81xx_format03{
	uint8_t mt81xx_offset_StartByte;
	uint8_t mt81xx_offset_SlaveAddr;
	uint8_t mt81xx_offset_Function;
	uint8_t mt81xx_offset_SA_HiByte; 
	uint8_t mt81xx_offset_SA_LoByte;
	uint8_t mt81xx_offset_NO_HiByte;
	uint8_t mt81xx_offset_NO_LoByte;
	uint8_t mt81xx_offset_CS_LoByte;
	uint8_t mt81xx_offset_CS_HiByte;
	uint8_t mt81xx_offset_StopByte;
}mt81xx_format03_t;

// write
struct mt81xx_format06{
	uint8_t mt81xx_offset_StartByte;
	uint8_t mt81xx_offset_SlaveAddr;
	uint8_t mt81xx_offset_Function;
	uint8_t mt81xx_offset_RA_HiByte;
	uint8_t mt81xx_offset_RA_LoByte;
	uint8_t mt81xx_offset_Data_HiByte;
	uint8_t mt81xx_offset_Data_LoByte;
	uint8_t mt81xx_offset_CS_LoByte;
	uint8_t mt81xx_offset_CS_HiByte;
	uint8_t mt81xx_offset_StopByte;
};

// rs485
struct mt81xx_format08{
	uint8_t mt81xx_offset_StartByte;
	uint8_t mt81xx_offset_SlaveAddr;
	uint8_t mt81xx_offset_Function;
	uint8_t mt81xx_offset_MO_HiByte;
	uint8_t mt81xx_offset_MO_LoByte;
	uint8_t mt81xx_offset_SNH_HiByte;
	uint8_t mt81xx_offset_SNH_LoByte;
	uint8_t mt81xx_offset_SNL_HiByte;
	uint8_t mt81xx_offset_SNL_LoByte;
	uint8_t mt81xx_offset_NewAddr;
	uint8_t mt81xx_offset_CS_LoByte;
	uint8_t mt81xx_offset_CS_HiByte;
	uint8_t mt81xx_offset_StopByte;
};

struct mt81xx_format6C{
	uint8_t mt81xx_offset_StartByte;
	uint8_t mt81xx_offset_SlaveAddr;
	uint8_t mt81xx_offset_Function;
	uint8_t mt81xx_offset_RA_HiByte;
	uint8_t mt81xx_offset_RA_LoByte;
	uint8_t mt81xx_offset_Data_HiByte;
	uint8_t mt81xx_offset_Data_LoByte;
	uint8_t mt81xx_offset_CS_LoByte;
	uint8_t mt81xx_offset_CS_HiByte;
	uint8_t mt81xx_offset_StopByte;
};

struct mt81xx_formatAA{
	uint8_t mt81xx_offset_StartByte;
	uint8_t mt81xx_offset_SlaveAddr;
	uint8_t mt81xx_offset_Function;
	uint8_t mt81xx_offset_RA_HiByte;
	uint8_t mt81xx_offset_RA_LoByte;
	uint8_t mt81xx_offset_Data_HiByte;
	uint8_t mt81xx_offset_Data_LoByte;
	uint8_t mt81xx_offset_CS_LoByte;
	uint8_t mt81xx_offset_CS_HiByte;
	uint8_t mt81xx_offset_StopByte;
};

// broadcast
struct mt81xx_format0B{
	uint8_t mt81xx_offset_StartByte;
	uint8_t mt81xx_offset_SlaveAddr;
	uint8_t mt81xx_offset_Function;
	uint8_t mt81xx_offset_RA_HiByte;
	uint8_t mt81xx_offset_RA_LoByte;
	uint8_t mt81xx_offset_Data_HiByte;
	uint8_t mt81xx_offset_Data_LoByte;
	uint8_t mt81xx_offset_CS_LoByte;
	uint8_t mt81xx_offset_CS_HiByte;
	uint8_t mt81xx_offset_StopByte;
};

typedef struct mt81xx_format_rtn{
	uint8_t mt81xx_offset_StartByte;
	uint8_t mt81xx_offset_SlaveAddr;
	uint8_t mt81xx_offset_Function;
	uint8_t mt81xx_offset_Len;
	uint8_t *rest;
}mt81xx_format_rtn_t;


union _UNI_485FRAME{
	struct mt81xx_format03 im_mt81xx_format03;
	struct mt81xx_format06 im_mt81xx_format06;
	struct mt81xx_format08 im_mt81xx_format08;
	struct mt81xx_format6C im_mt81xx_format6C;
	struct mt81xx_formatAA im_mt81xx_formatAA;
	struct mt81xx_format0B im_mt81xx_format0B;
	struct mt81xx_format_rtn im_mt81xx_format_rtn;
};

typedef struct _RAW_485{
	mt81xx_cmd_type_t pkt_type;
	union _UNI_485FRAME uniFrame;
}raw_485_t;

typedef struct rs485_rx_desc{
	int fd;
	uint32_t wait_size;
	uint8_t * buffer;
}rx485_rx_desc_t;

enum rs485_rtn_code {
	rs485_ok = 0x00,
	rs485_err_size,
	rs485_err_timeout
};

extern int append_CRC16(raw_485_t * FrameBuffer);
extern int check_CRC16(mt81xx_format_rtn_t * FrameBuffer);

extern int create_mt81xx_cmd(raw_485_t* FrameBuffer, uint16_t dev_addr_plusNew, uint16_t reg, uint32_t size);

extern uint8_t read_rs485_cmd(uint8_t inverter_addr, uint16_t reg_addr, uint8_t size, uint8_t* buff4rd);
extern uint8_t write_rs485_cmd(uint8_t inverter_addr, uint16_t reg_addr, uint8_t size, uint8_t* data2wr);

#endif

