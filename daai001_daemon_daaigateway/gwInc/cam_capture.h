#ifndef CAM_CAPTURE_H
#define CAM_CAPTURE_H

#include "format.h"

bool_t CamCapture_startCapture(char* targetFilePath, char* targetFileName, char* devVideoPath);

char* Time_getCurrent();

long long Time_getCurrentMicrosecond();

#endif /* CAM_CAPTURE_H */
