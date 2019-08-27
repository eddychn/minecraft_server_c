#include "core.h"
#include "zlib.h"
#include "error.h"

const char* const Error_Strings[] = {
	"All ok",
};

char Error_WinBuf[512] = {0};
int Error_Type = ET_SERVER;
int Error_Code = 0;

const char* Error_GetString() {
	int len;

	switch(Error_Type) {
		case ET_SERVER:
			return Error_Strings[Error_Code];
		case ET_ZLIB:
			return zError(Error_Code);
		case ET_WIN:
			len = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, Error_Code, 0, Error_WinBuf, 512, NULL);
			if(len > 0) {
				Error_WinBuf[len - 1] = 0;
				Error_WinBuf[len - 2] = 0;
			}
			return Error_WinBuf;
		default:
			return Error_Strings[0];
	}
}

void Error_SetCode(char* efile, int eline, char* efunc, int etype, int ecode) {
	Error_File = efile;
	Error_Line = eline;
	Error_Func = efunc;
	Error_Type = etype;
	Error_Code = ecode;
}

char* Error_GetFunc() {
	if(Error_Func)
		return Error_Func;
	else
		return "[unknown function]";
}

char* Error_GetFile() {
	if(Error_File)
		return Error_File;
	else
		return "[unknown file]";
}

void Error_SetSuccess() {
	Error_Type = ET_NOERR;
	Error_File = NULL;
	Error_Func = NULL;
	Error_Line = -1;
	Error_Code = 0;
}
