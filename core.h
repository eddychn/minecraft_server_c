#ifndef CORE_H
#define CORE_H
#define ZLIB_WINAPI
#include "error.h"
#include "log.h"

typedef unsigned short ushort;
typedef unsigned char  uchar;
typedef int            bool;
typedef unsigned int   uint;
typedef unsigned char  BlockID;
typedef unsigned char  ClientID;

#define true  1
#define false 0

#define DELIM " "
#define SOFTWARE_NAME "C-Server"
#define SOFTWARE_VERSION "0.1"
#define CHATLINE "<%s>: %s"

#define CPE_CHAT      0
#define CPE_STATUS1   1
#define CPE_STATUS2   2
#define CPE_STATUS3   3
#define CPE_BRIGHT1   11
#define CPE_BRIGHT2   12
#define CPE_BRIGHT3   13
#define CPE_ANNOUNCE  100

#define ISHEX(ch) ((ch > '/' && ch < ':') || (ch > '@' && ch < 'G') || (ch > '`' && ch < 'g'))

typedef struct ext {
	char* name;
	int   version;
	struct ext*  next;
} EXT;

typedef struct vector {
	float x;
	float y;
	float z;
} VECTOR;

typedef struct svector {
	short x;
	short y;
	short z;
} SVECTOR;

typedef struct angle {
	float yaw;
	float pitch;
} ANGLE;
#endif
