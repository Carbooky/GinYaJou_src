#ifndef _UTIL
#define _UTIL

#include "format.h"
#include <libxml/parser.h>
#include "jsmn.h"
//#define INV_RSVD_STARTING_ID_FOR_EVERSOLAR 200

//int Transfer_uploadFile_multi(const char* dir, char** filename, int item_num,  const char* url, const char* username, const char* password);

typedef struct eversolar_list{
	uint8_t ID;
	char SN[19];
	uint8_t aging_cnt;
	struct eversolar_list* next;
}eversolar_list_t;

typedef struct gw_sync_pkt{
	uint8_t prefix;
	char timestamp[15];
	char out_serial[32];
	uint8_t cmd_rsp;
	uint16_t FCS;
}gw_sync_pkt_t;

int syscall_getRsp(const char* command, char* returnString);

void ITRI_Hex2Str( uint8_t *pStr, uint8_t decValue);
void error(const char *msg);
int append_FCS(uint8_t * FrameBuffer);
int check_FCS(uint8_t * FrameBuffer);

uint8_t get_group(void* frame);
uint8_t get_api_cmd(void* frame);
//extern uint16_t get_sn(void* frame);
//extern uint8_t get_dataLen(void* frame);
uint8_t get_cmd(void* frame);
uint8_t get_frameLen(void* frame);
uint8_t * get_api_data(void* frame);
//extern uint8_t build_gpio_rtn_template(raw_pkg_t *, uint8_t);
uint8_t create_rtn_template(raw_pkg_t *);

int inv_meta_data_dump( inverter_info_t * data_ptr);
int update_inv_msg2host(inverter_msg2host_t *msg2host, inverter_info_t *inverter_info);
//int convert_invmsg_to_asci(char *buff, inverter_msg2host_t* msg2host);
//extern int getbaud(int fd);
int isready(int fd);
void switch_bytes(uint8_t* data_ptr, int len);

int open_port_RS485(char* device_node, int mode);

int list_add(eversolar_list_t** head, uint8_t id, uint8_t* sn);
int list_del(eversolar_list_t** head, uint8_t id_to_del);
int list_del_all(eversolar_list_t** head);

int get_sim5215_info(char* device, char* rsp_string, int len);
int clear_serial_rx_buffer(int port);

int self_json_pack_string_mid(char* string_p, char* name, char* value);
int self_json_pack_string_tail(char* string_p, char* name, char* value);

int acquire_unique_name(char * path2xml, char * uniqueName);
int change_unique_name_my(char * path2xml, char * uniqueName);

uint16_t append_FCS_add(uint8_t * data, int len);


jsmntype_t get_value_by_key(char* js, jsmntok_t* tokens, int token_num, char* string, char* rtn_string);


int network_3G_start(void);
int network_eth0_start(void);
int network_wifi_start(void);
int network_3G_stop(void);
int network_eth0_stop(void);

int read_3g_info_file(char* fileName, char* rtn_info, int len);
int write_3g_info_file(char* fileName);

extern char info_3G[128];

// for USB LCD
#define LCD_CMD_PRE             0xFE
#define LCD_CMD_CLRSCR  0x58
#define LCD_CMD_SETCUR  0x47
#define LCD_CMD_ASCROFF 0x52
// API
int init_USB_LCD(char * device);
int ctrl_USB_LCD(char* device, int cmd);
int string_USB_LCD(char* device, char* string, int line);

int network_link_check(void);

#endif

