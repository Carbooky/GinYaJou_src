#include <stdio.h>
#include <stdlib.h>
#include <my_global.h>
#include <mysql.h>
#include <stdbool.h>
#include <string.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include "iolib.h"
#include "gpio.h"

int io_dir_5v_array[16] = {BANK0_01, BANK0_02, BANK0_03, BANK0_04, BANK0_05, BANK0_06, BANK0_07, BANK0_08};//{8,7, 8,8, 8,9, 8,10, 8,11, 8,12, 8,13, 8,14};
int io_dir_24v_array[16] = {BANK1_01, BANK1_02, BANK1_03, BANK1_04, BANK1_05, BANK1_06, BANK1_07, BANK1_08};//{8,15, 8,16, 8,17, 8,18, 9,15, 9,16, 9,23, 9,27};
unsigned char i2c_output_nxp_bank[4] = {NXP_OUTPUT_BANK0, NXP_OUTPUT_BANK1, NXP_OUTPUT_BANK2, NXP_OUTPUT_BANK3};
unsigned char i2c_dir_nxp_bank[4] = {NXP_CONFIG_BANK0, NXP_CONFIG_BANK1, NXP_CONFIG_BANK2, NXP_CONFIG_BANK3};

int gpio_update_interval = 60;

int global_nxp_i2c_handle=0;

int ctrl_gpio_state(int* array, int direction, int position , int set_value, int* rtn_value){
	char strbuff[64];
	if(!array)
		return -1;
	if(position >8)
		return -1;
    
	printf("wants to control %d, %d, direction=%d\n", array[position*2], array[position*2+1], direction);

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
	}
	usleep(10000);

		if(is_high(array[position*2], array[position*2+1]))
			*rtn_value = 1;
		else if(is_low(array[position*2], array[position*2+1]))
			*rtn_value = 0;

	return 0;
}

#define I2C_NXP_ADDR 0x24

int nxp_i2c_init(void){
	char *filename = "/dev/i2c-1";
	
	if ((global_nxp_i2c_handle = open(filename, O_RDWR)) < 0)
	{
    	fprintf(stderr, "i2c_open open error: %s\n", strerror(errno));
    	return -1;
	}
  	if (ioctl(global_nxp_i2c_handle,I2C_SLAVE,I2C_NXP_ADDR) < 0)
  	{
    	fprintf(stderr, "i2c_open ioctl error: %s\n", strerror(errno));
    	return -1;
  	}

  	// set bank1, bank3 as output, bank0, bank2 as input
  	i2c_smbus_write_byte_data(global_nxp_i2c_handle, i2c_dir_nxp_bank[0], 0xff);
  	usleep(2000);
  	i2c_smbus_write_byte_data(global_nxp_i2c_handle, i2c_dir_nxp_bank[2], 0xff);
  	usleep(2000);
  	i2c_smbus_write_byte_data(global_nxp_i2c_handle, i2c_dir_nxp_bank[1], 0x00);
  	usleep(2000);
  	i2c_smbus_write_byte_data(global_nxp_i2c_handle, i2c_dir_nxp_bank[3], 0x00);

  	return 1;
}

int nxp_i2c_release(void){

	close(global_nxp_i2c_handle);

	return 1;
}

int ctrl_gpio_state_nxp_i2c_read(int position , int* rtn_value){
	unsigned int read_data;
	unsigned char position_3bit;
	unsigned char read_byte_data;
	
	// legal check 
	if(global_nxp_i2c_handle<=0){
		fprintf(stderr, "i2c error: %s, init()?\n", strerror(errno));
		return -1;
	}

	// read value into rtn_value
	read_data = (unsigned int)i2c_smbus_read_byte_data(global_nxp_i2c_handle, i2c_output_nxp_bank[position>>3]);
	read_byte_data = (unsigned char)read_data;
	position_3bit = 0x07 & position;

	if( read_byte_data & (1<<position_3bit))
		*rtn_value = 1;
	else
		*rtn_value = 0;
	
	return 1;
}

int ctrl_gpio_state_nxp_i2c_write(int position, int set_value){
	unsigned int read_data;
	unsigned char position_3bit;
	unsigned char read_byte_data, byte_data_to_write;
	
	// legal check 
	if(global_nxp_i2c_handle<=0){
		fprintf(stderr, "i2c error: %s, init()?\n", strerror(errno));
		return -1;
	}

	switch(position>>3){
		case 1:	//output
		case 3:	//output
			break;
		default:
			puts("access bank permission denied !!");
			return -1;
	}

	// read value into rtn_value
	read_data = (unsigned int)i2c_smbus_read_byte_data(global_nxp_i2c_handle, i2c_output_nxp_bank[position>>3]);
	read_byte_data = (unsigned char)read_data;
	position_3bit = 0x07 & position;
	
	usleep(10000);

	// set value
	if(set_value==1){
		byte_data_to_write = read_byte_data |(1<<position_3bit);
		i2c_smbus_write_byte_data(global_nxp_i2c_handle, i2c_output_nxp_bank[position>>3], byte_data_to_write);
	}else if(set_value==0){
		byte_data_to_write = read_byte_data & ~(1<<position_3bit);
		i2c_smbus_write_byte_data(global_nxp_i2c_handle, i2c_output_nxp_bank[position>>3], byte_data_to_write);
	}

	return 1;
}

int get_n_set_gpio(MYSQL * db_conn_p, int bank_num, int pin_num){
	int direction=1; // output
	char strToDB[512];
	char strToDBplusCmd[512];
	char tmp_time[20]="";
	char * strchr_p; 
	char ascii_pin[3];
	//int bank_num=0, pin_num=1;
	int rtn_value=0;
	int mysql_rtn;
	gpio_element_t tmp_gpio_body;

	gpio_element_t* in_gpio = &tmp_gpio_body;

	bzero((void*)in_gpio, (int)sizeof(gpio_element_t));
	// validation check
	if(!db_conn_p)
		return -1;
	
	if(in_gpio->dbTableName==NULL)
		return -1;

	//
	// 1. read DB
	//
  	// get data from DB

  	sprintf(strToDB, "SELECT * from gw_io_bank%d_0%d", bank_num, pin_num);
	if (mysql_query(db_conn_p, strToDB)){
		fprintf(stderr, "%s\n", mysql_error(db_conn_p));
		printf("mysql create database failed !!\n");
	}
  
  	MYSQL_RES *result = mysql_store_result(db_conn_p);
  	if (result == NULL){
		fprintf(stderr, "%s\n", mysql_error(db_conn_p));
		printf("mysql create database failed !!\n");
	}

  	int num_fields = mysql_num_fields(result);

  	MYSQL_ROW row;
  	MYSQL_FIELD *field;
  	int j;
  	if(row = mysql_fetch_row(result))
  	{ 
    	if(row[1])		// dbTableName
    		strcpy(in_gpio->dbTableName, row[1]);

    	if(row[2])		// pinName
    		strcpy(in_gpio->pinName, row[2]);
    	
    	if(row[3])		// onBoard
    		in_gpio->onBoard = atoi(row[3]);
    	
    	if(row[4])		// type
    		in_gpio->type = atoi(row[4]);

    	if(row[5])		// currValue
    		in_gpio->currValue = atoi(row[5]);

    	if(row[6])		// directoin
    		in_gpio->direction = atoi(row[6]);

    	if(row[7])		// setValue
    		in_gpio->setValue = atoi(row[7]);

    	if(row[8])		// setBy
    		in_gpio->setBy = atoi(row[8]);
    		
    }

    printf("we get :onBoard=%d, type=%d, currValue=%d, direction=%d, setValue=%d, setBy=%d, dbTableName=%s\n", in_gpio->onBoard, in_gpio->type, in_gpio->currValue, in_gpio->direction, in_gpio->setValue, in_gpio->setBy, in_gpio->dbTableName);

	//
	// 2. update gpio
	//
	if(bank_num == 0){	// for 5v array
    	ctrl_gpio_state(io_dir_5v_array, in_gpio->direction, pin_num-1, in_gpio->setValue, &rtn_value);
		printf("rtn_value=%d\n",rtn_value);
		in_gpio->currValue = rtn_value;
	}else if(bank_num == 1){
    	ctrl_gpio_state(io_dir_24v_array, in_gpio->direction, pin_num-1, in_gpio->setValue, &rtn_value);
		printf("rtn_value=%d\n",rtn_value);
		in_gpio->currValue = rtn_value;
	}
	//
	// 3. write DB, add an entry
	//
	// get current time
	syscall_getRsp("date \"+%Y-%m-%d %H:%M:%S\"", tmp_time);
	tmp_time[19] = '\0';
	strcpy(in_gpio->datetime, tmp_time);
	// add into DB
	sprintf(strToDB, "(dbTableName,pinName,onBoard,type,currValue,direction,setValue,setBy,datetime) VALUES ('%s','%s','%d','%d','%d','%d','%d','%d', '%s')", in_gpio->dbTableName, in_gpio->pinName, in_gpio->onBoard, in_gpio->type, in_gpio->currValue, in_gpio->direction, in_gpio->setValue, in_gpio->setBy, in_gpio->datetime);
	sprintf(strToDBplusCmd, "INSERT INTO %s %s", in_gpio->dbTableName, strToDB);
	printf("cmd to send:%s:\n", strToDBplusCmd);
	mysql_rtn = mysql_query(db_conn_p, strToDBplusCmd);
	if (mysql_rtn) {
    	puts("insert data into DB failed !!");
    	//finish_with_error(global_DB_con);
	}

	mysql_free_result(result);
	return 1;
}

int get_n_set_nxp_i2c(MYSQL * db_conn_p, int bank_num, int pin_num){
	int direction=1; // output
	char strToDB[512];
	char strToDBplusCmd[512];
	char tmp_time[20]="";
	char * strchr_p; 
	char ascii_pin[3];
	//int bank_num=0, pin_num=1;
	int rtn_value=0;
	int mysql_rtn;
	int position;
	gpio_element_t tmp_gpio_body;

	gpio_element_t* in_gpio = &tmp_gpio_body;

	bzero((void*)in_gpio, (int)sizeof(gpio_element_t));
	// validation check
	if(!db_conn_p)
		return -1;
	
	if(in_gpio->dbTableName==NULL)
		return -1;

	//
	// 1. read DB
	//
  	// get data from DB

  	sprintf(strToDB, "SELECT * from gw_nxp_bank%d_0%d", bank_num, pin_num);
	if (mysql_query(db_conn_p, strToDB)){
		fprintf(stderr, "%s\n", mysql_error(db_conn_p));
		printf("mysql create database failed !!\n");
	}
  
  	MYSQL_RES *result = mysql_store_result(db_conn_p);
  	if (result == NULL){
		fprintf(stderr, "%s\n", mysql_error(db_conn_p));
		printf("mysql create database failed !!\n");
	}

  	int num_fields = mysql_num_fields(result);

  	MYSQL_ROW row;
  	MYSQL_FIELD *field;
  	int j;
  	if(row = mysql_fetch_row(result))
  	{
  		if(row[1])		// valid
    		in_gpio->valid = atoi(row[1]);

    	if(row[2])		// dbTableName
    		strcpy(in_gpio->dbTableName, row[2]);

    	if(row[3])		// pinName
    		strcpy(in_gpio->pinName, row[3]);
    	
    	if(row[4])		// onBoard
    		in_gpio->onBoard = atoi(row[4]);
    	
    	if(row[5])		// type
    		in_gpio->type = atoi(row[5]);

    	if(row[6])		// currValue
    		in_gpio->currValue = atoi(row[6]);

    	if(row[7])		// directoin
    		in_gpio->direction = atoi(row[7]);

    	if(row[8])		// setValue
    		in_gpio->setValue = atoi(row[8]);

    	if(row[9])		// setBy
    		in_gpio->setBy = atoi(row[9]);
    		
    }

    if( (in_gpio->valid == 0)||(in_gpio->valid ==NULL) ){
    	printf("Entry of bank=%d,pin=%d is not valid !!\n",bank_num,pin_num);
    	mysql_free_result(result);
		return 2;
	}
	printf("we get :valid=%d, onBoard=%d, type=%d, currValue=%d, direction=%d, setValue=%d, setBy=%d, dbTableName=%s\n",in_gpio->valid, in_gpio->onBoard, in_gpio->type, in_gpio->currValue, in_gpio->direction, in_gpio->setValue, in_gpio->setBy, in_gpio->dbTableName);
	//
	// 2. update gpio
	//
	position = (bank_num << 3) + pin_num; 

	if(in_gpio->direction)
		ctrl_gpio_state_nxp_i2c_write(position, in_gpio->setValue);
    
    ctrl_gpio_state_nxp_i2c_read(position , &rtn_value);
	
	printf("rtn_value=%d\n",rtn_value);
	in_gpio->currValue = rtn_value;
	//
	// 3. write DB, add an entry
	//
	// get current time
	syscall_getRsp("date \"+%Y-%m-%d %H:%M:%S\"", tmp_time);
	tmp_time[19] = '\0';
	strcpy(in_gpio->datetime, tmp_time);
	// add into DB
	sprintf(strToDB, "(valid,dbTableName,pinName,onBoard,type,currValue,direction,setValue,setBy,datetime) VALUES ('1','%s','%s','%d','%d','%d','%d','%d','%d', '%s')", in_gpio->dbTableName, in_gpio->pinName, in_gpio->onBoard, in_gpio->type, in_gpio->currValue, in_gpio->direction, in_gpio->setValue, in_gpio->setBy, in_gpio->datetime);
	sprintf(strToDBplusCmd, "INSERT INTO %s %s", in_gpio->dbTableName, strToDB);
	printf("cmd to send:%s:\n", strToDBplusCmd);
	mysql_rtn = mysql_query(db_conn_p, strToDBplusCmd);
	if (mysql_rtn) {
    	puts("insert data into DB failed !!");
    	//finish_with_error(global_DB_con);
	}

	mysql_free_result(result);
	return 1;
}

void *thread_gpio_management(void *argu){
	(void) argu;
	int i;
	MYSQL * get_mysql_p = (MYSQL *)argu;
	
	iolib_init();
	while(1){
		
		for(i=1;i<9;i++)
			get_n_set_gpio(get_mysql_p, 0, i);

		for(i=1;i<9;i++)
			get_n_set_gpio(get_mysql_p, 1, i);

		usleep(500000);
		//sleep(gpio_update_interval);

	}
	iolib_free();
}

void *thread_nxp_i2c_management(void *argu){
	(void) argu;
	int i;
	MYSQL * get_mysql_p = (MYSQL *)argu;
	
	nxp_i2c_init();
	while(1){
		
		for(i=0;i<8;i++)
			get_n_set_nxp_i2c(get_mysql_p, 0, i);

		for(i=0;i<8;i++)
			get_n_set_nxp_i2c(get_mysql_p, 1, i);

		for(i=0;i<8;i++)
			get_n_set_nxp_i2c(get_mysql_p, 2, i);

		for(i=0;i<8;i++)
			get_n_set_nxp_i2c(get_mysql_p, 3, i);					

		sleep(5);
		//sleep(gpio_update_interval);

	}
	nxp_i2c_release();
}