#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <semaphore.h>

int main(void){
int fd;

/*
  fd = open("/dev/watchdog", O_RDWR|O_NOCTTY);
  if(fd < 0){
    printf("Fail to open \n");
    return -1;
  }

  puts("Activate hardware timer !!");
  write(fd, "1", 1);

  //puts("Force a kernel panic");
  //system("echo c > /proc/sysrq_trigger");

  while(1){
    sleep(1);
  }

  puts("Success");

  close(fd);
*/

#define IP_TO_PROBE "csac.dtdns.net"
#define TOTAL_PING_COUNT 5

  int rtn, ping_count, ping_qualify;
  char rtnStr[24];
  char cmd[1024];
  int rtn_3G_exist = 0;

  bzero(rtnStr, 24);
  sprintf(cmd, "ping -c %d %s | grep 'received' | awk -F',' '{ print $2 }' | awk '{ print $1 }'", TOTAL_PING_COUNT, IP_TO_PROBE); 
  rtn = syscall_getRsp(cmd, rtnStr);
  ping_count = atoi(rtnStr);
  ping_qualify = TOTAL_PING_COUNT >> 1; // make half of the total as the qualify standard 
  if(ping_count > ping_qualify){
    puts(" network OK !!");
    total_3g_retry_cnt = 0;
  }
  else {
    total_3g_retry_cnt ++;

  // kill current 3G process
    puts("Disconnected !!! Try to re-connect 3G network"); 
  
//    system("./ppp-stop_adv");

    bzero(cmd, 1024);
    bzero(rtnStr, 24);
    sprintf(cmd, "ps aux |awk '/pppd call wcdma/{print $2 " " $11}'|sed '2,10d'");
    syscall_getRsp(cmd, rtnStr);
    printf("rtn=%s\n", rtnStr);
    
    sleep(5);

    rtn_3G_exist = get_sim5215_info(DEV_3G, info_3G, sizeof(info_3G));

    if(rtn_3G_exist > 0) { // 3G does response, but not sure whether we can connect
      if(total_3g_retry_cnt < 3){
        system("pppd call wcdma &");
        sleep(7);
        system("route del default; route add default gw 10.64.64.64 ppp0");
      }else{
        system("dhclient eth0 &");
        total_3g_retry_cnt = 0;
      }
    }else{ // no 3G
      system("dhclient eth0 &");
      total_3g_retry_cnt = 0;
    }
  }




  return 0;
}
