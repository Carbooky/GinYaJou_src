#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <fcntl.h>
#include <termios.h>

#include "digimeter.h"

uint8_t rs232_run_flag = 0;


int open_port_RS485(char* device_node, int mode){
  int fd_rs485;
  struct termios options;

  if(!device_node)
  	return -1;

  fd_rs485 = open(device_node, mode);

  if(fd_rs485 == -1)
    {
      perror("open_port: Unable to open:");
      return -1;
    }
    else
    {
      fcntl(fd_rs485, F_SETFL, 0);
    }
  
  tcgetattr(fd_rs485, &options);

  bzero(&options, sizeof(options));
    
  options.c_cflag = INVERTER_BAUDRATE |CLOCAL | CREAD;//(CLOCAL | CREAD);
  options.c_cflag &= ~PARENB;//|= ~PARENB;//&= ~PARENB;
  options.c_cflag &= ~CSTOPB;//options.c_cflag &= ~CSTOPB;
  options.c_cflag |= CS8;
    
  options.c_iflag = IGNPAR;
  options.c_oflag &= ~OPOST;
  options.c_lflag &= ~(ICANON|ECHO|ECHOE|ISIG);
  options.c_cc[VMIN] = 0;		// read() return after receiving VMIN of 
								// words, it works
  options.c_cc[VTIME] = 4;

  tcflush(fd_rs485, TCIFLUSH);
  tcsetattr(fd_rs485, TCSANOW, &options);
  
  return fd_rs485;
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

void switch_bytes(uint8_t* data_ptr, int len){
	uint8_t tmp_byte;
	int i;
	
	for(i=0; i < (len<<1); i=i+2){
		tmp_byte = data_ptr[i+0];
		data_ptr[i+0] = data_ptr[i+1];
		data_ptr[i+1] = tmp_byte;
	}
}

int syscall_getRsp(const char* command, char* returnString) {
  FILE *fp;
  //int status;
  char path[1024];

  /* Open the command for reading. */
  fp = popen(command, "r");
  if (fp == NULL) {
    printf("Failed to run command\n" );
    return -1;
  }

  /* Read the output a line at a time - output it. */
  while (fgets(path, sizeof(path)-1, fp) != NULL) {
    strcpy(returnString, path);
  }

  /* close */
  pclose(fp);
  
  return 1;
}


/****************************
** Digimeter Inverter protocol
****************************/

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


int update_inv_msg2host_digimeter(inverter_digimeter_raw_t *data, inverter_digimeter_raw_t *inverter_info, uint16_t invId){

	memcpy(data, inverter_info, sizeof(inverter_digimeter_raw_t));

	data->inverterId 			= invId;

	return 1;
}


int convert_invmsg_to_asci_digimeter(char *buff, inverter_digimeter_raw_t* msg2host){
	int n = 0;
	char tmp_time[15]="";
	char modelName[16]="N/A";//, companyName[11]="N/A";
	char serialName[16]="N/A";
	uint16_t error=0, type, serial, addr;
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

	serial = msg2host->digimeter_serial;
	type = msg2host->digimeter_type;
	addr = msg2host->digimeter_addr;

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
		// make it from W to KW
	sumpwr = sumpwr / 1000;

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


int invInfoGet_DigiMeter(uint16_t invAddr, inverter_digimeter_raw_t *data, char* scan_port_name){

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

int get_digimeter_value_dummy(char* out_buff, int size_of_buff){
	inverter_digimeter_raw_t dummy_info;
	char buff[256];
	int allowed_size=0;

	bzero(&dummy_info, sizeof(inverter_digimeter_raw_t));
	bzero (buff, 256);

	srand(time(NULL));

	dummy_info.digimeter_serial = 0x1235;
	dummy_info.digimeter_type = 3;
	dummy_info.digimeter_addr = 0x0204;
	dummy_info.digimeter_status = 0x8000;
	dummy_info.digimeter_voltexp_2 = 0xffff;
	dummy_info.digimeter_currexp_2 = 0xffff;
	dummy_info.digimeter_wattexp = 1;
	dummy_info.digimeter_whexp = 1;
	dummy_info.digimeter_sumpf = rand()%500 + 500;
	dummy_info.digimeter_sumvrmshw = 0;
	dummy_info.digimeter_sumvrmslw =rand()%200 + 2000;
	dummy_info.digimeter_sumirmshw = 0;
	dummy_info.digimeter_sumirmslw =rand()%30 + 50;
	dummy_info.digimeter_sumwhw = 0;
	dummy_info.digimeter_sumwlw = rand()%300 + 200;
	dummy_info.digimeter_poswhhw = rand()%40;
	dummy_info.digimeter_poswhlw = rand()%65535;

	convert_invmsg_to_asci_digimeter(buff, &dummy_info);

	if (strlen(buff) > size_of_buff){
		memcpy(out_buff, buff, size_of_buff);
		return -1;
	}
	else
		memcpy(out_buff, buff, strlen(buff));

	return 1;
}

int get_digimeter_value(char* out_buff, int size_of_buff, char* interface){
	inverter_digimeter_raw_t tmp_info;
	char buff[1024];  		
	
	bzero(&tmp_info, sizeof(inverter_digimeter_raw_t));

  	invInfoGet_DigiMeter(2, &tmp_info, interface);
  	convert_invmsg_to_asci_digimeter(buff, &tmp_info);

	if (strlen(buff) > size_of_buff){
		memcpy(out_buff, buff, size_of_buff);
		return -1;
	}
	else
		memcpy(out_buff, buff, strlen(buff));

}

#if 0
int main(void){
  char rtn[1024];
  

  bzero(rtn, 1024);
  

  //get_digimeter_value_dummy(cmd, 1024);
  get_digimeter_value(rtn, 1024, "/dev/ttyO2");


  printf("we get rtn:%s \n", rtn);

  return 1;
}


#endif