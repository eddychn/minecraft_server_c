#include "core.h"
#include "platform.h"
#include "str.h"
#include "config.h"
#include "error.h"

#define CFG_SYSERROR \
store->etype = ET_SYS; \
store->ecode = Error_GetSysCode(); \
store->eline = 0;

#define CFG_TYPE(expectedType) \
if(ent->type != expectedType) { \
	Error_PrintF2(ET_SERVER, EC_CFGINVGET, true, ent->key, ent->store->path, Config_TypeName(expectedType), Config_TypeName(ent->type)); \
}

CFGStore Config_NewStore(const char* path) {
	CFGStore store = Memory_Alloc(1, sizeof(struct cfgStore));
	store->path = String_AllocCopy(path);
	return store;
}

CFGEntry Config_GetEntry(CFGStore store, const char* key) {
	CFGEntry ent = store->firstCfgEntry;

	while(ent) {
		if(String_CaselessCompare(ent->key, key))
			return ent;
		ent = ent->next;
	}

	return NULL;
}

CFGEntry Config_CheckEntry(CFGStore store, const char* key) {
	CFGEntry ent = Config_GetEntry(store, key);
	if(!ent) {
		Error_PrintF2(ET_SERVER, EC_CFGUNK, true, key, store->path);
	}
	return ent;
}

CFGEntry Config_NewEntry(CFGStore store, const char* key, cs_int32 type) {
	CFGEntry ent = Config_GetEntry(store, key);
	if(ent) return ent;

	ent = Memory_Alloc(1, sizeof(struct cfgEntry));
	ent->key = String_AllocCopy(key);
	ent->store = store;
	ent->type = type;

	if(store->firstCfgEntry)
		store->lastCfgEntry->next = ent;
	else
		store->firstCfgEntry = ent;

	store->lastCfgEntry = ent;
	return ent;
}

static void EmptyEntry(CFGEntry ent) {
	if(ent->type == CFG_STR && ent->value.vchar)
		Memory_Free((void*)ent->value.vchar);

	ent->changed = false;
	ent->value.vchar = NULL;
}

static bool AllCfgEntriesParsed(CFGStore store) {
	CFGEntry ent = store->firstCfgEntry;
	bool loaded = true;

	while(ent && loaded) {
		loaded = ent->readed;
		ent = ent->next;
	}

	return loaded;
}

const char* Config_TypeName(cs_int32 type) {
	switch (type) {
		case CFG_STR:
			return "string";
		case CFG_INT:
			return "integer";
		case CFG_INT16:
			return "short";
		case CFG_INT8:
			return "byte";
		case CFG_BOOL:
			return "boolean";
		default:
			return "unknownType";
	}
}

cs_int32 Config_TypeNameToInt(const char* name) {
	if(String_CaselessCompare(name, "string")) {
		return CFG_STR;
	} else if(String_CaselessCompare(name, "integer")) {
		return CFG_INT;
	} else if(String_CaselessCompare(name, "short")) {
		return CFG_INT16;
	} else if(String_CaselessCompare(name, "byte")) {
		return CFG_INT8;
	} else if(String_CaselessCompare(name, "boolean")) {
		return CFG_BOOL;
	}
	return CFG_INVTYPE;
}

bool Config_ToStr(CFGEntry ent, char* value, cs_uint8 len) {
	switch (ent->type) {
		case CFG_INT:
		case CFG_INT16:
		case CFG_INT8:
			if(ent->changed)
				String_FormatBuf(value, len, "%d", ent->value.vint);
			else
				String_FormatBuf(value, len, "%d", ent->defvalue.vint);
			break;
		case CFG_BOOL:
			if(ent->changed)
				String_Copy(value, len, ent->value.vbool ? "True" : "False");
			else
				String_Copy(value, len, ent->defvalue.vbool ? "True" : "False");
			break;
		case CFG_STR:
			if(ent->changed)
				String_Copy(value, len, ent->value.vchar);
			else
				String_Copy(value, len, ent->defvalue.vchar);
			break;
		default:
			return false;
	}
	return true;
}

void Config_PrintError(CFGStore store) {
	switch (store->etype) {
		case ET_SERVER:
			if(store->eline > 0) {
				Error_PrintF2(store->etype, store->ecode, false, store->eline, store->path);
			} else {
				Error_PrintF2(store->etype, store->ecode, false, store->path);
			}
			break;
		case ET_SYS:
			Error_PrintSys(false);
			break;
	}
}

bool Config_Load(CFGStore store) {
	FILE* fp = File_Open(store->path, "r");
	if(!fp) {
		if(errno == ENOENT) return true;
		CFG_SYSERROR;
		return false;
	}

	bool haveComment = false;
	char line[MAX_CFG_LEN * 2 + 2];
	char comment[MAX_CFG_LEN];
	cs_int32 lnret = 0, linenum = 0;

	while((lnret = File_ReadLine(fp, line, 256)) > 0 && ++linenum) {
		if(!haveComment && *line == '#') {
			haveComment = true;
			String_Copy(comment, MAX_CFG_LEN, line);
			continue;
		}
		char* value = (char*)String_FirstChar(line, '=');
		if(!value) {
			store->etype = ET_SERVER;
			store->ecode = EC_CFGLINEPARSE;
			store->eline = linenum;
			return false;
		}
		*value++ = '\0';
		CFGEntry ent = Config_CheckEntry(store, line);
		ent->readed = true;

		if(haveComment) {
			Config_SetComment(ent, comment);
			haveComment = false;
		}

		switch (ent->type) {
			case CFG_STR:
				Config_SetStr(ent, value);
				break;
			case CFG_INT:
				if(*value < '0' || *value > '9') break;
				Config_SetInt(ent, String_ToInt(value));
				break;
			case CFG_INT8:
				if(*value < '0' || *value > '9') break;
				Config_SetInt8(ent, (cs_int8)String_ToInt(value));
				break;
			case CFG_INT16:
				if(*value < '0' || *value > '9') break;
				Config_SetInt16(ent, (cs_int16)String_ToInt(value));
				break;
			case CFG_BOOL:
				Config_SetBool(ent, String_Compare(value, "True"));
				break;
		}
	}

	if(lnret == -1) {
		store->etype = ET_SERVER;
		store->ecode = EC_CFGEND;
		store->eline = 0;
		return false;
	}

	store->modified = !AllCfgEntriesParsed(store);
	File_Close(fp);

	store->etype = ET_NOERR;
	store->ecode = 0;
	store->eline = 0;
	return true;
}

bool Config_Save(CFGStore store) {
	if(!store->modified) return true;

	char tmpname[256];
	String_FormatBuf(tmpname, 256, "%s.tmp", store->path);

	FILE* fp = File_Open(tmpname, "w");
	if(!fp) {
		CFG_SYSERROR;
		return false;
	}

	CFGEntry ptr = store->firstCfgEntry;

	while(ptr) {
		if(ptr->commentary)
			if(!File_WriteFormat(fp, "#%s\n", ptr->commentary)) {
				CFG_SYSERROR;
				return false;
			}
		if(!File_Write(ptr->key, String_Length(ptr->key), 1, fp)) {
			CFG_SYSERROR;
			return false;
		}

		char* vchar;
		cs_int32 vint;
		bool vbool;

		switch (ptr->type) {
			case CFG_STR:
				vchar = (char*)(ptr->changed ? ptr->value.vchar : ptr->defvalue.vchar);
				if(!File_WriteFormat(fp, "=%s\n", vchar)) {
					CFG_SYSERROR;
					return false;
				}
				break;
			case CFG_INT:
			case CFG_INT16:
			case CFG_INT8:
				vint = ptr->changed ? ptr->value.vint : ptr->defvalue.vint;
				if(!File_WriteFormat(fp, "=%d\n", vint)) {
					CFG_SYSERROR;
					return false;
				}
				break;
			case CFG_BOOL:
				vbool = ptr->changed ? ptr->value.vbool : ptr->defvalue.vbool;
				if(!File_WriteFormat(fp, "=%s\n", vbool ? "True" : "False")) {
					CFG_SYSERROR;
					return false;
				}
				break;
			default:
				if(!File_Write("=Unknown value\n", 16, 1, fp)) {
					CFG_SYSERROR;
					return false;
				}
				break;
		}
		ptr = ptr->next;
	}

	File_Close(fp);
	store->modified = false;
	if(!File_Rename(tmpname, store->path)) {
		CFG_SYSERROR;
		return false;
	}

	store->etype = ET_NOERR;
	store->ecode = 0;
	store->eline = 0;
	return true;
}

void Config_SetComment(CFGEntry ent, const char* commentary) {
	if(ent->commentary)
		Memory_Free((void*)ent->commentary);
	ent->commentary = String_AllocCopy(commentary);
}

void Config_SetLimit(CFGEntry ent, cs_int32 min, cs_int32 max) {
	ent->haveLimits = true;
	ent->limits[1] = min;
	ent->limits[0] = max;
}

void Config_SetDefaultInt(CFGEntry ent, cs_int32 value) {
	CFG_TYPE(CFG_INT);
	ent->defvalue.vint = value;
}

void Config_SetDefaultInt8(CFGEntry ent, cs_int8 value) {
	CFG_TYPE(CFG_INT8);
	ent->defvalue.vint8 = value;
}

void Config_SetDefaultInt16(CFGEntry ent, cs_int16 value) {
	CFG_TYPE(CFG_INT16);
	ent->defvalue.vint16 = value;
}

void Config_SetInt(CFGEntry ent, cs_int32 value) {
	CFG_TYPE(CFG_INT);
	if(ent->defvalue.vint != value) {
		ent->changed = true;
		if(ent->haveLimits)
			value = min(max(value, ent->limits[1]), ent->limits[0]);
		ent->value.vint = value;
	}
}

void Config_SetInt16(CFGEntry ent, cs_int16 value) {
	CFG_TYPE(CFG_INT16);
	if(ent->defvalue.vint16 != value) {
		ent->changed = true;
		if(ent->haveLimits)
			value = (cs_int16)min(max(value, ent->limits[1]), ent->limits[0]);
		ent->value.vint16 = value;
		ent->store->modified = true;
	}
}

void Config_SetInt8(CFGEntry ent, cs_int8 value) {
	CFG_TYPE(CFG_INT8);
	if(ent->defvalue.vint8 != value) {
		ent->changed = true;
		if(ent->haveLimits)
			value = (cs_int8)min(max(value, ent->limits[1]), ent->limits[0]);
		ent->value.vint8 = value;
		ent->store->modified = true;
	}
}


cs_int32 Config_GetInt(CFGStore store, const char* key) {
	CFGEntry ent = Config_CheckEntry(store, key);
	CFG_TYPE(CFG_INT);
	return ent->changed ? ent->value.vint : ent->defvalue.vint;
}

cs_uint32 Config_GetUInt(CFGStore store, const char* key) {
	return (cs_uint32)Config_GetInt(store, key);
}

cs_int8 Config_GetInt8(CFGStore store, const char* key) {
	CFGEntry ent = Config_CheckEntry(store, key);
	CFG_TYPE(CFG_INT8);
	return ent->changed ? ent->value.vint8 : ent->defvalue.vint8;
}

cs_uint8 Config_GetUInt8(CFGStore store, const char* key) {
	return (cs_uint8)Config_GetInt8(store, key);
}

cs_int16 Config_GetInt16(CFGStore store, const char* key) {
	CFGEntry ent = Config_CheckEntry(store, key);
	CFG_TYPE(CFG_INT16);
	return ent->changed ? ent->value.vint16 : ent->defvalue.vint16;
}

cs_uint16 Config_GetUInt16(CFGStore store, const char* key) {
	return (cs_uint16)Config_GetInt16(store, key);
}

void Config_SetDefaultStr(CFGEntry ent, const char* value) {
	CFG_TYPE(CFG_STR);
	if(ent->defvalue.vchar)
		Memory_Free((void*)ent->defvalue.vchar);
	ent->defvalue.vchar = String_AllocCopy(value);
}

void Config_SetStr(CFGEntry ent, const char* value) {
	CFG_TYPE(CFG_STR);
	if(!String_Compare(value, ent->defvalue.vchar)) {
		EmptyEntry(ent);
		ent->changed = true;
		ent->value.vchar = String_AllocCopy(value);
		ent->store->modified = true;
	}
}

const char* Config_GetStr(CFGStore store, const char* key) {
	CFGEntry ent = Config_CheckEntry(store, key);
	CFG_TYPE(CFG_STR);
	return ent->changed ? ent->value.vchar : ent->defvalue.vchar;
}

void Config_SetDefaultBool(CFGEntry ent, bool value) {
	CFG_TYPE(CFG_BOOL);
	ent->defvalue.vbool = value;
}

void Config_SetBool(CFGEntry ent, bool value) {
	CFG_TYPE(CFG_BOOL);
	if(ent->defvalue.vbool != value) {
		ent->changed = true;
		ent->value.vbool = value;
		ent->store->modified = true;
	}
}

bool Config_GetBool(CFGStore store, const char* key) {
	CFGEntry ent = Config_CheckEntry(store, key);
	CFG_TYPE(CFG_BOOL);
	return ent->changed ? ent->value.vbool : ent->defvalue.vbool;
}

void Config_EmptyStore(CFGStore store) {
	CFGEntry prev, ent = store->firstCfgEntry;

	while(ent) {
		prev = ent;
		ent = ent->next;
		if(prev->commentary)
			Memory_Free((void*)prev->commentary);
		if(prev->type == CFG_STR)
			Memory_Free((void*)prev->defvalue.vchar);
		EmptyEntry(prev);
		Memory_Free(prev);
	}

	store->modified = true;
	store->firstCfgEntry = NULL;
	store->lastCfgEntry = NULL;
}

void Config_DestroyStore(CFGStore store) {
	Memory_Free((void*)store->path);
	Config_EmptyStore(store);
	Memory_Free(store);
}
