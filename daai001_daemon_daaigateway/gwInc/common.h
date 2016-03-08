/////////////////////////////////////////////////////////////////////////////
//                                                                         //
// <common.h> - Common module.                                             //
//                                                                         //
// Version :        0.0.1                                                  //
// Language :       C                                                      //
// Platform :       Linux                                                  //
// Application :    TR069                                                  //
// Author :         Ireta                                                  //
// Created Date :   2012_07_27                                             //
// Header :                                                                //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//                           Maintenance                                   //
/////////////////////////////////////////////////////////////////////////////

/*
  Version  Modified By  Date        Remarks
=============================================================================
  0.0.1	   Ireta        2012-07-27  Basic initiate
*/

#ifndef COMMON_H
#define COMMON_H

/////////////////////////////////////////////////////////////////////////////
//                           Headers                                       //
/////////////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include <stdio.h>

#include "boolean_utils.h"

# ifdef __cplusplus
extern "C" {
# endif

#ifndef DEBUG
#define DEBUG true
#endif

#ifndef ERROR
#define ERROR true
#endif
#if DEBUG
#define dprintf(format, args...) {printf("[DEBUG: %s:%d] "format, __FILE__, __LINE__, ##args); fflush(stdout);}
#else
#define dprintf(format, args...)
#endif

#if ERROR
#define eprintf(format, args...) {printf("[ERROR: %s:%d] "format, __FILE__, __LINE__, ##args); fflush(stdout);}
#else
#define eprintf(format, args...)
#endif

#define safe_free(x) {if (x) free(x);}

#define MEM_CLEAR(x) memset (&(x), 0x00, sizeof (x))

#define ARRAY_SIZE(a)  (sizeof(a)/sizeof(a[0]))

/////////////////////////////////////////////////////////////////////////////
//                      Constant Declarations                              //
/////////////////////////////////////////////////////////////////////////////

#define STRING_NORMAL_LENGTH	256
#define STRING_MAX_LENGTH		1024

# ifdef __cplusplus
}
# endif

#endif // COMMON_H
