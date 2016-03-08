#include <pthread.h>
#include <signal.h>
#include <stdio.h>

void Pthread_exit_handler(int sig) {
	printf("this signal is %d \n", sig);
	pthread_exit(0);
}

int Pthread_cancel(pthread_t thread) {
#ifdef ANDROID_OS
	return pthread_kill(thread, SIGUSR1);
#else
	return pthread_cancel(thread);
#endif
}

void Pthread_init() {
	signal(SIGUSR1, Pthread_exit_handler);
}
