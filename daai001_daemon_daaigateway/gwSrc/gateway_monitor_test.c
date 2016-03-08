#include <stdio.h>
#include <stdlib.h>

#include <gateway_api.h>
#include <unistd.h>
#include <strings.h>
#include <fcntl.h>
#include "protocols.h"
#include <signal.h>

#define TEST_XML_FILE_LOCATION "./TR106_BaselineProfile.xml"

#define REBOOT_LOG "./restart_cnt.log"

#define ENABLE_INTERFACE USE_INTERFACE_1
//#define SCAN_INVERTER_TYPE (USE_DELTA | USE_EVERSOLAR)

#define IP_TO_PROBE "csac.dtdns.net"
#define TOTAL_PING_COUNT 5
#define DEV_3G "/dev/ttyO4"
#define NETWORK_LINK_LOST 0
#define NETWORK_LINK_WIRED 1
#define NETWORK_LINK_WIFI 2
#define NETWORK_LINK_3G 3
#define NETWORK_LINK_UNKNOWN 0

volatile int htimer_fd;

int network_link_check(void){
  	int ping_qualify, ping_count;
  	char rtn_str[64];
  	char rtnStr[24];
  	char cmd[1024];

  	bzero(rtnStr, 24);
  	sprintf(cmd, "ping -c %d %s | grep 'received' | awk -F',' '{ print $2 }' | awk '{ print $1 }'", TOTAL_PING_COUNT, IP_TO_PROBE); 
  	syscall_getRsp(cmd, rtnStr);
  	ping_count = atoi(rtnStr);
  	ping_qualify = TOTAL_PING_COUNT >> 1; // make half of the total as the qualify standard 

  	if(ping_count > ping_qualify){
   		puts(" network OK !!");

   		// check interface
		bzero(cmd, 1024);
		bzero(rtnStr, 24);
		sprintf(cmd, "route -n|awk '/UG/{print $8}'");
		syscall_getRsp(cmd, rtnStr);
		//printf("Rtn=%s \n", rtnStr);
		rtnStr[4] = '\0';
		if(strcmp(rtnStr,"ppp0")==0){
    		puts("Egress interface = 3G");
    		return NETWORK_LINK_3G;
    		//system("./setLed1Blink3.sh &");
    	}else if(strcmp(rtnStr,"eth0")==0){
    		puts("Egress interface = wired");
    		return NETWORK_LINK_WIRED;
    		//system("./setLed1Blink1.sh &");
    	}else if(strcmp(rtnStr,"wlan")==0){
    		puts("Egress interface = wifi");
    		return NETWORK_LINK_WIFI;
    		//system("./setLed1Blink2.sh &");
    	}else{
    		puts("Egress interface = unknown!!");
    		return NETWORK_LINK_UNKNOWN;
    		//system("./setLed1Bright.sh &");
    	}

  	}
  	else {
    	puts(" Network disconnected !!! "); 
    	return NETWORK_LINK_LOST;
    }
    return NETWORK_LINK_UNKNOWN;
}

int daemon_daai_check(void){
	char rtnStr[64];
	char cmd[1024];
	char rtn_str[64];
	int rtn_num;

	bzero(cmd, 1024);
	bzero(rtnStr, 64);
	bzero(rtn_str, sizeof(rtn_str));
	rtn_num=0;
	sprintf(cmd, "ps aux |awk '/daai001_daaigateway/{print $11}'|sed '2,10d'");
	syscall_getRsp(cmd, rtnStr);
	rtnStr[28]='\0';
	//printf("we get <--%s-->", rtnStr);
	if(strcmp(rtnStr,"Profiles/daai001_daaigateway")==0){
    	puts("gateway process exist !!");
    	return 1;
    	//system("./setLed1Blink3.sh &");    	
    }else{
    	puts("gateway process disappear !!");
    	return 0;
    }
    return 0;
}

int daemon_TR_stop_and_start(void){
	char rtnStr[24];
	char cmd[1024];
	char rtn_str[64];
	int rtn_num;

	puts("try to kill TR069 !!");
	// check daemon
	bzero(cmd, 1024);
	bzero(rtnStr, 24);
	bzero(rtn_str, sizeof(rtn_str));
	rtn_num=0;
	sprintf(cmd, "ps aux |awk '/TR069/{print $2 " " $11}'|sed '2,10d'");
	syscall_getRsp(cmd, rtnStr);
	printf("Rtn=%s \n", rtnStr);
	sscanf(rtnStr, "%d %s", &rtn_num, rtn_str);
	if((strcmp(rtn_str,"TR069")==0)||(strcmp(rtn_str,"./TR069")==0)){
    	bzero(cmd, 1024);
    	puts("TR069 exist, but daai001_daaigateway disappear, kill TR069 !!");
    	sprintf(cmd, "kill %d", rtn_num);
      	system(cmd);
    	//system("./setLed1Blink3.sh &");    	
    }else{
    	puts("TR069 disappear as well as daai001_daaigateway, restart TR069 !!");
    	system("./TR069 &");
    }
}

int network_3G_start(void){
	char rtnStr[24];
	char cmd[1024];
  	char info_3G[64];
  	int rssi;
  	int rtn_3G_exist;

	rtn_3G_exist = get_sim5215_info(DEV_3G, info_3G, sizeof(info_3G));
	rssi =0;
	printf("What we got in info_3g=%s \n", info_3G);
	sscanf(info_3G, "Rssi=%d", &rssi);
	printf("We get rssi=%d\n", rssi);
	if(rtn_3G_exist > 0) { // 3G does response, but not sure whether we can connect
		system("pppd call wcdma &");
	    sleep(7);
	}
	return 1;
}

int network_eth0_start(void){
	char rtnStr[24];
	char cmd[1024];

	bzero(cmd, 1024);
	bzero(rtnStr, 24);
	sprintf(cmd, "ifconfig |awk '/eth/{print $1}'");
	syscall_getRsp(cmd, rtnStr);
	//printf("Rtn=%s \n", rtnStr);
	rtnStr[4] = '\0';
	if(strcmp(rtnStr, "eth0")==0){
		puts("try to use eth0 as Egress interface");
		system("dhclient eth0 &");
		sleep(5);
	}
	return 1;
}

int network_wifi_start(void){
	char rtnStr[24];
	char cmd[1024];

	bzero(cmd, 1024);
	bzero(rtnStr, 24);
	sprintf(cmd, "ifconfig |awk '/wlan/{print $1}'");
	syscall_getRsp(cmd, rtnStr);
	//printf("Rtn=%s \n", rtnStr);
	rtnStr[5] = '\0';
	if(strcmp(rtnStr, "wlan0")==0){
		puts("try to use wlan0 as Egress interface");
	}	
	return 1;
}

int network_3G_stop(void){
	char rtnStr[24];
	char cmd[1024];
	char rtn_str[64];
	int rtn_num;	
	// 3.1. start wcdma_empty, solve the async issue
	system("pppd call wcdma_empty &");
	sleep(10);
	// 3.2. remove all pppd
	while(1){
	    // find pppd process
	    bzero(rtn_str, 64);
	    bzero(cmd, 1024);
	    bzero(rtnStr, 24);
	    rtn_num=0;
	    sprintf(cmd, "ps aux |awk '/pppd call wcdma/{print $2 " " $11}'|sed '2,10d'");
	    syscall_getRsp(cmd, rtnStr);
	    sscanf(rtnStr, "%d %s", &rtn_num, rtn_str);
	    printf("We get rtn_str=%s, rtn_num=%d\n", rtn_str, rtn_num);
	    // if process is pppd, kill it !!
	    if(strcmp(rtn_str,"pppd")==0){
	    	bzero(cmd, 1024);
	    	printf("Found daemon=%s, pid=%d, kill it !!\n", rtn_str, rtn_num);
	    	sprintf(cmd, "kill %d", rtn_num);
	    	system(cmd);
	    }else{
	    	puts("No pppd daemon found so far !!");
	    	break;
	    }
	    sleep(4);
	}
	return 1;
}

int network_eth0_stop(void){
	char rtnStr[24];
	char cmd[1024];
	char rtn_str[64];
	int rtn_num;	
	while(1){
		bzero(cmd, 1024);
		bzero(rtnStr, 24);
		bzero(rtn_str, sizeof(rtn_str));
		rtn_num=0;
		sprintf(cmd, "ps aux |awk '/dhclient/{print $2 " " $11}'|sed '2,10d'");
		syscall_getRsp(cmd, rtnStr);
		sscanf(rtnStr, "%d %s", &rtn_num, rtn_str);
		//printf("Rtn=%s \n", rtnStr);
		rtn_str[8]='\0';
		if(strcmp(rtn_str,"dhclient")==0){
    		puts("dhclient exist !! kill it!!");
    		bzero(cmd,1024);
    		sprintf(cmd, "kill %d", rtn_num);
    		system(cmd); 	
    	}else{
    		puts("No dhclient so far !!");
    		break;
    	}
    }
    // set static ip back to 199
    system("ifconfig eth0 192.168.1.199");
    return 1;
}

int network_wifi_stop(void){
	return 0;
}

int reboot_cnt_get_and_set_main(bool_t ifSet, bool_t reset, char* rtn_string){
	char filename[32] = "restart_cnt.log";

	FILE* file_p;
	char out_buff[4096];
	int rtn_value;
	char tmp_time_now[15];

	//tmp_charP = tmp_time;
	//sprintf(filename, "%s", WR_CACHE_DIR, tmp_time_now);

	syscall_getRsp("date +%Y%m%d%H%M%S", tmp_time_now);
	tmp_time_now[14] = '\0';

	// output to a file
	file_p = fopen(filename, "r+");
	if (file_p==NULL){
		printf("Open fine=%s error !!\n", filename);
		return -1;
	}

	bzero(out_buff, sizeof(out_buff));
	//fread(out_buff, 1, 1, file_p); // dummy read "["
	fread(out_buff, 1, 3, file_p);
	rtn_value = atoi(out_buff);

	if(reset || (rtn_value>99)){
		rtn_value = 0;
		ifSet = TRUE;
	}

	fseek(file_p, 0, SEEK_SET);
	fread(out_buff, 1, 20, file_p);
	out_buff[20]='\0';
	strcpy(rtn_string, out_buff);

	if(ifSet){
		// inc the value
		sprintf(out_buff, "%03d_%s\n", rtn_value + 1, tmp_time_now);
		//printf("gonna write %s\n", out_buff);
		fseek(file_p, 0, SEEK_SET);
		fwrite(out_buff, 1, strlen(out_buff), file_p);
		//fputs("44", file_p);
	}
	
	fclose(file_p);
	return 1;
}


void fun1(void)
{
	puts("Function 1 was called !!");
}

void fun2(void)
{
	puts("Function 2 was called !!");
}

void signal_fun(int sig)
{
	puts("Signal caught !! close htimer_fd");
	close(htimer_fd);
}

int is_Dev_Valid(char* intName){
  	char cmd[1024];
  	char rtnStr[64];

    if(strcmp(intName,"")==0){
  	  printf("Input Error !!\n", intName);
  	  return -1;
  	}

  	// check interface exist !!
  	bzero(cmd, 1024);
  	sprintf(cmd, "ls %s", intName);
  	syscall_getRsp(cmd, rtnStr);
 
  	if(strcmp(rtnStr,"")==0){
  	  printf("Device %s not valid !!\n", intName);
  	  return -1;
  	}


	return 1;
}

int is_Wifi_Valid(){



	return 1;
}

int is_Ethernet_Valid(){



	return 1;
}

int run_motech_inv_test(){
	char in_cmdreq[10] = {0x0a,0x03,0x03,0x00,0x17,0x00,0x02,0x75,0xed,0x0d};
	char out_rsp[11+5];
	app2rs485_t cmd2rs485;
	rs4852app_t rsp_pkt;
	int mod_rtn, i;

	cmd2rs485.cmd = 0x01;
	cmd2rs485.rs485_port_num = 2;
	cmd2rs485.app2rs485_cmdlen = sizeof(in_cmdreq);
	cmd2rs485.app2rs485_cmd = in_cmdreq;
	cmd2rs485.req_pkt_len = 11;

	bzero(&rsp_pkt, sizeof(rs4852app_t));

	mod_rtn = app_to_rs485_cmd(&cmd2rs485, &rsp_pkt);

	if(mod_rtn > 0){
		printf("get return !!\n");
		printf("rtnstat = 0x%d, len = %d, port=%d,\n", rsp_pkt.rsp, rsp_pkt.rs4852app_rsplen, rsp_pkt.rs485_port_num);
		if(rsp_pkt.rsp == RS485_STAT_RSPSIZEOK){
			for(i=0;i<rsp_pkt.rs4852app_rsplen;i++){
				printf("i=%d, data=0x%x\n",i,rsp_pkt.rs4852app_rsp[i]);
			}
			if(rsp_pkt.rs4852app_rsp[0]==0x0a && rsp_pkt.rs4852app_rsp[1]==0x03)
				return 1;
			else
				return -1;
		}else
			return -1;
	}
	return -1;
}

int main(void){
  int rtn, ping_count, ping_qualify;
  char rtnStr[24];
  char cmd[1024];
  int rtn_3G_exist = 0;
  
  int total_3g_retry_cnt;
  int rtn_num;
  char rtn_str[64];
  int daemon_exist = 0;
  int network_status = NETWORK_LINK_UNKNOWN;
  bool_t redundant_exist = FALSE;
  bool_t hwatchdog_exist = FALSE;
  int tr_missing_cnt = 0;
  int time_to_wr_hwtimer = 0;

//  }
#if 0
  	htimer_fd = open("/dev/watchdog", O_RDWR|O_NOCTTY);
  	if(htimer_fd < 0){
  	  printf("Fail to open \n");
  	  return -1;
  	}else
  		puts("activate hardware timer success!! reset hw timer should be kept or system reboot !!");

    signal(SIGINT, signal_fun);
#endif

    // set leds all NONE
    system("echo none > /sys/class/leds/beaglebone\:green\:usr0/trigger");
    system("echo none > /sys/class/leds/beaglebone\:green\:usr1/trigger");
    system("echo none > /sys/class/leds/beaglebone\:green\:usr2/trigger");
    system("echo none > /sys/class/leds/beaglebone\:green\:usr3/trigger");

    // set led2 dark for daemon

    // set led3 heart beat
    system("echo heartbeat > /sys/class/leds/beaglebone\:green\:usr3/trigger");
	while(1){
/*********************
**  network check
**********************/	
  		if(network_status = network_link_check()){
    		total_3g_retry_cnt = 0;
    		// update timer
    		puts("sync network time !!");
    		system("ntpdate tock.stdtime.gov.tw &");
    		time_to_wr_hwtimer ++;
    		
    		if(time_to_wr_hwtimer > 50){
    			system("hwclock -w &");
    			time_to_wr_hwtimer = 0;
    		}

    		if(network_status==NETWORK_LINK_WIRED){
    			// led, slow blink
    			system("echo timer > /sys/class/leds/beaglebone\:green\:usr0/trigger");
    			system("echo 1000 > /sys/class/leds/beaglebone\:green\:usr0/delay_off");
				system("echo 1000 > /sys/class/leds/beaglebone\:green\:usr0/delay_on");
    		}else if(network_status==NETWORK_LINK_WIFI){
    			// led
    			system("echo timer > /sys/class/leds/beaglebone\:green\:usr0/trigger");
    			system("echo 1800 > /sys/class/leds/beaglebone\:green\:usr0/delay_off");
				system("echo 200 > /sys/class/leds/beaglebone\:green\:usr0/delay_on");
    		}else if(network_status==NETWORK_LINK_3G){
    			// led
    			system("echo timer > /sys/class/leds/beaglebone\:green\:usr0/trigger");
    			system("echo 200 > /sys/class/leds/beaglebone\:green\:usr0/delay_off");
				system("echo 200 > /sys/class/leds/beaglebone\:green\:usr0/delay_on");
    		}
  		}
  		else {
 		   	total_3g_retry_cnt ++;
 		   	system("echo none > /sys/class/leds/beaglebone\:green\:usr0/trigger");
			system("echo 255 > /sys/class/leds/beaglebone\:green\:usr0/brightness");
 	   	}
    		
/*******************************
**  ttyO2 with Motech check
********************************/	

    if(run_motech_inv_test() > 0){
    	puts("inv ok, set led !!");
    	    system("echo timer > /sys/class/leds/beaglebone\:green\:usr1/trigger");
    		system("echo 1800 > /sys/class/leds/beaglebone\:green\:usr1/delay_off");
			system("echo 200 > /sys/class/leds/beaglebone\:green\:usr1/delay_on");
    }else{
    	puts("inv error, !!");
		system("echo none > /sys/class/leds/beaglebone\:green\:usr1/trigger");
		system("echo 255 > /sys/class/leds/beaglebone\:green\:usr1/brightness");
	}

/*********************
**  daemon check
**********************/			
		// check daemon
		//daemon_exist = daemon_daai_check();	// just for JinYaJo test, uncomment this for normal
		daemon_exist = 1;

		// daemon
		if(daemon_exist==1){
			tr_missing_cnt = 0;
    		system("echo none > /sys/class/leds/beaglebone\:green\:usr2/trigger");
			system("echo 0 > /sys/class/leds/beaglebone\:green\:usr2/brightness");
   		}else{
   			tr_missing_cnt ++;
    		system("echo none > /sys/class/leds/beaglebone\:green\:usr2/trigger");
			system("echo 255 > /sys/class/leds/beaglebone\:green\:usr2/brightness");
			if(tr_missing_cnt>3){
				daemon_TR_stop_and_start();
				tr_missing_cnt = 0;
			}
   		}
/*********************
**  redundancy check
**********************/		


/*********************
**  Hardware Timer
**********************/	

		//puts("reset hardware timer !!");
		//write(htimer_fd, "1", 1);
		
		//puts("Force a kernel panic");
		//system("echo c > /proc/sysrq_trigger");
/*********************
**  Take action
**********************/	
	// network
		if(network_status==0){
			// priority 1.Eth0, 2.Wifi, 3.3G, total_3g_retry_cnt <3 for Eth0, total_3g_retry_cnt <5 for ppp0, total_3g_retry_cnt <7 for Wlan
			system("route del default");
			
			if(total_3g_retry_cnt < 3){
				// 3.0 stop all other network
				network_eth0_stop();
    			network_wifi_stop();  
				// start ppp0 process
				// 3.1 stop pppd call
				network_3G_stop();
				// 3.3. wakeup ppp0 if rssi != 0
				network_3G_start();
				sleep(3);
				system("route del default");
				system("route add default gw 10.64.64.64 ppp0");
			} else if (total_3g_retry_cnt < 6){
				// 1.0. stop all other network
				network_wifi_stop();
    			network_3G_stop();
				// 1.1. kill all dhclient ps
				network_eth0_stop();
				// 1.2. do DHCP connect
   				network_eth0_start();
			} else if(total_3g_retry_cnt < 8){				
				// Wifi
				// 2.0. stop all other network
				network_eth0_stop();
    			network_3G_stop();

   				network_wifi_start();
			} else {
				puts("No available network interfaces, sleep 10 times, and then retry !!");
				if(total_3g_retry_cnt > 20)
					total_3g_retry_cnt = 0;
			}
			printf(" #%d/20 Network re-connect times !!\n", total_3g_retry_cnt);
		}


 
    	sleep(5);
  	}
  return 0;
}


#if 0
int main(void){
int a;
	FILE *fp;
	char readBuff[256] = "";
//	char pyrano_value[256];
//	char invRtnBuff[256];
	//int rtn, invNum; // invIDs[256], i;

	//uint32_t invAddr[6];
//	scan_inv_info_t tmp_info;

	//motech_to_xml_schema_file();
//	generate_xml_schema_file();
	char rtn_string[256];
	jsmn_parser p;
	jsmntok_t tokens[30];
	char *js;
	jsmntype_t rtn_type;
	char str[128];
	pthread_t thread_test;
	redundant_t tmp_body;
	eversolar_list_t* head = NULL;
	int dummy_int;


	scan_inv_info_t tmp_inv_info;

	bzero(&tmp_inv_info, sizeof(tmp_inv_info));

	while(1){
		inverter_scan_all(&tmp_inv_info, "/dev/ttyO2", &head);
		puts("hit any key to restart");
		scanf("%d", &dummy_int);
	}

/*
	strcpy(tmp_body.access_bus_name, "/dev/ttyUSB0");
	strcpy(tmp_body.unique_name, "DEVICE_NAME_BB");
	tmp_body.enable = 1;
	tmp_body.state = RDNT_STATE_IDLE;
	tmp_body.host_level = 4;

	system("./setLedAllNone.sh");
	
	if(pthread_create(&thread_test, NULL, thread_redundant_host_control, &tmp_body)<0){
		printf("create thread_inverter_manager thread failed !!\n");
		return -1;
	}

daai_getSensorInfo(rtn_string, 256);

printf("RTN:%s\n", rtn_string);

puts("Press any key to read json.txt file");
scanf("%d", &a);

	fp = fopen("json.txt", "r");
	fread(readBuff, 1, 256, fp);

	js = readBuff;

	jsmn_init(&p);
    jsmn_parse(&p, js, strlen(js), tokens, 30);

    while(1){
        bzero(rtn_string, sizeof(rtn_string));
        bzero(str, sizeof(str));
        puts("Insert key to search for:");
        scanf("%20s", str);

        rtn_type = get_value_by_key(js, tokens, 30, str, rtn_string);
        if(rtn_type==JSMN_STRING)
          printf("rtn=StringType:%s\n", rtn_string);
        else if(rtn_type==JSMN_PRIMITIVE)
          printf("rtn=PrimitiveType:%.2f\n", atof(rtn_string));
    }


	fclose(fp);
*/

#if 0
// Start of Redundant test
	int i;
	gw_sync_pkt_t test_pkt;

	puts("Start our own test !!");

	bzero(&test_pkt, sizeof(gw_sync_pkt_t));

	create_gateway_redundant_req(&test_pkt, TEST_XML_FILE_LOCATION);

	printf("buff data : ");
	for(i=0; i<sizeof(gw_sync_pkt_t); i++)
		printf("0x%c ", ((uint8_t*)&test_pkt)[i] );

	puts(" ");
// End of Redundant test


scan_inv_info_t in_list, out_list;
aging_list_t aging_list;
int iteration=0, i;

bzero(&in_list, sizeof(scan_inv_info_t));
bzero(&out_list, sizeof(scan_inv_info_t));
bzero(&aging_list, sizeof(aging_list_t));

while(1){

	printf("#%d iterations \n", iteration);

	if(iteration == 0){
		puts("initial simulate 2 entries exist");
		in_list.num = 2;
		in_list.invInfo[0].invId = 33;
		in_list.invInfo[0].invType = 01;
		in_list.invInfo[1].invId = 55;
		in_list.invInfo[1].invType = 02;
	}

	if(iteration == 1){
		puts("simulate 2 new entries");
		in_list.num = 4;
		in_list.invInfo[0].invId = 33;
		in_list.invInfo[0].invType = 01;
		in_list.invInfo[1].invId = 55;
		in_list.invInfo[1].invType = 02;
		in_list.invInfo[2].invId = 77;
		in_list.invInfo[2].invType = 03;
		in_list.invInfo[3].invId = 99;
		in_list.invInfo[3].invType = 04;
	}

	if(iteration == 3){
		puts("simulate lost 2 entries");
		in_list.num = 2;
		in_list.invInfo[1].invId = 55;
		in_list.invInfo[1].invType = 02;
		in_list.invInfo[2].invId = 99;
		in_list.invInfo[2].invType = 04;
	}

	if(iteration == 6){
		puts("simulate total 2 entries that one entry is different");
		in_list.num = 2;
		in_list.invInfo[1].invId = 55;
		in_list.invInfo[1].invType = 7;
		in_list.invInfo[2].invId = 99;
		in_list.invInfo[2].invType = 4;
	}

	if(iteration == 7){
		puts("simulate all disappear");
		in_list.num = 0;
	}	

	aging_list_fun(&in_list, &out_list, &aging_list);

	printf("in_list, num=%d, \n", in_list.num);
	for(i=0;i<in_list.num;i++)
		printf("entry:%d, id=%d, type=%d \n",i , in_list.invInfo[i].invId, in_list.invInfo[i].invType);
	
	printf("out_list, num=%d, \n", out_list.num);
	for(i=0;i<out_list.num;i++)
		printf("entry:%d, id=%d, type=%d \n",i , out_list.invInfo[i].invId, out_list.invInfo[i].invType);
	
	//printf("aging list, num=%d, \n", aging_list.num);
	for(i=0;i<aging_list.num;i++){
		if(aging_list.aging_element[i].count == INV_SCAN_MISS_TIME_BEFORE_REMOVE)
			printf("warning !! entry:%d, id=%d, type=%d gonna be remove next time \n",i , out_list.invInfo[i].invId, out_list.invInfo[i].invType);
	}

	sleep(4);

	puts(" ");

	iteration ++;
}

#endif
#if 0
	char sn[17] = "1234123412341235";
	uint8_t Id = 35;
	int rtn;
	uint8_t valid_id;
	eversolar_list_t * test_p=NULL;

	valid_id = 33;
	rtn = eversolar_appendEntry(sn, valid_id);

	printf("return = %d \n", rtn);

	valid_id = eversolar_getIdFromSn(test_p, sn);
	printf("valid id =%d\n", valid_id);







	scan_inv_info_t rtn_scan_info;
	inverter_msg2host_eversolar_t tmp_body;
	char rtn_string[256];
	eversolar_list_t* test_link_p;
	bzero(&rtn_scan_info, sizeof(scan_inv_info_t));

	puts("Test EverSolar");
	invInfoGet_Eversolar(1, &rtn_scan_info, TRUE, "/dev/ttyO1", &test_link_p);
	printf("eversolar scan result get #%d inverters, id=%d, %d, %d, %d\n", rtn_scan_info.num, rtn_scan_info.invInfo[0].invId, rtn_scan_info.invInfo[1].invId, rtn_scan_info.invInfo[2].invId, rtn_scan_info.invInfo[3].invId);

	invInfoGet_Eversolar(202, &tmp_body, FALSE, "/dev/ttyO1", &test_link_p);
	convert_invmsg_to_asci_eversolar(rtn_string, &tmp_body, 256);
	printf("RTN:%s\n", rtn_string);

	invInfoGet_Eversolar(203, &tmp_body, FALSE, "/dev/ttyO1", &test_link_p);
	convert_invmsg_to_asci_eversolar(rtn_string, &tmp_body, 256);
	printf("RTN:%s\n", rtn_string);

	invInfoGet_Eversolar(206, &tmp_body, FALSE, "/dev/ttyO1", &test_link_p);
	convert_invmsg_to_asci_eversolar(rtn_string, &tmp_body, 256);
	printf("RTN:%s\n", rtn_string);

#endif

#if 0
while(1){

	invInfoGet_Delta(333, &inverter_msg2host_delta_array[333], "/dev/ttyO2");
	convert_invmsg_to_asci_delta(invRtnBuff, &inverter_msg2host_delta_array[333]);
	printf("Delta RTN:%s\n", invRtnBuff);

	invInfoGet_Ali(444, &inverter_msg2host_ali_array[444], "/dev/ttyO2");
	convert_invmsg_to_asci_ali(invRtnBuff, &inverter_msg2host_ali_array[444]);
	printf("RTN:%s\n", invRtnBuff);

	invInfoGet_EnerSolis(111, &inverter_msg2host_enersolis_array[111], "/dev/ttyO2");
	convert_invmsg_to_asci_enersolis(invRtnBuff, &inverter_msg2host_enersolis_array[111]);
	printf("EnerSolis RTN:%s\n", invRtnBuff);


	invInfoGet_Motech(222, &inverter_msg2host_motech_array[222], "/dev/ttyO2");
	convert_invmsg_to_asci_motech(invRtnBuff, &inverter_msg2host_motech_array[222]);
	printf("Motech RTN:%s\n", invRtnBuff);
}

	bzero(&tmp_info, sizeof(scan_inv_info_t));
	inverter_scan_enersolis(&tmp_info);

	printf("we get %d inverters, first 3 value is %d, %d, %d\n", tmp_info.num, tmp_info.addr[0], tmp_info.addr[1], tmp_info.addr[2]);



#endif


	//daai_setSysInit(fun1);

	//scan_inv_info_t test_pyrano;

	while(1){

/*
		inverter_scan_pyranometer(&test_pyrano, MODEMDEVICE_RF, 0);
		printf("num=%d, id=%d\n", test_pyrano.num, test_pyrano.invInfo[0].invId);
		sleep(2);

		sleep(20);

		sleep(5);
		inverter_scan_pyranometer(invRtnBuff, MODEMDEVICE_RJ45, 256);
		daai_getEnergy(264, pyrano_value, 256);
*/
	}

#if 0
		daai_getSysInv(invRtnBuff, 1024);
		printf("We get return string:%s\n", invRtnBuff);
		sscanf(invRtnBuff, "%d,%d,%d,%d,%d,%d,%d", &invNum, &invAddr[0], &invAddr[1], &invAddr[2], &invAddr[3], &invAddr[4], &invAddr[5]);
		printf("This time we get %d inverter(s)\n", invNum);
		
		//sscanf(invRtnBuff, "%s,", testBuff);
		//printf("The rest of it are:%s \n", testBuff);

		//system("./ppp-stop");
		daai_getSensorInfo(invRtnBuff, 256);
		printf("SensorInfo:%s\n", invRtnBuff);
		//system("pppd call wcdma &");

		rtn = daai_getEnergy(invAddr[0], pyrano_value, 256);
		if(rtn>0)
			printf("Upload Pyrano data:%s\n", pyrano_value);
		else 
			puts("call to daai_getEnergy() returns error");
	}
	
		
		rtn = daai_getEnergy(269, pyrano_value, 256);
		if(rtn>0)
			printf("Upload Pyrano data:%s\n", pyrano_value);
		else 
			puts("call to daai_getEnergy() returns error");
		
		rtn = daai_getEnergy(533, pyrano_value, 256);
		if(rtn>0)
			printf("Upload Pyrano data:%s\n", pyrano_value);
		else 
			puts("call to daai_getEnergy() returns error");


/*
		rtn = daai_getEnergy(3, pyrano_value, 256);
		if(rtn>0)
			printf("Upload Motech data:%s\n", pyrano_value);
		else 
			puts("call to daai_getEnergy() returns error");

		bzero(pyrano_value, 256);
		rtn = daai_getEnergy(16, pyrano_value, 256);
		if(rtn>0)
			printf("Upload ThermoMeter1 data:%s\n", pyrano_value);
		else 
			puts("call to daai_getEnergy() returns error");
*/

	}

#endif

	return 1;
}

#endif