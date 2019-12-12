#include "core.h"
#include "str.h"
#include "log.h"
#include "platform.h"
#include "console.h"
#include "command.h"
#include "server.h"
#include "lang.h"

Thread conThread;

static cs_int32 ReadLine(char* buf, cs_int32 buflen) {
	cs_int32 len = 0, c = 0;

	while((c = File_GetChar(stdin)) != EOF && c != '\n' && len < buflen)
		if(c != '\r') buf[len++] = (char)c;
	while(c != '\n' && File_GetChar(stdin) != '\n') {}
	buf[len] = '\0';

	return len;
}

static void HandleCommand(char* cmd) {
	if(!Command_Handle(cmd, NULL))
		Log_Info(Lang_Get(LANG_CMDUNK));
}

THREAD_FUNC(ConsoleThread) {
	char buf[CON_STR_LEN] = {0};
	(void)param;

	while(Server_Active) {
		if(ReadLine(buf, CON_STR_LEN) > 0 && Server_Active)
			HandleCommand(buf);
	}
	return 0;
}

void Console_Start(void) {
	Thread_Create(ConsoleThread, NULL, true);
}
