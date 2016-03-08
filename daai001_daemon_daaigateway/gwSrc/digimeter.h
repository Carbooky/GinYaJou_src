#ifndef _DIGIMETER_H
#define _DIGIMETER_H
/****************************
** Digimeter protocol
****************************/
#define bool_t _Bool
#define TRUE 1
#define FALSE 0

#define INV_MSG_CRASH 2
#define INV_MSG_CRC_ERR 3

#define INVERTER_BAUDRATE B9600

#define SCAN_REPEAT_TIMES_WHEN_FAILED 1
#define CMD_REPEAT_TIMES_WHEN_FAILED 2

#define RETRY_TIMES_FOR_ONE_CMD 3   // retry times before abort reading  
#define WAITING_TIME_EACH_RETRY 300000		// the value is in ms unit, 
#define WAITING_TIME_EACH_SCAN_SENSOR 400000
#define WAITING_TIME_EACH_SCAN_INV 260000  // 300ms results in about 170 secs for Delta scan from 10 ~ 250 
#define WAITING_TIME_EACH_SENSOR 400000
#define RETRY_TIMES_FOR_ONE_SCAN (RETRY_TIMES_FOR_ONE_CMD-1)

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

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

typedef enum{
	INV_CMD_EVENT_NORMAL = 0,
	INV_CMD_EVENT_TIMEOUT = 0x2,
	INV_CMD_EVENT_ERR = 0x4,
	INV_CMD_EVENT_STOP = 0x8,
	INV_CMD_EVENT_END = 0x10,
	INV_CMD_EVENT_NOCHANGE = 0x20,
	INV_CMD_EVENT_SCAN = 0x40
}inv_cmd_event_t;

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

//int convert_invmsg_to_asci_digimeter(char *buff, inverter_digimeter_raw_t* msg2host);
//int update_inv_msg2host_digimeter(inverter_digimeter_raw_t *data, inverter_digimeter_raw_t *inverter_info, uint16_t invId);
//int invInfoGet_DigiMeter(uint16_t invAddr, inverter_digimeter_raw_t *data, char* scan_port_name);
int get_digimeter_value(char* out_buff, int size_of_buff, char* interface);

#endif
