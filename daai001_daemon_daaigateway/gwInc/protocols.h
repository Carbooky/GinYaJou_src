#ifndef _PROTOCOLS_H
#define _PROTOCOLS_H

#include <termios.h>
#include "ZtcTypedef.h"
#include "format.h"
#include "util.h"

#define INV_STATUS_WAIT 	0    // amber
#define INV_STATUS_NORMAL	1    // green
#define INV_STATUS_FAULT	2    // red
#define INV_STATUS_UNKNOWN	9    // black

/****************************
** PYRANOMETER protocol
****************************/

typedef struct inverter_pyranometer_raw_struct{
	uint16_t rsvd00;
	uint16_t rsvd01;
	uint16_t rsvd02;
	uint16_t dPHi;			// 03H
	uint16_t dpLo;			// 04H
	uint16_t rsvd05;
	uint16_t rsvd06;
	uint16_t disp_value;
	uint16_t dot;			// 08H
	uint16_t rsvd09;
	uint16_t rsvd0a;
	uint16_t rsvd0b;
	uint16_t rsvd0c;
	uint16_t under;			// 0DH
	uint16_t avg;			// 0EH
	uint16_t baud;			// 0FH
	uint16_t addr;			// 10H
	uint16_t frae;			// 11H
	uint16_t rsvd12;
	uint16_t rsvd13;
	uint16_t rsvd14;
	uint16_t rsvd15;
	uint16_t rs485case;		// 16H
	uint16_t rsvd17;
	uint16_t disp_adj;		// 18H
	uint16_t disp_HiWd;		// 1000H
	uint16_t disp_LoWd;		// 1001H
}inverter_pyranometer_raw_t;


typedef enum{
	
	INV_PYRANOMETER_STATE_INIT=0,
	INV_PYRANOMETER_STATE_RDY,
	INV_PYRANOMETER_STATE_TIMEOUT,
	INV_PYRANOMETER_STATE_ERR,
	INV_PYRANOMETER_STATE_STOP,
	INV_PYRANOMETER_STATE_1ST = 0x10,
	INV_PYRANOMETER_STATE_2ND,
	INV_PYRANOMETER_STATE_3ST,
	INV_PYRANOMETER_STATE_END,

	INV_PYRANOMETER_STATE_SCAN_RUN = 0x101,
	INV_PYRANOMETER_STATE_SCAN_END
	
}inv_pyranometer_state_t;

typedef struct InvPyranometerRegsMatrix{
	inv_pyranometer_state_t state_num;		// state number
	uint16_t reg_addr;				// reg address to be read
	uint16_t* ptr_addr;				// where to store this return value
	uint8_t cpy_size;				// read size in unit Byte, 
									// while ModBus count in Word containing 2 Bytes 
}inv_pyranometer_regsmatrix_t;


typedef struct inverter_msg2host_pyranometer_struct{
	uint16_t inverterId;
	char value[8];
//	uint32_t dpValue;
//	uint16_t disp_adj;
}inverter_msg2host_pyranometer_t;

typedef struct inverter_msg2host_thermometer_struct{
	uint16_t inverterId;
	char value[8];
}inverter_msg2host_thermometer_t;

typedef struct inverter_msg2host_jda_struct{
	uint16_t inverterId;
	char value[8];
}inverter_msg2host_jda_t;

/****************************
** MOTECH Inverter protocol
****************************/
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
	mt81xx_cmd_0B = 0x0b,
	eversolar_cmd_reg = 0x11,
	eversolar_cmd_reg_alloc,
	eversolar_cmd_rd,
	eversolar_cmd_wr,
	eversolar_cmd_exe,
}cmd_type_t;

// modbus
typedef struct modbus_format_read{
	uint8_t modbus_offset_SlaveAddr;
	uint8_t modbus_offset_Function;
	uint8_t modbus_offset_SA_HiByte; 
	uint8_t modbus_offset_SA_LoByte;
	uint8_t modbus_offset_NO_HiByte;
	uint8_t modbus_offset_NO_LoByte;
	uint8_t modbus_offset_CS_LoByte;
	uint8_t modbus_offset_CS_HiByte;
}modbus_format_read_t;

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

typedef struct modbus_format_rtn{
	uint8_t modbus_offset_SlaveAddr;
	uint8_t modbus_offset_Function;
	uint8_t modbus_offset_Len;
	uint8_t modbus_offset_data_base;
}modbus_format_rtn_t;

// used in CRC check
typedef struct mt81xx_format_rtn{
	uint8_t mt81xx_offset_StartByte;
	uint8_t mt81xx_offset_SlaveAddr;
	uint8_t mt81xx_offset_Function;
	uint8_t mt81xx_offset_Len;
	uint8_t mt81xx_offset_data_base;
}mt81xx_format_rtn_t;

struct eversolar_format_reg{
	uint8_t eversolar_offset_header_L;
	uint8_t eversolar_offset_header_H;
	uint8_t eversolar_offset_src_L;
	uint8_t eversolar_offset_src_H;
	uint8_t eversolar_offset_dst_L;
	uint8_t eversolar_offset_dst_H;
	uint8_t eversolar_offset_ctrlCode;
	uint8_t eversolar_offset_funCode;
	uint8_t eversolar_offset_len;
	uint8_t eversolar_offset_FCS_L;
	uint8_t eversolar_offset_FCS_H;
};

struct eversolar_format_reg_alloc{
	uint8_t eversolar_offset_header_L;
	uint8_t eversolar_offset_header_H;
	uint8_t eversolar_offset_src_L;
	uint8_t eversolar_offset_src_H;
	uint8_t eversolar_offset_dst_L;
	uint8_t eversolar_offset_dst_H;
	uint8_t eversolar_offset_ctrlCode;
	uint8_t eversolar_offset_funCode;
	uint8_t eversolar_offset_len;
	uint8_t eversolar_offset_data00;
	uint8_t eversolar_offset_data01;
	uint8_t eversolar_offset_data02;
	uint8_t eversolar_offset_data03;
	uint8_t eversolar_offset_data04;
	uint8_t eversolar_offset_data05;
	uint8_t eversolar_offset_data06;
	uint8_t eversolar_offset_data07;
	uint8_t eversolar_offset_data08;
	uint8_t eversolar_offset_data09;
	uint8_t eversolar_offset_data10;
	uint8_t eversolar_offset_data11;
	uint8_t eversolar_offset_data12;
	uint8_t eversolar_offset_data13;
	uint8_t eversolar_offset_data14;
	uint8_t eversolar_offset_data15;
	uint8_t eversolar_offset_alloc;
	uint8_t eversolar_offset_FCS_L;
	uint8_t eversolar_offset_FCS_H;
};

struct eversolar_format_rd{
	uint8_t eversolar_offset_header_L;
	uint8_t eversolar_offset_header_H;
	uint8_t eversolar_offset_src_L;
	uint8_t eversolar_offset_src_H;
	uint8_t eversolar_offset_dst_L;
	uint8_t eversolar_offset_dst_H;
	uint8_t eversolar_offset_ctrlCode;
	uint8_t eversolar_offset_funCode;
	uint8_t eversolar_offset_len;
	uint8_t eversolar_offset_FCS_L;
	uint8_t eversolar_offset_FCS_H;
};

union _UNI_485FRAME{
	struct mt81xx_format03 im_mt81xx_format03;
	struct mt81xx_format06 im_mt81xx_format06;
	struct mt81xx_format08 im_mt81xx_format08;
	struct mt81xx_format6C im_mt81xx_format6C;
	struct mt81xx_formatAA im_mt81xx_formatAA;
	struct mt81xx_format0B im_mt81xx_format0B;
	struct mt81xx_format_rtn im_mt81xx_format_rtn;
	struct modbus_format_read im_modbus_format_read;
	struct eversolar_format_reg im_eversolar_format_reg;
	struct eversolar_format_reg_alloc im_eversolar_format_reg_alloc;
	struct eversolar_format_rd im_eversolar_format_rd;
};

typedef struct _RAW_485{
	cmd_type_t pkt_type;
	union _UNI_485FRAME uniFrame;
}raw_485_t;

typedef struct InvMotechRegsMatrix{
	inv_cmd_state_t state_num;		// state number
	uint16_t reg_addr;				// reg address to be read
	//uint16_t* ptr_addr;				// where to store this return value
	uint32_t offset;
	uint8_t cpy_size;				// read size in unit Byte, 
									// while ModBus count in Word containing 2 Bytes 
}inv_motech_regsmatrix_t;

int update_inv_msg2host_pyranometer(inverter_msg2host_pyranometer_t *msg2host, inverter_pyranometer_raw_t *inverter_info, uint8_t invId);
//int invInfoGet_pyranometer(gInverterAddr_t invAddr, inverter_msg2host_pyranometer_t *data, char* scan_port_name);
int convert_invmsg_to_asci_pyranometer(char *buff, inverter_msg2host_pyranometer_t* msg2host);
int convert_invmsg_to_asci_thermometer(char *buff, inverter_msg2host_thermometer_t* msg2host);
int convert_invmsg_to_asci_thermograph(char *buff, inverter_msg2host_thermometer_t* msg2host);

int append_motech_CRC16(raw_485_t * FrameBuffer);
int append_CRC16(modbus_format_read_t * FrameBuffer, int in_cal_size);
int check_motech_CRC16(mt81xx_format_rtn_t * FrameBuffer);
int check_CRC16(modbus_format_rtn_t * FrameBuffer);

int create_modbus_read(void* FrameBuffer, uint8_t dev_addr, uint16_t reg, uint8_t size);
int create_mt81xx_cmd(raw_485_t* FrameBuffer, uint16_t dev_addr_plusNew, uint16_t reg, uint32_t size);

uint8_t read_rs485_cmd(uint8_t inverter_addr, uint16_t reg_addr, uint8_t size, uint8_t* buff4rd);
uint8_t write_rs485_cmd(uint8_t inverter_addr, uint16_t reg_addr, uint8_t size, uint8_t* data2wr);

int update_inv_msg2host_motech(inverter_msg2host_new_t *msg2host, inverter_info_t *inverter_info, uint16_t invId);
int convert_invmsg_to_asci_motech(char* ,inverter_msg2host_new_t*);

int inverter_scan_motech(scan_inv_info_t* inv_info, char* scan_port_name);
int invInfoGet_Motech(uint16_t invAddr, inverter_msg2host_new_t *data, char* scan_port_name);
int inv_state_chg_next(inv_cmd_state_t* curr_inv_cmd_state, inv_cmd_event_t in_event);

/****************************
** DELTA Inverter protocol
****************************/
#define PROTOCOL_TYPE_5K 5
#define PROTOCOL_TYPE_20K 20

#define ZEVERSOLAR_DEV_TYPE_NON_ISO_3P  0x33

typedef struct delta_format04{
	uint8_t delta_offset_SlaveAddr;
	uint8_t delta_offset_Function;
	uint8_t delta_offset_SA_HiByte; 
	uint8_t delta_offset_SA_LoByte;
	uint8_t delta_offset_NO_HiByte;
	uint8_t delta_offset_NO_LoByte;
	uint8_t delta_offset_CS_LoByte;
	uint8_t delta_offset_CS_HiByte;
}delta_format04_t;

typedef struct delta_format06{
	uint8_t delta_offset_SlaveAddr;
	uint8_t delta_offset_Function;
	uint8_t delta_offset_RA_HiByte;
	uint8_t delta_offset_RA_LoByte;
	uint8_t delta_offset_Data_HiByte;
	uint8_t delta_offset_Data_LoByte;
	uint8_t delta_offset_CS_LoByte;
	uint8_t delta_offset_CS_HiByte;
}delta_format06_t;

typedef struct delta_format_rtn{
	uint8_t delta_offset_SlaveAddr;
	uint8_t delta_offset_Function;
	uint8_t delta_offset_ByteCount;
}delta_format_rtn_t;


typedef struct inverter_delta_raw_struct{
	//address 1032
	uint16_t inverterId;
	uint16_t rtc_Year;
	uint16_t rtc_Month;
	uint16_t rtc_Day;
	uint16_t rtc_Hour;
	uint16_t rtc_Minute;
	uint16_t rtc_Second;
	
	//address 1040
	uint16_t fw1Revision;
	uint16_t fw1Data;
	uint16_t fw2Revision;
	uint16_t fw2Data;
	uint16_t fw3Revision;
	uint16_t fw3Data;
	uint16_t fw4Revision;
	uint16_t fw4Data;
	
	// address 1056 for AC
	uint16_t AC_measurementIndex;
	uint16_t AC_voltage;
	uint16_t AC_current;
	uint16_t AC_wattage;
	uint16_t AC_frequency;
	uint16_t AC_percentage;
	uint16_t AC_redundantVoltage;
	uint16_t AC_redundantFrequency;
	uint16_t AC_rsvd_1064;
	uint16_t AC_rsvd_1065;
	uint16_t AC_rsvd_1066;
	uint16_t AC_rsvd_1067;
	uint16_t AC_adcVoltage;
	uint16_t AC_adcCurrent;
	uint16_t AC_adcWattage;
	uint16_t AC_adcRedundantVoltage;

	//address 1048
	uint16_t inverterStatus;
	uint16_t reconnectedTime;
	uint16_t Device_Type;
	uint16_t DC_rsvd_1051;
	uint16_t DC_rsvd_1052;
	uint16_t DC_rsvd_1053;
	uint16_t DC_rsvd_1054;
	uint16_t DC_rsvd_1055;
	// address 1056 for DC
	uint16_t DC_measurementIndex;
	uint16_t DC_voltage;
	uint16_t DC_current;
	uint16_t DC_wattage;
	uint16_t DC_frequency;
	uint16_t DC_percentage;
	uint16_t DC_redundantVoltage;
	uint16_t DC_redundantFrequency;
	uint16_t DC_rsvd_1064;
	uint16_t DC_rsvd_1065;
	uint16_t DC_rsvd_1066;
	uint16_t DC_rsvd_1067;	
	uint16_t DC_adcVoltage;
	uint16_t DC_adcCurrent;
	uint16_t DC_adcWattage;
	uint16_t DC_adcRedundantVoltage;

	uint16_t todayWh_Low;
	uint16_t todayWh_High;
	uint16_t todayRuntime_Low;
	uint16_t todayRuntime_High;
	uint16_t lifeWh_Low;
	uint16_t lifeWh_High;
	uint16_t lifeRuntime_Low;
	uint16_t lifeRuntime_High;
	
	// address 1080
	uint16_t delta5k_AmbTemp;
	uint16_t delta5k_BoostHsTemp;
	uint16_t delta5k_InvHsTemp;
	uint16_t Temp4;
	
	// address 1088
	uint16_t eventIndex;
	uint16_t eventOff0;
	uint16_t eventOff1;
	uint16_t eventOff2;
	uint16_t eventOff3;
	uint16_t eventOff4;
	uint16_t eventOff5;
	uint16_t eventOff6;
	uint16_t eventOff7;
	uint16_t eventOff8;
	uint16_t eventOff9;
	
	// address 40975
	uint16_t serialNumber01;
	uint16_t serialNumber23;
	uint16_t serialNumber45;
	uint16_t serialNumber67;
	uint16_t serialNumber89;
	uint16_t serialNumber1011;
	
	// address 1152
	uint16_t delta5k_MaxAmbTemp;
	uint16_t delta5k_MaxBoostHsTemp;
	uint16_t delta5k_MaxMaxInvHsTemp;
	
	// address 1160
	uint16_t delta5k_Solar1_RLeakage1;
	uint16_t delta5k_Solar1_RLeakage2;
	uint16_t delta5k_Solar1_RLeakage3;
	uint16_t delta5k_Solar1_RLeakageAvg;
	uint16_t delta5k_Adins_adc;

	uint16_t AC_a_voltage;
	uint16_t AC_a_current;
	uint16_t AC_a_wattage;
	uint16_t AC_a_frequency;
	uint16_t AC_a_percentage;
	uint16_t AC_a_redundantVoltage;
	uint16_t AC_a_redundantFrequency;
	uint16_t AC_a_rsvd_1064;
	uint16_t AC_a_rsvd_1065;
	uint16_t AC_a_rsvd_1066;
	uint16_t AC_a_rsvd_1067;
	uint16_t AC_a_adcVoltage;
	uint16_t AC_a_adcCurrent;
	uint16_t AC_a_adcWattage;
	uint16_t AC_a_adcRedundantVoltage;

	uint16_t AC_b_voltage;
	uint16_t AC_b_current;
	uint16_t AC_b_wattage;
	uint16_t AC_b_frequency;
	uint16_t AC_b_percentage;
	uint16_t AC_b_redundantVoltage;
	uint16_t AC_b_redundantFrequency;
	uint16_t AC_b_rsvd_1064;
	uint16_t AC_b_rsvd_1065;
	uint16_t AC_b_rsvd_1066;
	uint16_t AC_b_rsvd_1067;
	uint16_t AC_b_adcVoltage;
	uint16_t AC_b_adcCurrent;
	uint16_t AC_b_adcWattage;
	uint16_t AC_b_adcRedundantVoltage;

	uint16_t AC_c_voltage;
	uint16_t AC_c_current;
	uint16_t AC_c_wattage;
	uint16_t AC_c_frequency;
	uint16_t AC_c_percentage;
	uint16_t AC_c_redundantVoltage;
	uint16_t AC_c_redundantFrequency;
	uint16_t AC_c_rsvd_1064;
	uint16_t AC_c_rsvd_1065;
	uint16_t AC_c_rsvd_1066;
	uint16_t AC_c_rsvd_1067;
	uint16_t AC_c_adcVoltage;
	uint16_t AC_c_adcCurrent;
	uint16_t AC_c_adcWattage;
	uint16_t AC_c_adcRedundantVoltage;	

	uint16_t DC_1_voltage;
	uint16_t DC_1_current;
	uint16_t DC_1_wattage;
	uint16_t DC_1_frequency;
	uint16_t DC_1_percentage;
	uint16_t DC_1_redundantVoltage;
	uint16_t DC_1_redundantFrequency;
	uint16_t DC_1_rsvd_1064;
	uint16_t DC_1_rsvd_1065;
	uint16_t DC_1_rsvd_1066;
	uint16_t DC_1_rsvd_1067;	
	uint16_t DC_1_adcVoltage;
	uint16_t DC_1_adcCurrent;
	uint16_t DC_1_adcWattage;
	uint16_t DC_1_adcRedundantVoltage;

	uint16_t DC_2_voltage;
	uint16_t DC_2_current;
	uint16_t DC_2_wattage;
	uint16_t DC_2_frequency;
	uint16_t DC_2_percentage;
	uint16_t DC_2_redundantVoltage;
	uint16_t DC_2_redundantFrequency;
	uint16_t DC_2_rsvd_1064;
	uint16_t DC_2_rsvd_1065;
	uint16_t DC_2_rsvd_1066;
	uint16_t DC_2_rsvd_1067;	
	uint16_t DC_2_adcVoltage;
	uint16_t DC_2_adcCurrent;
	uint16_t DC_2_adcWattage;
	uint16_t DC_2_adcRedundantVoltage;	

	uint16_t CE_Err0;
	uint16_t CE_Err1;
	uint16_t CE_Err2;
	uint16_t CE_Err3;
	uint16_t CE_Err4;
	uint16_t CE_Err5;
	uint16_t CE_Err6;
	uint16_t CE_Err7;
	uint16_t CE_Err8;
	uint16_t CE_Err9;
	uint16_t CE_Err10;
	uint16_t CE_Err11;
	uint16_t CE_Err12;
	uint16_t CE_Err13;
	uint16_t CE_Err14;
	uint16_t CE_Err15;

	uint16_t CE_War0;
	uint16_t CE_War1;
	uint16_t CE_War2;
	uint16_t CE_War3;
	uint16_t CE_War4;
	uint16_t CE_War5;
	uint16_t CE_War6;
	uint16_t CE_War7;
	uint16_t CE_War8;
	uint16_t CE_War9;
	uint16_t CE_War10;
	uint16_t CE_War11;
	uint16_t CE_War12;
	uint16_t CE_War13;
	uint16_t CE_War14;
	uint16_t CE_War15;

	uint16_t CE_Fau0;
	uint16_t CE_Fau1;
	uint16_t CE_Fau2;
	uint16_t CE_Fau3;
	uint16_t CE_Fau4;
	uint16_t CE_Fau5;
	uint16_t CE_Fau6;
	uint16_t CE_Fau7;
	uint16_t CE_Fau8;
	uint16_t CE_Fau9;
	uint16_t CE_Fau10;
	uint16_t CE_Fau11;
	uint16_t CE_Fau12;
	uint16_t CE_Fau13;
	uint16_t CE_Fau14;
	uint16_t CE_Fau15;	

	//uint8_t serialNum[13];	// keep an extra byte for '\0'
	uint8_t serialNum[17];	// keep an extra byte for '\0'
	uint8_t use_protocol;

}inverter_delta_raw_t;

typedef enum{
	
	INV_DELTA_STATE_INIT=0,
	INV_DELTA_STATE_RDY,
	INV_DELTA_STATE_TIMEOUT,
	INV_DELTA_STATE_ERR,
	INV_DELTA_STATE_STOP,

	INV_DELTA_STATE_SET_BASIC= 0x10,
	INV_DELTA_STATE_GET_BASIC,
	INV_DELTA_STATE_CHECK_TYPE,

	INV_DELTA_STATE_SET_AC = 0x40, // following will be executed one by one till END
	INV_DELTA_STATE_GET_AC_DATA,
	INV_DELTA_STATE_SET_DC,
	INV_DELTA_STATE_GET_DC_DATA,
	INV_DELTA_STATE_SET_SN,
	INV_DELTA_STATE_GET_SN_DATA,	
	INV_DELTA_STATE_SET_EVNT1,
	INV_DELTA_STATE_GET_EVNT1,
	INV_DELTA_STATE_DELTA_END,
	// end of normal 5k

	INV_DELTA_STATE_SET_AC_A = 0x80,
	INV_DELTA_STATE_GET_AC_A_DATA,
	INV_DELTA_STATE_SET_AC_B,
	INV_DELTA_STATE_GET_AC_B_DATA,
	INV_DELTA_STATE_SET_AC_C,
	INV_DELTA_STATE_GET_AC_C_DATA,
	INV_DELTA_STATE_SET_DC_1,
	INV_DELTA_STATE_GET_DC_1_DATA,
	INV_DELTA_STATE_SET_DC_2,
	INV_DELTA_STATE_GET_DC_2_DATA,
	INV_DELTA_STATE_SET_SN_20K,
	INV_DELTA_STATE_GET_SN_20K,	
	INV_DELTA_STATE_SET_EMSG_20K,
	INV_DELTA_STATE_GET_EMSG_20K,
	INV_DELTA_STATE_SET_EVNT2,
	INV_DELTA_STATE_GET_EVNT2,	
	INV_DELTA_STATE_SET_SUM,
	INV_DELTA_STATE_GET_SUM,	
	INV_DELTA_STATE_DELTA_END_20K,
	// end of 20k

	// End of useless
	
	INV_DELTA_STATE_SCAN_RUN = 0x101,
	INV_DELTA_STATE_SCAN_REPEAT,
	INV_DELTA_STATE_SCAN_END
	
}inv_delta_state_t;

typedef struct InvDeltaRegsMatrix{
	inv_delta_state_t state_num;		// state number
	uint16_t reg_addr;				// reg address to be read
	//uint16_t* ptr_addr;				// where to store this return value
	uint32_t offset;
	uint8_t cpy_size;				// read size in unit Byte, 
									// while ModBus count in Word containing 2 Bytes 
	uint8_t wr_data;				// data to be written
}inv_delta_regsmatrix_t;

typedef struct inverter_msg2host_delta_struct{

	uint16_t inverterId;

	uint16_t inverterStatus;
	uint16_t reconnectedTime;

	// address 1056 for DC
	uint16_t DC_measurementIndex;
	uint16_t DC_voltage;
	uint16_t DC_current;
	uint16_t DC_wattage;
	uint16_t DC_frequency;
	uint16_t DC_percentage;
	uint16_t DC_redundantVoltage;
	uint16_t DC_redundantFrequency;

	uint16_t DC_adcVoltage;
	uint16_t DC_adcCurrent;
	uint16_t DC_adcWattage;
	uint16_t DC_adcRedundantVoltage;

	uint16_t todayWh_Low;
	uint16_t todayWh_High;
	uint16_t todayRuntime_Low;
	uint16_t todayRuntime_High;
	uint16_t lifeWh_Low;
	uint16_t lifeWh_High;
	uint16_t lifeRuntime_Low;
	uint16_t lifeRuntime_High;

	uint16_t delta5k_AmbTemp;
	uint16_t delta5k_BoostHsTemp;
	uint16_t delta5k_InvHsTemp;
	uint16_t Temp4;
	
	uint16_t AC_measurementIndex;
	uint16_t AC_voltage;
	uint16_t AC_current;
	uint16_t AC_wattage;
	uint16_t AC_frequency;
	uint16_t AC_percentage;
	uint16_t AC_redundantVoltage;
	uint16_t AC_redundantFrequency;

	uint16_t AC_adcVoltage;
	uint16_t AC_adcCurrent;
	uint16_t AC_adcWattage;
	uint16_t AC_adcRedundantVoltage;
	
	uint16_t eventIndex;
	uint16_t event0;
	uint16_t event1;
	uint16_t event2;
	uint16_t event3;
	uint16_t event4;
	uint16_t event5;
	uint16_t event6;
	uint16_t event7;
	uint16_t event8;
	uint16_t event9;

	uint16_t AC_A_voltage;
	uint16_t AC_A_current;
	uint16_t AC_A_wattage;
	uint16_t AC_A_frequency;
	uint16_t AC_A_redundantVoltage;
	uint16_t AC_A_redundantFrequency;
	uint16_t AC_A_adcVoltage;
	uint16_t AC_A_adcCurrent;
	uint16_t AC_A_adcWattage;
	uint16_t AC_A_adcRedundantVoltage;

	uint16_t AC_B_voltage;
	uint16_t AC_B_current;
	uint16_t AC_B_wattage;
	uint16_t AC_B_frequency;
	uint16_t AC_B_redundantVoltage;
	uint16_t AC_B_redundantFrequency;
	uint16_t AC_B_adcVoltage;
	uint16_t AC_B_adcCurrent;
	uint16_t AC_B_adcWattage;
	uint16_t AC_B_adcRedundantVoltage;

	uint16_t AC_C_voltage;
	uint16_t AC_C_current;
	uint16_t AC_C_wattage;
	uint16_t AC_C_frequency;
	uint16_t AC_C_redundantVoltage;
	uint16_t AC_C_redundantFrequency;
	uint16_t AC_C_adcVoltage;
	uint16_t AC_C_adcCurrent;
	uint16_t AC_C_adcWattage;
	uint16_t AC_C_adcRedundantVoltage;	

	uint16_t DC_1_voltage;
	uint16_t DC_1_current;
	uint16_t DC_1_wattage;
	uint16_t DC_1_adcVoltage;
	uint16_t DC_1_adcCurrent;
	uint16_t DC_1_adcWattage;

	uint16_t DC_2_voltage;
	uint16_t DC_2_current;
	uint16_t DC_2_wattage;
	uint16_t DC_2_adcVoltage;
	uint16_t DC_2_adcCurrent;
	uint16_t DC_2_adcWattage;

	uint16_t msg_errE15_E00;
	uint16_t msg_errE31_E16;
	uint16_t msg_errE47_E32;

	uint16_t msg_warW15_W00;

	uint16_t msg_fauF15_F00;
	uint16_t msg_fauF31_F16;
	uint16_t msg_fauF47_F32;
	uint16_t msg_fauF63_F48;
	uint16_t msg_fauF79_F64;

	uint16_t dev_type;
	//uint8_t serialNum[13];
	uint8_t serialNum[17];
	uint8_t use_protocol;

}inverter_msg2host_delta_t;

int check_modbus_CRC16(bool_t write_rsp, delta_format_rtn_t* FrameBuffer);
int append_modbus_CRC16(void* FrameBuffer);
int create_delta_cmd(void* FrameBuffer, bool_t cmd_wr, uint16_t dev_addr_plusNew, uint16_t reg, uint16_t data_and_size);
int update_inv_msg2host_delta(inverter_msg2host_delta_t *data, inverter_delta_raw_t *inverter_info, uint16_t invId);
int convert_invmsg_to_asci_delta(char *buff, inverter_msg2host_delta_t* msg2host);
int inverter_scan_delta(scan_inv_info_t* inv_info, char* scan_port_name);
int invInfoGet_Delta(uint16_t invAddr, inverter_msg2host_delta_t *data, char* scan_port_name);
int inv_delta_state_chg_next(inv_delta_state_t* curr_inv_cmd_state, inv_cmd_event_t in_event);

/****************************
** EVERSOLAR Inverter protocol
****************************/
/*
typedef struct inverter_eversolar_raw_struct{
	
	uint16_t inverterId;
	uint8_t sn[16];
	uint8_t alloc_addr;
// first phase of capture, 15 words a time
	//address 0xB4
	// reg addr start from 0x00
	uint16_t temperature;	// internal temperature	
	uint16_t Vpv1;			// PV1 voltage, uint 0.1V
	uint16_t Vpv2;			// PV2 voltage
	uint16_t NA1;			
	uint16_t Ipv1;			// PV1 Current, uint 0.1A
	uint16_t Ipv2;			// PV2 Current
	uint16_t NA2;		
	uint16_t E_Total_H;		// Total energy to grid (3phrase)
	uint16_t E_Total_L;		// uint 0.1kWH
	uint16_t h_Total_H;		// Total energy to grid (3phrase)
	uint16_t h_Total_L;		// uint Hr
	uint16_t Pac;			// Total power to grid, uint in W
	uint16_t Mode;			// 
	uint16_t E_Today;		// accumulated power of that day, unit in 0.01 kWH

	// reg addr starts from 0x20
	uint16_t surTemp;
	uint16_t bdTemp;
	uint16_t irr;
	uint16_t windSpeed;
	uint16_t Cos;
	uint16_t Phase;
	uint16_t WaitingTime;
	uint16_t TmpFaultValue;
	uint16_t PV1FaultValue;
	uint16_t PV2FaultValue;
	uint16_t NA15;
	uint16_t GFCIFaultValue;
	uint16_t Errormessage_H;
	uint16_t Errormessage_L;
}inverter_eversolar_raw_t;
*/

typedef struct inverter_msg2host_eversolar_struct{
	uint16_t inverterId;
	uint16_t PAC;
	char serialNum[20];
	char modelName[20];
	double PDC;
	double E_TODAY;
	double E_TOTAL;
	double VPV;		
	double VPV2;			
	double IPV;		
	double IPV2;
	double VAC;
	double IAC;
	double FREQUENCY;
	uint32_t HOURS;
	double TEMP;
	uint16_t MODE;
	uint32_t ErrWarMsg;

	double Iac_R_20K;  // 0x41
	double Vac_R_20K;  // 0x42
	double Fac_R_20K;  // 0x43

	double Iac_S_20K;  // 0x81
	double Vac_S_20K; 	// 0x82

	double Iac_T_20K;  // 0xc1
	double Vac_T_20K; 	// 0xc2

	uint16_t WaitingTime_20K; // 0x38
	uint16_t CosPhi_20K;	// 0x36
	uint16_t Phase_20K; // 0x37		

	uint32_t ErrWarMsg_20K;
	
	uint32_t HOURS_20K;

	uint8_t use_protocol;

	//uint16_t TEMP_20K;	// 0x00
	//uint16_t E_TODAY; 
	//uint16_t VPV1_20K;	// 0x01
	//uint16_t VPV2_20K;	// 0x02
	//uint16_t IPV1_20K;	// 0x04
	//uint16_t IPV2_20K;	// 0x05
	//uint16_t E_Total_H_20K;	// 0x07
	//uint16_t E_Total_L_20K;	// 0x08
	//uint16_t h_Total_H_20K;	// 0x09
	//uint16_t h_Total_L_20K;	// 0x0a
	//uint16_t PAC_20K;	// 0x0b
	//uint16_t MODE_20K;	// 0x0c
	//uint16_t E_Today_20K; // 0x0d

}inverter_msg2host_eversolar_t;

typedef struct inv_eversolar_scan_rsp{
	uint8_t header_aa;
	uint8_t header_55;
	uint8_t src_addr_h;
	uint8_t src_addr_l;
	uint8_t dst_addr_h;
	uint8_t dst_addr_l;
	uint8_t ctrl;
	uint8_t func;
	uint8_t len;
	uint8_t data_0;
	uint8_t data_1;
	uint8_t data_2;
	uint8_t data_3;
	uint8_t data_4;
	uint8_t data_5;
	uint8_t data_6;
	uint8_t data_7;
	uint8_t data_8;
	uint8_t data_9;
	uint8_t data_a;
	uint8_t data_b;
	uint8_t data_c;
	uint8_t data_d;
	uint8_t data_e;
	uint8_t data_f;
	uint8_t fcs_h;
	uint8_t fcs_l;
}inv_eversolar_scan_rsp_t;

typedef struct inv_eversolar_alloc_rsp{
	uint8_t header_aa;
	uint8_t header_55;
	uint8_t src_addr_h;
	uint8_t src_addr_l;
	uint8_t dst_addr_h;
	uint8_t dst_addr_l;
	uint8_t ctrl;
	uint8_t func;
	uint8_t len;
	uint8_t ack;
	uint8_t fcs_h;
	uint8_t fcs_l;
}inv_eversolar_alloc_t;


typedef struct inv_eversolar_pure_rd_rsp{
	uint8_t MachineType;
	uint8_t VARange[6];
	uint8_t FirmwareVer[5];
	uint8_t ModelName[16];
	uint8_t Manufacture[16];
	uint8_t SerialNum[16];
	uint8_t Nom[4];
	
	uint16_t TEMP;	//0x00
	uint16_t E_TODAY;	// 0x0d
	uint16_t VPV;	// 0x01
	uint16_t VPV2;	// 0x02
	uint16_t IPV;	// 0x04
	uint16_t IPV2;	// 0x05
	uint16_t IAC;	// 0x41
	uint16_t VAC;	// 0x42
	uint16_t FREQUENCY;	// 0x43
	uint16_t PAC;	// 0x44
	uint16_t ZAC;	// 0x45
	uint16_t E_TOTAL_H;		// 0x47
	uint16_t E_TOTAL;		// 0x48
	uint16_t HOURS_UP_H;	// 0x49
	uint16_t HOURS_UP;		// 0x4a
	uint16_t OP_MODE;		// 0x4c
	uint16_t GVFaultValue;	// 0x78
	uint16_t GFFaultValue;	// 0x79
	uint16_t GZFaultValue;	// 0x7a
	uint16_t TmpFaultValue;	// 0x7b
	uint16_t PV1FaultValue;	// 0x3a
	uint16_t PV2FaultValue;	// 0x3b
	uint16_t GFCIFaultValue;	// 0x7d
	uint16_t ErrWarMsg_H;	// 0x7e
	uint16_t ErrWarMsg_L;	// 0x7f
	
	uint16_t TEMP_20K;	// 0x00
	//uint16_t E_TODAY; 
	uint16_t VPV1_20K;	// 0x01
	uint16_t VPV2_20K;	// 0x02
	uint16_t IPV1_20K;	// 0x04
	uint16_t IPV2_20K;	// 0x05
	uint16_t E_Total_H_20K;	// 0x07
	uint16_t E_Total_L_20K;	// 0x08
	uint16_t h_Total_H_20K;	// 0x09
	uint16_t h_Total_L_20K;	// 0x0a
	uint16_t PAC_20K;	// 0x0b
	uint16_t MODE_20K;	// 0x0c
	uint16_t E_Today_20K; // 0x0d
	
	uint16_t Iac_R_20K;  // 0x41
	uint16_t Vac_R_20K;  // 0x42
	uint16_t Fac_R_20K;  // 0x43

	uint16_t Iac_S_20K;  // 0x81
	uint16_t Vac_S_20K; 	// 0x82

	uint16_t Iac_T_20K;  // 0xc1
	uint16_t Vac_T_20K; 	// 0xc2

	uint16_t ErrWarMsg_H_20K;	// 0x3e
	uint16_t ErrWarMsg_L_20K;	// 0x3f

	uint16_t WaitingTime_20K; // 0x38
	uint16_t CosPhi_20K;	// 0x36
	uint16_t Phase_20K; // 0x37	
}inv_eversolar_pure_rd_t;

#if 0
typedef struct inv_eversolar_pure_rd_rsp{
	uint8_t MachineType;
	uint8_t VARange[6];
	uint8_t FirmwareVer[5];
	uint8_t ModelName[16];
	uint8_t Manufacture[16];
	uint8_t SerialNum[16];
	uint8_t Nom[4];
	uint16_t TEMP;
	uint16_t E_TODAY;
	uint16_t VPV;
	uint16_t VPV2;
	uint16_t IPV;
	uint16_t IPV2;
	uint16_t IAC;
	uint16_t VAC;
	uint16_t FREQUENCY;
	uint16_t PAC;
	uint16_t NA_0;
	uint16_t NA_1;
	uint16_t E_TOTAL;
	uint16_t NA_2;
	uint16_t HOURS_UP;
	uint16_t OP_MODE;
	
	uint16_t TEMP_20K;	// 0x00
	//uint16_t E_TODAY; 
	uint16_t VPV1_20K;	// 0x01
	uint16_t VPV2_20K;	// 0x02
	uint16_t IPV1_20K;	// 0x04
	uint16_t IPV2_20K;	// 0x05
	uint16_t E_Total_H_20K;	// 0x07
	uint16_t E_Total_L_20K;	// 0x08
	uint16_t h_Total_H_20K;	// 0x09
	uint16_t h_Total_L_20K;	// 0x0a
	uint16_t PAC_20K;	// 0x0b
	uint16_t MODE_20K;	// 0x0c
	uint16_t E_Today_20K; // 0x0d
	
	uint16_t Iac_R_20K;  // 0x41
	uint16_t Vac_R_20K;  // 0x42
	uint16_t Fac_R_20K;  // 0x43

	uint16_t Iac_S_20K;  // 0x81
	uint16_t Vac_S_20K; 	// 0x82

	uint16_t Iac_T_20K;  // 0xc1
	uint16_t Vac_T_20K; 	// 0xc2

	uint16_t ErrWarMsg_H_20K;	// 0x3e
	uint16_t ErrWarMsg_L_20K;	// 0x3f

	uint16_t WaitingTime_20K; // 0x38
	uint16_t CosPhi_20K;	// 0x36
	uint16_t Phase_20K; // 0x37	
}inv_eversolar_pure_rd_t;


typedef struct inv_eversolar_pure_rd_20k_rsp{
	uint8_t MachineType;
	uint8_t VARange[6];
	uint8_t FirmwareVer[5];
	uint8_t ModelName[16];
	uint8_t Manufacture[16];
	uint8_t SerialNum[16];
	uint8_t Nom[4];
	uint16_t TEMP;	// 0x00
	//uint16_t E_TODAY; 
	uint16_t VPV1;	// 0x01
	uint16_t VPV2;	// 0x02
	uint16_t IPV1;	// 0x04
	uint16_t IPV2;	// 0x05
	uint16_t E_Total_H;	// 0x07
	uint16_t E_Total_L;	// 0x08
	uint16_t h_Total_H;	// 0x09
	uint16_t h_Total_L;	// 0x0a
	uint16_t PAC;	// 0x0b
	uint16_t MODE;	// 0x0c
	uint16_t E_Today; // 0x0d
	
	uint16_t Iac_R;  // 0x41
	uint16_t Vac_R;  // 0x42
	uint16_t Fac_R;  // 0x43

	uint16_t Iac_S;  // 0x81
	uint16_t Vac_S; 	// 0x82

	uint16_t Iac_T;  // 0xc1
	uint16_t Vac_T; 	// 0xc2

	uint16_t ErrWarMsg_H;	// 0x3e
	uint16_t ErrWarMsg_L;	// 0x3f

	uint16_t WaitingTime; // 0x38
	uint16_t CosPhi;	// 0x36
	uint16_t Phase; // 0x37
}inv_eversolar_pure_rd_20k_t;
#endif

typedef enum{
	
	INV_EVERSOLAR_STATE_INIT=0,
	INV_EVERSOLAR_STATE_RDY,
	INV_EVERSOLAR_STATE_TIMEOUT,
	INV_EVERSOLAR_STATE_ERR,
	INV_EVERSOLAR_STATE_STOP,
	
	INV_EVERSOLAR_STATE_ADV_LIST = 0x009,		//0x010
	
	INV_EVERSOLAR_STATE_QUERY_INFO = 0x010,
	INV_EVERSOLAR_STATE_CHECK_TYPE,

	INV_EVERSOLAR_STATE_CAPTURE,
	INV_EVERSOLAR_STATE_END,

	INV_EVERSOLAR_STATE_CAPTURE_20K = 0x018,
	INV_EVERSOLAR_STATE_END_20K,


	// if this state is declared after INV_EVERSOLAR_STATE_END, state will NOT be run
	// End of useless
	
	INV_EVERSOLAR_STATE_SCAN_RUN = 0x101,
	INV_EVERSOLAR_STATE_SCAN_ALLOC,
	INV_EVERSOLAR_STATE_SCAN_END
	
}inv_eversolar_state_t;

typedef struct InvEversolarRegsMatrix{
	inv_eversolar_state_t state_num;		// state number
	uint8_t ctrl_code;				// ctrl code
	uint8_t func_code;				// functional code
	//uint8_t* ptr_addr;				// where to store this return value
	uint32_t offset;
	uint8_t cpy_size;				// read size in unit Byte, 
									// while ModBus count in Word containing 2 Bytes 
}inv_eversolar_regsmatrix_t;

int create_eversolar_cmd(void* FrameBuffer, uint8_t ctrl_code, uint8_t fun_code, uint8_t dev_addr, uint8_t* sn, uint8_t id);

int convert_invmsg_to_asci_eversolar(char *buff, inverter_msg2host_eversolar_t* msg2host, int len);
int convert_invmsg_to_asci_zeversolar(char *buff, inverter_msg2host_eversolar_t* msg2host, int len);
int update_inv_msg2host_eversolar(inverter_msg2host_eversolar_t *data, inv_eversolar_pure_rd_t *inverter_info, uint16_t invId);
int invInfoGet_Eversolar(uint16_t invAddr, void *data, bool_t isScan, char* scan_port_name, eversolar_list_t** list_head);
int invInfoGet_Zeversolar(uint16_t invAddr, void *data, bool_t isScan, char* scan_port_name, eversolar_list_t** list_head);

uint8_t eversolar_getIdFromSn(eversolar_list_t* head, char* sn);
int eversolar_appendEntry(char* sn, uint8_t Id);

//#define EVERSOLAR_ADDR_START 50
//#define EVERSOLAR_ADDR_END 199
//#define EVERSOLAR_ADDR_AMOUNT (EVERSOLAR_ADDR_END - EVERSOLAR_ADDR_START)  // allowable number of inverters

//extern eversolar_list_t* eversolar_table_head;
/****************************
** ALI Inverter protocol
****************************/
typedef struct inverter_ali_raw_struct{
	
	uint16_t inverterId;

// first phase of capture, 15 words a time
	//address 0xB4
	uint16_t State_mode;		// 0xB4
	uint16_t Error_Code1;		// 0xB5
	uint16_t Error_Code2;
	uint16_t Error_Code3;
	uint16_t Error_Code4;
	
	//address 0xB9
	uint16_t Vac_L1;
	uint16_t Iac_L1;
	uint16_t Pac_L1;
	uint16_t Fac;
	uint16_t Vpv_A;
	uint16_t Vpv_B;
	uint16_t Ipv_A;
	uint16_t Ipv_B;
	uint16_t Ppv_A;
	uint16_t Ppv_B;

// second phase of capture, 12 words a time
	// address 0xC3
	uint16_t Bus_V;
	uint16_t Eac_H;
	uint16_t Eac_L;
	uint16_t EpvA_H;
	uint16_t EpvA_L;
	uint16_t EpvB_H;
	uint16_t EpvB_L;

	// address 0xCA
	uint16_t Riso_A;
	uint16_t Riso_B;
	uint16_t Ires;
	uint16_t Heatsink_temp;
	uint16_t Eac_TODAY;
	uint16_t Rsvd_0xCF;
	uint16_t Rsvd_0xD0;
	uint16_t Rsvd_0xD1;
	uint16_t Rsvd_0xD2;

// third phase of capture, 12 words a time
	//address 0xD3
	uint16_t CO2_REDUCTION_H;
	uint16_t CO2_REDUCTION_L;
	uint16_t Ton_today;
	uint16_t Ton_total_Hr;
	uint16_t Ton_total_Min;
	uint16_t Ton_total_Sec;

	// address 0x104 for DC
	uint16_t MODEL_NAME;
	uint16_t Version_Master;
	uint16_t Version_Slave;
	uint16_t SN_HIGH;
	uint16_t SN_LOW;
	uint16_t DEVICE_VER;

}inverter_ali_raw_t;


typedef enum{
	
	INV_ALI_STATE_INIT=0,
	INV_ALI_STATE_RDY,
	INV_ALI_STATE_TIMEOUT,
	INV_ALI_STATE_ERR,
	INV_ALI_STATE_STOP,
	
	INV_ALI_STATE_C1 = 0x10,
	INV_ALI_STATE_C2,
	INV_ALI_STATE_C3,
	INV_ALI_STATE_END,
	// End of useless
	
	INV_ALI_STATE_SCAN_RUN = 0x101,
	INV_ALI_STATE_SCAN_END
	
}inv_ali_state_t;

typedef struct InvAliRegsMatrix{
	inv_ali_state_t state_num;		// state number
	uint16_t reg_addr;				// reg address to be read
	//uint16_t* ptr_addr;				// where to store this return value
	uint32_t offset;
	uint8_t cpy_size;				// read size in unit Byte, 
									// while ModBus count in Word containing 2 Bytes 
}inv_ali_regsmatrix_t;


typedef struct inverter_msg2host_ali_struct{
	uint8_t state_code;			// 0xb4
	uint8_t inv_id;
	uint8_t brand_name[16];		// 0x77 ~ 0x7e
	uint8_t type_name[16];		// 0x67 ~ 0x6e
	uint8_t sn_name[16];		// 0x6f ~ 0x76
	uint16_t total_hr;			// 0xd6
	uint16_t total_min;			// 0xd7
	uint16_t total_sec;			// 0xd8
	uint16_t total_gen_h;		// 0xc4
	uint16_t total_gen_l;		// 0xc5
	uint16_t pac;				// 0xbb
	uint16_t pdc;				// 0xc1 + 0xc2
	uint16_t total_out_today;  // 0xce
}inverter_msg2host_ali_t;

int convert_invmsg_to_asci_ali(char *buff, inverter_ali_raw_t* msg2host);
int update_inv_msg2host_ali(inverter_ali_raw_t *data, inverter_ali_raw_t *inverter_info, uint16_t invId);
int invInfoGet_Ali(uint16_t invAddr, inverter_ali_raw_t *data, char* scan_port_name);
int inverter_scan_ali(scan_inv_info_t* inv_info, char* scan_port_name);

/****************************
** EnerSolis Inverter protocol
****************************/
typedef struct inverter_enersolis_raw_struct{
	
	uint16_t inverterId;

	// Identifier part  16 words
	uint16_t inverterType;
	uint16_t power;
	uint16_t modelName0;
	uint16_t modelName1;
	uint16_t modelName2;
	uint16_t modelName3;
	uint16_t modelName4;
	uint16_t company0;
	uint16_t company1;
	uint16_t company2;
	uint16_t company3;
	uint16_t company4;
	uint16_t operVer;

	// Status part  2 words
	uint16_t errWord0;  // E15 ~ E00
	uint16_t errWord1;  // E31 ~ E16

	// Alarm part
	uint16_t alarmWord0;  // A15 ~ A00
	uint16_t alarmWord1;  // A31 ~ A16	

	// Measurement part
	uint16_t outputPower;
	uint16_t AC_voltagePL1;
	uint16_t AC_voltagePL2;
	uint16_t AC_voltagePL1L2;
	uint16_t AC_outputCurrentL1;
	uint16_t AC_outputCurrentL2;
	uint16_t AC_frequencyL1;
	uint16_t DCBusPosVolt;
	uint16_t DCBusNegVolt;
	uint16_t invIntlTemp;
	uint16_t invHeatSinkTemp;
	uint16_t DC1inputVolt;
	uint16_t DC2inputVolt;
	uint16_t DC1inputCurr;
	uint16_t DC2inputCurr;
	uint16_t inputPwrA;
	uint16_t inputPwrB;
	uint16_t totalOutputPwr0;
	uint16_t totalOutputPwr1;
	uint16_t batteryVolt;
	uint16_t batteryChgCurr;
	uint16_t batteryDisChgCurr;
	uint16_t totalChgPwr0;
	uint16_t totalChgPwr1;
	uint16_t batteryTempSTVmin;
	uint16_t stVmax;
	uint16_t stFmin;
	uint16_t stFmax;

	uint16_t AC_voltagePL2L3;
	uint16_t AC_frequencyL2;
	uint16_t AC_voltagePL3;
	uint16_t AC_voltagePL3L1;
	uint16_t AC_frequencyL3;
	uint16_t AC_outputCurrentL3;
	uint16_t eventCode12;
	uint16_t eventCode34;
	uint16_t eventCode56;
	uint16_t stVmin2;
	uint16_t stVmax2;
	uint16_t stFmin2;
	uint16_t stFmax2;

}inverter_enersolis_raw_t;


typedef enum{
	
	INV_ENERSOLIS_STATE_INIT=0,
	INV_ENERSOLIS_STATE_RDY,
	INV_ENERSOLIS_STATE_TIMEOUT,
	INV_ENERSOLIS_STATE_ERR,
	INV_ENERSOLIS_STATE_STOP,
	
	INV_ENERSOLIS_STATE_C1 = 0x10,
	INV_ENERSOLIS_STATE_C2,
	INV_ENERSOLIS_STATE_C3,
	INV_ENERSOLIS_STATE_C4,
	INV_ENERSOLIS_STATE_C5,
	INV_ENERSOLIS_STATE_END,
	// End of useless
	
	INV_ENERSOLIS_STATE_SCAN_RUN = 0x101,
	INV_ENERSOLIS_STATE_SCAN_REPEAT,
	INV_ENERSOLIS_STATE_SCAN_END
	
}inv_enersolis_state_t;

#if 0
typedef struct InvEnerSolisRegsMatrix{
	inv_enersolis_state_t state_num;		// state number
	uint16_t reg_addr;				// reg address to be read
	uint16_t* ptr_addr;				// where to store this return value
	uint32_t offset;
	uint8_t cpy_size;				// read size in unit Byte, 
									// while ModBus count in Word containing 2 Bytes 
}inv_enersolis_regsmatrix_t;
#endif

typedef struct InvEnerSolisRegsMatrix{
	inv_enersolis_state_t state_num;		// state number
	uint16_t reg_addr;				// reg address to be read
	uint16_t* ptr_addr;				// where to store this return value
	uint8_t cpy_size;				// read size in unit Byte, 
									// while ModBus count in Word containing 2 Bytes 
}inv_enersolis_regsmatrix_t;

int convert_invmsg_to_asci_enersolis(char *buff, inverter_enersolis_raw_t* msg2host);
//int update_inv_msg2host_enersolis(inverter_enersolis_raw_t *data, inverter_enersolis_raw_t *inverter_info, uint8_t invId);
int update_inv_msg2host_enersolis(inverter_enersolis_raw_t *data, inverter_enersolis_raw_t *inverter_info, uint16_t invId);
//int invInfoGet_EnerSolis(gInverterAddr_t invAddr, inverter_enersolis_raw_t *data, char* scan_port_name);
int invInfoGet_EnerSolis(uint16_t invAddr, inverter_enersolis_raw_t *data, char* scan_port_name);
int inverter_scan_enersolis(scan_inv_info_t* inv_info, char* scan_port_name);

/****************************
** Digimeter protocol
****************************/

typedef struct inverter_digimeter_raw_struct{
	uint16_t inverterId;
	// address 0H
	uint16_t digimeter_serial;
	uint16_t digimeter_type;
	uint16_t digimeter_addr;
	uint16_t digimeter_parity;
	uint16_t digimeter_pt;
	uint16_t digimeter_ptexp;
	uint16_t digimeter_ct;
	uint16_t digimeter_ctexp;
	uint16_t digimeter_voltmax;
	uint16_t digimeter_currmax;
	uint16_t digimeter_wattmax;
	uint16_t digimeter_status;
	uint16_t digimeter_cali;
	uint16_t digimeter_ch1voltrslv;
	uint16_t digimeter_ch2currrslv;
	uint16_t digimeter_voltexp_1;
	uint16_t digimeter_currexp_1;
	uint16_t digimeter_voltset;
	uint16_t digimeter_currset;
	uint16_t digimeter_whaccumrate;
	uint16_t digimeter_whposhw;
	uint16_t digimeter_whposlw;
	uint16_t digimeter_whngthw;
	uint16_t digimeter_whngtlw;
	
	// address 78H
	uint16_t digimeter_freq;
	uint16_t digimeter_xpt;
	uint16_t digimeter_xct;
	uint16_t digimeter_xptxct;
	uint16_t digimeter_voltexp_2;
	uint16_t digimeter_currexp_2;
	uint16_t digimeter_wattexp;
	uint16_t digimeter_whexp;
	uint16_t digimeter_ch1voltmax;
	uint16_t digimeter_ch1voltmin;
	uint16_t digimeter_ch1voltavg;
	uint16_t digimeter_ch1currmax;
	uint16_t digimeter_ch1currmin;
	uint16_t digimeter_ch1curravg;

	uint16_t digimeter_ch2voltmax;
	uint16_t digimeter_ch2voltmin;
	uint16_t digimeter_ch2voltavg;
	uint16_t digimeter_ch2currmax;
	uint16_t digimeter_ch2currmin;
	uint16_t digimeter_ch2curravg;

	uint16_t digimeter_ch3voltmax;
	uint16_t digimeter_ch3voltmin;
	uint16_t digimeter_ch3voltavg;
	uint16_t digimeter_ch3currmax;
	uint16_t digimeter_ch3currmin;
	uint16_t digimeter_ch3curravg;

	// 92H
	uint16_t digimeter_ch1pf;
	uint16_t digimeter_ch2pf;
	uint16_t digimeter_ch3pf;
	uint16_t digimeter_sumpf;
	uint16_t digimeter_12vrmshw;
	uint16_t digimeter_12vrmslw;
	uint16_t digimeter_23vrmshw;
	uint16_t digimeter_23vrmslw;
	uint16_t digimeter_13vrmshw;
	uint16_t digimeter_13vrmslw;
	uint16_t digimeter_ch1vrmshw;
	uint16_t digimeter_ch1vrmslw;
	uint16_t digimeter_ch2vrmshw;
	uint16_t digimeter_ch2vrmslw;
	uint16_t digimeter_ch3vrmshw;
	uint16_t digimeter_ch3vrmslw;
	uint16_t digimeter_sumvrmshw;
	uint16_t digimeter_sumvrmslw;
	uint16_t digimeter_ch1irmshw;
	uint16_t digimeter_ch1irmslw;
	uint16_t digimeter_ch2irmshw;
	uint16_t digimeter_ch2irmslw;
	uint16_t digimeter_ch3irmshw;
	uint16_t digimeter_ch3irmslw;
	uint16_t digimeter_sumirmshw;
	uint16_t digimeter_sumirmslw;
	// ACH
	uint16_t digimeter_ch1whw;
	uint16_t digimeter_ch1wlw;
	uint16_t digimeter_ch2whw;
	uint16_t digimeter_ch2wlw;
	uint16_t digimeter_ch3whw;
	uint16_t digimeter_ch3wlw;
	uint16_t digimeter_sumwhw;
	uint16_t digimeter_sumwlw;
	uint16_t digimeter_ch1vahw;
	uint16_t digimeter_ch1valw;
	uint16_t digimeter_ch2vahw;
	uint16_t digimeter_ch2valw;
	uint16_t digimeter_ch3vahw;
	uint16_t digimeter_ch3valw;
	uint16_t digimeter_sumvahw;
	uint16_t digimeter_sumvalw;

	// BCH
	uint16_t digimeter_ch1varhw;
	uint16_t digimeter_ch1varlw;
	uint16_t digimeter_ch2varhw;
	uint16_t digimeter_ch2varlw;
	uint16_t digimeter_ch3varhw;
	uint16_t digimeter_ch3varlw;
	uint16_t digimeter_sumvarhw;
	uint16_t digimeter_sumvarlw;
	uint16_t digimeter_whhw;
	uint16_t digimeter_whlw;
	uint16_t digimeter_poswhhw;
	uint16_t digimeter_poswhlw;
	uint16_t digimeter_ngtwhhw;
	uint16_t digimeter_ngtwhlw;

}inverter_digimeter_raw_t;


typedef enum{
	
	INV_DIGIMETER_STATE_INIT=0,
	INV_DIGIMETER_STATE_RDY,
	INV_DIGIMETER_STATE_TIMEOUT,
	INV_DIGIMETER_STATE_ERR,
	INV_DIGIMETER_STATE_STOP,
	
	INV_DIGIMETER_STATE_C1 = 0x10,
	INV_DIGIMETER_STATE_C2,
	INV_DIGIMETER_STATE_C3,	
	INV_DIGIMETER_STATE_END,
	INV_DIGIMETER_STATE_C4,
	INV_DIGIMETER_STATE_C5,
	// End of useless
	
	INV_DIGIMETER_STATE_SCAN_RUN = 0x101,
	INV_DIGIMETER_STATE_SCAN_END
	
}inv_digimeter_state_t;

typedef struct InvDigiMeterRegsMatrix{
	inv_digimeter_state_t state_num;		// state number
	uint16_t reg_addr;				// reg address to be read
	//uint16_t* ptr_addr;				// where to store this return value
	uint32_t offset;
	uint8_t cpy_size;				// read size in unit Byte, 
									// while ModBus count in Word containing 2 Bytes 
}inv_digimeter_regsmatrix_t;


typedef struct inverter_msg2host_digimeter_struct{
	uint16_t inverterId;
	// address 0H
	uint16_t digimeter_serial;
	uint16_t digimeter_type;
	uint16_t digimeter_addr;
	uint16_t digimeter_parity;
	uint16_t digimeter_pt;
	uint16_t digimeter_ptexp;
	uint16_t digimeter_ct;
	uint16_t digimeter_ctexp;
	uint16_t digimeter_voltmax;
	uint16_t digimeter_currmax;
	uint16_t digimeter_wattmax;
	uint16_t digimeter_status;
	uint16_t digimeter_cali;
	uint16_t digimeter_ch1voltrslv;
	uint16_t digimeter_ch2currrslv;
	uint16_t digimeter_voltexp_1;
	uint16_t digimeter_currexp_1;
	uint16_t digimeter_voltset;
	uint16_t digimeter_currset;
	uint16_t digimeter_whaccumrate;
	uint16_t digimeter_whposhw;
	uint16_t digimeter_whposlw;
	uint16_t digimeter_whngthw;
	uint16_t digimeter_whngtlw;
	
	// address 78H
	uint16_t digimeter_freq;
	uint16_t digimeter_xpt;
	uint16_t digimeter_xct;
	uint16_t digimeter_xptxct;
	uint16_t digimeter_voltexp_2;
	uint16_t digimeter_currexp_2;
	uint16_t digimeter_wattexp;
	uint16_t digimeter_whexp;
	uint16_t digimeter_ch1voltmax;
	uint16_t digimeter_ch1voltmin;
	uint16_t digimeter_ch1voltavg;
	uint16_t digimeter_ch1currmax;
	uint16_t digimeter_ch1currmin;
	uint16_t digimeter_ch1curravg;

	uint16_t digimeter_ch2voltmax;
	uint16_t digimeter_ch2voltmin;
	uint16_t digimeter_ch2voltavg;
	uint16_t digimeter_ch2currmax;
	uint16_t digimeter_ch2currmin;
	uint16_t digimeter_ch2curravg;

	uint16_t digimeter_ch3voltmax;
	uint16_t digimeter_ch3voltmin;
	uint16_t digimeter_ch3voltavg;
	uint16_t digimeter_ch3currmax;
	uint16_t digimeter_ch3currmin;
	uint16_t digimeter_ch3curravg;

	// 92H
	uint16_t digimeter_ch1pf;
	uint16_t digimeter_ch2pf;
	uint16_t digimeter_ch3pf;
	uint16_t digimeter_sumpf;
	uint16_t digimeter_12vrmshw;
	uint16_t digimeter_12vrmslw;
	uint16_t digimeter_23vrmshw;
	uint16_t digimeter_23vrmslw;
	uint16_t digimeter_13vrmshw;
	uint16_t digimeter_13vrmslw;
	uint16_t digimeter_ch1vrmshw;
	uint16_t digimeter_ch1vrmslw;
	uint16_t digimeter_ch2vrmshw;
	uint16_t digimeter_ch2vrmslw;
	uint16_t digimeter_ch3vrmshw;
	uint16_t digimeter_ch3vrmslw;
	uint16_t digimeter_sumvrmshw;
	uint16_t digimeter_sumvrmslw;
	uint16_t digimeter_ch1irmshw;
	uint16_t digimeter_ch1irmslw;
	uint16_t digimeter_ch2irmshw;
	uint16_t digimeter_ch2irmslw;
	uint16_t digimeter_ch3irmshw;
	uint16_t digimeter_ch3irmslw;
	uint16_t digimeter_sumirmshw;
	uint16_t digimeter_sumirmslw;
	// ACH
	uint16_t digimeter_ch1whw;
	uint16_t digimeter_ch1wlw;
	uint16_t digimeter_ch2whw;
	uint16_t digimeter_ch2wlw;
	uint16_t digimeter_ch3whw;
	uint16_t digimeter_ch3wlw;
	uint16_t digimeter_sumwhw;
	uint16_t digimeter_sumwlw;
	uint16_t digimeter_ch1vahw;
	uint16_t digimeter_ch1valw;
	uint16_t digimeter_ch2vahw;
	uint16_t digimeter_ch2valw;
	uint16_t digimeter_ch3vahw;
	uint16_t digimeter_ch3valw;
	uint16_t digimeter_sumvahw;
	uint16_t digimeter_sumvalw;

	// BCH
	uint16_t digimeter_ch1varhw;
	uint16_t digimeter_ch1varlw;
	uint16_t digimeter_ch2varhw;
	uint16_t digimeter_ch2varlw;
	uint16_t digimeter_ch3varhw;
	uint16_t digimeter_ch3varlw;
	uint16_t digimeter_sumvarhw;
	uint16_t digimeter_sumvarlw;
	uint16_t digimeter_whhw;
	uint16_t digimeter_whlw;
	uint16_t digimeter_poswhhw;
	uint16_t digimeter_poswhlw;
	uint16_t digimeter_ngtwhhw;
	uint16_t digimeter_ngtwhlw;
}inverter_msg2host_digimeter_t;

int convert_invmsg_to_asci_digimeter(char *buff, inverter_msg2host_digimeter_t* msg2host);
int update_inv_msg2host_digimeter(inverter_msg2host_digimeter_t *data, inverter_digimeter_raw_t *inverter_info, uint16_t invId);
int invInfoGet_DigiMeter(uint16_t invAddr, inverter_msg2host_digimeter_t *data, char* scan_port_name);
int inverter_scan_digimeter(scan_inv_info_t* inv_info, char* scan_port_name);

/****************************
** Global
****************************/
/*
#define USE_MOTECH  		0x0001
#define USE_TOUGH   		0x0002
#define USE_EVERSOLAR 		0x0004
#define USE_DELTA			0x0008
#define USE_ENERSOLIS       0x0010
#define USE_PYRANOMETER     0x0100
#define USE_THERMOGRAPH		0x0200
#define USE_THERMOMETER		0x0400
#define USE_WEATHERSITE		0x0800
#define USE_DIGIMETER       0x1000

#define PYRANOMETER_MAX 5  // 2~5 is for pyranometer
#define THERMOGRAPH_MAX 7  // 6~7 is for thermograph
#define PYRANO_THERMO_MAX 10  // 8~9 is for thermometer
#define DIGIMETER_START 1
#define DIGIMETER_MAX 10 // 10 ~ 19
*/

extern uint16_t scan_inv_types;


//extern inv_type_id_t TYPE_ID_TABLE[256];

extern scan_inv_info_t sys_inv_info_pyranometer;
extern scan_inv_info_t sys_inv_info_thermometer;
extern scan_inv_info_t sys_inv_info_motech;
extern scan_inv_info_t sys_inv_info_ali;
extern scan_inv_info_t sys_inv_info_eversolar;
extern scan_inv_info_t sys_inv_info_delta;
//extern scan_inv_info_t sys_inv_info_all;
extern scan_inv_info_t sys_inv_info_enersolis;


extern inverter_msg2host_new_t inverter_msg2host_motech_array[256*USE_NUM_OF_CHANNEL];
extern inverter_ali_raw_t inverter_msg2host_ali_array[256*USE_NUM_OF_CHANNEL];
extern inverter_msg2host_eversolar_t inverter_msg2host_eversolar_array[256*USE_NUM_OF_CHANNEL];
extern inverter_msg2host_delta_t inverter_msg2host_delta_array[256*USE_NUM_OF_CHANNEL];
extern inverter_enersolis_raw_t inverter_msg2host_enersolis_array[256*USE_NUM_OF_CHANNEL];
extern inverter_msg2host_pyranometer_t inverter_msg2host_pyranometer_array[256*USE_NUM_OF_CHANNEL];  // entry should be the same as PYRANO_THERMO_MAX+1
extern inverter_msg2host_thermometer_t inverter_msg2host_thermometer_array[256*USE_NUM_OF_CHANNEL];  // entry should be the same as PYRANO_THERMO_MAX+1
extern inverter_msg2host_thermometer_t inverter_msg2host_thermograph_array[256*USE_NUM_OF_CHANNEL];  // entry should be the same as PYRANO_THERMO_MAX+1
extern inverter_msg2host_digimeter_t inverter_msg2host_digimeter_array[256*USE_NUM_OF_CHANNEL];
extern inverter_msg2host_jda_t inverter_msg2host_jda_array[256*USE_NUM_OF_CHANNEL];

int inverter_scan_all(scan_inv_info_t* inv_info, char* scan_port_name, eversolar_list_t** eversolar_list_head);

int inverter_scan_pyranometer(scan_inv_info_t* inv_info, char* scan_port_name, uint16_t offset);

int inverter_scan_jda(scan_inv_info_t* inv_info, char* scan_port_name, uint16_t offset);

#endif
