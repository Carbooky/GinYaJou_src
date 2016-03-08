#ifndef _ZTC_TYPEDEF_H  
#define _ZTC_TYPEDEF_H

//#include "..\targetver.h"

#pragma pack(1)     /* set alignment to 1 byte boundary */

#define boolean_t BOOL

#define INT8 char
#define UINT8 unsigned char

#define int8_t char
//#define uint8_t unsigned char

typedef unsigned char uint8_t;

#define INT16 short
#define UINT16 unsigned short

#define int16_t short
//#define uint16_t unsigned short

typedef unsigned short uint16_t;

#define INT32 int
#define UINT32 unsigned int
#define int32_t int
//#define uint32_t unsigned int

typedef unsigned int uint32_t;

#define LongMacAddr_t  unsigned long long
//typedef unsigned long long LongMacAddr_t;
#define ShortNwkAddr_t uint16_t
//#define ShortNwkAddr_t unsigned short

//#define EndPoint_t uint8_t
#define EndPoint_t unsigned char

typedef uint8_t Energy3B_t[3];

struct Key16Byte_t {
	uint8_t key[16];
};

struct _DataTypeUINT8
{
	uint8_t data[1];
};

struct _DataTypeUINT16
{
	uint16_t data[2];
};

struct _DataTypeUINT24
{
	uint8_t data[3];
};

struct _DataTypeUINT32
{
	uint8_t data[4];
};

struct _DataTypeUINT64
{
	uint8_t data[8];
};

struct _DataTypeUINT128
{
	uint8_t data[16];
};

union _UNI_VARIABLE_SIZE_DATA
{
	struct _DataTypeUINT8		dataTypeUINT8;
	struct _DataTypeUINT16		dataTypeUINT16;
	struct _DataTypeUINT24		dataTypeUINT24;
	struct _DataTypeUINT32		dataTypeUINT32;
	struct _DataTypeUINT64		dataTypeUINT64;
	struct _DataTypeUINT128		dataTypeUINT128;
};


struct _DataTypeBytes128
{
	uint8_t data[128];
};


enum FRAMEFORMAT_POS{
	FRAMEFORMAT_POS_SYNC=0,
	FRAMEFORMAT_POS_OPGROUP,
	FRAMEFORMAT_POS_OPCODE,
	FRAMEFORMAT_POS_LENGTH,
	FRAMEFORMAT_POS_DATAFIELD_START,
};

#define ZTC_DATAFRAME_BUFSIZE (256+5)
#define ZTC_DATAFRAME_NAME_LEN (64)
struct _ZTC_DATAFRAME {
	char name[ZTC_DATAFRAME_NAME_LEN];
	uint8_t dataframe[ZTC_DATAFRAME_BUFSIZE];
};

/////////////////////////////////////////////////////////////
//OTA Header file
struct _OTA_HEADER_FIELDS
{
	uint32_t FileIdentifier;
	uint16_t HeaderVersion;
	uint16_t HeaderLength;
	uint16_t FieldControl;
	uint16_t ManufacturerCode;
	uint16_t ImageType;
	uint32_t FileVersion;
	uint16_t ZigBeeStackVerion;
	int8_t	 HeaderString[32];
	uint32_t TotalImageSize;
	//security 
	//file destination
	uint16_t MinHardwareVersion;
	uint16_t MaxHardwareVersion;
};

/////////////////////////////////////////////////////////////
#endif //_ZTC_TYPEDEF_H

