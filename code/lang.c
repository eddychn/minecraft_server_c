#include "core.h"
#include "lang.h"

const char* Lang_Get(int langid) {
	switch (langid) {
		case LANG_SUCC:
			return "successfully";
		case LANG_UNKERR:
			return "Unexpected error.";

		case LANG_SVSTART:
			return "%s started on %s:%d";
		case LANG_SVFULL:
			return "Server is full";
		case LANG_SVMANYCONN:
			return "Too many connections from one IP.";
		case LANG_SVPLCONN:
			return "Player %s connected with %s";
		case LANG_SVPLDISCONN:
			return "Player %s disconnected";
		case LANG_SVPLUGINLOAD:
			return "Loading C plugins";
		case LANG_HBEAT:
			return "Creating heartbeat thread";
		case LANG_SVDELTALT0:
			return "Time ran backwards? Server_Delta < 0.";
		case LANG_SVLONGTICK:
			return "Last server tick took %dms!";
		case LANG_SVLOOPDONE:
			return "Main loop done";
		case LANG_SVSTOP0:
			return "Kicking players";
		case LANG_SVSTOP1:
			return "Saving worlds";
		case LANG_SVSTOP2:
			return "Unloading plugins";
		case LANG_SVLOADING:
			return "Loading %s";
		case LANG_SVSAVING:
			return "Saving %s";

		case LANG_KICKNOREASON:
			return "Kicked without reason";
		case LANG_KICKPROTOVER:
			return "Invalid protocol version";
		case LANG_KICKNAMEUSED:
			return "This name already in use";
		case LANG_KICKAUTHFAIL:
			return "Auth failed";
		case LANG_KICKSVSTOP:
			return "Server stopped";
		case LANG_KICKMAPFAIL:
			return "Map sending failed";
		case LANG_KICKPACKETREAD:
			return "Packet reading error";
		case LANG_KICKBLOCKID:
			return "Invalid block ID";
		case LANG_KICKPACKETSPAM:
			return "Too many packets per second";

		case LANG_PLNAMEUNK:
			return "unconnected";

		case LANG_CPEVANILLA:
			return "vanilla";

		case LANG_CMDUSAGE:
			return "Usage: %s";
		case LANG_CMDAD:
			return "Access denied";
		case LANG_CMDPLNF:
			return "Player not found";
		case LANG_CMDUNK:
			return "Unknown command";
		case LANG_CMDONLYCL:
			return "This command can't be used from console.";
		case LANG_CMDONLYCON:
			return "This command can be used only from console.";

		case LANG_CPLD:
			return "loaded";
		case LANG_CPUNLD:
			return "unloaded";
		case LANG_CPCB:
			return "can't be";
		case LANG_CPNL:
			return "is not loaded";
		case LANG_CPINF0:
			return "Plugin \"%s\" %s.";
		case LANG_CPINF1:
			return "Plugin \"%s\" %s %s.";
		case LANG_CPLIST:
			return "Loaded plugins list:";
		case LANG_CPINVNAME:
			return "Invalid plugin name";
		case LANG_CPAPIOLD:
			return "Plugin \"%s\" is too old. Server uses PluginAPI v%03d, but plugin compiled for v%03d.";
		case LANG_CPAPIUPG:
			return "Please upgrade your server software. Plugin \"%s\" compiled for PluginAPI v%03d, but server uses v%d.";

		case LANG_DBGSYM0:
			return "Symbol: %s - 0x%0X";
		case LANG_DBGSYM1:
			return "\tFile: %s: %d";

		case LANG_HBRESPERR:
			return "Heartbeat server responded with an error %d";
		case LANG_HBRR:
			return "request reading";
		case LANG_HBRSP:
			return "response";
		case LANG_HBERR:
			return "%s error: %d";
		case LANG_HBPLAY:
			return "Server play URL: %s";

		case LANG_WSNOTVALID:
			return "Not a websocket connection.";

		case LANG_ERRFMT:
			return "%s: %d in function %s: %s";
	}

	return "Invalid langid";
}
