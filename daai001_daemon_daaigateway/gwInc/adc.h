#ifndef _ADC_H
#define _ADC_H

#define ADC_PIN_01 8,7
#define ADC_PIN_02 8,8

typedef struct _ADC_ELEMENT
{
	char dbTableName[16];
	char pinName[16];
	int onBoard;
	int type;
	int currValue;
	char datetime[32];
}adc_element_t;

int get_n_set_gpio(MYSQL * db_conn_p, int bank_num, int pin_num);

int ctrl_gpio_state(int* array, int direction, int position , int set_value, int* rtn_value);

void *thread_gpio_management(void *argu);

extern int adc_pin_array[2];

extern int gpio_update_interval;

#endif