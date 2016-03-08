#ifndef PTHREAD_UTILS_H
#define PTHREAD_UTILS_H

#include <pthread.h>

# ifdef __cplusplus
extern "C" {
# endif

void Pthread_exit_handler(int sig);

int Pthread_cancel(pthread_t thread);

void Pthread_init();

# ifdef __cplusplus
}
#endif

#endif // PTHREAD_UTILS_H
