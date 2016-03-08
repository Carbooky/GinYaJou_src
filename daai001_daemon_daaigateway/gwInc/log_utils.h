#ifndef LOG_UTILS__H
#define LOG_UTILS__H

#include <stdio.h>
#include <stdbool.h>

/*
# ifdef __cplusplus
extern "C" {
# endif
*/

// define for enbedded system
#define FILE_MAX_SIZE 2000000 // 2M bytes/file
#define FILE_NAME_LENGTH 55

typedef struct loger {
	FILE *fp;
	char file_name[FILE_NAME_LENGTH];
	int file_num;
	bool init_state;
} loger_t;

#define LOG_LEVEL  7

#define LOG_EMERG   0   /* system is unusable */
#define LOG_ALERT   1   /* action must be taken immediately */
#define LOG_CRIT    2   /* critical conditions */
#define LOG_ERR     3   /* error conditions */
#define LOG_WARNING 4   /* warning conditions */
#define LOG_NOTICE  5   /* normal but significant condition */
#define LOG_INFO    6   /* informational */
#define LOG_DEBUG   7   /* debug-level messages */

int log_msg(loger_t *loger, int pri, char* logmsg, ...);
loger_t* log_init(char* name, int num, char* path);
void log_close(loger_t* loger);
void log_clear(loger_t* loger);

/*
# ifdef __cplusplus
}
#endif
*/

#endif // LOG_UTILS_H
