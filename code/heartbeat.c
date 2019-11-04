#include "core.h"
#include "str.h"
#include "log.h"
#include "server.h"
#include "random.h"
#include "heartbeat.h"
#include "platform.h"
#include "http.h"
#include "lang.h"
#include "hash.h"

#define HBEAT_URL "/server/heartbeat/?name=%s&port=%d&users=%d&max=%d&salt=%s&public=%s&web=true&software=%s"
#define PLAY_URL "http://www.classicube.net/server/play/"
#define PLAY_URL_LEN 38

const char* PlayURL = NULL;
char Secret[17] = {0};
cs_uint32 Delay = 5000;

static void NewSecret(void) {
	RNGState secrnd;
	Random_SeedFromTime(&secrnd);
	for(cs_int32 i = 0; i < 16; i++) {
		cs_int32 min, max;
		switch(Random_Range(&secrnd, 0, 3)) {
			case 0:
				min = 48;
				max = 57;
				break;
			case 1:
				min = 65;
				max = 90;
				break;
			default:
				min = 97;
				max = 122;
				break;
		}
		Secret[i] = (char)Random_Range(&secrnd, min, max);
	}
}

const char* reserved = "!*'();:@&=+$,/?#[]%";
static void TrimReserved(char* name, cs_int32 len) {
	for(cs_int32 i = 0; i < len; i++) {
		char sym = name[i];
		if(sym == '\0') break;
		if(sym == ' ') name[i] = '+';
		if(String_LastChar(reserved, sym)) name[i] = '.';
	}
}

static void DoRequest() {
	if(*Secret == '\0') NewSecret();
	struct httpRequest req = {0};
	struct httpResponse resp = {0};
	char path[512] = {0};
	char name[65] = {0};
	String_Copy(name, 65, Config_GetStr(Server_Config, CFG_SERVERNAME_KEY));
	TrimReserved(name, 65);

	cs_uint16 port = Config_GetInt16(Server_Config, CFG_SERVERPORT_KEY);
	bool public = Config_GetBool(Server_Config, CFG_HEARTBEAT_PUBLIC_KEY);
	cs_uint8 max = Config_GetInt8(Server_Config, CFG_MAXPLAYERS_KEY);
	cs_uint8 count = Clients_GetCount(STATE_INGAME);
	String_FormatBuf(path, 512, HBEAT_URL,
		name,
		port,
		count,
		max,
		Secret,
		public ? "true" : "false",
		SOFTWARE_NAME "%%47" GIT_COMMIT_SHA
	);

	Socket fd = Socket_New();
	req.sock = fd;

	HttpRequest_SetHost(&req, "classicube.net", 80);
	HttpRequest_SetPath(&req, path);
	HttpRequest_SetHeaderStr(&req, "Pragma", "no-cache");
	HttpRequest_SetHeaderStr(&req, "Connection", "close");

	if(HttpRequest_Perform(&req, &resp)) {
		if(!PlayURL && resp.body && resp.code == 200) {
			if(String_CaselessCompare2(resp.body, PLAY_URL, PLAY_URL_LEN)) {
				PlayURL = String_AllocCopy(resp.body);
				Log_Info(Lang_Get(LANG_HBPLAY), resp.body);
			}
		}
		if(resp.code != 200)
			Log_Error(Lang_Get(LANG_HBRESPERR), resp.code);
	} else {
		if(req.error == HTTP_ERR_RESPONSE_READ)
			Log_Error(Lang_Get(LANG_HBERR), Lang_Get(LANG_HBRR), resp.error);
		else
			Log_Error(Lang_Get(LANG_HBERR), Lang_Get(LANG_HBRSP), resp.error);
	}

	Socket_Close(fd);
	HttpRequest_Cleanup(&req);
	HttpResponse_Cleanup(&resp);
}

static TRET HeartbeatThreadProc(TARG param) {
	(void)param;
	while(true) {
		DoRequest();
		Sleep(Delay);
		if(!Server_Active) break;
	}
	return 0;
}

static const char hexchars[] = "0123456789abcdef";

bool Heartbeat_CheckKey(Client client) {
	if(*Secret == '\0') return true;
	const char* key = client->playerData->key;
	const char* name =  client->playerData->name;

	MD5_CTX ctx = {0};
	cs_uint8 hash[16] = {0};
	char hash_hex[16 * 2 + 1] = {0};

	MD5_Init(&ctx);
	MD5_Update(&ctx, Secret, String_Length(Secret));
	MD5_Update(&ctx, name, String_Length(name));
	MD5_Final(hash, &ctx);

	for(cs_int32 i = 0; i < 16; i++) {
		cs_uint8 b = hash[i];
		hash_hex[i * 2] = hexchars[b >> 4];
		hash_hex[i * 2 + 1] = hexchars[b & 0xF];
	}

	return String_CaselessCompare(hash_hex, key);
}

void Heartbeat_Start(cs_uint32 delay) {
	Delay = delay * 1000;
	Thread_Create(HeartbeatThreadProc, NULL, true);
}
