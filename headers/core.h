#ifndef CORE_H
#define CORE_H
#if defined(_WIN32)
#  define WINDOWS
#  define PATH_DELIM '\\'
#  define DLIB_EXT "dll"
#  define WIN32_LEAN_AND_MEAN
#  define _CRT_SECURE_NO_WARNINGS
#  include <winsock2.h>
#  include <ws2tcpip.h>
#  include <stdint.h>

#  define ZLIB_WINAPI
#  ifndef PLUGIN_BUILD
#    define API __declspec(dllexport, noinline)
#    define VAR __declspec(dllexport)
#  else
#    define API __declspec(dllimport)
#    define VAR __declspec(dllimport)
#    define EXP __declspec(dllexport)
#  endif

typedef void* ITER_DIR;
typedef WIN32_FIND_DATA ITER_FILE;
typedef void* Thread;
typedef uint32_t TRET;
typedef void* Waitable;
typedef CRITICAL_SECTION Mutex;
typedef SOCKET Socket;
#elif defined(__unix__)
#  define POSIX
#  define PATH_DELIM '/'
#  define DLIB_EXT "so"
#  define _GNU_SOURCE
#  include <errno.h>
#  include <stdint.h>
#  include <sys/socket.h>
#  include <sys/stat.h>
#  include <sys/time.h>
#  include <sys/types.h>
#  include <arpa/inet.h>
#  include <unistd.h>
#  include <dlfcn.h>
#  include <pthread.h>
#  include <dirent.h>
#  include <netdb.h>

#  ifndef PLUGIN_BUILD
#    define API __attribute__((visibility("default"), noinline))
#    define VAR __attribute__((visibility("default")))
#  else
#    define API
#    define VAR
#    define EXP __attribute__((__visibility__("default")))
#  endif

#  define min(a, b) (((a)<(b))?(a):(b))
#  define max(a, b) (((a)>(b))?(a):(b))
#  define Sleep(ms) (usleep(ms * 1000))
#  define INVALID_SOCKET -1
#  define SD_SEND   SHUT_WR
#  define MAX_PATH  PATH_MAX

typedef DIR* ITER_DIR;
typedef struct dirent* ITER_FILE;
typedef void* TRET;
typedef pthread_t* Thread;
typedef pthread_mutex_t Mutex;
typedef struct _Waitable {
	Mutex mutex;
	pthread_cond_t cond;
} *Waitable;
typedef int32_t Socket;
#else
#  error Unknown OS
#endif

#include <stdio.h>
#include <zlib.h>

#ifndef true
#  define true  1
#  define false 0
#endif

typedef uint32_t bool;
typedef uint8_t Order;
typedef uint8_t BlockID;
typedef uint8_t Weather;
typedef uint8_t ClientID;
typedef uint8_t MessageType;
typedef void* TARG;
typedef TRET(*TFUNC)(TARG);

#ifdef PLUGIN_BUILD
EXP bool Plugin_Load();
EXP bool Plugin_Unload();
EXP int32_t Plugin_ApiVer;
#endif

#define SOFTWARE_NAME "C-Server"
#define SOFTWARE_FULLNAME SOFTWARE_NAME "/" GIT_COMMIT_SHA
#define CHATLINE "<%s>: %s"
#define MAINCFG "server.cfg"
#define WORLD_MAGIC 0x54414457
#define PLUGIN_API_NUM 1

#define MAX_PLUGINS 32
#define	MAX_CMD_OUT 1024
#define MAX_CLIENT_PPS 128
#define MAX_CFG_LEN 128
#define MAX_CLIENTS 128
#define MAX_PACKETS 256
#define MAX_WORLDS 64
#define MAX_EVENTS 64
#define EVENT_TYPES 15

#define ISHEX(ch) ((ch > '/' && ch < ':') || (ch > '@' && ch < 'G') || (ch > '`' && ch < 'g'))
#define MODE(b) (b ? Lang_Get(LANG_ENABLED) : Lang_Get(LANG_DISABLED))

typedef struct _Color4 {
	int16_t r, g, b, a;
} Color4;

typedef struct _Color3 {
	int16_t r, g, b;
} Color3;
#endif
