#ifndef _GPIO_H
#define _GPIO_H

#define BANK0_01 8,7
#define BANK0_02 8,8
#define BANK0_03 8,9
#define BANK0_04 8,10
#define BANK0_05 8,11
#define BANK0_06 8,12
#define BANK0_07 8,13
#define BANK0_08 8,14

#define BANK1_01 8,15
#define BANK1_02 8,16
#define BANK1_03 8,17
#define BANK1_04 8,18
#define BANK1_05 9,15
#define BANK1_06 9,16
#define BANK1_07 9,23
#define BANK1_08 9,27

#define NXP_OUTPUT_BANK0 0x08
#define NXP_OUTPUT_BANK1 0x09
#define NXP_OUTPUT_BANK2 0x0a
#define NXP_OUTPUT_BANK3 0x0b

#define NXP_CONFIG_BANK0 0x18
#define NXP_CONFIG_BANK1 0x19
#define NXP_CONFIG_BANK2 0x1a
#define NXP_CONFIG_BANK3 0x1b

typedef struct _GPIO_ELEMENT
{
	int valid;
	char dbTableName[16];
	char pinName[16];
	int onBoard;
	int type;
	int currValue;
	int direction;
	int setValue;
	int setBy;
	char datetime[32];
}gpio_element_t;

int get_n_set_gpio(MYSQL * db_conn_p, int bank_num, int pin_num);

int ctrl_gpio_state(int* array, int direction, int position , int set_value, int* rtn_value);

int ctrl_gpio_state_nxp_i2c_read(int position , int* rtn_value);
int ctrl_gpio_state_nxp_i2c_write(int position, int set_value);
int nxp_i2c_init(void);
int nxp_i2c_release(void);
int get_n_set_nxp_i2c(MYSQL * db_conn_p, int bank_num, int pin_num);

void *thread_gpio_management(void *argu);
void *thread_nxp_i2c_management(void *argu);

extern int io_dir_5v_array[16];
extern int io_dir_24v_array[16];
extern int gpio_update_interval;

#endif