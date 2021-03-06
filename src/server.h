#ifndef SERVER_H
#define SERVER_H
#include "config.h"

#define CFG_SERVERIP_KEY "server-ip"
#define CFG_SERVERPORT_KEY "server-port"
#define CFG_SERVERNAME_KEY "server-name"
#define CFG_SERVERMOTD_KEY "server-motd"
#define CFG_LOGLEVEL_KEY "log-level"
#define CFG_LOCALOP_KEY "always-local-op"
#define CFG_MAXPLAYERS_KEY "max-players"
#define CFG_CONN_KEY "max-connections-per-ip"
#define CFG_HEARTBEAT_KEY "heartbeat-enabled"
#define CFG_HEARTBEATDELAY_KEY "heartbeat-delay"
#define CFG_HEARTBEAT_PUBLIC_KEY "heartbeat-public"

VAR cs_bool Server_Active;
VAR CStore *Server_Config;
VAR cs_uint64 Server_StartTime;
Socket Server_Socket;

cs_bool Server_Init(void);
void Server_DoStep(cs_int32 delta);
void Server_StartLoop(void);
void Server_Stop(void);
#endif // SERVER_H
