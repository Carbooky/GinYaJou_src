#include <stdio.h>
#include <stdlib.h>

#include <gateway_api.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include "protocols.h"
#include <signal.h>
#include <math.h>
#include <time.h>

#define TEST_XML_FILE_LOCATION "./TR106_BaselineProfile.xml"



#define IP_TO_PROBE "csac.dtdns.net"
#define TOTAL_PING_COUNT 5
#define DEV_3G "/dev/ttyO4"
#define NETWORK_LINK_LOST 0
#define NETWORK_LINK_WIRED 1
#define NETWORK_LINK_WIFI 2
#define NETWORK_LINK_3G 3
#define NETWORK_LINK_UNKNOWN 0

volatile int htimer_fd;
/*
int network_link_check(void){
  	int ping_qualify, ping_count;
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
*/

int daemon_daai_check(void){
	char rtnStr[64];
	char cmd[1024];
	char rtn_str[64];

	bzero(cmd, 1024);
	bzero(rtnStr, 64);
	bzero(rtn_str, sizeof(rtn_str));

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

    return 1;
}

#if 0
int network_3G_start(void){
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
#endif

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
	(void) sig;
	puts("Signal caught !! close htimer_fd");
	close(htimer_fd);
}

#if 0
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
	printf("currTime = %d\n", currTimes_s);
	if (currTimes_s == 0)
		accurr = accurr;
	else if(currTimes_s < 0){
		currTimes_f = abs(currTimes_s);
		currTimes_f = pow(10,currTimes_f);
		accurr = accurr / currTimes_f;
		printf("accurr = %f\n", accurr);
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

	n = sprintf(buff, "%04d,%s,%s,%s,%s,0x%4x,%0.1f,%0.1f,%.2f,%0.2f,%.2f,%.2f", 
		rs485_id, "DIGIMETER", modelName, serialName, tmp_time, error,
		acvolt, accurr, freq, pwrfc, sumpwr, sumpwrh 
	);

	buff[n] = '\0';	

	return 1; 
	
}
#endif 

int get_digimeter_value(char* out_buff, int size_of_buff){
	inverter_msg2host_digimeter_t dummy_info;
	char buff[256];

	bzero(&dummy_info, sizeof(inverter_msg2host_digimeter_t));
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

	if ((int)strlen(buff) > size_of_buff){
		memcpy(out_buff, buff, size_of_buff);
		return -1;
	}
	else
		memcpy(out_buff, buff, strlen(buff));

	return 1;
}

int capture_MAC_addr_my(char* rtn_MAC){
	char rtn_mac[18];
	int i;

	if(syscall_getRsp("cat /sys/class/net/eth0/address", rtn_mac)<0){
		rtn_MAC[0]='\0';
		return -1;
	}

	printf("rtnMAC=%s\n", rtn_mac);
	//replace ":" by "-"
	for(i=2;i<15;i=i+3){
		rtn_mac[i] = '-';
		printf("i=%d\n", i);
	}

	printf("modified rtnMAC=%s\n", rtn_mac);

	rtn_mac[17]='\0';

	strcpy(rtn_MAC,rtn_mac);

	return 1;
}

#define BASH_PROFILE_MY "/etc/bash.bashrc"
//#define BASH_PROFILE_MY "./test.bb"

int set_var_to_env_my(void){
	FILE* file_p = fopen(BASH_PROFILE_MY, "a+");

  	if(file_p ==NULL)
  		return -1;

  	fputs("\nexport IS_AUTH=YES\n", file_p);
  	fclose(file_p);

  	return 1;
}

int check_var_exist_env_my(void){
	char rtn_string[18];
	bzero(rtn_string, 18);
	syscall_getRsp("env |grep IS_AUTH", rtn_string);
	rtn_string[7]='\0';
	if(strcmp("IS_AUTH",rtn_string)==0)
		return 1;

	return -1;
}

void dummy_callback(void){
	char test_buff[256];
	puts("I'm stupid callback func ack to daemon !!");
	daai_getSensorInfo(test_buff, 256);
	printf("sensor:--%s--\n",test_buff);
}

int main(void){
/*
  int rtn, ping_count, ping_qualify;
  
  char cmd[1024];
  int rtn_3G_exist = 0;
  
  int total_3g_retry_cnt;
  
  char rtn_str[64];
  int daemon_exist = 0;
  int network_status = NETWORK_LINK_UNKNOWN;
  bool_t redundant_exist = FALSE;
  bool_t hwatchdog_exist = FALSE;
  int tr_missing_cnt = 0;
  int time_to_wr_hwtimer = 0;
  inverter_digimeter_raw_t tmp_info;
  int i;
  scan_inv_info_t inv_info;
*/

#if 1
//write_3g_info_file("./3g_tmp_status.txt");

	while(1){
		network_3G_stop();
		sleep(300);
	}

return 1;
#endif

#if 0

char str_1[16];
char str_2[16];

        puts("test init lcd");
        init_USB_LCD("/dev/ttyACM0");

        while(1){
        sprintf(str_1, "DaemonVer=%s", DAEMON_VERSION);
        string_USB_LCD("/dev/ttyACM0", str_1, 1);

        //sprintf(str_2, "TR069Ver=%s", "2.4.1");
        //string_USB_LCD("/dev/ttyACM0", str_2, 2);

        // show inv status
        
        sprintf(str_2, "Found Inv=%s", sys_inv_info_all.num);
        string_USB_LCD("/dev/ttyACM0", str_2, 2);

        sleep(2);
        }
#endif


#if 0
// test 3G
  	char buff_tt[64];
  	read_3g_info_file("./3g_info.txt", buff_tt, 64);

  	printf("1, we get --%s--\n", buff_tt);

	write_3g_info_file("./3g_info.txt");

  	read_3g_info_file("./3g_info.txt", buff_tt, 64);

  	printf("2, we get --%s--\n", buff_tt);
#endif 

#if 0
network_3G_stop();

network_3G_start();
#endif

//#if 0
// test daemon
  daai_setSysInit(dummy_callback);
  //gateway_api_init();

  sleep(180);

  int rtn_num;
  rtn_num = sys_inv_info_all.num;

  char rtnStr[512];
  bzero(rtnStr, sizeof(rtnStr));
  daai_getSysInv(rtnStr, sizeof(rtnStr));
  printf("get inv list:%s\n", rtnStr);


  if (rtn_num > 1){
  	bzero(rtnStr, sizeof(rtnStr));
  	daai_getEnergy((int)sys_inv_info_all.invInfo[0].invId, rtnStr, sizeof(rtnStr));
  	//int ind = sys_inv_info_all.invInfo[0].invId;
  	//convert_invmsg_to_asci_enersolis(rtnStr, &inverter_msg2host_enersolis_array[ind]);
  	printf("inv %d value:%s\n",sys_inv_info_all.invInfo[0].invId, rtnStr);

  	sleep(1);

  	bzero(rtnStr, sizeof(rtnStr));
  	daai_getEnergy((int)sys_inv_info_all.invInfo[1].invId, rtnStr, sizeof(rtnStr));
  	printf("inv 6 value:%s\n", rtnStr);
  }else if(rtn_num > 0){
  	bzero(rtnStr, sizeof(rtnStr));
  	daai_getEnergy((int)sys_inv_info_all.invInfo[0].invId, rtnStr, sizeof(rtnStr));
  	printf("inv 2 value:%s\n", rtnStr);
  }  
//#endif 


/*
  sleep(70);
  int rtn_num;
  rtn_num = sys_inv_info_all.num;

  char rtnStr[128];
  bzero(rtnStr, sizeof(rtnStr));
  daai_getSysInv(rtnStr, sizeof(rtnStr));
  printf("get inv list:%s\n", rtnStr);


  if (rtn_num > 1){
  	bzero(rtnStr, sizeof(rtnStr));
  	daai_getEnergy((int)sys_inv_info_all.invInfo[0].invId, rtnStr, sizeof(rtnStr));
  	printf("inv 2 value:%s\n", rtnStr);

  	sleep(1);

  	bzero(rtnStr, sizeof(rtnStr));
  	daai_getEnergy((int)sys_inv_info_all.invInfo[1].invId, rtnStr, sizeof(rtnStr));
  	printf("inv 6 value:%s\n", rtnStr);
  }else if(rtn_num > 0){
  	bzero(rtnStr, sizeof(rtnStr));
  	daai_getEnergy((int)sys_inv_info_all.invInfo[0].invId, rtnStr, sizeof(rtnStr));
  	printf("inv 2 value:%s\n", rtnStr);
  }  
*/



/*
  scan_inv_info_t test_scan;

  for(i=0;i<5;i++){
  	rtn = inverter_scan_jda(&test_scan, "/dev/ttyO2", 0xffff);
  	sleep(5);
  }
*/
  sleep(500);

#if 0
  set_var_to_env_my();

  if(check_var_exist_env_my()>0)
  	puts("variable exist !!");
  else
  	puts("variable doesn't exist!!");

  return 1;


	//gateway_api_init();
  	char rtn_MAC[18];
  	capture_MAC_addr_my(rtn_MAC);

  	change_unique_name_my("/home/ubuntu/gateway_proj/tr-c_daai/env/Profiles/", rtn_MAC);
  	puts("write done !");

  	acquire_unique_name("./TR106_BaselineProfile.xml", rtn_str);
  	printf("return string:%s\n", rtn_str);
	
  	sleep(3);


	sleep(300);
#endif

#if 0
  bzero(&inv_info, sizeof(scan_inv_info_t));
  bzero(cmd, 1024);
  bzero(&tmp_info, sizeof(inverter_digimeter_raw_t));

  //get_digimeter_value(cmd, 1024);

  invInfoGet_DigiMeter(2, &tmp_info, "/dev/ttyO2");
  convert_invmsg_to_asci_digimeter(cmd, &tmp_info);
  
  printf("we get rtn:%s \n", cmd);

  inverter_scan_digimeter(&inv_info, "/dev/ttyO2", 0xffff);
  printf("scan result found %d digimeter(s)\n", inv_info.num);



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
    		


/*********************
**  daemon check
**********************/			
		// check daemon
		daemon_exist = daemon_daai_check();

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


 
    	sleep(150);
  	}

  #endif
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

