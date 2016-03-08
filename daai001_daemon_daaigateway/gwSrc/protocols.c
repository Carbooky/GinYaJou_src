
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <math.h>
#include "protocols.h"

#include "lib_crc16.h"
#include "util.h"

#include "gateway_api.h"

#define INVERTER_DELTA_WR_REG  0x031f
#define INVERTER_DELTA_ERR_REG 0x0320

#define EVERSOLAR_CTRLCODE_REG 0x10
#define EVERSOLAR_CTRLCODE_RD  0x11
#define EVERSOLAR_CTRLCODE_WR  0x12
#define EVERSOLAR_CTRLCODE_EXE 0x13

/****************************
** Global
****************************/

uint16_t scan_inv_types = SCAN_INVERTER_TYPE;

uint8_t LCD_total_id=255;
uint8_t LCD_curr_id=1;
uint32_t LCD_invType=USE_MOTECH; 

inverter_msg2host_new_t inverter_msg2host_motech_array[256*USE_NUM_OF_CHANNEL];
inverter_ali_raw_t inverter_msg2host_ali_array[256*USE_NUM_OF_CHANNEL];
inverter_msg2host_eversolar_t inverter_msg2host_eversolar_array[256*USE_NUM_OF_CHANNEL];
inverter_msg2host_delta_t inverter_msg2host_delta_array[256*USE_NUM_OF_CHANNEL];
inverter_enersolis_raw_t inverter_msg2host_enersolis_array[256*USE_NUM_OF_CHANNEL];

inverter_msg2host_pyranometer_t inverter_msg2host_pyranometer_array[256*USE_NUM_OF_CHANNEL];  // entry should be the same as PYRANO_THERMO_MAX+1
inverter_msg2host_thermometer_t inverter_msg2host_thermograph_array[256*USE_NUM_OF_CHANNEL];  // entry should be the same as PYRANO_THERMO_MAX+1
inverter_msg2host_thermometer_t inverter_msg2host_thermometer_array[256*USE_NUM_OF_CHANNEL];  // entry should be the same as PYRANO_THERMO_MAX+1

inverter_msg2host_digimeter_t inverter_msg2host_digimeter_array[256*USE_NUM_OF_CHANNEL];

inverter_msg2host_jda_t inverter_msg2host_jda_array[256*USE_NUM_OF_CHANNEL];

int inv_digimeter_state_chg_next(inv_digimeter_state_t* curr_inv_cmd_state, inv_cmd_event_t in_event);

int inverter_scan_all(scan_inv_info_t* inv_info, char* scan_port_name, eversolar_list_t** eversolar_list_head){
	//uint8_t result_type;
	scan_inv_info_t tmp_info, total_info, pyrano_tmp_info;
	int i, slot_thermometer=0, slot_thermograph=0;
	uint8_t tmp_addr;
	int rtn;
	scan_inv_info_t sys_inv_info_pyranometer;
	scan_inv_info_t sys_inv_info_thermometer;
	scan_inv_info_t sys_inv_info_thermograph;

	if(inv_info == NULL)
		return -1;

	// initialized to zero
	bzero(&total_info, sizeof(scan_inv_info_t));

	if(scan_inv_types & (USE_PYRANOMETER|USE_THERMOGRAPH|USE_THERMOMETER) ){
	
		bzero(&pyrano_tmp_info, sizeof(scan_inv_info_t));
		bzero(&tmp_info, sizeof(scan_inv_info_t));

		for(i=0;i<2;i++){

			printf("Scan Pyranometer, Thermo ... %d times\n", i+1);
			rtn = inverter_scan_pyranometer(&pyrano_tmp_info, scan_port_name, 0xffff);
			if(rtn < 0)
				return -1;

			if(pyrano_tmp_info.num > tmp_info.num){
				memcpy(&tmp_info, &pyrano_tmp_info, sizeof(scan_inv_info_t));
				puts("pyrano scan get greater number !!");
			}
			sleep(1);
		}

		bzero(&sys_inv_info_pyranometer, sizeof(scan_inv_info_t));
		bzero(&sys_inv_info_thermometer, sizeof(scan_inv_info_t));
		bzero(&sys_inv_info_thermograph, sizeof(scan_inv_info_t));

		if(tmp_info.num != 0){
			// add these new inverter's id
			for(i=0;i<tmp_info.num;i++){
				tmp_addr = tmp_info.invInfo[i].invId;
				
				if(tmp_addr <= PYRANOMETER_MAX){	// this item is Pyranometer

					sys_inv_info_pyranometer.num = sys_inv_info_pyranometer.num + 1;
					sys_inv_info_pyranometer.invInfo[i].invId = tmp_addr;

					total_info.invInfo[i].invId = tmp_addr;
					total_info.invInfo[i].invType = USE_PYRANOMETER;
					continue;
				}

				if(tmp_addr <= THERMOGRAPH_MAX){	// this item is Pyranometer

					sys_inv_info_thermograph.num = sys_inv_info_thermograph.num + 1;
					sys_inv_info_thermograph.invInfo[slot_thermograph].invId = tmp_addr;

					total_info.invInfo[i].invId = tmp_addr;
					total_info.invInfo[i].invType = USE_THERMOGRAPH;
					slot_thermograph ++;
					continue;
				}

				if(tmp_addr <= PYRANO_THERMO_MAX){		// this item is Thermometer
					
					sys_inv_info_thermometer.num = sys_inv_info_thermometer.num + 1;
					sys_inv_info_thermometer.invInfo[slot_thermometer].invId = tmp_addr;	// fill the first slot of thermometer with first value

					total_info.invInfo[i].invId = tmp_addr;
					total_info.invInfo[i].invType = USE_THERMOMETER;
					slot_thermometer ++;
				}
			}	
			
			// increment the total amount with the newly found inverters
			total_info.num += tmp_info.num;
			printf("total #%d of Pyranometer(s), #%d of Thermograph(s), #%d of Thermometer(s) found on %s !!", sys_inv_info_pyranometer.num,sys_inv_info_thermograph.num, sys_inv_info_thermometer.num, scan_port_name);
		
		}else
			printf("No any Pyranometer or Thermometer was found on %s!!\n", scan_port_name);
	}

	// scan jda
	if(scan_inv_types & USE_JDA){
		bzero(&pyrano_tmp_info, sizeof(scan_inv_info_t));
		bzero(&tmp_info, sizeof(scan_inv_info_t));

		for(i=0;i<1;i++){

			printf("Scan JDA Pyranometer, Thermo ... %d times\n", i+1);
			rtn = inverter_scan_jda(&pyrano_tmp_info, scan_port_name, 0xffff);
			if(rtn < 0)
				return -1;

			if(pyrano_tmp_info.num > tmp_info.num){
				memcpy(&tmp_info, &pyrano_tmp_info, sizeof(scan_inv_info_t));
				puts("pyrano scan get greater number !!");
			}
			usleep(500000);
		}

		bzero(&sys_inv_info_pyranometer, sizeof(scan_inv_info_t));
		bzero(&sys_inv_info_thermometer, sizeof(scan_inv_info_t));
		bzero(&sys_inv_info_thermograph, sizeof(scan_inv_info_t));

		if(tmp_info.num != 0){
			// add these new inverter's id
			for(i=0;i<tmp_info.num;i++){
				tmp_addr = tmp_info.invInfo[i].invId;
				
				if(tmp_addr <= PYRANOMETER_MAX){	// this item is Pyranometer

					sys_inv_info_pyranometer.num = sys_inv_info_pyranometer.num + 1;
					sys_inv_info_pyranometer.invInfo[i].invId = tmp_addr;

					total_info.invInfo[i].invId = tmp_addr;
					total_info.invInfo[i].invType = USE_JDA;
					continue;
				}

				if(tmp_addr <= THERMOGRAPH_MAX){	// this item is Pyranometer

					sys_inv_info_thermograph.num = sys_inv_info_thermograph.num + 1;
					sys_inv_info_thermograph.invInfo[slot_thermograph].invId = tmp_addr;

					total_info.invInfo[i].invId = tmp_addr;
					total_info.invInfo[i].invType = USE_JDA_TG;
					slot_thermograph ++;
					continue;
				}

				if(tmp_addr <= PYRANO_THERMO_MAX){		// this item is Thermometer
					
					sys_inv_info_thermometer.num = sys_inv_info_thermometer.num + 1;
					sys_inv_info_thermometer.invInfo[slot_thermometer].invId = tmp_addr;	// fill the first slot of thermometer with first value

					total_info.invInfo[i].invId = tmp_addr;
					total_info.invInfo[i].invType = USE_JDA_TM;
					slot_thermometer ++;
				}
			}	
			
			// increment the total amount with the newly found inverters
			total_info.num += tmp_info.num;
			printf("total #%d of JDA Pyranometer(s), #%d of Thermograph(s), #%d of Thermometer(s) found on %s !!", sys_inv_info_pyranometer.num,sys_inv_info_thermograph.num, sys_inv_info_thermometer.num, scan_port_name);
		
		}else
			printf("No any JDA Pyranometer or Thermometer was found on %s!!\n", scan_port_name);
	}

	// scan digimeter
	if(scan_inv_types & USE_DIGIMETER){
		puts("Scan Digimeter ...");
		//rtn = inverter_scan_digimeter(&tmp_info, scan_port_name, 0xffff);
		rtn = inverter_scan_digimeter(&tmp_info, scan_port_name);

		if(rtn<0)
			return -1;

		if(tmp_info.num != 0){

			// add these new inverter's id
			for(i=0;i<tmp_info.num;i++){
				total_info.invInfo[i+total_info.num].invId = tmp_info.invInfo[i].invId;
				total_info.invInfo[i+total_info.num].invType = USE_DIGIMETER;
			}
			
			// increment the total amount with the newly found inverters
			total_info.num += tmp_info.num;
			//sys_inv_info_motech.num = tmp_info.num;

			printf("total #%d of Digimeter(s) found on %s !!", tmp_info.num, scan_port_name);

		}else
			puts("No any digimeter device found !!");
	}

	if(scan_inv_types & USE_MOTECH){
		puts("Scan Motech ...");
		rtn = inverter_scan_motech(&tmp_info, scan_port_name);
		
		if(rtn<0)
			return -1;

		if(tmp_info.num != 0){

			// add these new inverter's id
			for(i=0;i<tmp_info.num;i++){
				total_info.invInfo[i+total_info.num].invId = tmp_info.invInfo[i].invId;
				total_info.invInfo[i+total_info.num].invType = USE_MOTECH;
			}
			
			// increment the total amount with the newly found inverters
			total_info.num += tmp_info.num;
			//sys_inv_info_motech.num = tmp_info.num;

			printf("total #%d of Motech inverter(s) found on %s !!", tmp_info.num, scan_port_name);

		}else
			puts("No any inverter of brand Motech !!");
	}

	if(scan_inv_types & USE_TOUGH){
		puts("Scan ALLIS ...");
		rtn = inverter_scan_ali(&tmp_info, scan_port_name);
		
		if(rtn < 0)
			return -1;

		if(tmp_info.num != 0){

			// add these new inverter's id
			for(i=0;i<tmp_info.num;i++){
				total_info.invInfo[i+total_info.num].invId = tmp_info.invInfo[i].invId;
				total_info.invInfo[i+total_info.num].invType = USE_TOUGH;
			}
			
			// increment the total amount with the newly found inverters
			total_info.num += tmp_info.num;
			//sys_inv_info_ali.num = tmp_info.num;

			printf("total #%d of Allis inverter(s) found on %s !!", tmp_info.num, scan_port_name);
		}else
			puts("No any inverter of brand Allis !!");
	}

	if(scan_inv_types & USE_EVERSOLAR){
		printf("Start Scan Eversolar @%s\n", scan_port_name);
		//inverter_scan_eversolar(&tmp_info);
		//rtn = invInfoGet_Eversolar(0, (void*)&tmp_info, TRUE, scan_port_name, eversolar_list_head);
		rtn = invInfoGet_Zeversolar(0, (void*)&tmp_info, TRUE, scan_port_name, eversolar_list_head);
		
		if(rtn <0)
			return -1;

		if(tmp_info.num != 0){

			// add these new inverter's id
			for(i=0;i<tmp_info.num;i++){
				total_info.invInfo[i+total_info.num].invId = tmp_info.invInfo[i].invId;
				total_info.invInfo[i+total_info.num].invType = USE_EVERSOLAR;
			}
			
			// increment the total amount with the newly found inverters
			total_info.num += tmp_info.num;
			//sys_inv_info_eversolar.num = tmp_info.num;

			printf("total #%d of Eversolar inverter(s) found on %s !!", tmp_info.num, scan_port_name);

		}else
			puts("No any inverter of brand Eversolar !!");
	}

	if(scan_inv_types & USE_DELTA){
		puts("Scan DELTA ...");
		//inverter_scan_eversolar(&tmp_info);
		rtn = inverter_scan_delta(&tmp_info, scan_port_name);

		if(rtn < 0)
			return -1;

		if(tmp_info.num != 0){

			// add these new inverter's id
			for(i=0;i<tmp_info.num;i++){
				total_info.invInfo[i+total_info.num].invId = tmp_info.invInfo[i].invId;
				total_info.invInfo[i+total_info.num].invType = USE_DELTA;
			}
			
			// increment the total amount with the newly found inverters
			total_info.num += tmp_info.num;
			//sys_inv_info_delta.num = tmp_info.num;

			printf("total #%d of Delta inverter(s) found on %s !!", tmp_info.num, scan_port_name);

		}else
			puts("No any inverter of brand Delta !!");
	}

	if(scan_inv_types & USE_ENERSOLIS){
		puts("Scan ENERSOLIS ...");
		//inverter_scan_eversolar(&tmp_info);
		rtn = inverter_scan_enersolis(&tmp_info, scan_port_name);
		
		if(rtn <0)
			return -1;

		if(tmp_info.num != 0){

			// add these new inverter's id
			for(i=0;i<tmp_info.num;i++){
				total_info.invInfo[i+total_info.num].invId = tmp_info.invInfo[i].invId;
				total_info.invInfo[i+total_info.num].invType = USE_ENERSOLIS;
			}
			
			// increment the total amount with the newly found inverters
			total_info.num += tmp_info.num;
			//sys_inv_info_enersolis.num = tmp_info.num;

			printf("total #%d of EnerSolis inverter(s) found on %s !!", tmp_info.num, scan_port_name);

		}else
			puts("No any inverter of brand EnerSolis !!");
	}


	total_info.invInfo[total_info.num].invId = 0;

	printf("Test print, current total num=%d\n", total_info.num);

	memcpy(inv_info, &total_info, sizeof(scan_inv_info_t));

	return 1;
}

/****************************
** Pyranometer protocol
****************************/
int inv_pyranometer_state_chg_next(inv_pyranometer_state_t* curr_inv_cmd_state, inv_cmd_event_t in_event){
	
	switch (*curr_inv_cmd_state) {
		case INV_PYRANOMETER_STATE_INIT:
			if(!in_event)	// event normal
				*curr_inv_cmd_state = INV_PYRANOMETER_STATE_RDY;
			else
				*curr_inv_cmd_state = INV_PYRANOMETER_STATE_STOP;
		break;	
		case INV_PYRANOMETER_STATE_RDY:
			if(in_event == INV_CMD_EVENT_SCAN)
				*curr_inv_cmd_state = INV_PYRANOMETER_STATE_SCAN_RUN;
			else if(in_event == INV_CMD_EVENT_NORMAL)
				*curr_inv_cmd_state = INV_PYRANOMETER_STATE_1ST;
			else if (in_event&(INV_CMD_EVENT_ERR|INV_CMD_EVENT_STOP))
				*curr_inv_cmd_state = INV_PYRANOMETER_STATE_STOP;
		break;
		case INV_PYRANOMETER_STATE_ERR:
			if(!in_event)
				*curr_inv_cmd_state = INV_PYRANOMETER_STATE_RDY;
			else
				*curr_inv_cmd_state = INV_PYRANOMETER_STATE_STOP;
		break;
		case INV_PYRANOMETER_STATE_STOP:
			if(!in_event)
				*curr_inv_cmd_state = INV_PYRANOMETER_STATE_RDY;
			else
				*curr_inv_cmd_state = INV_PYRANOMETER_STATE_STOP;
		break;
		case INV_PYRANOMETER_STATE_TIMEOUT:
			if(!in_event)
				*curr_inv_cmd_state = INV_PYRANOMETER_STATE_ERR;
			else
				*curr_inv_cmd_state = INV_PYRANOMETER_STATE_STOP;
		break;
		
		case INV_PYRANOMETER_STATE_1ST:
		case INV_PYRANOMETER_STATE_2ND:
		case INV_PYRANOMETER_STATE_3ST:
			if (in_event == INV_CMD_EVENT_NOCHANGE)
				*curr_inv_cmd_state = *curr_inv_cmd_state;
			else if(in_event == INV_CMD_EVENT_NORMAL)	// event normal
				*curr_inv_cmd_state = *curr_inv_cmd_state + 1;
			else if (in_event&INV_CMD_EVENT_TIMEOUT)
				*curr_inv_cmd_state = INV_PYRANOMETER_STATE_TIMEOUT;
			else if (in_event&INV_CMD_EVENT_ERR)
				*curr_inv_cmd_state = INV_PYRANOMETER_STATE_ERR;
			else if (in_event&INV_CMD_EVENT_STOP)
				*curr_inv_cmd_state = INV_PYRANOMETER_STATE_STOP;
			else if (in_event&INV_CMD_EVENT_END)
				*curr_inv_cmd_state = INV_PYRANOMETER_STATE_END;
		break;
		
		case INV_PYRANOMETER_STATE_SCAN_RUN:
			if(in_event == INV_CMD_EVENT_STOP)
				*curr_inv_cmd_state = INV_PYRANOMETER_STATE_STOP;
			else if (in_event == INV_CMD_EVENT_END)
				*curr_inv_cmd_state = INV_PYRANOMETER_STATE_SCAN_END;
			else 
				*curr_inv_cmd_state = *curr_inv_cmd_state;
		break;
		
		case INV_PYRANOMETER_STATE_END:
			if(in_event == INV_CMD_EVENT_NORMAL)	// event normal
				*curr_inv_cmd_state = INV_PYRANOMETER_STATE_RDY;
			else if(in_event == INV_CMD_EVENT_END)
				*curr_inv_cmd_state = INV_PYRANOMETER_STATE_INIT;
			else
				*curr_inv_cmd_state = INV_PYRANOMETER_STATE_STOP;
		break;
		
		case INV_PYRANOMETER_STATE_SCAN_END:
			if(in_event == INV_CMD_EVENT_STOP)
				*curr_inv_cmd_state = INV_PYRANOMETER_STATE_STOP;
		
		default:;
	}
	
	return 1;
}


bool_t fun_invIsPyranometer(uint16_t value){
	if(value >= 799){
		if(value <= 1199)
			return TRUE;

		return FALSE;
	}
	return FALSE;
}

int inverter_scan_pyranometer(scan_inv_info_t* inv_info, char* scan_port_name, uint16_t offset){

  	int fd_rs485;

  	uint8_t rcv_buff[128];
  	uint8_t *bufptr;
  	//char str_1[16];

  	int i;
	raw_485_t raw_485_pkg;
	uint32_t req_size, rsp_size, bytes, nbytes;

	//const char invBnTough[6] = "TOUGH";
	bool_t invIsPyranometer, invIsThermometer, invIsThermograph;
  	uint8_t rs232_timeout = FALSE;

    char curr_value[8];
    int rtn_float=0;	  	
	  		
	int net_timeout_cnt = 0;
  
  	int valid_cnt=0;

  	unsigned char * tmp_bufp;
	uint16_t inv_addr=1 /*1 means start from 2*/, inv_reg2rd, actual_id=0; //, pyranometer_16_value=0;
	
	inv_pyranometer_state_t inv_cmd_state = INV_PYRANOMETER_STATE_INIT;
	uint8_t cpy_size;

	float test_reassemble_float=0;
	uint16_t disp_value, lo_limit, hi_limit, digit;

	//
	// Initial UART
	if(!scan_port_name)
		return -1;

  	fd_rs485 = open_port_RS485(scan_port_name, O_RDWR| O_NOCTTY);

  	if(fd_rs485 < 0){
  		printf("Open port %s failed !!", scan_port_name);
  		return -1;
  	}

   
	//printf("Current BaudRate=%d\n",getbaud(fd_rs485) );
	
	while(1){

		// print current state
		//printf("inv_cmd_state=%d\n", inv_cmd_state);

		switch (inv_cmd_state){

		case INV_PYRANOMETER_STATE_INIT:
			inv_reg2rd = 0;
			inv_info->num = 0;
		break;

		case INV_PYRANOMETER_STATE_RDY:
			cpy_size = 0;
			inv_reg2rd = 0;
		break;
		
		case INV_PYRANOMETER_STATE_TIMEOUT:
			cpy_size = 0;
			inv_reg2rd = 0;
		break;
		
		case INV_PYRANOMETER_STATE_STOP:
			cpy_size = 0;
			inv_reg2rd = 0;
		break;
		
		case INV_PYRANOMETER_STATE_SCAN_RUN:
			inv_reg2rd = 0x0100;
			inv_addr = inv_addr + 1;
			//cpy_ptr = tmp_char;
			req_size = 9;  // nine word
			cpy_size = req_size * 2;  // in bytes
			invIsPyranometer = FALSE;
			invIsThermometer = FALSE;
			invIsThermograph = FALSE;
		break;

		case INV_PYRANOMETER_STATE_SCAN_END:
		//initial
			//cpy_ptr = NULL;
			cpy_size = 0;
			//sem_wait(&sem_rs232);	
		break;
		
		default:
		// display
			printf("default state, inv_cmd_state=%d\n", inv_cmd_state);
		// next
			inv_pyranometer_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_STOP);
		}	// switch case
	//
	// Does task, then update state
	// 
		
		if (inv_cmd_state == INV_PYRANOMETER_STATE_INIT)	{
			//printf("RS232 Initial again !!\n");
			//sleep(1);
			inv_pyranometer_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NORMAL);
		}
		else if(inv_cmd_state == INV_PYRANOMETER_STATE_RDY)
			inv_pyranometer_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_SCAN);
		else if (inv_cmd_state == INV_PYRANOMETER_STATE_TIMEOUT){
			inv_pyranometer_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_STOP);
		}
		else if(inv_cmd_state == INV_PYRANOMETER_STATE_SCAN_END)
		{
			puts("Finish Scan process!!\n");
			
			// dump message
			
			inv_pyranometer_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_STOP);
			printf("To the end of Inverter probe !!\n");
			
			// break the while
			break;
		}
		else if(inv_cmd_state == INV_PYRANOMETER_STATE_STOP)
		{
			printf("reach Pyranometer SCAN stop state !!\n");
			close(fd_rs485);
			return 0;
			//inv_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NORMAL);
		}
		else if(inv_cmd_state&0xf00)
		{
			// check if it reach the end
			if(inv_addr > PYRANO_THERMO_MAX - 1)
				inv_pyranometer_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_END);

			// add this to clear any duplicate data
			clear_serial_rx_buffer(fd_rs485);
			
			bzero(&raw_485_pkg, sizeof(raw_485_pkg));
			raw_485_pkg.pkt_type = mt81xx_cmd_read;	
			
			if(create_modbus_read(&raw_485_pkg.uniFrame.im_modbus_format_read, inv_addr, inv_reg2rd , req_size)<0)
				printf("Create Read Cmd to Rs485 failed!!\n");
			if(append_CRC16((modbus_format_read_t*)&raw_485_pkg.uniFrame.im_modbus_format_read.modbus_offset_SlaveAddr, 6)<0)
				printf("Adding CRC16 error !!\n");
			//tmp_p = (uint8_t*)&(raw_485_pkg.uniFrame.im_modbus_format_read);
    		//printf("What we have created 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x \n",tmp_p[0], tmp_p[1], tmp_p[2], tmp_p[3], tmp_p[4], tmp_p[5], tmp_p[6],tmp_p[7]);
    		printf("Scan PyranoMeter inv# = %d on %s\n", inv_addr, scan_port_name);

			//sprintf(str_1, "Pyro.%d_256", inv_addr);
    		//string_USB_LCD("/dev/ttyACM0", str_1, 1);

			i=0;

			i = sizeof(raw_485_pkg.uniFrame.im_modbus_format_read);

			//select Tx
			//write(fd_gpio88, "1", 1);
			write(fd_rs485, &(raw_485_pkg.uniFrame.im_modbus_format_read), i);
	  	
	  		usleep(WAITING_TIME_AFTER_ISSUE_CMD);
	  	
			//process Rx
	  		bufptr = rcv_buff;
			nbytes=0;
			bzero(bufptr, 128);
			rs232_timeout=FALSE;
	  		rsp_size = req_size*2 + 5;
            
	  		//printf("rsp_size should be %d\n", rsp_size);
	  		for(;;){ // do rs485 receiving during 3 second, or timeout
	  	  	// check if there's data in com port, or time out
				
				if(net_timeout_cnt==RETRY_TIMES_FOR_ONE_SCAN/*isready(fd_rs485)==0*/){
	  				//rs232_timeout=TRUE;
	  				puts("Crash");
	  				rs232_timeout=INV_MSG_CRASH;
				
	  				break;
	  			}
	  		
				// read it !!
                bytes = read(fd_rs485, (char*)bufptr, rsp_size);        // it will always receive

                nbytes +=bytes;
                bufptr += bytes;
                // check satisfy
        		if(bytes <= 0){         // get no data at all before timeout
                        rs232_timeout=INV_MSG_CRASH;
                        //puts("Scan Crash");
                        break;
                }else if(rsp_size == nbytes){ // get exact data before timeout
                        rs232_timeout=FALSE;
                        break;
                }else{          // get partial data, keep watching !!
                        rs232_timeout=FALSE;
                        //break;
                }
        		usleep(WAITING_TIME_EACH_SCAN_SENSOR); //0.4 s
                net_timeout_cnt ++;

	  		}

	  		// pre-fetch the value
 			if(rsp_size == nbytes){
 				//printf("rs485RTN:[0]=%x,[1]=%x,[2]=%x,[3]=%x,[4]=%x\n", rcv_buff[0], rcv_buff[1], rcv_buff[2], rcv_buff[3], rcv_buff[4]);
 				bufptr = rcv_buff + 3;	// locate pointer to MODBUS 1st Word
               	disp_value = bufptr[0];
               	disp_value = disp_value << 8;
               	disp_value = disp_value + bufptr[1];
 				lo_limit = bufptr[2];
               	lo_limit = lo_limit << 8;
               	lo_limit = lo_limit + bufptr[3];
 				hi_limit = bufptr[4];
               	hi_limit = hi_limit << 8;
               	hi_limit = hi_limit + bufptr[5];
 				digit = bufptr[16];
               	digit = digit << 8;
               	digit = digit + bufptr[17];
               	
               	printf("digit=%d\n", digit);
 				
 				if (digit == 0)
               	    digit = 1;
               	else
               	    digit = digit * 10;
 				
 				test_reassemble_float = disp_value;
               	test_reassemble_float = test_reassemble_float / digit;
               	
               	if((test_reassemble_float < 0)||(test_reassemble_float > 2000))
               		test_reassemble_float = 0;
              	if (digit ==1)
              		rtn_float = sprintf(curr_value, "%.0f", test_reassemble_float);
              	else
              		rtn_float = sprintf(curr_value, "%.1f", test_reassemble_float);
              	if(rtn_float > (int)sizeof(curr_value)){
              		puts("Error while get the Pyranometer value !!");
              		rs232_timeout=INV_MSG_CRASH;
              	}
              		printf("float value might be %s\n", curr_value);
            }

			valid_cnt=0;
			invIsPyranometer = FALSE;
			invIsThermometer = FALSE;
			invIsThermograph = FALSE;


			if(rs232_timeout==FALSE){
				if(nbytes){
					tmp_bufp = rcv_buff;
					
					if(check_CRC16((modbus_format_rtn_t *)tmp_bufp)>0){
						// copy the data
						tmp_bufp += 2; // locate ptr to pointing "Byte Count"
						
						if( cpy_size == *((uint8_t*)tmp_bufp) ){
							actual_id = inv_addr + offset;
							if (inv_addr <= PYRANOMETER_MAX){
								invIsPyranometer = TRUE;   // currently, we assume this is a valid pyranometer upon correct response size
								if(offset != 0xffff){
									inverter_msg2host_pyranometer_array[actual_id].inverterId = actual_id;
									strcpy(inverter_msg2host_pyranometer_array[actual_id].value, curr_value);
								}else
									puts("In pyranometer scanning, we only get list, but don't update slot data !!");
							}else if (inv_addr <= THERMOGRAPH_MAX){
								invIsThermograph = TRUE;   // currently, we assume this is a valid pyranometer upon correct response size
								if(offset != 0xffff){
									inverter_msg2host_thermograph_array[actual_id].inverterId = actual_id;
									strcpy(inverter_msg2host_thermograph_array[actual_id].value, curr_value);
								}else
									puts("In pyranometer scanning, we only get list, but don't update slot data !!");
							}else {
								invIsThermometer = TRUE;   // currently, we assume this is a valid pyranometer upon correct response size
								if(offset != 0xffff){
									inverter_msg2host_thermometer_array[actual_id].inverterId = actual_id;
									strcpy(inverter_msg2host_thermometer_array[actual_id].value, curr_value);
								}else
									puts("In pyranometer scanning, we only get list, but don't update slot data !!");
							}
	  		  			}else
	  		  				printf("copy size mismatch, copy abort, data size in rtn is %d, cpy_size == %d\n", tmp_bufp[0], cpy_size);
	  		  		
							// modify count
							nbytes = valid_cnt;
							valid_cnt=0;					
					}else{
						printf("receive RS485 packet with CRC error !!\n");	
						
					}
				}
			}
	  	
			//change state
			if(rs232_timeout==INV_MSG_CRASH){
				inv_pyranometer_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NORMAL);
			}
    		else if(rs232_timeout==FALSE && invIsPyranometer){
    			inv_pyranometer_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NORMAL);	
    			printf("Pyranometer address=%d found on %s\n", inv_addr, scan_port_name);
    			inv_info->num = inv_info->num + 1;
    			inv_info->invInfo[inv_info->num -1].invId = inv_addr;
    			inv_info->invInfo[inv_info->num -1].invType = USE_PYRANOMETER;
			}
    		else if(rs232_timeout==FALSE && invIsThermograph){
    			inv_pyranometer_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NORMAL);	
    			printf("Thermograph address=%d found on %s\n", inv_addr, scan_port_name);
    			inv_info->num = inv_info->num + 1;
    			inv_info->invInfo[inv_info->num -1].invId = inv_addr; 
    			inv_info->invInfo[inv_info->num -1].invType = USE_THERMOGRAPH;
    		} 			
    		else if(rs232_timeout==FALSE && invIsThermometer){
    			inv_pyranometer_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NORMAL);	
    			printf("Thermometer address=%d found on %s\n", inv_addr, scan_port_name);
    			inv_info->num = inv_info->num + 1;
    			inv_info->invInfo[inv_info->num -1].invId = inv_addr;
    			inv_info->invInfo[inv_info->num -1].invType = USE_THERMOMETER;	    			
			}else{
				printf("RS485 timeout !!\n");
				inv_pyranometer_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_TIMEOUT);
			}
		}
		else
			inv_pyranometer_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NORMAL);

		usleep(WAITING_TIME_EACH_SENSOR);
	}

	close(fd_rs485);
	return 1;
}

int convert_invmsg_to_asci_pyranometer(char *buff, inverter_msg2host_pyranometer_t* msg2host){
	char tmp_time[15]="";

	syscall_getRsp("date +%Y%m%d%H%M%S", tmp_time);
	tmp_time[14] = '\0';

	sprintf(buff, "%.4d,PYRANOMETER,%s,%s", msg2host->inverterId, tmp_time, msg2host->value);

	return 1; 
	
}

int convert_invmsg_to_asci_thermograph(char *buff, inverter_msg2host_thermometer_t* msg2host){
	char tmp_time[15]="";

	syscall_getRsp("date +%Y%m%d%H%M%S", tmp_time);
	tmp_time[14] = '\0';

	sprintf(buff, "%.4d,THERMOGRAPH,%s,%s", msg2host->inverterId, tmp_time, msg2host->value);

	return 1; 
	
}

int convert_invmsg_to_asci_thermometer(char *buff, inverter_msg2host_thermometer_t* msg2host){
	char tmp_time[15]="";

	syscall_getRsp("date +%Y%m%d%H%M%S", tmp_time);
	tmp_time[14] = '\0';

	sprintf(buff, "%.4d,THERMOMETER,%s,%s", msg2host->inverterId, tmp_time, msg2host->value);

	return 1; 
	
}

int inverter_scan_jda(scan_inv_info_t* inv_info, char* scan_port_name, uint16_t offset){

  	int fd_rs485;

  	uint8_t rcv_buff[128];
  	uint8_t *bufptr;

  	int i, increment=0;
	raw_485_t raw_485_pkg;
	uint32_t req_size, rsp_size, bytes, nbytes;

	//const char invBnTough[6] = "TOUGH";
	bool_t invIsPyranometer, invIsThermometer, invIsThermograph;
  	uint8_t rs232_timeout = FALSE;
    
    char curr_value[8];
    int rtn_float=0;	
  
  	int valid_cnt=0;

  	unsigned char * tmp_bufp;
	uint16_t inv_addr=1 , inv_reg2rd, actual_id=0; //, pyranometer_16_value=0;
	
	inv_pyranometer_state_t inv_cmd_state = INV_PYRANOMETER_STATE_INIT;
	uint8_t cpy_size;

	float test_reassemble_float=0;
	uint16_t disp_value, digit;

	int net_timeout_cnt = 0;

	//
	// Initial UART
	if(!scan_port_name)
		return -1;

  	fd_rs485 = open_port_RS485(scan_port_name, O_RDWR| O_NOCTTY);

  	if(fd_rs485 < 0){
  		printf("Open port %s failed !!", scan_port_name);
  		return -1;
  	}

   
	//printf("Current BaudRate=%d\n",getbaud(fd_rs485) );
	
	while(1){

		// print current state
		//printf("inv_cmd_state=%d\n", inv_cmd_state);

		switch (inv_cmd_state){

		case INV_PYRANOMETER_STATE_INIT:
			inv_reg2rd = 0;
			inv_info->num = 0;
		break;

		case INV_PYRANOMETER_STATE_RDY:
			cpy_size = 0;
			inv_reg2rd = 0;
		break;
		
		case INV_PYRANOMETER_STATE_TIMEOUT:
			cpy_size = 0;
			inv_reg2rd = 0;
		break;
		
		case INV_PYRANOMETER_STATE_STOP:
			cpy_size = 0;
			inv_reg2rd = 0;
		break;
		
		case INV_PYRANOMETER_STATE_SCAN_RUN:
			inv_reg2rd = 0x0200;
			
			if(increment==2){
				inv_addr = inv_addr + 1;
				increment=0;
			}

			//cpy_ptr = tmp_char;
			req_size = 2;  // two word
			cpy_size = req_size * 2;  // in bytes
			invIsPyranometer = FALSE;
			invIsThermometer = FALSE;
			invIsThermograph = FALSE;
		break;

		case INV_PYRANOMETER_STATE_SCAN_END:
		//initial
			//cpy_ptr = NULL;
			cpy_size = 0;
			//sem_wait(&sem_rs232);	
		break;
		
		default:
		// display
			printf("default state, inv_cmd_state=%d\n", inv_cmd_state);
		// next
			inv_pyranometer_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_STOP);
		}	// switch case
	//
	// Does task, then update state
	// 
		
		if (inv_cmd_state == INV_PYRANOMETER_STATE_INIT)	{
			//printf("RS232 Initial again !!\n");
			//sleep(1);
			inv_pyranometer_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NORMAL);
		}
		else if(inv_cmd_state == INV_PYRANOMETER_STATE_RDY)
			inv_pyranometer_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_SCAN);
		else if (inv_cmd_state == INV_PYRANOMETER_STATE_TIMEOUT){
			inv_pyranometer_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_STOP);
		}
		else if(inv_cmd_state == INV_PYRANOMETER_STATE_SCAN_END)
		{
			puts("Finish Scan process!!\n");
			
			// dump message
			
			inv_pyranometer_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_STOP);
			printf("To the end of Inverter probe !!\n");
			
			// break the while
			break;
		}
		else if(inv_cmd_state == INV_PYRANOMETER_STATE_STOP)
		{
			printf("reach Pyranometer SCAN stop state !!\n");
			close(fd_rs485);
			return 0;
			//inv_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NORMAL);
		}
		else if(inv_cmd_state&0xf00)
		{
			// check if it reach the end
			if(inv_addr > PYRANO_THERMO_MAX - 1)
				inv_pyranometer_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_END);

			// add this to clear any duplicate data
			clear_serial_rx_buffer(fd_rs485);
			
			bzero(&raw_485_pkg, sizeof(raw_485_pkg));
			raw_485_pkg.pkt_type = mt81xx_cmd_read;	
			
			if(create_modbus_read(&raw_485_pkg.uniFrame.im_modbus_format_read, inv_addr, inv_reg2rd , req_size)<0)
				printf("Create Read Cmd to Rs485 failed!!\n");
			if(append_CRC16((modbus_format_read_t*)&raw_485_pkg.uniFrame.im_modbus_format_read.modbus_offset_SlaveAddr, 6)<0)
				printf("Adding CRC16 error !!\n");
			//tmp_p = (uint8_t*)&(raw_485_pkg.uniFrame.im_modbus_format_read);
    		//printf("What we have created 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x \n",tmp_p[0], tmp_p[1], tmp_p[2], tmp_p[3], tmp_p[4], tmp_p[5], tmp_p[6],tmp_p[7]);
    		printf("Scan PyranoMeter inv# = %d on %s\n", inv_addr, scan_port_name);

#ifdef USE_LCD
    		char str_1[16];
  			char str_2[16];
    		if(offset==0xffff){
    			sprintf(str_1, "Scan JDA #%02d/10", inv_addr);
    			string_USB_LCD("/dev/ttyACM0", str_1, 1);
    			sprintf(str_2, "JDA/Total %d/%d", inv_info->num, sys_inv_info_all2.num);
    			string_USB_LCD("/dev/ttyACM0", str_2, 2);
    		}else{
    			sprintf(str_1, "Get JDA #%02d/10", inv_addr);
    			string_USB_LCD("/dev/ttyACM0", str_1, 1);
    			sprintf(str_2, "JDA/Total %d/%d", inv_info->num, sys_inv_info_all2.num);
    			string_USB_LCD("/dev/ttyACM0", str_2, 2);
    		}
#endif
			i=0;

			i = sizeof(raw_485_pkg.uniFrame.im_modbus_format_read);

			//select Tx
			//write(fd_gpio88, "1", 1);
			write(fd_rs485, &(raw_485_pkg.uniFrame.im_modbus_format_read), i);
	  	
	  		usleep(WAITING_TIME_AFTER_ISSUE_CMD);
	  	
			//process Rx
	  		bufptr = rcv_buff;
			nbytes=0;
			bzero(bufptr, 128);
			rs232_timeout=FALSE;
	  		rsp_size = req_size*2 + 5;
          
	  		//printf("rsp_size should be %d\n", rsp_size);
	  		for(;;){ // do rs485 receiving during 3 second, or timeout
	  	  	// check if there's data in com port, or time out
				
				if(net_timeout_cnt==RETRY_TIMES_FOR_ONE_SCAN/*isready(fd_rs485)==0*/){
	  				//rs232_timeout=TRUE;
	  				puts("Crash");
	  				rs232_timeout=INV_MSG_CRASH;
				
	  				break;
	  			}
	  		
				// read it !!
                bytes = read(fd_rs485, (char*)bufptr, rsp_size);        // it will always receive

                nbytes +=bytes;
                bufptr += bytes;
                // check satisfy
        		if(bytes <= 0){         // get no data at all before timeout
                        rs232_timeout=INV_MSG_CRASH;
                        //puts("Scan Crash");
                        break;
                }else if(rsp_size == nbytes){ // get exact data before timeout
                        rs232_timeout=FALSE;
                        break;
                }else{          // get partial data, keep watching !!
                        rs232_timeout=FALSE;
                        //break;
                }
        		usleep(WAITING_TIME_JDA_SCAN); //0.1 s
                net_timeout_cnt ++;

	  		}

	  		// pre-fetch the value
 			if(rsp_size == nbytes){
 				//printf("rs485RTN:[0]=%x,[1]=%x,[2]=%x,[3]=%x,[4]=%x\n", rcv_buff[0], rcv_buff[1], rcv_buff[2], rcv_buff[3], rcv_buff[4]);
 				bufptr = rcv_buff + 3;	// locate pointer to MODBUS 1st Word
               	disp_value = bufptr[0];
               	disp_value = disp_value << 8;
               	disp_value = disp_value + bufptr[1];
               	digit = bufptr[3];

               	printf("digit=%d\n", digit);
 				
 				if (digit == 0)
               	    digit = 1;
               	else if (digit == 1)
               	    digit = 10;
               	else if (digit == 2)
               		digit = 100;
               	else if (digit == 3)
               	    digit = 1000;

 				test_reassemble_float = disp_value;
               	test_reassemble_float = test_reassemble_float / digit;
               	
               	if((test_reassemble_float < 0)||(test_reassemble_float > 2000))
               		test_reassemble_float = 0;
              	
              	if (digit == 1)
              		rtn_float = sprintf(curr_value, "%.0f", test_reassemble_float);
              	else if (digit == 10)
              		rtn_float = sprintf(curr_value, "%.1f", test_reassemble_float);
              	else if (digit == 100)
              		rtn_float = sprintf(curr_value, "%.2f", test_reassemble_float);
              	else if (digit == 1000)
              		rtn_float = sprintf(curr_value, "%.3f", test_reassemble_float);              	

              	if(rtn_float > (int)sizeof(curr_value)){
              		puts("Error while get the Pyranometer value !!");
              		rs232_timeout=INV_MSG_CRASH;
              	}
              		printf("float value might be %s\n", curr_value);
            }

			valid_cnt=0;
			invIsPyranometer = FALSE;
			invIsThermometer = FALSE;
			invIsThermograph = FALSE;


			if(rs232_timeout==FALSE){
				if(nbytes){
					tmp_bufp = rcv_buff;
					
					if(check_CRC16((modbus_format_rtn_t *)tmp_bufp)>0){
						// copy the data
						tmp_bufp += 2; // locate ptr to pointing "Byte Count"
						
						if( cpy_size == *((uint8_t*)tmp_bufp) ){
							actual_id = inv_addr + offset;
							if (inv_addr <= PYRANOMETER_MAX){
								invIsPyranometer = TRUE;   // currently, we assume this is a valid pyranometer upon correct response size
								if(offset != 0xffff){
									inverter_msg2host_jda_array[actual_id].inverterId = actual_id;
									strcpy(inverter_msg2host_jda_array[actual_id].value, curr_value);
								}else
									puts("In pyranometer scanning, we only get list, but don't update slot data !!");
							}else if (inv_addr <= THERMOGRAPH_MAX){
								invIsThermograph = TRUE;   // currently, we assume this is a valid pyranometer upon correct response size
								if(offset != 0xffff){
									inverter_msg2host_jda_array[actual_id].inverterId = actual_id;
									strcpy(inverter_msg2host_jda_array[actual_id].value, curr_value);
								}else
									puts("In pyranometer scanning, we only get list, but don't update slot data !!");
							}else {
								invIsThermometer = TRUE;   // currently, we assume this is a valid pyranometer upon correct response size
								if(offset != 0xffff){
									inverter_msg2host_jda_array[actual_id].inverterId = actual_id;
									strcpy(inverter_msg2host_jda_array[actual_id].value, curr_value);
								}else
									puts("In pyranometer scanning, we only get list, but don't update slot data !!");
							}
	  		  			}else
	  		  				printf("copy size mismatch, copy abort, data size in rtn is %d, cpy_size == %d\n", tmp_bufp[0], cpy_size);
	  		  		
							// modify count
							nbytes = valid_cnt;
							valid_cnt=0;					
					}else{
						printf("receive RS485 packet with CRC error !!\n");	
						
					}
				}
			}
	  	
			//change state
			if(rs232_timeout==INV_MSG_CRASH){
				inv_pyranometer_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NORMAL);
				increment++; 
			}
    		else if(rs232_timeout==FALSE && invIsPyranometer){
    			inv_pyranometer_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NORMAL);	
    			printf("Pyranometer address=%d found on %s\n", inv_addr, scan_port_name);
    			inv_info->num = inv_info->num + 1;
    			inv_info->invInfo[inv_info->num -1].invId = inv_addr;
    			inv_info->invInfo[inv_info->num -1].invType = USE_JDA;
    			increment = 2;
			}
    		else if(rs232_timeout==FALSE && invIsThermograph){
    			inv_pyranometer_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NORMAL);	
    			printf("Thermograph address=%d found on %s\n", inv_addr, scan_port_name);
    			inv_info->num = inv_info->num + 1;
    			inv_info->invInfo[inv_info->num -1].invId = inv_addr; 
    			inv_info->invInfo[inv_info->num -1].invType = USE_JDA_TG;
    			increment = 2;
    		} 			
    		else if(rs232_timeout==FALSE && invIsThermometer){
    			inv_pyranometer_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NORMAL);	
    			printf("Thermometer address=%d found on %s\n", inv_addr, scan_port_name);
    			inv_info->num = inv_info->num + 1;
    			inv_info->invInfo[inv_info->num -1].invId = inv_addr;
    			inv_info->invInfo[inv_info->num -1].invType = USE_JDA_TM;
    			increment = 2; 			
			}else{
				printf("RS485 timeout !!\n");
				inv_pyranometer_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_TIMEOUT);
				increment++;
			}
		}
		else
			inv_pyranometer_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NORMAL);

		//usleep(WAITING_TIME_JDA_SCAN);
	}

	close(fd_rs485);
	return 1;
}


/****************************
** MOTECH Inverter protocol
****************************/

inverter_info_t inverter_info;

// defined as matrix 1
inv_motech_regsmatrix_t motech_reg_def[32] = {
	{INV_CMD_STATE_INIT, 	0x0, 	0, 	0},
	{INV_CMD_STATE_RDY, 	0x0, 	0, 	0},
	{INV_CMD_STATE_TIMEOUT,	0x0,	0,	0},
	{INV_CMD_STATE_STOP, 	0x0,	0,	0},
	{INV_CMD_STATE_W85, 	0x85, 	(uint32_t)&(inverter_info.W_0x85) - (uint32_t)&inverter_info, 	sizeof(inverter_info.W_0x85)},
	{INV_CMD_STATE_BN, 		0x0306, (uint32_t)&(inverter_info.brand_name) - (uint32_t)&inverter_info, sizeof(inverter_info.brand_name)},
	{INV_CMD_STATE_TN,		0x030e, (uint32_t)&(inverter_info.type_name) - (uint32_t)&inverter_info, sizeof(inverter_info.type_name)},
	{INV_CMD_STATE_SN, 		0x0316, (uint32_t)&(inverter_info.sn_name) - (uint32_t)&inverter_info, 	sizeof(inverter_info.sn_name)},
	{INV_CMD_STATE_W17, 	0x17,  	(uint32_t)&(inverter_info.W_0x17) - (uint32_t)&inverter_info, 	sizeof(inverter_info.W_0x17)},
	{INV_CMD_STATE_W12, 	0x12, 	(uint32_t)&(inverter_info.W_0x12) - (uint32_t)&inverter_info, 	sizeof(inverter_info.W_0x12)},
	{INV_CMD_STATE_W56, 	0x56, 	(uint32_t)&(inverter_info.W_0x56) - (uint32_t)&inverter_info,	sizeof(inverter_info.W_0x56)},
	{INV_CMD_STATE_W86, 	0x86,	(uint32_t)&(inverter_info.W_0x86) - (uint32_t)&inverter_info, 	sizeof(inverter_info.W_0x86)},
	{INV_CMD_STATE_W83, 	0x83, 	(uint32_t)&(inverter_info.W_0x83) - (uint32_t)&inverter_info, 	sizeof(inverter_info.W_0x83)},
	{INV_CMD_STATE_WB5,		0xb5, 	(uint32_t)&(inverter_info.inverter_meta_data.State) - (uint32_t)&inverter_info, 15*2},
	{INV_CMD_STATE_WC4, 	0xc4, 	(uint32_t)&(inverter_info.inverter_meta_data.TOTAL_POWER_H) - (uint32_t)&inverter_info, 15*2},
	{INV_CMD_STATE_WD3, 	0xd3, 	(uint32_t)&(inverter_info.inverter_meta_data.TIME_SEC_CNT) - (uint32_t)&inverter_info, 15*2},
	{INV_CMD_STATE_WE2, 	0xe2, 	(uint32_t)&(inverter_info.inverter_meta_data.IpvB) - (uint32_t)&inverter_info, 15*2},
	{INV_CMD_STATE_WF1, 	0xf1, 	(uint32_t)&(inverter_info.inverter_meta_data.DeltaCPU_Ires) - (uint32_t)&inverter_info, 15*2},
	{INV_CMD_STATE_W100, 	0x100, 	(uint32_t)&(inverter_info.inverter_meta_data.FacTimeTRIP) - (uint32_t)&inverter_info, 14*2},
	{INV_CMD_STATE_WC1, 	0xc1, 	(uint32_t)&(inverter_info.W_0xc1) - (uint32_t)&inverter_info, 	sizeof(inverter_info.W_0xc1)},
	{INV_CMD_STATE_WF7, 	0xf7, 	(uint32_t)&(inverter_info.W_0xf7) - (uint32_t)&inverter_info, 	sizeof(inverter_info.W_0xf7)},
	{INV_CMD_STATE_W01, 	0x01, 	(uint32_t)&(inverter_info.W_0x01) - (uint32_t)&inverter_info, 	sizeof(inverter_info.W_0x01)},
	{INV_CMD_STATE_W0C, 	0x0c, 	(uint32_t)&(inverter_info.W_0x0c) - (uint32_t)&inverter_info, 	sizeof(inverter_info.W_0x0c)},
	{INV_CMD_STATE_W15, 	0x15, 	(uint32_t)&(inverter_info.W_0x15) - (uint32_t)&inverter_info, 	sizeof(inverter_info.W_0x15)},
	{INV_CMD_STATE_END, 	0x0, 	0, 0}
};



#if 0
// modify this api to fit scan NO change condition, 
// should add INV_CMD_STATE_SCAN_REPEAT case 
int inv_state_chg_next(inv_cmd_state_t* curr_inv_cmd_state, inv_cmd_event_t in_event){
	
	switch (*curr_inv_cmd_state) {
		case INV_CMD_STATE_INIT:
			if(!in_event)	// event normal
				*curr_inv_cmd_state = INV_CMD_STATE_RDY;
			else
				*curr_inv_cmd_state = INV_CMD_STATE_STOP;
		break;	
		case INV_CMD_STATE_RDY:
			if(in_event == INV_CMD_EVENT_SCAN)
				*curr_inv_cmd_state = INV_CMD_STATE_SCAN_RUN;
			else if(in_event == INV_CMD_EVENT_NORMAL)
				*curr_inv_cmd_state = INV_CMD_STATE_W85;
			else if (in_event&(INV_CMD_EVENT_ERR|INV_CMD_EVENT_STOP))
				*curr_inv_cmd_state = INV_CMD_STATE_STOP;
		break;
		case INV_CMD_STATE_ERR:
			if(!in_event)
				*curr_inv_cmd_state = INV_CMD_STATE_RDY;
			else
				*curr_inv_cmd_state = INV_CMD_STATE_STOP;
		break;
		case INV_CMD_STATE_STOP:
			if(!in_event)
				*curr_inv_cmd_state = INV_CMD_STATE_RDY;
			else
				*curr_inv_cmd_state = INV_CMD_STATE_STOP;
		break;
		case INV_CMD_STATE_TIMEOUT:
			if(!in_event)
				*curr_inv_cmd_state = INV_CMD_STATE_ERR;
			else
				*curr_inv_cmd_state = INV_CMD_STATE_STOP;
		break;
		
		case INV_CMD_STATE_W85:
		case INV_CMD_STATE_BN:
		case INV_CMD_STATE_TN:
		case INV_CMD_STATE_SN:
		case INV_CMD_STATE_W17:
		case INV_CMD_STATE_W12:
		case INV_CMD_STATE_W56:
		case INV_CMD_STATE_W86:
		case INV_CMD_STATE_W83:
		case INV_CMD_STATE_WB5:
		case INV_CMD_STATE_WC4:
		case INV_CMD_STATE_WD3:
		case INV_CMD_STATE_WE2:
		case INV_CMD_STATE_WF1:
		case INV_CMD_STATE_W100:
		case INV_CMD_STATE_WC1:
		case INV_CMD_STATE_WF7:
		case INV_CMD_STATE_W01:
		case INV_CMD_STATE_W0C:
		case INV_CMD_STATE_W15:
			if (in_event == INV_CMD_EVENT_NOCHANGE)
				*curr_inv_cmd_state = *curr_inv_cmd_state;
			else if(in_event == INV_CMD_EVENT_NORMAL)	// event normal
				*curr_inv_cmd_state = *curr_inv_cmd_state + 1;
			else if (in_event&INV_CMD_EVENT_TIMEOUT)
				*curr_inv_cmd_state = INV_CMD_STATE_TIMEOUT;
			else if (in_event&INV_CMD_EVENT_ERR)
				*curr_inv_cmd_state = INV_CMD_STATE_ERR;
			else if (in_event&INV_CMD_EVENT_STOP)
				*curr_inv_cmd_state = INV_CMD_STATE_STOP;
		break;
		
		case INV_CMD_STATE_SCAN_RUN:
			if (in_event == INV_CMD_EVENT_NOCHANGE)
				*curr_inv_cmd_state = INV_CMD_STATE_SCAN_REPEAT;
			else if(in_event == INV_CMD_EVENT_STOP)
				*curr_inv_cmd_state = INV_CMD_STATE_STOP;
			else if (in_event == INV_CMD_EVENT_END)
				*curr_inv_cmd_state = INV_CMD_STATE_SCAN_END;
			else 
				*curr_inv_cmd_state = *curr_inv_cmd_state;
		break;

		case INV_CMD_STATE_SCAN_REPEAT:
			if (in_event == INV_CMD_EVENT_NORMAL)
				*curr_inv_cmd_state = INV_CMD_STATE_SCAN_RUN;		
			else if (in_event == INV_CMD_EVENT_NOCHANGE)
				*curr_inv_cmd_state = INV_CMD_STATE_SCAN_REPEAT;
			else if(in_event == INV_CMD_EVENT_STOP)
				*curr_inv_cmd_state = INV_CMD_STATE_STOP;
			else if (in_event == INV_CMD_EVENT_END)
				*curr_inv_cmd_state = INV_CMD_STATE_SCAN_END;
			else 
				*curr_inv_cmd_state = *curr_inv_cmd_state;
		break;	
		
		case INV_CMD_STATE_END:
			if(in_event == INV_CMD_EVENT_NORMAL)	// event normal
				*curr_inv_cmd_state = INV_CMD_STATE_RDY;
			else if(in_event == INV_CMD_EVENT_END)
				*curr_inv_cmd_state = INV_CMD_STATE_INIT;
			else
				*curr_inv_cmd_state = INV_CMD_STATE_STOP;
		break;
		
		case INV_CMD_STATE_SCAN_END:
			if(in_event == INV_CMD_EVENT_STOP)
				*curr_inv_cmd_state = INV_CMD_STATE_STOP;
		
		default:;
	}
	
	return 1;
}
#endif

int inv_state_chg_next(inv_cmd_state_t* curr_inv_cmd_state, inv_cmd_event_t in_event){
	
	switch (*curr_inv_cmd_state) {
		case INV_CMD_STATE_INIT:
			if(!in_event)	// event normal
				*curr_inv_cmd_state = INV_CMD_STATE_RDY;
			else
				*curr_inv_cmd_state = INV_CMD_STATE_STOP;
		break;	
		case INV_CMD_STATE_RDY:
			if(in_event == INV_CMD_EVENT_SCAN)
				*curr_inv_cmd_state = INV_CMD_STATE_SCAN_RUN;
			else if(in_event == INV_CMD_EVENT_NORMAL)
				*curr_inv_cmd_state = INV_CMD_STATE_W85;
			else if (in_event&(INV_CMD_EVENT_ERR|INV_CMD_EVENT_STOP))
				*curr_inv_cmd_state = INV_CMD_STATE_STOP;
		break;
		case INV_CMD_STATE_ERR:
			if(!in_event)
				*curr_inv_cmd_state = INV_CMD_STATE_RDY;
			else
				*curr_inv_cmd_state = INV_CMD_STATE_STOP;
		break;
		case INV_CMD_STATE_STOP:
			if(!in_event)
				*curr_inv_cmd_state = INV_CMD_STATE_RDY;
			else
				*curr_inv_cmd_state = INV_CMD_STATE_STOP;
		break;
		case INV_CMD_STATE_TIMEOUT:
			if(!in_event)
				*curr_inv_cmd_state = INV_CMD_STATE_ERR;
			else
				*curr_inv_cmd_state = INV_CMD_STATE_STOP;
		break;
		
		case INV_CMD_STATE_W85:
		case INV_CMD_STATE_BN:
		case INV_CMD_STATE_TN:
		case INV_CMD_STATE_SN:
		case INV_CMD_STATE_W17:
		case INV_CMD_STATE_W12:
		case INV_CMD_STATE_W56:
		case INV_CMD_STATE_W86:
		case INV_CMD_STATE_W83:
		case INV_CMD_STATE_WB5:
		case INV_CMD_STATE_WC4:
		case INV_CMD_STATE_WD3:
		case INV_CMD_STATE_WE2:
		case INV_CMD_STATE_WF1:
		case INV_CMD_STATE_W100:
		case INV_CMD_STATE_WC1:
		case INV_CMD_STATE_WF7:
		case INV_CMD_STATE_W01:
		case INV_CMD_STATE_W0C:
		case INV_CMD_STATE_W15:
			if (in_event == INV_CMD_EVENT_NOCHANGE)
				*curr_inv_cmd_state = *curr_inv_cmd_state;
			else if(in_event == INV_CMD_EVENT_NORMAL)	// event normal
				*curr_inv_cmd_state = *curr_inv_cmd_state + 1;
			else if (in_event&INV_CMD_EVENT_TIMEOUT)
				*curr_inv_cmd_state = INV_CMD_STATE_TIMEOUT;
			else if (in_event&INV_CMD_EVENT_ERR)
				*curr_inv_cmd_state = INV_CMD_STATE_ERR;
			else if (in_event&INV_CMD_EVENT_STOP)
				*curr_inv_cmd_state = INV_CMD_STATE_STOP;
		break;
		
		case INV_CMD_STATE_SCAN_RUN:
			if(in_event == INV_CMD_EVENT_STOP)
				*curr_inv_cmd_state = INV_CMD_STATE_STOP;
			else if (in_event == INV_CMD_EVENT_END)
				*curr_inv_cmd_state = INV_CMD_STATE_SCAN_END;
			else 
				*curr_inv_cmd_state = *curr_inv_cmd_state;
		break;
		
		case INV_CMD_STATE_END:
			if(in_event == INV_CMD_EVENT_NORMAL)	// event normal
				*curr_inv_cmd_state = INV_CMD_STATE_RDY;
			else if(in_event == INV_CMD_EVENT_END)
				*curr_inv_cmd_state = INV_CMD_STATE_INIT;
			else
				*curr_inv_cmd_state = INV_CMD_STATE_STOP;
		break;
		
		case INV_CMD_STATE_SCAN_END:
			if(in_event == INV_CMD_EVENT_STOP)
				*curr_inv_cmd_state = INV_CMD_STATE_STOP;
		
		default:;
	}
	
	return 1;
}

int getRegsMatrix_Motech(inverter_info_t* ptr_base, inv_cmd_state_t state, uint16_t* out_addr, uint16_t** out_ptr, uint8_t* out_size){
	uint32_t i;
	bool_t isFound=FALSE;

	for (i=0; i<(sizeof(motech_reg_def)/sizeof(inv_motech_regsmatrix_t)); i++){
		if(motech_reg_def[i].state_num == state){
			isFound = TRUE;
			*out_addr = motech_reg_def[i].reg_addr;
			*out_ptr = (uint16_t*)((uint32_t)ptr_base + (uint32_t)motech_reg_def[i].offset);//motech_reg_def[i].ptr_addr;
			*out_size = motech_reg_def[i].cpy_size;
			break;
		}
	}

	if (isFound)
		return 1;
	else{
		printf("State #%d no found in matrix\n", state);
		*out_addr = 0x0;
		*out_ptr = NULL;
		*out_size = 0;
		return -1;
	}
}


int inverter_scan_motech(scan_inv_info_t* inv_info, char* scan_port_name){

  int fd_rs485;
  //int fd_gpio88;

  int bytes;

  uint8_t rcv_buff[128];
  uint8_t *bufptr;

  int nbytes;

  int i;
	raw_485_t raw_485_pkg;
	int req_size, rsp_size;


  uint8_t rs232_timeout = FALSE;
  uint8_t rcv_len;
  
  int tmp_i, valid_cnt=0;
  bool_t tail_valid=FALSE;
  uint8_t * tmp_bufp;
	
	uint16_t inv_addr /*= 0x03*/, inv_reg2rd;
	
	inv_cmd_state_t inv_cmd_state = INV_CMD_STATE_INIT;
	uint8_t cpy_size;
	//uint16_t * cpy_ptr=NULL;
	const char invBnMotech[7] = "MOTECH";
	bool_t invIsMOTECH;	
	
	//
	// Initial UART
	if(!scan_port_name)
		return -1;
  	
  	fd_rs485 = open_port_RS485(scan_port_name, O_RDWR| O_NOCTTY);
  	
  	if(fd_rs485 < 0){
  		printf("Open port %s failed !!", scan_port_name);
  		return -1;
  	}

  	printf("We get RS485 port number:%d\n", fd_rs485);
   
	//printf("Current BaudRate=%d\n",getbaud(fd_rs485) );
	
	while(1){

		switch (inv_cmd_state){

		case INV_CMD_STATE_INIT:
			inv_addr = INV_START_ADDR - 1;
			inv_reg2rd = 0;
			inv_info->num = 0;
		break;

		case INV_CMD_STATE_RDY:
			cpy_size = 0;
			inv_reg2rd = 0;
		break;
		
		case INV_CMD_STATE_TIMEOUT:
			cpy_size = 0;
			inv_reg2rd = 0;
		break;
		
		case INV_CMD_STATE_STOP:
			cpy_size = 0;
			inv_reg2rd = 0;
		break;
		
		case INV_CMD_STATE_SCAN_RUN:
			inv_reg2rd = 0x67;
			inv_addr = inv_addr + 1;
			cpy_size = 16;
			invIsMOTECH = FALSE;			
		break;

		case INV_CMD_STATE_SCAN_END:
		//initial
			//cpy_ptr = NULL;
			cpy_size = 0;
			//sem_wait(&sem_rs232);	
		break;
		
		default:
		// display
			printf("default state, inv_cmd_state=%d\n", inv_cmd_state);
		// next
			inv_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_STOP);
		}	// switch case
	//
	// Does task, then update state
	// 
		
		if (inv_cmd_state == INV_CMD_STATE_INIT)	{
			//printf("RS232 Initial again !!\n");
			//sleep(1);
			inv_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NORMAL);
		}
		else if(inv_cmd_state == INV_CMD_STATE_RDY)
			inv_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_SCAN);
		else if (inv_cmd_state == INV_CMD_STATE_TIMEOUT){
				inv_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_STOP);
		}
		else if(inv_cmd_state == INV_CMD_STATE_SCAN_END)
		{
			puts("Finish Scan process!!\n");
			
			// dump message
			
			inv_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_STOP);
			printf("To the end of Inverter probe !!\n");
			
			// break the while
			break;
		}
		else if(inv_cmd_state == INV_CMD_STATE_STOP)
		{
			printf("reach Motech SCAN stop state !!\n");
			close(fd_rs485);
			return 0;
			//inv_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NORMAL);
		}
		else if(inv_cmd_state&0xf00)
		{
			// check if it reach the end
			if(inv_addr > INV_MAX)
				inv_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_END);
			
			bzero(&raw_485_pkg, sizeof(raw_485_pkg));
			raw_485_pkg.pkt_type = mt81xx_cmd_read;	
			req_size = 8;
			if(create_mt81xx_cmd(&raw_485_pkg, inv_addr, inv_reg2rd , req_size)<0)
				printf("Create Read Cmd to Rs485 failed!!\n");
			if(append_motech_CRC16(&raw_485_pkg)<0)
				printf("Adding CRC16 error !!\n");
    	
    		printf("Scan Motech inv# = %d on %s\n", inv_addr, scan_port_name);
#ifdef USE_LCD
    		char str_1[16], str_2[16];
    		sprintf(str_1, "ScanMT #%d/%d", inv_addr, INV_MAX);
    		string_USB_LCD("/dev/ttyACM0", str_1, 1);
    		sprintf(str_2, "Found MT #%d", inv_info->num);
    		string_USB_LCD("/dev/ttyACM0", str_2, 2);
#endif
    	
			i=0;

			while(((uint8_t*)&(raw_485_pkg.uniFrame.im_mt81xx_format03))[i] != 0x0d){
				//printf("%.2x ",((uint8_t*)&(raw_485_pkg.uniFrame.im_mt81xx_format03))[i]);
				i++;
			}

			//select Tx
			//write(fd_gpio88, "1", 1);
			write(fd_rs485, &(raw_485_pkg.uniFrame.im_mt81xx_format03), i+1);
	  	
	  		usleep(WAITING_TIME_AFTER_ISSUE_CMD);
	  	
			//process Rx
	  		bufptr = rcv_buff;
			nbytes=0;
			bzero(bufptr, 128);
			rs232_timeout=FALSE;
	  		rsp_size = req_size*2 + 7;
	  	
	  		int net_timeout_cnt = 0;

	  		for(;;){ // do rs485 receiving during 3 second, or timeout
	  	  	// check if there's data in com port, or time out
				
				if(net_timeout_cnt==RETRY_TIMES_FOR_ONE_SCAN/*isready(fd_rs485)==0*/){
	  				//rs232_timeout=TRUE;
	  				puts("Crash");
	  				rs232_timeout=INV_MSG_CRASH;
				
	  				break;
	  			}
	  		
				// read it !!
	  	 		bytes = read(fd_rs485, (char*)bufptr, rsp_size);	// it will always receive

				nbytes +=bytes;
				bufptr += bytes;
				// check satisfy

				if(bytes <= 0){		// get no data at all before timeout
	  				rs232_timeout=INV_MSG_CRASH;
	  				//puts("Scan Crash");
	  				break;
	  			}else if(rsp_size == nbytes){ // get exact data before timeout
					rs232_timeout=FALSE;
					break;
				}else{		// get partial data, keep watching !!
					rs232_timeout=FALSE;
					//break;
				}
				
				usleep(WAITING_TIME_EACH_SCAN_INV); //0.4 s
				net_timeout_cnt ++;
	  		}

			valid_cnt=0;
			rcv_len = 0;
			
			if(rs232_timeout==FALSE){
				if(nbytes){
					// find 0x0a, check tail 0x0d
					
					tmp_bufp = rcv_buff;
					tail_valid=TRUE;
					for (tmp_i=0; tmp_i<nbytes; tmp_i++){

						if(rcv_buff[tmp_i]==0x0a){
							rcv_len = rcv_buff[tmp_i+3]; // 0x0a prefix is followed by addr, then length <--we want
							tmp_bufp = rcv_buff + tmp_i; // tmp_bufp pointing to the SOF
							//printf("tmp_bufp = %p\n", tmp_bufp);
							
							if(rcv_buff[tmp_i+rcv_len+6]==0x0d){
								valid_cnt = rcv_len+6;
								tail_valid=TRUE;
								break;
							}else{
								valid_cnt = 0;
								puts("pkt tail not valid !!");
								tail_valid=FALSE;
								break;
							}
						}
					}
					
					if(check_motech_CRC16((mt81xx_format_rtn_t*)tmp_bufp)>0){
						// copy the data
						tmp_bufp += 3; // locate ptr to pointing "Byte Count"

						if(tail_valid && ( cpy_size == *((uint8_t*)tmp_bufp) )){
							tmp_bufp += 1;	// locate to data
							
							//printf("We get #%d data:%s\n", cpy_size, tmp_bufp);

							tmp_bufp[sizeof(invBnMotech)-1] = '\0';
							
							if(strcmp(invBnMotech, (char*)tmp_bufp)==0)
								invIsMOTECH = TRUE;
							else
								invIsMOTECH = FALSE;

							// switch bytes
							// if it is ASCII field, don't switch
							//switch_bytes((uint8_t*)tmp_bufp, cpy_size>>1);
							
							//memcpy(cpy_ptr, tmp_bufp, cpy_size);
							//printf("Get data 0x%.4x\n", cpy_ptr[0]);
	  		  			}else
	  		  				printf("copy size mismatch, copy abort, data size in rtn is %d, cpy_size == %d\n", tmp_bufp[0], cpy_size);
	  		  		
						// modify count
						nbytes = valid_cnt;
						valid_cnt=0;					
					
					}else{
						printf("receive RS485 packet with CRC error !!\n");	
						
					}
				}
			}	

			//change state
			if(rs232_timeout==INV_MSG_CRASH){
				inv_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NORMAL);
			}
    		else if(rs232_timeout==FALSE && invIsMOTECH){
    			inv_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NORMAL);	
    			printf("MOTECH inverter address=%d found on %s\n", inv_addr, scan_port_name);
    			inv_info->num = inv_info->num + 1;
    			inv_info->invInfo[inv_info->num -1].invId = inv_addr;
			}else{
				printf("RS485 timeout !!\n");
				inv_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_TIMEOUT);
			}
		}
		else
			inv_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NORMAL);
	}

	//puts("exit motech_scan !");
	close(fd_rs485);
	
	return 1;
}

int invInfoGet_Motech(uint16_t invAddr, inverter_msg2host_new_t *data, char* scan_port_name){

  int fd_rs485;
  
  int bytes;

  uint8_t rcv_buff[128];
  uint8_t *bufptr;

  raw_485_t raw_485_pkg;
  uint32_t req_size, nbytes;

  uint8_t rs232_timeout = FALSE;
  
  bool_t tail_valid=FALSE, this_pkt_valid=TRUE;
  uint8_t * tmp_bufp, * data_basep;
  uint16_t inv_addr /*= 0x03*/, inv_reg2rd;
	
  inv_cmd_state_t inv_cmd_state = INV_CMD_STATE_INIT;
  uint8_t cpy_size;
  uint16_t * cpy_ptr=NULL;
	
  int net_timeout_cnt, rs485_timeout_cnt, size_to_read;
  
  inverter_info_t inverter_info_local;

  bzero(&inverter_info_local, sizeof(inverter_info_t));

  //printf("In handle rs232 thread !!\n");

  //ptr = &cpy_ptr_value;

	//
	// Initial UART
  if(!scan_port_name)
  	return -1;  
  fd_rs485 = open_port_RS485(scan_port_name, O_RDWR| O_NOCTTY);
  
  if(fd_rs485 < 0){
  	printf("Open port %s failed !!", scan_port_name);
  	return -1;
  }
   
	//printf("Current BaudRate=%d\n",getbaud(fd_rs485) );
  /***************************
  *  Get State
  ****************************/
	while(1){
		// get inv_reg2rd, cpy_ptr, cpy_size by current state
		if(getRegsMatrix_Motech(&inverter_info_local, inv_cmd_state, &inv_reg2rd, &cpy_ptr, &cpy_size)<=0){
			printf("state no found in motech protocol, inv_cmd_state=%d, goes to STOP state \n", inv_cmd_state);
			inv_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_STOP);
		}

		// print current state
		printf("inv_cmd_state=%d\n", inv_cmd_state);
		

  /***************************
  *  Process Special States
  ****************************/		
		if (inv_cmd_state == INV_CMD_STATE_INIT)	{
			//sleep(1);  // 20130611
			rs485_timeout_cnt = 0;
			this_pkt_valid=TRUE;
#ifdef USE_LCD
			scan_inv_info_t tmp_info;
		  	char str_1[16];
  			char str_2[16];			
			sprintf(str_1, "GetMT ID=%d...", invAddr);			
    		string_USB_LCD("/dev/ttyACM0", str_1, 1);
			getSubSetInvInfo(&sys_inv_info_all2, USE_MOTECH, &tmp_info);
			sprintf(str_2, "MT/TOTAL %d/%d", tmp_info.num, sys_inv_info_all2.num);
    		string_USB_LCD("/dev/ttyACM0", str_2, 2);
#endif			
			inv_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NORMAL);
		}
		else if (inv_cmd_state == INV_CMD_STATE_RDY)
			inv_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NORMAL);
		else if (inv_cmd_state == INV_CMD_STATE_TIMEOUT)
			inv_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_STOP);

		// when state is END, loop reaches the end
		else if(inv_cmd_state == INV_CMD_STATE_END)
		{
#ifdef USE_LCD
			sprintf(str_1, "GetMT ID=%d OK", invAddr);
    		string_USB_LCD("/dev/ttyACM0", str_1, 1);
#endif    		
			puts("Dump meta data!!");
			inv_meta_data_dump(&inverter_info_local);
			
			// reduce amount of data from raw data to only what we care about
			if(this_pkt_valid)
				update_inv_msg2host_motech(data, &inverter_info_local,invAddr);
			else{
				puts("This capturing cantains invalid field, update this entire packet with ID set to 0");
				invAddr = 0;
				update_inv_msg2host_motech(data, &inverter_info_local,invAddr);
			}
			
			//sleep(1);
			if(!rs232_run_flag){
				inv_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_STOP);
				//printf("To the end of Inverter probe, waiting for re-start semaphore!!\n");
			}
			else{
				inv_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_END);
				printf("To the end of Inverter probe, automatically restart later !!\n");
			}
			
			// break the while loop
			break;
		}
		
		else if(inv_cmd_state == INV_CMD_STATE_STOP)
		{
			printf("reach Motech info_get stop state, update id = 0 to this slot !!\n");
			invAddr = 0;
			update_inv_msg2host_motech(data, &inverter_info_local, invAddr);	

			break;	
		}

  /***************************
  *  Normal States
  ****************************/		
		else if(inv_cmd_state&0xf0)
		{
			 
			//if(inv_cmd_state == INV_CMD_STATE_W85)

			bzero(&raw_485_pkg, sizeof(raw_485_pkg));
			raw_485_pkg.pkt_type = mt81xx_cmd_read;
			// req_size is counted in word size, one word contains two bytes
			req_size = cpy_size >> 1;
			inv_addr = (uint16_t)invAddr;
			
			// create data frame
			if(create_mt81xx_cmd(&raw_485_pkg, inv_addr, inv_reg2rd , req_size)<0)
				printf("Create Read Cmd to Rs485 failed!!\n");
			if(append_motech_CRC16(&raw_485_pkg)<0)
				printf("Adding CRC16 error !!\n");

  /***************************
  *  Send Read
  ****************************/			
			//write(fd_rs485, &(raw_485_pkg.uniFrame.im_mt81xx_format03), i+1);
	  		write(fd_rs485, &(raw_485_pkg.uniFrame.im_mt81xx_format03), sizeof(mt81xx_format03_t));

			//switch tx/rx
			// predict the time needed for tx, then goes next
			//usleep(11000); // (#of data)*1000 + 3ms margin
			usleep(WAITING_TIME_AFTER_ISSUE_CMD);

  /***************************
  *  Waiting for Response
  ****************************/	  	
  			//process Rx
	  		bufptr = rcv_buff;
			nbytes=0;
			bzero(bufptr, 128);
			rs232_timeout=FALSE;
			// start + slave + fun + len + Data... + CRC_lo + CRC_Hi + stop
	  		size_to_read = req_size*2 + 7;
	  	
	  		net_timeout_cnt = 0;

	  		for(;;){ // do rs485 receiving during 3 second, or timeout
	  	  	// check if there's data in com port, or time out
				
				if(net_timeout_cnt==RETRY_TIMES_FOR_ONE_CMD/*isready(fd_rs485)==0*/){
	  				//rs232_timeout=TRUE;
	  				puts("Crash");
	  				rs232_timeout=INV_MSG_CRASH;
				
	  				break;
	  			}
	  			
	  			usleep(WAITING_TIME_EACH_RETRY); //0.4 s
				
				// read it !!
	  	 		bytes = read(fd_rs485, (char*)bufptr, size_to_read);	// it will always receive

				nbytes +=bytes;
				bufptr += bytes;
				size_to_read -= bytes;
				// check satisfy

				if(size_to_read <= 0){
					rs232_timeout=FALSE;
					break;
				}

				net_timeout_cnt ++;
	  		}

  /***************************
  *  check received packet
  ****************************/
			if(rs232_timeout==FALSE){
				if(nbytes && (cpy_ptr!=NULL)){
					
					// check if the leading is 0x0a, check tail is 0x0d
					if((rcv_buff[0]==0x0a)&&(rcv_buff[nbytes-1]==0x0d))
						tail_valid = TRUE;
					else
						tail_valid = FALSE;

					tmp_bufp = rcv_buff;

					if(check_motech_CRC16((mt81xx_format_rtn_t *)tmp_bufp)>0){

						if(tail_valid && ( cpy_size == ((mt81xx_format_rtn_t*)tmp_bufp)->mt81xx_offset_Len )){
							
							data_basep = &(((mt81xx_format_rtn_t*)tmp_bufp)->mt81xx_offset_data_base);

							// switch bytes, if it is ASCII field, don't switch
							if ((inv_cmd_state != INV_CMD_STATE_BN)&&(inv_cmd_state != INV_CMD_STATE_TN)&&(inv_cmd_state != INV_CMD_STATE_SN))
								switch_bytes( data_basep, cpy_size>>1);

							// store the received data into internal place
							memcpy(cpy_ptr, data_basep, cpy_size);
							
	  		  			}else
	  		  				printf("copy size mismatch, copy abort, cpy_ptr == %d\n", sizeof(cpy_ptr));
					
					}else{
						printf("receive RS485 packet with CRC error !!\n");	
						rs232_timeout=INV_MSG_CRC_ERR;
					}
				}else
					puts("cpy_ptr is NULL or nbyte is zero");

			}	

  /***************************
  *  Change State
  ****************************/
			if(rs232_timeout==INV_MSG_CRASH){
				if(rs485_timeout_cnt < CMD_REPEAT_TIMES_WHEN_FAILED){
					inv_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NOCHANGE);
					puts("Try re-issue this cmd !!");
					rs485_timeout_cnt ++;
				}
				else{  // abort read this reg after rs485_timeout_cnt times of retries
					inv_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_STOP);	
					puts("Abort reading this item in RS485 !!");
					rs485_timeout_cnt = 0;
					this_pkt_valid = FALSE;
				}
			}else if(rs232_timeout==INV_MSG_CRC_ERR){
				// clear rx buffer
				bytes = 1;
				sleep(1);
				while(bytes > 0){
					bytes = read(fd_rs485, (char*)rcv_buff, 32);
					puts("clear RS232 Rx buffer !!");
				}				
				if(rs485_timeout_cnt < CMD_REPEAT_TIMES_WHEN_FAILED){
					inv_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NOCHANGE);
					rs485_timeout_cnt ++;
				}
				else{  // abort read this reg after rs485_timeout_cnt times of retries
					inv_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_STOP);	
					puts("Abort reading this item in RS485 !!");
					rs485_timeout_cnt = 0;
					this_pkt_valid = FALSE;
				}				
			}
    		else if(rs232_timeout==FALSE){
    			inv_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NORMAL);	
    			rs485_timeout_cnt = 0;
			}else{
					printf("RS485 timeout !!\n");
					inv_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_TIMEOUT);
					this_pkt_valid = FALSE;
			}
		}
		else
			inv_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NORMAL);
	}

	puts("exit getInfo_motech !");
	close(fd_rs485);
	
	return 1;
}

int convert_invmsg_to_asci_motech(char *buff, inverter_msg2host_new_t* msg2host){
	int n;
	double tmp_gen, tmp_gen_h, tmp_gen_l;
	char tmp_time[15]="";
	double tmp_etoday;
	uint16_t inv_status = INV_STATUS_UNKNOWN;

	//int atoi_int = 0;

	inv_status = msg2host->state_code;
	if( inv_status < 41 )
		inv_status  = INV_STATUS_WAIT;
	else if( inv_status == 50 )
		inv_status  = INV_STATUS_NORMAL;
	else if( (inv_status == 60) || (inv_status == 80))
		inv_status  = INV_STATUS_FAULT;
	else
		inv_status  = INV_STATUS_UNKNOWN;	

	syscall_getRsp("date +%Y%m%d%H%M%S", tmp_time);
	tmp_time[14] = '\0';

	tmp_gen_h = (uint32_t)msg2host->total_gen_h;
	tmp_gen_h = tmp_gen_h * 1000;
	tmp_gen_l = (uint32_t)msg2host->total_gen_l;
	tmp_gen_l = tmp_gen_l /10;

	tmp_gen = tmp_gen_h + tmp_gen_l;

	tmp_etoday = (double)msg2host->total_today;
	tmp_etoday = tmp_etoday /10;  // 0.1kWh unit

	n = sprintf(buff, "%04d,", msg2host->inv_id); 
	buff += n;	
	//n = sprintf(buff, "%s,", msg2host->brand_name); 
	n = sprintf(buff, "%s,", "MOTECH"); 
	buff += n;	
	n = sprintf(buff, "%s,", msg2host->type_name); 
	buff += n;
	n = sprintf(buff, "%s,", msg2host->sn_name); 
	buff += n;
	n = sprintf(buff, "%s,", tmp_time); 
	buff += n;
	n = sprintf(buff, "%d,", inv_status); 
	buff += n;
	n = sprintf(buff, "%d:", msg2host->total_hr); 
	buff += n;
	n = sprintf(buff, "%d:", msg2host->total_min); 
	buff += n;
	n = sprintf(buff, "%d,", msg2host->total_sec); 
	buff += n;
	n = sprintf(buff, "%.2f,", tmp_gen); 
	buff += n;	
	
	//atoi_int = atoi(inverter_msg2host_pyranometer_array[2].value);
	//atoi_int = atoi_int * 40;
	//n = sprintf(buff, "%d,", atoi_int); 
	
	n = sprintf(buff, "%d,", msg2host->pac); 
	buff += n;
	n = sprintf(buff, "%d,", msg2host->pdc); 
	buff += n;
	n = sprintf(buff, "%.1f", tmp_etoday); 
	buff += n;	
	buff[0] = '\0';
	
	return 1; 
	
}

int update_inv_msg2host_motech(inverter_msg2host_new_t *msg2host, inverter_info_t *inverter_info, uint16_t invId){
	
	msg2host->state_code = (uint8_t)inverter_info->inverter_meta_data.State;
	msg2host->inv_id = invId;
	memcpy(msg2host->brand_name, inverter_info->brand_name, 16);
	memcpy(msg2host->type_name, inverter_info->type_name, 16);
	memcpy(msg2host->sn_name, inverter_info->sn_name, 16);

	msg2host->total_hr = inverter_info->inverter_meta_data.TIME_HR_CNT;
	msg2host->total_min = inverter_info->inverter_meta_data.TIME_MIN_CNT;
	msg2host->total_sec = inverter_info->inverter_meta_data.TIME_SEC_CNT;
	msg2host->total_gen_h = inverter_info->inverter_meta_data.TOTAL_POWER_H;
	msg2host->total_gen_l = inverter_info->inverter_meta_data.TOTAL_POWER_L;
	msg2host->pac = inverter_info->inverter_meta_data.Pac;
	msg2host->pdc = inverter_info->inverter_meta_data.Pdc;
	msg2host->total_today = inverter_info->inverter_meta_data.Eac_L_TODAY;

	return 1;
}

int append_CRC16(modbus_format_read_t* FrameBuffer, int in_cal_size){
	int cal_size, i;
	unsigned short crc_result=0xffff;

	// check if required claculation length is greater than 16
	if(in_cal_size > 0xf)
		return -1;

	cal_size = in_cal_size;
	
	// start from slave address byte
	i=0;
	
	// calculate
	if(cal_size!=0xff){
		while(cal_size){
			// counting from SlaveAddr
			crc_result = update_crc_16(crc_result, (char) ((uint8_t*)&(FrameBuffer->modbus_offset_SlaveAddr))[i] );
			cal_size--;
			i++;
		}

		// fill the crc 
		FrameBuffer->modbus_offset_CS_LoByte = ((uint8_t*)&crc_result)[0];
		FrameBuffer->modbus_offset_CS_HiByte = ((uint8_t*)&crc_result)[1];
		return 1;
	}else
		return -1;
}

int append_motech_CRC16(raw_485_t* FrameBuffer){
	int cal_size=0xff;
//	unsigned short crc_result=0xffff;
	int implementRdy = FALSE;
//	raw_485_t* tmpP = FrameBuffer;

	// determine cal_size
	switch(((raw_485_t*)FrameBuffer)->pkt_type){
		case mt81xx_cmd_read:
			cal_size = 0x06;
			implementRdy = TRUE;
			break;
		case mt81xx_cmd_write:
		case mt81xx_cmd_6C:
		case mt81xx_cmd_AA:
		case mt81xx_cmd_0B:
			cal_size = 0x06;
			implementRdy = FALSE;
			break;
		case mt81xx_cmd_485:
			cal_size = 0x09;
			implementRdy = FALSE;
			break;
		default:
			cal_size = 0xff;
			implementRdy = FALSE;
	}

	if(implementRdy){
		append_CRC16((modbus_format_read_t*)&(FrameBuffer->uniFrame.im_mt81xx_format03.mt81xx_offset_SlaveAddr), cal_size);
		return 1;
	}else
		return -1;
}


int check_CRC16(modbus_format_rtn_t * FrameBuffer){
	int cal_size=0xff, i;
	unsigned short crc_result=0xffff;

	// get cal_size
	cal_size = FrameBuffer->modbus_offset_Len;
	
	// start from slave address byte
	i=0;
	// add 3 bytes, total length is $len plus (addr + fun + len) 
	cal_size +=3;	
	
	// calculate
	if(cal_size!=0xff){
		while(cal_size){
			crc_result = update_crc_16(crc_result, (char)((uint8_t*)(&(FrameBuffer->modbus_offset_SlaveAddr)))[i]);
			//printf("byte=0x%.2x\n", ((uint8_t *)(&((raw_485_t*)FrameBuffer)->uniFrame))[i] );
			cal_size--;
			i++;
		}

		if(((uint8_t*)(&(FrameBuffer->modbus_offset_SlaveAddr)))[i] == ((uint8_t*)&crc_result)[0]){
			i++;
			if(((uint8_t*)(&(FrameBuffer->modbus_offset_SlaveAddr)))[i] == ((uint8_t*)&crc_result)[1])
				return 1;
		}


		return -1;
	}else
		return -1;
}

int check_motech_CRC16(mt81xx_format_rtn_t * FrameBuffer){

	return check_CRC16((modbus_format_rtn_t*)&(FrameBuffer->mt81xx_offset_SlaveAddr));
}

int create_modbus_read(void* FrameBuffer, uint8_t dev_addr, uint16_t reg, uint8_t size){
	modbus_format_read_t* modbus_format_p = (modbus_format_read_t*) FrameBuffer;

	modbus_format_p->modbus_offset_SlaveAddr = dev_addr;
	modbus_format_p->modbus_offset_Function = 0x03;
	modbus_format_p->modbus_offset_SA_HiByte = ((uint8_t*)&reg)[1];
	modbus_format_p->modbus_offset_SA_LoByte = ((uint8_t*)&reg)[0];
	modbus_format_p->modbus_offset_NO_HiByte = 0x00;		// only 00 allowe
	modbus_format_p->modbus_offset_NO_LoByte = size;	// number of words
	modbus_format_p->modbus_offset_CS_LoByte = 0xff;
	modbus_format_p->modbus_offset_CS_HiByte = 0xff;

	return 1;
}

// assume Little Endian machine
int create_mt81xx_cmd(raw_485_t* FrameBuffer, uint16_t dev_addr_plusNew, uint16_t reg, uint32_t size){
	
	// decide pkt size
	switch(((raw_485_t*)FrameBuffer)->pkt_type){
		case mt81xx_cmd_read:
			FrameBuffer->uniFrame.im_mt81xx_format03.mt81xx_offset_StartByte = 0x0a;
			create_modbus_read(&(FrameBuffer->uniFrame.im_mt81xx_format03.mt81xx_offset_SlaveAddr), ((uint8_t*)&dev_addr_plusNew)[0], reg, ((uint8_t*)&size)[0]);
			FrameBuffer->uniFrame.im_mt81xx_format03.mt81xx_offset_StopByte = 0x0d;			   
			break;
		case mt81xx_cmd_write:
			FrameBuffer->uniFrame.im_mt81xx_format06.mt81xx_offset_StartByte = 0x0a;
			FrameBuffer->uniFrame.im_mt81xx_format06.mt81xx_offset_SlaveAddr = ((uint8_t*)&dev_addr_plusNew)[0];
			FrameBuffer->uniFrame.im_mt81xx_format06.mt81xx_offset_Function = 0x06;
			FrameBuffer->uniFrame.im_mt81xx_format06.mt81xx_offset_RA_HiByte = ((uint8_t*)&reg)[1];
			FrameBuffer->uniFrame.im_mt81xx_format06.mt81xx_offset_RA_LoByte = ((uint8_t*)&reg)[0];
			FrameBuffer->uniFrame.im_mt81xx_format06.mt81xx_offset_Data_HiByte = 0x00;		// only 00 allowe
			FrameBuffer->uniFrame.im_mt81xx_format06.mt81xx_offset_Data_LoByte = ((uint8_t*)&size)[0];	
			FrameBuffer->uniFrame.im_mt81xx_format06.mt81xx_offset_CS_LoByte = 0xff;
			FrameBuffer->uniFrame.im_mt81xx_format06.mt81xx_offset_CS_HiByte = 0xff;
			FrameBuffer->uniFrame.im_mt81xx_format06.mt81xx_offset_StopByte = 0x0d;			   
			break;	
		case mt81xx_cmd_6C:
			FrameBuffer->uniFrame.im_mt81xx_format6C.mt81xx_offset_StartByte = 0x0a;
			FrameBuffer->uniFrame.im_mt81xx_format6C.mt81xx_offset_SlaveAddr = ((uint8_t*)&dev_addr_plusNew)[0];
			FrameBuffer->uniFrame.im_mt81xx_format6C.mt81xx_offset_Function = 0x6c;
			FrameBuffer->uniFrame.im_mt81xx_format6C.mt81xx_offset_RA_HiByte = ((uint8_t*)&reg)[1];
			FrameBuffer->uniFrame.im_mt81xx_format6C.mt81xx_offset_RA_LoByte = ((uint8_t*)&reg)[0];
			FrameBuffer->uniFrame.im_mt81xx_format6C.mt81xx_offset_Data_HiByte = 0x00;		// only 00 allowe
			FrameBuffer->uniFrame.im_mt81xx_format6C.mt81xx_offset_Data_LoByte = ((uint8_t*)&size)[0];	
			FrameBuffer->uniFrame.im_mt81xx_format6C.mt81xx_offset_CS_LoByte = 0xff;
			FrameBuffer->uniFrame.im_mt81xx_format6C.mt81xx_offset_CS_HiByte = 0xff;
			FrameBuffer->uniFrame.im_mt81xx_format6C.mt81xx_offset_StopByte = 0x0d;			   
			break;
		case mt81xx_cmd_AA:
			FrameBuffer->uniFrame.im_mt81xx_formatAA.mt81xx_offset_StartByte = 0x0a;
			FrameBuffer->uniFrame.im_mt81xx_formatAA.mt81xx_offset_SlaveAddr = ((uint8_t*)&dev_addr_plusNew)[0];
			FrameBuffer->uniFrame.im_mt81xx_formatAA.mt81xx_offset_Function = 0xaa;
			FrameBuffer->uniFrame.im_mt81xx_formatAA.mt81xx_offset_RA_HiByte = ((uint8_t*)&reg)[1];
			FrameBuffer->uniFrame.im_mt81xx_formatAA.mt81xx_offset_RA_LoByte = ((uint8_t*)&reg)[0];
			FrameBuffer->uniFrame.im_mt81xx_formatAA.mt81xx_offset_Data_HiByte = 0x00;		// only 00 allowe
			FrameBuffer->uniFrame.im_mt81xx_formatAA.mt81xx_offset_Data_LoByte = ((uint8_t*)&size)[0];	
			FrameBuffer->uniFrame.im_mt81xx_formatAA.mt81xx_offset_CS_LoByte = 0xff;
			FrameBuffer->uniFrame.im_mt81xx_formatAA.mt81xx_offset_CS_HiByte = 0xff;
			FrameBuffer->uniFrame.im_mt81xx_formatAA.mt81xx_offset_StopByte = 0x0d;			   
			break;
		case mt81xx_cmd_0B:
			FrameBuffer->uniFrame.im_mt81xx_format0B.mt81xx_offset_StartByte = 0x0a;
			FrameBuffer->uniFrame.im_mt81xx_format0B.mt81xx_offset_SlaveAddr = ((uint8_t*)&dev_addr_plusNew)[0];
			FrameBuffer->uniFrame.im_mt81xx_format0B.mt81xx_offset_Function = 0x0b;
			FrameBuffer->uniFrame.im_mt81xx_format0B.mt81xx_offset_RA_HiByte = ((uint8_t*)&reg)[1];
			FrameBuffer->uniFrame.im_mt81xx_format0B.mt81xx_offset_RA_LoByte = ((uint8_t*)&reg)[0];
			FrameBuffer->uniFrame.im_mt81xx_format0B.mt81xx_offset_Data_HiByte = 0x00;		// only 00 allowe
			FrameBuffer->uniFrame.im_mt81xx_format0B.mt81xx_offset_Data_LoByte = ((uint8_t*)&size)[0];	
			FrameBuffer->uniFrame.im_mt81xx_format0B.mt81xx_offset_CS_LoByte = 0xff;
			FrameBuffer->uniFrame.im_mt81xx_format0B.mt81xx_offset_CS_HiByte = 0xff;
			FrameBuffer->uniFrame.im_mt81xx_format0B.mt81xx_offset_StopByte = 0x0d;				
			break; 
		case mt81xx_cmd_485:
			FrameBuffer->uniFrame.im_mt81xx_format08.mt81xx_offset_StartByte = 0x0a;
			FrameBuffer->uniFrame.im_mt81xx_format08.mt81xx_offset_SlaveAddr = ((uint8_t*)&dev_addr_plusNew)[0];
			FrameBuffer->uniFrame.im_mt81xx_format08.mt81xx_offset_Function = 0x08;
			FrameBuffer->uniFrame.im_mt81xx_format08.mt81xx_offset_MO_HiByte = ((uint8_t*)&reg)[1];
			FrameBuffer->uniFrame.im_mt81xx_format08.mt81xx_offset_MO_LoByte = ((uint8_t*)&reg)[0];
			FrameBuffer->uniFrame.im_mt81xx_format08.mt81xx_offset_SNH_HiByte = ((uint8_t*)&size)[3];		// only 00 allowe
			FrameBuffer->uniFrame.im_mt81xx_format08.mt81xx_offset_SNH_LoByte = ((uint8_t*)&size)[2];	
			FrameBuffer->uniFrame.im_mt81xx_format08.mt81xx_offset_SNL_HiByte = ((uint8_t*)&size)[1];		// only 00 allowe
			FrameBuffer->uniFrame.im_mt81xx_format08.mt81xx_offset_SNL_LoByte = ((uint8_t*)&size)[0];	
			FrameBuffer->uniFrame.im_mt81xx_format08.mt81xx_offset_NewAddr = ((uint8_t*)&dev_addr_plusNew)[1];	
			FrameBuffer->uniFrame.im_mt81xx_format08.mt81xx_offset_CS_LoByte = 0xff;
			FrameBuffer->uniFrame.im_mt81xx_format08.mt81xx_offset_CS_HiByte = 0xff;
			FrameBuffer->uniFrame.im_mt81xx_format08.mt81xx_offset_StopByte = 0x0d;		
			break;
		default:
			return -1;
	}
	
	return 0;
}


/****************************
** DELTA Inverter protocol
****************************/

inverter_delta_raw_t inverter_delta_info;

#if 0
// defined as matrix 2  // for 20K
inv_delta_regsmatrix_t delta_reg_def[32] = {
	{INV_DELTA_STATE_INIT, 			0x0, 					0, 	0, 0},
	{INV_DELTA_STATE_RDY, 			0x0, 					0, 	0, 0},
	{INV_DELTA_STATE_TIMEOUT,		0x0,					0,	0, 0},
	{INV_DELTA_STATE_ERR,			0x0,					0,	0, 0},
	{INV_DELTA_STATE_STOP,			0x0, 					0, 	0, 0},
	{INV_DELTA_STATE_SET_AC, 		INVERTER_DELTA_WR_REG, 	(uint32_t)&(inverter_delta_info.AC_measurementIndex) - (uint32_t)&inverter_delta_info, 0, 0},	// Set Management Index to Utility 
	{INV_DELTA_STATE_GET_AC_DATA,	0x041f, 				(uint32_t)&(inverter_delta_info.AC_measurementIndex) - (uint32_t)&inverter_delta_info, 16*2, 0},	// read Utility data from 1056(0x420) to 1071(0x42f), 16 short byte data
	{INV_DELTA_STATE_SET_DC,		INVERTER_DELTA_WR_REG, 	(uint32_t)&(inverter_delta_info.inverterStatus) - (uint32_t)&inverter_delta_info, 	0, 0x30},	// Set Management Index to DC 
	{INV_DELTA_STATE_GET_DC_DATA,	0x0417,  				(uint32_t)&(inverter_delta_info.inverterStatus) - (uint32_t)&inverter_delta_info, 	35*2, 0},		// read DC data from 1048(0x418) to 1082(0x43A), 35 short byte data
	{INV_DELTA_STATE_SET_SN, 		INVERTER_DELTA_WR_REG, 	(uint32_t)(inverter_delta_info.serialNum) - (uint32_t)&inverter_delta_info, 0, 0x30},	// Set Management Index to Utility 
	{INV_DELTA_STATE_GET_SN_DATA,	0xa00e, 				(uint32_t)(inverter_delta_info.serialNum) - (uint32_t)&inverter_delta_info, 6*2, 0},	// read Utility data from 1056(0x420) to 1071(0x42f), 16 short byte data
	{INV_DELTA_STATE_SET_EMSG,		INVERTER_DELTA_ERR_REG, (uint32_t)&(inverter_delta_info.eventIndex) - (uint32_t)&inverter_delta_info, 	0, 0},
	{INV_DELTA_STATE_GET_EMSG,		0x043f, 				(uint32_t)&(inverter_delta_info.eventIndex) - (uint32_t)&inverter_delta_info,	11*2, 0},
	{INV_DELTA_STATE_DELTA_END,		0x0,					0, 	0, 0}
};
#endif
// for 5K
inv_delta_regsmatrix_t delta_reg_def[36] = {
	{INV_DELTA_STATE_INIT, 			0x0, 					0, 	0, 0},
	{INV_DELTA_STATE_RDY, 			0x0, 					0, 	0, 0},
	{INV_DELTA_STATE_TIMEOUT,		0x0,					0,	0, 0},
	{INV_DELTA_STATE_ERR,			0x0,					0,	0, 0},
	{INV_DELTA_STATE_STOP,			0x0, 					0, 	0, 0},
	{INV_DELTA_STATE_SET_AC, 		INVERTER_DELTA_WR_REG, 	(uint32_t)&(inverter_delta_info.AC_measurementIndex) - (uint32_t)&inverter_delta_info, 0, 0},	// Set Management Index to Utility 
	{INV_DELTA_STATE_GET_AC_DATA,	0x041f, 				(uint32_t)&(inverter_delta_info.AC_measurementIndex) - (uint32_t)&inverter_delta_info, 16*2, 0},	// read Utility data from 1056(0x420) to 1071(0x42f), 16 short byte data
	{INV_DELTA_STATE_SET_DC,		INVERTER_DELTA_WR_REG, 	(uint32_t)&(inverter_delta_info.inverterStatus) - (uint32_t)&inverter_delta_info, 	0, 0x30},	// Set Management Index to DC 
	{INV_DELTA_STATE_GET_DC_DATA,	0x0417,  				(uint32_t)&(inverter_delta_info.inverterStatus) - (uint32_t)&inverter_delta_info, 	35*2, 0},		// read DC data from 1048(0x418) to 1082(0x43A), 35 short byte data
	{INV_DELTA_STATE_SET_AC_A, 		INVERTER_DELTA_WR_REG, 	(uint32_t)&(inverter_delta_info.AC_a_voltage) - (uint32_t)&inverter_delta_info, 0, 0x00},	// Set Management Index to AC_A
	{INV_DELTA_STATE_GET_AC_A_DATA,	0x0420, 				(uint32_t)&(inverter_delta_info.AC_a_voltage) - (uint32_t)&inverter_delta_info, 15*2, 0},	// read AC_A from 1057(0x421) to 1071(0x42f), 15 short byte data
	{INV_DELTA_STATE_SET_AC_B, 		INVERTER_DELTA_WR_REG, 	(uint32_t)&(inverter_delta_info.AC_b_voltage) - (uint32_t)&inverter_delta_info, 0, 0x01},	// Set Management Index to AC_B
	{INV_DELTA_STATE_GET_AC_B_DATA,	0x0420, 				(uint32_t)&(inverter_delta_info.AC_b_voltage) - (uint32_t)&inverter_delta_info, 15*2, 0},	// read AC_B from 1057(0x421) to 1071(0x42f), 15 short byte data
	{INV_DELTA_STATE_SET_AC_C, 		INVERTER_DELTA_WR_REG, 	(uint32_t)&(inverter_delta_info.AC_c_voltage) - (uint32_t)&inverter_delta_info, 0, 0x02},	// Set Management Index to AC_C
	{INV_DELTA_STATE_GET_AC_C_DATA,	0x0420, 				(uint32_t)&(inverter_delta_info.AC_c_voltage) - (uint32_t)&inverter_delta_info, 15*2, 0},	// read AC_C from 1057(0x421) to 1071(0x42f), 15 short byte data
	{INV_DELTA_STATE_SET_DC_1,		INVERTER_DELTA_WR_REG, 	(uint32_t)&(inverter_delta_info.DC_1_voltage) - (uint32_t)&inverter_delta_info, 0, 0x30},	// Set Management Index to DC_1 
	{INV_DELTA_STATE_GET_DC_1_DATA,	0x0420,  				(uint32_t)&(inverter_delta_info.DC_1_voltage) - (uint32_t)&inverter_delta_info, 15*2, 0},	// read DC data from 1057(0x421) to 1082(0x42f), 15 short byte data
	{INV_DELTA_STATE_SET_DC_2,		INVERTER_DELTA_WR_REG, 	(uint32_t)&(inverter_delta_info.DC_2_voltage) - (uint32_t)&inverter_delta_info, 0, 0x31},	// Set Management Index to DC_2 
	{INV_DELTA_STATE_GET_DC_2_DATA,	0x0420,  				(uint32_t)&(inverter_delta_info.DC_2_voltage) - (uint32_t)&inverter_delta_info, 15*2, 0},	// read DC data from 1057(0x421) to 1082(0x42f), 15 short byte data
	{INV_DELTA_STATE_SET_SN, 		INVERTER_DELTA_WR_REG, 	(uint32_t)(inverter_delta_info.serialNum) - (uint32_t)&inverter_delta_info, 0, 0x30},	// Set Management Index to Utility 
	{INV_DELTA_STATE_GET_SN_DATA,	0xa00e, 				(uint32_t)(inverter_delta_info.serialNum) - (uint32_t)&inverter_delta_info, 6*2, 0},	// read Utility data from 1056(0x420) to 1071(0x42f), 16 short byte data
	{INV_DELTA_STATE_SET_SN_20K, 	INVERTER_DELTA_WR_REG, 	(uint32_t)(inverter_delta_info.serialNum) - (uint32_t)&inverter_delta_info, 0, 0x30},	// Set Management Index to Utility 
	{INV_DELTA_STATE_GET_SN_20K,	0x046f, 				(uint32_t)(inverter_delta_info.serialNum) - (uint32_t)&inverter_delta_info, 8*2, 0},	// read Utility data from 1056(0x420) to 1071(0x42f), 16 short byte data
	{INV_DELTA_STATE_SET_EVNT1,		INVERTER_DELTA_ERR_REG, (uint32_t)&(inverter_delta_info.eventIndex) - (uint32_t)&inverter_delta_info, 	0, 0},
	{INV_DELTA_STATE_GET_EVNT1,		0x043f, 				(uint32_t)&(inverter_delta_info.eventIndex) - (uint32_t)&inverter_delta_info,	11*2, 0},
	{INV_DELTA_STATE_SET_EVNT2,		INVERTER_DELTA_ERR_REG, (uint32_t)&(inverter_delta_info.eventIndex) - (uint32_t)&inverter_delta_info, 	0, 0},
	{INV_DELTA_STATE_GET_EVNT2,		0x043f, 				(uint32_t)&(inverter_delta_info.eventIndex) - (uint32_t)&inverter_delta_info,	11*2, 0},
	{INV_DELTA_STATE_SET_EMSG_20K,	INVERTER_DELTA_WR_REG,  (uint32_t)&(inverter_delta_info.CE_Err0) - (uint32_t)&inverter_delta_info, 	0, 0},
	{INV_DELTA_STATE_GET_EMSG_20K,	0x0bff, 				(uint32_t)&(inverter_delta_info.CE_Err0) - (uint32_t)&inverter_delta_info,	48*2, 0},
	{INV_DELTA_STATE_SET_BASIC,		INVERTER_DELTA_WR_REG, 	(uint32_t)&(inverter_delta_info.inverterStatus) - (uint32_t)&inverter_delta_info, 	0, 0x30},	
	{INV_DELTA_STATE_GET_BASIC,		0x0417,  				(uint32_t)&(inverter_delta_info.inverterStatus) - (uint32_t)&inverter_delta_info, 	3*2, 0},	
	{INV_DELTA_STATE_SET_SUM,		INVERTER_DELTA_WR_REG, 	(uint32_t)&(inverter_delta_info.todayWh_Low) - (uint32_t)&inverter_delta_info, 	0, 0},	
	{INV_DELTA_STATE_GET_SUM,		0x042f,  				(uint32_t)&(inverter_delta_info.todayWh_Low) - (uint32_t)&inverter_delta_info, 	12*2, 0},		
	{INV_DELTA_STATE_DELTA_END_20K,		0x0,					0, 	0, 0},
	{INV_DELTA_STATE_DELTA_END,		0x0,					0, 	0, 0},
	{INV_DELTA_STATE_CHECK_TYPE,		0x0,					0, 	0, 0}
};



int check_modbus_CRC16(bool_t write_rsp, delta_format_rtn_t* FrameBuffer){
	int cal_size=0xff, i;
	unsigned short crc_result=0xffff;

	if(write_rsp){
		cal_size = 6;
		i=0;
	}else{
		// get cal_size
		cal_size = FrameBuffer->delta_offset_ByteCount;
		// start from slave address byte, not from start byte(0x0a)
		i=0;
		// add 3 bytes, total length is $len plus (addr + fun + len) 
		cal_size +=3;			
	}
	
	// calculate
	if(cal_size!=0xff){
		while(cal_size){
			crc_result = update_crc_16(crc_result, (char)((uint8_t*)(&(FrameBuffer->delta_offset_SlaveAddr)))[i]);
			//printf("byte=0x%.2x\n", ((uint8_t *)(&((raw_485_t*)FrameBuffer)->uniFrame))[i] );
			cal_size--;
			i++;
		}
		
		//printf("crc_check %.2x", ((uint8_t*)(&((raw_485_t*)FrameBuffer)->uniFrame))[i]);
		
		// fill the crc 
		//printf( "CRC=0x%x ,0x%x\n",((uint8_t*)(&(FrameBuffer->mt81xx_offset_StartByte)))[i], ((uint8_t*)(&(FrameBuffer->mt81xx_offset_StartByte)))[i+1] );
		if(((uint8_t*)(&(FrameBuffer->delta_offset_SlaveAddr)))[i] == ((uint8_t*)&crc_result)[0]){
			i++;
			if(((uint8_t*)(&(FrameBuffer->delta_offset_SlaveAddr)))[i] == ((uint8_t*)&crc_result)[1])
				return 1;
		}
		return -1;
	}else
		return -1;
}

int append_modbus_CRC16(void* FrameBuffer){
	int cal_size=0xff, i;
	unsigned short crc_result=0xffff;

	cal_size=6;
	
	// start from slave address byte, not from start byte(0x0a)
	i=0;
	
	// calculate
	if(cal_size!=0xff){
		while(cal_size){
			crc_result = update_crc_16(crc_result, (char)((uint8_t*)(&((delta_format04_t*)FrameBuffer)->delta_offset_SlaveAddr))[i]);
			cal_size--;
			i++;
		}
		
		// fill the crc 
		((uint8_t*)(&((delta_format04_t*)FrameBuffer)->delta_offset_SlaveAddr))[i] = ((uint8_t*)&crc_result)[0];
		i++;
		((uint8_t*)(&((delta_format04_t*)FrameBuffer)->delta_offset_SlaveAddr))[i] = ((uint8_t*)&crc_result)[1];
		
		return 1;
	}else
		return -1;
}

// assume Little Endian machine
int create_delta_cmd(void* FrameBuffer, bool_t cmd_wr, uint16_t dev_addr_plusNew, uint16_t reg, uint16_t data_and_size){
	
	// decide pkt size
	if(cmd_wr){
		((delta_format06_t*)FrameBuffer)->delta_offset_SlaveAddr = ((uint8_t*)&dev_addr_plusNew)[0];
		((delta_format06_t*)FrameBuffer)->delta_offset_Function = 0x06;
		((delta_format06_t*)FrameBuffer)->delta_offset_RA_HiByte = ((uint8_t*)&reg)[1];
		((delta_format06_t*)FrameBuffer)->delta_offset_RA_LoByte = ((uint8_t*)&reg)[0];
		((delta_format06_t*)FrameBuffer)->delta_offset_Data_HiByte = ((uint8_t*)&data_and_size)[1];		// only 00 allowe
		((delta_format06_t*)FrameBuffer)->delta_offset_Data_LoByte = ((uint8_t*)&data_and_size)[0];	
		((delta_format06_t*)FrameBuffer)->delta_offset_CS_LoByte = 0xff;
		((delta_format06_t*)FrameBuffer)->delta_offset_CS_HiByte = 0xff;	   
	}else{
		((delta_format04_t*)FrameBuffer)->delta_offset_SlaveAddr = ((uint8_t*)&dev_addr_plusNew)[0];
		((delta_format04_t*)FrameBuffer)->delta_offset_Function = 0x04;
		((delta_format04_t*)FrameBuffer)->delta_offset_SA_HiByte = ((uint8_t*)&reg)[1];
		((delta_format04_t*)FrameBuffer)->delta_offset_SA_LoByte = ((uint8_t*)&reg)[0];
		((delta_format04_t*)FrameBuffer)->delta_offset_NO_HiByte = 0x00;		// only 00 allowe
		((delta_format04_t*)FrameBuffer)->delta_offset_NO_LoByte = ((uint8_t*)&data_and_size)[0];	
		((delta_format04_t*)FrameBuffer)->delta_offset_CS_LoByte = 0xff;
		((delta_format04_t*)FrameBuffer)->delta_offset_CS_HiByte = 0xff;	   
	}
	return 0;
}


int inv_delta_state_chg_next(inv_delta_state_t* curr_inv_cmd_state, inv_cmd_event_t in_event){		//inv_delta_cmd_state_t
	
	switch (*curr_inv_cmd_state) {
		case INV_DELTA_STATE_INIT:
			if(!in_event)	// event normal
				*curr_inv_cmd_state = INV_DELTA_STATE_RDY;
			else
				*curr_inv_cmd_state = INV_DELTA_STATE_STOP;
		break;	
		case INV_DELTA_STATE_RDY:
			if(in_event == INV_CMD_EVENT_SCAN)
				*curr_inv_cmd_state = INV_DELTA_STATE_SCAN_RUN;
			else if(in_event == INV_CMD_EVENT_NORMAL)
				*curr_inv_cmd_state = INV_DELTA_STATE_SET_BASIC; //*curr_inv_cmd_state = INV_DELTA_STATE_SET_AC;
			else if (in_event&(INV_CMD_EVENT_ERR|INV_CMD_EVENT_STOP))
				*curr_inv_cmd_state = INV_DELTA_STATE_STOP;
		break;
		case INV_DELTA_STATE_ERR:
			if(!in_event)
				*curr_inv_cmd_state = INV_DELTA_STATE_RDY;
			else
				*curr_inv_cmd_state = INV_DELTA_STATE_STOP;
		break;
		case INV_DELTA_STATE_STOP:
			if(!in_event)
				*curr_inv_cmd_state = INV_DELTA_STATE_RDY;
			else
				*curr_inv_cmd_state = INV_DELTA_STATE_STOP;
		break;
		case INV_DELTA_STATE_TIMEOUT:
			if(!in_event)
				*curr_inv_cmd_state = INV_DELTA_STATE_ERR;
			else
				*curr_inv_cmd_state = INV_DELTA_STATE_STOP;
		break;
		
		case INV_DELTA_STATE_CHECK_TYPE:
			if(in_event == INV_CMD_EVENT_20K_TYPE)
				*curr_inv_cmd_state = INV_DELTA_STATE_SET_AC_A;
			else if(in_event == INV_CMD_EVENT_5K_TYPE)
				*curr_inv_cmd_state = INV_DELTA_STATE_SET_AC;
			else if (in_event&(INV_CMD_EVENT_ERR|INV_CMD_EVENT_STOP))
				*curr_inv_cmd_state = INV_DELTA_STATE_STOP;
		break;

		case INV_DELTA_STATE_SET_AC:
		case INV_DELTA_STATE_GET_AC_DATA:		
		case INV_DELTA_STATE_SET_DC:
		case INV_DELTA_STATE_GET_DC_DATA:		
		case INV_DELTA_STATE_SET_SN:
		case INV_DELTA_STATE_GET_SN_DATA:
		case INV_DELTA_STATE_SET_EVNT1:
		case INV_DELTA_STATE_GET_EVNT1:
		case INV_DELTA_STATE_SET_EVNT2:
		case INV_DELTA_STATE_GET_EVNT2:
		case INV_DELTA_STATE_SET_BASIC:
		case INV_DELTA_STATE_GET_BASIC:
		case INV_DELTA_STATE_SET_AC_A:
		case INV_DELTA_STATE_GET_AC_A_DATA:
		case INV_DELTA_STATE_SET_AC_B:
		case INV_DELTA_STATE_GET_AC_B_DATA:
		case INV_DELTA_STATE_SET_AC_C:
		case INV_DELTA_STATE_GET_AC_C_DATA:
		case INV_DELTA_STATE_SET_DC_1:
		case INV_DELTA_STATE_GET_DC_1_DATA:
		case INV_DELTA_STATE_SET_DC_2:
		case INV_DELTA_STATE_GET_DC_2_DATA:
		case INV_DELTA_STATE_SET_EMSG_20K:
		case INV_DELTA_STATE_GET_EMSG_20K:
		case INV_DELTA_STATE_SET_SN_20K:
		case INV_DELTA_STATE_GET_SN_20K:
		case INV_DELTA_STATE_SET_SUM:
		case INV_DELTA_STATE_GET_SUM:
			if (in_event == INV_CMD_EVENT_NOCHANGE)
				*curr_inv_cmd_state = *curr_inv_cmd_state;
			else if(in_event == INV_CMD_EVENT_NORMAL)	// event normal
				*curr_inv_cmd_state = *curr_inv_cmd_state + 1;
			else if (in_event&INV_CMD_EVENT_TIMEOUT)
				*curr_inv_cmd_state = INV_DELTA_STATE_TIMEOUT;
			else if (in_event&INV_CMD_EVENT_ERR)
				*curr_inv_cmd_state = INV_DELTA_STATE_ERR;
			else if (in_event&INV_CMD_EVENT_STOP)
				*curr_inv_cmd_state = INV_DELTA_STATE_STOP;
		break;
		
		case INV_DELTA_STATE_SCAN_RUN:
			if (in_event == INV_CMD_EVENT_NOCHANGE)
				*curr_inv_cmd_state = INV_DELTA_STATE_SCAN_REPEAT;
			else if(in_event == INV_CMD_EVENT_STOP)
				*curr_inv_cmd_state = INV_DELTA_STATE_STOP;
			else if (in_event == INV_CMD_EVENT_END)
				*curr_inv_cmd_state = INV_DELTA_STATE_SCAN_END;
			else 
				*curr_inv_cmd_state = *curr_inv_cmd_state;
		break;

		case INV_DELTA_STATE_SCAN_REPEAT:
			if (in_event == INV_CMD_EVENT_NORMAL)
				*curr_inv_cmd_state = INV_DELTA_STATE_SCAN_RUN;		
			else if (in_event == INV_CMD_EVENT_NOCHANGE)
				*curr_inv_cmd_state = INV_DELTA_STATE_SCAN_REPEAT;
			else if(in_event == INV_CMD_EVENT_STOP)
				*curr_inv_cmd_state = INV_DELTA_STATE_STOP;
			else if (in_event == INV_CMD_EVENT_END)
				*curr_inv_cmd_state = INV_DELTA_STATE_SCAN_END;
			else 
				*curr_inv_cmd_state = *curr_inv_cmd_state;
		break;		

		case INV_DELTA_STATE_DELTA_END:
		case INV_DELTA_STATE_DELTA_END_20K:	
			if(in_event == INV_CMD_EVENT_NORMAL)	// event normal
				*curr_inv_cmd_state = INV_DELTA_STATE_RDY;
			else if(in_event == INV_CMD_EVENT_END)
				*curr_inv_cmd_state = INV_DELTA_STATE_INIT;
			else
				*curr_inv_cmd_state = INV_DELTA_STATE_STOP;
		break;
		
		case INV_DELTA_STATE_SCAN_END:
			if(in_event == INV_CMD_EVENT_STOP)
				*curr_inv_cmd_state = INV_DELTA_STATE_STOP;
		
		default:;
	}
	
	return 1;
}


int convert_invmsg_to_asci_delta(char *buff, inverter_msg2host_delta_t* msg2host){
	int n;
	double tmp_value;
	char tmp_time[15]="";
	uint16_t inv_status = INV_STATUS_UNKNOWN;

	inv_status = msg2host->inverterStatus;
	if( inv_status < 9 )
		inv_status  = INV_STATUS_WAIT;
	else if( inv_status < 50 )
		inv_status  = INV_STATUS_NORMAL;
	else if (inv_status == 50)
		inv_status  = INV_STATUS_FAULT;
	else
		inv_status  = INV_STATUS_UNKNOWN;	

	syscall_getRsp("date +%Y%m%d%H%M%S", tmp_time);
	tmp_time[14] = '\0';

	if(msg2host->use_protocol == PROTOCOL_TYPE_20K){
		n = sprintf(buff, "%04d,", msg2host->inverterId); 
		buff += n;
		n = sprintf(buff, "%s,", "DELTA_20K"); 
		buff += n;
		n = sprintf(buff, "%d00W,", msg2host->dev_type); 
		buff += n;
		n = sprintf(buff, "%s_%04d,", msg2host->serialNum, msg2host->inverterId); 
		buff += n;
		n = sprintf(buff, "%s,", tmp_time); 
		buff += n;
		n = sprintf(buff, "%d,", inv_status);  // 
		buff += n;
		n = sprintf(buff, "%d,", msg2host->reconnectedTime);  // unit: second
		buff += n;

		tmp_value = (double)msg2host->todayWh_High;
		tmp_value = tmp_value * 65536;
		tmp_value += (double)msg2host->todayWh_Low;
		//tmp_value = tmp_value * 10;
		n = sprintf(buff, "%.0f,", tmp_value); // 10Wh
		buff += n;	
		tmp_value = (double)msg2host->todayRuntime_High;
		tmp_value = tmp_value * 65536;
		tmp_value += (double)msg2host->todayRuntime_Low;
		n = sprintf(buff, "%.0f,", tmp_value); // 1 sec
		buff += n;	
		tmp_value = (double)msg2host->lifeWh_High;
		tmp_value = tmp_value * 65536;
		tmp_value += (double)msg2host->lifeWh_Low;
		//tmp_value = tmp_value * 10;
		n = sprintf(buff, "%.0f,", tmp_value); // 10Wh
		buff += n;	
		tmp_value = (double)msg2host->lifeRuntime_High;
		tmp_value = tmp_value * 65536;
		tmp_value += (double)msg2host->lifeRuntime_Low;
		n = sprintf(buff, "%.0f,", tmp_value); // 1 sec
		buff += n;	
		n = sprintf(buff, "%d,", msg2host->delta5k_AmbTemp);  // 1 C
		buff += n;	
		n = sprintf(buff, "%d,", msg2host->delta5k_BoostHsTemp);  // 1 C
		buff += n;	
		n = sprintf(buff, "%d,", msg2host->delta5k_InvHsTemp);  // 1 C
		buff += n;

		tmp_value = (double)msg2host->AC_A_voltage;	// 0.1V
		tmp_value = tmp_value /10;
		n = sprintf(buff, "%.1f,", tmp_value);
		buff += n;
		tmp_value = (double)msg2host->AC_A_current; // 0.01A
		tmp_value = tmp_value /100;
		n = sprintf(buff, "%.2f,", tmp_value); 
		buff += n;
		n = sprintf(buff, "%d,", msg2host->AC_A_wattage);	// 1W
		buff += n;
		tmp_value = (double)msg2host->AC_A_frequency; // 0.01Hz
		tmp_value = tmp_value /100;
		n = sprintf(buff, "%.2f,", tmp_value); 
		buff += n;	
		tmp_value = (double)msg2host->AC_A_redundantVoltage; // 0.1V
		tmp_value = tmp_value /10;
		n = sprintf(buff, "%.1f,", tmp_value);	 // 0.1V
		buff += n;	
		tmp_value = (double)msg2host->AC_A_redundantFrequency; // 0.01Hz
		tmp_value = tmp_value /100;	
		n = sprintf(buff, "%.2f,", tmp_value); 
		buff += n;	
		n = sprintf(buff, "%d,", msg2host->AC_A_adcVoltage);	// 1 ADC count 
		buff += n;	
		n = sprintf(buff, "%d,", msg2host->AC_A_adcCurrent);	// 1 ADC count
		buff += n;	
		n = sprintf(buff, "%d,", msg2host->AC_A_adcWattage);  // 1 ADC count
		buff += n;	
		n = sprintf(buff, "%d,", msg2host->AC_A_adcRedundantVoltage);  // 1 ADC count
		buff += n;

		tmp_value = (double)msg2host->AC_B_voltage;	// 0.1V
		tmp_value = tmp_value /10;
		n = sprintf(buff, "%.1f,", tmp_value);
		buff += n;
		tmp_value = (double)msg2host->AC_B_current; // 0.01A
		tmp_value = tmp_value /100;
		n = sprintf(buff, "%.2f,", tmp_value); 
		buff += n;
		n = sprintf(buff, "%d,", msg2host->AC_B_wattage);	// 1W
		buff += n;
		tmp_value = (double)msg2host->AC_B_frequency; // 0.01Hz
		tmp_value = tmp_value /100;
		n = sprintf(buff, "%.2f,", tmp_value); 
		buff += n;	
		tmp_value = (double)msg2host->AC_B_redundantVoltage; // 0.1V
		tmp_value = tmp_value /10;
		n = sprintf(buff, "%.1f,", tmp_value);	 // 0.1V
		buff += n;	
		tmp_value = (double)msg2host->AC_B_redundantFrequency; // 0.01Hz
		tmp_value = tmp_value /100;	
		n = sprintf(buff, "%.2f,", tmp_value); 
		buff += n;	
		n = sprintf(buff, "%d,", msg2host->AC_B_adcVoltage);	// 1 ADC count 
		buff += n;	
		n = sprintf(buff, "%d,", msg2host->AC_B_adcCurrent);	// 1 ADC count
		buff += n;	
		n = sprintf(buff, "%d,", msg2host->AC_B_adcWattage);  // 1 ADC count
		buff += n;	
		n = sprintf(buff, "%d,", msg2host->AC_B_adcRedundantVoltage);  // 1 ADC count
		buff += n;

		tmp_value = (double)msg2host->AC_C_voltage;	// 0.1V
		tmp_value = tmp_value /10;
		n = sprintf(buff, "%.1f,", tmp_value);
		buff += n;
		tmp_value = (double)msg2host->AC_C_current; // 0.01A
		tmp_value = tmp_value /100;
		n = sprintf(buff, "%.2f,", tmp_value); 
		buff += n;
		n = sprintf(buff, "%d,", msg2host->AC_C_wattage);	// 1W
		buff += n;
		tmp_value = (double)msg2host->AC_C_frequency; // 0.01Hz
		tmp_value = tmp_value /100;
		n = sprintf(buff, "%.2f,", tmp_value); 
		buff += n;	
		tmp_value = (double)msg2host->AC_C_redundantVoltage; // 0.1V
		tmp_value = tmp_value /10;
		n = sprintf(buff, "%.1f,", tmp_value);	 // 0.1V
		buff += n;	
		tmp_value = (double)msg2host->AC_C_redundantFrequency; // 0.01Hz
		tmp_value = tmp_value /100;	
		n = sprintf(buff, "%.2f,", tmp_value); 
		buff += n;	
		n = sprintf(buff, "%d,", msg2host->AC_C_adcVoltage);	// 1 ADC count 
		buff += n;	
		n = sprintf(buff, "%d,", msg2host->AC_C_adcCurrent);	// 1 ADC count
		buff += n;	
		n = sprintf(buff, "%d,", msg2host->AC_C_adcWattage);  // 1 ADC count
		buff += n;	
		n = sprintf(buff, "%d,", msg2host->AC_C_adcRedundantVoltage);  // 1 ADC count
		buff += n;

		tmp_value = (double)msg2host->DC_1_voltage;	// 0.1V
		tmp_value = tmp_value /10;
		n = sprintf(buff, "%.1f,", tmp_value); 
		buff += n;
		tmp_value = (double)msg2host->DC_1_current; // 0.01A
		tmp_value = tmp_value /100;
		n = sprintf(buff, "%.2f,", tmp_value); 
		buff += n;
		n = sprintf(buff, "%d,", msg2host->DC_1_wattage);	// 1W
		buff += n;
		n = sprintf(buff, "%d,", msg2host->DC_1_adcVoltage);	// 1 ADC count 
		buff += n;	
		n = sprintf(buff, "%d,", msg2host->DC_1_adcCurrent);	// 1 ADC count
		buff += n;	
		n = sprintf(buff, "%d,", msg2host->DC_1_adcWattage);  // 1 ADC count
		buff += n;	

		tmp_value = (double)msg2host->DC_2_voltage;	// 0.1V
		tmp_value = tmp_value /10;
		n = sprintf(buff, "%.1f,", tmp_value); 
		buff += n;
		tmp_value = (double)msg2host->DC_2_current; // 0.01A
		tmp_value = tmp_value /100;
		n = sprintf(buff, "%.2f,", tmp_value); 
		buff += n;
		n = sprintf(buff, "%d,", msg2host->DC_2_wattage);	// 1W
		buff += n;
		n = sprintf(buff, "%d,", msg2host->DC_2_adcVoltage);	// 1 ADC count 
		buff += n;	
		n = sprintf(buff, "%d,", msg2host->DC_2_adcCurrent);	// 1 ADC count
		buff += n;	
		n = sprintf(buff, "%d,", msg2host->DC_2_adcWattage);  // 1 ADC count
		buff += n;	

		n = sprintf(buff, "%d_%d_%d_%d_%d_%d_%d_%d_%d_%d_%d,", msg2host->eventIndex, msg2host->event0, msg2host->event1, msg2host->event2, msg2host->event3, msg2host->event4, msg2host->event5, msg2host->event6, msg2host->event7, msg2host->event8, msg2host->event9);  // 1 ADC count
		buff += n;

		n = sprintf(buff, "0x%04x_%04x_%04x,", msg2host->msg_errE47_E32, msg2host->msg_errE31_E16, msg2host->msg_errE15_E00);  // 1 ADC count
		buff += n;

		n = sprintf(buff, "0x%04x,", msg2host->msg_warW15_W00);  // 1 ADC count
		buff += n;

		n = sprintf(buff, "0x%04x_%04x_%04x_%04x_%04x", msg2host->msg_fauF79_F64, msg2host->msg_fauF63_F48, msg2host->msg_fauF47_F32, msg2host->msg_fauF31_F16, msg2host->msg_fauF15_F00);  // 1 ADC count
		buff += n;
	}
	else
	{
		n = sprintf(buff, "%04d,", msg2host->inverterId); 
		buff += n;
		n = sprintf(buff, "%s,", "DELTA"); 
		buff += n;
		n = sprintf(buff, "%d00W,", msg2host->dev_type); 
		buff += n;
		n = sprintf(buff, "%s_%04d,", msg2host->serialNum, msg2host->inverterId); 
		buff += n;
		n = sprintf(buff, "%s,", tmp_time); 
		buff += n;
		n = sprintf(buff, "%d,", inv_status);  // 
		buff += n;
		n = sprintf(buff, "%d,", msg2host->reconnectedTime);  // unit: second
		buff += n;
		n = sprintf(buff, "%d,", msg2host->DC_measurementIndex); 
		buff += n;	
		tmp_value = (double)msg2host->DC_voltage;	// 0.1V
		tmp_value = tmp_value /10;
		n = sprintf(buff, "%.1f,", tmp_value); 
		buff += n;
		tmp_value = (double)msg2host->DC_current; // 0.01A
		tmp_value = tmp_value /100;
		n = sprintf(buff, "%.2f,", tmp_value); 
		buff += n;
		n = sprintf(buff, "%d,", msg2host->DC_wattage);	// 1W
		buff += n;
		tmp_value = (double)msg2host->DC_frequency; // 0.01Hz
		tmp_value = tmp_value /100;
		n = sprintf(buff, "%.2f,", tmp_value); 
		buff += n;	
		n = sprintf(buff, "%d,", msg2host->DC_percentage);	// 1% 
		buff += n;	
		tmp_value = (double)msg2host->DC_redundantVoltage; // 0.1V
		tmp_value = tmp_value /10;
		n = sprintf(buff, "%.1f,", tmp_value);	 // 0.1V
		buff += n;	
		tmp_value = (double)msg2host->DC_redundantFrequency; // 0.01Hz
		tmp_value = tmp_value /100;	
		n = sprintf(buff, "%.2f,", tmp_value); 
		buff += n;	
		n = sprintf(buff, "%d,", msg2host->DC_adcVoltage);	// 1 ADC count 
		buff += n;	
		n = sprintf(buff, "%d,", msg2host->DC_adcCurrent);	// 1 ADC count
		buff += n;	
		n = sprintf(buff, "%d,", msg2host->DC_adcWattage);  // 1 ADC count
		buff += n;	
		n = sprintf(buff, "%d,", msg2host->DC_adcRedundantVoltage);  // 1 ADC count
		buff += n;
		tmp_value = (double)msg2host->todayWh_High;
		tmp_value = tmp_value * 65536;
		tmp_value += (double)msg2host->todayWh_Low;
		//tmp_value = tmp_value * 10;
		n = sprintf(buff, "%.0f,", tmp_value); // 10Wh
		buff += n;	
		tmp_value = (double)msg2host->todayRuntime_High;
		tmp_value = tmp_value * 65536;
		tmp_value += (double)msg2host->todayRuntime_Low;
		n = sprintf(buff, "%.0f,", tmp_value); // 1 sec
		buff += n;	
		tmp_value = (double)msg2host->lifeWh_High;
		tmp_value = tmp_value * 65536;
		tmp_value += (double)msg2host->lifeWh_Low;
		//tmp_value = tmp_value * 10;
		n = sprintf(buff, "%.0f,", tmp_value); // 10Wh
		buff += n;	
		tmp_value = (double)msg2host->lifeRuntime_High;
		tmp_value = tmp_value * 65536;
		tmp_value += (double)msg2host->lifeRuntime_Low;
		n = sprintf(buff, "%.0f,", tmp_value); // 1 sec
		buff += n;	
		n = sprintf(buff, "%d,", msg2host->delta5k_AmbTemp);  // 1 C
		buff += n;	
		n = sprintf(buff, "%d,", msg2host->delta5k_BoostHsTemp);  // 1 C
		buff += n;	
		n = sprintf(buff, "%d,", msg2host->delta5k_InvHsTemp);  // 1 C
		buff += n;
		
		n = sprintf(buff, "%d,", msg2host->AC_measurementIndex); 
		buff += n;	
		tmp_value = (double)msg2host->AC_voltage;	// 0.1V
		tmp_value = tmp_value /10;
		n = sprintf(buff, "%.1f,", tmp_value);
		buff += n;
		tmp_value = (double)msg2host->AC_current; // 0.01A
		tmp_value = tmp_value /100;
		n = sprintf(buff, "%.2f,", tmp_value); 
		buff += n;
		n = sprintf(buff, "%d,", msg2host->AC_wattage);	// 1W
		buff += n;
		tmp_value = (double)msg2host->AC_frequency; // 0.01Hz
		tmp_value = tmp_value /100;
		n = sprintf(buff, "%.2f,", tmp_value); 
		buff += n;	
		n = sprintf(buff, "%d,", msg2host->AC_percentage);	// 1% 
		buff += n;	
		tmp_value = (double)msg2host->AC_redundantVoltage; // 0.1V
		tmp_value = tmp_value /10;
		n = sprintf(buff, "%.1f,", tmp_value);	 // 0.1V
		buff += n;	
		tmp_value = (double)msg2host->AC_redundantFrequency; // 0.01Hz
		tmp_value = tmp_value /100;	
		n = sprintf(buff, "%.2f,", tmp_value); 
		buff += n;	
		n = sprintf(buff, "%d,", msg2host->AC_adcVoltage);	// 1 ADC count 
		buff += n;	
		n = sprintf(buff, "%d,", msg2host->AC_adcCurrent);	// 1 ADC count
		buff += n;	
		n = sprintf(buff, "%d,", msg2host->AC_adcWattage);  // 1 ADC count
		buff += n;	
		n = sprintf(buff, "%d,", msg2host->AC_adcRedundantVoltage);  // 1 ADC count
		buff += n;
		
		n = sprintf(buff, "%d_%d_%d_%d_%d_%d_%d_%d_%d_%d_%d", msg2host->eventIndex, msg2host->event0, msg2host->event1, msg2host->event2, msg2host->event3, msg2host->event4, msg2host->event5, msg2host->event6, msg2host->event7, msg2host->event8, msg2host->event9);  // 1 ADC count
		buff += n;
	}
	
	buff[0] = '\0';
	
	return 1; 
	
}

int update_inv_msg2host_delta(inverter_msg2host_delta_t *data, inverter_delta_raw_t *inverter_info, uint16_t invId){
	
	data->inverterId							= invId												;
	data->inverterStatus			  	= inverter_info->inverterStatus			    ;
	data->reconnectedTime					= inverter_info->reconnectedTime				;
	data->dev_type					= inverter_info->Device_Type;

	data->DC_measurementIndex			= inverter_info->DC_measurementIndex		;
	data->DC_voltage							= inverter_info->DC_voltage						  ;
	data->DC_current							= inverter_info->DC_current						  ;
	data->DC_wattage							= inverter_info->DC_wattage						  ;
	data->DC_frequency						= inverter_info->DC_frequency					  ;
	data->DC_percentage						= inverter_info->DC_percentage					;
	data->DC_redundantVoltage			= inverter_info->DC_redundantVoltage		;
	data->DC_redundantFrequency		= inverter_info->DC_redundantFrequency	;
	data->DC_adcVoltage						= inverter_info->DC_adcVoltage					;
	data->DC_adcCurrent						= inverter_info->DC_adcCurrent					;
	data->DC_adcWattage						= inverter_info->DC_adcWattage					;
	data->DC_adcRedundantVoltage	= inverter_info->DC_adcRedundantVoltage ;
	data->todayWh_Low							= inverter_info->todayWh_Low						;
	data->todayWh_High						= inverter_info->todayWh_High					  ;
	data->todayRuntime_Low				= inverter_info->todayRuntime_Low			  ;
	data->todayRuntime_High				= inverter_info->todayRuntime_High			;
	data->lifeWh_Low							= inverter_info->lifeWh_Low						  ;
	data->lifeWh_High							= inverter_info->lifeWh_High						;
	data->lifeRuntime_Low					= inverter_info->lifeRuntime_Low				;
	data->lifeRuntime_High				= inverter_info->lifeRuntime_High			  ;

	data->delta5k_AmbTemp 				= inverter_info->delta5k_AmbTemp				;
	data->delta5k_BoostHsTemp 		= inverter_info->delta5k_BoostHsTemp		;
	data->delta5k_InvHsTemp 			= inverter_info->delta5k_InvHsTemp			;
	
	data->AC_measurementIndex			= inverter_info->AC_measurementIndex		;
	data->AC_voltage							= inverter_info->AC_voltage						  ;
	data->AC_current							= inverter_info->AC_current						  ;
	data->AC_wattage							= inverter_info->AC_wattage						  ;
	data->AC_frequency						= inverter_info->AC_frequency					  ;
	data->AC_percentage						= inverter_info->AC_percentage					;
	data->AC_redundantVoltage			= inverter_info->AC_redundantVoltage		;
	data->AC_redundantFrequency		= inverter_info->AC_redundantFrequency	;
	data->AC_adcVoltage						= inverter_info->AC_adcVoltage					;
	data->AC_adcCurrent						= inverter_info->AC_adcCurrent					;
	data->AC_adcWattage						= inverter_info->AC_adcWattage					;
	data->AC_adcRedundantVoltage	= inverter_info->AC_adcRedundantVoltage ;

	data->eventIndex							= inverter_info->eventIndex							;
	data->event0									= inverter_info->eventOff0							;
	data->event1									= inverter_info->eventOff1							;
	data->event2									= inverter_info->eventOff2							;
	data->event3									= inverter_info->eventOff3							;
	data->event4									= inverter_info->eventOff4							;
	data->event5									= inverter_info->eventOff5							;
	data->event6									= inverter_info->eventOff6							;
	data->event7									= inverter_info->eventOff7							;
	data->event8									= inverter_info->eventOff8							;
	data->event9									= inverter_info->eventOff9							;

//memcpy(data->serialNum, inverter_info->serialNum, 13);	// for 5K
	memcpy(data->serialNum, inverter_info->serialNum, 17);	// for 5K

	//inverter_info->serialNum[12] = '\0';	// for 5K
	inverter_info->serialNum[16] = '\0';	// for 20K


	data->use_protocol	= inverter_info->use_protocol;

	return 1;
}


int update_inv_msg2host_delta20K(inverter_msg2host_delta_t *data, inverter_delta_raw_t *inverter_info, uint16_t invId){
	
	data->inverterId					= invId;
	data->inverterStatus			  	= inverter_info->inverterStatus;
	data->reconnectedTime				= inverter_info->reconnectedTime;
	data->dev_type						= inverter_info->Device_Type;

	data->todayWh_Low					= inverter_info->todayWh_Low;
	data->todayWh_High					= inverter_info->todayWh_High;
	data->todayRuntime_Low				= inverter_info->todayRuntime_Low;
	data->todayRuntime_High				= inverter_info->todayRuntime_High;
	data->lifeWh_Low					= inverter_info->lifeWh_Low;
	data->lifeWh_High					= inverter_info->lifeWh_High;
	data->lifeRuntime_Low				= inverter_info->lifeRuntime_Low;
	data->lifeRuntime_High				= inverter_info->lifeRuntime_High;

	data->delta5k_AmbTemp 				= inverter_info->delta5k_AmbTemp;
	data->delta5k_BoostHsTemp 			= inverter_info->delta5k_BoostHsTemp;
	data->delta5k_InvHsTemp				= inverter_info->delta5k_InvHsTemp;
	data->Temp4							= inverter_info->Temp4;

	data->AC_A_voltage							= inverter_info->AC_a_voltage;
	data->AC_A_current							= inverter_info->AC_a_current;
	data->AC_A_wattage							= inverter_info->AC_a_wattage;
	data->AC_A_frequency						= inverter_info->AC_a_frequency;
	data->AC_A_redundantVoltage					= inverter_info->AC_a_redundantVoltage;
	data->AC_A_redundantFrequency				= inverter_info->AC_a_redundantFrequency;
	data->AC_A_adcVoltage						= inverter_info->AC_a_adcVoltage;
	data->AC_A_adcCurrent						= inverter_info->AC_a_adcCurrent;
	data->AC_A_adcWattage						= inverter_info->AC_a_adcWattage;
	data->AC_A_adcRedundantVoltage				= inverter_info->AC_a_adcRedundantVoltage;

	data->AC_B_voltage							= inverter_info->AC_b_voltage;
	data->AC_B_current							= inverter_info->AC_b_current;
	data->AC_B_wattage							= inverter_info->AC_b_wattage;
	data->AC_B_frequency						= inverter_info->AC_b_frequency;
	data->AC_B_redundantVoltage					= inverter_info->AC_b_redundantVoltage;
	data->AC_B_redundantFrequency				= inverter_info->AC_b_redundantFrequency;
	data->AC_B_adcVoltage						= inverter_info->AC_b_adcVoltage;
	data->AC_B_adcCurrent						= inverter_info->AC_b_adcCurrent;
	data->AC_B_adcWattage						= inverter_info->AC_b_adcWattage;
	data->AC_B_adcRedundantVoltage				= inverter_info->AC_b_adcRedundantVoltage;

	data->AC_C_voltage							= inverter_info->AC_c_voltage;
	data->AC_C_current							= inverter_info->AC_c_current;
	data->AC_C_wattage							= inverter_info->AC_c_wattage;
	data->AC_C_frequency						= inverter_info->AC_c_frequency;
	data->AC_C_redundantVoltage					= inverter_info->AC_c_redundantVoltage;
	data->AC_C_redundantFrequency				= inverter_info->AC_c_redundantFrequency;
	data->AC_C_adcVoltage						= inverter_info->AC_c_adcVoltage;
	data->AC_C_adcCurrent						= inverter_info->AC_c_adcCurrent;
	data->AC_C_adcWattage						= inverter_info->AC_c_adcWattage;
	data->AC_C_adcRedundantVoltage				= inverter_info->AC_c_adcRedundantVoltage;	

	data->DC_1_voltage							= inverter_info->DC_1_voltage;
	data->DC_1_current							= inverter_info->DC_1_current;
	data->DC_1_wattage							= inverter_info->DC_1_wattage;
	data->DC_1_adcVoltage						= inverter_info->DC_1_adcVoltage;
	data->DC_1_adcCurrent						= inverter_info->DC_1_adcCurrent;
	data->DC_1_adcWattage						= inverter_info->DC_1_adcWattage;

	data->DC_2_voltage							= inverter_info->DC_2_voltage;
	data->DC_2_current							= inverter_info->DC_2_current;
	data->DC_2_wattage							= inverter_info->DC_2_wattage;
	data->DC_2_adcVoltage						= inverter_info->DC_2_adcVoltage;
	data->DC_2_adcCurrent						= inverter_info->DC_2_adcCurrent;
	data->DC_2_adcWattage						= inverter_info->DC_2_adcWattage;

	data->msg_errE15_E00	= inverter_info->CE_Err0;
	data->msg_errE31_E16	= inverter_info->CE_Err1;
	data->msg_errE47_E32	= inverter_info->CE_Err2;
	
	data->msg_warW15_W00	= inverter_info->CE_War0;

	data->msg_fauF15_F00	= inverter_info->CE_Fau0;
	data->msg_fauF31_F16	= inverter_info->CE_Fau1;
	data->msg_fauF47_F32	= inverter_info->CE_Fau2;
	data->msg_fauF63_F48	= inverter_info->CE_Fau3;
	data->msg_fauF79_F64	= inverter_info->CE_Fau4;

	data->eventIndex		= inverter_info->eventIndex	;
	data->event0			= inverter_info->eventOff0	;
	data->event1			= inverter_info->eventOff1	;
	data->event2			= inverter_info->eventOff2	;
	data->event3			= inverter_info->eventOff3	;
	data->event4			= inverter_info->eventOff4	;
	data->event5			= inverter_info->eventOff5	;
	data->event6			= inverter_info->eventOff6	;
	data->event7			= inverter_info->eventOff7	;
	data->event8			= inverter_info->eventOff8	;
	data->event9			= inverter_info->eventOff9	;

	//memcpy(data->serialNum, inverter_info->serialNum, 13);	// for 5K
	memcpy(data->serialNum, inverter_info->serialNum, 17);	// for 20K
	//printf("target:-%s-, src:-%s-, srcP=%p\n", data->serialNum, inverter_info->serialNum, inverter_info->serialNum);
	//inverter_info->serialNum[12] = '\0';	// for 5K
	inverter_info->serialNum[16] = '\0';	// for 20K

	data->use_protocol	= inverter_info->use_protocol;

	return 1;
}


int getRegsMatrix_Delta(inverter_delta_raw_t* base_ptr, inv_delta_state_t state, uint16_t* out_addr, uint16_t** out_ptr, uint8_t* out_size, uint16_t* out_wr_value){
	uint32_t i;
	bool_t isFound=FALSE;
		
	for (i=0; i<(sizeof(delta_reg_def)/sizeof(inv_delta_regsmatrix_t)); i++){
		if(delta_reg_def[i].state_num == state){
			isFound = TRUE;
			*out_addr = delta_reg_def[i].reg_addr;
			*out_ptr = (uint16_t*)((uint32_t)base_ptr + (uint32_t)delta_reg_def[i].offset);
			*out_size = delta_reg_def[i].cpy_size;
			*out_wr_value = delta_reg_def[i].wr_data;
			break;
		}
	}

	if (isFound)
		return 1;
	else{
		printf("State #%d no found in matrix\n", state);
		*out_addr = 0x0;
		*out_ptr = NULL;
		*out_size = 0;
		*out_wr_value = 0;
		return -1;
	}
}


int inverter_scan_delta(scan_inv_info_t* inv_info, char* scan_port_name){
	bool_t inv_delta_wr_cmd=FALSE;

  	int fd_rs485;
  	//int fd_gpio88;

  	int bytes;
	delta_format04_t raw_485_pkg_rd;
	//	delta_format06_t raw_485_pkg_wr;

  	uint8_t rcv_buff[128];
  	uint8_t *bufptr;

  	int nbytes;

	//  int i;
	//	raw_485_t raw_485_pkg;
	int req_size, rsp_size;

  	uint8_t rs232_timeout = FALSE;
	//  uint8_t rcv_len;
  
 	int valid_cnt=0;
 	bool_t isASCII=FALSE;
 	uint8_t * tmp_bufp;
	
	uint16_t inv_addr, inv_reg2rd;
	
	inv_delta_state_t inv_cmd_state = INV_DELTA_STATE_INIT;
	uint8_t cpy_size;
	uint16_t * cpy_ptr=NULL;
	
//	bool_t wr_data;
	
	int net_timeout_cnt, rs485_timeout_cnt;
	
	//printf("In handle rs232 thread !!\n");

	//
	// Initial UART
	if(!scan_port_name)
		return -1;

	fd_rs485 = open_port_RS485(scan_port_name, O_RDWR| O_NOCTTY);
  	if(fd_rs485 < 0){
  		printf("Open port %s failed !!", scan_port_name);
  		return -1;
  	}	
	while(1){

		switch (inv_cmd_state){

		case INV_DELTA_STATE_INIT:
			inv_addr = INV_START_ADDR - 1;
			inv_reg2rd = 0;
			isASCII=FALSE;
			inv_info->num = 0;
		break;

		case INV_DELTA_STATE_RDY:
			cpy_size = 0;
			inv_reg2rd = 0;
			isASCII=FALSE;
		break;
		
		case INV_DELTA_STATE_TIMEOUT:
			cpy_size = 0;
			inv_reg2rd = 0;
		break;
		
		case INV_DELTA_STATE_STOP:
			cpy_size = 0;
			inv_reg2rd = 0;
		break;
		
		case INV_DELTA_STATE_SCAN_RUN:
			inv_reg2rd = 0x0407;		//
			inv_addr = inv_addr + 1;
			isASCII=FALSE;
			//usleep(1000000);
		break;

		case INV_DELTA_STATE_SCAN_REPEAT:
			inv_reg2rd = 0x0407;		//
			//inv_addr = inv_addr;
			isASCII=FALSE;
			//usleep(1000000);
		break;
		
		case INV_DELTA_STATE_SCAN_END:
		//initial
			cpy_ptr = NULL;
			cpy_size = 0;
			//sem_wait(&sem_rs232);	
		break;
		
		default:
		// display
			printf("default state, inv_cmd_state=%d\n", inv_cmd_state);
		// next
			inv_delta_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_STOP);
		}	// switch case
	//
	// Does task, then update state
	// 
		
		if (inv_cmd_state == INV_DELTA_STATE_INIT)	{
			//printf("RS232 Initial again !!\n");
			//sleep(1);
			rs485_timeout_cnt = 0;
			inv_delta_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NORMAL);
		}
		else if(inv_cmd_state == INV_DELTA_STATE_RDY)
			inv_delta_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_SCAN);
		else if (inv_cmd_state == INV_DELTA_STATE_TIMEOUT){
			inv_delta_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_STOP);
		}
		else if(inv_cmd_state == INV_DELTA_STATE_SCAN_END)
		{
			puts("Finish Scan process!!\n");
			
			// dump message
			
			inv_delta_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_STOP);
			printf("To the end of Inverter probe !!\n");

			// break the while
			break;
		}
		else if(inv_cmd_state == INV_DELTA_STATE_STOP)
		{
			printf("reach Delta SCAN stop state !!\n");
			close(fd_rs485);
			return 0;
			//inv_delta_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NORMAL);
		}
		else if(inv_cmd_state&0xf00)
		{
			// check if it reach the end
			if(inv_addr >= INV_MAX)
				inv_delta_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_END);
			
   			// this is a read command
	  		bzero(&raw_485_pkg_rd, sizeof(raw_485_pkg_rd));
			//raw_485_pkg_rd.pkt_type = 0x04;	
			req_size = 2;
			printf("Scan Delta inv# = %d on %s, fd=%d\n", inv_addr, scan_port_name, fd_rs485);
#ifdef USE_LCD
			char str_1[16], str_2[16];
    		sprintf(str_1, "ScanDT #%d/%d", inv_addr, INV_MAX);
    		string_USB_LCD("/dev/ttyACM0", str_1, 1);
    		sprintf(str_2, "Found DT #%d", inv_info->num);
    		string_USB_LCD("/dev/ttyACM0", str_2, 2);
#endif
			//inv_addr = (uint16_t)invAddr;
			if(create_delta_cmd(&raw_485_pkg_rd, FALSE, inv_addr, inv_reg2rd , req_size)<0)
				printf("Create Read Cmd to Rs485 failed!!\n");
			if(append_modbus_CRC16(&raw_485_pkg_rd)<0)
				printf("Adding CRC16 error !!\n");
			
			// Send this packet to inverter
			write(fd_rs485, &(raw_485_pkg_rd), 8);
	  	
			//process Rx
	  		bufptr = rcv_buff;
			nbytes=0;
			bzero(bufptr, 128);
			rs232_timeout=FALSE;

	  		rsp_size = req_size*2 + 5;
	  		
	  		net_timeout_cnt = 0;
	  		//printf("rsp_size should be %d\n", rsp_size);

	  		usleep(WAITING_TIME_AFTER_ISSUE_CMD);

	  		for(;;){ // do rs485 receiving during 3 second, or timeout
	  	  	// check if there's data in com port, or time out
				
				if(net_timeout_cnt==RETRY_TIMES_FOR_ONE_SCAN/*isready(fd_rs485)==0*/){
	  				//rs232_timeout=TRUE;
	  				puts("Crash");
	  				rs232_timeout=INV_MSG_CRASH;
				
	  				break;
	  			}
				
				// read it !!
	  	 		bytes = read(fd_rs485, (char*)bufptr, rsp_size);	// it will always receive

				nbytes +=bytes;
				bufptr += bytes;
				// check satisfy

				if(bytes <= 0){		// get no data at all before timeout
	  				rs232_timeout=INV_MSG_CRASH;
	  				//puts("Scan Crash");
	  				break;
	  			}else if(rsp_size == nbytes){ // get exact data before timeout
					rs232_timeout=FALSE;
					break;
				}else{		// get partial data, keep watching !!
					rs232_timeout=FALSE;
					//break;
				}
				
				usleep(WAITING_TIME_EACH_SCAN_INV); //
				net_timeout_cnt ++;
	  		}

			valid_cnt=0;

			if(rs232_timeout==FALSE){
				if(nbytes && (cpy_ptr!=NULL)){
					// find 0x0a, check tail 0x0d
					
					tmp_bufp = rcv_buff;
					
					if(check_modbus_CRC16(inv_delta_wr_cmd, (delta_format_rtn_t *)tmp_bufp)>0){
						//printf("receive RS485 packet with CRC correct !!\n");
						
						// copy the data
						if(!inv_delta_wr_cmd){ // if this is a read operation
							tmp_bufp += 2; // locate ptr to pointing "Byte Count"
						
							//printf("cpy_size = %d, *tmp_bufp = %d\n", cpy_size, *((uint8_t*)tmp_bufp));
							if(cpy_size == *((uint8_t*)tmp_bufp)){
								tmp_bufp += 1;	// locate to data
								
								// switch bytes
								if(!isASCII)		// if it is ASCII field, don't switch
									switch_bytes((uint8_t*)tmp_bufp, cpy_size>>1);
								
								memcpy(cpy_ptr, tmp_bufp, cpy_size);
								//printf("Get data 0x%.4x\n", cpy_ptr[0]);
	  		  				}else
	  		  					printf("copy size mismatch, copy abort, cpy_ptr == %d\n", sizeof(cpy_ptr));
	  		  			}
						// modify count
						nbytes = valid_cnt;
						valid_cnt=0;					
					
					}else{
						printf("receive RS485 packet with CRC error !!\n");	
						rs232_timeout=INV_MSG_CRC_ERR;
					}
				}
			}	
	  	
			//change state
			if(rs232_timeout==INV_MSG_CRASH){
				if(rs485_timeout_cnt< SCAN_REPEAT_TIMES_WHEN_FAILED){	// retry 
					inv_delta_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NOCHANGE);
					//puts("Try re-issue this cmd !!");
					rs485_timeout_cnt++;
				}
				else{
					inv_delta_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NORMAL);
					rs485_timeout_cnt = 0;
				}
			}else if(rs232_timeout==INV_MSG_CRC_ERR){
				// clear rx buffer
				bytes = 1;
				sleep(1);
				while(bytes > 0){
					bytes = read(fd_rs485, (char*)rcv_buff, 32);
					puts("clear RS232 Rx buffer !!");
				}				
				if(rs485_timeout_cnt<1){	// retry 
					inv_delta_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NOCHANGE);
					rs485_timeout_cnt++;
				}
				else{
					inv_delta_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NORMAL);
					rs485_timeout_cnt = 0;
				}				
			}
    		else if(rs232_timeout==FALSE){
    			inv_delta_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NORMAL);	
    			printf("Delta inverter address=%d found on %s\n", inv_addr, scan_port_name);
    			inv_info->num += 1;		// increment the number 
    			inv_info->invInfo[inv_info->num-1].invId = inv_addr;
			}else{
				printf("RS485 timeout !!\n");
				inv_delta_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_TIMEOUT);
			}
		}
		else
			inv_delta_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NORMAL);
	}

	puts("exit inverter_scan_delta !");
	close(fd_rs485);

	return 1;
}

int invInfoGet_Delta(uint16_t invAddr, inverter_msg2host_delta_t *data, char* scan_port_name){

  int fd_rs485;
  int bytes;
  bool_t inv_delta_wr_cmd, this_pkt_valid=TRUE;
  uint8_t rcv_buff[128];
  uint8_t *bufptr;
  int nbytes;
  delta_format04_t raw_485_pkg_rd;
  delta_format06_t raw_485_pkg_wr;
  uint16_t req_size;
  uint8_t rs232_timeout = FALSE;
  int valid_cnt=0;
  bool_t isASCII=FALSE;
  uint8_t * tmp_bufp;
  uint16_t inv_addr /*= 0x03*/, inv_reg2rd;
  inv_delta_state_t inv_cmd_state = INV_DELTA_STATE_INIT;
  uint8_t cpy_size;
  uint16_t wr_data;
  uint16_t * cpy_ptr=NULL;
  inverter_delta_raw_t inverter_delta_info_local;
  int device_type = 0;
  
  //int i;

  int net_timeout_cnt, rs485_timeout_cnt, size_to_read;

  bzero(&inverter_delta_info_local, sizeof(inverter_delta_raw_t));

	// Initial UART
  if(!scan_port_name)
  	return -1;  
  fd_rs485 = open_port_RS485(scan_port_name, O_RDWR| O_NOCTTY);
  
  if(fd_rs485 < 0){
  	printf("Open port %s failed !!", scan_port_name);
  	return -1;
  }   

  /***************************
  *  Get State
  ****************************/
	while(1){
		// get inv_reg2rd, cpy_ptr, cpy_size by current state
		if(getRegsMatrix_Delta(&inverter_delta_info_local, inv_cmd_state, &inv_reg2rd, &cpy_ptr, &cpy_size, &wr_data)<=0){
			printf("state no found in delta protocol, inv_cmd_state=%d, goes to STOP state \n", inv_cmd_state);
			inv_delta_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_STOP);
		}

		if((inv_reg2rd == INVERTER_DELTA_WR_REG) || (inv_reg2rd == INVERTER_DELTA_ERR_REG))
			inv_delta_wr_cmd = TRUE;
		else
			inv_delta_wr_cmd = FALSE;

		// print current state
		printf("inv_cmd_state=0x%x\n", inv_cmd_state);
		//printf("capturing reg addr=%d\n", inv_reg2rd+1);

		if (inv_cmd_state == INV_DELTA_STATE_INIT)	{
			rs485_timeout_cnt = 0;
			this_pkt_valid = TRUE;
#ifdef USE_LCD
			scan_inv_info_t tmp_info;
			char str_1[16], str_2[16];			
			sprintf(str_1, "GetDT ID=%d...", invAddr);
    		string_USB_LCD("/dev/ttyACM0", str_1, 1);
			getSubSetInvInfo(&sys_inv_info_all2, USE_DELTA, &tmp_info);
			sprintf(str_2, "DT/TOTAL %d/%d", tmp_info.num, sys_inv_info_all2.num);
    		string_USB_LCD("/dev/ttyACM0", str_2, 2);
#endif
			inv_delta_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NORMAL);
		}
		else if (inv_cmd_state == INV_DELTA_STATE_RDY)
			inv_delta_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NORMAL);
		else if (inv_cmd_state == INV_DELTA_STATE_TIMEOUT)
			inv_delta_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_STOP);
		else if((inv_cmd_state == INV_DELTA_STATE_DELTA_END)||(inv_cmd_state == INV_DELTA_STATE_DELTA_END_20K))
		{
#ifdef USE_LCD			
			sprintf(str_1, "GetDT ID=%d OK", invAddr);
    		string_USB_LCD("/dev/ttyACM0", str_1, 1);
#endif
			puts("Dump meta data!!\n");
			//inv_meta_data_dump(&inverter_info);
			
			if(this_pkt_valid){
				if(inverter_delta_info_local.use_protocol == PROTOCOL_TYPE_5K)
					update_inv_msg2host_delta(data, &inverter_delta_info_local,invAddr);
				else if(inverter_delta_info_local.use_protocol == PROTOCOL_TYPE_20K)
					update_inv_msg2host_delta20K(data, &inverter_delta_info_local,invAddr);
			}
			else{
				puts("This packet has invalid field, upload this packet with ID=0 !!");
				invAddr = 0;
				update_inv_msg2host_delta(data, &inverter_delta_info_local,invAddr);	
			}

			if(!rs232_run_flag){
				inv_delta_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_STOP);
				//printf("To the end of Inverter probe, waiting for re-start semaphore!!\n");
			}
			else{
				inv_delta_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_END);
				printf("To the end of Inverter probe, automatically restart later !!\n");
			}

			break;
		}
		else if(inv_cmd_state == INV_DELTA_STATE_STOP)
		{
			printf("reach Delta info_get stop state, update id = 0 to this slot !!\n");
			invAddr = 0;
			update_inv_msg2host_delta(data, &inverter_delta_info_local,invAddr);				

			break;
		}
		else if(inv_cmd_state == INV_DELTA_STATE_CHECK_TYPE){

			device_type = inverter_delta_info_local.Device_Type;
			//device_type = 80;
			
			printf("Test print: delta dev_type=%d00W\n", device_type);			
			
			if((device_type > 99)&&(device_type < 500)){	// 99=9.9KW, use 20K protocol for up to 10KW
				inverter_delta_info_local.use_protocol = PROTOCOL_TYPE_20K;
				inv_delta_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_20K_TYPE);
			}
			else{
				inverter_delta_info_local.use_protocol = PROTOCOL_TYPE_5K;
				inv_delta_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_5K_TYPE);
			}
		}
		else if(inv_cmd_state&0xf0)
		{
			// create pkg
			if(inv_delta_wr_cmd){
				bzero(&raw_485_pkg_wr, sizeof(raw_485_pkg_wr));
				//raw_485_pkg_rd.pkt_type = 0x06;	
				req_size = cpy_size >> 1;
				inv_addr = (uint16_t)invAddr;
				if(create_delta_cmd(&raw_485_pkg_wr, TRUE, inv_addr, inv_reg2rd , wr_data)<0)
					printf("Create Read Cmd to Rs485 failed!!\n");
				if(append_modbus_CRC16(&raw_485_pkg_wr)<0)
					printf("Adding CRC16 error !!\n");
				write(fd_rs485, &(raw_485_pkg_wr), 8);
	  		}else{
	  			bzero(&raw_485_pkg_rd, sizeof(raw_485_pkg_rd));
				//raw_485_pkg_rd.pkt_type = 0x04;	
				req_size = cpy_size >> 1;
				inv_addr = (uint16_t)invAddr;
				if(create_delta_cmd(&raw_485_pkg_rd, FALSE, inv_addr, inv_reg2rd , req_size)<0)
					printf("Create Read Cmd to Rs485 failed!!\n");
				if(append_modbus_CRC16(&raw_485_pkg_rd)<0)
					printf("Adding CRC16 error !!\n");
				write(fd_rs485, &(raw_485_pkg_rd), 8);
	  		}

	  		usleep(WAITING_TIME_AFTER_ISSUE_CMD);
	  	
			//process Rx
	  		bufptr = rcv_buff;
			nbytes=0;
			bzero(bufptr, 128);
			rs232_timeout=FALSE;
			if(inv_delta_wr_cmd)
	  			size_to_read = 8;
	  		else
	  			size_to_read = req_size*2 + 5;
	  		
	  		net_timeout_cnt = 0;
	  	//printf("rsp_size should be %d\n", rsp_size);
	  		for(;;){ // do rs485 receiving during 3 second, or timeout
	  	  	// check if there's data in com port, or time out
				
	  			if(net_timeout_cnt==RETRY_TIMES_FOR_ONE_CMD){
	  				//rs232_timeout=TRUE;
	  				puts("Timeout !!");
	  				rs232_timeout=INV_MSG_CRASH;
	  				break;
	  			}
	  			
	  			usleep(WAITING_TIME_EACH_RETRY);

				// read it !!
	  	  		bytes = read(fd_rs485, (char*)bufptr, size_to_read);	// it will always receive
				
				nbytes +=bytes;
				bufptr += bytes;
				size_to_read -= bytes;
				
				// check satisfy
				if(size_to_read <= 0){
					rs232_timeout=FALSE;
					break;
				}
				net_timeout_cnt ++;
	  		}

			valid_cnt=0;
	
			if(rs232_timeout==FALSE){
				if(nbytes && (cpy_ptr!=NULL)){
					// find 0x0a, check tail 0x0d
					
					tmp_bufp = rcv_buff;
					
					if(check_modbus_CRC16(inv_delta_wr_cmd, (delta_format_rtn_t *)tmp_bufp)>0){
						//printf("receive RS485 packet with CRC correct !!\n");
						
						// copy the data
						if(!inv_delta_wr_cmd){ // if this is a read operation
							tmp_bufp += 2; // locate ptr to pointing "Byte Count"
						
							//printf("cpy_size = %d, *tmp_bufp = %d\n", cpy_size, *((uint8_t*)tmp_bufp));
							if(cpy_size == *((uint8_t*)tmp_bufp)){
								tmp_bufp += 1;	// locate to data
								
								// switch bytes
								if(!isASCII)		// if it is ASCII field, don't switch
									switch_bytes((uint8_t*)tmp_bufp, cpy_size>>1);
								
								memcpy(cpy_ptr, tmp_bufp, cpy_size);
/*
								printf("get:");
								for(i=0;i<cpy_size;i++)
									printf("0x%02x ",tmp_bufp[i]);
								puts(" ");
*/
								//printf("cpy_ptr=%p\n", cpy_ptr);
								//printf("Get data 0x%.4x\n", cpy_ptr[0]);
	  		  				}else
	  		  					printf("copy size mismatch, copy abort, cpy_ptr == %d\n", sizeof(cpy_ptr));
	  		  			}
						// modify count
						nbytes = valid_cnt;
						valid_cnt=0;					
					
					}else{
						printf("receive RS485 packet with CRC error !!\n");	
						rs232_timeout=INV_MSG_CRC_ERR;
					}
				}
			}	
	  	
			//change state
			if(rs232_timeout==INV_MSG_CRASH){
				if(rs485_timeout_cnt < CMD_REPEAT_TIMES_WHEN_FAILED){
					inv_delta_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NOCHANGE);
					puts("Try re-issue this cmd !!");
					rs485_timeout_cnt ++;
				}
				else{  // abort read this reg after rs485_timeout_cnt times of retries
					//inv_delta_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NORMAL);	
					inv_delta_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_STOP);	
					puts("Abort reading because of no data response to this command !!");
					this_pkt_valid = FALSE;
					rs485_timeout_cnt = 0;
				}
			}else if(rs232_timeout==INV_MSG_CRC_ERR){
				// clear rx buffer
				bytes = 1;
				sleep(1);
				while(bytes > 0){
					bytes = read(fd_rs485, (char*)rcv_buff, 32);
					printf("clear RS232 Rx buffer datalen=%d !!\n", bytes);
				}				
				if(rs485_timeout_cnt < CMD_REPEAT_TIMES_WHEN_FAILED){
					inv_delta_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NOCHANGE);
					rs485_timeout_cnt ++;
				}
				else{  // abort read this reg after rs485_timeout_cnt times of retries
					//inv_delta_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NORMAL);	
					inv_delta_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_STOP);	
					puts("Abort reading because of CRC error !!");
					this_pkt_valid = FALSE;
					rs485_timeout_cnt = 0;
				}				
			}
    		else if(rs232_timeout==FALSE){
    			inv_delta_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NORMAL);	
    			rs485_timeout_cnt = 0;
			}else{
				printf("RS485 timeout !!\n");
				inv_delta_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_TIMEOUT);
				this_pkt_valid = FALSE;
			}
		}
		else
			inv_delta_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NORMAL);
	}

	puts("exit get info Delta !\n");
	close(fd_rs485);
	return 1;
}


/****************************
** EVERSOLAR Inverter protocol
****************************/
inv_eversolar_pure_rd_t eversolar_inverter_info;
//uint8_t current_unreg_sn[17];
//eversolar_list_t* eversolar_table_head = NULL;

// cmd_state, ctrl_code, func_code, offset, size
inv_eversolar_regsmatrix_t eversolar_reg_def[14] = {
	{INV_EVERSOLAR_STATE_INIT, 			0, 0, 	0, 	0},
	{INV_EVERSOLAR_STATE_RDY, 			EVERSOLAR_CTRLCODE_REG, 0x04, 	0, 	0},
	{INV_EVERSOLAR_STATE_TIMEOUT,		0x00, 0x0,	0,	0},
	{INV_EVERSOLAR_STATE_STOP, 			0x00, 0x0,	0,	0},
	{INV_EVERSOLAR_STATE_SCAN_RUN,		EVERSOLAR_CTRLCODE_REG, 0x00,	0,	18},
	{INV_EVERSOLAR_STATE_SCAN_ALLOC,	EVERSOLAR_CTRLCODE_REG, 0x01,	0,	1},
	{INV_EVERSOLAR_STATE_SCAN_END,		0x00, 0x0,	0,	0},
	{INV_EVERSOLAR_STATE_ADV_LIST,		0x00, 0x0,	0,	0},
	{INV_EVERSOLAR_STATE_CAPTURE, 		EVERSOLAR_CTRLCODE_RD, 0x02, 	(uint32_t)&(eversolar_inverter_info.TEMP) - (uint32_t)&eversolar_inverter_info, 		50},		// OLQ, off-line query, SN[0] stores the MSB
	{INV_EVERSOLAR_STATE_CAPTURE_20K, 	EVERSOLAR_CTRLCODE_RD, 0x02, 	(uint32_t)&(eversolar_inverter_info.TEMP_20K) - (uint32_t)&eversolar_inverter_info, 		48},		// OLQ, off-line query, SN[0] stores the MSB
	{INV_EVERSOLAR_STATE_QUERY_INFO, 	EVERSOLAR_CTRLCODE_RD, 0x03, 	(uint32_t)&(eversolar_inverter_info.MachineType) - (uint32_t)&eversolar_inverter_info, 		97},	// 0x61 	// orig 64		
	//{INV_EVERSOLAR_STATE_QUERY_INFO, 	EVERSOLAR_CTRLCODE_RD, 0x00, 	(uint32_t)&(eversolar_inverter_info.MachineType) - (uint32_t)&eversolar_inverter_info, 		30},	// 0x61 	// orig 64		
	{INV_EVERSOLAR_STATE_END, 			0x00, 0x0, 	0, 0},
	{INV_EVERSOLAR_STATE_END_20K, 		0x00, 0x0, 	0, 0},
	{INV_EVERSOLAR_STATE_CHECK_TYPE, 	0x00, 0x0, 	0, 0}
};

int convert_invmsg_to_asci_eversolar(char *buff, inverter_msg2host_eversolar_t* msg2host, int len){
	int n;
	char tmp_time[15]="";
	
	syscall_getRsp("date +%Y%m%d%H%M%S", tmp_time);
	tmp_time[14] = '\0';

	//double tmp_value;
	
	// avaliable size check
	if(len < 256)
		return -1;
	msg2host->serialNum[16] = '\0';
	msg2host->modelName[16] = '\0';
	n = sprintf(buff, "%04d,%s,%s,%s,%s,%d,%d,%.2f,%.2f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.2f,%d,%.1f",msg2host->inverterId, "EverSolar", msg2host->modelName, msg2host->serialNum, tmp_time, msg2host->MODE, msg2host->PAC,  
		msg2host->PDC, msg2host->E_TODAY, msg2host->E_TOTAL, msg2host->VPV, msg2host->VPV2, 
		msg2host->IPV, msg2host->IPV2, msg2host->VAC, msg2host->IAC, msg2host->FREQUENCY, msg2host->HOURS, msg2host->TEMP);

	buff[n] = '\0';
	
	return 1; 
}

int convert_invmsg_to_asci_zeversolar(char *buff, inverter_msg2host_eversolar_t* msg2host, int len){
	int n;
	char tmp_time[15]="";
	
	syscall_getRsp("date +%Y%m%d%H%M%S", tmp_time);
	tmp_time[14] = '\0';

	//double tmp_value;
	
	// avaliable size check
	if(len < 256)
		return -1;
	
	if(msg2host->use_protocol == PROTOCOL_TYPE_20K){
		msg2host->serialNum[16] = '\0';
		msg2host->modelName[16] = '\0';
		n = sprintf(buff, "%04d,%s,%s,%s,%s,%d,%d,%.0f,%.2f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.2f,%.1f,%.1f,%.1f,%.1f,%d,%d,%d,0x%08x,%d,%.1f",msg2host->inverterId, "ZeverSolar_20K", msg2host->modelName, msg2host->serialNum, tmp_time, msg2host->MODE, msg2host->PAC,  
			msg2host->PDC, msg2host->E_TODAY, msg2host->E_TOTAL, msg2host->VPV, msg2host->VPV2, 
			msg2host->IPV, msg2host->IPV2, msg2host->Vac_R_20K, msg2host->Iac_R_20K, msg2host->Fac_R_20K, msg2host->Vac_S_20K, msg2host->Iac_S_20K, msg2host->Vac_T_20K, msg2host->Iac_T_20K, 
			msg2host->WaitingTime_20K, msg2host->CosPhi_20K, msg2host->Phase_20K, msg2host->ErrWarMsg_20K, msg2host->HOURS_20K, msg2host->TEMP);
	}
	else{
		msg2host->serialNum[16] = '\0';
		msg2host->modelName[16] = '\0';
		n = sprintf(buff, "%04d,%s,%s,%s,%s,%d,%d,%.0f,%.2f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.2f,0x%08x,%d,%.1f",msg2host->inverterId, "ZeverSolar", msg2host->modelName, msg2host->serialNum, tmp_time, msg2host->MODE, msg2host->PAC,  
			msg2host->PDC, msg2host->E_TODAY, msg2host->E_TOTAL, msg2host->VPV, msg2host->VPV2, 
			msg2host->IPV, msg2host->IPV2, msg2host->VAC, msg2host->IAC, msg2host->FREQUENCY, msg2host->ErrWarMsg, msg2host->HOURS, msg2host->TEMP);
	}

	buff[n] = '\0';
	
	return 1; 
}
/*
int convert_invmsg_to_asci_zeversolar_20K(char *buff, inverter_msg2host_eversolar_t* msg2host, int len){
	int n;
	char tmp_time[15]="";
	
	syscall_getRsp("date +%Y%m%d%H%M%S", tmp_time);
	tmp_time[14] = '\0';

	//double tmp_value;
	
	// avaliable size check
	if(len < 256)
		return -1;
	msg2host->serialNum[16] = '\0';
	msg2host->modelName[16] = '\0';
	n = sprintf(buff, "%04d,%s,%s,%s,%s,%d,%d,%.2f,%.2f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.2f,%.1f,%.1f,%.1f,%.1f,%d,%d,%d,0x%08x,%d,%.1f",msg2host->inverterId, "ZeverSolar_20K", msg2host->modelName, msg2host->serialNum, tmp_time, msg2host->MODE, msg2host->PAC,  
		msg2host->PDC, msg2host->E_TODAY, msg2host->E_TOTAL, msg2host->VPV, msg2host->VPV2, 
		msg2host->IPV, msg2host->IPV2, msg2host->Vac_R_20K, msg2host->Iac_R_20K, msg2host->Fac_R_20K, msg2host->Vac_S_20K, msg2host->Iac_S_20K, msg2host->Vac_T_20K, msg2host->Iac_T_20K, 
		msg2host->WaitingTime_20K, msg2host->CosPhi_20K, msg2host->Phase_20K, msg2host->ErrWarMsg_20K, msg2host->HOURS_20K, msg2host->TEMP);

	buff[n] = '\0';
	
	return 1; 
}
*/
int update_inv_msg2host_eversolar(inverter_msg2host_eversolar_t *data, inv_eversolar_pure_rd_t *inverter_info, uint16_t invId){
	uint32_t p1, p2, tmp_32;
	uint16_t v1, v2, i1, i2;
	double p1_dub, p2_dub;
	double tmp_double;
	int size_struct = sizeof(inv_eversolar_pure_rd_t), size_struct_words;
	uint16_t inv_status = INV_STATUS_UNKNOWN;

	memcpy(data->serialNum, inverter_info->SerialNum, 16);
	memcpy(data->modelName, inverter_info->ModelName, 16);

	size_struct -= 64;		// minus the ID info content, total is 64, start from Machine Type...
	size_struct_words = size_struct >> 1;

	if(inverter_info->MachineType == ZEVERSOLAR_DEV_TYPE_NON_ISO_3P)
		data->use_protocol = PROTOCOL_TYPE_20K;
	else
		data->use_protocol = PROTOCOL_TYPE_5K;

	// switch byte first
	switch_bytes((uint8_t*)&(inverter_info->TEMP), size_struct_words);

	data->inverterId    = invId;
	//printf("inv update invid = %d\n", data->inverterId);

	v1 = inverter_info->VPV;
	v2 = inverter_info->VPV2;
	i1 = inverter_info->IPV;
	i2 = inverter_info->IPV2;
	p1 = v1 * i1;
	p2 = v2 * i2; 
	
	p1_dub = (double)p1;
	p1_dub = p1_dub/100;
	p2_dub = (double)p2;
	p2_dub = p2_dub/100;

	if(p1>5000000 || p2>5000000) 
		data->PDC = 0;
	else
		data->PDC = p1_dub + p2_dub;

	data->PAC  = (inverter_info->PAC);
	//printf("inv update pac = %d\n", data->PAC);	

	tmp_double = (double)(inverter_info->E_TODAY);
	tmp_double = tmp_double/100;
	data->E_TODAY = tmp_double;
	//printf("inv update e_today = %.2f\n", data->E_TODAY);

	tmp_double = (double)(inverter_info->E_TOTAL_H);
	tmp_double = tmp_double * 65536;
	tmp_double = tmp_double + (double)(inverter_info->E_TOTAL);
	tmp_double = tmp_double/10;
	data->E_TOTAL = tmp_double;
	//printf("inv update e_today = %.1f\n", data->E_TOTAL);

	tmp_double = (double)(inverter_info->VPV);
	tmp_double = tmp_double/10;
	data->VPV  = tmp_double;
	//printf("inv update vpv = %.1f\n", data->VPV);

	tmp_double = (double)(inverter_info->VPV2);
	tmp_double = tmp_double/10;
	data->VPV2 = tmp_double;
	//printf("inv update vpv2 = %.1f\n", data->VPV2);
	
	tmp_double = (double)(inverter_info->IPV);
	tmp_double = tmp_double/10;
	data->IPV  = tmp_double;
	//printf("inv update ipv = %.1f\n", data->IPV);
	
	tmp_double = (double)(inverter_info->IPV2);
	tmp_double = tmp_double/10;	
	data->IPV2 = tmp_double;
	//printf("inv update ipv2 = %.1f\n", data->IPV2);
	
	tmp_double = (double)(inverter_info->VAC);
	tmp_double = tmp_double/10;
	data->VAC  = tmp_double;
	//printf("inv update vac = %.1f\n", data->VAC);
	
	tmp_double = (double)(inverter_info->IAC);
	tmp_double = tmp_double/10;	
	data->IAC = tmp_double;
	//printf("inv update iac= %.1f\n", data->IAC);
	
	tmp_double = (double)(inverter_info->FREQUENCY);
	tmp_double = tmp_double/100;	
	data->FREQUENCY = tmp_double;
	//printf("inv update freq = %.2f\n", data->FREQUENCY);

	tmp_32 = (uint32_t)inverter_info->HOURS_UP_H;
	tmp_32 = tmp_32 << 16;
	tmp_32 = tmp_32 + inverter_info->HOURS_UP;
	data->HOURS = tmp_32;
	//printf("inv update hours = %d\n", data->HOURS);

	tmp_32 = (uint32_t)inverter_info->ErrWarMsg_H;
	tmp_32 = tmp_32 << 16;
	tmp_32 = tmp_32 + inverter_info->ErrWarMsg_L;
	data->ErrWarMsg = tmp_32;

	tmp_double = (double)(inverter_info->TEMP);
	tmp_double = tmp_double/10;	
	data->TEMP = tmp_double;
	//printf("inv update temp = %.1f\n", data->TEMP);
	
	inv_status = inverter_info->OP_MODE;
	if( inv_status == 0 )
		data->MODE  = INV_STATUS_WAIT;
	else if( inv_status == 1 )
		data->MODE  = INV_STATUS_NORMAL;
	else if( (inv_status == 2) || (inv_status == 3))
		data->MODE  = INV_STATUS_FAULT;
	else
		data->MODE  = INV_STATUS_UNKNOWN;
	//printf("inv update mode = %d\n", data->MODE);
	
	return 1;
}

int update_inv_msg2host_zeversolar_20K(inverter_msg2host_eversolar_t *data, inv_eversolar_pure_rd_t *inverter_info, uint16_t invId){
	uint32_t p1, p2, tmp_32;
	uint16_t v1, v2, i1, i2;
	double p1_dub, p2_dub;
	double tmp_double;
	uint16_t inv_status = INV_STATUS_UNKNOWN;
	int size_struct = sizeof(inv_eversolar_pure_rd_t), size_struct_words;

	memcpy(data->serialNum, inverter_info->SerialNum, 16);
	memcpy(data->modelName, inverter_info->ModelName, 16);

	size_struct -= 64;		// minus the ID info content, total is 64, start from Machine Type...
	size_struct_words = size_struct >> 1;

	if(inverter_info->MachineType == ZEVERSOLAR_DEV_TYPE_NON_ISO_3P)
		data->use_protocol = PROTOCOL_TYPE_20K;
	else
		data->use_protocol = PROTOCOL_TYPE_5K;

	// switch byte first
	switch_bytes((uint8_t*)&(inverter_info->TEMP), size_struct_words);

	data->inverterId    = invId;
	//printf("inv update invid = %d\n", data->inverterId);

	v1 = inverter_info->VPV1_20K;
	v2 = inverter_info->VPV2_20K;
	i1 = inverter_info->IPV1_20K;
	i2 = inverter_info->IPV2_20K;
	p1 = v1 * i1;
	p2 = v2 * i2; 
	
	p1_dub = (double)p1;
	p1_dub = p1_dub/100;
	p2_dub = (double)p2;
	p2_dub = p2_dub/100;

	if(p1>5000000 || p2>5000000) 
		data->PDC = 0;
	else
		data->PDC = p1_dub + p2_dub;

	data->PAC  = (inverter_info->PAC_20K);
	//printf("inv update pac = %d\n", data->PAC);	

	tmp_double = (double)(inverter_info->E_Today_20K);
	tmp_double = tmp_double/100;
	data->E_TODAY = tmp_double;
	//printf("inv update e_today = %.2f\n", data->E_TODAY);

	tmp_double = (double)(inverter_info->E_Total_H_20K);
	tmp_double = tmp_double * 65536;
	tmp_double = tmp_double + (double)(inverter_info->E_Total_L_20K);
	//tmp_double = (double)(inverter_info->E_TOTAL);
	tmp_double = tmp_double/10;
	data->E_TOTAL = tmp_double;
	//printf("inv update e_today = %.1f\n", data->E_TOTAL);

	tmp_double = (double)(inverter_info->VPV1_20K);
	tmp_double = tmp_double/10;
	data->VPV  = tmp_double;
	//printf("inv update vpv = %.1f\n", data->VPV);

	tmp_double = (double)(inverter_info->VPV2_20K);
	tmp_double = tmp_double/10;
	data->VPV2 = tmp_double;
	//printf("inv update vpv2 = %.1f\n", data->VPV2);
	
	tmp_double = (double)(inverter_info->IPV1_20K);
	tmp_double = tmp_double/10;
	data->IPV  = tmp_double;
	//printf("inv update ipv = %.1f\n", data->IPV);
	
	tmp_double = (double)(inverter_info->IPV2_20K);
	tmp_double = tmp_double/10;	
	data->IPV2 = tmp_double;
	//printf("inv update ipv2 = %.1f\n", data->IPV2);
	
	tmp_double = (double)(inverter_info->Fac_R_20K);
	tmp_double = tmp_double/100;
	data->Fac_R_20K  = tmp_double;
	
	tmp_double = (double)(inverter_info->Vac_R_20K);
	tmp_double = tmp_double/10;	
	data->Vac_R_20K = tmp_double;

	tmp_double = (double)(inverter_info->Iac_R_20K);
	tmp_double = tmp_double/10;
	data->Iac_R_20K  = tmp_double;
	
	tmp_double = (double)(inverter_info->Vac_S_20K);
	tmp_double = tmp_double/10;	
	data->Vac_S_20K = tmp_double;	

	tmp_double = (double)(inverter_info->Iac_S_20K);
	tmp_double = tmp_double/10;
	data->Iac_S_20K  = tmp_double;
	
	tmp_double = (double)(inverter_info->Vac_T_20K);
	tmp_double = tmp_double/10;	
	data->Vac_T_20K = tmp_double;	
	
	tmp_double = (double)(inverter_info->Iac_T_20K);
	tmp_double = tmp_double/10;	
	data->Iac_T_20K = tmp_double;
	//printf("inv update freq = %.2f\n", data->FREQUENCY);
	
	tmp_32 = (uint32_t)inverter_info->h_Total_H_20K;
	tmp_32 = tmp_32 << 16;
	tmp_32 = tmp_32 + inverter_info->h_Total_L_20K;
	data->HOURS_20K = tmp_32;
	//printf("H=0x%x, L=0x%x, HOURS_20K=0x%x\n", inverter_info->h_Total_H_20K, inverter_info->h_Total_L_20K,data->HOURS_20K);

	tmp_32 = (uint32_t)inverter_info->ErrWarMsg_H_20K;
	tmp_32 = tmp_32 << 16;
	tmp_32 = tmp_32 + inverter_info->ErrWarMsg_L_20K;
	data->ErrWarMsg_20K = tmp_32;
	//printf("inv update hours = %d\n", data->HOURS);

	tmp_double = (double)(inverter_info->TEMP_20K);
	tmp_double = tmp_double/10;	
	data->TEMP = tmp_double;
	//printf("inv update temp = %.1f\n", data->TEMP);

	inv_status = inverter_info->MODE_20K;
	if( inv_status == 0 )
		data->MODE  = INV_STATUS_WAIT;
	else if( inv_status == 1 )
		data->MODE  = INV_STATUS_NORMAL;
	else if( (inv_status == 2) || (inv_status == 3))
		data->MODE  = INV_STATUS_FAULT;
	else
		data->MODE  = INV_STATUS_UNKNOWN;
	
	//data->MODE  = inverter_info->MODE_20K;	     
	//printf("inv update mode = %d\n", data->MODE);

	data->WaitingTime_20K 	=	inverter_info->WaitingTime_20K; 
	data->CosPhi_20K		=	inverter_info->CosPhi_20K;	
	data->Phase_20K			=	inverter_info->Phase_20K;	
	
	return 1;
}


int append_eversolar_FCS(raw_485_t* FrameBuffer){
	int cal_size=0xff;
//	unsigned short crc_result=0xffff;
	int implementRdy = FALSE;
//	raw_485_t* tmpP = FrameBuffer;
	uint16_t result;

	// determine cal_size
	switch(FrameBuffer->pkt_type){
		case eversolar_cmd_reg:
			cal_size = 0x09;
			implementRdy = TRUE;

			result = append_FCS_add((uint8_t *)&(FrameBuffer->uniFrame.im_eversolar_format_reg.eversolar_offset_header_L), cal_size);
			FrameBuffer->uniFrame.im_eversolar_format_reg.eversolar_offset_FCS_L =((uint8_t*)&result)[1];
			FrameBuffer->uniFrame.im_eversolar_format_reg.eversolar_offset_FCS_H =((uint8_t*)&result)[0];

			break;		
		case eversolar_cmd_rd:
			cal_size = 0x09;
			implementRdy = TRUE;

			result = append_FCS_add((uint8_t *)&(FrameBuffer->uniFrame.im_eversolar_format_rd.eversolar_offset_header_L), cal_size);
			FrameBuffer->uniFrame.im_eversolar_format_rd.eversolar_offset_FCS_L =((uint8_t*)&result)[1];
			FrameBuffer->uniFrame.im_eversolar_format_rd.eversolar_offset_FCS_H =((uint8_t*)&result)[0];
			break;
		case eversolar_cmd_reg_alloc:
			cal_size = 26;	// 9 + 17
			implementRdy = TRUE;
			
			result = append_FCS_add((uint8_t *)&(FrameBuffer->uniFrame.im_eversolar_format_reg_alloc.eversolar_offset_header_L), cal_size);
			FrameBuffer->uniFrame.im_eversolar_format_reg_alloc.eversolar_offset_FCS_L =((uint8_t*)&result)[1];
			FrameBuffer->uniFrame.im_eversolar_format_reg_alloc.eversolar_offset_FCS_H =((uint8_t*)&result)[0];

			break;
		default:
			cal_size = 0xff;
			implementRdy = FALSE;
	}

	if(implementRdy){
//		result = append_FCS_add((uint8_t *)&(FrameBuffer->uniFrame.im_eversolar_format_reg.eversolar_offset_header_L), cal_size);
//		FrameBuffer->uniFrame.im_eversolar_format_reg.eversolar_offset_FCS_L =((uint8_t*)&result)[1];
//		FrameBuffer->uniFrame.im_eversolar_format_reg.eversolar_offset_FCS_H =((uint8_t*)&result)[0];
		return 1;
	}else
		return -1;

}

int check_eversolar_FCS(uint8_t * data, int len){
	uint16_t result = 0;
	int i;
	uint16_t in_result = 0;

	if(data==NULL)
		return -1;

	if(len > 140) // abortrary value
		return -1;

	len += 9;
	for(i=0; i<len; i++)
		result += data[i];
	
	in_result = data[i] << 8;
	in_result += data[i+1];

	if(result == in_result)
		return 1;
	else
		return -1;
}

int create_eversolar_cmd(void* FrameBuffer, uint8_t ctrl_code, uint8_t fun_code, uint8_t dev_addr, uint8_t *sn, uint8_t id){
	raw_485_t * inFrame = (raw_485_t*) FrameBuffer;
	struct eversolar_format_rd* eversolar_rdp;
	struct eversolar_format_reg* eversolar_regp;
	struct eversolar_format_reg_alloc* eversolar_reg_allocp;

	// check if pakcet type is correct
	if(!(inFrame->pkt_type & 0x10)){
		puts("pkt type is not eversolar while creating command");
		return -1;
	}

	if(ctrl_code == EVERSOLAR_CTRLCODE_REG){
		switch(fun_code){
		case 0x00:
			eversolar_regp = (struct eversolar_format_reg*)&(inFrame->uniFrame.im_eversolar_format_reg);

			eversolar_regp->eversolar_offset_header_L		= 0xaa;
			eversolar_regp->eversolar_offset_header_H		= 0x55;
			eversolar_regp->eversolar_offset_src_L			= 0x01;
			eversolar_regp->eversolar_offset_src_H			= 0x00;
			eversolar_regp->eversolar_offset_dst_L			= 0x00;
			eversolar_regp->eversolar_offset_dst_H			= 0x00;	// dest should be all 0
			eversolar_regp->eversolar_offset_ctrlCode		= EVERSOLAR_CTRLCODE_REG;
			eversolar_regp->eversolar_offset_funCode		= fun_code;
			eversolar_regp->eversolar_offset_len			= 0x00;
			eversolar_regp->eversolar_offset_FCS_L			= 0xff;
			eversolar_regp->eversolar_offset_FCS_H			= 0xff;
			break;	
		case 0x01:	
			eversolar_reg_allocp = (struct eversolar_format_reg_alloc*)&(inFrame->uniFrame.im_eversolar_format_reg_alloc);
			eversolar_reg_allocp->eversolar_offset_header_L		= 0xaa;
			eversolar_reg_allocp->eversolar_offset_header_H		= 0x55;
			eversolar_reg_allocp->eversolar_offset_src_L			= 0x01;
			eversolar_reg_allocp->eversolar_offset_src_H			= 0x00;
			eversolar_reg_allocp->eversolar_offset_dst_L			= 0x00;
			eversolar_reg_allocp->eversolar_offset_dst_H			= 0x00;
			eversolar_reg_allocp->eversolar_offset_ctrlCode		= EVERSOLAR_CTRLCODE_REG;
			eversolar_reg_allocp->eversolar_offset_funCode		= fun_code;
			eversolar_reg_allocp->eversolar_offset_len			= 0x11;	// 16+1
			memcpy(&(eversolar_reg_allocp->eversolar_offset_data00), sn, 16), 
			eversolar_reg_allocp->eversolar_offset_alloc			= id;
			eversolar_reg_allocp->eversolar_offset_FCS_L			= 0xff;
			eversolar_reg_allocp->eversolar_offset_FCS_H			= 0xff;
			break;
		case 0x04:
			eversolar_regp = (struct eversolar_format_reg*)&(inFrame->uniFrame.im_eversolar_format_reg);
			eversolar_regp->eversolar_offset_header_L		= 0xaa;
			eversolar_regp->eversolar_offset_header_H		= 0x55;
			eversolar_regp->eversolar_offset_src_L			= 0x01;
			eversolar_regp->eversolar_offset_src_H			= 0x00;
			eversolar_regp->eversolar_offset_dst_L			= 0x00;
			eversolar_regp->eversolar_offset_dst_H			= 0x00;	// dest should be all 0
			eversolar_regp->eversolar_offset_ctrlCode		= EVERSOLAR_CTRLCODE_REG;
			eversolar_regp->eversolar_offset_funCode		= fun_code;
			eversolar_regp->eversolar_offset_len			= 0x00;
			eversolar_regp->eversolar_offset_FCS_L			= 0xff;
			eversolar_regp->eversolar_offset_FCS_H			= 0xff;
			break;				
		default:
			printf("create eversolar CTRL cmd with fun_code=0x%x failed !!", fun_code);
			return -1;
		}
	}else if(ctrl_code == EVERSOLAR_CTRLCODE_RD){

		eversolar_rdp = &(inFrame->uniFrame.im_eversolar_format_rd);

		switch(fun_code){
		case 0x02:
			eversolar_rdp->eversolar_offset_header_L		= 0xaa;
			eversolar_rdp->eversolar_offset_header_H		= 0x55;
			eversolar_rdp->eversolar_offset_src_L			= 0x01;
			eversolar_rdp->eversolar_offset_src_H			= 0x00;
			eversolar_rdp->eversolar_offset_dst_L			= 0x00;
			eversolar_rdp->eversolar_offset_dst_H			= dev_addr;
			eversolar_rdp->eversolar_offset_ctrlCode		= EVERSOLAR_CTRLCODE_RD;
			eversolar_rdp->eversolar_offset_funCode			= fun_code;
			eversolar_rdp->eversolar_offset_len				= 0;
			eversolar_rdp->eversolar_offset_FCS_L			= 0xff;
			eversolar_rdp->eversolar_offset_FCS_H			= 0xff;
			break;
		case 0x03:
			eversolar_rdp->eversolar_offset_header_L		= 0xaa;
			eversolar_rdp->eversolar_offset_header_H		= 0x55;
			eversolar_rdp->eversolar_offset_src_L			= 0x01;
			eversolar_rdp->eversolar_offset_src_H			= 0x00;
			eversolar_rdp->eversolar_offset_dst_L			= 0x00;
			eversolar_rdp->eversolar_offset_dst_H			= dev_addr;
			eversolar_rdp->eversolar_offset_ctrlCode		= EVERSOLAR_CTRLCODE_RD;
			eversolar_rdp->eversolar_offset_funCode			= fun_code;
			eversolar_rdp->eversolar_offset_len				= 0;
			eversolar_rdp->eversolar_offset_FCS_L			= 0xff;
			eversolar_rdp->eversolar_offset_FCS_H			= 0xff;
			break;		
		case 0x00:
			eversolar_rdp->eversolar_offset_header_L		= 0xaa;
			eversolar_rdp->eversolar_offset_header_H		= 0x55;
			eversolar_rdp->eversolar_offset_src_L			= 0x01;
			eversolar_rdp->eversolar_offset_src_H			= 0x00;
			eversolar_rdp->eversolar_offset_dst_L			= 0x00;
			eversolar_rdp->eversolar_offset_dst_H			= dev_addr;
			eversolar_rdp->eversolar_offset_ctrlCode		= EVERSOLAR_CTRLCODE_RD;
			eversolar_rdp->eversolar_offset_funCode			= fun_code;
			eversolar_rdp->eversolar_offset_len				= 0;
			eversolar_rdp->eversolar_offset_FCS_L			= 0xff;
			eversolar_rdp->eversolar_offset_FCS_H			= 0xff;
			break;					
		default:
			printf("create eversolar RD cmd with fun_code=0x%x failed !!", fun_code);
		}
	}else{
		puts("create eversolar_cmd failed !!");
		return -1;
	}

	return 1;
}

int inv_eversolar_state_chg_next(inv_eversolar_state_t* curr_inv_cmd_state, inv_cmd_event_t in_event){		//inv_ali_cmd_state_t
	
	switch (*curr_inv_cmd_state) {
		case INV_EVERSOLAR_STATE_INIT:
			if(!in_event)	// event normal
				*curr_inv_cmd_state = INV_EVERSOLAR_STATE_RDY;
			else
				*curr_inv_cmd_state = INV_EVERSOLAR_STATE_STOP;
		break;	
		case INV_EVERSOLAR_STATE_RDY:
			if(in_event == INV_CMD_EVENT_SCAN)
				*curr_inv_cmd_state = INV_EVERSOLAR_STATE_SCAN_RUN;
			else if(in_event == INV_CMD_EVENT_NORMAL)
				*curr_inv_cmd_state = INV_EVERSOLAR_STATE_ADV_LIST;
			else if (in_event&(INV_CMD_EVENT_ERR|INV_CMD_EVENT_STOP))
				*curr_inv_cmd_state = INV_EVERSOLAR_STATE_STOP;
		break;
		case INV_EVERSOLAR_STATE_ERR:
			if(!in_event)
				*curr_inv_cmd_state = INV_EVERSOLAR_STATE_RDY;
			else
				*curr_inv_cmd_state = INV_EVERSOLAR_STATE_STOP;
		break;
		case INV_EVERSOLAR_STATE_STOP:
			if(!in_event)
				*curr_inv_cmd_state = INV_EVERSOLAR_STATE_RDY;
			else
				*curr_inv_cmd_state = INV_EVERSOLAR_STATE_STOP;
		break;
		case INV_EVERSOLAR_STATE_TIMEOUT:
			if(!in_event)
				*curr_inv_cmd_state = INV_EVERSOLAR_STATE_ERR;
			else
				*curr_inv_cmd_state = INV_EVERSOLAR_STATE_STOP;
		break;

		case INV_EVERSOLAR_STATE_ADV_LIST:
			if(in_event == INV_CMD_EVENT_NORMAL)	// event normal
				*curr_inv_cmd_state = INV_EVERSOLAR_STATE_QUERY_INFO;//INV_EVERSOLAR_STATE_CAPTURE;
			else 
				*curr_inv_cmd_state = INV_EVERSOLAR_STATE_STOP;
		break;
		case INV_EVERSOLAR_STATE_CHECK_TYPE:
			if(in_event == INV_CMD_EVENT_20K_TYPE)
				*curr_inv_cmd_state = INV_EVERSOLAR_STATE_CAPTURE_20K;
			else if(in_event == INV_CMD_EVENT_5K_TYPE)
				*curr_inv_cmd_state = INV_EVERSOLAR_STATE_CAPTURE;
			else if (in_event&(INV_CMD_EVENT_ERR|INV_CMD_EVENT_STOP))
				*curr_inv_cmd_state = INV_DELTA_STATE_STOP;			
		break;
		
		case INV_EVERSOLAR_STATE_CAPTURE:
		case INV_EVERSOLAR_STATE_CAPTURE_20K:
		case INV_EVERSOLAR_STATE_QUERY_INFO:
			if (in_event == INV_CMD_EVENT_NOCHANGE)
				*curr_inv_cmd_state = *curr_inv_cmd_state;
			else if(in_event&INV_CMD_EVENT_TIMEOUT)	// event normal
				*curr_inv_cmd_state = INV_EVERSOLAR_STATE_END;
			else if(in_event == INV_CMD_EVENT_NORMAL)	// event normal
				*curr_inv_cmd_state = *curr_inv_cmd_state + 1;
			else if (in_event&INV_CMD_EVENT_ERR)
				*curr_inv_cmd_state = INV_EVERSOLAR_STATE_ERR;
			else if (in_event&INV_CMD_EVENT_STOP)
				*curr_inv_cmd_state = INV_EVERSOLAR_STATE_STOP;
		break;	

		case INV_EVERSOLAR_STATE_SCAN_RUN:		// 257
			if(in_event == INV_CMD_EVENT_STOP)
				*curr_inv_cmd_state = INV_EVERSOLAR_STATE_STOP;
			else if (in_event == INV_CMD_EVENT_NORMAL)
				*curr_inv_cmd_state = INV_EVERSOLAR_STATE_SCAN_ALLOC;
			else if (in_event == INV_CMD_EVENT_TIMEOUT)
				*curr_inv_cmd_state = INV_EVERSOLAR_STATE_SCAN_END;
		break;
		
		case INV_EVERSOLAR_STATE_END:		// 259
		case INV_EVERSOLAR_STATE_END_20K:
			if(in_event == INV_CMD_EVENT_NORMAL)	// event normal
				*curr_inv_cmd_state = INV_EVERSOLAR_STATE_RDY;
			else if(in_event == INV_CMD_EVENT_END)
				*curr_inv_cmd_state = INV_EVERSOLAR_STATE_INIT;
			else
				*curr_inv_cmd_state = INV_EVERSOLAR_STATE_STOP;
		break;

		case INV_EVERSOLAR_STATE_SCAN_ALLOC:
			if(in_event == INV_CMD_EVENT_NORMAL)
				*curr_inv_cmd_state = INV_EVERSOLAR_STATE_SCAN_RUN;
			else if(in_event == INV_CMD_EVENT_END)
				*curr_inv_cmd_state = INV_EVERSOLAR_STATE_SCAN_END;
			else
				*curr_inv_cmd_state = INV_EVERSOLAR_STATE_STOP;
		break;

		case INV_EVERSOLAR_STATE_SCAN_END:
			*curr_inv_cmd_state = INV_EVERSOLAR_STATE_STOP;
		
		default:;
	}
	
	return 1;
}


int getRegsMatrix_Eversolar(inv_eversolar_pure_rd_t* ptr_base, inv_eversolar_state_t state, uint8_t* out_ctrl, uint8_t* out_func, uint8_t** out_ptr, uint8_t* out_size){
	uint32_t i;
	bool_t isFound=FALSE;

	if (ptr_base == NULL)
		return -1;

	for (i=0; i<(sizeof(eversolar_reg_def)/sizeof(inv_eversolar_regsmatrix_t)); i++){
		//printf("In Matrix, i=%d, state=%d \n", i, eversolar_reg_def[i].state_num);
		if(eversolar_reg_def[i].state_num == state){
			isFound = TRUE;
			*out_ctrl = eversolar_reg_def[i].ctrl_code;
			*out_func = eversolar_reg_def[i].func_code;
			*out_ptr = (uint8_t*)((uint32_t)ptr_base + (uint32_t)eversolar_reg_def[i].offset);
			*out_size = eversolar_reg_def[i].cpy_size;
			break;
		}
	}

	if (isFound)
		return 1;
	else{
		printf("State #%d no found in matrix\n", state);
		*out_ctrl = 0xff;
		*out_func = 0xff;
		*out_ptr = NULL;
		*out_size = 0;
		return -1;
	}
}

int eversolar_appendEntry(char* sn, uint8_t Id){
	FILE * pFile;
	char linebuf[128];
	uint8_t tmp_sn[19]="";
	eversolar_list_t *rsvd_list_head=NULL, *current;
	int tmp_id;
	bool_t valid=TRUE;

	if(sn==NULL || Id==0)
		return -1;

	// collect all the entries in the file into a linklist !!
	pFile = fopen(EVERSOLAR_PREDEFINED_ADDR_FILE, "a+");
	if (pFile==NULL) {
		puts("file eversolar_ID_table.txt no found");
		return -1;
	}

	// create reserved list from file
	while(fgets(linebuf, 128, pFile) != NULL){
		// omit line with leading "#"
		if(linebuf[0] == '#')
			continue;

		// deal with legal line
		sscanf(linebuf, "%s %d", tmp_sn, &tmp_id);
		
		if(list_add(&rsvd_list_head, (uint8_t)tmp_id, tmp_sn)<0)
			puts("add tmp link list failed");
	}

	current = rsvd_list_head;
	while(current!=NULL){
		if((current->ID == Id)||(strcmp(current->SN, sn)==0)){
			valid=FALSE;
			break;
		}
		
		current = current->next;
	}

	if(valid){
		memcpy(tmp_sn, sn, 18);
		sprintf(linebuf, "%s %.3d\n", tmp_sn, Id);
		fputs (linebuf,pFile);
	}else
		printf("file table has an entry having either SN or ID the same with the entry we're going to add !!");


	list_del_all(&rsvd_list_head);
	fclose(pFile);
	return 1;
}

uint8_t eversolar_getIdFromSn(eversolar_list_t *head, char* sn){
	FILE * pFile;
	char linebuf[128];  // each line occupies 16+3 = 20 bytes, maximum 50 * 20 = 1000
	uint8_t tmp_sn[18];
	int tmp_id, black_list_cnt;
	eversolar_list_t * current, *rsvd_list_head=NULL;
	bool_t pend_id_valid=FALSE, rsvd_id_valid=FALSE, id_valid=FALSE;

	if(strlen(sn) > 18)
		return -1;
	
	pFile = fopen(EVERSOLAR_PREDEFINED_ADDR_FILE, "r");
	if (pFile==NULL) {
		puts("file eversolar_ID_table.txt no found");
		return -1;
	}

	// if search in lookup table
	while(fgets(linebuf, 128, pFile) != NULL){
		// omit line with leading "#"
		if(linebuf[0] == '#')
			continue;

		// deal with legal line
		sscanf(linebuf, "%s %d", tmp_sn, &tmp_id);
		// string lookup table entires into a temp link list for future use
		if(list_add(&rsvd_list_head, (uint8_t)tmp_id, tmp_sn)<0)
			puts("add tmp link list failed");

		if(strcmp((char*)tmp_sn, sn)==0){
			puts("one line fit");
			if(tmp_id < 251){
				fclose(pFile);
				list_del_all(&rsvd_list_head);
				return (uint8_t)tmp_id;
			}
			else{
				puts("reserved id getting by the sn is illegal");
				fclose(pFile);
				list_del_all(&rsvd_list_head);
				return -1;
			}
			
		}
		printf("In table, %s has %d\n", tmp_sn, tmp_id);
	}
	// so far, input sn doesn't fit any SN in table, every entry were copied into rsvd_list_head

	black_list_cnt = 0;
	do{
		// else get random between allowable range
		tmp_id = EVERSOLAR_ADDR_START + black_list_cnt;
		printf("try id = %d\n", tmp_id);
		
		// check if there's already an item having this is id in existing link list
		current = head;
		if(current==NULL)
			pend_id_valid = TRUE;
		while(current != NULL){
			if(current->ID == (uint8_t)tmp_id){
				puts("pending list already has this ID");
				pend_id_valid = FALSE;
				break;
			}else
				pend_id_valid = TRUE;
	
			// advance to next item
			current = current->next;
		}
		
		// check if there's an entry having this id in reserved table
		current = rsvd_list_head;
		if(current==NULL)
			rsvd_id_valid = TRUE;
		while(current != NULL){
			if(current->ID == (uint8_t)tmp_id){
				puts("reserved list already has this ID");
				rsvd_id_valid = FALSE;
				break;
			}else
				rsvd_id_valid = TRUE;
	
			// advance to next item
			current = current->next;
		}

		black_list_cnt ++;

		id_valid = pend_id_valid && rsvd_id_valid;
		if(!id_valid)
			printf("id %d is occupied \n", tmp_id);

	} while ((id_valid == FALSE)&&(black_list_cnt <EVERSOLAR_ADDR_AMOUNT));

	// free tmp list
	if(rsvd_list_head != NULL)
		list_del_all(&rsvd_list_head);

	fclose(pFile);
	
	printf("black list count = %d\n", black_list_cnt);

	if((black_list_cnt <EVERSOLAR_ADDR_AMOUNT))
		return tmp_id;
	else{
		printf("Amount of ID %d exceed the allowable range %d!!\n", black_list_cnt, EVERSOLAR_ADDR_AMOUNT);
		return -1;
	}
}

int eversolar_releaseId(eversolar_list_t **head, uint8_t id){
	// del item in link list
	
	return list_del(head, id);
}

int eversolar_parse_rsp(void* pkt, uint8_t *out_buff, int valid_len){
	inv_eversolar_scan_rsp_t* normal_pktp;
	int len=0;
	uint8_t * data_start_p;

	// check valid
	normal_pktp = (inv_eversolar_scan_rsp_t*)pkt;
	if( !((normal_pktp->header_aa==0xaa)&&(normal_pktp->header_55==0x55)) ){
		puts("rsp pkt invalid !!");
		return 0;
	}
	
	len = normal_pktp->len;
	data_start_p = &(normal_pktp->data_0);

	if(len > (valid_len)){
		printf("packet length is grater than allowable length, rsp size=%d, allowable=%d\n", len, valid_len);
		puts("but we still proceed instead of return -1 !!");
		//return -1;
	}

	//if(check_eversolar_FCS(pkt, len)>0){
	//	memcpy(out_buff, data_start_p, len);
	if(check_eversolar_FCS(pkt, valid_len)>0){
		memcpy(out_buff, data_start_p, valid_len);
	
		//out_buff[len] = '\0';
		return 1;
	}
	else{
		puts("in eversolar receive pakcet with FCS error !!");
		return -1;
	}
}

#if 0
int invInfoGet_Eversolar(uint16_t invAddr, void *data, bool_t isScan, char* scan_port_name, eversolar_list_t** list_head){

  int fd_rs485;

  uint8_t rcv_buff[512];
  uint8_t *bufptr;

  raw_485_t raw_485_pkg;
  uint32_t req_size, nbytes, bytes;

  uint8_t rs232_timeout = FALSE;
  
  //bool_t tail_valid=FALSE;
  uint8_t * tmp_bufp; //, * data_basep;
  uint16_t inv_addr /*= 0x03, inv_reg2rd*/;
  uint8_t ctrl_code, func_code;
	
  inv_eversolar_state_t inv_cmd_state = INV_EVERSOLAR_STATE_INIT;
  uint8_t cpy_size;
  uint8_t * cpy_ptr=NULL;
  int retry_cnt = 3;
  uint8_t tmp_id = 255, tmp_cmp_id = 0;
  uint8_t current_unreg_sn[17] = "", dummy_sn[17]=""; 

  int net_timeout_cnt, rs485_timeout_cnt, size_to_read;
  int i;
  eversolar_list_t * current_rdp;
  bool_t prevStateIsRdy = TRUE, idFound=FALSE, this_pkt_valid=TRUE;
  int allocated_this_time = 0;
  inv_eversolar_pure_rd_t eversolar_inverter_info_local;
	// Initial UART

  bzero(&eversolar_inverter_info_local, sizeof(inv_eversolar_pure_rd_t));

  if(!scan_port_name)
  	return -1;
		 
  fd_rs485 = open_port_RS485(scan_port_name, O_RDWR| O_NOCTTY | O_NONBLOCK);
  
  if(fd_rs485 < 0){
  	printf("Open port %s failed !!", scan_port_name);
  	return -1;
  }

	//printf("Current BaudRate=%d\n",getbaud(fd_rs485) );
  /***************************
  *  Get State
  ****************************/
	while(1){
		// get inv_reg2rd, cpy_ptr, cpy_size by current state
		if(getRegsMatrix_Eversolar(&eversolar_inverter_info_local, inv_cmd_state, &ctrl_code, &func_code, &cpy_ptr, &cpy_size)<=0){
			printf("state no found in eversolar protocol, inv_cmd_state=%d, goes to STOP state \n", inv_cmd_state);
			inv_eversolar_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_STOP);
		}

		// print current state
		printf("inv_cmd_state=0x%x\n", inv_cmd_state);
		

  /***************************
  *  Process Special States
  ****************************/		
		if (inv_cmd_state == INV_EVERSOLAR_STATE_INIT)	{
			//sleep(1);  // 20130611
			rs485_timeout_cnt = 0;
			this_pkt_valid = TRUE;
			bzero(current_unreg_sn, 17);
			inv_eversolar_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NORMAL);
		}

		else if (inv_cmd_state == INV_EVERSOLAR_STATE_RDY){
			if(isScan){
				inv_eversolar_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_SCAN);
				retry_cnt = EVERSOLAR_SCAN_CRASH_CNT_BEFORE_LEAVE;
			}
			else{
				inv_eversolar_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NORMAL);
				//current_rdp = eversolar_table_head;
				current_rdp = *list_head;
				retry_cnt = CMD_REPEAT_TIMES_WHEN_FAILED;
				prevStateIsRdy = TRUE;

				//NULL check
				if(current_rdp == NULL){
					puts("Current list head is NULL");
					break;
				}
			}
		}

		else if (inv_cmd_state == INV_EVERSOLAR_STATE_TIMEOUT)
			inv_eversolar_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_STOP);
		
		// when state is END, loop reaches the end
		else if(inv_cmd_state == INV_EVERSOLAR_STATE_END)
		{
			inverter_msg2host_eversolar_t *tmp_eversolar_p;
			puts("Dump meta data!!");

			//inv_meta_data_dump(&inverter_info);

			if(this_pkt_valid){
				//if(update_inv_msg2host_eversolar((inverter_msg2host_eversolar_t*)data, (inv_eversolar_pure_rd_t*)tmp_capture,invAddr)<0)
				memcpy(eversolar_inverter_info_local.SerialNum, current_rdp->SN, 16);
				memcpy(eversolar_inverter_info_local.ModelName, "             N/A", 16);
				if(update_inv_msg2host_eversolar((inverter_msg2host_eversolar_t*)data, &eversolar_inverter_info_local, invAddr)<0)
					puts("update eversolar msg2host error !!");

				puts("print at update point");
				tmp_eversolar_p = data;
				printf("inverter=%d, pac=%d\n", tmp_eversolar_p->inverterId, tmp_eversolar_p->PAC);

			}
			else{
				puts("This packet has invalid field, upload this packet with ID=0 !!");
				invAddr=0;
				//update_inv_msg2host_eversolar((inverter_msg2host_eversolar_t*)data, (inv_eversolar_pure_rd_t*)tmp_capture,invAddr);
				update_inv_msg2host_eversolar((inverter_msg2host_eversolar_t*)data, &eversolar_inverter_info_local,invAddr);
			}

			inv_eversolar_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_STOP);
			printf("To the end of Eversolar capturing !!\n");
			
			// break the while loop
			break;
		}
		
		else if(inv_cmd_state == INV_EVERSOLAR_STATE_STOP)
		{
			printf("reach EverSolar stop state, update id = 0 to this slot !!\n");
			invAddr = 0;
			//update_inv_msg2host_eversolar((inverter_msg2host_eversolar_t*)data, (inv_eversolar_pure_rd_t*)tmp_capture,invAddr);			
			update_inv_msg2host_eversolar((inverter_msg2host_eversolar_t*)data, &eversolar_inverter_info_local,invAddr);	
			break;
		}

  /***************************
  *  Normal States -- Allocate
  ****************************/	
  		else if(inv_cmd_state == INV_EVERSOLAR_STATE_SCAN_ALLOC){
  			// alloc a value for this new inverter
  			//tmp_id = eversolar_getIdFromSn(eversolar_table_head, (char*)current_unreg_sn);
			tmp_id = eversolar_getIdFromSn(*list_head, (char*)current_unreg_sn);
  			
  			printf("In EverSolar get ID=%d by SN=%s \n", tmp_id, current_unreg_sn);

  			puts("Clear rx buffer first to remove multiple response upon previous scan command");
  			clear_serial_rx_buffer(fd_rs485);

  			if(tmp_id == 255){
  				puts("tmp_id error, end this scan !!");
  				inv_eversolar_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_END);
  				continue;
  			}

  			// set data for response
  			req_size = cpy_size;

  			// create reg packet
  			bzero(&raw_485_pkg, sizeof(raw_485_pkg));
			raw_485_pkg.pkt_type = eversolar_cmd_reg_alloc;
  			if(create_eversolar_cmd(&raw_485_pkg, ctrl_code, func_code, inv_addr, current_unreg_sn, tmp_id)<0)
				printf("Create Read Cmd to Rs485 failed!!\n");
			if(append_eversolar_FCS(&raw_485_pkg)<0)
				printf("Adding CRC16 error !!\n");

			write(fd_rs485, &(raw_485_pkg.uniFrame.im_eversolar_format_reg_alloc), sizeof(struct eversolar_format_reg_alloc));

			sleep(1);
			// no matter how, we assume this command is received by inverter,
			list_add(list_head, tmp_id, current_unreg_sn);
  			eversolar_appendEntry((char*)current_unreg_sn, tmp_id);
  			//printf("we get id = %d, add to list\n", tmp_id);

/*
  			// purposely we re-issue this same command again
  			write(fd_rs485, &(raw_485_pkg.uniFrame.im_eversolar_format_reg_alloc), sizeof(struct eversolar_format_reg_alloc));
  			sleep(1);
*/
			//process Rx
	  		bufptr = rcv_buff;
			nbytes=0;
			bzero(bufptr, 512);
			rs232_timeout=FALSE;
			// header(2) + source(2) + dest(2) + ctrl + fun + len + Data... + FCS(2)
	  		size_to_read = req_size + 11;
	  	
	  		net_timeout_cnt = 0;

	  		for(;;){ // do rs485 receiving during 3 second, or timeout
	  	  	// check if there's data in com port, or time out
				
				if(net_timeout_cnt==RETRY_TIMES_FOR_ONE_SCAN/*isready(fd_rs485)==0*/){
	  				//rs232_timeout=TRUE;
	  				puts("Timeout !!");
	  				rs232_timeout=INV_MSG_CRASH;
				
	  				break;
	  			}
	  			
	  			usleep(WAITING_TIME_EACH_RETRY); //0.4 s
				
				// read it !!
	  	 		bytes = read(fd_rs485, (char*)bufptr, size_to_read);	// it will always receive

				nbytes +=bytes;
				bufptr += bytes;
				size_to_read -= bytes;
				// check satisfy

				if(size_to_read <= 0){
					rs232_timeout=FALSE;
					break;
				}
				
				net_timeout_cnt ++;
	  		}

			if(rs232_timeout==FALSE){
				if(nbytes && (current_unreg_sn!=NULL)){

					tmp_bufp = rcv_buff;

					if( (eversolar_parse_rsp(tmp_bufp, dummy_sn, cpy_size))>0 ){
						printf("We get proper response pkt byte, append this \n");

					}
					else
						puts("receive illegal response pkt at eversolar inverter");

				}else
					puts("current_unreg_sn is NULL or nbyte is zero");
			}	

    		if(rs232_timeout==FALSE){
    			printf("Register SN=%s, ID=%d success !!\n", current_unreg_sn, tmp_id);
			}else{
				printf("Get no response from SN=%s, ID=%d registration !!, but we still assume it success!!\n", current_unreg_sn, tmp_id);
			}
			
			//sleep(1);
			puts("Clear rx buffer first to remove multiple response upon previous scan command");
  			clear_serial_rx_buffer(fd_rs485);
			

			allocated_this_time ++;
			printf("Total #%d of new eversolar inv found this scan !!", allocated_this_time);
			if(allocated_this_time >= EVERSOLAR_LIMIT_NEW_FOUND_INV_A_TIME){
				puts("Allocation of ID this scan reaches the limit, go to summary!");
				inv_eversolar_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_END);	
			}
			else
				inv_eversolar_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NORMAL);	
  		}
/*******************************
  *  Normal States -- SCAN_RUN
********************************/  		
  		else if (inv_cmd_state == INV_EVERSOLAR_STATE_SCAN_RUN){
  			inv_addr = 0;

  			bzero(&raw_485_pkg, sizeof(raw_485_pkg));
			raw_485_pkg.pkt_type = eversolar_cmd_reg;
			// req_size is counted in word size, one word contains two bytes
			req_size = cpy_size;
			inv_addr = (uint16_t)invAddr;
			
			// create data frame
			printf("Scan EverSolar #%d\n", rs485_timeout_cnt);
			if(create_eversolar_cmd(&raw_485_pkg, ctrl_code, func_code, inv_addr, NULL, 0)<0)
				printf("Create Read Cmd to Rs485 failed!!\n");
			if(append_eversolar_FCS(&raw_485_pkg)<0)
				printf("Adding CRC16 error !!\n");

	  		write(fd_rs485, &(raw_485_pkg.uniFrame.im_eversolar_format_reg), sizeof(struct eversolar_format_reg));

	  		char bufff[64];
	  		memcpy(bufff, &(raw_485_pkg.uniFrame.im_eversolar_format_reg), sizeof(struct eversolar_format_reg));

	  		for(i=0;i<sizeof(struct eversolar_format_reg);i++){
	  			printf("0x%02x ", bufff[i] );
	  		}


			//process Rx
	  		bufptr = rcv_buff;
			nbytes=0;
			bzero(bufptr, 512);
			rs232_timeout=FALSE;
			// header(2) + source(2) + dest(2) + ctrl + fun + len + Data... + FCS(2)
	  		size_to_read = req_size + 11;
	  	
	  		net_timeout_cnt = 0;

	  		for(;;){ // do rs485 receiving during 3 second, or timeout
	  	  	// check if there's data in com port, or time out
				
				if(net_timeout_cnt==RETRY_TIMES_FOR_ONE_SCAN/*isready(fd_rs485)==0*/){
	  				//rs232_timeout=TRUE;
	  				puts("No valid size of rsp pkt get while eversolar scan !!");
	  				rs232_timeout=INV_MSG_CRASH;
				
	  				break;
	  			}
	  		
	  			usleep(WAITING_TIME_EACH_SCAN_INV); //0.4 s

				// read it !!
	  	 		bytes = read(fd_rs485, (char*)bufptr, size_to_read);	// it will always receive

				nbytes +=bytes;
				bufptr += bytes;
				size_to_read -= bytes;
				// check satisfy

				if(size_to_read <= 0){
					rs232_timeout=FALSE;
					break;
				}
				
				net_timeout_cnt ++;
	  		}

			if(rs232_timeout==FALSE){
				if(nbytes && (current_unreg_sn!=NULL)){

					tmp_bufp = rcv_buff;

					if( (eversolar_parse_rsp(tmp_bufp, current_unreg_sn, cpy_size))>0 ){
						printf("We get proper response pkt byte, append this \n");
						
						// add list
  						/*
  						if(inv_cmd_state == INV_EVERSOLAR_STATE_SCAN_ALLOC)
  								list_add(&eversolar_table_head, tmp_id, current_unreg_sn);
  						printf("current list head = 0x%x\n", (uint32_t)eversolar_table_head);
  						*/
					}
					else{
						puts("receive illegal response pkt at eversolar inverter");
						rs232_timeout=INV_MSG_CRC_ERR;
					}

					printf("sn value is %s \n", current_unreg_sn);

				}else
					puts("current_unreg_sn is NULL or nbyte is zero");
			}	

			if(rs232_timeout==INV_MSG_CRASH){
				if(rs485_timeout_cnt < retry_cnt){
					inv_eversolar_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NOCHANGE);
					puts("Continue scanning eversolar!!");
					rs485_timeout_cnt ++;
				}
				else{  // abort read this reg after rs485_timeout_cnt times of retries
					inv_eversolar_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_TIMEOUT);	
					puts("Abort scanning eversolar !!");
					rs485_timeout_cnt = 0;
				}
			}else if(rs232_timeout==INV_MSG_CRC_ERR){
				// clear rx buffer
				bytes = 1;
				sleep(1);
				while(bytes > 0){
					bytes = read(fd_rs485, (char*)rcv_buff, 32);
					puts("clear RS232 Rx buffer !!");
				}				
				if(rs485_timeout_cnt < retry_cnt){
					inv_eversolar_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NOCHANGE);
					rs485_timeout_cnt ++;
				}
				else{  // abort read this reg after rs485_timeout_cnt times of retries
					inv_eversolar_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_TIMEOUT);	
					puts("Abort reading this item in RS485 !!");
					rs485_timeout_cnt = 0;
				}
			}
    		else if(rs232_timeout==FALSE){
    			//
    			//puts("Test only, we don't proceed to add the list !!");
    			puts("proceed to add the list !!");

    			inv_eversolar_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NORMAL);	
    			//
    			
    			rs485_timeout_cnt = 0;
			}else{
					printf("RS485 timeout !!\n");
					inv_eversolar_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_TIMEOUT);
			}

  		}
  		else if(inv_cmd_state == INV_EVERSOLAR_STATE_SCAN_END){
  			//current_rdp = eversolar_table_head;
  			current_rdp = *list_head;
  			scan_inv_info_t* rtn_inv_info;
  			i=0; // record the ID count
  			rtn_inv_info = (scan_inv_info_t*)data;
  			while(current_rdp != NULL){
  				
  				rtn_inv_info->invInfo[i].invId = current_rdp->ID;
  				
  				current_rdp = current_rdp->next;
  				i ++;
  			}
  			rtn_inv_info->num = i;
	  		break;
  		}
  		else if(inv_cmd_state == INV_EVERSOLAR_STATE_ADV_LIST){
  			if(prevStateIsRdy)
	  			current_rdp = current_rdp;
	  		else
	  			current_rdp = current_rdp->next;

	  		prevStateIsRdy = FALSE;
  			inv_eversolar_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NORMAL);
  		}
		//else if(inv_cmd_state == INV_EVERSOLAR_STATE_CAPTURE)
		else if((inv_cmd_state & 0xff0) == 0x010)
		{
			bzero(&raw_485_pkg, sizeof(raw_485_pkg));
			raw_485_pkg.pkt_type = eversolar_cmd_rd;
			// req_size is counted in word size, one word contains two bytes
			req_size = cpy_size;

			tmp_cmp_id = invAddr&0xff;

			// check if the required id is needed
			if(current_rdp != NULL){
				puts("current list_head is not empty");
				puts("ids are:");
				while(current_rdp != NULL){
					printf("ID=%d \n", current_rdp->ID);

					if(current_rdp->ID != tmp_cmp_id){
						current_rdp = current_rdp->next;
						idFound = FALSE;
						continue;
					}

					idFound = TRUE;
					inv_addr = invAddr;
					printf("id=%d found \n", inv_addr);
					break;
				}
			}else{
					puts("Reach the end of the list");
					break;
			}

			if(idFound == FALSE){
				printf("\n");
				break;
			}

			if(create_eversolar_cmd(&raw_485_pkg, ctrl_code, func_code, (uint8_t)inv_addr, NULL, 0)<0)
				printf("Create Read Cmd to Rs485 failed!!\n");
			if(append_eversolar_FCS(&raw_485_pkg)<0)
				printf("Adding CRC16 error !!\n");

	  		write(fd_rs485, &(raw_485_pkg.uniFrame.im_eversolar_format_rd), sizeof(struct eversolar_format_rd));

			//process Rx
	  		bufptr = rcv_buff;
			nbytes=0;
			bzero(bufptr, 512);
			rs232_timeout=FALSE;
			// header(2) + source(2) + dest(2) + ctrl + fun + len + Data... + FCS(2)
	  		size_to_read = req_size + 11;
	  	
	  		net_timeout_cnt = 0;

	  		for(;;){ // do rs485 receiving during 3 second, or timeout
	  	  	// check if there's data in com port, or time out
				
				if(net_timeout_cnt==RETRY_TIMES_FOR_ONE_CMD/*isready(fd_rs485)==0*/){
	  				//rs232_timeout=TRUE;
	  				puts("Crash");
	  				rs232_timeout=INV_MSG_CRASH;
				
	  				break;
	  			}
	  		
	  			usleep(WAITING_TIME_EACH_RETRY); //0.4 s

				// read it !!
	  	 		bytes = read(fd_rs485, (char*)bufptr, size_to_read);	// it will always receive

				nbytes +=bytes;
				bufptr += bytes;
				size_to_read -= bytes;
				// check satisfy

				if(size_to_read <= 0){
					rs232_timeout=FALSE;
					break;
				}
								
				net_timeout_cnt ++;
	  		}

			if(rs232_timeout==FALSE){
				if(nbytes && (cpy_ptr!=NULL)){

					tmp_bufp = rcv_buff;

					if( (eversolar_parse_rsp(tmp_bufp, cpy_ptr, cpy_size))>0 ){
						printf("We get proper response pkt byte, append this \n");

						// add list
						/*
  						if(inv_cmd_state == INV_EVERSOLAR_STATE_SCAN_ALLOC)
  								list_add(&eversolar_table_head, tmp_id, current_unreg_sn);
  						
  						//printf("current list head = 0x%x\n", (uint32_t)eversolar_table_head);
  						*/
					}
					else{
						puts("receive illegal response pkt at eversolar inverter");
						rs232_timeout=INV_MSG_CRC_ERR;
					}

				}else
					puts("cpy_ptr is NULL or nbyte is zero");
			}	

			if(rs232_timeout==INV_MSG_CRASH){
				if(rs485_timeout_cnt < retry_cnt){
					inv_eversolar_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NOCHANGE);
					puts("Try re-issue this cmd !!");
					rs485_timeout_cnt ++;
				}
				else{  // abort read this reg after rs485_timeout_cnt times of retries
					inv_eversolar_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_TIMEOUT);	
					puts("Abort reading this item in RS485 !!");
					rs485_timeout_cnt = 0;
					this_pkt_valid = FALSE;
					current_rdp->aging_cnt ++;
					printf("Eversolar lost this id=%d %d times\n", current_rdp->ID, current_rdp->aging_cnt);
				}
			}else if(rs232_timeout==INV_MSG_CRC_ERR){
				// clear rx buffer
				bytes = 1;
				usleep(300000);
				while(bytes > 0){
					bytes = read(fd_rs485, (char*)rcv_buff, 32);
					puts("clear RS232 Rx buffer !!");
				}				
				if(rs485_timeout_cnt < retry_cnt){
					inv_eversolar_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NOCHANGE);
					rs485_timeout_cnt ++;
				}
				else{  // abort read this reg after rs485_timeout_cnt times of retries
					inv_eversolar_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_TIMEOUT);	
					puts("Abort reading this item in RS485 !!");
					rs485_timeout_cnt = 0;
					this_pkt_valid = FALSE;
					current_rdp->aging_cnt ++;
					printf("Eversolar lost this id=%d %d times\n", current_rdp->ID, current_rdp->aging_cnt);
				}
			}
    		else if(rs232_timeout==FALSE){
    			inv_eversolar_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NORMAL);	
    			rs485_timeout_cnt = 0;
    			current_rdp->aging_cnt = 0; // reset aging count
			}else{
					printf("RS485 timeout !!\n");
					// if timeout, we clear the tmp_capture data for END to update all zero to host
					bzero(&eversolar_inverter_info_local, sizeof(inv_eversolar_pure_rd_t));
					inv_eversolar_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_TIMEOUT);
					this_pkt_valid = FALSE;
					current_rdp->aging_cnt ++;
					printf("Eversolar lost this id=%d %d times\n", current_rdp->ID, current_rdp->aging_cnt);
			}
			
			if(current_rdp->aging_cnt >= EVERSOLAR_RETRY_B4_DEL_LIST){
				printf("Eversolar remove id=%d from list_head !!\n", current_rdp->ID);
				list_del(list_head, current_rdp->ID);
			}

		}else
			inv_eversolar_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NORMAL);
	}

	puts("exit eversolar !");
	close(fd_rs485);
	
	return 1;
}
#endif

//
//  Zeversolar 
//
int invInfoGet_Zeversolar(uint16_t invAddr, void *data, bool_t isScan, char* scan_port_name, eversolar_list_t** list_head){

  int fd_rs485;

  uint8_t rcv_buff[512];
  uint8_t *bufptr;

  raw_485_t raw_485_pkg;
  uint32_t req_size, nbytes, bytes;

  uint8_t rs232_timeout = FALSE;
  
  //bool_t tail_valid=FALSE;
  uint8_t * tmp_bufp; //, * data_basep;
  uint16_t inv_addr /*= 0x03, inv_reg2rd*/;
  uint8_t ctrl_code, func_code;
	
  inv_eversolar_state_t inv_cmd_state = INV_EVERSOLAR_STATE_INIT;
  uint8_t cpy_size;
  uint8_t * cpy_ptr=NULL;
  int retry_cnt = 3;
  uint8_t tmp_id = 255, tmp_cmp_id = 0;
  uint8_t current_unreg_sn[19] = "", dummy_sn[17]=""; 

  int net_timeout_cnt, rs485_timeout_cnt, size_to_read;
  int i, tmp_rsp_len=0;
  eversolar_list_t * current_rdp;
  bool_t prevStateIsRdy = TRUE, idFound=FALSE, this_pkt_valid=TRUE;
  int allocated_this_time = 0;
  inv_eversolar_pure_rd_t eversolar_inverter_info_local;
  inv_eversolar_scan_rsp_t* tmp_pkt_p;
  bool_t isSet=FALSE;
	// Initial UART
  scan_inv_info_t* rtn_inv_info;
  i=0; // record the ID count
  rtn_inv_info = (scan_inv_info_t*)data;

  bzero(&eversolar_inverter_info_local, sizeof(inv_eversolar_pure_rd_t));

  if(!scan_port_name)
  	return -1;
		 
  fd_rs485 = open_port_RS485(scan_port_name, O_RDWR| O_NOCTTY | O_NONBLOCK);
  
  if(fd_rs485 < 0){
  	printf("Open port %s failed !!", scan_port_name);
  	return -1;
  }

	//printf("Current BaudRate=%d\n",getbaud(fd_rs485) );
  /***************************
  *  Get State
  ****************************/
	while(1){
		// get inv_reg2rd, cpy_ptr, cpy_size by current state
		if(getRegsMatrix_Eversolar(&eversolar_inverter_info_local, inv_cmd_state, &ctrl_code, &func_code, &cpy_ptr, &cpy_size)<=0){
			printf("state no found in eversolar protocol, inv_cmd_state=%d, goes to STOP state \n", inv_cmd_state);
			inv_eversolar_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_STOP);
		}

		// print current state
		printf("inv_cmd_state=0x%x\n", inv_cmd_state);
		

  /***************************
  *  Process Special States
  ****************************/		
		if (inv_cmd_state == INV_EVERSOLAR_STATE_INIT)	{
			//sleep(1);  // 20130611
			rs485_timeout_cnt = 0;
			this_pkt_valid = TRUE;
			bzero(current_unreg_sn, 19);
			rtn_inv_info->num = 0;

			inv_eversolar_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NORMAL);
		}

		else if (inv_cmd_state == INV_EVERSOLAR_STATE_RDY){
			if(isScan){
				// start issuing RESET CMD to get every inverter un-registered 
  				// create reg packet
  				bzero(&raw_485_pkg, sizeof(raw_485_pkg));
				raw_485_pkg.pkt_type = eversolar_cmd_reg;

  				if(create_eversolar_cmd(&raw_485_pkg, ctrl_code, func_code, inv_addr, current_unreg_sn, tmp_id)<0)
					printf("Create RE-register Cmd to Rs485 failed!!\n");
				if(append_eversolar_FCS(&raw_485_pkg)<0)
					printf("Adding CRC16 error !!\n");

				write(fd_rs485, &(raw_485_pkg.uniFrame.im_eversolar_format_reg), sizeof(struct eversolar_format_reg));
				usleep(50000);
				write(fd_rs485, &(raw_485_pkg.uniFrame.im_eversolar_format_reg), sizeof(struct eversolar_format_reg));
				usleep(50000);
				write(fd_rs485, &(raw_485_pkg.uniFrame.im_eversolar_format_reg), sizeof(struct eversolar_format_reg));
				usleep(WAITING_TIME_AFTER_ISSUE_CMD);
				//end of Reset cmd
/*
				printf("Test print RST cmd:");
				for(i=0;i<(int)sizeof(struct eversolar_format_reg);i++)
					printf("0x%2x ", ((uint8_t*)&(raw_485_pkg.uniFrame.im_eversolar_format_reg))[i]);
				puts(" ");
*/
				inv_eversolar_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_SCAN);
				retry_cnt = EVERSOLAR_SCAN_CRASH_CNT_BEFORE_LEAVE;
			}
			else{
				inv_eversolar_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NORMAL);
				//current_rdp = eversolar_table_head;
				current_rdp = *list_head;
				retry_cnt = CMD_REPEAT_TIMES_WHEN_FAILED;
				prevStateIsRdy = TRUE;

				//NULL check
				if(current_rdp == NULL){
					puts("Current list head is NULL");
					break;
				}
			}
		}

		else if (inv_cmd_state == INV_EVERSOLAR_STATE_TIMEOUT)
			inv_eversolar_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_STOP);
		
		// when state is END, loop reaches the end
		else if((inv_cmd_state == INV_EVERSOLAR_STATE_END)||(inv_cmd_state == INV_EVERSOLAR_STATE_END_20K))
		{
			//inverter_msg2host_eversolar_t *tmp_eversolar_p;
			puts("Dump meta data!!");

			//inv_meta_data_dump(&inverter_info);
 
			if(this_pkt_valid){
				//if(update_inv_msg2host_eversolar((inverter_msg2host_eversolar_t*)data, (inv_eversolar_pure_rd_t*)tmp_capture,invAddr)<0)
				memcpy(eversolar_inverter_info_local.SerialNum, current_rdp->SN, 16);
				//memcpy(eversolar_inverter_info_local.ModelName, "             N/A", 16);
				eversolar_inverter_info_local.VARange[5] = '\0';	// content of VARange is 6 bytes
				sprintf((char*)eversolar_inverter_info_local.ModelName, "%s0W", eversolar_inverter_info_local.VARange);
				//eversolar_inverter_info_local.Manufacture[15] = '\0';
				//printf("Manu=%s, model=%s\n", eversolar_inverter_info_local.Manufacture, eversolar_inverter_info_local.ModelName);
				//printf("Test print, zeversolar MachineType=0x%x, VARange=%.*s,Firm=%.*s,Mod=%.*s,Manu=%16s,Serial=%16s,vol=%4s\n", eversolar_inverter_info_local.MachineType, eversolar_inverter_info_local.VARange, eversolar_inverter_info_local.FirmwareVer, eversolar_inverter_info_local.ModelName ,eversolar_inverter_info_local.Manufacture ,eversolar_inverter_info_local.SerialNum ,eversolar_inverter_info_local.Nom);

				if(eversolar_inverter_info_local.MachineType==ZEVERSOLAR_DEV_TYPE_NON_ISO_3P){
					if(update_inv_msg2host_zeversolar_20K((inverter_msg2host_eversolar_t*)data, &eversolar_inverter_info_local, invAddr)<0)
						puts("update eversolar 20K msg2host error !!");
#ifdef USE_LCD
					char str_1[16];
  					char str_2[16];
					sprintf(str_1, "GetZ20 SN=%d OK", invAddr);
    				string_USB_LCD("/dev/ttyACM0", str_1, 1);
#endif
				}else{
					if(update_inv_msg2host_eversolar((inverter_msg2host_eversolar_t*)data, &eversolar_inverter_info_local, invAddr)<0)
						puts("update eversolar msg2host error !!");
#ifdef USE_LCD
					sprintf(str_1, "GetZ5 SN=%d OK", invAddr);
    				string_USB_LCD("/dev/ttyACM0", str_1, 1);					
#endif
				}

				//puts("print at update point");
				//tmp_eversolar_p = data;
				//printf("inverter=%d, pac=%d\n", tmp_eversolar_p->inverterId, tmp_eversolar_p->PAC);

			}
			else{
				puts("This packet has invalid field, upload this packet with ID=0 !!");
				invAddr=0;
				//update_inv_msg2host_eversolar((inverter_msg2host_eversolar_t*)data, (inv_eversolar_pure_rd_t*)tmp_capture,invAddr);
				update_inv_msg2host_eversolar((inverter_msg2host_eversolar_t*)data, &eversolar_inverter_info_local,invAddr);
			}

			inv_eversolar_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_STOP);
			printf("To the end of Eversolar capturing !!\n");
			
			// break the while loop
			break;
		}
		
		else if(inv_cmd_state == INV_EVERSOLAR_STATE_STOP)
		{
			printf("reach EverSolar stop state, update id = 0 to this slot !!\n");
			invAddr = 0;
			//update_inv_msg2host_eversolar((inverter_msg2host_eversolar_t*)data, (inv_eversolar_pure_rd_t*)tmp_capture,invAddr);			
			update_inv_msg2host_eversolar((inverter_msg2host_eversolar_t*)data, &eversolar_inverter_info_local,invAddr);	
			break;
		}

  /***************************
  *  Normal States -- Allocate
  ****************************/	
  		else if(inv_cmd_state == INV_EVERSOLAR_STATE_SCAN_ALLOC){
  			// alloc a value for this new inverter
			tmp_id = eversolar_getIdFromSn(*list_head, (char*)current_unreg_sn);
  			
  			printf("In EverSolar get ID=%d by SN=%s \n", tmp_id, current_unreg_sn);

  			puts("Clear rx buffer first to remove multiple response upon previous scan command");
  			clear_serial_rx_buffer(fd_rs485);

  			if(tmp_id == 255){
  				puts("tmp_id error, end this scan !!");
  				inv_eversolar_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_END);
  				continue;
  			}

  			// set data for response
  			req_size = cpy_size;

  			// create reg packet
  			bzero(&raw_485_pkg, sizeof(raw_485_pkg));
			raw_485_pkg.pkt_type = eversolar_cmd_reg_alloc;
  			if(create_eversolar_cmd(&raw_485_pkg, ctrl_code, func_code, inv_addr, current_unreg_sn, tmp_id)<0)
				printf("Create Read Cmd to Rs485 failed!!\n");
			if(append_eversolar_FCS(&raw_485_pkg)<0)
				printf("Adding CRC16 error !!\n");

			write(fd_rs485, &(raw_485_pkg.uniFrame.im_eversolar_format_reg_alloc), sizeof(struct eversolar_format_reg_alloc));

			sleep(1);
			// no matter how, we assume this command is received by inverter,
			list_add(list_head, tmp_id, current_unreg_sn);
  			eversolar_appendEntry((char*)current_unreg_sn, tmp_id);
  			//printf("we get id = %d, add to list\n", tmp_id);

/*
  			// purposely we re-issue this same command again
  			write(fd_rs485, &(raw_485_pkg.uniFrame.im_eversolar_format_reg_alloc), sizeof(struct eversolar_format_reg_alloc));
  			sleep(1);
*/
			//process Rx
	  		bufptr = rcv_buff;
			nbytes=0;
			bzero(bufptr, 512);
			rs232_timeout=FALSE;
			// header(2) + source(2) + dest(2) + ctrl + fun + len + Data... + FCS(2)
	  		size_to_read = req_size + 11;
	  	
	  		net_timeout_cnt = 0;

	  		for(;;){ // do rs485 receiving during 3 second, or timeout
	  	  	// check if there's data in com port, or time out
				
				if(net_timeout_cnt==RETRY_TIMES_FOR_ONE_SCAN/*isready(fd_rs485)==0*/){
	  				//rs232_timeout=TRUE;
	  				puts("Timeout !!");
	  				rs232_timeout=INV_MSG_CRASH;
				
	  				break;
	  			}
	  			
	  			usleep(WAITING_TIME_EACH_RETRY); //0.4 s
				
				// read it !!
	  	 		bytes = read(fd_rs485, (char*)bufptr, size_to_read);	// it will always receive

				nbytes +=bytes;
				bufptr += bytes;
				size_to_read -= bytes;
				// check satisfy

				if(size_to_read <= 0){
					rs232_timeout=FALSE;
					break;
				}
				
				net_timeout_cnt ++;
	  		}

			if(rs232_timeout==FALSE){
				if(nbytes && (current_unreg_sn!=NULL)){

					tmp_bufp = rcv_buff;

					if( (eversolar_parse_rsp(tmp_bufp, dummy_sn, cpy_size))>0 ){
						printf("We get proper response pkt byte, append this \n");

					}
					else
						puts("receive illegal response pkt at eversolar inverter");

				}else
					puts("current_unreg_sn is NULL or nbyte is zero");
			}	

    		if(rs232_timeout==FALSE){
    			printf("Register SN=%s, ID=%d success !!\n", current_unreg_sn, tmp_id);
			}else{
				printf("Get no response from SN=%s, ID=%d registration !!, but we still assume it success!!\n", current_unreg_sn, tmp_id);
			}
			
			//sleep(1);
			puts("Clear rx buffer first to remove multiple response upon previous scan command");
  			clear_serial_rx_buffer(fd_rs485);
			

			allocated_this_time ++;
			printf("Total #%d of new eversolar inv found this scan !!", allocated_this_time);
			if(allocated_this_time >= EVERSOLAR_LIMIT_NEW_FOUND_INV_A_TIME){
				puts("Allocation of ID this scan reaches the limit, go to summary!");
				inv_eversolar_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_END);	
			}
			else
				inv_eversolar_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NORMAL);	
  		}
/*******************************
  *  Normal States -- SCAN_RUN
********************************/  		
  		else if (inv_cmd_state == INV_EVERSOLAR_STATE_SCAN_RUN){
  			inv_addr = 0;

  			bzero(&raw_485_pkg, sizeof(raw_485_pkg));
			raw_485_pkg.pkt_type = eversolar_cmd_reg;
			// req_size is counted in word size, one word contains two bytes
			req_size = cpy_size;
			inv_addr = (uint16_t)invAddr;
			
			// create data frame
			printf("Scan EverSolar #%d @%s\n", rs485_timeout_cnt, scan_port_name);
#ifdef USE_LCD
			sprintf(str_1, "ScanZV #%02d/15", rs485_timeout_cnt);
    		string_USB_LCD("/dev/ttyACM0", str_1, 1);
			sprintf(str_2, "Found ZV #%d", rtn_inv_info->num);
    		string_USB_LCD("/dev/ttyACM0", str_2, 2);
#endif

			if(create_eversolar_cmd(&raw_485_pkg, ctrl_code, func_code, inv_addr, NULL, 0)<0)
				printf("Create Read Cmd to Rs485 failed!!\n");
			if(append_eversolar_FCS(&raw_485_pkg)<0)
				printf("Adding CRC16 error !!\n");

	  		write(fd_rs485, &(raw_485_pkg.uniFrame.im_eversolar_format_reg), sizeof(struct eversolar_format_reg));

/*
	  		char bufff[64];
	  		memcpy(bufff, &(raw_485_pkg.uniFrame.im_eversolar_format_reg), sizeof(struct eversolar_format_reg));

	  		for(i=0;i<(int)sizeof(struct eversolar_format_reg);i++){
	  			printf("0x%02x ", bufff[i] );
	  		}
*/
	  		usleep(WAITING_TIME_AFTER_ISSUE_CMD);

			//process Rx
	  		bufptr = rcv_buff;
			nbytes=0;
			bzero(bufptr, 512);
			rs232_timeout=FALSE;
			// header(2) + source(2) + dest(2) + ctrl + fun + len + Data... + FCS(2)
	  		size_to_read = req_size + 11;
	  	
	  		net_timeout_cnt = 0;

	  		for(;;){ // do rs485 receiving during 3 second, or timeout
	  	  	// check if there's data in com port, or time out
				
				if(net_timeout_cnt==RETRY_TIMES_FOR_ONE_SCAN/*isready(fd_rs485)==0*/){
	  				//rs232_timeout=TRUE;
	  				//puts("No valid size of rsp pkt get while eversolar scan !!");
	  				rs232_timeout=INV_MSG_CRASH;
				
	  				break;
	  			}
	  		
	  			usleep(WAITING_TIME_EACH_SCAN_INV); //0.4 s

				// read it !!
	  	 		bytes = read(fd_rs485, (char*)bufptr, size_to_read);	// it will always receive

				nbytes +=bytes;
				bufptr += bytes;
				size_to_read -= bytes;
				// check satisfy

				if(size_to_read <= 0){
					rs232_timeout=FALSE;
					break;
				}
				
				net_timeout_cnt ++;
	  		}

			if(rs232_timeout==FALSE){
				if(nbytes && (current_unreg_sn!=NULL)){

					tmp_bufp = rcv_buff;
					/*
					puts("SCAN get :");
					for(i=0;i<(int)nbytes;i++)
						printf("0x%2x ", tmp_bufp[i]);

					puts(" ");
					*/

					if( (eversolar_parse_rsp(tmp_bufp, current_unreg_sn, cpy_size))>0 ){
						printf("We get proper response pkt byte, append this \n");
						
						// add list
  						/*
  						if(inv_cmd_state == INV_EVERSOLAR_STATE_SCAN_ALLOC)
  								list_add(&eversolar_table_head, tmp_id, current_unreg_sn);
  						printf("current list head = 0x%x\n", (uint32_t)eversolar_table_head);
  						*/
					}
					else{
						puts("receive illegal response pkt at eversolar inverter");
						rs232_timeout=INV_MSG_CRC_ERR;
					}

					printf("sn value is %s \n", current_unreg_sn);

				}else
					puts("current_unreg_sn is NULL or nbyte is zero");
			}	

			if(rs232_timeout==INV_MSG_CRASH){
				if(rs485_timeout_cnt < retry_cnt){
					inv_eversolar_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NOCHANGE);
					puts("Continue scanning eversolar!!");
					rs485_timeout_cnt ++;
				}
				else{  // abort read this reg after rs485_timeout_cnt times of retries
					inv_eversolar_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_TIMEOUT);	
					puts("Abort scanning eversolar !!");
					rs485_timeout_cnt = 0;
				}
			}else if(rs232_timeout==INV_MSG_CRC_ERR){
				// clear rx buffer
				bytes = 1;
				sleep(1);
				while(bytes > 0){
					bytes = read(fd_rs485, (char*)rcv_buff, 32);
					puts("clear RS232 Rx buffer !!");
				}				
				if(rs485_timeout_cnt < retry_cnt){
					inv_eversolar_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NOCHANGE);
					rs485_timeout_cnt ++;
				}
				else{  // abort read this reg after rs485_timeout_cnt times of retries
					inv_eversolar_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_TIMEOUT);	
					puts("Abort reading this item in RS485 !!");
					rs485_timeout_cnt = 0;
				}
			}
    		else if(rs232_timeout==FALSE){
    			//
    			//puts("Test only, we don't proceed to add the list !!");
    			puts("proceed to add the list !!");

    			inv_eversolar_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NORMAL);	
    			//
    			
    			rs485_timeout_cnt = 0;
			}else{
					printf("RS485 timeout !!\n");
					inv_eversolar_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_TIMEOUT);
			}

  		}
  		else if(inv_cmd_state == INV_EVERSOLAR_STATE_SCAN_END){
  			//current_rdp = eversolar_table_head;
  			current_rdp = *list_head;
  			//scan_inv_info_t* rtn_inv_info;
  			//i=0; // record the ID count
  			//rtn_inv_info = (scan_inv_info_t*)data;
  			while(current_rdp != NULL){
  				
  				rtn_inv_info->invInfo[i].invId = current_rdp->ID;
  				
  				current_rdp = current_rdp->next;
  				i ++;
  			}
  			rtn_inv_info->num = i;
	  		break;
  		}
  		else if(inv_cmd_state == INV_EVERSOLAR_STATE_ADV_LIST){
  			if(prevStateIsRdy)
	  			current_rdp = current_rdp;
	  		else
	  			current_rdp = current_rdp->next;

	  		prevStateIsRdy = FALSE;
  			inv_eversolar_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NORMAL);
  		}
  		else if(inv_cmd_state == INV_EVERSOLAR_STATE_CHECK_TYPE){		//previous state should be INV_EVERSOLAR_STATE_QUERY_INFO
  			printf("MachineType=0x%x\n", eversolar_inverter_info_local.MachineType);
	  		if(eversolar_inverter_info_local.MachineType==ZEVERSOLAR_DEV_TYPE_NON_ISO_3P){
	  			inv_eversolar_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_20K_TYPE);
	  			puts("Use 20K Zeversolar protocol");
#ifdef USE_LCD	  			
	  			sprintf(str_1, "GetZ20 SN=%d...", invAddr);
    			string_USB_LCD("/dev/ttyACM0", str_1, 1);
#endif
	  		}
			else{
				inv_eversolar_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_5K_TYPE);
				puts("Use 5K Zeversolar protocol");
#ifdef USE_LCD
	  			sprintf(str_1, "GetZ5 SN=%d...", invAddr);
    			string_USB_LCD("/dev/ttyACM0", str_1, 1);				
#endif
			}
  		}
		//else if(inv_cmd_state == INV_EVERSOLAR_STATE_CAPTURE)
		else if((inv_cmd_state & 0xff0) == 0x010)
		{
			bzero(&raw_485_pkg, sizeof(raw_485_pkg));
			raw_485_pkg.pkt_type = eversolar_cmd_rd;
			// req_size is counted in word size, one word contains two bytes
			req_size = cpy_size;

			tmp_cmp_id = invAddr&0xff;

			// check if the required id is needed
			if(current_rdp != NULL){
				puts("current list_head is not empty");
				puts("ids are:");
				while(current_rdp != NULL){
					printf("ID=%d \n", current_rdp->ID);

					if(current_rdp->ID != tmp_cmp_id){
						current_rdp = current_rdp->next;
						idFound = FALSE;
						continue;
					}

					idFound = TRUE;
					inv_addr = invAddr;
					printf("id=%d found \n", inv_addr);
					break;
				}
			}else{
					puts("Reach the end of the list");
					break;
			}

			if(idFound == FALSE){
				printf("\n");
				break;
			}

			if(create_eversolar_cmd(&raw_485_pkg, ctrl_code, func_code, (uint8_t)inv_addr, NULL, 0)<0)
				printf("Create Read Cmd to Rs485 failed!!\n");
			if(append_eversolar_FCS(&raw_485_pkg)<0)
				printf("Adding CRC16 error !!\n");

	  		write(fd_rs485, &(raw_485_pkg.uniFrame.im_eversolar_format_rd), sizeof(struct eversolar_format_rd));

			//process Rx
	  		bufptr = rcv_buff;
			nbytes=0;
			bzero(bufptr, 512);
			rs232_timeout=FALSE;
			// header(2) + source(2) + dest(2) + ctrl + fun + len + Data... + FCS(2)
	  		size_to_read = req_size + 11;
	  	
	  		net_timeout_cnt = 0;

	  		usleep(WAITING_TIME_AFTER_ISSUE_CMD);

	  		if(inv_cmd_state == INV_EVERSOLAR_STATE_QUERY_INFO){
				for(;;){ // do rs485 receiving during 3 second, or timeout
	  	  		// check if there's data in com port, or time out
					if(net_timeout_cnt==RETRY_TIMES_FOR_ONE_CMD/*isready(fd_rs485)==0*/){
	  					//rs232_timeout=TRUE;
	  					puts("Crash");
	  					rs232_timeout=INV_MSG_CRASH;
	  					break;
	  				}
	  				usleep(WAITING_TIME_EACH_RETRY); //0.4 s

					// read it !!
	  	 			bytes = read(fd_rs485, (char*)bufptr, size_to_read);	// it will always receive

					nbytes +=bytes;
					bufptr += bytes;
					size_to_read -= bytes;
					// check satisfy

					// if we receive the field of LEN
					if((nbytes >= 9)&& (!isSet)){
						tmp_pkt_p = (inv_eversolar_scan_rsp_t*)rcv_buff;
						tmp_rsp_len = tmp_pkt_p->len;
						cpy_size = tmp_rsp_len;
						tmp_rsp_len +=11;	// total size to read
						size_to_read = tmp_rsp_len - nbytes;
						isSet = TRUE;
						//printf("just test, in query_info, pktlen=%d, rcv bytes=%d , pt=%p so far\n", tmp_rsp_len, nbytes, tmp_pkt_p);
					}

					if(size_to_read <= 0){
						rs232_timeout=FALSE;
						break;
					}
			
					net_timeout_cnt ++;
	  			}
	  		}
	  		else {
	  			for(;;){ // do rs485 receiving during 3 second, or timeout
	  	  		// check if there's data in com port, or time out
					if(net_timeout_cnt==RETRY_TIMES_FOR_ONE_CMD/*isready(fd_rs485)==0*/){
	  					//rs232_timeout=TRUE;
	  					puts("Crash");
	  					rs232_timeout=INV_MSG_CRASH;
	  					break;
	  				}
	  				usleep(WAITING_TIME_EACH_RETRY); //0.4 s

					// read it !!
	  	 			bytes = read(fd_rs485, (char*)bufptr, size_to_read);	// it will always receive

					nbytes +=bytes;
					bufptr += bytes;
					size_to_read -= bytes;
					// check satisfy

					if(size_to_read <= 0){
						rs232_timeout=FALSE;
						break;
					}
			
					net_timeout_cnt ++;
	  			}
	  		}

			if(rs232_timeout==FALSE){
				if(nbytes && (cpy_ptr!=NULL)){

					tmp_bufp = rcv_buff;
					
					/*
					printf("we get : ");
					for(i=0;i<cpy_size+11;i++)
						printf("0x%02x, ",tmp_bufp[i]);
					*/
					
					//printf("just test, in sequence, cpy_size=%d, rcv bytes=%d ptr=%p so far\n", cpy_size, nbytes, tmp_bufp);
					if( (eversolar_parse_rsp(tmp_bufp, cpy_ptr, cpy_size))>0 ){
						printf("We get proper response pkt byte, append this \n");

						// add list
						/*
  						if(inv_cmd_state == INV_EVERSOLAR_STATE_SCAN_ALLOC)
  								list_add(&eversolar_table_head, tmp_id, current_unreg_sn);
  						
  						//printf("current list head = 0x%x\n", (uint32_t)eversolar_table_head);
  						*/
					}
					else{
						puts("receive illegal response pkt at eversolar inverter");
						rs232_timeout=INV_MSG_CRC_ERR;
					}

				}else
					puts("cpy_ptr is NULL or nbyte is zero");
			}	

			if(rs232_timeout==INV_MSG_CRASH){
				if(rs485_timeout_cnt < retry_cnt){
					inv_eversolar_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NOCHANGE);
					puts("Try re-issue this cmd !!");
					rs485_timeout_cnt ++;
				}
				else{  // abort read this reg after rs485_timeout_cnt times of retries
					inv_eversolar_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_TIMEOUT);	
					puts("Abort reading this item in RS485 !!");
					rs485_timeout_cnt = 0;
					this_pkt_valid = FALSE;
					current_rdp->aging_cnt ++;
					printf("Eversolar lost this id=%d %d times\n", current_rdp->ID, current_rdp->aging_cnt);
				}
			}else if(rs232_timeout==INV_MSG_CRC_ERR){
				// clear rx buffer
				bytes = 1;
				usleep(300000);
				while(bytes > 0){
					bytes = read(fd_rs485, (char*)rcv_buff, 32);
					puts("clear RS232 Rx buffer !!");
				}				
				if(rs485_timeout_cnt < retry_cnt){
					inv_eversolar_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NOCHANGE);
					rs485_timeout_cnt ++;
				}
				else{  // abort read this reg after rs485_timeout_cnt times of retries
					inv_eversolar_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_TIMEOUT);	
					puts("Abort reading this item in RS485 !!");
					rs485_timeout_cnt = 0;
					this_pkt_valid = FALSE;
					current_rdp->aging_cnt ++;
					printf("Eversolar lost this id=%d %d times\n", current_rdp->ID, current_rdp->aging_cnt);
				}
			}
    		else if(rs232_timeout==FALSE){
    			inv_eversolar_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NORMAL);	
    			rs485_timeout_cnt = 0;
    			current_rdp->aging_cnt = 0; // reset aging count
			}else{
					printf("RS485 timeout !!\n");
					// if timeout, we clear the tmp_capture data for END to update all zero to host
					bzero(&eversolar_inverter_info_local, sizeof(inv_eversolar_pure_rd_t));
					inv_eversolar_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_TIMEOUT);
					this_pkt_valid = FALSE;
					current_rdp->aging_cnt ++;
					printf("Eversolar lost this id=%d %d times\n", current_rdp->ID, current_rdp->aging_cnt);
			}
			
			if(current_rdp->aging_cnt >= EVERSOLAR_RETRY_B4_DEL_LIST){
				printf("Eversolar remove id=%d from list_head !!\n", current_rdp->ID);
				list_del(list_head, current_rdp->ID);
			}

		}else{
			puts("No any state matched, omit this state !!");
			inv_eversolar_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NORMAL);
		}
	}

	puts("exit eversolar !");
	close(fd_rs485);
	
	return 1;
}


/****************************
** ALI Inverter protocol
****************************/
inverter_ali_raw_t ali_inverter_info;

inv_ali_regsmatrix_t ali_reg_def[8] = {
	{INV_ALI_STATE_INIT, 	0x0, 	0, 	0},
	{INV_ALI_STATE_RDY, 	0x0, 	0, 	0},
	{INV_ALI_STATE_TIMEOUT,	0x0,	0,	0},
	{INV_ALI_STATE_STOP, 	0x0,	0,	0},
	{INV_ALI_STATE_C1, 		0xb4, 	(uint32_t)&(ali_inverter_info.State_mode) - (uint32_t)&ali_inverter_info, 		15*2},
	{INV_ALI_STATE_C2, 		0xc3, 	(uint32_t)&(ali_inverter_info.Bus_V) - (uint32_t)&ali_inverter_info, 			12*2},
	{INV_ALI_STATE_C3,		0xd3, 	(uint32_t)&(ali_inverter_info.CO2_REDUCTION_H) - (uint32_t)&ali_inverter_info, 	12*2},
	{INV_ALI_STATE_END, 	0x0, 	0, 0}
};

int update_inv_msg2host_ali(inverter_ali_raw_t *data, inverter_ali_raw_t *inverter_info, uint16_t invId){
	
	memcpy(data, inverter_info, sizeof(inverter_ali_raw_t));
	
	data->inverterId = invId;

	return 1;
}

int convert_invmsg_to_asci_ali(char *buff, inverter_ali_raw_t* msg2host){
	int n;
	double tmp_gen, tmp_gen_h, tmp_gen_l;
	int sn = msg2host->SN_HIGH;
	int pdc = msg2host->Ppv_A;
	char tmp_time[15]="";
	uint16_t inv_status = INV_STATUS_UNKNOWN;

	inv_status = msg2host->State_mode;
	if( (inv_status == 60) || (inv_status == 12) )
		inv_status  = INV_STATUS_FAULT;
	else if( inv_status < 43 )
		inv_status  = INV_STATUS_WAIT;
	else if( inv_status == 50 )
		inv_status  = INV_STATUS_NORMAL;
	else
		inv_status  = INV_STATUS_UNKNOWN;
	
	syscall_getRsp("date +%Y%m%d%H%M%S", tmp_time);
	tmp_time[14] = '\0';

	sn = sn << 16;
	sn = sn + msg2host->SN_LOW;

	tmp_gen_h = (uint32_t)msg2host->Eac_H;
	tmp_gen_h = tmp_gen_h * 1000;
	tmp_gen_l = (uint32_t)msg2host->Eac_L;
	tmp_gen_l = tmp_gen_l /10;

	tmp_gen = tmp_gen_h + tmp_gen_l;
	
	pdc = pdc + msg2host->Ppv_B;

	n = sprintf(buff, "%04d,", msg2host->inverterId); 
	buff += n;
	n = sprintf(buff, "%s,", "TOUGH"); 
	buff += n;	
	n = sprintf(buff, "%d,", msg2host->MODEL_NAME); 
	buff += n;
	n = sprintf(buff, "%d,", sn); 
	buff += n;
	n = sprintf(buff, "%s,", tmp_time); 
	buff += n;	
	n = sprintf(buff, "%d,", inv_status); 
	buff += n;
	n = sprintf(buff, "%d:", msg2host->Ton_total_Hr); 
	buff += n;
	n = sprintf(buff, "%d:", msg2host->Ton_total_Min); 
	buff += n;
	n = sprintf(buff, "%d,", msg2host->Ton_total_Sec); 
	buff += n;
	n = sprintf(buff, "%.2f,", tmp_gen); 
	buff += n;	
	n = sprintf(buff, "%d,", msg2host->Pac_L1); 
	buff += n;
	n = sprintf(buff, "%d,", pdc); 
	buff += n;
	n = sprintf(buff, "%d", msg2host->Ton_today); 
	buff += n;

	buff[0] = '\0';
	
	return 1; 
	
}

int inv_ali_state_chg_next(inv_ali_state_t* curr_inv_cmd_state, inv_cmd_event_t in_event){		//inv_ali_cmd_state_t
	
	switch (*curr_inv_cmd_state) {
		case INV_ALI_STATE_INIT:
			if(!in_event)	// event normal
				*curr_inv_cmd_state = INV_ALI_STATE_RDY;
			else
				*curr_inv_cmd_state = INV_ALI_STATE_STOP;
		break;	
		case INV_ALI_STATE_RDY:
			if(in_event == INV_CMD_EVENT_SCAN)
				*curr_inv_cmd_state = INV_ALI_STATE_SCAN_RUN;
			else if(in_event == INV_CMD_EVENT_NORMAL)
				*curr_inv_cmd_state = INV_ALI_STATE_C1;
			else if (in_event&(INV_CMD_EVENT_ERR|INV_CMD_EVENT_STOP))
				*curr_inv_cmd_state = INV_ALI_STATE_STOP;
		break;
		case INV_ALI_STATE_ERR:
			if(!in_event)
				*curr_inv_cmd_state = INV_ALI_STATE_RDY;
			else
				*curr_inv_cmd_state = INV_ALI_STATE_STOP;
		break;
		case INV_ALI_STATE_STOP:
			if(!in_event)
				*curr_inv_cmd_state = INV_ALI_STATE_RDY;
			else
				*curr_inv_cmd_state = INV_ALI_STATE_STOP;
		break;
		case INV_ALI_STATE_TIMEOUT:
			if(!in_event)
				*curr_inv_cmd_state = INV_ALI_STATE_ERR;
			else
				*curr_inv_cmd_state = INV_ALI_STATE_STOP;
		break;
		
		case INV_ALI_STATE_C1:
		case INV_ALI_STATE_C2:
		case INV_ALI_STATE_C3:
			if (in_event == INV_CMD_EVENT_NOCHANGE)
				*curr_inv_cmd_state = *curr_inv_cmd_state;
			else if(in_event == INV_CMD_EVENT_NORMAL)	// event normal
				*curr_inv_cmd_state = *curr_inv_cmd_state + 1;
			else if (in_event&INV_CMD_EVENT_TIMEOUT)
				*curr_inv_cmd_state = INV_ALI_STATE_TIMEOUT;
			else if (in_event&INV_CMD_EVENT_ERR)
				*curr_inv_cmd_state = INV_ALI_STATE_ERR;
			else if (in_event&INV_CMD_EVENT_STOP)
				*curr_inv_cmd_state = INV_ALI_STATE_STOP;
		break;
		
		case INV_ALI_STATE_SCAN_RUN:
			if(in_event == INV_CMD_EVENT_STOP)
				*curr_inv_cmd_state = INV_ALI_STATE_STOP;
			else if (in_event == INV_CMD_EVENT_END)
				*curr_inv_cmd_state = INV_ALI_STATE_SCAN_END;
			else 
				*curr_inv_cmd_state = *curr_inv_cmd_state;
		break;
		
		case INV_ALI_STATE_END:
			if(in_event == INV_CMD_EVENT_NORMAL)	// event normal
				*curr_inv_cmd_state = INV_ALI_STATE_RDY;
			else if(in_event == INV_CMD_EVENT_END)
				*curr_inv_cmd_state = INV_ALI_STATE_INIT;
			else
				*curr_inv_cmd_state = INV_ALI_STATE_STOP;
		break;
		
		case INV_ALI_STATE_SCAN_END:
			if(in_event == INV_CMD_EVENT_STOP)
				*curr_inv_cmd_state = INV_ALI_STATE_STOP;
		
		default:;
	}
	
	return 1;
}


int getRegsMatrix_Ali(inverter_ali_raw_t* ptr_base, inv_ali_state_t state, uint16_t* out_addr, uint16_t** out_ptr, uint8_t* out_size){
	uint32_t i;
	bool_t isFound=FALSE;

	for (i=0; i<(sizeof(ali_reg_def)/sizeof(inv_ali_regsmatrix_t)); i++){
		if(ali_reg_def[i].state_num == state){
			isFound = TRUE;
			*out_addr = ali_reg_def[i].reg_addr;
			*out_ptr = (uint16_t*)((uint32_t)ptr_base + (uint32_t)ali_reg_def[i].offset);
			*out_size = ali_reg_def[i].cpy_size;
			break;
		}
	}

	if (isFound)
		return 1;
	else{
		printf("State #%d no found in matrix\n", state);
		*out_addr = 0x0;
		*out_ptr = NULL;
		*out_size = 0;
		return -1;
	}
}

int inverter_scan_ali(scan_inv_info_t* inv_info, char* scan_port_name){

  	int fd_rs485;
  //int fd_gpio88;

  	uint8_t rcv_buff[128];
  	uint8_t *bufptr;

  	int i;
	raw_485_t raw_485_pkg;
	uint32_t req_size, rsp_size, nbytes, bytes;

	const uint8_t invBnTough[6] = "TOUGH";
	bool_t invIsTOUGH;
  	uint8_t rs232_timeout = FALSE;
  	uint8_t rcv_len;
  
  	uint32_t tmp_i, valid_cnt=0;
 	bool_t tail_valid=FALSE;
  	uint8_t * tmp_bufp;
	uint16_t inv_addr, inv_reg2rd;
	
	inv_cmd_state_t inv_cmd_state = INV_CMD_STATE_INIT;
	uint8_t cpy_size;
	
	//printf("In handle rs232 thread !!\n");

	//
	// Initial UART
	if(!scan_port_name)
		return -1;
  	fd_rs485 = open_port_RS485(scan_port_name, O_RDWR| O_NOCTTY);

  	if(fd_rs485 < 0){
  		printf("Open port %s failed !!", scan_port_name);
  		return -1;
  	}
   
	//printf("Current BaudRate=%d\n",getbaud(fd_rs485) );
	
	while(1){

		switch (inv_cmd_state){

		case INV_CMD_STATE_INIT:
			inv_reg2rd = 0;
			inv_info->num = 0;
			inv_addr = INV_START_ADDR -1;
		break;

		case INV_CMD_STATE_RDY:
			cpy_size = 0;
			inv_reg2rd = 0;
		break;
		
		case INV_CMD_STATE_TIMEOUT:
			cpy_size = 0;
			inv_reg2rd = 0;
		break;
		
		case INV_CMD_STATE_STOP:
			cpy_size = 0;
			inv_reg2rd = 0;
		break;
		
		case INV_CMD_STATE_SCAN_RUN:
			inv_reg2rd = 0x67;
			inv_addr = inv_addr + 1;
			//cpy_ptr = tmp_char;
			cpy_size = 16;
			invIsTOUGH = FALSE;
		break;

		case INV_CMD_STATE_SCAN_END:
		//initial
			//cpy_ptr = NULL;
			cpy_size = 0;
			//sem_wait(&sem_rs232);	
		break;
		
		default:
		// display
			printf("default state, inv_cmd_state=%d\n", inv_cmd_state);
		// next
			inv_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_STOP);
		}	// switch case
	//
	// Does task, then update state
	// 
		
		if (inv_cmd_state == INV_CMD_STATE_INIT)	{
			//printf("RS232 Initial again !!\n");
			//sleep(1);
			inv_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NORMAL);
		}
		else if(inv_cmd_state == INV_CMD_STATE_RDY)
			inv_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_SCAN);
		else if (inv_cmd_state == INV_CMD_STATE_TIMEOUT){
				inv_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_STOP);
		}
		else if(inv_cmd_state == INV_CMD_STATE_SCAN_END)
		{
			puts("Finish Scan process!!\n");
			
			// dump message
			
			inv_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_STOP);
			printf("To the end of Inverter probe !!\n");
			
			// break the while
			break;
		}
		else if(inv_cmd_state == INV_CMD_STATE_STOP)
		{
			printf("reach Ali SCAN stop state !!\n");
			close(fd_rs485);
			return 0;
			//inv_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NORMAL);
		}
		else if(inv_cmd_state&0xf00)
		{
			// check if it reach the end
			if(inv_addr > INV_MAX)
				inv_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_END);
			
			bzero(&raw_485_pkg, sizeof(raw_485_pkg));
			raw_485_pkg.pkt_type = mt81xx_cmd_read;	
			req_size = 8;
			if(create_mt81xx_cmd(&raw_485_pkg, inv_addr, inv_reg2rd , req_size)<0)
				printf("Create Read Cmd to Rs485 failed!!\n");
			if(append_motech_CRC16(&raw_485_pkg)<0)
				printf("Adding CRC16 error !!\n");
    	
    		printf("Scan TOUGH inv# = %d on %s\n", inv_addr, scan_port_name);
    	
			i=0;

			while(((uint8_t*)&(raw_485_pkg.uniFrame.im_mt81xx_format03))[i] != 0x0d){
				//printf("%.2x ",((uint8_t*)&(raw_485_pkg.uniFrame.im_mt81xx_format03))[i]);
				i++;
			}
			
			//select Tx
			//write(fd_gpio88, "1", 1);
			write(fd_rs485, &(raw_485_pkg.uniFrame.im_mt81xx_format03), i+1);
	  	
	  		usleep(WAITING_TIME_AFTER_ISSUE_CMD);
	  	
			//process Rx
	  		bufptr = rcv_buff;
			nbytes=0;
			bzero(bufptr, 128);
			rs232_timeout=FALSE;
	  		rsp_size = req_size*2 + 7;
	  	
	  		int net_timeout_cnt = 0;
	  		//printf("rsp_size should be %d\n", rsp_size);
	  		for(;;){ // do rs485 receiving during 3 second, or timeout
	  	  	// check if there's data in com port, or time out
				
				if(net_timeout_cnt==RETRY_TIMES_FOR_ONE_SCAN/*isready(fd_rs485)==0*/){
	  				//rs232_timeout=TRUE;
	  				puts("Crash");
	  				rs232_timeout=INV_MSG_CRASH;
				
	  				break;
	  			}
	  		
				// read it !!
	  	 		bytes = read(fd_rs485, (char*)bufptr, rsp_size);	// it will always receive

				nbytes +=bytes;
				bufptr += bytes;
				// check satisfy

				if(bytes <= 0){		// get no data at all before timeout
	  				rs232_timeout=INV_MSG_CRASH;
	  				//puts("Scan Crash");
	  				break;
	  			}else if(rsp_size == nbytes){ // get exact data before timeout
					rs232_timeout=FALSE;
					break;
				}else{		// get partial data, keep watching !!
					rs232_timeout=FALSE;
					//break;
				}
				
				usleep(WAITING_TIME_EACH_SCAN_INV); //0.4 s
				net_timeout_cnt ++;
	  		}

			valid_cnt=0;
			rcv_len = 0;
			
			if(rs232_timeout==FALSE){
				if(nbytes){
					// find 0x0a, check tail 0x0d
					
					for (tmp_i=0; tmp_i<nbytes; tmp_i++){

						if(rcv_buff[tmp_i]==0x0a){
							rcv_len = rcv_buff[tmp_i+3]; // 0x0a prefix is followed by addr, then length <--we want
							tmp_bufp = rcv_buff + tmp_i; // tmp_bufp pointing to the SOF
							//printf("tmp_bufp = %p\n", tmp_bufp);
							
							if(rcv_buff[tmp_i+rcv_len+6]==0x0d){
								valid_cnt = rcv_len+6;
								tail_valid=TRUE;
								break;
							}else{
								valid_cnt = 0;
								puts("pkt tail not valid !!");
								tail_valid=FALSE;
								break;
							}
						}
					}
					
					if(check_motech_CRC16((mt81xx_format_rtn_t *)tmp_bufp)>0){
						//printf("receive RS485 packet with CRC correct !!\n");
						
						// copy the data
						tmp_bufp += 3; // locate ptr to pointing "Byte Count"
						
						//printf("cpy_size = %d, *tmp_bufp = %d\n", cpy_size, *((uint8_t*)tmp_bufp));
						if(tail_valid && ( cpy_size == *((uint8_t*)tmp_bufp) )){
							tmp_bufp += 1;	// locate to data
							
							//printf("We get #%d data:%s\n", cpy_size, tmp_bufp);

							tmp_bufp[sizeof(invBnTough)-1] = '\0';

							if(strcmp((char*)invBnTough, (char*)tmp_bufp)==0)
								invIsTOUGH = TRUE;
							else
								invIsTOUGH = FALSE;
							// switch bytes
							// if it is ASCII field, don't switch
							//switch_bytes((uint8_t*)tmp_bufp, cpy_size>>1);
							
							//memcpy(cpy_ptr, tmp_bufp, cpy_size);
							//printf("Get data 0x%.4x\n", cpy_ptr[0]);
	  		  			}else
	  		  				printf("copy size mismatch, copy abort, data size in rtn is %d, cpy_size == %d\n", tmp_bufp[0], cpy_size);
	  		  		
							// modify count
							nbytes = valid_cnt;
							valid_cnt=0;					
					}else{
						printf("receive RS485 packet with CRC error !!\n");	
						
					}
				}
			}	
	  	
			//change state
			if(rs232_timeout==INV_MSG_CRASH){
				inv_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NORMAL);
			}
    		else if(rs232_timeout==FALSE && invIsTOUGH){
    			inv_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NORMAL);	
    			printf("TOUGH inverter address=%d found on %s\n", inv_addr, scan_port_name);
    			inv_info->num = inv_info->num + 1;
    			inv_info->invInfo[inv_info->num -1].invId = inv_addr;
			}else{
				printf("RS485 timeout !!\n");
				inv_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_TIMEOUT);
			}
		}
		else
			inv_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NORMAL);
		
	}

	puts("exit scan ali !");
	close(fd_rs485);

	return 1;
}

int invInfoGet_Ali(uint16_t invAddr, inverter_ali_raw_t *data, char* scan_port_name){

  int fd_rs485;

  uint8_t rcv_buff[128];
  uint8_t *bufptr;

  raw_485_t raw_485_pkg;
  uint32_t req_size, nbytes, bytes;

  uint8_t rs232_timeout = FALSE;
  
  bool_t tail_valid=FALSE, this_pkt_valid = TRUE;
  uint8_t * tmp_bufp, * data_basep;
  uint16_t inv_addr /*= 0x03*/, inv_reg2rd;
	
  inv_ali_state_t inv_cmd_state = INV_ALI_STATE_INIT;
  uint8_t cpy_size;
  uint16_t * cpy_ptr=NULL;

  int net_timeout_cnt, rs485_timeout_cnt, size_to_read;

  inverter_ali_raw_t ali_inverter_info_local;

  bzero(&ali_inverter_info_local, sizeof(inverter_ali_raw_t));

	// Initial UART
  if(!scan_port_name)
	return -1;  
  fd_rs485 = open_port_RS485(scan_port_name, O_RDWR| O_NOCTTY);
  
  if(fd_rs485 < 0){
  	printf("Open port %s failed !!", scan_port_name);
  	return -1;
  }

	//printf("Current BaudRate=%d\n",getbaud(fd_rs485) );
  /***************************
  *  Get State
  ****************************/
	while(1){
		// get inv_reg2rd, cpy_ptr, cpy_size by current state
		if(getRegsMatrix_Ali(&ali_inverter_info_local, inv_cmd_state, &inv_reg2rd, &cpy_ptr, &cpy_size)<=0){
			printf("state no found in ali protocol, inv_cmd_state=%d, goes to STOP state \n", inv_cmd_state);
			inv_ali_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_STOP);
		}

		// print current state
		printf("inv_cmd_state=%d\n", inv_cmd_state);
		

  /***************************
  *  Process Special States
  ****************************/		
		if (inv_cmd_state == INV_ALI_STATE_INIT)	{
			//sleep(1);  // 20130611
			rs485_timeout_cnt = 0;
			this_pkt_valid = TRUE;
			inv_ali_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NORMAL);
		}
		else if (inv_cmd_state == INV_ALI_STATE_RDY)
			inv_ali_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NORMAL);
		else if (inv_cmd_state == INV_ALI_STATE_TIMEOUT)
			inv_ali_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_STOP);
		else if(inv_cmd_state == INV_ALI_STATE_END)
		{
			
			puts("Dump meta data!!\n");

			//inv_meta_data_dump(&inverter_info);
			
			if(this_pkt_valid)
				update_inv_msg2host_ali(data, &ali_inverter_info_local,invAddr);
			else{
				puts("This capture has error field, update this with ID=0");
				invAddr=0;
				update_inv_msg2host_ali(data, &ali_inverter_info_local,invAddr);
			}

			//sleep(1);
			if(!rs232_run_flag){
				inv_ali_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_STOP);
				//printf("To the end of Inverter probe, waiting for re-start semaphore!!\n");
			}
			else{
				inv_ali_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_END);
				printf("To the end of Inverter probe, automatically restart later !!\n");
			}
			
			// break the while loop
			break;
		}
		else if(inv_cmd_state == INV_ALI_STATE_STOP)
		{
			printf("reach Ali info_get stop state, update id = 0 to this slot !!\n");
			invAddr = 0;
			update_inv_msg2host_ali(data, &ali_inverter_info_local,invAddr);				
			
			break;
		}

  /***************************
  *  Normal States
  ****************************/		
		else if(inv_cmd_state&0xf0)
		{
			bzero(&raw_485_pkg, sizeof(raw_485_pkg));
			raw_485_pkg.pkt_type = mt81xx_cmd_read;
			// req_size is counted in word size, one word contains two bytes
			req_size = cpy_size >> 1;
			inv_addr = (uint16_t)invAddr;
			
			// create data frame
			if(create_mt81xx_cmd(&raw_485_pkg, inv_addr, inv_reg2rd , req_size)<0)
				printf("Create Read Cmd to Rs485 failed!!\n");
			if(append_motech_CRC16(&raw_485_pkg)<0)
				printf("Adding CRC16 error !!\n");

  /***************************
  *  Send Read
  ****************************/			
	  		write(fd_rs485, &(raw_485_pkg.uniFrame.im_mt81xx_format03), sizeof(mt81xx_format03_t));
			
			usleep(WAITING_TIME_AFTER_ISSUE_CMD);
  /***************************
  *  Waiting for Response
  ****************************/	  	
  			//process Rx
	  		bufptr = rcv_buff;
			nbytes=0;
			bzero(bufptr, 128);
			rs232_timeout=FALSE;
			// start + slave + fun + len + Data... + CRC_lo + CRC_Hi + stop
	  		size_to_read = req_size*2 + 7;
	  	
	  		net_timeout_cnt = 0;

	  		for(;;){ // do rs485 receiving during 3 second, or timeout
	  	  	// check if there's data in com port, or time out
				
				if(net_timeout_cnt==RETRY_TIMES_FOR_ONE_CMD/*isready(fd_rs485)==0*/){
	  				//rs232_timeout=TRUE;
	  				puts("Timeout !!");
	  				rs232_timeout=INV_MSG_CRASH;
				
	  				break;
	  			}
	  		
	  			usleep(WAITING_TIME_EACH_RETRY); //0.4 s

				// read it !!
	  	 		bytes = read(fd_rs485, (char*)bufptr, size_to_read);	// it will always receive

				nbytes +=bytes;
				bufptr += bytes;
				size_to_read -= bytes;
				// check satisfy

				if(size_to_read <= 0){
					rs232_timeout=FALSE;
					break;
				}
				
				
				net_timeout_cnt ++;
	  		}

  /***************************
  *  check received packet
  ****************************/
			if(rs232_timeout==FALSE){
				if(nbytes && (cpy_ptr!=NULL)){
					
					// check if the leading is 0x0a, check tail is 0x0d
					if((rcv_buff[0]==0x0a)&&(rcv_buff[nbytes-1]==0x0d))
						tail_valid = TRUE;
					else
						tail_valid = FALSE;

					tmp_bufp = rcv_buff;

					if(check_motech_CRC16((mt81xx_format_rtn_t *)tmp_bufp)>0){

						if(tail_valid && ( cpy_size == ((mt81xx_format_rtn_t*)tmp_bufp)->mt81xx_offset_Len )){
							
							data_basep = &(((mt81xx_format_rtn_t*)tmp_bufp)->mt81xx_offset_data_base);

							// switch bytes, if it is ASCII field, don't switch
							//if ((inv_cmd_state != INV_ALI_STATE_BN)&&(inv_cmd_state != INV_CMD_STATE_TN)&&(inv_cmd_state != INV_CMD_STATE_SN))
								switch_bytes( data_basep, cpy_size>>1);

							// store the received data into internal place
							memcpy(cpy_ptr, data_basep, cpy_size);
							
	  		  			}else
	  		  				printf("copy size mismatch, copy abort, cpy_ptr == %d\n", sizeof(cpy_ptr));
					
					}else{
						printf("receive RS485 packet with CRC error !!\n");	
						rs232_timeout=INV_MSG_CRC_ERR;
					}
				}else
					puts("cpy_ptr is NULL or nbyte is zero");

			}	
			
  /***************************
  *  Change State
  ****************************/
			if(rs232_timeout==INV_MSG_CRASH){
				if(rs485_timeout_cnt < CMD_REPEAT_TIMES_WHEN_FAILED){
					inv_ali_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NOCHANGE);
					puts("Try re-issue this cmd !!");
					rs485_timeout_cnt ++;
				}
				else{  // abort read this reg after rs485_timeout_cnt times of retries
					inv_ali_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_STOP);	
					puts("Abort reading this item in RS485 !!");
					rs485_timeout_cnt = 0;
					this_pkt_valid = FALSE;
				}
			}else if(rs232_timeout==INV_MSG_CRC_ERR){
				// clear rx buffer
				bytes = 1;
				sleep(1);
				while(bytes > 0){
					bytes = read(fd_rs485, (char*)rcv_buff, 32);
					puts("clear RS232 Rx buffer !!");
				}
				if(rs485_timeout_cnt < CMD_REPEAT_TIMES_WHEN_FAILED){
					inv_ali_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NOCHANGE);
					rs485_timeout_cnt ++;
				}
				else{  // abort read this reg after rs485_timeout_cnt times of retries
					inv_ali_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_STOP);	
					puts("Abort reading because of no data respond to this command !!");
					rs485_timeout_cnt = 0;
					this_pkt_valid = FALSE;
				}				
			}
    		else if(rs232_timeout==FALSE){
    			inv_ali_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NORMAL);	
    			rs485_timeout_cnt = 0;
			}else{
					printf("RS485 timeout because of CRC error !!\n");
					inv_ali_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_TIMEOUT);
					this_pkt_valid = FALSE;
			}
		}
		else
			inv_ali_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NORMAL);
	}

	puts("exit get info ali !");
	close(fd_rs485);
	
	return 1;
}


/****************************
** ENERSOLIS Inverter protocol
****************************/

inverter_enersolis_raw_t enersolis_inverter_info;

inv_enersolis_regsmatrix_t enersolis_reg_def[10] = {
	{INV_ENERSOLIS_STATE_INIT, 	0x0, 	NULL, 	0},
	{INV_ENERSOLIS_STATE_RDY, 	0x0, 	NULL, 	0},
	{INV_ENERSOLIS_STATE_TIMEOUT,	0x0,	NULL,	0},
	{INV_ENERSOLIS_STATE_STOP, 	0x0,	NULL,	0},
	{INV_ENERSOLIS_STATE_C1, 		0xc010, 	&(enersolis_inverter_info.errWord0), 		2*2},
	{INV_ENERSOLIS_STATE_C2, 		0xc0b0, 	&(enersolis_inverter_info.inverterType), 	16*2},
	{INV_ENERSOLIS_STATE_C3,		0xc000, 	&(enersolis_inverter_info.alarmWord0), 	2*2},
	{INV_ENERSOLIS_STATE_C4,		0xc020, 	&(enersolis_inverter_info.outputPower), 	28*2},
	{INV_ENERSOLIS_STATE_C5,		0xc03c, 	&(enersolis_inverter_info.AC_voltagePL2L3), 	13*2},
	{INV_ENERSOLIS_STATE_END, 	0x0, 	NULL, 0}
};


int update_inv_msg2host_enersolis(inverter_enersolis_raw_t *data, inverter_enersolis_raw_t *inverter_info, uint16_t invId){

	memcpy(data, inverter_info, sizeof(inverter_enersolis_raw_t));

	data->inverterId 			= invId;

	return 1;
}


int convert_invmsg_to_asci_enersolis(char *buff, inverter_enersolis_raw_t* msg2host){
	int n = 0;
	char tmp_time[15]="";
	char modelName[11]="N/A";//, companyName[11]="N/A";
	uint32_t error=0, alarm=0;
	double pdcA, pdcB, pac, ac_frequency, ac_outcurr1, ac_outcurr2, ac_outcurr3, dc1_input_curr, dc2_input_curr, e_total;
	uint16_t invType=0;

	syscall_getRsp("date +%Y%m%d%H%M%S", tmp_time);
	tmp_time[14] = '\0';
	
	invType = msg2host->inverterType;

	switch_bytes((uint8_t*)&msg2host->modelName0, 10);

	memcpy(modelName, &msg2host->modelName0, 2);
	memcpy(&modelName[2], &msg2host->modelName1, 2);
	memcpy(&modelName[4], &msg2host->modelName2, 2);
	memcpy(&modelName[6], &msg2host->modelName3, 2);
	memcpy(&modelName[8], &msg2host->modelName4, 2);
	modelName[10] = '\0';

	error = msg2host->errWord1;
	error = error << 16;
	error = error + msg2host->errWord0;

	alarm = msg2host->alarmWord1;
	alarm = alarm << 16;
	alarm = alarm + msg2host->alarmWord0;

	e_total = msg2host->totalOutputPwr0;
	e_total = e_total * 65536;
	e_total = e_total + msg2host->totalOutputPwr1;

	pac = msg2host->outputPower;
	pac = pac * 10; // in W unit
	
	ac_frequency = msg2host->AC_frequencyL1;
	ac_frequency = ac_frequency/10;

	ac_outcurr1 = msg2host->AC_outputCurrentL1;
	ac_outcurr1 = ac_outcurr1/10;

	ac_outcurr2 = msg2host->AC_outputCurrentL2;
	ac_outcurr2 = ac_outcurr2/10;

	ac_outcurr3 = msg2host->AC_outputCurrentL3;
	ac_outcurr3 = ac_outcurr3/10;	

	dc1_input_curr = msg2host->DC1inputCurr;
	dc1_input_curr = dc1_input_curr/10;

	dc2_input_curr = msg2host->DC2inputCurr;
	dc2_input_curr = dc2_input_curr/10;

	pdcA = msg2host->inputPwrA;
	pdcA = pdcA * 10; // in W unit

	pdcB = msg2host->inputPwrB;
	pdcB = pdcB * 10; // in W unit


	n = sprintf(buff, "%04d,%s,%s,%04d,%s,%08x,%08x,%.0f,%d,%d,%d,%2.1f,%2.1f,%2.1f,%2.1f,%d,%d,%.1f,%.1f,%d,%d,%d,%d,%.0f,%.0f,%.0f,0x%04x", 
		msg2host->inverterId, "ENERSOLIS", modelName, msg2host->inverterId, tmp_time, alarm, error, 
		pac, msg2host->AC_voltagePL1, msg2host->AC_voltagePL2, msg2host->AC_voltagePL3, ac_frequency,
		ac_outcurr1, ac_outcurr2, ac_outcurr3,
		msg2host->DC1inputVolt, msg2host->DC2inputVolt, dc1_input_curr, dc2_input_curr,
		msg2host->DCBusPosVolt, msg2host->DCBusNegVolt, msg2host->invIntlTemp, msg2host->invHeatSinkTemp, 
		pdcA, pdcB, e_total, invType);

	buff[n] = '\0';	

	return 1; 
	
}


int inv_enersolis_state_chg_next(inv_enersolis_state_t* curr_inv_cmd_state, inv_cmd_event_t in_event){		//inv_ali_cmd_state_t
	
	switch (*curr_inv_cmd_state) {
		case INV_ENERSOLIS_STATE_INIT:
			if(!in_event)	// event normal
				*curr_inv_cmd_state = INV_ENERSOLIS_STATE_RDY;
			else
				*curr_inv_cmd_state = INV_ENERSOLIS_STATE_STOP;
		break;	
		case INV_ENERSOLIS_STATE_RDY:
			if(in_event == INV_CMD_EVENT_SCAN)
				*curr_inv_cmd_state = INV_ENERSOLIS_STATE_SCAN_RUN;
			else if(in_event == INV_CMD_EVENT_NORMAL)
				*curr_inv_cmd_state = INV_ENERSOLIS_STATE_C1;
			else if (in_event&(INV_CMD_EVENT_ERR|INV_CMD_EVENT_STOP))
				*curr_inv_cmd_state = INV_ENERSOLIS_STATE_STOP;
		break;
		case INV_ENERSOLIS_STATE_ERR:
			if(!in_event)
				*curr_inv_cmd_state = INV_ENERSOLIS_STATE_RDY;
			else
				*curr_inv_cmd_state = INV_ENERSOLIS_STATE_STOP;
		break;
		case INV_ENERSOLIS_STATE_STOP:
			if(!in_event)
				*curr_inv_cmd_state = INV_ENERSOLIS_STATE_RDY;
			else
				*curr_inv_cmd_state = INV_ENERSOLIS_STATE_STOP;
		break;
		case INV_ENERSOLIS_STATE_TIMEOUT:
			if(!in_event)
				*curr_inv_cmd_state = INV_ENERSOLIS_STATE_ERR;
			else
				*curr_inv_cmd_state = INV_ENERSOLIS_STATE_STOP;
		break;
		
		case INV_ENERSOLIS_STATE_C1:
		case INV_ENERSOLIS_STATE_C2:
		case INV_ENERSOLIS_STATE_C3:
		case INV_ENERSOLIS_STATE_C4:
		case INV_ENERSOLIS_STATE_C5:
			if (in_event == INV_CMD_EVENT_NOCHANGE)
				*curr_inv_cmd_state = *curr_inv_cmd_state;
			else if(in_event == INV_CMD_EVENT_NORMAL)	// event normal
				*curr_inv_cmd_state = *curr_inv_cmd_state + 1;
			else if (in_event&INV_CMD_EVENT_TIMEOUT)
				*curr_inv_cmd_state = INV_ENERSOLIS_STATE_TIMEOUT;
			else if (in_event&INV_CMD_EVENT_ERR)
				*curr_inv_cmd_state = INV_ENERSOLIS_STATE_ERR;
			else if (in_event&INV_CMD_EVENT_STOP)
				*curr_inv_cmd_state = INV_ENERSOLIS_STATE_STOP;
		break;

		case INV_ENERSOLIS_STATE_SCAN_RUN:
			if (in_event == INV_CMD_EVENT_NOCHANGE)
				*curr_inv_cmd_state = INV_ENERSOLIS_STATE_SCAN_REPEAT;
			else if(in_event == INV_CMD_EVENT_STOP)
				*curr_inv_cmd_state = INV_ENERSOLIS_STATE_STOP;
			else if (in_event == INV_CMD_EVENT_END)
				*curr_inv_cmd_state = INV_ENERSOLIS_STATE_SCAN_END;
			else 
				*curr_inv_cmd_state = *curr_inv_cmd_state;
		break;

		case INV_ENERSOLIS_STATE_SCAN_REPEAT:
			if (in_event == INV_CMD_EVENT_NORMAL)
				*curr_inv_cmd_state = INV_ENERSOLIS_STATE_SCAN_RUN;		
			else if (in_event == INV_CMD_EVENT_NOCHANGE)
				*curr_inv_cmd_state = INV_ENERSOLIS_STATE_SCAN_REPEAT;
			else if(in_event == INV_CMD_EVENT_STOP)
				*curr_inv_cmd_state = INV_ENERSOLIS_STATE_STOP;
			else if (in_event == INV_CMD_EVENT_END)
				*curr_inv_cmd_state = INV_ENERSOLIS_STATE_SCAN_END;
			else 
				*curr_inv_cmd_state = *curr_inv_cmd_state;
		break;	
		
		case INV_ENERSOLIS_STATE_END:
			if(in_event == INV_CMD_EVENT_NORMAL)	// event normal
				*curr_inv_cmd_state = INV_ENERSOLIS_STATE_RDY;
			else if(in_event == INV_CMD_EVENT_END)
				*curr_inv_cmd_state = INV_ENERSOLIS_STATE_INIT;
			else
				*curr_inv_cmd_state = INV_ENERSOLIS_STATE_STOP;
		break;
		
		case INV_ENERSOLIS_STATE_SCAN_END:
			if(in_event == INV_CMD_EVENT_STOP)
				*curr_inv_cmd_state = INV_ENERSOLIS_STATE_STOP;
		
		default:;
	}
	
	return 1;
}


int getRegsMatrix_EnerSolis(inv_enersolis_state_t state, uint16_t* out_addr, uint16_t** out_ptr, uint8_t* out_size){
	uint32_t i;
	bool_t isFound=FALSE;

	for (i=0; i<(sizeof(enersolis_reg_def)/sizeof(inv_enersolis_regsmatrix_t)); i++){
		if(enersolis_reg_def[i].state_num == state){
			isFound = TRUE;
			*out_addr = enersolis_reg_def[i].reg_addr;
			*out_ptr = enersolis_reg_def[i].ptr_addr;
			*out_size = enersolis_reg_def[i].cpy_size;
			break;
		}
	}

	if (isFound)
		return 1;
	else{
		printf("State #%d no found in matrix\n", state);
		*out_addr = 0x0;
		*out_ptr = NULL;
		*out_size = 0;
		return -1;
	}
}


int invInfoGet_EnerSolis(uint16_t invAddr, inverter_enersolis_raw_t *data, char* scan_port_name){

  int fd_rs485;

  uint8_t rcv_buff[256];
  uint8_t *bufptr;

  raw_485_t raw_485_pkg;
  uint32_t req_size, nbytes, bytes;

  uint8_t rs232_timeout = FALSE;
  
  bool_t this_pkt_valid = TRUE;
  uint8_t * tmp_bufp, * data_basep;
  uint16_t inv_addr /*= 0x03*/, inv_reg2rd;
	
  inv_enersolis_state_t inv_cmd_state = INV_ENERSOLIS_STATE_INIT;
  uint8_t cpy_size;
  uint16_t * cpy_ptr=NULL;

  int net_timeout_cnt, rs485_timeout_cnt ,size_to_read;

	// Initial UART
  if(!scan_port_name)
	return -1;  
  fd_rs485 = open_port_RS485(scan_port_name, O_RDWR| O_NOCTTY);
  
  if(fd_rs485 < 0){
  	printf("Open port %s failed !!", scan_port_name);
  	return -1;
  }   
	//printf("Current BaudRate=%d\n",getbaud(fd_rs485) );
  /***************************
  *  Get State
  ****************************/
	while(1){
		// get inv_reg2rd, cpy_ptr, cpy_size by current state
		if(getRegsMatrix_EnerSolis(inv_cmd_state, &inv_reg2rd, &cpy_ptr, &cpy_size)<=0){
			printf("state no found in enersolis protocol, inv_cmd_state=%d, goes to STOP state \n", inv_cmd_state);
			inv_enersolis_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_STOP);
		}

		// print current state
		printf("inv_cmd_state=%d\n", inv_cmd_state);
		

  /***************************
  *  Process Special States
  ****************************/		
		if (inv_cmd_state == INV_ENERSOLIS_STATE_INIT)	{
			//sleep(1);  // 20130611
			rs485_timeout_cnt = 0;
			this_pkt_valid = TRUE;
			inv_enersolis_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NORMAL);
		}
		else if (inv_cmd_state == INV_ENERSOLIS_STATE_RDY)
			inv_enersolis_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NORMAL);
		else if (inv_cmd_state == INV_ENERSOLIS_STATE_TIMEOUT)
			inv_enersolis_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_STOP);
		else if(inv_cmd_state == INV_ENERSOLIS_STATE_END)
		{
			
			printf("Dump meta data, invId=%d !!\n", invAddr);

			//inv_meta_data_dump(&inverter_info);
			
			if(this_pkt_valid)
				update_inv_msg2host_enersolis(data, &enersolis_inverter_info, invAddr);
			else{
				puts("This capture has error field, update this with ID=0");
				invAddr=0;
				update_inv_msg2host_enersolis(data, &enersolis_inverter_info, invAddr);
			}

			//sleep(1);
			if(!rs232_run_flag){
				inv_enersolis_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_STOP);
				//printf("To the end of Inverter probe, waiting for re-start semaphore!!\n");
			}
			else{
				inv_enersolis_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_END);
				printf("To the end of Inverter probe, automatically restart later !!\n");
			}
			
			// break the while loop
			break;
		}
		else if(inv_cmd_state == INV_ENERSOLIS_STATE_STOP)
		{
			printf("reach EnerSolis info_get stop state, update id = 0 to this slot !!\n");
			invAddr = 0;
			update_inv_msg2host_enersolis(data, &enersolis_inverter_info,invAddr);	

			break;
		}

  /***************************
  *  Normal States
  ****************************/		
		else if(inv_cmd_state&0xf0)
		{
			bzero(&raw_485_pkg, sizeof(raw_485_pkg));
			raw_485_pkg.pkt_type = mt81xx_cmd_read;
			// req_size is counted in word size, one word contains two bytes
			req_size = cpy_size >> 1;
			inv_addr = invAddr&0x00ff;
			
			// create data frame
			if(create_modbus_read(&raw_485_pkg.uniFrame.im_modbus_format_read, inv_addr, inv_reg2rd , req_size)<0)
				printf("Create Read Cmd to Rs485 failed!!\n");
			if(append_CRC16((modbus_format_read_t*)&raw_485_pkg.uniFrame.im_modbus_format_read.modbus_offset_SlaveAddr, 6)<0)
				printf("Adding CRC16 error !!\n");

  /***************************
  *  Send Read
  ****************************/			

			write(fd_rs485, &(raw_485_pkg.uniFrame.im_modbus_format_read), sizeof(raw_485_pkg.uniFrame.im_modbus_format_read));

			usleep(WAITING_TIME_AFTER_ISSUE_CMD);
  /***************************
  *  Waiting for Response
  ****************************/	  	
  			//process Rx
	  		bufptr = rcv_buff;
			nbytes=0;
			bzero(bufptr, 256);
			rs232_timeout=FALSE;
			// slave + fun + len + Data... + CRC_lo + CRC_Hi 
	  		size_to_read = req_size*2 + 5;
	  	
	  		net_timeout_cnt = 0;

	  		for(;;){ // do rs485 receiving during 3 second, or timeout
	  	  	// check if there's data in com port, or time out
				
				if(net_timeout_cnt==RETRY_TIMES_FOR_ONE_CMD/*isready(fd_rs485)==0*/){
	  				//rs232_timeout=TRUE;
	  				puts("Timeout !!");
	  				rs232_timeout=INV_MSG_CRASH;
				
	  				break;
	  			}
				
				usleep(WAITING_TIME_EACH_RETRY); 	  		
				
				// read it !!
	  	 		bytes = read(fd_rs485, (char*)bufptr, size_to_read);	// it will always receive

				nbytes +=bytes;
				bufptr += bytes;
				size_to_read -= bytes;
				// check satisfy

				//printf("#%d time(s) to read data, demand size=%d, bytes this time=%d, total bytes =%d, size to read=%d\n", net_timeout_cnt, (req_size*2 + 5), bytes, nbytes, size_to_read);

				if(size_to_read <= 0){
					rs232_timeout=FALSE;
					break;
				}
				
				net_timeout_cnt ++;
	  		}

  /***************************
  *  check received packet
  ****************************/
			if(rs232_timeout==FALSE){
				if(nbytes && (cpy_ptr!=NULL)){

					tmp_bufp = rcv_buff;

					if(check_CRC16((modbus_format_rtn_t *)tmp_bufp)>0){

						if( cpy_size == ((modbus_format_rtn_t*)tmp_bufp)->modbus_offset_Len ){
							
							data_basep = &(((modbus_format_rtn_t*)tmp_bufp)->modbus_offset_data_base);

							// switch bytes, if it is ASCII field, don't switch
							//if ((inv_cmd_state != INV_ALI_STATE_BN)&&(inv_cmd_state != INV_CMD_STATE_TN)&&(inv_cmd_state != INV_CMD_STATE_SN))
								switch_bytes( data_basep, cpy_size>>1);

							// store the received data into internal place
							memcpy(cpy_ptr, data_basep, cpy_size);
							
	  		  			}else
	  		  				printf("copy size mismatch, copy abort, cpy_ptr == %d\n", sizeof(cpy_ptr));
					
					}else{
						printf("receive RS485 packet with CRC error !!\n");	
						rs232_timeout=INV_MSG_CRC_ERR;
					}
				}else
					puts("cpy_ptr is NULL or nbyte is zero");

			}	
			
  /***************************
  *  Change State
  ****************************/
			if(rs232_timeout==INV_MSG_CRASH){
				if(rs485_timeout_cnt < CMD_REPEAT_TIMES_WHEN_FAILED){
					inv_enersolis_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NOCHANGE);
					puts("Try re-issue this cmd !!");
					rs485_timeout_cnt ++;
				}
				else{  // abort read this reg after rs485_timeout_cnt times of retries
					inv_enersolis_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_STOP);	
					puts("Abort reading because of no data respond to this command !!");
					rs485_timeout_cnt = 0;
					this_pkt_valid = FALSE;
				}
			}else if(rs232_timeout==INV_MSG_CRC_ERR){
				// clear rx buffer
				bytes = 1;
				sleep(1);
				while(bytes > 0){
					bytes = read(fd_rs485, (char*)rcv_buff, 32);
					puts("clear RS232 Rx buffer !!");
				}

				if(rs485_timeout_cnt < CMD_REPEAT_TIMES_WHEN_FAILED){
					inv_enersolis_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NOCHANGE);
					rs485_timeout_cnt ++;
				}
				else{  // abort read this reg after rs485_timeout_cnt times of retries
					inv_enersolis_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_STOP);	
					puts("Abort reading because of CRC error !!");
					rs485_timeout_cnt = 0;
					this_pkt_valid = FALSE;
				}
			}else if(rs232_timeout==FALSE){
    			inv_enersolis_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NORMAL);	
    			rs485_timeout_cnt = 0;
			}else{
					printf("RS485 timeout !!\n");
					inv_enersolis_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_TIMEOUT);
					this_pkt_valid = FALSE;
			}
		}
		else
			inv_enersolis_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NORMAL);
	}

	puts("exit get info enersolis !");
	close(fd_rs485);
	
	return 1;
}


int inverter_scan_enersolis(scan_inv_info_t* inv_info, char* scan_port_name){

  	int fd_rs485;
  //int fd_gpio88;

  	uint8_t rcv_buff[128];
  	uint8_t *bufptr;

  	int i, rs485_timeout_cnt = 0;
	raw_485_t raw_485_pkg;
	uint32_t req_size, rsp_size, nbytes, bytes;

	//const uint8_t invBnEnerSolis[10] = "EnerSolis";
	bool_t invIsEnerSolis;
  	uint8_t rs232_timeout = FALSE;
  	//uint8_t rcv_len;
  
  	uint32_t valid_cnt=0;
 
  	uint8_t * tmp_bufp;
	uint16_t inv_addr, inv_reg2rd;
	
	uint8_t cpy_size;
	inv_enersolis_state_t inv_cmd_state = INV_ENERSOLIS_STATE_INIT;
	
	//printf("In handle rs232 thread !!\n");

	//
	// Initial UART
	if(!scan_port_name)
		return -1;

  	fd_rs485 = open_port_RS485(scan_port_name, O_RDWR| O_NOCTTY);
    
    if(fd_rs485 < 0){
  		printf("Open port %s failed !!", scan_port_name);
  		return -1;
  	}
	//printf("Current BaudRate=%d\n",getbaud(fd_rs485) );
	
	while(1){

		switch (inv_cmd_state){

		case INV_ENERSOLIS_STATE_INIT:
			inv_reg2rd = 0;
			inv_info->num = 0;
			inv_addr = INV_START_ADDR - 1;
		break;

		case INV_ENERSOLIS_STATE_RDY:
			cpy_size = 0;
			inv_reg2rd = 0;
		break;
		
		case INV_ENERSOLIS_STATE_TIMEOUT:
			cpy_size = 0;
			inv_reg2rd = 0;
		break;
		
		case INV_ENERSOLIS_STATE_STOP:
			cpy_size = 0;
			inv_reg2rd = 0;
		break;
		
		case INV_ENERSOLIS_STATE_SCAN_RUN:
			inv_reg2rd = 0xc0b0;
			inv_addr = inv_addr + 1;
			req_size = 8;
			cpy_size = req_size * 2;
			invIsEnerSolis = FALSE;
		break;

		case INV_ENERSOLIS_STATE_SCAN_REPEAT:
			inv_reg2rd = 0xc0b0;		
			req_size = 8;
			cpy_size = req_size * 2;
			invIsEnerSolis = FALSE;
		break;

		case INV_ENERSOLIS_STATE_SCAN_END:
		//initial
			//cpy_ptr = NULL;
			cpy_size = 0;
			//sem_wait(&sem_rs232);	
		break;
		
		default:
		// display
			printf("default state, inv_cmd_state=%d\n", inv_cmd_state);
		// next
			inv_enersolis_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_STOP);
		}	// switch case
	//
	// Does task, then update state
	// 
		
		if (inv_cmd_state == INV_ENERSOLIS_STATE_INIT)	{
			//printf("RS232 Initial again !!\n");
			//sleep(1);
			rs485_timeout_cnt = 0;
			inv_enersolis_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NORMAL);
		}
		else if(inv_cmd_state == INV_ENERSOLIS_STATE_RDY)
			inv_enersolis_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_SCAN);
		else if (inv_cmd_state == INV_ENERSOLIS_STATE_TIMEOUT){
				inv_enersolis_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_STOP);
		}
		else if(inv_cmd_state == INV_ENERSOLIS_STATE_SCAN_END)
		{
			puts("Finish Scan process!!\n");
			
			// dump message
			
			inv_enersolis_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_STOP);
			printf("To the end of Inverter probe !!\n");
			
			// break the while
			break;
		}
		else if(inv_cmd_state == INV_ENERSOLIS_STATE_STOP)
		{
			printf("reach EnerSolis SCAN stop state !!\n");
			close(fd_rs485);
			return 0;
			//inv_enersolis_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NORMAL);
		}
		else if(inv_cmd_state&0xf00)
		{
			// check if it reach the end
			if(inv_addr > INV_MAX)
				inv_enersolis_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_END);
			
			bzero(&raw_485_pkg, sizeof(raw_485_pkg));
			raw_485_pkg.pkt_type = mt81xx_cmd_read;	
			
			if(create_modbus_read(&raw_485_pkg.uniFrame.im_modbus_format_read, inv_addr, inv_reg2rd , req_size)<0)
				printf("Create Read Cmd to Rs485 failed!!\n");
			if(append_CRC16((modbus_format_read_t*)&raw_485_pkg.uniFrame.im_modbus_format_read.modbus_offset_SlaveAddr, 6)<0)
				printf("Adding CRC16 error !!\n");
/*
			if(create_mt81xx_cmd(&raw_485_pkg, inv_addr, inv_reg2rd , req_size)<0)
				printf("Create Read Cmd to Rs485 failed!!\n");
			if(append_motech_CRC16(&raw_485_pkg)<0)
				printf("Adding CRC16 error !!\n");
*/

    		printf("Scan EnerSolis inv# = %d on %s, fd=%d\n", inv_addr, scan_port_name, fd_rs485);
    	
			i=0;

			i = sizeof(raw_485_pkg.uniFrame.im_modbus_format_read);
/*			
			while(((uint8_t*)&(raw_485_pkg.uniFrame.im_mt81xx_format03))[i] != 0x0d){
				//printf("%.2x ",((uint8_t*)&(raw_485_pkg.uniFrame.im_mt81xx_format03))[i]);
				i++;
			}
*/			
			//select Tx
			//write(fd_gpio88, "1", 1);
			//write(fd_rs485, &(raw_485_pkg.uniFrame.im_mt81xx_format03), i+1);

			write(fd_rs485, &(raw_485_pkg.uniFrame.im_modbus_format_read), i);
	  	
	  		usleep(WAITING_TIME_AFTER_ISSUE_CMD);
	  	
			//process Rx
	  		bufptr = rcv_buff;
			nbytes=0;
			bzero(bufptr, 128);
			rs232_timeout=FALSE;
	  		rsp_size = req_size*2 + 5;
	  	
	  		int net_timeout_cnt = 0;
	  		//printf("rsp_size should be %d\n", rsp_size);
	  		for(;;){ // do rs485 receiving during 3 second, or timeout
	  	  	// check if there's data in com port, or time out
				
				if(net_timeout_cnt==RETRY_TIMES_FOR_ONE_SCAN/*isready(fd_rs485)==0*/){
	  				//rs232_timeout=TRUE;
	  				puts("Crash");
	  				rs232_timeout=INV_MSG_CRASH;
				
	  				break;
	  			}

				//usleep(100000); //0.4 s	  		
				
				// read it !!
	  	 		bytes = read(fd_rs485, (char*)bufptr, rsp_size);	// it will always receive
	  	 		
				nbytes +=bytes;
				bufptr += bytes;
				// check satisfy

				if(bytes <= 0){		// get no data at all before timeout
	  				rs232_timeout=INV_MSG_CRASH;
	  				//puts("Scan Crash");
	  				break;
	  			}else if(rsp_size == nbytes){ // get exact data before timeout
					rs232_timeout=FALSE;
					break;
				}else{		// get partial data, keep watching !!
					rs232_timeout=FALSE;
					//break;
				}
				
				usleep(WAITING_TIME_EACH_SCAN_INV);
				net_timeout_cnt ++;
	  		}

			valid_cnt=0;
			//rcv_len = 0;
			
			if(rs232_timeout==FALSE){
				if(nbytes){

					tmp_bufp = rcv_buff;
					if(check_CRC16((modbus_format_rtn_t *)tmp_bufp)>0){
						//printf("receive RS485 packet with CRC correct !!\n");
						
						// copy the data
						tmp_bufp += 2; // locate ptr to pointing "Byte Count"
						
						//printf("cpy_size = %d, *tmp_bufp = %d\n", cpy_size, *((uint8_t*)tmp_bufp));
						if( cpy_size == *((uint8_t*)tmp_bufp) ){
							
							// assume it must be EnerSolis
							invIsEnerSolis = TRUE;
							/*
							tmp_bufp += 1;	// locate to data
							
							tmp_bufp += 4;	// locate to Model name
							//printf("We get #%d data:%s\n", cpy_size, tmp_bufp);

							tmp_bufp[sizeof(invBnEnerSolis)-1] = '\0';

							//invIsEnerSolis = TRUE;

							
							if(strcmp((char*)invBnEnerSolis, (char*)tmp_bufp)==0)
								invIsEnerSolis = TRUE;
							else
								invIsEnerSolis = FALSE;
							*/

							// switch bytes
							// if it is ASCII field, don't switch
							//switch_bytes((uint8_t*)tmp_bufp, cpy_size>>1);
							
							//memcpy(cpy_ptr, tmp_bufp, cpy_size);
							//printf("Get data 0x%.4x\n", cpy_ptr[0]);
	  		  			}else
	  		  				printf("copy size mismatch, copy abort, data size in rtn is %d, cpy_size == %d\n", tmp_bufp[0], cpy_size);
	  		  		
							// modify count
							nbytes = valid_cnt;
							valid_cnt=0;					
					}else{
						printf("receive RS485 packet with CRC error !!\n");	
						
					}
				}
			}	
	  	
			//change state
			if(rs232_timeout==INV_MSG_CRASH){
				if(rs485_timeout_cnt< SCAN_REPEAT_TIMES_WHEN_FAILED){	// retry
					inv_enersolis_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NOCHANGE);
					rs485_timeout_cnt++;
				}
				else{
					inv_enersolis_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NORMAL);	
					rs485_timeout_cnt = 0;
				}
			}else if(rs232_timeout==INV_MSG_CRC_ERR){
				// clear rx buffer
				bytes = 1;
				usleep(500000);
				while(bytes > 0){
					bytes = read(fd_rs485, (char*)rcv_buff, 32);
					puts("clear RS232 Rx buffer !!");
				}				
				if(rs485_timeout_cnt<1){	// retry 
					inv_enersolis_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NOCHANGE);
					rs485_timeout_cnt++;
				}
				else{
					inv_enersolis_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NORMAL);
					rs485_timeout_cnt = 0;
				}							
			}
    		else if(rs232_timeout==FALSE && invIsEnerSolis){
    			inv_enersolis_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NORMAL);	
    			printf("EnerSolis inverter address=%d found on %s\n", inv_addr, scan_port_name);
    			inv_info->num += 1;
    			inv_info->invInfo[inv_info->num -1].invId = inv_addr;
			}else{
				printf("RS485 timeout !!\n");
				inv_enersolis_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_TIMEOUT);
			}
		}
		else
			inv_enersolis_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NORMAL);
		
	}

	puts("exit scan enersolis !");
	close(fd_rs485);

	return 1;
}

#if 0
/****************************
** ENERSOLIS Inverter protocol
****************************/

inverter_enersolis_raw_t enersolis_inverter_info;

inv_enersolis_regsmatrix_t enersolis_reg_def[10] = {
	{INV_ENERSOLIS_STATE_INIT, 	0x0, 	0, 	0},
	{INV_ENERSOLIS_STATE_RDY, 	0x0, 	0, 	0},
	{INV_ENERSOLIS_STATE_TIMEOUT,	0x0,	0,	0},
	{INV_ENERSOLIS_STATE_STOP, 	0x0,	0,	0},
	{INV_ENERSOLIS_STATE_C1, 		0xc010, 	(uint32_t)&(enersolis_inverter_info.errWord0) - (uint32_t)&enersolis_inverter_info, 		2*2},

	{INV_ENERSOLIS_STATE_END, 	0x0, 	0, 0}
};


int update_inv_msg2host_enersolis(inverter_enersolis_raw_t *data, inverter_enersolis_raw_t *inverter_info, uint16_t invId){

	memcpy(data, inverter_info, sizeof(inverter_enersolis_raw_t));

	data->inverterId 			= invId;

	return 1;
}


int convert_invmsg_to_asci_enersolis(char *buff, inverter_enersolis_raw_t* msg2host){
	int n = 0;
	char tmp_time[15]="";
	char modelName[11]="N/A";//, companyName[11]="N/A";
	uint32_t error=0, alarm=0;
	double pdcA, pdcB, pac, ac_frequency, ac_outcurr1, ac_outcurr2, ac_outcurr3, dc1_input_curr, dc2_input_curr, e_total;
	
	syscall_getRsp("date +%Y%m%d%H%M%S", tmp_time);
	tmp_time[14] = '\0';
	
	switch_bytes((uint8_t*)&msg2host->modelName0, 10);

	memcpy(modelName, &msg2host->modelName0, 2);
	memcpy(&modelName[2], &msg2host->modelName1, 2);
	memcpy(&modelName[4], &msg2host->modelName2, 2);
	memcpy(&modelName[6], &msg2host->modelName3, 2);
	memcpy(&modelName[8], &msg2host->modelName4, 2);
	modelName[10] = '\0';

	error = msg2host->errWord1;
	error = error << 16;
	error = error + msg2host->errWord0;

	alarm = msg2host->alarmWord1;
	alarm = alarm << 16;
	alarm = alarm + msg2host->alarmWord0;

	e_total = msg2host->totalOutputPwr0;
	e_total = e_total * 65536;
	e_total = e_total + msg2host->totalOutputPwr1;

	pac = msg2host->outputPower;
	pac = pac * 10; // in W unit
	
	ac_frequency = msg2host->AC_frequencyL1;
	ac_frequency = ac_frequency/10;

	ac_outcurr1 = msg2host->AC_outputCurrentL1;
	ac_outcurr1 = ac_outcurr1/10;

	ac_outcurr2 = msg2host->AC_outputCurrentL2;
	ac_outcurr2 = ac_outcurr2/10;

	ac_outcurr3 = msg2host->AC_outputCurrentL3;
	ac_outcurr3 = ac_outcurr3/10;	

	dc1_input_curr = msg2host->DC1inputCurr;
	dc1_input_curr = dc1_input_curr/10;

	dc2_input_curr = msg2host->DC2inputCurr;
	dc2_input_curr = dc2_input_curr/10;

	pdcA = msg2host->inputPwrA;
	pdcA = pdcA * 10; // in W unit

	pdcB = msg2host->inputPwrB;
	pdcB = pdcB * 10; // in W unit


	n = sprintf(buff, "%04d,%s,%s,%04d,%s,%08x,%08x,%.0f,%d,%d,%d,%2.1f,%2.1f,%2.1f,%2.1f,%d,%d,%.1f,%.1f,%d,%d,%d,%d,%.0f,%.0f,%.0f", 
		msg2host->inverterId, "ENERSOLIS", modelName, msg2host->inverterId, tmp_time, alarm, error, 
		pac, msg2host->AC_voltagePL1, msg2host->AC_voltagePL2, msg2host->AC_voltagePL3, ac_frequency,
		ac_outcurr1, ac_outcurr2, ac_outcurr3,
		msg2host->DC1inputVolt, msg2host->DC2inputVolt, dc1_input_curr, dc2_input_curr,
		msg2host->DCBusPosVolt, msg2host->DCBusNegVolt, msg2host->invIntlTemp, msg2host->invHeatSinkTemp, 
		pdcA, pdcB, e_total);

	buff[n] = '\0';	

	return 1; 
	
}


int inv_enersolis_state_chg_next(inv_enersolis_state_t* curr_inv_cmd_state, inv_cmd_event_t in_event){		//inv_ali_cmd_state_t
	
	switch (*curr_inv_cmd_state) {
		case INV_ENERSOLIS_STATE_INIT:
			if(!in_event)	// event normal
				*curr_inv_cmd_state = INV_ENERSOLIS_STATE_RDY;
			else
				*curr_inv_cmd_state = INV_ENERSOLIS_STATE_STOP;
		break;	
		case INV_ENERSOLIS_STATE_RDY:
			if(in_event == INV_CMD_EVENT_SCAN)
				*curr_inv_cmd_state = INV_ENERSOLIS_STATE_SCAN_RUN;
			else if(in_event == INV_CMD_EVENT_NORMAL)
				*curr_inv_cmd_state = INV_ENERSOLIS_STATE_C1;
			else if (in_event&(INV_CMD_EVENT_ERR|INV_CMD_EVENT_STOP))
				*curr_inv_cmd_state = INV_ENERSOLIS_STATE_STOP;
		break;
		case INV_ENERSOLIS_STATE_ERR:
			if(!in_event)
				*curr_inv_cmd_state = INV_ENERSOLIS_STATE_RDY;
			else
				*curr_inv_cmd_state = INV_ENERSOLIS_STATE_STOP;
		break;
		case INV_ENERSOLIS_STATE_STOP:
			if(!in_event)
				*curr_inv_cmd_state = INV_ENERSOLIS_STATE_RDY;
			else
				*curr_inv_cmd_state = INV_ENERSOLIS_STATE_STOP;
		break;
		case INV_ENERSOLIS_STATE_TIMEOUT:
			if(!in_event)
				*curr_inv_cmd_state = INV_ENERSOLIS_STATE_ERR;
			else
				*curr_inv_cmd_state = INV_ENERSOLIS_STATE_STOP;
		break;
		
		case INV_ENERSOLIS_STATE_C1:
		case INV_ENERSOLIS_STATE_C2:
		case INV_ENERSOLIS_STATE_C3:
		case INV_ENERSOLIS_STATE_C4:
		case INV_ENERSOLIS_STATE_C5:
			if (in_event == INV_CMD_EVENT_NOCHANGE)
				*curr_inv_cmd_state = *curr_inv_cmd_state;
			else if(in_event == INV_CMD_EVENT_NORMAL)	// event normal
				*curr_inv_cmd_state = *curr_inv_cmd_state + 1;
			else if (in_event&INV_CMD_EVENT_TIMEOUT)
				*curr_inv_cmd_state = INV_ENERSOLIS_STATE_TIMEOUT;
			else if (in_event&INV_CMD_EVENT_ERR)
				*curr_inv_cmd_state = INV_ENERSOLIS_STATE_ERR;
			else if (in_event&INV_CMD_EVENT_STOP)
				*curr_inv_cmd_state = INV_ENERSOLIS_STATE_STOP;
		break;
		
		case INV_ENERSOLIS_STATE_SCAN_RUN:
			if(in_event == INV_CMD_EVENT_STOP)
				*curr_inv_cmd_state = INV_ENERSOLIS_STATE_STOP;
			else if (in_event == INV_CMD_EVENT_END)
				*curr_inv_cmd_state = INV_ENERSOLIS_STATE_SCAN_END;
			else 
				*curr_inv_cmd_state = *curr_inv_cmd_state;
		break;
		
		case INV_ENERSOLIS_STATE_END:
			if(in_event == INV_CMD_EVENT_NORMAL)	// event normal
				*curr_inv_cmd_state = INV_ENERSOLIS_STATE_RDY;
			else if(in_event == INV_CMD_EVENT_END)
				*curr_inv_cmd_state = INV_ENERSOLIS_STATE_INIT;
			else
				*curr_inv_cmd_state = INV_ENERSOLIS_STATE_STOP;
		break;
		
		case INV_ENERSOLIS_STATE_SCAN_END:
			if(in_event == INV_CMD_EVENT_STOP)
				*curr_inv_cmd_state = INV_ENERSOLIS_STATE_STOP;
		
		default:;
	}
	
	return 1;
}


int getRegsMatrix_EnerSolis(inverter_enersolis_raw_t* ptr_base, inv_enersolis_state_t state, uint16_t* out_addr, uint16_t** out_ptr, uint8_t* out_size){
	uint32_t i;
	bool_t isFound=FALSE;

	for (i=0; i<(sizeof(enersolis_reg_def)/sizeof(inv_enersolis_regsmatrix_t)); i++){
		if(enersolis_reg_def[i].state_num == state){
			isFound = TRUE;
			*out_addr = enersolis_reg_def[i].reg_addr;
			*out_ptr = (uint16_t*)((uint32_t)ptr_base + (uint32_t)enersolis_reg_def[i].offset);//enersolis_reg_def[i].ptr_addr;
			*out_size = enersolis_reg_def[i].cpy_size;
			break;
		}
	}

	if (isFound)
		return 1;
	else{
		printf("State #%d no found in matrix\n", state);
		*out_addr = 0x0;
		*out_ptr = NULL;
		*out_size = 0;
		return -1;
	}
}


int invInfoGet_EnerSolis(uint16_t invAddr, inverter_enersolis_raw_t *data, char* scan_port_name){

  int fd_rs485;

  uint8_t rcv_buff[256];
  uint8_t *bufptr;

  raw_485_t raw_485_pkg;
  uint32_t req_size, nbytes, bytes;

  uint8_t rs232_timeout = FALSE;
  
  bool_t this_pkt_valid = TRUE;
  uint8_t * tmp_bufp, * data_basep;
  uint16_t inv_addr /*= 0x03*/, inv_reg2rd;
	
  inv_enersolis_state_t inv_cmd_state = INV_ENERSOLIS_STATE_INIT;
  uint8_t cpy_size;
  uint16_t * cpy_ptr=NULL;

  int net_timeout_cnt, rs485_timeout_cnt ,size_to_read;

  inverter_enersolis_raw_t enersolis_inverter_info_local;

  bzero(&enersolis_inverter_info_local, sizeof(inverter_enersolis_raw_t));

	// Initial UART
  if(!scan_port_name)
	return -1;  
  fd_rs485 = open_port_RS485(scan_port_name, O_RDWR| O_NOCTTY);
  
  if(fd_rs485 < 0){
  	printf("Open port %s failed !!", scan_port_name);
  	return -1;
  }   
	//printf("Current BaudRate=%d\n",getbaud(fd_rs485) );
  /***************************
  *  Get State
  ****************************/
	while(1){
		// get inv_reg2rd, cpy_ptr, cpy_size by current state
		if(getRegsMatrix_EnerSolis(&enersolis_inverter_info_local, inv_cmd_state, &inv_reg2rd, &cpy_ptr, &cpy_size)<=0){
			printf("state no found in enersolis protocol, inv_cmd_state=%d, goes to STOP state \n", inv_cmd_state);
			inv_enersolis_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_STOP);
		}

		// print current state
		printf("inv_cmd_state=%d\n", inv_cmd_state);
		

  /***************************
  *  Process Special States
  ****************************/		
		if (inv_cmd_state == INV_ENERSOLIS_STATE_INIT)	{
			//sleep(1);  // 20130611
			rs485_timeout_cnt = 0;
			this_pkt_valid = TRUE;
			inv_enersolis_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NORMAL);
		}
		else if (inv_cmd_state == INV_ENERSOLIS_STATE_RDY)
			inv_enersolis_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NORMAL);
		else if (inv_cmd_state == INV_ENERSOLIS_STATE_TIMEOUT)
			inv_enersolis_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_STOP);
		else if(inv_cmd_state == INV_ENERSOLIS_STATE_END)
		{
			
			printf("Dump meta data, invId=%d !!\n", invAddr);

			//inv_meta_data_dump(&inverter_info);
			
			if(this_pkt_valid)
				update_inv_msg2host_enersolis(data, &enersolis_inverter_info_local, invAddr);
			else{
				puts("This capture has error field, update this with ID=0");
				invAddr=0;
				update_inv_msg2host_enersolis(data, &enersolis_inverter_info_local, invAddr);
			}

			//sleep(1);
			if(!rs232_run_flag){
				inv_enersolis_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_STOP);
				//printf("To the end of Inverter probe, waiting for re-start semaphore!!\n");
			}
			else{
				inv_enersolis_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_END);
				printf("To the end of Inverter probe, automatically restart later !!\n");
			}
			
			// break the while loop
			break;
		}
		else if(inv_cmd_state == INV_ENERSOLIS_STATE_STOP)
		{
			printf("reach EnerSolis info_get stop state, update id = 0 to this slot !!\n");
			invAddr = 0;
			update_inv_msg2host_enersolis(data, &enersolis_inverter_info_local,invAddr);	

			break;
		}

  /***************************
  *  Normal States
  ****************************/		
		else if(inv_cmd_state&0xf0)
		{
			bzero(&raw_485_pkg, sizeof(raw_485_pkg));
			raw_485_pkg.pkt_type = mt81xx_cmd_read;
			// req_size is counted in word size, one word contains two bytes
			req_size = cpy_size >> 1;
			inv_addr = invAddr&0x00ff;
			
			// create data frame
			if(create_modbus_read(&raw_485_pkg.uniFrame.im_modbus_format_read, inv_addr, inv_reg2rd , req_size)<0)
				printf("Create Read Cmd to Rs485 failed!!\n");
			if(append_CRC16((modbus_format_read_t*)&raw_485_pkg.uniFrame.im_modbus_format_read.modbus_offset_SlaveAddr, 6)<0)
				printf("Adding CRC16 error !!\n");

  /***************************
  *  Send Read
  ****************************/			

			write(fd_rs485, &(raw_485_pkg.uniFrame.im_modbus_format_read), sizeof(raw_485_pkg.uniFrame.im_modbus_format_read));

  /***************************
  *  Waiting for Response
  ****************************/	  	
  			//process Rx
	  		bufptr = rcv_buff;
			nbytes=0;
			bzero(bufptr, 256);
			rs232_timeout=FALSE;
			// slave + fun + len + Data... + CRC_lo + CRC_Hi 
	  		size_to_read = req_size*2 + 5;
	  	
	  		net_timeout_cnt = 0;

	  		for(;;){ // do rs485 receiving during 3 second, or timeout
	  	  	// check if there's data in com port, or time out
				
				if(net_timeout_cnt==RETRY_TIMES_FOR_ONE_CMD/*isready(fd_rs485)==0*/){
	  				//rs232_timeout=TRUE;
	  				puts("Timeout !!");
	  				rs232_timeout=INV_MSG_CRASH;
				
	  				break;
	  			}
				
				usleep(WAITING_TIME_EACH_RETRY); 	  		
				
				// read it !!
	  	 		bytes = read(fd_rs485, (char*)bufptr, size_to_read);	// it will always receive

				nbytes +=bytes;
				bufptr += bytes;
				size_to_read -= bytes;
				// check satisfy

				//printf("#%d time(s) to read data, demand size=%d, bytes this time=%d, total bytes =%d, size to read=%d\n", net_timeout_cnt, (req_size*2 + 5), bytes, nbytes, size_to_read);

				if(size_to_read <= 0){
					rs232_timeout=FALSE;
					break;
				}
				
				net_timeout_cnt ++;
	  		}

  /***************************
  *  check received packet
  ****************************/
			if(rs232_timeout==FALSE){
				if(nbytes && (cpy_ptr!=NULL)){

					tmp_bufp = rcv_buff;

					if(check_CRC16((modbus_format_rtn_t *)tmp_bufp)>0){

						if( cpy_size == ((modbus_format_rtn_t*)tmp_bufp)->modbus_offset_Len ){
							
							data_basep = &(((modbus_format_rtn_t*)tmp_bufp)->modbus_offset_data_base);

							// switch bytes, if it is ASCII field, don't switch
							//if ((inv_cmd_state != INV_ALI_STATE_BN)&&(inv_cmd_state != INV_CMD_STATE_TN)&&(inv_cmd_state != INV_CMD_STATE_SN))
								switch_bytes( data_basep, cpy_size>>1);

							// store the received data into internal place
							memcpy(cpy_ptr, data_basep, cpy_size);
							
	  		  			}else
	  		  				printf("copy size mismatch, copy abort, cpy_ptr == %d\n", sizeof(cpy_ptr));
					
					}else{
						printf("receive RS485 packet with CRC error !!\n");	
						rs232_timeout=INV_MSG_CRC_ERR;
					}
				}else
					puts("cpy_ptr is NULL or nbyte is zero");

			}	
			
  /***************************
  *  Change State
  ****************************/
			if(rs232_timeout==INV_MSG_CRASH){
				if(rs485_timeout_cnt < CMD_REPEAT_TIMES_WHEN_FAILED){
					inv_enersolis_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NOCHANGE);
					puts("Try re-issue this cmd !!");
					rs485_timeout_cnt ++;
				}
				else{  // abort read this reg after rs485_timeout_cnt times of retries
					inv_enersolis_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_STOP);	
					puts("Abort reading because of no data respond to this command !!");
					rs485_timeout_cnt = 0;
					this_pkt_valid = FALSE;
				}
			}else if(rs232_timeout==INV_MSG_CRC_ERR){
				// clear rx buffer
				bytes = 1;
				sleep(1);
				while(bytes > 0){
					bytes = read(fd_rs485, (char*)rcv_buff, 32);
					puts("clear RS232 Rx buffer !!");
				}

				if(rs485_timeout_cnt < CMD_REPEAT_TIMES_WHEN_FAILED){
					inv_enersolis_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NOCHANGE);
					rs485_timeout_cnt ++;
				}
				else{  // abort read this reg after rs485_timeout_cnt times of retries
					inv_enersolis_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_STOP);	
					puts("Abort reading because of CRC error !!");
					rs485_timeout_cnt = 0;
					this_pkt_valid = FALSE;
				}
			}else if(rs232_timeout==FALSE){
    			inv_enersolis_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NORMAL);	
    			rs485_timeout_cnt = 0;
			}else{
					printf("RS485 timeout !!\n");
					inv_enersolis_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_TIMEOUT);
					this_pkt_valid = FALSE;
			}
		}
		else
			inv_enersolis_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NORMAL);
	}

	puts("exit get info enersolis !");
	close(fd_rs485);
	
	return 1;
}


int inverter_scan_enersolis(scan_inv_info_t* inv_info, char* scan_port_name){

  	int fd_rs485;
  //int fd_gpio88;

  	uint8_t rcv_buff[128];
  	uint8_t *bufptr;

  	int i;
	raw_485_t raw_485_pkg;
	uint32_t req_size, rsp_size, nbytes, bytes;

	const char invBnEnerSolis[10] = "EnerSolis";
	char weGetBrandName[10] = "";

	bool_t invIsEnerSolis;
  	uint8_t rs232_timeout = FALSE;
  	//uint8_t rcv_len;

  	uint32_t valid_cnt=0;

  	uint8_t * tmp_bufp;
	uint16_t inv_addr, inv_reg2rd;
	
	uint8_t cpy_size;
	inv_cmd_state_t inv_cmd_state = INV_CMD_STATE_INIT;
	
	//printf("In handle rs232 thread !!\n");

	//
	// Initial UART
	if(!scan_port_name)
		return -1;

  	fd_rs485 = open_port_RS485(scan_port_name, O_RDWR| O_NOCTTY);
    
    if(fd_rs485 < 0){
  		printf("Open port %s failed !!", scan_port_name);
  		return -1;
  	}
	//printf("Current BaudRate=%d\n",getbaud(fd_rs485) );
	
	while(1){

		switch (inv_cmd_state){

		case INV_CMD_STATE_INIT:
			inv_reg2rd = 0;
			inv_info->num = 0;
			inv_addr = INV_START_ADDR - 1;
		break;

		case INV_CMD_STATE_RDY:
			cpy_size = 0;
			inv_reg2rd = 0;
		break;
		
		case INV_CMD_STATE_TIMEOUT:
			cpy_size = 0;
			inv_reg2rd = 0;
		break;
		
		case INV_CMD_STATE_STOP:
			cpy_size = 0;
			inv_reg2rd = 0;
		break;
		
		case INV_CMD_STATE_SCAN_RUN:
			inv_reg2rd = 0xc0b0;
			inv_addr = inv_addr + 1;
			//cpy_ptr = tmp_char;
			req_size = 8;
			cpy_size = req_size * 2;
			invIsEnerSolis = FALSE;
		break;

		case INV_CMD_STATE_SCAN_END:
		//initial
			//cpy_ptr = NULL;
			cpy_size = 0;
			//sem_wait(&sem_rs232);	
		break;
		
		default:
		// display
			printf("default state, inv_cmd_state=%d\n", inv_cmd_state);
		// next
			inv_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_STOP);
		}	// switch case
	//
	// Does task, then update state
	// 
		
		if (inv_cmd_state == INV_CMD_STATE_INIT)	{
			//printf("RS232 Initial again !!\n");
			//sleep(1);
			inv_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NORMAL);
		}
		else if(inv_cmd_state == INV_CMD_STATE_RDY)
			inv_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_SCAN);
		else if (inv_cmd_state == INV_CMD_STATE_TIMEOUT){
				inv_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_STOP);
		}
		else if(inv_cmd_state == INV_CMD_STATE_SCAN_END)
		{
			puts("Finish Scan process!!\n");
			
			// dump message
			
			inv_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_STOP);
			printf("To the end of Inverter probe !!\n");
			
			// break the while
			break;
		}
		else if(inv_cmd_state == INV_CMD_STATE_STOP)
		{
			printf("reach EnerSolis SCAN stop state !!\n");
			close(fd_rs485);
			return 0;
			//inv_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NORMAL);
		}
		else if(inv_cmd_state&0xf00)
		{
			// check if it reach the end
			if(inv_addr > INV_MAX)
				inv_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_END);
			
			bzero(&raw_485_pkg, sizeof(raw_485_pkg));
			raw_485_pkg.pkt_type = mt81xx_cmd_read;	
			
			if(create_modbus_read(&raw_485_pkg.uniFrame.im_modbus_format_read, inv_addr, inv_reg2rd , req_size)<0)
				printf("Create Read Cmd to Rs485 failed!!\n");
			if(append_CRC16((modbus_format_read_t*)&raw_485_pkg.uniFrame.im_modbus_format_read.modbus_offset_SlaveAddr, 6)<0)
				printf("Adding CRC16 error !!\n");
/*
			if(create_mt81xx_cmd(&raw_485_pkg, inv_addr, inv_reg2rd , req_size)<0)
				printf("Create Read Cmd to Rs485 failed!!\n");
			if(append_motech_CRC16(&raw_485_pkg)<0)
				printf("Adding CRC16 error !!\n");
*/

    		printf("Scan EnerSolis inv# = %d on %s\n", inv_addr, scan_port_name);
    	
			i=0;

			i = sizeof(raw_485_pkg.uniFrame.im_modbus_format_read);
/*			
			while(((uint8_t*)&(raw_485_pkg.uniFrame.im_mt81xx_format03))[i] != 0x0d){
				//printf("%.2x ",((uint8_t*)&(raw_485_pkg.uniFrame.im_mt81xx_format03))[i]);
				i++;
			}
*/			
			//select Tx
			//write(fd_gpio88, "1", 1);
			//write(fd_rs485, &(raw_485_pkg.uniFrame.im_mt81xx_format03), i+1);

			write(fd_rs485, &(raw_485_pkg.uniFrame.im_modbus_format_read), i);
	  	
	  		usleep(10000);
	  	
			//process Rx
	  		bufptr = rcv_buff;
			nbytes=0;
			bzero(bufptr, 128);
			rs232_timeout=FALSE;
	  		rsp_size = req_size*2 + 5;
	  	
	  		int net_timeout_cnt = 0;
	  		//printf("rsp_size should be %d\n", rsp_size);
	  		for(;;){ // do rs485 receiving during 3 second, or timeout
	  	  	// check if there's data in com port, or time out
				
				if(net_timeout_cnt==RETRY_TIMES_FOR_ONE_SCAN/*isready(fd_rs485)==0*/){
	  				//rs232_timeout=TRUE;
	  				puts("Crash");
	  				rs232_timeout=INV_MSG_CRASH;
				
	  				break;
	  			}

				usleep(WAITING_TIME_EACH_SCAN_INV); //0.4 s	  		
				
				// read it !!
	  	 		bytes = read(fd_rs485, (char*)bufptr, rsp_size);	// it will always receive
	  	 		
				nbytes +=bytes;
				bufptr += bytes;
				// check satisfy

				if(bytes <= 0){		// get no data at all before timeout
	  				rs232_timeout=INV_MSG_CRASH;
	  				//puts("Scan Crash");
	  				break;
	  			}else if(rsp_size == nbytes){ // get exact data before timeout
					rs232_timeout=FALSE;
					break;
				}else{		// get partial data, keep watching !!
					rs232_timeout=FALSE;
					//break;
				}
				
				net_timeout_cnt ++;
	  		}

			valid_cnt=0;
			//rcv_len = 0;
			
			if(rs232_timeout==FALSE){
				if(nbytes){

					tmp_bufp = rcv_buff;
					if(check_CRC16((modbus_format_rtn_t *)tmp_bufp)>0){
						//printf("receive RS485 packet with CRC correct !!\n");
						
						int i;
						puts("");
						for(i=0;i<20;i++)
							printf("0x%x ", tmp_bufp[i]);
						puts("");
						// copy the data
						tmp_bufp += 2; // locate ptr to pointing "Byte Count"
						
						//printf("cpy_size = %d, *tmp_bufp = %d\n", cpy_size, *((uint8_t*)tmp_bufp));
						if( cpy_size == *((uint8_t*)tmp_bufp) ){
							tmp_bufp += 1;	// locate to data
							


							//tmp_bufp += 4;	// locate to Model name
							//printf("We get #%d data:%s\n", cpy_size, tmp_bufp);

							//tmp_bufp[sizeof(invBnEnerSolis)-1] = '\0';
							tmp_bufp[20] = '\0';
							printf("we get value=-%s-\n", tmp_bufp);
							//bzero(weGetBrandName, sizeof(weGetBrandName));
							//memcpy(weGetBrandName, tmp_bufp, strlen(invBnEnerSolis));

							
							//printf("strlen = %d", strlen(invBnEnerSolis));
							//invIsEnerSolis = TRUE;

							//printf("we get -%s-, we want -%s-,\n", weGetBrandName, invBnEnerSolis);
							//if(strcmp(invBnEnerSolis, (char*)tmp_bufp)==0)
							//if(strcmp(invBnEnerSolis, weGetBrandName)==0)
								invIsEnerSolis = TRUE;
							//else
							//	invIsEnerSolis = FALSE;
							

							// switch bytes
							// if it is ASCII field, don't switch
							//switch_bytes((uint8_t*)tmp_bufp, cpy_size>>1);
							
							//memcpy(cpy_ptr, tmp_bufp, cpy_size);
							//printf("Get data 0x%.4x\n", cpy_ptr[0]);
	  		  			}else
	  		  				printf("copy size mismatch, copy abort, data size in rtn is %d, cpy_size == %d\n", tmp_bufp[0], cpy_size);
	  		  		
							// modify count
							nbytes = valid_cnt;
							valid_cnt=0;					
					}else{
						printf("receive RS485 packet with CRC error !!\n");	
						
					}
				}
			}	

	  		if(rs232_timeout==INV_MSG_CRASH)
				puts("status CRASH !!");
			else if (rs232_timeout==FALSE)
				puts("status Not timeout !!");

			//change state
			if(rs232_timeout==INV_MSG_CRASH){
				inv_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NORMAL);
			}
    		else if(rs232_timeout==FALSE && invIsEnerSolis){
    			inv_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NORMAL);	
    			printf("EnerSolis inverter address=%d found on %s\n", inv_addr, scan_port_name);
    			inv_info->num = inv_info->num + 1;
    			inv_info->invInfo[inv_info->num -1].invId = inv_addr;
			}else{
				printf("RS485 timeout, This might not inverter of type EnerSolis !!\n");
				inv_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_TIMEOUT);
			}
		}
		else
			inv_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NORMAL);
		
	}

	puts("exit scan enersolis !");
	close(fd_rs485);

	return 1;
}
#endif


/****************************
** Digimeter Inverter protocol
****************************/
int inverter_scan_digimeter(scan_inv_info_t* inv_info, char* scan_port_name/*, uint16_t offset*/){

  	int fd_rs485;

  	uint8_t rcv_buff[128];
  	uint8_t *bufptr;

  	int i;
	raw_485_t raw_485_pkg;
	uint32_t req_size, rsp_size, bytes, nbytes;

	//const char invBnTough[6] = "TOUGH";
  	uint8_t rs232_timeout = FALSE;

  
  	int valid_cnt=0;

  	unsigned char * tmp_bufp;
	uint16_t inv_addr /*1 means start from 2*/, inv_reg2rd;//, actual_id=0; //, pyranometer_16_value=0;
	
	inv_digimeter_state_t inv_cmd_state = INV_DIGIMETER_STATE_INIT;
	uint8_t cpy_size;
  		
	int net_timeout_cnt = 0;

	bool_t invIsDigimeter = FALSE;

	//float test_reassemble_float=0;
	//uint16_t disp_value, lo_limit, hi_limit, digit;

	//
	// Initial UART
	if(!scan_port_name)
		return -1;

  	fd_rs485 = open_port_RS485(scan_port_name, O_RDWR| O_NOCTTY);

  	if(fd_rs485 < 0){
  		printf("Open port %s failed !!", scan_port_name);
  		return -1;
  	}

   
	//printf("Current BaudRate=%d\n",getbaud(fd_rs485) );
	
	while(1){

		// print current state
		//printf("inv_cmd_state=%d\n", inv_cmd_state);

		switch (inv_cmd_state){

		case INV_DIGIMETER_STATE_INIT:
			inv_reg2rd = 0;
			inv_info->num = 0;
			inv_addr = DIGIMETER_START - 1;
		break;

		case INV_DIGIMETER_STATE_RDY:
			cpy_size = 0;
			inv_reg2rd = 0;
		break;
		
		case INV_DIGIMETER_STATE_TIMEOUT:
			cpy_size = 0;
			inv_reg2rd = 0;
		break;
		
		case INV_DIGIMETER_STATE_STOP:
			cpy_size = 0;
			inv_reg2rd = 0;
		break;
		
		case INV_DIGIMETER_STATE_SCAN_RUN:
			inv_reg2rd = 0x0002;
			inv_addr = inv_addr + 1;
			req_size = 9;  // nine word
			cpy_size = req_size * 2;  // in bytes
		break;

		case INV_DIGIMETER_STATE_SCAN_END:
			cpy_size = 0;

		break;
		
		default:
		// display
			printf("default state, inv_cmd_state=%d\n", inv_cmd_state);
		// next
			inv_digimeter_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_STOP);
		}	// switch case
	//
	// Does task, then update state
	// 
		
		if (inv_cmd_state == INV_DIGIMETER_STATE_INIT)	{
			//printf("RS232 Initial again !!\n");
			//sleep(1);
			inv_digimeter_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NORMAL);
		}
		else if(inv_cmd_state == INV_DIGIMETER_STATE_RDY)
			inv_digimeter_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_SCAN);
		else if (inv_cmd_state == INV_DIGIMETER_STATE_TIMEOUT){
			inv_digimeter_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_STOP);
		}
		else if(inv_cmd_state == INV_DIGIMETER_STATE_SCAN_END)
		{
			puts("Finish Digimeter Scan process!!\n");
			
			// dump message
			
			inv_digimeter_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_STOP);
			printf("To the end of Inverter probe !!\n");
			
			// break the while
			break;
		}
		else if(inv_cmd_state == INV_DIGIMETER_STATE_STOP)
		{
			printf("reach Digimeter SCAN stop state !!\n");
			close(fd_rs485);
			return 0;

		}
		else if(inv_cmd_state&0xf00)
		{
			// check if it reach the end
			if(inv_addr > DIGIMETER_MAX - 1)
				inv_digimeter_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_END);

			// add this to clear any duplicate data
			clear_serial_rx_buffer(fd_rs485);
			
			bzero(&raw_485_pkg, sizeof(raw_485_pkg));
			raw_485_pkg.pkt_type = mt81xx_cmd_read;	
			
			if(create_modbus_read(&raw_485_pkg.uniFrame.im_modbus_format_read, inv_addr, inv_reg2rd , req_size)<0)
				printf("Create Read Cmd to Rs485 failed!!\n");
			if(append_CRC16((modbus_format_read_t*)&raw_485_pkg.uniFrame.im_modbus_format_read.modbus_offset_SlaveAddr, 6)<0)
				printf("Adding CRC16 error !!\n");
			//tmp_p = (uint8_t*)&(raw_485_pkg.uniFrame.im_modbus_format_read);
    		//printf("What we have created 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x \n",tmp_p[0], tmp_p[1], tmp_p[2], tmp_p[3], tmp_p[4], tmp_p[5], tmp_p[6],tmp_p[7]);
    		printf("Scan Digimeter inv# = %d on %s\n", inv_addr, scan_port_name);
    	
			i=0;

			i = sizeof(raw_485_pkg.uniFrame.im_modbus_format_read);

			//select Tx
			//write(fd_gpio88, "1", 1);
			write(fd_rs485, &(raw_485_pkg.uniFrame.im_modbus_format_read), i);
	  	
	  		usleep(WAITING_TIME_AFTER_ISSUE_CMD);
	  	
			//process Rx
	  		bufptr = rcv_buff;
			nbytes=0;
			bzero(bufptr, 128);
			rs232_timeout=FALSE;
	  		rsp_size = req_size*2 + 5;
            
	  		//printf("rsp_size should be %d\n", rsp_size);
	  		for(;;){ // do rs485 receiving during 3 second, or timeout
	  	  	// check if there's data in com port, or time out
				
				if(net_timeout_cnt==RETRY_TIMES_FOR_ONE_SCAN/*isready(fd_rs485)==0*/){
	  				//rs232_timeout=TRUE;
	  				puts("Crash");
	  				rs232_timeout=INV_MSG_CRASH;
				
	  				break;
	  			}
	  		
				// read it !!
                bytes = read(fd_rs485, (char*)bufptr, rsp_size);        // it will always receive

                nbytes +=bytes;
                bufptr += bytes;
                // check satisfy
        		if(bytes <= 0){         // get no data at all before timeout
                        rs232_timeout=INV_MSG_CRASH;
                        //puts("Scan Crash");
                        break;
                }else if(rsp_size == nbytes){ // get exact data before timeout
                        rs232_timeout=FALSE;
                        break;
                }else{          // get partial data, keep watching !!
                        rs232_timeout=FALSE;
                        //break;
                }
        		usleep(WAITING_TIME_EACH_SCAN_SENSOR); //0.4 s
                net_timeout_cnt ++;

	  		}

			valid_cnt=0;
			invIsDigimeter = FALSE;

			if(rs232_timeout==FALSE){
				if(nbytes){
					tmp_bufp = rcv_buff;
					
					if(check_CRC16((modbus_format_rtn_t *)tmp_bufp)>0){
						// copy the data
						tmp_bufp += 2; // locate ptr to pointing "Byte Count"
						
						if( cpy_size == *((uint8_t*)tmp_bufp) ){
							// check if data value equals to what we want
							tmp_bufp += 2;  // locate to low word
							if (tmp_bufp[0] == inv_addr)
								invIsDigimeter = TRUE;

							//actual_id = inv_addr + offset;
							if (inv_addr <= DIGIMETER_MAX){
								invIsDigimeter = TRUE;   // currently, we assume this is a valid digimeter upon correct response size
								/*
								if(offset != 0xffff){
								//	inverter_msg2host_digimeter_array[actual_id].inverterId = actual_id;
								//	strcpy(inverter_msg2host_digimeter_array[actual_id].value, curr_value);
								}else
									puts("In digimeter scanning, we only get list, but don't update slot data !!");
								*/
							}
	  		  			}else
	  		  				printf("copy size mismatch, copy abort, data size in rtn is %d, cpy_size == %d\n", tmp_bufp[0], cpy_size);
	  		  		
							// modify count
							nbytes = valid_cnt;
							valid_cnt=0;					
					}else{
						printf("receive RS485 packet with CRC error !!\n");	
						
					}
				}
			}
	  	
			//change state
			if(rs232_timeout==INV_MSG_CRASH){
				inv_digimeter_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NORMAL);
			}
    		else if(rs232_timeout==FALSE && invIsDigimeter){
    			inv_digimeter_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NORMAL);	
    			printf("Digimeter address=%d found on %s\n", inv_addr, scan_port_name);
    			inv_info->num = inv_info->num + 1;
    			inv_info->invInfo[inv_info->num -1].invId = inv_addr;
    			inv_info->invInfo[inv_info->num -1].invType = USE_DIGIMETER;			
			}else{
				printf("RS485 timeout !!\n");
				inv_digimeter_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_TIMEOUT);
			}
		}
		else
			inv_digimeter_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NORMAL);

		usleep(WAITING_TIME_EACH_SENSOR);
	}

	close(fd_rs485);
	return 1;
}

inverter_digimeter_raw_t digimeter_inverter_info;

inv_digimeter_regsmatrix_t digimeter_reg_def[10] = {
	{INV_DIGIMETER_STATE_INIT, 	0x0, 	0, 	0},
	{INV_DIGIMETER_STATE_RDY, 	0x0, 	0, 	0},
	{INV_DIGIMETER_STATE_TIMEOUT,	0x0,	0,	0},
	{INV_DIGIMETER_STATE_STOP, 	0x0,	0,	0},
	{INV_DIGIMETER_STATE_C1, 		0x0, 	(uint32_t)&(digimeter_inverter_info.digimeter_serial) - (uint32_t)&digimeter_inverter_info, 		24*2},
	{INV_DIGIMETER_STATE_C2, 		0x78, 	(uint32_t)&(digimeter_inverter_info.digimeter_freq) - (uint32_t)&digimeter_inverter_info, 	52*2},
	{INV_DIGIMETER_STATE_C3, 		0xac, 	(uint32_t)&(digimeter_inverter_info.digimeter_ch1whw) - (uint32_t)&digimeter_inverter_info, 	30*2},
	{INV_DIGIMETER_STATE_END, 	0x0, 	0, 0}
};


int update_inv_msg2host_digimeter(inverter_msg2host_digimeter_t *data, inverter_digimeter_raw_t *inverter_info, uint16_t invId){

	memcpy(data, inverter_info, sizeof(inverter_msg2host_digimeter_t));

	data->inverterId 			= invId;

	return 1;
}


int convert_invmsg_to_asci_digimeter(char *buff, inverter_msg2host_digimeter_t* msg2host){
	int n = 0;
	char tmp_time[15]="";
	char modelName[16]="N/A";//, companyName[11]="N/A";
	char serialName[16]="N/A";
	uint16_t error=0, type;
	uint16_t rs485_id=0, serVer=0;
	
	//double voltTimes, currTimes, wTimes, whTimes;
	double voltTimes_f, currTimes_f, wTimes_f, whTimes_f;
	short voltTimes_s, currTimes_s, wTimes_s, whTimes_s;

	double acvolt;
	double accurr;
	double freq=0;
	double pwrfc;
	double sumpwr;
	double sumpwrh;
	
	syscall_getRsp("date +%Y%m%d%H%M%S", tmp_time);
	tmp_time[14] = '\0';

	type = msg2host->digimeter_type;

	// modelName
	switch(type) {
		case 0:
			strcpy(modelName, "1P2W");
			break;
		case 1:
			strcpy(modelName, "1P3W");
			break;
		case 2:
			strcpy(modelName, "3P3W2V2A");
			break;		
		case 3:
			strcpy(modelName, "3P3W3V3A");
			break;			
		case 4:
			strcpy(modelName, "3P4W3V3A");
			break;			
		default:
			strcpy(modelName, "TypeUnknown");
	}

	// serialName
	rs485_id = msg2host->digimeter_addr;
	rs485_id = rs485_id >> 8;
	serVer = msg2host->digimeter_serial;

	sprintf(serialName, "%x_%04d", serVer, rs485_id);

	// voltage
	acvolt = msg2host->digimeter_sumvrmshw;
	acvolt = acvolt * 65536;
	acvolt = acvolt + msg2host->digimeter_sumvrmslw;

	//voltTimes = (double)msg2host->digimeter_voltexp_2;
	voltTimes_s = msg2host->digimeter_voltexp_2;
	//printf("voltTime = %d", voltTimes_s);
	if (voltTimes_s == 0)
		acvolt = acvolt;
	else if(voltTimes_s < 0){ 
		voltTimes_f = abs(voltTimes_s);
		voltTimes_f = pow(10,voltTimes_f);
		acvolt = acvolt / voltTimes_f;
	}
	else{
		voltTimes_f = abs(voltTimes_s);
		voltTimes_f = pow(10,voltTimes_f);
		acvolt = acvolt * voltTimes_f;
	}

	// current
	accurr = msg2host->digimeter_sumirmshw;
	accurr = accurr * 65536;
	accurr = accurr + msg2host->digimeter_sumirmslw;

	currTimes_s = msg2host->digimeter_currexp_2;
	//printf("currTime = %d\n", currTimes_s);
	if (currTimes_s == 0)
		accurr = accurr;
	else if(currTimes_s < 0){
		currTimes_f = abs(currTimes_s);
		currTimes_f = pow(10,currTimes_f);
		accurr = accurr / currTimes_f;
		//printf("accurr = %f\n", accurr);
	}
	else{
		currTimes_f = abs(currTimes_s);
		currTimes_f = pow(10,currTimes_f);
		accurr = accurr * currTimes_f;
	}

	// power
	sumpwr = msg2host->digimeter_sumwhw;
	sumpwr = sumpwr * 65536;
	sumpwr = sumpwr + msg2host->digimeter_sumwlw;

	wTimes_s = msg2host->digimeter_wattexp;
	if (wTimes_s == 0)
		sumpwr = sumpwr;
	else if(wTimes_s < 0){
		wTimes_f = abs(wTimes_s);
		wTimes_f = pow(10,wTimes_f);
		sumpwr = sumpwr / wTimes_f;
	}
	else{
		wTimes_f = abs(wTimes_s);
		wTimes_f = pow(10,wTimes_f);
		sumpwr = sumpwr * wTimes_f;
	}
	//	// make it from W to KW
	//sumpwr = sumpwr / 1000;

	// energy
	sumpwrh = msg2host->digimeter_poswhhw;
	sumpwrh = sumpwrh * 65536;
	sumpwrh = sumpwrh + msg2host->digimeter_poswhlw;

	whTimes_s = msg2host->digimeter_whexp;
	if (whTimes_s == 0)
		sumpwrh = sumpwrh;
	else if(whTimes_s < 0){
		whTimes_f = abs(whTimes_s);
		whTimes_f = pow(10,whTimes_f);
		sumpwrh = sumpwrh / whTimes_f;
	}
	else{
		whTimes_f = abs(whTimes_s);
		whTimes_f = pow(10,whTimes_f);
		sumpwrh = sumpwrh * whTimes_f;
	}
		// make it from WH to KWH
	sumpwrh = sumpwrh / 1000;

	// error
	error = msg2host->digimeter_status;
	
	// frequency
	freq = 0;
	
	// power factor
	pwrfc = (double)msg2host->digimeter_sumpf;
	pwrfc = pwrfc / 1000;

	n = sprintf(buff, "%04d,%s,%s,%s,%s,0x%x,%0.1f,%0.1f,%.2f,%0.2f,%.2f,%.2f", 
		msg2host->inverterId, "DIGIMETER", modelName, serialName, tmp_time, error,
		acvolt, accurr, freq, pwrfc, sumpwr, sumpwrh 
	);

	buff[n] = '\0';	
	return 1; 
}


int inv_digimeter_state_chg_next(inv_digimeter_state_t* curr_inv_cmd_state, inv_cmd_event_t in_event){		//inv_ali_cmd_state_t
	
	switch (*curr_inv_cmd_state) {
		case INV_DIGIMETER_STATE_INIT:
			if(!in_event)	// event normal
				*curr_inv_cmd_state = INV_DIGIMETER_STATE_RDY;
			else
				*curr_inv_cmd_state = INV_DIGIMETER_STATE_STOP;
		break;	
		case INV_DIGIMETER_STATE_RDY:
			if(in_event == INV_CMD_EVENT_SCAN)
				*curr_inv_cmd_state = INV_DIGIMETER_STATE_SCAN_RUN;
			else if(in_event == INV_CMD_EVENT_NORMAL)
				*curr_inv_cmd_state = INV_DIGIMETER_STATE_C1;
			else if (in_event&(INV_CMD_EVENT_ERR|INV_CMD_EVENT_STOP))
				*curr_inv_cmd_state = INV_DIGIMETER_STATE_STOP;
		break;
		case INV_DIGIMETER_STATE_ERR:
			if(!in_event)
				*curr_inv_cmd_state = INV_DIGIMETER_STATE_RDY;
			else
				*curr_inv_cmd_state = INV_DIGIMETER_STATE_STOP;
		break;
		case INV_DIGIMETER_STATE_STOP:
			if(!in_event)
				*curr_inv_cmd_state = INV_DIGIMETER_STATE_RDY;
			else
				*curr_inv_cmd_state = INV_DIGIMETER_STATE_STOP;
		break;
		case INV_DIGIMETER_STATE_TIMEOUT:
			if(!in_event)
				*curr_inv_cmd_state = INV_DIGIMETER_STATE_ERR;
			else
				*curr_inv_cmd_state = INV_DIGIMETER_STATE_STOP;
		break;
		
		case INV_DIGIMETER_STATE_C1:
		case INV_DIGIMETER_STATE_C2:
		case INV_DIGIMETER_STATE_C3:
		case INV_DIGIMETER_STATE_C4:
		case INV_DIGIMETER_STATE_C5:
			if (in_event == INV_CMD_EVENT_NOCHANGE)
				*curr_inv_cmd_state = *curr_inv_cmd_state;
			else if(in_event == INV_CMD_EVENT_NORMAL)	// event normal
				*curr_inv_cmd_state = *curr_inv_cmd_state + 1;
			else if (in_event&INV_CMD_EVENT_TIMEOUT)
				*curr_inv_cmd_state = INV_DIGIMETER_STATE_TIMEOUT;
			else if (in_event&INV_CMD_EVENT_ERR)
				*curr_inv_cmd_state = INV_DIGIMETER_STATE_ERR;
			else if (in_event&INV_CMD_EVENT_STOP)
				*curr_inv_cmd_state = INV_DIGIMETER_STATE_STOP;
		break;
		
		case INV_DIGIMETER_STATE_SCAN_RUN:
			if(in_event == INV_CMD_EVENT_STOP)
				*curr_inv_cmd_state = INV_DIGIMETER_STATE_STOP;
			else if (in_event == INV_CMD_EVENT_END)
				*curr_inv_cmd_state = INV_DIGIMETER_STATE_SCAN_END;
			else 
				*curr_inv_cmd_state = *curr_inv_cmd_state;
		break;
		
		case INV_DIGIMETER_STATE_END:
			if(in_event == INV_CMD_EVENT_NORMAL)	// event normal
				*curr_inv_cmd_state = INV_DIGIMETER_STATE_RDY;
			else if(in_event == INV_CMD_EVENT_END)
				*curr_inv_cmd_state = INV_DIGIMETER_STATE_INIT;
			else
				*curr_inv_cmd_state = INV_DIGIMETER_STATE_STOP;
		break;
		
		case INV_DIGIMETER_STATE_SCAN_END:
			if(in_event == INV_CMD_EVENT_STOP)
				*curr_inv_cmd_state = INV_DIGIMETER_STATE_STOP;
		
		default:;
	}
	
	return 1;
}


int getRegsMatrix_DigiMeter(inverter_digimeter_raw_t* ptr_base, inv_digimeter_state_t state, uint16_t* out_addr, uint16_t** out_ptr, uint8_t* out_size){
	uint32_t i;
	bool_t isFound=FALSE;

	for (i=0; i<(sizeof(digimeter_reg_def)/sizeof(inv_digimeter_regsmatrix_t)); i++){
		if(digimeter_reg_def[i].state_num == state){
			isFound = TRUE;
			*out_addr = digimeter_reg_def[i].reg_addr;
			*out_ptr = (uint16_t*)((uint32_t)ptr_base + (uint32_t)digimeter_reg_def[i].offset);//enersolis_reg_def[i].ptr_addr;
			*out_size = digimeter_reg_def[i].cpy_size;
			break;
		}
	}

	if (isFound)
		return 1;
	else{
		printf("State #%d no found in matrix\n", state);
		*out_addr = 0x0;
		*out_ptr = NULL;
		*out_size = 0;
		return -1;
	}
}


int invInfoGet_DigiMeter(uint16_t invAddr, inverter_msg2host_digimeter_t *data, char* scan_port_name){

  int fd_rs485;

  uint8_t rcv_buff[256];
  uint8_t *bufptr;

  raw_485_t raw_485_pkg;
  uint32_t req_size, nbytes, bytes;

  uint8_t rs232_timeout = FALSE;
  
  bool_t this_pkt_valid = TRUE;
  uint8_t * tmp_bufp, * data_basep;
  uint16_t inv_addr /*= 0x03*/, inv_reg2rd;
	
  inv_digimeter_state_t inv_cmd_state = INV_DIGIMETER_STATE_INIT;
  uint8_t cpy_size;
  uint16_t * cpy_ptr=NULL;

  int net_timeout_cnt, rs485_timeout_cnt ,size_to_read;

  inverter_digimeter_raw_t digimeter_inverter_info_local;

  bzero(&digimeter_inverter_info_local, sizeof(inverter_digimeter_raw_t));

	// Initial UART
  if(!scan_port_name)
	return -1;  
  fd_rs485 = open_port_RS485(scan_port_name, O_RDWR| O_NOCTTY);
  
  if(fd_rs485 < 0){
  	printf("Open port %s failed !!", scan_port_name);
  	return -1;
  }   
	//printf("Current BaudRate=%d\n",getbaud(fd_rs485) );
  /***************************
  *  Get State
  ****************************/
	while(1){
		// get inv_reg2rd, cpy_ptr, cpy_size by current state
		if(getRegsMatrix_DigiMeter(&digimeter_inverter_info_local, inv_cmd_state, &inv_reg2rd, &cpy_ptr, &cpy_size)<=0){
			printf("state no found in digimeter protocol, inv_cmd_state=%d, goes to STOP state \n", inv_cmd_state);
			inv_digimeter_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_STOP);
		}

		// print current state
		printf("inv_cmd_state=%d\n", inv_cmd_state);
		

  /***************************
  *  Process Special States
  ****************************/		
		if (inv_cmd_state == INV_DIGIMETER_STATE_INIT)	{
			//sleep(1);  // 20130611
			rs485_timeout_cnt = 0;
			this_pkt_valid = TRUE;
			inv_digimeter_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NORMAL);
		}
		else if (inv_cmd_state == INV_DIGIMETER_STATE_RDY)
			inv_digimeter_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NORMAL);
		else if (inv_cmd_state == INV_DIGIMETER_STATE_TIMEOUT)
			inv_digimeter_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_STOP);
		else if(inv_cmd_state == INV_DIGIMETER_STATE_END)
		{
			
			printf("Dump meta data, invId=%d !!\n", invAddr);

			//inv_meta_data_dump(&inverter_info);
			
			if(this_pkt_valid)
				update_inv_msg2host_digimeter(data, &digimeter_inverter_info_local, invAddr);
			else{
				puts("This capture has error field, update this with ID=0");
				invAddr=0;
				update_inv_msg2host_digimeter(data, &digimeter_inverter_info_local, invAddr);
			}

			//sleep(1);
			if(!rs232_run_flag){
				inv_digimeter_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_STOP);
				//printf("To the end of Inverter probe, waiting for re-start semaphore!!\n");
			}
			else{
				inv_digimeter_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_END);
				printf("To the end of Inverter probe, automatically restart later !!\n");
			}
			
			// break the while loop
			break;
		}
		else if(inv_cmd_state == INV_DIGIMETER_STATE_STOP)
		{
			printf("reach DigiMeter info_get stop state, update id = 0 to this slot !!\n");
			invAddr = 0;
			update_inv_msg2host_digimeter(data, &digimeter_inverter_info_local,invAddr);	

			break;
		}

  /***************************
  *  Normal States
  ****************************/		
		else if(inv_cmd_state&0xf0)
		{
			bzero(&raw_485_pkg, sizeof(raw_485_pkg));
			raw_485_pkg.pkt_type = mt81xx_cmd_read;
			// req_size is counted in word size, one word contains two bytes
			req_size = cpy_size >> 1;
			inv_addr = invAddr&0x00ff;
			
			// create data frame
			if(create_modbus_read(&raw_485_pkg.uniFrame.im_modbus_format_read, inv_addr, inv_reg2rd , req_size)<0)
				printf("Create Read Cmd to Rs485 failed!!\n");
			if(append_CRC16((modbus_format_read_t*)&raw_485_pkg.uniFrame.im_modbus_format_read.modbus_offset_SlaveAddr, 6)<0)
				printf("Adding CRC16 error !!\n");

  /***************************
  *  Send Read
  ****************************/			

			write(fd_rs485, &(raw_485_pkg.uniFrame.im_modbus_format_read), sizeof(raw_485_pkg.uniFrame.im_modbus_format_read));

			usleep(WAITING_TIME_AFTER_ISSUE_CMD);
  /***************************
  *  Waiting for Response
  ****************************/	  	
  			//process Rx
	  		bufptr = rcv_buff;
			nbytes=0;
			bzero(bufptr, 256);
			rs232_timeout=FALSE;
			// slave + fun + len + Data... + CRC_lo + CRC_Hi 
	  		size_to_read = req_size*2 + 5;
	  	
	  		net_timeout_cnt = 0;

	  		for(;;){ // do rs485 receiving during 3 second, or timeout
	  	  	// check if there's data in com port, or time out
				
				if(net_timeout_cnt==RETRY_TIMES_FOR_ONE_CMD/*isready(fd_rs485)==0*/){
	  				//rs232_timeout=TRUE;
	  				puts("Timeout !!");
	  				rs232_timeout=INV_MSG_CRASH;
				
	  				break;
	  			}
				
				usleep(WAITING_TIME_EACH_RETRY); 	  		
				
				// read it !!
	  	 		bytes = read(fd_rs485, (char*)bufptr, size_to_read);	// it will always receive

				nbytes +=bytes;
				bufptr += bytes;
				size_to_read -= bytes;
				// check satisfy

				//printf("#%d time(s) to read data, demand size=%d, bytes this time=%d, total bytes =%d, size to read=%d\n", net_timeout_cnt, (req_size*2 + 5), bytes, nbytes, size_to_read);

				if(size_to_read <= 0){
					rs232_timeout=FALSE;
					break;
				}
				
				net_timeout_cnt ++;
	  		}

  /***************************
  *  check received packet
  ****************************/
			if(rs232_timeout==FALSE){
				if(nbytes && (cpy_ptr!=NULL)){

					tmp_bufp = rcv_buff;

					if(check_CRC16((modbus_format_rtn_t *)tmp_bufp)>0){

						if( cpy_size == ((modbus_format_rtn_t*)tmp_bufp)->modbus_offset_Len ){
							
							data_basep = &(((modbus_format_rtn_t*)tmp_bufp)->modbus_offset_data_base);

							// switch bytes, if it is ASCII field, don't switch
							//if ((inv_cmd_state != INV_ALI_STATE_BN)&&(inv_cmd_state != INV_CMD_STATE_TN)&&(inv_cmd_state != INV_CMD_STATE_SN))
								switch_bytes( data_basep, cpy_size>>1);

							// store the received data into internal place
							memcpy(cpy_ptr, data_basep, cpy_size);
							
	  		  			}else
	  		  				printf("copy size mismatch, copy abort, cpy_ptr == %d\n", sizeof(cpy_ptr));
					
					}else{
						printf("receive RS485 packet with CRC error !!\n");	
						rs232_timeout=INV_MSG_CRC_ERR;
					}
				}else
					puts("cpy_ptr is NULL or nbyte is zero");

			}	
			
  /***************************
  *  Change State
  ****************************/
			if(rs232_timeout==INV_MSG_CRASH){
				if(rs485_timeout_cnt < CMD_REPEAT_TIMES_WHEN_FAILED){
					inv_digimeter_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NOCHANGE);
					puts("Try re-issue this cmd !!");
					rs485_timeout_cnt ++;
				}
				else{  // abort read this reg after rs485_timeout_cnt times of retries
					inv_digimeter_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_STOP);	
					puts("Abort reading because of no data respond to this command !!");
					rs485_timeout_cnt = 0;
					this_pkt_valid = FALSE;
				}
			}else if(rs232_timeout==INV_MSG_CRC_ERR){
				// clear rx buffer
				bytes = 1;
				sleep(1);
				while(bytes > 0){
					bytes = read(fd_rs485, (char*)rcv_buff, 32);
					puts("clear RS232 Rx buffer !!");
				}

				if(rs485_timeout_cnt < CMD_REPEAT_TIMES_WHEN_FAILED){
					inv_digimeter_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NOCHANGE);
					rs485_timeout_cnt ++;
				}
				else{  // abort read this reg after rs485_timeout_cnt times of retries
					inv_digimeter_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_STOP);	
					puts("Abort reading because of CRC error !!");
					rs485_timeout_cnt = 0;
					this_pkt_valid = FALSE;
				}
			}else if(rs232_timeout==FALSE){
    			inv_digimeter_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NORMAL);	
    			rs485_timeout_cnt = 0;
			}else{
					printf("RS485 timeout !!\n");
					inv_digimeter_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_TIMEOUT);
					this_pkt_valid = FALSE;
			}
		}
		else
			inv_digimeter_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NORMAL);
	}

	puts("exit get info digimeter !");
	close(fd_rs485);
	
	return 1;
}

#if 0
int inverter_scan_enersolis(scan_inv_info_t* inv_info, char* scan_port_name){

  	int fd_rs485;
  //int fd_gpio88;

  	uint8_t rcv_buff[128];
  	uint8_t *bufptr;

  	int i;
	raw_485_t raw_485_pkg;
	uint32_t req_size, rsp_size, nbytes, bytes;

	const uint8_t invBnEnerSolis[10] = "EnerSolis";
	bool_t invIsEnerSolis;
  	uint8_t rs232_timeout = FALSE;
  	//uint8_t rcv_len;
  
  	uint32_t valid_cnt=0;
 
  	uint8_t * tmp_bufp;
	uint16_t inv_addr, inv_reg2rd;
	
	uint8_t cpy_size;
	inv_cmd_state_t inv_cmd_state = INV_CMD_STATE_INIT;
	
	//printf("In handle rs232 thread !!\n");

	//
	// Initial UART
	if(!scan_port_name)
		return -1;

  	fd_rs485 = open_port_RS485(scan_port_name, O_RDWR| O_NOCTTY);
    
    if(fd_rs485 < 0){
  		printf("Open port %s failed !!", scan_port_name);
  		return -1;
  	}
	//printf("Current BaudRate=%d\n",getbaud(fd_rs485) );
	
	while(1){

		switch (inv_cmd_state){

		case INV_CMD_STATE_INIT:
			inv_reg2rd = 0;
			inv_info->num = 0;
			inv_addr = INV_START_ADDR - 1;
		break;

		case INV_CMD_STATE_RDY:
			cpy_size = 0;
			inv_reg2rd = 0;
		break;
		
		case INV_CMD_STATE_TIMEOUT:
			cpy_size = 0;
			inv_reg2rd = 0;
		break;
		
		case INV_CMD_STATE_STOP:
			cpy_size = 0;
			inv_reg2rd = 0;
		break;
		
		case INV_CMD_STATE_SCAN_RUN:
			inv_reg2rd = 0xc0b0;
			inv_addr = inv_addr + 1;
			//cpy_ptr = tmp_char;
			req_size = 8;
			cpy_size = req_size * 2;
			invIsEnerSolis = FALSE;
		break;

		case INV_CMD_STATE_SCAN_END:
		//initial
			//cpy_ptr = NULL;
			cpy_size = 0;
			//sem_wait(&sem_rs232);	
		break;
		
		default:
		// display
			printf("default state, inv_cmd_state=%d\n", inv_cmd_state);
		// next
			inv_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_STOP);
		}	// switch case
	//
	// Does task, then update state
	// 
		
		if (inv_cmd_state == INV_CMD_STATE_INIT)	{
			//printf("RS232 Initial again !!\n");
			//sleep(1);
			inv_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NORMAL);
		}
		else if(inv_cmd_state == INV_CMD_STATE_RDY)
			inv_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_SCAN);
		else if (inv_cmd_state == INV_CMD_STATE_TIMEOUT){
				inv_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_STOP);
		}
		else if(inv_cmd_state == INV_CMD_STATE_SCAN_END)
		{
			puts("Finish Scan process!!\n");
			
			// dump message
			
			inv_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_STOP);
			printf("To the end of Inverter probe !!\n");
			
			// break the while
			break;
		}
		else if(inv_cmd_state == INV_CMD_STATE_STOP)
		{
			printf("reach EnerSolis SCAN stop state !!\n");
			close(fd_rs485);
			return 0;
			//inv_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NORMAL);
		}
		else if(inv_cmd_state&0xf00)
		{
			// check if it reach the end
			if(inv_addr > INV_MAX)
				inv_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_END);
			
			bzero(&raw_485_pkg, sizeof(raw_485_pkg));
			raw_485_pkg.pkt_type = mt81xx_cmd_read;	
			
			if(create_modbus_read(&raw_485_pkg.uniFrame.im_modbus_format_read, inv_addr, inv_reg2rd , req_size)<0)
				printf("Create Read Cmd to Rs485 failed!!\n");
			if(append_CRC16((modbus_format_read_t*)&raw_485_pkg.uniFrame.im_modbus_format_read.modbus_offset_SlaveAddr, 6)<0)
				printf("Adding CRC16 error !!\n");
/*
			if(create_mt81xx_cmd(&raw_485_pkg, inv_addr, inv_reg2rd , req_size)<0)
				printf("Create Read Cmd to Rs485 failed!!\n");
			if(append_motech_CRC16(&raw_485_pkg)<0)
				printf("Adding CRC16 error !!\n");
*/

    		printf("Scan EnerSolis inv# = %d on %s\n", inv_addr, scan_port_name);
    	
			i=0;

			i = sizeof(raw_485_pkg.uniFrame.im_modbus_format_read);
/*			
			while(((uint8_t*)&(raw_485_pkg.uniFrame.im_mt81xx_format03))[i] != 0x0d){
				//printf("%.2x ",((uint8_t*)&(raw_485_pkg.uniFrame.im_mt81xx_format03))[i]);
				i++;
			}
*/			
			//select Tx
			//write(fd_gpio88, "1", 1);
			//write(fd_rs485, &(raw_485_pkg.uniFrame.im_mt81xx_format03), i+1);

			write(fd_rs485, &(raw_485_pkg.uniFrame.im_modbus_format_read), i);
	  	
	  		usleep(10000);
	  	
			//process Rx
	  		bufptr = rcv_buff;
			nbytes=0;
			bzero(bufptr, 128);
			rs232_timeout=FALSE;
	  		rsp_size = req_size*2 + 5;
	  	
	  		int net_timeout_cnt = 0;
	  		//printf("rsp_size should be %d\n", rsp_size);
	  		for(;;){ // do rs485 receiving during 3 second, or timeout
	  	  	// check if there's data in com port, or time out
				
				if(net_timeout_cnt==RETRY_TIMES_FOR_ONE_SCAN/*isready(fd_rs485)==0*/){
	  				//rs232_timeout=TRUE;
	  				puts("Crash");
	  				rs232_timeout=INV_MSG_CRASH;
				
	  				break;
	  			}

				usleep(WAITING_TIME_EACH_SCAN_INV); //0.4 s	  		
				
				// read it !!
	  	 		bytes = read(fd_rs485, (char*)bufptr, rsp_size);	// it will always receive
	  	 		
				nbytes +=bytes;
				bufptr += bytes;
				// check satisfy

				if(bytes <= 0){		// get no data at all before timeout
	  				rs232_timeout=INV_MSG_CRASH;
	  				//puts("Scan Crash");
	  				break;
	  			}else if(rsp_size == nbytes){ // get exact data before timeout
					rs232_timeout=FALSE;
					break;
				}else{		// get partial data, keep watching !!
					rs232_timeout=FALSE;
					//break;
				}
				
				net_timeout_cnt ++;
	  		}

			valid_cnt=0;
			//rcv_len = 0;
			
			if(rs232_timeout==FALSE){
				if(nbytes){

					tmp_bufp = rcv_buff;
					if(check_CRC16((modbus_format_rtn_t *)tmp_bufp)>0){
						//printf("receive RS485 packet with CRC correct !!\n");
						
						// copy the data
						tmp_bufp += 2; // locate ptr to pointing "Byte Count"
						
						//printf("cpy_size = %d, *tmp_bufp = %d\n", cpy_size, *((uint8_t*)tmp_bufp));
						if( cpy_size == *((uint8_t*)tmp_bufp) ){
							tmp_bufp += 1;	// locate to data
							
							tmp_bufp += 4;	// locate to Model name
							//printf("We get #%d data:%s\n", cpy_size, tmp_bufp);

							tmp_bufp[sizeof(invBnEnerSolis)-1] = '\0';

							//invIsEnerSolis = TRUE;

							
							if(strcmp((char*)invBnEnerSolis, (char*)tmp_bufp)==0)
								invIsEnerSolis = TRUE;
							else
								invIsEnerSolis = FALSE;
							

							// switch bytes
							// if it is ASCII field, don't switch
							//switch_bytes((uint8_t*)tmp_bufp, cpy_size>>1);
							
							//memcpy(cpy_ptr, tmp_bufp, cpy_size);
							//printf("Get data 0x%.4x\n", cpy_ptr[0]);
	  		  			}else
	  		  				printf("copy size mismatch, copy abort, data size in rtn is %d, cpy_size == %d\n", tmp_bufp[0], cpy_size);
	  		  		
							// modify count
							nbytes = valid_cnt;
							valid_cnt=0;					
					}else{
						printf("receive RS485 packet with CRC error !!\n");	
						
					}
				}
			}	
	  	
			//change state
			if(rs232_timeout==INV_MSG_CRASH){
				inv_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NORMAL);
			}
    		else if(rs232_timeout==FALSE && invIsEnerSolis){
    			inv_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NORMAL);	
    			printf("EnerSolis inverter address=%d found on %s\n", inv_addr, scan_port_name);
    			inv_info->num = inv_info->num + 1;
    			inv_info->invInfo[inv_info->num -1].invId = inv_addr;
			}else{
				printf("RS485 timeout !!\n");
				inv_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_TIMEOUT);
			}
		}
		else
			inv_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NORMAL);
		
	}

	puts("exit scan enersolis !");
	close(fd_rs485);

	return 1;
}
#endif 

