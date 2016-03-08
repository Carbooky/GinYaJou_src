#include <stdio.h>
#include <stdlib.h>
#include <my_global.h>
#include <mysql.h>
#include <stdbool.h>
#include <string.h>
#include "adc.h"

int adc_pin_array[2] = {ADC_PIN_01, ADC_PIN_02};

int read_adc_value(int* array, int position ,int* rtn_value){
	char strbuff[64];
	if(!array)
		return -1;
	if(position >8)
		return -1;
    
	printf("adc wants to control %d, %d,\n", array[position*2], array[position*2+1]);

	// set direction
	if(direction){	//output
		iolib_setdir(array[position*2], array[position*2+1] , DIR_OUT);
		// set value
		if(set_value==1)
			pin_high(array[position*2], array[position*2+1]);
		else if(set_value==0)
			pin_low(array[position*2], array[position*2+1]);
	}else{	// input
		// get value
		iolib_setdir(array[position*2], array[position*2+1], DIR_IN);
		if(is_high(array[position*2], array[position*2+1]))
			*rtn_value = 1;
		else if(is_low(array[position*2], array[position*2+1]))
			*rtn_value = 0;
	}
	return 0;
}
