#include "core.h"
#include "platform.h"
#include "str.h"
#include "block.h"
#include "client.h"
#include "cpe.h"
#include "packets.h"
#include "event.h"

EXT firstExtension;
uint16_t extensionsCount;

void CPE_RegisterExtension(const char* name, int version) {
	EXT tmp = Memory_Alloc(1, sizeof(struct cpeExt));

	tmp->name = name;
	tmp->version = version;
	tmp->next = firstExtension;
	firstExtension = tmp;
	++extensionsCount;
}

void CPE_StartHandshake(CLIENT client) {
	CPEPacket_WriteInfo(client);
	EXT ptr = firstExtension;
	while(ptr) {
		CPEPacket_WriteExtEntry(client, ptr);
		ptr = ptr->next;
	}
}

struct extReg {
	const char* name;
	int version;
};

static const struct extReg serverExtensions[] = {
	{"FastMap", 1},
	{"EmoteFix", 1},
	{"FullCP437", 1},
	{"SetHotbar", 1},
	{"HeldBlock", 1},
	{"TwoWayPing", 1},
	{"HackControl", 1},
	{"PlayerClick", 1},
	{"ChangeModel", 1},
	{"MessageTypes", 1},
	{"ClickDistance", 1},
	{"LongerMessages", 1},
	{"InventoryOrder", 1},
	{"EnvWeatherType", 1},
	{"BlockPermissions", 1},
	{"ExtEntityPositions", 1},
	{NULL, 0}
};

#define MODELS_COUNT 15

static const char* validModelNames[MODELS_COUNT] = {
	"humanoid",
	"chicken",
	"creeper",
	"pig",
	"sheep",
	"skeleton",
	"sheep",
	"sheep_nofur",
	"skeleton",
	"spider",
	"zombie",
	"head",
	"sit",
	"chibi",
	NULL
};

bool CPE_CheckModel(int16_t model) {
	if(model < 256) return Block_IsValid((BlockID)model);
	model -= 256; return model < MODELS_COUNT;
}

int16_t CPE_GetModelNum(const char* model) {
	int16_t modelnum = -1;
	for(int16_t i = 0; validModelNames[i]; i++) {
		const char* cmdl = validModelNames[i];
		if(String_CaselessCompare(model, cmdl)) {
			modelnum = i + 256;
			break;
		}
	}
	if(modelnum == -1) {
		int tmp = String_ToInt(model);
		if(tmp < 0 || tmp > 255)
			modelnum = 256;
		else
			modelnum = (int16_t)tmp;
	}

	return modelnum;
}

const char* CPE_GetModelStr(int16_t num) {
	return num >= 0 && num < MODELS_COUNT ? validModelNames[num] : NULL;
}

void Packet_RegisterCPEDefault(void) {
	const struct extReg* ext;
	for(ext = serverExtensions; ext->name; ext++) {
		CPE_RegisterExtension(ext->name, ext->version);
	}
	Packet_Register(0x10, "ExtInfo", 66, CPEHandler_ExtInfo);
	Packet_Register(0x11, "ExtEntry", 68, CPEHandler_ExtEntry);
	Packet_Register(0x2B, "TwoWayPing", 3, CPEHandler_TwoWayPing);
	Packet_Register(0x22, "PlayerClick", 14, CPEHandler_PlayerClick);
	Packet_RegisterCPE(0x08, EXT_ENTPOS, 1, 15, NULL);
}

void CPEPacket_WriteInfo(CLIENT client) {
	PacketWriter_Start(client);

	*data = 0x10;
	WriteNetString(++data, SOFTWARE_FULLNAME); data += 63;
	*(uint16_t*)++data = htons(extensionsCount);

	PacketWriter_End(client, 67);
}

void CPEPacket_WriteExtEntry(CLIENT client, EXT ext) {
	PacketWriter_Start(client);

	*data = 0x11;
	WriteNetString(++data, ext->name); data += 63;
	*(uint32_t*)++data = htonl(ext->version);

	PacketWriter_End(client, 69);
}

void CPEPacket_WriteClickDistance(CLIENT client, short dist) {
	PacketWriter_Start(client);

	*data = 0x12;
	*(short*)++data = dist;

	PacketWriter_End(client, 3);
}

void CPEPacket_WriteInventoryOrder(CLIENT client, Order order, BlockID block) {
	PacketWriter_Start(client);

	*data = 0x2C;
	*++data = block;
	*++data = order;

	PacketWriter_End(client, 3);
}

void CPEPacket_WriteHoldThis(CLIENT client, BlockID block, bool preventChange) {
	PacketWriter_Start(client);

	*data = 0x14;
	*++data = block;
	*++data = (char)preventChange;

	PacketWriter_End(client, 3);
}

void CPEPacket_WriteHackControl(CLIENT client, HACKS hacks) {
	PacketWriter_Start(client);

	*data = 0x20;
	*++data = (char)hacks->flying;
	*++data = (char)hacks->noclip;
	*++data = (char)hacks->speeding;
	*++data = (char)hacks->spawnControl;
	*++data = (char)hacks->tpv;
	*(short*)++data = hacks->jumpHeight;

	PacketWriter_End(client, 8);
}

void CPEPacket_WriteSetHotBar(CLIENT client, Order order, BlockID block) {
	PacketWriter_Start(client);

	*data = 0x2D;
	*++data = block;
	*++data = order;

	PacketWriter_End(client, 3);
}

void CPEPacket_WriteWeatherType(CLIENT client, Weather type) {
	PacketWriter_Start(client);

	*data = 0x1F;
	*++data = type;

	PacketWriter_End(client, 2);
}

void CPEPacket_WriteTexturePack(CLIENT client, const char* url) {
	PacketWriter_Start(client);

	*data = 0x28;
	WriteNetString(++data, url);

	PacketWriter_End(client, 65);
}

void CPEPacket_WriteMapProperty(CLIENT client, uint8_t property, int value) {
	PacketWriter_Start(client);

	*data = 0x29;
	*++data = property;
	*(int*)++data = htonl(value);

	PacketWriter_End(client, 6);
}

void CPEPacket_WriteTwoWayPing(CLIENT client, uint8_t direction, short num) {
	PacketWriter_Start(client);
	if(client->playerData->state != STATE_INGAME) {
		PacketWriter_Stop(client);
	}

	*data = 0x2B;
	*++data = direction;
	*(uint16_t*)++data = num;

	PacketWriter_End(client, 4);
}

void CPEPacket_WriteSetModel(CLIENT client, ClientID id, int16_t model) {
	PacketWriter_Start(client);

	*data = 0x01D;
	*++data = id;
	if(model < 256) {
		char modelname[4];
		String_FormatBuf(modelname, 4, "%d", model);
		WriteNetString(++data, modelname);
	} else
		WriteNetString(++data, CPE_GetModelStr(model - 256));

	PacketWriter_End(client, 66);
}

void CPEPacket_WriteBlockPerm(CLIENT client, BlockID id, bool allowPlace, bool allowDestroy) {
	PacketWriter_Start(client);

	*data = 0x1C;
	*++data = id;
	*++data = (char)allowPlace;
	*++data = (char)allowDestroy;

	PacketWriter_End(client, 4);
}

/*
	CPE packet handlers
*/

bool CPEHandler_ExtInfo(CLIENT client, const char* data) {
	ValidateCpeClient(client, false);
	ValidateClientState(client, STATE_MOTD, false);

	if(!ReadNetString(&data, &client->cpeData->appName)) return false;
	client->cpeData->_extCount = ntohs(*(uint16_t*)data);
	return true;
}

bool CPEHandler_ExtEntry(CLIENT client, const char* data) {
	ValidateCpeClient(client, false);
	ValidateClientState(client, STATE_MOTD, false);

	CPEDATA cpd = client->cpeData;
	EXT tmp = Memory_Alloc(1, sizeof(struct cpeExt));
	if(!ReadNetString(&data, &tmp->name)) return false;
	tmp->version = ntohl(*(uint32_t*)data);
	tmp->crc32 = String_CRC32((uint8_t*)tmp->name);

	if(tmp->crc32 == EXT_HACKCTRL && !cpd->hacks)
		cpd->hacks = Memory_Alloc(1, sizeof(struct cpeHacks));
	if(tmp->crc32 == EXT_LONGMSG && !cpd->message)
		cpd->message = Memory_Alloc(1, 193);

	tmp->next = cpd->firstExtension;
	cpd->firstExtension = tmp;

	--cpd->_extCount;
	if(cpd->_extCount == 0) {
		Event_Call(EVT_ONHANDSHAKEDONE, (void*)client);
		Client_HandshakeStage2(client);
	}

	return true;
}

bool CPEHandler_TwoWayPing(CLIENT client, const char* data) {
	ValidateCpeClient(client, false);
	CPEDATA cpd = client->cpeData;
	uint8_t pingDirection = *data;
	uint16_t pingData = *(uint16_t*)++data;

	if(pingDirection == 0) {
		CPEPacket_WriteTwoWayPing(client, 0, pingData);
		CPEPacket_WriteTwoWayPing(client, 1, ++cpd->pingData);
		cpd->pingStarted = true;
		cpd->pingStart = Time_GetMSec();
		return true;
	} else if(pingDirection == 1) {
		if(cpd->pingStarted) {
			cpd->pingStarted = false;
			if(cpd->pingData == pingData)
				cpd->pingTime = (uint32_t)((Time_GetMSec() - cpd->pingStart) / 2);
			return true;
		}
	}
	return false;
}

bool CPEHandler_PlayerClick(CLIENT client, const char* data) {
	ValidateCpeClient(client, false);
	ValidateClientState(client, STATE_INGAME, false);

	char button = *data;
	char action = *++data;
	short yaw = ntohs(*(uint16_t*)++data); ++data;
	short pitch = ntohs(*(uint16_t*)++data); ++data;
	ClientID tgID = *++data;
	short tgBlockX = ntohs(*(short*)++data); ++data;
	short tgBlockY = ntohs(*(short*)++data); ++data;
	short tgBlockZ = ntohs(*(short*)++data); ++data;
	char tgBlockFace = *++data;

	Event_OnClick(
		client, button,
		action, yaw,
		pitch, tgID,
		tgBlockX,
		tgBlockY,
		tgBlockZ,
		tgBlockFace
	);
	return true;
}
