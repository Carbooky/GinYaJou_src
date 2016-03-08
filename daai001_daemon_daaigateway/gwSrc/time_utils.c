#include <time.h>
#include <stdio.h>
#include <sys/time.h>
#include <stdlib.h>
#include <pthread.h>

#include "time_utils.h"

pthread_mutex_t time_mutex = PTHREAD_MUTEX_INITIALIZER;

char utcTimeStr[sizeof(struct tm)] = "";
char currMonthStr[sizeof(struct tm)] = "N/A";

char* Time_getCurrent() {
	pthread_mutex_lock(&time_mutex);
	time_t timeCurrent;
	struct tm* cTime;

	time(&timeCurrent);
	cTime = gmtime(&timeCurrent);

	strftime(utcTimeStr, sizeof(struct tm), "%FT%T+08:00", cTime);
	pthread_mutex_unlock(&time_mutex);
	return utcTimeStr;
}

long long Time_getCurrentMicrosecond() {
	long long ms = 0;
	struct timeval tv;
	gettimeofday(&tv, NULL);
	ms = tv.tv_sec * 1000000 + tv.tv_usec;
	return ms;
}

char* Month_getCurrent(){
	pthread_mutex_lock(&time_mutex);
	time_t timeCurrent;
	struct tm* cTime;

	time(&timeCurrent);
	cTime = gmtime(&timeCurrent);

	strftime(currMonthStr, sizeof(struct tm), "%b", cTime);
	pthread_mutex_unlock(&time_mutex);	
	return currMonthStr;
}
