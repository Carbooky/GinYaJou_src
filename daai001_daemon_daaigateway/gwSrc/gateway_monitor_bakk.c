#include <stdio.h>
#include <stdlib.h>

#include <gateway_api.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include "protocols.h"
#include <signal.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <pthread.h>
#include <ctype.h>

#define TEST_XML_FILE_LOCATION "./TR106_BaselineProfile.xml"

#define REBOOT_LOG "./restart_cnt.log"

#define IP_TO_PROBE "csac.dtdns.net"
#define TOTAL_PING_COUNT 5
#define DEV_3G "/dev/ttyO4"
/*
#define NETWORK_LINK_LOST 0
#define NETWORK_LINK_WIRED 1
#define NETWORK_LINK_WIFI 2
#define NETWORK_LINK_3G 3
#define NETWORK_LINK_UNKNOWN_BUT_LINK_OK 10
#define NETWORK_LINK_UNKNOWN 0
*/

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
    		return NETWORK_LINK_UNKNOWN_BUT_LINK_OK;
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

int is_Dev_Valid(char* intName){
  	char cmd[1024];
  	char rtnStr[64];

    if(strcmp(intName,"")==0){
  	  puts("Input Error !! ");
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

int capture_MAC_addr(char* rtn_MAC){
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

#define SERVER_IP "192.168.1.200"
#define SERVPORT 2000
#define BufferLength 100

int connect_auth_server(char* serverIP, char* MAC2reg){
	int sd, rc, length = sizeof(int);
	struct sockaddr_in serveraddr;
	char buffer[BufferLength];
	char server[255];
	char temp;
	//int totalcnt = 0;
	struct hostent *hostp;
	char data[100];//"This is a test string from client lol!!! ";

	strcpy(data, MAC2reg);


	if((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("Client-socket() error");
		return -1;
	}
	else
		printf("Client-socket() OK\n");

	if(serverIP==NULL)
		return -1;

	strcpy(server, serverIP);

	memset(&serveraddr, 0x00, sizeof(struct sockaddr_in));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(SERVPORT);

	if((serveraddr.sin_addr.s_addr = inet_addr(server)) == (unsigned long)INADDR_NONE){
		/* get host address */
		hostp = gethostbyname(server);
		if(hostp == (struct hostent *)NULL){
			printf("HOST NOT FOUND --> ");
			/* h_errno is usually defined */
			/* in netdb.h */
			printf("h_errno = %d\n",h_errno);
			printf("---This is a client program---\n");
			close(sd);
			return -1;
		}

		memcpy(&serveraddr.sin_addr, hostp->h_addr, sizeof(serveraddr.sin_addr));
	}

	if((rc = connect(sd, (struct sockaddr *)&serveraddr, sizeof(serveraddr))) < 0){
		perror("Client-connect() error");
		close(sd);
		return -1;
	}else
		printf("Connection established...\n");

	printf("Sending some string to the f***ing %s...\n", server);
	rc = write(sd, data, sizeof(data));

	if(rc < 0)
	{
		perror("Client-write() error");
		rc = getsockopt(sd, SOL_SOCKET, SO_ERROR, &temp, (socklen_t*)&length);
		if(rc == 0){
		/* Print out the asynchronously received error. */
			errno = temp;
			perror("SO_ERROR was");
		}
		close(sd);
		return -1;
	}else{
		printf("Client-write() is OK\n");
		printf("String successfully sent lol!\n");
		printf("Waiting the %s to echo back...\n", server);
	}

	//totalcnt = 0;
	//rc = read(sd, &buffer[totalcnt], 2);
	rc=2;
	buffer[0] ='Y';
	buffer[1] ='S';

	if(rc < 0)
	{
		perror("Client-read() error");
		close(sd);
		return -1;
	}
	else if (rc == 0)
	{
		printf("Server program has issued a close()\n");
		close(sd);
		return -1;
	}
	else if(rc==2)
	{
		printf("Get retrun=%c, %c\n", buffer[0], buffer[1]);
		
		close(sd);

		if(buffer[0]=='Y' && buffer[1]=='S')
			return 1;
		else
			return -1;
	}

	close(sd);
	return -1;
}

//do a nonblocking connect 
//  return -1 on a system call error, 0 on success
//  sa - host to connect to, filled by caller
//  sock - the socket to connect
//  timeout - how long to wait to connect
inline int 
conn_nonb(struct sockaddr_in sa, int sock, int timeout, char* MAC2reg)
{   
    int flags = 0, error = 0, ret = 0;
    fd_set  rset, wset;
    socklen_t   len = sizeof(error);
    struct timeval  ts;
    char data[100];

    ts.tv_sec = timeout;
    
    strcpy(data, MAC2reg);

    //clear out descriptor sets for select
    //add socket to the descriptor sets
    FD_ZERO(&rset);
    FD_SET(sock, &rset);
    wset = rset;    //structure assignment ok
    
    //set socket nonblocking flag
    if( (flags = fcntl(sock, F_GETFL, 0)) < 0)
        return -1;
    
    if(fcntl(sock, F_SETFL, flags | O_NONBLOCK) < 0)
        return -1;
    
    //initiate non-blocking connect
    if( (ret = connect(sock, (struct sockaddr *)&sa, 16)) < 0 )
        if (errno != EINPROGRESS)
            return -1;

    if(ret == 0){    //then connect succeeded right away
        printf("get connect to server !!!!!!!! \n");
        goto done;
    }

    //we are waiting for connect to complete now
    if( (ret = select(sock + 1, &rset, &wset, NULL, (timeout) ? &ts : NULL)) < 0)
        return -1;
    if(ret == 0){   //we had a timeout
        errno = ETIMEDOUT;
        return -1;
    }

    //we had a positivite return so a descriptor is ready
    if (FD_ISSET(sock, &rset) || FD_ISSET(sock, &wset)){
        if(getsockopt(sock, SOL_SOCKET, SO_ERROR, &error, &len) < 0)
            return -1;
    }else
        return -1;

    if(error){  //check if we had a socket error
        errno = error;
        return -1;
    }

done:
	// send data to server
	write(sock, data, strlen(data)); 
    //put socket back in blocking mode
    if(fcntl(sock, F_SETFL, flags) < 0)
        return -1;

    return 0;
}

#define IS_AUTH_FILE "./is_auth.txt"
#define GATEWAY_MONITOR_FILE "./GM_ver.txt"
#define GATEWAY_VER	"V1.05"

int convert_str_lower_to_upper(char* instr, char* outstr){
	int i;
	char tmp[64];

	bzero(tmp, 64);

	if(instr==NULL)
		return -1;

	for(i=0;i<(int)strlen(instr);i++)
		tmp[i]=toupper(instr[i]);

	strcpy(outstr, tmp);

	return strlen(tmp);
}

int write_ver_to_file(void){
	FILE* file_p = fopen(GATEWAY_MONITOR_FILE, "w");

  	if(file_p ==NULL)
  		return -1;

  	fputs(GATEWAY_VER, file_p);
  	fclose(file_p);

  	system("sync");

  	return 1;
}

int set_var_to_file(void){
	FILE* file_p = fopen(IS_AUTH_FILE, "w");

  	if(file_p ==NULL)
  		return -1;

  	fputs("IS_AUTH=YES\n", file_p);
  	fclose(file_p);

  	system("sync");

  	return 1;
}

int check_var_exist(void){
	char rtn_string[18];
	bzero(rtn_string, 18);
	int rcnt=0;
	FILE* file_p = fopen(IS_AUTH_FILE, "r");
	if(file_p == NULL)
		return -1;

	rcnt = fread(rtn_string, 1, 18, file_p);
	if(rcnt>0){
		rtn_string[7]='\0';
		if(strcmp("IS_AUTH",rtn_string)==0){
			fclose(file_p);
			return 1;
		}
	}
	fclose(file_p);
	return -1;
}


#define EXT_IP_FILE "./ramdisk/gateway_ip.txt"
int update_extIP_to_file(void){
	char getIP[16]="";
	int ip_legal_num=0;
	int dummy1, dummy2, dummy3, dummy4;
	// acquire IP first
	//syscall_getRsp("curl http://ipecho.net/plain", getIP);
	
	syscall_getRsp("curl -s http://checkip.dyndns.org/ | grep -o \"[[:digit:].]\\+\"", getIP);
  	
	//scanf("%s",getIP);

  	// check if it is a legal IP
  	if(strcmp(getIP, "")==0){
  		puts("rtn IP is empty");
  		strcpy(getIP,"0.0.0.0");
  	}else{	// legal IP
  		ip_legal_num = sscanf(getIP, "%d.%d.%d.%d", &dummy1, &dummy2, &dummy3, &dummy4);
  		if(ip_legal_num!=4){
  			puts("rtn IP is illegal");
  			strcpy(getIP,"0.0.0.0");
  		}
  		printf("rnt num=%d\n", ip_legal_num);
  	}
	
	getIP[15] = '\0';

	// then try to write into file
	FILE* file_p = fopen(EXT_IP_FILE, "w");

  	if(file_p ==NULL)
  		return -1;

	fputs(getIP, file_p);
  	fclose(file_p);

  	system("sync");

  	return 1;
}

pthread_t update_ext_ip_sem, network_link_check_sem;

void *thread_update_ext_ip_to_file(void *argu){
	(void) argu;
	puts("start thread to update external IP into file !");
	while(1){
		update_extIP_to_file();
		puts("Update external IP !");
		sleep(180);
	}
}

int network_status_temporary=NETWORK_LINK_UNKNOWN;

void *thread_network_link_check(void *argu){
	(void) argu;
	puts("start thread network link check !");
	while(1){
		// set link lost before probe, if check stuck, we see lost
		network_status_temporary = NETWORK_LINK_LOST;
		network_status_temporary = network_link_check();
		puts("Update network status !");
		sleep(60);
	}
}

int lost_cnt=0;
int global_network_status = NETWORK_LINK_LOST;
void network_link_check_nonblocking(void){
	if(network_status_temporary == NETWORK_LINK_LOST){
		if(lost_cnt > 2)
			global_network_status = NETWORK_LINK_LOST;
		else
			lost_cnt ++;
	}else{
		lost_cnt = 0;
		global_network_status = network_status_temporary;
	}
}

int main(void){
  int total_reconnect_retry_cnt=0;

  int daemon_exist = 0;
  int network_status = NETWORK_LINK_UNKNOWN;
  //int tr_missing_cnt = 0;
  int time_to_wr_hwtimer = 0;

  int confirmed = 0;

//  }

#if 0
  update_extIP_to_file();
  puts("End");
  exit(1);
#endif

#if 0
  	htimer_fd = open("/dev/watchdog", O_RDWR|O_NOCTTY);
  	if(htimer_fd < 0){
  	  printf("Fail to open \n");
  	  return -1;
  	}else
  		puts("activate hardware timer success!! reset hw timer should be kept or system reboot !!");

    signal(SIGINT, signal_fun);
#endif
	//
	// Enter authentication phase
	//

    //while(1){
		// check if this is the first time
		//if(check_var_exist()>0){
		//	puts("this is NOT the first time !!");
		//	break;
		//}
		//else{
			//puts("this is the first time !!");
	
    		// set leds blink
    	system("echo timer > /sys/class/leds/beaglebone:green:usr0/trigger");
    	system("echo 400 > /sys/class/leds/beaglebone:green:usr0/delay_on");
		system("echo 200 > /sys/class/leds/beaglebone:green:usr0/delay_off");
		system("echo timer > /sys/class/leds/beaglebone:green:usr1/trigger");
    	system("echo 400 > /sys/class/leds/beaglebone:green:usr1/delay_on");
		system("echo 200 > /sys/class/leds/beaglebone:green:usr1/delay_off");
		system("echo timer > /sys/class/leds/beaglebone:green:usr2/trigger");
    	system("echo 400 > /sys/class/leds/beaglebone:green:usr2/delay_on");
		system("echo 200 > /sys/class/leds/beaglebone:green:usr2/delay_off");
		system("echo timer > /sys/class/leds/beaglebone:green:usr3/trigger");
    	system("echo 400 > /sys/class/leds/beaglebone:green:usr3/delay_on");
		system("echo 200 > /sys/class/leds/beaglebone:green:usr3/delay_off");

		// write version to file
		write_ver_to_file();

		// capture MAC address
		char mac_addr[18], mac_addr_upper[18];
		capture_MAC_addr(mac_addr);
	
		// change to upper case
		convert_str_lower_to_upper(mac_addr,mac_addr_upper);

		//check if this is the first time
		if(check_var_exist()>0){
			puts("this is NOT the first time !!");
			//break;
		}
		else{	
			// apply to XML file
			change_unique_name_my("./Profiles/", mac_addr_upper);
		  	puts("write done !");

		  	set_var_to_file();
		}		
		// notify server with fixed IP=192.168.1.200
		// wait a period of time for Network to work properly
		sleep(2);
		
		//confirmed = connect_auth_server(SERVER_IP, mac_addr_upper);
		
		struct sockaddr_in s_in;
		char ip[20];
		int sd;
			if((sd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
			{
				perror("Client-socket() error");
				return -1;
			}
			else
				printf("Client-socket() OK\n");

		s_in.sin_family = PF_INET;
   		s_in.sin_port = htons(SERVPORT);
    	sprintf(ip,SERVER_IP);   //just for testing purposes, it's NOT my IP
    	s_in.sin_addr.s_addr=inet_addr(ip);

    	confirmed = conn_nonb(s_in,sd,10, mac_addr_upper);
		

		if(confirmed==0){
			// write
			puts("get confirmed from server !!");
			//set_var_to_file();
			//break;
		}
		close(sd);


	//}

	//
	// End authentication phase
	//

    // set leds all NONE
    system("echo none > /sys/class/leds/beaglebone:green:usr0/trigger");
    system("echo none > /sys/class/leds/beaglebone:green:usr1/trigger");
    system("echo none > /sys/class/leds/beaglebone:green:usr2/trigger");
    system("echo none > /sys/class/leds/beaglebone:green:usr3/trigger");

    // set led2 dark for daemon
    
    // start tr from tr_watchdog 
    //system("/home/ubuntu/gateway_proj/tr-c_daai/env/tr069_watchdog &");

    // set led3 heart beat
    system("echo heartbeat > /sys/class/leds/beaglebone:green:usr3/trigger");

    sleep(10); // for some other process to get down, ex:static_ip setup

	if(pthread_create(&update_ext_ip_sem, NULL, thread_update_ext_ip_to_file, NULL)<0){
		printf("create thread_update_ext_ip_to_file thread failed !!\n");
		return -1;
	}

	if(pthread_create(&network_link_check_sem, NULL, thread_network_link_check, NULL)<0){
		printf("create thread_network_link_check thread failed !!\n");
		return -1;
	}

	while(1){
/*********************
**  network check
**********************/
		sleep(10);
		network_link_check_nonblocking();
		network_status = global_network_status;
		printf("Get network link=%d\n", network_status);
  		if(network_status > NETWORK_LINK_LOST){
    		total_reconnect_retry_cnt = 0;
    		// update timer
    		puts("sync network time !!");
    		system("ntpdate tock.stdtime.gov.tw &");
    		sleep(10);
    		system("ntpdate time.stdtime.gov.tw &");
    		time_to_wr_hwtimer ++;
    		
    		if(time_to_wr_hwtimer == 5)
    			system("hwclock -w &");
    		else if(time_to_wr_hwtimer > 1000)
    			time_to_wr_hwtimer = 0;

    		if(network_status==NETWORK_LINK_WIRED){
    			// led, slow blink
    			system("echo timer > /sys/class/leds/beaglebone:green:usr0/trigger");
    			system("echo 1000 > /sys/class/leds/beaglebone:green:usr0/delay_off");
				system("echo 1000 > /sys/class/leds/beaglebone:green:usr0/delay_on");
    		}else if(network_status==NETWORK_LINK_WIFI){
    			// led
    			system("ifconfig eth0 192.168.1.199");
    			system("echo timer > /sys/class/leds/beaglebone:green:usr0/trigger");
    			system("echo 1800 > /sys/class/leds/beaglebone:green:usr0/delay_off");
				system("echo 200 > /sys/class/leds/beaglebone:green:usr0/delay_on");
    		}else if(network_status==NETWORK_LINK_3G){
    			// led
    			system("ifconfig eth0 192.168.1.199");
    			system("echo timer > /sys/class/leds/beaglebone:green:usr0/trigger");
    			system("echo 200 > /sys/class/leds/beaglebone:green:usr0/delay_off");
				system("echo 200 > /sys/class/leds/beaglebone:green:usr0/delay_on");
    		}
  		}
  		else {
 		   	total_reconnect_retry_cnt ++;
 		   	system("echo none > /sys/class/leds/beaglebone:green:usr0/trigger");
			system("echo 255 > /sys/class/leds/beaglebone:green:usr0/brightness");
 	   	}
    		


/*********************
**  daemon check
**********************/			
		// check daemon
		daemon_exist = daemon_daai_check();

		// daemon
		if(daemon_exist==1){
			//tr_missing_cnt = 0;
    		system("echo none > /sys/class/leds/beaglebone:green:usr2/trigger");
			system("echo 0 > /sys/class/leds/beaglebone:green:usr2/brightness");
   		}else{
   			//tr_missing_cnt ++;
    		system("echo none > /sys/class/leds/beaglebone:green:usr2/trigger");
			system("echo 255 > /sys/class/leds/beaglebone:green:usr2/brightness");
			//if(tr_missing_cnt>3){
			//	daemon_TR_stop_and_start();
			//	tr_missing_cnt = 0;
			//}
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

		if(network_status==NETWORK_LINK_LOST){
			// priority 1.3G, 2.Eth0, 3.Wifi, total_3g_retry_cnt <3 for Eth0, total_3g_retry_cnt <5 for ppp0, total_3g_retry_cnt <7 for Wlan
			system("route del default");
			
			if(total_reconnect_retry_cnt < 3){
				// 3.0 stop all other network
				network_eth0_stop();
				// set static ip back to 199
    			system("ifconfig eth0 192.168.1.199");
    			
    			network_wifi_stop();  
				// start ppp0 process
				// 3.1 stop pppd call
				network_3G_stop();
				// 3.3. wakeup ppp0 if rssi != 0
				network_3G_start();
				//sleep(3);
				//system("route del default");
				//system("route add default gw 10.64.64.64 ppp0");
			} else if (total_reconnect_retry_cnt < 6){
				// 1.0. stop all other network
				network_wifi_stop();
    			network_3G_stop();
				// 1.1. kill all dhclient ps
				network_eth0_stop();
				// set static ip back to 199
    			system("ifconfig eth0 192.168.1.199");

				// 1.2. do DHCP connect
   				network_eth0_start();
			} else if(total_reconnect_retry_cnt < 8){				
				// Wifi
				// 2.0. stop all other network
				network_eth0_stop();
				// set static ip back to 199
    			system("ifconfig eth0 192.168.1.199");	

    			network_3G_stop();

   				network_wifi_start();
			} else {
				puts("No available network interfaces, sleep 10 times, and then retry !!");
				if(total_reconnect_retry_cnt > 20)
					total_reconnect_retry_cnt = 0;
			}
			printf(" #%d/20 Network re-connect times !!\n", total_reconnect_retry_cnt);
		}

    	sleep(110);
  	}
  return 0;
}
