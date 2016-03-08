#ifndef TIME_UTIL_H
#define TIME_UTIL_H

# ifdef __cplusplus
extern "C" {
# endif

char* Time_getCurrent();

long long Time_getCurrentMicrosecond();

char* Month_getCurrent();

# ifdef __cplusplus
}
# endif

#endif // TIME_UTIL_H
