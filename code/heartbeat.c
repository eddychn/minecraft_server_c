#include "core.h"
#include "str.h"
#include "platform.h"
#include "http.h"
#include "server.h"
#include "svmath.h"
#include "heartbeat.h"
#include "client.h"

#define HBEAT_URL "/server/heartbeat/?name=%s&port=%d&users=%d&max=%d&salt=%s&public=%s&web=true&software=%s"
#define PLAY_URL "http://www.classicube.net/server/play/"
#define PLAY_URL_LEN 38

const char* SoftwareName = SOFTWARE_NAME "%%47" SOFTWARE_VERSION;
uint32_t Delay = 5000;
THREAD Thread;

static void NewSecret(void) {
	RNGState secrnd;
	Random_Seed(&secrnd, (int)Time_GetMSec());
	for(int i = 0; i < 16; i++) {
		int min, max;
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
		Heartbeat_Secret[i] = (char)Random_Range(&secrnd, min, max);
	}
}

const char* reserved = "!*'();:@&=+$,/?#[]";
static void TrimReserved(char* name, int len) {
	for(int i = 0; i < len; i++) {
		char sym = name[i];
		if(sym == '\0') break;
		if(sym == ' ') name[i] = '+';
		if(String_LastChar(reserved, sym)) name[i] = '_';
	}
}

static void DoRequest() {
	if(*Heartbeat_Secret == '\0') NewSecret();
	struct httpRequest req = {0};
	struct httpResponse resp = {0};
	char path[512] = {0};
	char name[33] = {0};
	String_Copy(name, 33, Config_GetStr(Server_Config, CFG_SERVERNAME_KEY));
	TrimReserved(name, 33);

	uint16_t port = Config_GetInt16(Server_Config, CFG_SERVERPORT_KEY);
	bool public = Config_GetBool(Server_Config, CFG_HEARTBEAT_PUBLIC_KEY);
	uint8_t max = Config_GetInt8(Server_Config, CFG_MAXPLAYERS_KEY);
	uint8_t count = Clients_GetCount(STATE_INGAME);
	String_FormatBuf(path, 512, HBEAT_URL, name, port, count, max, Heartbeat_Secret, public ? "true" : "false", SoftwareName);

	SOCKET fd = Socket_New();
	req.sock = fd;

	HttpRequest_SetHost(&req, "classicube.net", 80);
	HttpRequest_SetPath(&req, path);
	HttpRequest_SetHeaderStr(&req, "Pragma", "no-cache");
	HttpRequest_SetHeaderStr(&req, "Connection", "close");

	if(HttpRequest_Perform(&req, &resp)) {
		if(!Heartbeat_URL && resp.body && resp.code == 200) {
			if(String_CaselessCompare2(resp.body, PLAY_URL, PLAY_URL_LEN)) {
				Heartbeat_URL = String_AllocCopy(resp.body);
				Log_Info("Server play URL: %s", resp.body);
			}
		}
		if(resp.code != 200)
			Log_Error("Heartbeat server responded with an error %d", resp.code);
	} else {
		if(req.error == HTTP_ERR_RESPONSE_READ)
			Log_Error("Response reading error: %d", resp.error);
		else
			Log_Error("Request sending error: %d", req.error);
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

void Heartbeat_Start(uint32_t delay) {
	Delay = delay * 1000;
	Thread = Thread_Create(HeartbeatThreadProc, NULL);
}

void Heartbeat_Close(void) {
	if(Thread_IsValid(Thread)) Thread_Close(Thread);
}
