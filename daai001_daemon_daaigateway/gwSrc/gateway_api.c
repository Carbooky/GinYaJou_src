#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <semaphore.h>
#include <netinet/in.h>
#include <sys/wait.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <linux/netdevice.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/stat.h>
//#include "libctools/transfer_utils.h"

#include <libxml/parser.h>

#include "gateway_api.h"

//#include "mt81xx.h"
#include "util.h"
#include "protocols.h"

//#include "ps_client.h"
//#include "WBQueue.h"

#include "cam_capture.h"

#define PLATFORM_PC 1
#define PLATFORM_TARGET 0

#define GPIO_WRITABLE_BIT 0x0f
#define PLATFORM PLATFORM_PC
//#define PLATFORM PLATFORM_PC

#define PORT_SRV 30000
#define LENGTH 512
#define BACKLOG 5

#define INV_MSG_CRASH 2
#define INV_MSG_CRC_ERR 3

/********************
* global defines  *
*********************/
//#define MODEMDEVICE "/dev/ttyUSB0"
//#define INVERTER_BAUDRATE B9600
#define XML_FILE_LOCATION "./Profiles/TR106_BaselineProfile.xml"
#define TRUE 1
#define FALSE 0
#define bool_t _Bool

#define PKT_REAL 1

#define PLATFORMISPC 0

#define GPIO_WIRTABLE_BIT 0x07

#define ALARM_BIT 					0
#define INV_AUTO_BIT 				1
#define INV_FORCE_BIT				2
#define SYSTEM_REBOOT_BIT 	3
#define UPDATE_PERIOD_LBIT	4
#define UPDATE_PERIOD_HBIT	5
#define SYSTEM_STATUS_LBIT  6
#define SYSTEM_STATUS_HBIT  7

#define STATUS_ALARM_IND_ENABLE 			(1 << ALARM_BIT)
#define STATUS_INV_AUTO_SCAN_ENABLE 	(1 << INV_AUTO_BIT)
#define STATUS_INV_FORCE_SCAN 				(1 << INV_FORCE_BIT)
#define STATUS_SYSTEM_REBOOT_BIT 			(1 << SYSTEM_REBOOT_BIT)
#define STATUS_UPDATE_PERIOD_2BIT 		((1 << UPDATE_PERIOD_LBIT) + (1 << UPDATE_PERIOD_HBIT))
#define STATUS_SYSTEM_2BIT 						((1 << SYSTEM_STATUS_LBIT) + (1 << SYSTEM_STATUS_HBIT))

int test_trigger=0;

//inverter_info_t inverter_info;
inverter_msg2host_t msg2host;
sem_t sem_rs232, sem_invScan, sem_waitInvScanFin, sem_alarm_stop, sem_alarm_start;
uint8_t rs232_run_flag = 0;

//msg2cloudServer_t msg2cloudServer;


char sys_MacAddr[18];
char sys_IP[16];

//char string_inv_info[67];  //num + 32addrs +1
char sys_time[13];		//[MM][dd][hh][mm][yyyy] + 1
//gGpioValue_t sys_GPIO;
char alarm_enable = 0;
char sys_ip_addr[16];
bool_t sys_TimeStampEnable = TRUE;
unsigned int sys_video = 2;
unsigned int sys_audio = 2;
volatile char sys_status = STATUS_ALARM_IND_ENABLE | STATUS_INV_AUTO_SCAN_ENABLE | 0x00 | 0x40;

bool_t status_alarm_snapshot_enable = TRUE;

volatile int update_acs_interval = 1;
gGpioValue_t get_Gpiovalue = 0;

//void (*alarm_func)(gGpioValue_t alarmGpio);
alarm_func_t alarm_func = NULL;
audio_func_t audio_func;
ind_func_t xml_func_ind = NULL;
ind_func_t xml_func_update = NULL;
//upload_func_t xml_func_upload = NULL;

pthread_t thread_update_scan, thread_senpu_manager, thread_gpio, thread_alarm, thread_timer, thread_inverter_manager;

/***************
*	Internal APIs
****************/
int set_sys_time(char * new_dateTime);
int reg_audCallBack( audio_func_t reg_func);
int reg_indCallBack( alarm_func_t reg_func);

void *thread_handle_update_scan(void *argu);
void *thread_handle_manager_senpu(void *argu);
void *thread_handle_manager_inverter(void *argu);
void *thread_handle_gpio(void *argu);
void *thread_handle_alarm(void *argu);
void *thread_handle_timer(void *argu);

typedef enum{
	ALARM_STATE_INIT = 0,
	ALARM_STATE_RDY,
	ALARM_STATE_TRIGGERED,
	ALARM_STATE_EXPIRED,
	ALARM_STATE_WEB_STOP_PRE,
	ALARM_STATE_WEB_STOP
}trigger_state_t;

trigger_state_t alarm_state = ALARM_STATE_INIT;

typedef enum{
	ALARM_EVENT_TRIGGER = 1,
	ALARM_EVENT_UNTRIGGERED,
	ALARM_EVENT_TIMEOUT,
	ALARM_EVENT_WEBSTOP,
	ALARM_EVENT_ADV
}trigger_event_t;

int countdown_secs = 0;


#define COUNTDOWN_WEBSTOP2RESUME 180
#define COUNTDOWN_EXPIRE2READY 	 600
#define COUNTDOWN_TRIGGER2EXPIRE 600

int alarm_state_change(trigger_state_t *curr_state, trigger_event_t event){
	
	printf("Event = %d, Alarm State =%d -->", event, *curr_state);

	switch(*curr_state){
		case ALARM_STATE_INIT:
			if(event == ALARM_EVENT_ADV)
				*curr_state = ALARM_STATE_RDY;
			break;
		case ALARM_STATE_RDY:
			if(event == ALARM_EVENT_TRIGGER)
				*curr_state = ALARM_STATE_TRIGGERED;
			break;
		case ALARM_STATE_TRIGGERED:
			if(event == ALARM_EVENT_TIMEOUT)
				*curr_state = ALARM_STATE_EXPIRED;
			else if (event == ALARM_EVENT_WEBSTOP)
				*curr_state = ALARM_STATE_WEB_STOP_PRE;
			else if (event == ALARM_EVENT_UNTRIGGERED)
				*curr_state = ALARM_STATE_RDY;
			break;
		case ALARM_STATE_EXPIRED:
			if(event == ALARM_EVENT_WEBSTOP)
				*curr_state = ALARM_STATE_WEB_STOP_PRE;
			else if(event == ALARM_EVENT_TIMEOUT)
				*curr_state = ALARM_STATE_RDY;
			break;
		case ALARM_STATE_WEB_STOP_PRE:
			if(event == ALARM_EVENT_ADV)
				*curr_state = ALARM_STATE_WEB_STOP;
			break;
		case ALARM_STATE_WEB_STOP:
			if(event == ALARM_EVENT_TIMEOUT)
				*curr_state = ALARM_STATE_RDY;
			break;
		default:;
	}

	printf("%d \n", *curr_state);

	return 1;
}
/*********** END Internal APIs **************/

#define SERVER_IP_PORT "http://csac.dtdns.net:8084/"
#define IMAGEDIR "upload/"

#define USE_REAL_CAPTURE_UPLOAD 0

bool_t senpu_CamCapture(char* gwid){
#if USE_REAL_CAPTURE_UPLOAD
	int n = 5;
	char tmp_time[15]="";
	bool_t bret;

	char* targetFileName[5];
	char targetFileName1[256] = "";
	char targetFileName2[256] = "";
	char targetFileName3[256] = "";
	char targetFileName4[256] = "";
	char targetFileName5[256] = "";

	targetFileName[0] = targetFileName1;
	targetFileName[1] = targetFileName2;
	targetFileName[2] = targetFileName3;
	targetFileName[3] = targetFileName4;
	targetFileName[4] = targetFileName5;

	char strUploadPath[256] = "";
	//char targetFileName[256] = "";
	char full_filename[256] = "";
	int k;
	int uploadResult = -1;

	// first stop the streaming video
	setStreamVideo("0");
	//sleep(1);
	
	while(n>0){
    	syscall_getRsp("date +\"%Y%m%d%H%M%S\"", tmp_time);
    	tmp_time[14] = '\0';
		
    	//sprintf(targetFileName,"image_%llu.jpg", Time_getCurrentMicrosecond());

	printf("This time n = %d\n", n);

    	switch(n){
    	case 5:
    		sprintf(targetFileName[4],"%s.jpg", tmp_time);
    		break;
    	case 4:
    		sprintf(targetFileName[3],"%s.jpg", tmp_time);
    		break;
    	case 3:
    		sprintf(targetFileName[2],"%s.jpg", tmp_time);
    		break;
    	case 2:
    		sprintf(targetFileName[1],"%s.jpg", tmp_time);
    		break;
    	case 1:
    		sprintf(targetFileName[0],"%s.jpg", tmp_time);
    		break;
    		default:;
    	}

	
	printf("after camcapture,slot =%d,  name = %s, IMAGEDIR=%s\n", n-1 ,targetFileName[n-1], IMAGEDIR);
    	bret = CamCapture_startCapture(IMAGEDIR, targetFileName[n-1], "/dev/video0");
	printf("after camcapture,slot =%d,  name = %s\n", n-1 ,targetFileName[n-1]);

    	sleep(4);
    	n--;
	}

	//sleep(1);
	puts("start to upload!");

		if (bret == TRUE) {
			sprintf(strUploadPath, "%sSolarEnergy/upload_gateway_picture?id=%s&filename=%s", SERVER_IP_PORT, gwid, targetFileName[n-1]); // servlet URL for Dora, jcwang 20130807
			printf("Test show the upload string:%s\n", strUploadPath);

			for (k = 0; k < 3; k++)  // re-send 3 times
					{
				uploadResult = Transfer_uploadFile_multi(IMAGEDIR, targetFileName, 5, strUploadPath, "", "");

				if (uploadResult == 0)
					break;
			}
		}
	
	
			// 20130731 jcwang
			// whatever to delete image file
			struct stat stFileInfo;

			int intStat;
			
	n=5;
	while(n>0){
			sprintf(full_filename, "%s/%s", IMAGEDIR, targetFileName[n-1]);
			intStat = stat(full_filename, &stFileInfo);
			if (intStat == 0) {
				printf("File Exist!\n");
				if (stFileInfo.st_mode & S_IFREG) {
					printf("Input is regular file. %s\n", full_filename);
					// to delete
					remove(full_filename);
					printf("finish remove : %s", full_filename);
				} else if (stFileInfo.st_mode & S_IFDIR) {

				}
			}
	
		usleep(100000);
		n--;
	}
	puts("Finish photo upload!");

	//sleep(1);
	// restart the streaming video
	setStreamVideo("1");

    return bret;
#else
    return TRUE;
#endif
}

int isready_net(int fd)
{
	int rc;
  fd_set fds;
  struct timeval tv;

  FD_ZERO(&fds);
  FD_SET(fd,&fds);
  tv.tv_sec = 1;
  tv.tv_usec = 0;

  	rc = select(fd+1,&fds,NULL,NULL,&tv);
		printf("select rtn %d \n", rc);

  if(rc < 0)
     return FD_ISSET(fd,&fds) ? 1 : 0;
  else if(rc==0)
  	 return 0;
  
  return -1;
}


int senpu_GPIOSet(gGpioValue_t gpioValue){
	
	if(gpioValue == 1){
		system("sudo ~/gpio/setGio16 1");
		//puts("get gio==1");
	}
	else{
		system("sudo ~/gpio/setGio16 0");
		//puts("get gio==0");
	}
	
	return 1;
}

int senpu_GPIOGet(gGpioValue_t *gpioValue){
	char tmpValue[2];
	char buff_syscall[128];
	
	//strcpy((char*)gpioValue, exec("cat /sys/class/gpio/gpio11/value"));
	syscall_getRsp("sudo /home/pi/gpio/getGio15", buff_syscall);
	strcpy(tmpValue, buff_syscall);
	
	*gpioValue = tmpValue[0] - '0';
	
	return 1;
}


void print_AcsMenu(void){
	

	printf("[1].(string)getGPIO\n");
	printf("[2].(string)getEnergy(index)\n");
	printf("[3].(string)getSysIp\n");
	printf("[4].(string)getSysMac\n");
	printf("[5].(string)getSysInv\n");
	//printf("[6].(string)getSysTimeStampEnable\n");

	printf("[7].(string)getAudio\n");
	printf("[8].(string)getStreamVideo\n");
	//printf("[9].(string)getSysTime\n");
	printf("[10].(string)getSysStatus\n");
	
	printf("[11].(int)setSysInit(string)\n");
	printf("[12].(int)setSysDispose\n");
		
	printf("[13].setGPIO(string)\n");
	//printf(" \n");
	//printf("[15].setSnapShot(string)\n");
	printf("[16].setAudio(string)\n");
	printf("[17].setStreamVideo(string)\n");
	//printf("[18].setSysTime(string)\n");
	printf("[19].setSysCtrl(string)\n");
	//printf("[20].(string)getEnergy_instant(index)\n");
	//printf("[21].(string)getSysInv_instant\n");

}

int senpu_SysCtrlSet(gSysCmd_t cmd, char *params){
	
	switch(cmd){
		case SYS_SET_TIME:
			set_sys_time(params);
			break;
		case SYS_SET_IND_CALLBACK:
			reg_indCallBack((alarm_func_t)params);
			break;
		case SYS_SET_AUD_CALLBACK:
			reg_audCallBack((audio_func_t)params);
			break;
		default:;
	}
	return 1;
}


int senpu_SnapShotSet(gSnapShot_t cmd, void *params){
	//char file_name[10] = {'p','i','c','t','u','r','e'};
	//char value=0;
	(void) cmd;
	(void) params;
	
	// do snap shot
	
	return 1;
}


#if 0
int inverter_scan(scan_inv_info_t* inv_info){
	struct termios options;

  int fd_rs485;
  //int fd_gpio88;

  int bytes;


  uint8_t rcv_buff[64];
  uint8_t *bufptr;

  int nbytes;

  int i;
	raw_485_t raw_485_pkg;
	int req_size, rsp_size;


  uint8_t rs232_timeout = FALSE;
  uint8_t rcv_len;
  
  int tmp_i, valid_cnt=0;
  bool_t tail_valid=FALSE, isASCII=FALSE;
  uint8_t * tmp_bufp;
	
	uint16_t inv_addr=0 /*= 0x03*/, inv_reg2rd;
	
	inv_cmd_state_t inv_cmd_state = INV_CMD_STATE_INIT;
	uint8_t cpy_size;
	uint16_t * cpy_ptr=NULL;
	
	//int net_timeout_cnt;
	
	printf("In handle rs232 thread !!\n");

	//
	// Initial UART
  fd_rs485 = open(MODEMDEVICE, O_RDWR| O_NOCTTY);
  if(fd_rs485 == -1)
  {
      perror("open_port: Unable to open:");
  }
  else
  {
      fcntl(fd_rs485, F_SETFL, 0);
      printf("hi again\n");
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
	options.c_cc[VTIME] = 1;

	tcflush(fd_rs485, TCIFLUSH);
  tcsetattr(fd_rs485, TCSANOW, &options);
   
	//printf("Current BaudRate=%d\n",getbaud(fd_rs485) );
	
	while(1){

		switch (inv_cmd_state){

		case INV_CMD_STATE_INIT:
			puts("Rs232 State at initial");
			inv_reg2rd = 0;
			isASCII=FALSE;
			inv_info->num = 0;
		break;

		case INV_CMD_STATE_RDY:
			cpy_size = 0;
			inv_reg2rd = 0;
			isASCII=FALSE;
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
			inv_reg2rd = 0x17;
			inv_addr = inv_addr + 1;
			isASCII=FALSE;
		break;

		case INV_CMD_STATE_SCAN_END:
		//initial
			cpy_ptr = NULL;
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
			printf("Scan STOP !!\n");
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
			req_size = 2;
			if(create_mt81xx_cmd(&raw_485_pkg, inv_addr, inv_reg2rd , req_size)<0)
				printf("Create Read Cmd to Rs485 failed!!\n");
			if(append_CRC16(&raw_485_pkg)<0)
				printf("Adding CRC16 error !!\n");
    	
    	printf("Scan inv# = %d\n", inv_addr);
    	
			i=0;

			while(((uint8_t*)&(raw_485_pkg.uniFrame.im_mt81xx_format03))[i] != 0x0d){
				//printf("%.2x ",((uint8_t*)&(raw_485_pkg.uniFrame.im_mt81xx_format03))[i]);
				i++;
			}
			
			//select Tx
			//write(fd_gpio88, "1", 1);
			write(fd_rs485, &(raw_485_pkg.uniFrame.im_mt81xx_format03), i+1);
	  	
	  	usleep(100);
	  	
			//process Rx
	  	bufptr = rcv_buff;
			nbytes=0;
			bzero(bufptr, 64);
			rs232_timeout=FALSE;
	  	rsp_size = req_size*2 + 7;
	  	
	  	int net_timeout_cnt = 0;
	  	//printf("rsp_size should be %d\n", rsp_size);
	  	for(;;){ // do rs485 receiving during 3 second, or timeout
	  	  	// check if there's data in com port, or time out
				
				if(net_timeout_cnt==2){
	  				//rs232_timeout=TRUE;
	  				puts("Crash");
	  				rs232_timeout=INV_MSG_CRASH;
				
	  				break;
	  		}
	  		
				// wait before read it !!
				usleep(100000); //0.1 s
				
				// non-blocking read 
	  	  bytes = read(fd_rs485, (char*)bufptr, rsp_size);	// it will always receive
				nbytes +=bytes;
				
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
			rcv_len = 0;
			
			if(rs232_timeout==FALSE){
				if(nbytes && (cpy_ptr!=NULL)){
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
					
					if(check_CRC16((mt81xx_format_rtn_t *)tmp_bufp)>0){
						//printf("receive RS485 packet with CRC correct !!\n");
						
						// copy the data
						tmp_bufp += 3; // locate ptr to pointing "Byte Count"
						
						//printf("cpy_size = %d, *tmp_bufp = %d\n", cpy_size, *((uint8_t*)tmp_bufp));
						if(tail_valid && ( cpy_size == *((uint8_t*)tmp_bufp) )){
							tmp_bufp += 1;	// locate to data
							
							// switch bytes
							if(!isASCII)		// if it is ASCII field, don't switch
								switch_bytes((uint8_t*)tmp_bufp, cpy_size>>1);
							
							memcpy(cpy_ptr, tmp_bufp, cpy_size);
							//printf("Get data 0x%.4x\n", cpy_ptr[0]);
	  		  	}else
	  		  		printf("copy size mismatch, copy abort, cpy_ptr == %d\n", sizeof(cpy_ptr));
	  		  		
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
    	else if(rs232_timeout==FALSE){
//		if((rs232_timeout==FALSE)||(rs232_timeout==INV_MSG_CRASH)){
    		inv_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NORMAL);	
    		printf("Found inverter address=%d\n", inv_addr);
    		inv_info->num = inv_info->num + 1;
    		inv_info->addr[inv_info->num -1] = inv_addr;
			}else{
				printf("RS485 timeout !!\n");
				inv_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_TIMEOUT);
			}
		}
		else
			inv_state_chg_next(&inv_cmd_state, INV_CMD_EVENT_NORMAL);
		
	}

	close(fd_rs485);
	//close(fd_gpio88);
	puts("Close rs485fd, gpio88fd !!\n");
	return -1;
}
#endif


int senpu_AudioSet(gAudioCmd_t cmd, int value){
	
	(void) value;
	
	switch(cmd){
		case AUDIO_SET_PLAYBACK_VOLUME:
			break;
		case AUDIO_SET_CAPTURE_VOLUME:
			//SetAudioinVolume(&value_set, 1);
			break;
		case AUDIO_PLAY_FILE_DEFAULT:
			// set the audio file to be alarm_1_16K.wav
			system("mpg123 /mnt/ramdisk/alarm.wav");
			break;
		case AUDIO_PLAY_FILE_RECORD:		
			// play the pre-recorded file
			system("mpg123 /mnt/ramdisk/record.wav");
			
			break;
		default:;
	}
	
	return 1;
}

int senpu_StreamVideoSet(gVideoCmd_t cmd, int in_value){
	
	(void) in_value;

	
	switch(cmd){
		case VIDEO_CMD_SET_VIDEO:

		break;
		default:;
	}
	return 1;
}

int create_server(void){
	int sockfd; 
	int nsockfd;
	struct sockaddr_in addr_local; /* client addr */
	struct sockaddr_in addr_remote; /* server addr */
	char revbuf[LENGTH]; // Receiver buffer
	char sdbuf[LENGTH];
	socklen_t sin_size;
	
	/* Get the Socket file descriptor */
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1 )
	{
		//fprintf(stderr, "ERROR: Failed to obtain Socket Descriptor. (errno = %d)\n", errno);
		printf("ERROR: Failed to obtain Socket Descriptor.");
		exit(1);
	}
	else 
		printf("[Server] Obtaining socket descriptor successfully.\n");

	/* Fill the client socket address struct */
	addr_local.sin_family = AF_INET; // Protocol Family
	addr_local.sin_port = htons(PORT_SRV); // Port number
	addr_local.sin_addr.s_addr = INADDR_ANY; // AutoFill local address
	bzero(&(addr_local.sin_zero), 8); // Flush the rest of struct

	/* Bind a special Port */
	if( bind(sockfd, (struct sockaddr*)&addr_local, sizeof(struct sockaddr)) == -1 )
	{
		//fprintf(stderr, "ERROR: Failed to bind Port. (errno = %d)\n", errno);
		printf("ERROR: Failed to bind Port.\n");
		exit(1);
	}
	else 
		printf("[Server] Binded tcp port %d in addr 127.0.0.1 sucessfully.\n",PORT_SRV);

	/* Listen remote connect/calling */
	if(listen(sockfd,BACKLOG) == -1)
	{
		//fprintf(stderr, "ERROR: Failed to listen Port. (errno = %d)\n", errno);
		printf("ERROR: Failed to listen Port.\n");
		exit(1);
	}
	else
		printf ("[Server] Listening the port %d successfully.\n", PORT_SRV);


	if ((nsockfd = accept(sockfd, (struct sockaddr *)&addr_remote, &sin_size)) == -1) {
		//fprintf(stderr, "ERROR: Obtaining new Socket Despcritor. (errno = %d)\n", errno);
		printf("ERROR: Obtaining new Socket Despcritor.\n");
		exit(1);
	}
	else 
		printf("[Server] Server has got connected from %s.\n", inet_ntoa(addr_remote.sin_addr));

	int fr_block_sz = 0;
	//fr_block_sz = recv(nsockfd, revbuf, LENGTH, 0);
	fr_block_sz = recv(nsockfd, revbuf, 1, 0);
	printf("Receive %d bytes from host, cmd=0x%x\n", fr_block_sz, revbuf[0]);
	
	sdbuf[0] = 'o';
	sdbuf[1] = 'k';
	send(nsockfd, sdbuf, 2, 0);
	
	close(nsockfd);
	close(sockfd);
	
	return 1;
}

int reg_XmlIndCallBack(ind_func_t reg_func){
	
	if(!reg_func){
		puts("reg_indCallBack failed, no callback funciton was specidied !!");
		return -1;
	}
	
	// set the pointer
	xml_func_ind =  reg_func;
	puts("Set XML indication callback function !!");
	
	return 1;	
}

int reg_XmlUpdateCallBack(ind_func_t reg_func){
	
	if(!reg_func){
		puts("reg_updateCallBack failed, no callback funciton was specidied !!");
		return -1;
	}
	
	// set the pointer
	xml_func_update =  reg_func;
	puts("Set XML update indication callback function !!");
	
	return 1;	
}

int reg_indCallBack( alarm_func_t reg_func){
	
	if(!reg_func){
		puts("reg_indCallBack failed, no callback funciton was specidied !!");
		return -1;
	}
	
	// set the pointer
	alarm_func = /*(void (*)(gGpioValue_t))*/ reg_func;
	puts("Set indication callback function !!");
	
	return 1;
}

int reg_audCallBack( audio_func_t reg_func){
	
	if(!reg_func){
		puts("reg_audCallBack failed, no callback funciton was specidied !!");
		return -1;
	}
	
	// set the pointer
	audio_func = reg_func;
	puts("Set audio callback function !!");
	
	return 1;
}

int invIsFound(int item, int inv_num, char* inv_addr){
	int ind = 0;
	
	while(ind<inv_num){
		
		if(item == inv_addr[ind])
			return 1;
		
		ind ++;
	}
	return -1;
}



int do_reConnect(int * in_socket, struct sockaddr_in* in_serv_addr){
	int retry = 40;
	int sockfd;

		// create socket
		sockfd = socket(AF_INET, SOCK_STREAM, 0);
  	if (sockfd < 0) 
      error("in do_reConnect, ERROR opening socket");
		else
			printf("re-open socket %d success \n", sockfd);
		
		
		if(fcntl(sockfd, F_SETFL, O_NONBLOCK)<0)
			puts("in do_reConnect, fcntl error");
		
	sleep(2);
	
	while(retry > 0){
		printf("Retry countdown %d \n", retry);

		if (connect(sockfd,(struct sockaddr *) in_serv_addr, sizeof(struct sockaddr)) < 0){
			printf("reconnect failed, %d times to retry before abort !!\n", retry);
			
			/*
			if(fcntl(sockfd, F_SETFL, O_NONBLOCK)<0)
				puts("in do_reConnect, to O_NONBLOCK,  fcntl error");
			*/
		}
		else{
			puts("Reconnected !!");
			//fcntl(sockfd, F_SETFL, O_NONBLOCK);
			*in_socket = sockfd;
			return 1;
		}
		retry --;	
		sleep(2);
	}
	
	close(sockfd);
	return -1; 
}


char* acquire_unique_name(char * path2xml){
    xmlDoc         *document;
    xmlNode        *root, *first_child, *node;
    xmlNode        *node1, *node2, *nodeOUI, *nodeSerial;
    char           *filename;
    char          OUI_string[64], Serial_string[64], rtn_string[128];
	// read xml

	// parse xml

	// compose return string

	// return

    if (path2xml == NULL) {
        puts("input xml file is illegal");
        return '\0';
    }
    filename = path2xml;

    document = xmlReadFile(filename, NULL, 0);
    root = xmlDocGetRootElement(document);
    fprintf(stdout, "Root is <%s> (%i)\n", root->name, root->type);
    first_child = root->children;
    for (node = first_child; node; node = node->next) {
        fprintf(stdout, "\t Child is <%s> (%i)\n", node->name, node->type);
      if(strcmp(node->name, "Device")==0){
        puts("Enter Device level");
        for(node1 = node->children; node1; node1 = node1->next){
          fprintf(stdout, "\t Device Child is <%s> (%i)\n", node1->name, node1->type);
          if(strcmp(node1->name, "DeviceInfo")==0){
            puts("Enter DeviceInfo level");
            for(node2 = node1->children; node2; node2 = node2->next){
              fprintf(stdout, "\t DeviceInfo Child is <%s> (%i)\n", node2->name, node2->type);

              if(strcmp(node2->name, "ManufacturerOUI")==0){
                nodeOUI = node2->children;
                fprintf(stdout, "\t OUI content is <%s>\n", nodeOUI->content);
                nodeOUI->content = String_trim(nodeOUI->content);
                strcpy(OUI_string, nodeOUI->content);
              }
              if(strcmp(node2->name, "SerialNumber")==0){
                nodeSerial = node2->children;
                fprintf(stdout, "\t Serial content is <%s>\n", nodeSerial->content);
                nodeSerial->content = String_trim(nodeSerial->content);
                strcpy(Serial_string, nodeSerial->content);
              }
            }
          }
        }
      }
    }
    fprintf(stdout, "...\n");
    
    sprintf(rtn_string, "%s-%s", OUI_string, Serial_string);
    return rtn_string;
}

void *thread_handle_gpio(void *argu){
	
	(void)argu;
	//char tmpValue[2];
	//bool_t success_capture = FALSE;
	char uniqueName[128];
	puts("gpio thread live ");
	gGpioValue_t prev_gpio_value = 0;
	
	strcpy(uniqueName, acquire_unique_name(XML_FILE_LOCATION));
	
	while(1){
		
		prev_gpio_value = get_Gpiovalue;

		// probe the Input
		senpu_GPIOGet(&get_Gpiovalue);

		if(sys_status & STATUS_ALARM_IND_ENABLE){
			if(get_Gpiovalue == 1){	// alarm GPIO was triggered
				if(xml_func_ind){
					//getGPIO(tmpValue, 3);
					// notify the ACS with 3
					xml_func_ind(3);
				}else{  // no alarm_func was registered !!
					puts("No callback func was set !! GIO was triggered !!");
				}
			}
		}

		if(get_Gpiovalue == 1){
			puts("GPIO thread capture I/O triggered !!");
			alarm_state_change(&alarm_state, ALARM_EVENT_TRIGGER);
		}else if((get_Gpiovalue == 0)&&(prev_gpio_value == 1))
			alarm_state_change(&alarm_state, ALARM_EVENT_UNTRIGGERED);


		if(status_alarm_snapshot_enable){
			if(get_Gpiovalue == 1){
				puts("SnapShot starts to take 5 pictures");
				//printf("Sting is :%s\n", acquire_unique_name(XML_FILE_LOCATION));
				
				if(senpu_CamCapture(uniqueName))
					puts("capture successful");
				//sleep(2);
			}
		}


		// determine if output should be activated
		if(alarm_enable){
			if(get_Gpiovalue == 1){ // gpio was triggered !!
				//senpu_GPIOSet(1);
				puts("GPIO should be triggered !!!!!!");
				system("aplay ./download/alarm.wav &");
				sleep(3);
				//senpu_GPIOSet(0);
			}
			//else					 // gpio was NOT triggered !!
				//senpu_GPIOSet(0);
		}
		puts("gpio thread probe !");
		sleep(2);
	}
}


int sem_clean(sem_t * sem2clean){

	while(1){
		if(sem_trywait(sem2clean)==0)
			asm("NOP");//puts("decreased 1 !!");
		else
			break;
	}
	return 1;
}


void *thread_handle_alarm(void *argu){
	
	(void)argu;

	puts("Start handle alarm thread !!");
	
	while(1){
		switch(alarm_state){
			case ALARM_STATE_INIT:
				puts("alarm_state = ALARM_STATE_INIT");
				alarm_state_change(&alarm_state, ALARM_EVENT_ADV);
				status_alarm_snapshot_enable = FALSE;
				//sem_init(&sem_alarm_start, 0, 0);
				break;
			case ALARM_STATE_RDY:
				puts("alarm_state = ALARM_STATE_RDY");

				sem_post(&sem_alarm_stop);
				sleep(2);
				sem_clean(&sem_alarm_stop);
				sem_clean(&sem_alarm_start);
				status_alarm_snapshot_enable = FALSE;
				//printf("now sem = %d\n", sem_alarm_start);
				break;
			case ALARM_STATE_TRIGGERED:
				puts("alarm_state = ALARM_STATE_TRIGGERED");
				// start timer A
				//sem_init(&sem_alarm_start, 0, 0);
				sem_clean(&sem_alarm_start);
				countdown_secs = COUNTDOWN_TRIGGER2EXPIRE;
				sem_post(&sem_alarm_start);
				//printf("now sem = %d\n", sem_alarm_start);
				// enable pic capture
				status_alarm_snapshot_enable = TRUE;

				break;
			case ALARM_STATE_EXPIRED:
				puts("alarm_state = ALARM_STATE_EXPIRED");
				// start timer B
				//sem_init(&sem_alarm_start, 0, 0);
				sem_clean(&sem_alarm_start);
				countdown_secs = COUNTDOWN_EXPIRE2READY;
				sem_post(&sem_alarm_start);
				//printf("now sem = %d\n", sem_alarm_start);
				// disable pic capture
				status_alarm_snapshot_enable = FALSE;
				break;
			case ALARM_STATE_WEB_STOP_PRE:
				puts("alarm_state = ALARM_STATE_WEB_STOP_PRE");
				sem_post(&sem_alarm_stop);

				//waiting for timer to stop at the begining
				sleep(2);

				alarm_state_change(&alarm_state, ALARM_EVENT_ADV);
				break;
			case ALARM_STATE_WEB_STOP:
				puts("alarm_state = ALARM_STATE_WEB_STOP");
				// start timer C
				sem_clean(&sem_alarm_start);
				countdown_secs = COUNTDOWN_WEBSTOP2RESUME;
				sem_post(&sem_alarm_start);				
				//printf("now sem = %d\n", sem_alarm_start);
				status_alarm_snapshot_enable = FALSE;
				break;
		}
		sleep(1);
	}
}

void *thread_handle_timer(void *argu){
	(void)argu;
	struct timespec waitingTime;

	int sem_rtn;

	sem_init(&sem_alarm_stop, 0, 0);

	while(1){
		// wait for a period of time enough for state to be stable
		sleep(3);

		sem_wait(&sem_alarm_start);
		printf("Start %d seconds countdown !!\n", countdown_secs);
		clock_gettime(CLOCK_REALTIME, &waitingTime);
		waitingTime.tv_sec += countdown_secs;
		waitingTime.tv_nsec = 0;
		sem_rtn = sem_timedwait(&sem_alarm_stop, &waitingTime);
		
		if(sem_rtn == 0)
			printf("#%d timer about !!\n", countdown_secs);
		else
			printf("#%d timer reaches the end !!", countdown_secs);

		//debug
		printf("sem_timedwait rtn = %d\n", sem_rtn);

		// call change state
		alarm_state_change(&alarm_state, ALARM_EVENT_TIMEOUT);

		sleep(1);
	}
}

void test_callback(gGpioValue_t in_GPIO){
	puts("In callback !!");
	
	printf("callback get GPIO value is 0x%x\n", in_GPIO);
	
}

void test_audcallback(void){
	puts("In audio callback !!");
	
	printf("an *.wav record file should be transferred by TR069 at alarm_2_16K.wav \n");
	
}

int set_sys_time(char * new_dateTime){
	char dateTime[13], out_dateTime[23];  // 12 chars + 10 chars + 1

	//format should be [MMDDhhmmYYYY], total 12 characters
	// check format
	
	memcpy(dateTime, new_dateTime, 12);
	dateTime[12] = '\0';
	
	sprintf(out_dateTime, "date -s \"%s\"\n", dateTime);
	out_dateTime[22] = '\0';
	
	system(out_dateTime);
	
	return 1;
}

/****************************************************
**				Here Goes the APIs for TR069					*****
****************************************************/

// 11. SysInit

// original ShanePu
#if 0 
int setSysInit(void * inputCallBackP, void * updateCallBackP){
	char tmp[256];
	
	if(inputCallBackP == NULL)
		return -1;

	reg_XmlIndCallBack((ind_func_t)inputCallBackP);
	
	reg_XmlUpdateCallBack((ind_func_t)updateCallBackP);

	//reg_XmlUploadCallBack((upload_func_t)uploadCallBackP);

	getSysIp(sys_ip_addr, 16);
	
	sleep(1);
	
	
	//start tcp streamer
	//getStreamVideo(tmp, 255);
	setStreamVideo("1");
	printf("Start streamer %s \n", tmp);
	
	sleep(1);
	
	
	//start rtsp audio
	//getAudio(tmp, 255);
	setAudio("3");
	printf("Start audio %s \n", tmp);
	
	sleep(1);
	// init GPIO
	
	
	//system("sudo -s; ");
	//puts("Init GPIO as follow: GPIO11 as intpu, GPIO9 as output");
	
	// try to call init
	senpu_api_init();	
	
	return 1;
}
#endif

int gateway_api_init(void){
//	unsigned char value;

	puts("gateway_api_init");
	//bzero(&inv_info_delta, sizeof(inv_info_t));
	
	if(scan_inv_types & USE_MOTECH)
		bzero(inverter_msg2host_senpu_array, (sizeof(inverter_msg2host_new_t))*256);
	
	if(scan_inv_types & USE_EVERSOLAR)
		bzero(inverter_msg2host_eversolar_array, (sizeof(inverter_msg2host_eversolar_t))*256);
	
	if(scan_inv_types & USE_TOUGH)
		bzero(inverter_msg2host_ali_array, (sizeof(inverter_ali_raw_t))*256);

	if(scan_inv_types & USE_DELTA)
		bzero(inverter_msg2host_delta_array, (sizeof(inverter_msg2host_delta_t))*256);
	/*
	if(pthread_create(&thread_update_inv, NULL, thread_handle_update_inv, NULL)<0){
		printf("create thread_update_inv thread failed !!\n");
		return -1;
	}
	*/
	
	if(pthread_create(&thread_update_scan, NULL, thread_handle_update_scan, NULL)<0){
		printf("create thread_update_scan thread failed !!\n");
		return -1;
	}
/*
	if(pthread_create(&thread_delta_manager, NULL, thread_handle_manager_delta, NULL)<0){
		printf("create thread_delta_manager thread failed !!\n");
		return -1;
	}
*/
	if(pthread_create(&thread_inverter_manager, NULL, thread_handle_manager_inverter, NULL)<0){
		printf("create thread_inverter_manager thread failed !!\n");
		return -1;
	}

	return 1;
}

int daai_setSysInit(void * updateCallBackP){

	if((updateCallBackP == NULL))
		return -1;

	reg_XmlUpdateCallBack((ind_func_t)updateCallBackP);
	
	gateway_api_init();
	
	return 1;
}


// 12. SysDispose

int daai_setSysDispose(void){
	//char ps_id[16], kill_cmd[16];
	//char buff_syscall[128];

	//ApproDrvExit();
	
	// kill IPNC process
	//system("cd /opt/ipnc/; ./killall.sh");
	/*
	puts("kill the video server");
	syscall_getRsp("ps aux | grep palantir | grep -v grep | awk '{print $2}'", buff_syscall);
	strcpy(ps_id, buff_syscall);
	sprintf(kill_cmd, "kill %s", ps_id);
	system(kill_cmd);
	
	puts("kill the rtsp audio server");
	syscall_getRsp("ps aux | grep rtsp | grep -v grep | awk '{print $2}'", buff_syscall);
	strcpy(ps_id, buff_syscall);
	sprintf(kill_cmd, "kill %s", ps_id);
	system(kill_cmd);	
	*/

	pthread_kill(thread_update_scan, SIGALRM);
	pthread_kill(thread_inverter_manager, SIGALRM);
	/*
	pthread_kill(thread_senpu_manager, SIGALRM);
	pthread_kill(thread_gpio, SIGALRM);
	pthread_kill(thread_alarm, SIGALRM);
	pthread_kill(thread_timer, SIGALRM);
	*/
	
	// block waiting the following thread to return one by one
	pthread_join(thread_update_scan, NULL);
	pthread_join(thread_inverter_manager, NULL);
	/*
	pthread_join(thread_senpu_manager, NULL);
	pthread_join(thread_gpio, NULL);
	pthread_join(thread_alarm, NULL);
	pthread_join(thread_timer, NULL);
	*/
	return 1;
}

// 1. get GPIO

int getGPIO(char * returnString, int maxLen){
	//gGpioValue_t sys_GPIO;
	
	if((maxLen<2)||(returnString==NULL))
		return -1;
	
	//senpu_GPIOGet(&sys_GPIO);

	printf("GetGPIO function was called !!");

	returnString[0] = alarm_enable + '0';
	returnString[1] = '\0';
	
	return 1;
}

uint8_t getInvTypeById(uint8_t in_Id){
	int i=0;
	bool_t found = FALSE;

	if((in_Id == 0) || (in_Id > 250))
		return 0;

	while(TYPE_ID_TABLE[i].invId != 0){
		if(TYPE_ID_TABLE[i].invId == in_Id){
			found = TRUE;
			break;
		}
		i++;
	}
	
	if(found)
		return TYPE_ID_TABLE[i].invType;
	else
		return 0;
}

// 2. get Energy

int daai_getEnergy(int index, char * returnString, int maxLen){
	
	char tmp_buff[256], check_ascii_id[4]="000";
	uint8_t curr_type;

	if (index==0 || index > 255)
		return -1;
	
	if (returnString == NULL)
		return -1;
		
	//senpu_EnergyGet_new(index, &inverter_msg2host_senpu_array[index]);

	curr_type = getInvTypeById((uint8_t)index);
	
	if(curr_type == USE_PYRANOMETER)
		convert_invmsg_to_asci_pyranometer(tmp_buff, &inverter_msg2host_pyranometer_array[index]);

	if(curr_type == USE_MOTECH)
		convert_invmsg_to_asci_motech(tmp_buff, &inverter_msg2host_senpu_array[index]);
	else if(curr_type == USE_TOUGH)
		convert_invmsg_to_asci_ali(tmp_buff, &inverter_msg2host_ali_array[index]);
	else if(curr_type == USE_EVERSOLAR)
		convert_invmsg_to_asci_eversolar(tmp_buff, &inverter_msg2host_eversolar_array[index], 256);
	else if (curr_type == USE_DELTA)
		convert_invmsg_to_asci_delta(tmp_buff, &inverter_msg2host_delta_array[index]);

	if (maxLen<(int)strlen(tmp_buff))
		return -1;

	strcpy(returnString, tmp_buff);

	// check if this capturing is marked as illegal by checking the leading ID==0
	memcpy(check_ascii_id, tmp_buff, 3);
	check_ascii_id[3] = '\0';
	if(strcmp(check_ascii_id, "000")==0){
		puts("one packet with ID=000 detected !! return XML with -1");
		return -1;
	}
	else
		return strlen(tmp_buff);
}

int getEnergy_instant(int index, char * returnString, int maxLen){
	
	char tmp_buff[256];
	uint8_t curr_type;

	if (index==0 || index > 255)
		return -1;
	
	if (returnString == NULL)
		return -1;
		
	//senpu_EnergyGet_new(index, &inverter_msg2host_senpu_array[index]);

	curr_type = getInvTypeById((uint8_t)index);
	
	if(curr_type == USE_MOTECH)
		convert_invmsg_to_asci_motech(tmp_buff, &inverter_msg2host_senpu_array[index]);
	else if(curr_type == USE_TOUGH)
		convert_invmsg_to_asci_ali(tmp_buff, &inverter_msg2host_ali_array[index]);

	if (maxLen<(int)strlen(tmp_buff))
		return -1;

	strcpy(returnString, tmp_buff);

	return strlen(tmp_buff);
}

// 3. get SysIP

int getSysIp(char* returnString, int maxLen){
	//int n;
	char buff_syscall[128];
	
	if (returnString == NULL)
		return -1;	
	
	/*
	// update internal IP address
	print_addresses(AF_INET);
	
	n = sprintf(sys_IP, "%d.%d.%d.%d", msg2cloudServer.addr_IP[0], msg2cloudServer.addr_IP[1], msg2cloudServer.addr_IP[2], msg2cloudServer.addr_IP[3]);
	sys_IP[n] = '\0';
	*/
	syscall_getRsp("curl http://ipecho.net/plain", buff_syscall);
	strcpy(sys_IP, buff_syscall);
			
	if( (int)strlen(sys_IP)>maxLen)
		return -1;
	
	//the last charater of the return of syscall_getRsp(...) is a '\n', try to put a null instead of it
	sys_IP[strlen(sys_IP)] = '\0';
	strcpy(returnString, sys_IP);
	
	return strlen(sys_IP);
}

// 4. get SysMac
int getSysMac(char* returnString, int maxLen){
	char buff_syscall[128];
	
	if ((maxLen<18)||(returnString == NULL))
		return -1;
		
	syscall_getRsp("cat /sys/class/net/eth0/address", buff_syscall);
	strcpy(sys_MacAddr, buff_syscall);
	
	if( (int)strlen(sys_MacAddr)>maxLen)
		return -1;
		 
	strcpy(returnString, sys_MacAddr);
	
	return strlen(sys_MacAddr);
}

// 5. get SysInv

int daai_getSysInv(char* returnString, int maxLen){
	int n, i;
	char buffer[512];
	char *buffp;

	if (returnString == NULL)
		return -1;

	buffp = buffer;

	n = sprintf(buffp, "%.3d", sys_inv_info_all.num);
	buffp +=n;
			
	for(i=0;i<sys_inv_info_all.num;i++){
		n = sprintf(buffp, ",%.3d", sys_inv_info_all.addr[i]);
		buffp += n;
	}	

	buffp[0] = '\0';
	
	if( (int)strlen(buffer) > maxLen)
		return -1;
			
	strcpy(returnString, buffer);
	return strlen(buffer);
}

int getSysInv_instant(char* returnString, int maxLen){
	int n, i;
	char buffer[512];
	char *buffp;

	if (returnString == NULL)
		return -1;

	buffp = buffer;

	//inverter_scan((scan_inv_info_t*)&sys_inv_info);
	n = sprintf(buffp, "%.3d", sys_inv_info_all.num);
	buffp +=n;
			
	for(i=0;i<sys_inv_info_all.num;i++){
		n = sprintf(buffp, ",%.3d", sys_inv_info_all.addr[i]);
		buffp += n;
	}

	buffp[0] = '\0';
	
	if( (int)strlen(buffer) > maxLen)
		return -1;
			
	strcpy(returnString, buffer);
	return strlen(buffer);
}

// 6. get SysTimeStampEnable
int getSysTimeStampEnable(char * returnString, int maxLen){
	
	// check len
	if ((maxLen < 1)||(returnString == NULL))
		return -1;
		
	if(sys_TimeStampEnable)
		strcpy(returnString, "1");
	else
		strcpy(returnString, "0");
		
	return 1;
}

// 7. get Audio

int getAudio(char * returnString, int maxLen){

	char ps_id[16], kill_cmd[16];
	char ip_address[16];
	char buffer[128];
	char buff_syscall[128];
	// check len
	if ((maxLen < 2)||(returnString == NULL))
		return -1;

	// stop audio first
	puts("kill the rtsp audio server");
	syscall_getRsp("ps aux | grep rtsp | grep -v grep | awk '{print $2}'", buff_syscall);
	strcpy(ps_id, buff_syscall);
	sprintf(kill_cmd, "kill %s", ps_id);
	system(kill_cmd);	
	
	// start audio
	system("/home/pi/gst-rtsp/rtsp &");
	
	getSysIp(ip_address, 16);

	sprintf(buffer, "rtsp://%s:8554/audio_only", ip_address);
	
	strcpy(returnString, buffer);
	return strlen(returnString);	
}

// 8. get StreamVideo
int getStreamVideo(char * returnString, int maxLen){

	char ps_id[16], kill_cmd[16];
	char ip_address[16];
	char buffer[128];
	char buff_syscall[128];
	
	// check len
	if ((maxLen < 2)||(returnString == NULL))
		return -1;
	
	// first kill tcp streamer
	puts("kill the tcp streaming server");
	syscall_getRsp("ps aux | grep palantir | grep -v grep | awk '{print $2}'", buff_syscall);
	strcpy(ps_id, buff_syscall);
	sprintf(kill_cmd, "kill %s", ps_id);
	system(kill_cmd);
	
	// start streamer
	system("/home/pi/palantir/palantir &");
	
	getSysIp(ip_address, 16);

	sprintf(buffer, "http://%s:3000", ip_address);
	
	strcpy(returnString, buffer);
	
	return strlen(returnString);	
	
}

// 9. get Time
int getSysTime(char * returnString, int maxLen){
	
	// check len
	if ((maxLen < 2)||(returnString == NULL))
		return -1;	
	
	strcpy(returnString, sys_time);
	
	return strlen(sys_time);
}

// 10. get SysStatus

int getSysStatus(char * returnString, int maxLen){
	
	char n=0;
	//char high_4bit, low_4bit;
	char tmp_status;
	// check len
	if ((maxLen < 2)||(returnString == NULL))
		return -1;

	char value_alarm, value_auto, value_force, value_reboot, value_period, value_status;
	
	tmp_status = sys_status;
	tmp_status &= STATUS_ALARM_IND_ENABLE;
	tmp_status = tmp_status >> ALARM_BIT;
	value_alarm = tmp_status;
	
	tmp_status = sys_status;
	tmp_status &= STATUS_INV_AUTO_SCAN_ENABLE;
	tmp_status = tmp_status >> INV_AUTO_BIT;
	value_auto = tmp_status;
	
	tmp_status = sys_status;
	tmp_status &= STATUS_INV_FORCE_SCAN;
	tmp_status = tmp_status >> INV_FORCE_BIT;
	value_force = tmp_status;
	
	tmp_status = sys_status;
	tmp_status &= STATUS_SYSTEM_REBOOT_BIT;
	tmp_status = tmp_status >> SYSTEM_REBOOT_BIT;
	value_reboot = tmp_status;

	tmp_status = sys_status;
	tmp_status &= STATUS_UPDATE_PERIOD_2BIT;
	tmp_status = tmp_status >> UPDATE_PERIOD_LBIT;
	value_period = tmp_status;
	
	tmp_status = sys_status;
	tmp_status &= STATUS_SYSTEM_2BIT;
	tmp_status = tmp_status >> SYSTEM_STATUS_LBIT;
	value_status = tmp_status;
	
	n = sprintf(returnString, "%d,%d,%d,%d,%d,%d", value_alarm, value_auto, value_force, value_reboot, value_period, value_status);

	return n;		
}

// 13. SetGPIO

int setGPIO(char * inputString){
	//char tmp[3];
	//gGpioValue_t gpio_value;

	if((strlen(inputString) > 5)||(inputString == NULL))
		return -1;
	
	//test
	puts("-------------");
	printf("setGPIO was called, input=%s\n", inputString);
	puts("-------------");
	
	//gpio_value = (gGpioValue_t)atoi(inputString);

	// when UI gpio button was pushed, it send 0x00, when release, it send 0x01
	if((inputString[3] - '0')==1)
	  alarm_enable = 0;
	else if((inputString[3] - '0')==0)
	  alarm_enable = 1;

	//alarm_enable = inputString[0] - '0';
	//senpu_GPIOSet(gpio_value);

	return 1;
}

// 14. set systimestampenable

// 15. set SnapShot
int setSnapShot(char * inputString){
	
	if((strlen(inputString) > 1)||(inputString == NULL))
		return -1;
	
	if (xml_func_ind==NULL)
		return -2;
	
	senpu_SnapShotSet(0, 0);
	
	// call the callback function at XML to indicate ACS the completion of the action
	xml_func_ind(9);
	
	return 1;
}

//16. set Audio

int setAudio(char * inputString){
	
	uint8_t in_value;
	char kill_cmd[32];
	char ps_id[8];
	char buff_syscall[128];
	
	if((strlen(inputString) > 1)||(inputString == NULL))
		return -1;

	in_value = (uint8_t)atoi(inputString);

	printf("setAudio was called, in_value=%d\n", in_value);

	switch(in_value){
	case 3:
		// stop audio first
		puts("kill the rtsp audio server");
		syscall_getRsp("ps aux | grep rtsp | grep -v grep | awk '{print $2}'", buff_syscall);
		strcpy(ps_id, buff_syscall);
		sprintf(kill_cmd, "sudo kill %s", ps_id);
		system(kill_cmd);	
		
		// start audio
		system("/home/pi/gst-rtsp/rtsp &");
		puts("setAudio with parameter=3 was called !!");	
		return 1;
		
	case 2:
		// just play audio
		sleep(1);
		system("aplay ./download/recording.wav &");
  	
		// sleep 5 seconds for recording to play
		sleep(5);
  	
		// kill pid if time expire
		syscall_getRsp("ps aux | grep aplay | grep -v grep | awk '{print $2}'", buff_syscall);
		strcpy(ps_id, buff_syscall);
		sprintf(kill_cmd, "sudo kill %s", ps_id);
		system(kill_cmd);
		puts("setAudio with parameter=2 was called !!");	
		return 1;
	case 1:
		alarm_state_change(&alarm_state, ALARM_EVENT_WEBSTOP);
		//sem_post(&sem_alarm_stop);
		return 1;
	default:;
		
	}
	return -1;	

}

// 17. set StreamVideo

int setStreamVideo(char * inputString){
	char ps_id[16], kill_cmd[16];
	char buff_syscall[128];
	uint8_t in_value;

	if((strlen(inputString) > 1)||(inputString == NULL))
		return -1;

	in_value = (uint8_t)atoi(inputString);

	switch(in_value){
		case 0:
			// find the associated id of the process palantir
			puts("kill the tcp streaming server");
			syscall_getRsp("ps aux | grep palantir | grep -v grep | awk '{print $2}'", buff_syscall);
			strcpy(ps_id, buff_syscall);
			sprintf(kill_cmd, "sudo kill %s", ps_id);
			system(kill_cmd);		
		break;
		case 1:
			// first kill tcp streamer
			puts("kill the tcp streaming server");
			syscall_getRsp("ps aux | grep palantir | grep -v grep | awk '{print $2}'", buff_syscall);
			strcpy(ps_id, buff_syscall);
			sprintf(kill_cmd, "sudo kill %s", ps_id);
			system(kill_cmd);
			
			// start streamer
			system("/home/pi/palantir/palantir &");
			puts("setStreamVideo with parameter=1 was called !!");	
			return 1;	
			
		break;
		default:;
	}
	
	return -1;

}

// 18. set SysTime

int setSysTime(char * inputString){

	if((strlen(inputString) > 13)||(inputString == NULL))
		return -1;

	if(senpu_SysCtrlSet(SYS_SET_TIME, inputString)){
		memcpy(sys_time, inputString, 13);
		sys_time[12] = '\0';
	}	
	
	return 1;
	
}

// 19. setSys Ctrl

int daai_setSysCtrl(char * inputString){
	
	char in_value_period;
	int a,b,c,d,e;
	
	if((strlen(inputString) > 15)||(inputString == NULL))
		return -1;
	
	sscanf(inputString, "%d,%d,%d,%d,%d", &a, &b, &c, &d, &e);
	
	printf("We get command from ACS, they are a=%d, b=%d, c=%d, d=%d, e=%d\n", a, b, c, d, e);

	if(a==1){  // force scan
		puts("schedule force scan");
		sys_status = sys_status | STATUS_INV_FORCE_SCAN;
	}

	if(b){  // tune interval in minutes unit
		if(b<10){
			printf("change capture interval to %d minutes\n", b);
			update_acs_interval = b;
		}
	}

        if(c==1){
                puts("Get 3G re-connect command, do it !!");
                system("sh /ramdisk/ppp-stop; sleep 5; pppd call wcdma &");
        }

        if(d){
                if(d==1){
                        puts("use RTN16 preEdited XML file");
                        system("cp /home/ubuntu/gateway_proj/tr-c/env/Profiles/TR106_BaselineProfile_rtn16 /home/ubuntu/gateway_proj/tr-c/env/Profiles/TR106_BaselineProfile.xml");
                        system("sudo reboot");
                }
                if(d==3){
                        puts("use CSAC 8083 preEdited XML file");
                        system("cp /home/ubuntu/gateway_proj/tr-c/env/Profiles/TR106_BaselineProfile_csac_8083 /home/ubuntu/gateway_proj/tr-c/env/Profiles/TR106_BaselineProfile.xml");
                        system("sudo reboot");
                }                    
                if(d==4){
                        puts("use CSAC 8084 preEdited XML file");
                        system("cp /home/ubuntu/gateway_proj/tr-c/env/Profiles/TR106_BaselineProfile_csac_8084 /home/ubuntu/gateway_proj/tr-c/env/Profiles/TR106_BaselineProfile.xml");
                        system("sudo reboot");
                }                
        }


	if(e==1){
		puts("Get reboot command, reboot the Machine");
		system("sudo reboot");
	}

/*	
	if(a)
		sys_status |= STATUS_ALARM_IND_ENABLE;
	else
		sys_status &= ~STATUS_ALARM_IND_ENABLE;
	
	if(b)
		sys_status |= STATUS_INV_AUTO_SCAN_ENABLE;
	else
		sys_status &= ~STATUS_INV_AUTO_SCAN_ENABLE;
	
	if(c)
		sys_status |= STATUS_INV_FORCE_SCAN;
	else
		sys_status &= ~STATUS_INV_FORCE_SCAN;
	
	if(d){
		puts("Reboot system");
		system("sudo reboot");

	}


	in_value_period = (char)(e << UPDATE_PERIOD_LBIT);
	sys_status &= ~STATUS_UPDATE_PERIOD_2BIT;
	sys_status |= in_value_period;
*/
	//sprintf(&in_value, "%x", tmp_value); // change itself to from decimal to hex
	//printf("Internal sys_status = %d\n", sys_status);
	
	
	// make interval variable take effect for inverter update loop

	/*
	update_acs_interval = ((sys_status&STATUS_UPDATE_PERIOD_2BIT) == 0x00) ? 4 :	
													((sys_status&STATUS_UPDATE_PERIOD_2BIT) == 0x10) ? 4 :  
													((sys_status&STATUS_UPDATE_PERIOD_2BIT) == 0x20) ? 4 :
														4;
	*/
	return 1;
}

int daai_getSensorInfo(char* returnString, int maxLen){
	
	(void) maxLen;
	
	strcpy(returnString, "721,38");
	
	return 1;
}

int daai_getSysCtrl(char * returnString, int maxLen){

	if(maxLen < 2)
		return -1;

	sprintf(returnString, "%.2d", sys_status);
	
	return 1;
}

void realIndXmlCallBackFunc(int event){
	printf("callback func at XML is called, input parameter is an int:%d\n", event);
}

void realUpdateXmlCallBackFunc(int event){
	printf("callback of update func at XML is called, input parameter is an int:%d\n", event);
}

void realUploadXmlCallBackFunc(char* str1, char* str2){
	printf("callback of update func at XML is called, input parameter are two strings 1:%s, 2:%s \n", str1, str2);
}


/**********************************************
**				End  APIs for TR069							*****
**********************************************/

void *thread_handle_update_scan(void *argu){
	(void)argu;

	bzero(TYPE_ID_TABLE, sizeof(inv_type_id_t) * 256);
	bzero(&sys_inv_info_all, sizeof(sys_inv_info_all));

	while(1){
		sem_wait(&sem_invScan);
		inverter_scan_all(&sys_inv_info_all);
		//inverter_scan_motech(&sys_inv_info_motech);
		//inverter_scan_ali(&sys_inv_info_ali);
		sem_post(&sem_waitInvScanFin);
	}
}

void *thread_handle_manager_inverter(void *argu){
	(void)argu;
	//int ret;
	//char output_string[256];
	struct timespec waitingTime;
	int prob_count;
	int i;
	int fd_rs485;
	//char ip_value[16];
	//char* tmp_charP;// = ip_value;
	(void)argu;
	FILE* info_out;
	char out_buff[4096], info_this_time[1024];
	int curr_position=0, bytes_this_time=0;

	sem_init(&sem_waitInvScanFin, 0, 0);

	while(1){
		// init
		
		// postscan
		if(sys_status & STATUS_INV_AUTO_SCAN_ENABLE){
			sem_post(&sem_invScan);
	
			// timeout waiting for scan process to complete, or exit to go next 
			clock_gettime(CLOCK_REALTIME, &waitingTime);
			waitingTime.tv_sec += 480;
			waitingTime.tv_nsec = 0;
			sem_timedwait(&sem_waitInvScanFin, &waitingTime);
		}

		// do it 10 times before enter re-scan inverter
		for(i=0;i<10;i++){
			// post read 
				
			if(scan_inv_types & USE_PYRANOMETER){
				prob_count = 0;
				while(prob_count < sys_inv_info_pyranometer.num){
					// time to run inv_scan ??
					
					invInfoGet_pyranometer(sys_inv_info_pyranometer.addr[prob_count], &inverter_msg2host_pyranometer_array[sys_inv_info_pyranometer.addr[prob_count]]);
								
					printf("update PYRANOMETER data into slot #%d\n", sys_inv_info_pyranometer.addr[prob_count]);
					
					prob_count ++;	
				}
			}			

			if(scan_inv_types & USE_MOTECH){
				prob_count = 0;
				while(prob_count < sys_inv_info_motech.num){
					// time to run inv_scan ??
					
					invInfoGet_Motech(sys_inv_info_motech.addr[prob_count], &inverter_msg2host_senpu_array[sys_inv_info_motech.addr[prob_count]]);
								
					printf("update MOTEHC inv data into slot #%d\n", sys_inv_info_motech.addr[prob_count]);
					
					prob_count ++;	
				}
			}

			if(scan_inv_types & USE_TOUGH){
				prob_count = 0;
				while(prob_count < sys_inv_info_ali.num){
					// time to run inv_scan ??
					
					invInfoGet_Ali(sys_inv_info_ali.addr[prob_count], &inverter_msg2host_ali_array[sys_inv_info_ali.addr[prob_count]]);
								
					printf("update TOUGH inv data into slot #%d\n", sys_inv_info_ali.addr[prob_count]);
					
					prob_count ++;	
				}
			}

			if(scan_inv_types & USE_EVERSOLAR){
				prob_count = 0;
				while(prob_count < sys_inv_info_eversolar.num){
					// time to run inv_scan ??
					
					invInfoGet_Eversolar(sys_inv_info_eversolar.addr[prob_count], &inverter_msg2host_eversolar_array[sys_inv_info_eversolar.addr[prob_count]], FALSE);
								
					printf("update EVERSOLAR inv data into slot #%d\n", sys_inv_info_eversolar.addr[prob_count]);
					
					prob_count ++;	
				}
			}
			
			if(scan_inv_types & USE_DELTA){
				prob_count = 0;
				while(prob_count < sys_inv_info_delta.num){
					// time to run inv_scan ??
					
					invInfoGet_Delta(sys_inv_info_delta.addr[prob_count], &inverter_msg2host_delta_array[sys_inv_info_delta.addr[prob_count]]);
					//getEnergyGet_delta(sys_inv_info_delta.addr[prob_count], &inverter_msg2host_delta_array[sys_inv_info_delta.addr[prob_count]]);		
						
					printf("update DELTA inv data into slot #%d\n", sys_inv_info_delta.addr[prob_count]);
					
					prob_count ++;	
				}
			}			
			
			// check manual inverter scan from ACS
			if(sys_status & STATUS_INV_FORCE_SCAN){
				// timeout waiting for scan process to complete, or exit to go next 
				sem_post(&sem_invScan);
				
				clock_gettime(CLOCK_REALTIME, &waitingTime);
				waitingTime.tv_sec += 480;
				waitingTime.tv_nsec = 0;
				sem_timedwait(&sem_waitInvScanFin, &waitingTime);
				
				// mark sys_status the scan bit
				sys_status = sys_status & ~STATUS_INV_FORCE_SCAN;
				
				sleep(3);
			}

			// get current time
			char finename[32];
			char tmp_time_now[15];

			syscall_getRsp("date +%Y%m%d%H%M%S", tmp_time_now);
			tmp_time_now[14] = '\0';

			//tmp_charP = tmp_time;
			sprintf(finename, "./info_cache/%s.dat", tmp_time_now);

			// output to a file
			info_out = fopen(finename, "w");
			if (info_out!=NULL){

			}
			//open file

			// write into buffer
			curr_position = 0;
			bytes_this_time = 0;
			bzero(out_buff, sizeof(out_buff));
			
			// prepare buff iterately
			//for(i=0;i<sys_inv_info_all.num;i++){
			for(i=0;i<3;i++){
				bzero(info_this_time, sizeof(info_this_time));
				
				daai_getEnergy(sys_inv_info_all.addr[i], info_this_time, sizeof(info_this_time));
				printf("Gonna output:%s \n", info_this_time);
				bytes_this_time = sprintf(&out_buff[curr_position], "%s,%s###\n", tmp_time_now, info_this_time);
				curr_position += bytes_this_time;
			}

			// write to file
			fwrite(out_buff, sizeof(char), sizeof(out_buff), info_out);

			//close file
			fclose(info_out);
			// sleep
			if(sys_inv_info_all.num < 10)
				sleep(10 * update_acs_interval);
			else if(sys_inv_info_all.num < 30) // 120 secs in gothering inv data
				sleep(45 * update_acs_interval);
			else if(sys_inv_info_all.num < 50) // 200 secs in gothering inv data
				sleep(40 * update_acs_interval);
			else if(sys_inv_info_all.num < 100) // 400 secs in gothering inv data
				sleep(30 * update_acs_interval);
		
			puts("@SENPU_API:Before callBack");		
			
			if(xml_func_update)
				xml_func_update(1);
	
			//puts("@SENPU_API:After callBack");		
	
			sleep(5);
		}
	}	
}


#if 0
int main(int argc, char *argv[]) 
{
	int input;
	char input_string[16];
	char output_string[256];

	int ret;

/*
	ret = setSysInit(realIndXmlCallBackFunc, realUpdateXmlCallBackFunc);
	printf("The return is an int:%d\n", ret);
	

	while(1)
		sleep(2);

*/



while(1)
{	
	
	print_AcsMenu();
	printf("Please Insert test API :");
	scanf("%d", &input);

	
	bzero(output_string, 256);	
	
		switch(input){
			case 1: 
				ret = getGPIO(output_string, 2);
				printf("The return is an int:%d, output string:%s\n", ret, output_string);
				break;
			case 2:
				puts("insert index for getEnergy");
				scanf("%d", &input);
				ret = getEnergy(input, output_string, 256);
				printf("The return is an int:%d, output string:%s\n", ret, output_string);
				break;
			case 3:
				ret = getSysIp(output_string, 256);
				printf("The return is an int:%d, output string:%s\n", ret, output_string);
				break;
			case 4:
				ret = getSysMac(output_string, 256);
				printf("The return is an int:%d, output string:%s\n", ret, output_string);
				break;
			case 5:
				ret = getSysInv(output_string, 256);
				printf("The return is an int:%d, output string:%s\n", ret, output_string);
				break;
			case 6:  
				ret = getSysTimeStampEnable(output_string, 256);
				printf("The return is an int:%d, output string:%s\n", ret, output_string);
				break;
			case 7:		// Audio
				ret = getAudio(output_string, 256);
				printf("The return is an int:%d, output string:%s\n", ret, output_string);
				break;
			case 8:		// StreamVideo
				ret = getStreamVideo(output_string, 256);
				printf("The return is an int:%d, output string:%s\n", ret, output_string);
				break;
			case 9:   // SysTime
				ret = getSysTime(output_string, 256);
				printf("The return is an int:%d, output string:%s\n", ret, output_string);
				break;
			case 10:	// SysStatus
				ret = getSysStatus(output_string, 256);
				printf("The return is an int:%d, output string:%s\n", ret, output_string);
				break;
			case 11:	// sysInit
				ret = setSysInit(realIndXmlCallBackFunc, realUpdateXmlCallBackFunc, realUploadXmlCallBackFunc);
				printf("The return is an int:%d\n", ret);
				break;
			case 12:	// sysDispose
				ret = setSysDispose();
				printf("The return is an int:%d\n", ret);
				break;
			case 13:
				puts("insert string for setGPIO");
				scanf("%s", input_string);
				ret = setGPIO(input_string);
				printf("The return is an int:%d\n", ret);
				break;			
			case 14:
				
				break;			
			case 15:
				puts("insert string for setSnapShot");
				scanf("%s", input_string);
				ret = setSnapShot(input_string);
				printf("The return is an int:%d\n", ret);
				break;
			case 16:
				puts("insert string for setAudio");
				scanf("%s", input_string);
				ret = setAudio(input_string);
				printf("The return is an int:%d\n", ret);
				break;
			case 17:
				puts("insert string for setStreamVideo");
				scanf("%s", input_string);
				ret = setStreamVideo(input_string);
				printf("The return is an int:%d\n", ret);
				break;
			case 18:
				puts("insert string for setSysTime");
				scanf("%s", input_string);
				ret = setSysTime(input_string);
				printf("The return is an int:%d\n", ret);
				break;
			case 19:
				puts("insert string for setSysCtrl");
				scanf("%s", input_string);
				ret = setSysCtrl(input_string);
				printf("The return is an int:%d\n", ret);
				break;
			case 20:
				puts("insert index for getEnergy");
				scanf("%d", &input);
				ret = getEnergy_instant(input, output_string, 256);
				printf("The return is an int:%d, output string:%s\n", ret, output_string);
				break;
			case 21:
				ret = getSysInv_instant(output_string, 256);
				printf("The return is an int:%d, output string:%s\n", ret, output_string);
				break;

			case 99:
			exit(0);
			default:;
		}
}



}

#endif
