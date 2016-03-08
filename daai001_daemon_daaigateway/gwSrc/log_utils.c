#include <sys/stat.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>

#include "time_utils.h"
#include "common.h"
#include "pthread_utils.h"

#include "log_utils.h"

#define LOG_PATH "log"
#define BACKUP_PERIODIC 3600 // one hours

pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_t backup_thread;

char m_ramlogPath[128] = "";

char *prioritynames[8] = { "emerg", "alert", "crit", "err", "warning", "notice", "info", "debug" };

void* backup(void* param) {
	char cmd[256] = "";
	sprintf(cmd, "cp -fr %s .", m_ramlogPath);
	while(true) {
		sleep(BACKUP_PERIODIC);
		system(cmd);
	}
}

int log_msg(loger_t *loger, int pri, char* logmsg, ...) {
	if (loger == NULL)
		return -1;
	if (logmsg == NULL)
		return 0;

	pthread_mutex_lock(&log_mutex);

	FILE *fp;

	char message_buf[22000] = "";
	int pidno = 0;
	pidno = getpid();

	if (pri <= LOG_LEVEL && loger->init_state) {

		/*
		 * Format: Fri Mar 20 07:15:42 2009 [pid:1234][err]: data
		 */
		va_list ap;
		char buf[20000] = "";
		va_start(ap, logmsg);
		vsprintf(buf, logmsg, ap);
		va_end(ap);

		sprintf(message_buf, "%s [pid:%d] [%s]: %s\n", Time_getCurrent(), pidno, prioritynames[pri], buf);

		char file_name[256] = "";
		sprintf(file_name, "%s/log%s.0", m_ramlogPath, loger->file_name);

		//dprintf("%s", message_buf);
		fprintf(loger->fp, "%s", message_buf);
		fflush(loger->fp);

		struct stat st;
		stat(file_name, &st);
		int size = st.st_size;

		if (size > FILE_MAX_SIZE) {
			char file[256] = "";
			char file2[256] = "";
			int i = loger->file_num - 1;
			while (i > 0) {
				sprintf(file2, "%s/log%s.%d", m_ramlogPath, loger->file_name, i);
				sprintf(file, "%s/log%s.%d", m_ramlogPath, loger->file_name, i - 1);
				rename(file, file2);
				i--;
			}
			fp = fopen(file_name, "a");
			if (fp != NULL) {
				fclose(loger->fp);
				loger->fp = fp;
			}

		}

	}
	pthread_mutex_unlock(&log_mutex);
	return 0;
}

loger_t* log_init(char* name, int num, char* path) {
	if (name == NULL)
		return NULL;
	if (strlen(name) > FILE_NAME_LENGTH)
		return NULL; // log*.0

	char cmd[256] = "";
	if (path == NULL) {
		sprintf(m_ramlogPath, "%s", LOG_PATH);
	} else {
		sprintf(m_ramlogPath, "%s/%s", path, LOG_PATH);
	}

	sprintf(cmd, "mkdir -p %s", m_ramlogPath);

	struct stat st;
	int systemResult = 0;
	if (stat(m_ramlogPath, &st) != 0) {
		systemResult = system(cmd);
		if (systemResult > 0) return NULL;
	}

	loger_t* loger = (loger_t *) malloc(sizeof(loger_t));

	if (loger == NULL) {
		return NULL;
	}

	FILE *fp;
	char file[256] = ""; // <-- too long file name maybe error
	int i = 0;

	// check each log file exist...
	for (i = 0; i < num; i++) {
		sprintf(file, "%s/log%s.%d", m_ramlogPath, name, i);
		//remove(file);
		if (!(fp = fopen(file, "a+"))) {
			eprintf("create %s file fail.\n", file);
			safe_free(loger);
			return NULL;
		} else {
			dprintf("%s is exist\n", file);
			if (i == 0) {
				loger->fp = fp;
			} else {
				fclose(fp);
			}
		}
	}

	strcpy(loger->file_name, name);
	loger->file_num = num;
	loger->init_state = true;

	// periodic backup to SD
	int intResult = pthread_create(&backup_thread, NULL, (void*) backup, NULL);

	if (intResult < 0) {
		dprintf("failed to start backup thread.\n");
		return NULL;
	}

	return loger;
}

void log_close(loger_t *loger) {
	if (loger == NULL)
		return;
	fclose(loger->fp);
	safe_free(loger);
	Pthread_cancel(backup_thread);
}

void log_clear(loger_t *loger) {
	if (loger == NULL)
		return;
	int num = loger->file_num;

	char file[256] = "";
	int i = 0;

	for (i = 0; i < num; i++) {
		sprintf(file, "%s/log%s.%d", m_ramlogPath, loger->file_name, i);
		remove(file);
	}
	log_close(loger);
}
