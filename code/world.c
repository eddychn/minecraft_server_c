#include "core.h"
#include "platform.h"
#include "error.h"
#include "str.h"
#include "server.h"
#include "client.h"
#include "world.h"
#include "event.h"
#include <zlib.h>

void Worlds_SaveAll(cs_bool join, cs_bool unload) {
	for(cs_int32 i = 0; i < MAX_WORLDS; i++) {
		World* world = Worlds_List[i];

		if(i < MAX_WORLDS && world) {
			if(World_Save(world, unload) && join) {
				Waitable_Wait(world->wait);
				if(!Server_Active)
					World_Free(world);
			}
		}
	}
}

World* World_Create(const char* name) {
	World* tmp = Memory_Alloc(1, sizeof(World));
	WorldInfo* wi = Memory_Alloc(1, sizeof(WorldInfo));

	tmp->name = String_AllocCopy(name);
	tmp->wait = Waitable_Create();
	tmp->process = WP_NOPROC;
	tmp->id = -1;
	tmp->info = wi;

	/*
	** Устанавливаем дефолтные значения
	** согласно документации по CPE.
	*/
	wi->props[PROP_SIDEBLOCK] = 7;
	wi->props[PROP_EDGEBLOCK] = 8;
	wi->props[PROP_FOGDIST] = 0;
	wi->props[PROP_SPDCLOUDS] = 256;
	wi->props[PROP_SPDWEATHER] = 256;
	wi->props[PROP_FADEWEATHER] = 128;
	wi->props[PROP_EXPFOG] = 0;
	wi->props[PROP_SIDEOFFSET] = -2;

	for(int i = 0; i < WORLD_COLORS_COUNT; i++) {
		wi->colors[i].r = -1;
		wi->colors[i].g = -1;
		wi->colors[i].b = -1;
	}

	return tmp;
}

cs_bool World_Add(World* world) {
	if(world->id == -1) {
		for(WorldID i = 0; i < MAX_WORLDS; i++) {
			if(!Worlds_List[i]) {
				world->id = i;
				Worlds_List[i] = world;
				return true;
			}
		}
		return false;
	}

	if(world->id > MAX_WORLDS) return false;
	Worlds_List[world->id] = world;
	return true;
}

World* World_GetByName(const char* name) {
	for(WorldID i = 0; i < MAX_WORLDS; i++) {
		World* world = Worlds_List[i];
		if(world && String_CaselessCompare(world->name, name))
			return world;
	}
	return NULL;
}

World* World_GetByID(WorldID id) {
	return id < MAX_WORLDS ? Worlds_List[id] : NULL;
}

void World_SetDimensions(World* world, const SVec* dims) {
	world->info->dimensions = *dims;
	world->size = dims->x * dims->y * dims->z;
}

cs_bool World_SetProperty(World* world, cs_uint8 property, cs_int32 value) {
	if(property > WORLD_PROPS_COUNT) return false;

	world->modified = true;
	world->info->props[property] = value;
	world->info->modval |= MV_PROPS;
	world->info->modprop |= 2 ^ property;
	return true;
}

cs_int32 World_GetProperty(World* world, cs_uint8 property) {
	if(property > WORLD_PROPS_COUNT) return 0;
	return world->info->props[property];
}

cs_bool World_SetTexturePack(World* world, const char* url) {
	if(String_CaselessCompare(world->info->texturepack, url)) {
		return true;
	}
	world->modified = true;
	world->info->modval |= MV_TEXPACK;
	if(!url || String_Length(url) > 64) {
		world->info->texturepack[0] = '\0';
		return true;
	}
	if(!String_Copy(world->info->texturepack, 65, url)) {
		world->info->texturepack[0] = '\0';
		return false;
	}
	return true;
}

const char* World_GetTexturePack(World* world) {
	return world->info->texturepack;
}

cs_bool World_SetWeather(World* world, Weather type) {
	if(type > 2) return false;

	world->info->wt = type;
	world->modified = true;
	world->info->modval |= MV_WEATHER;
	Event_Call(EVT_ONWEATHER, world);
	return true;
}

cs_bool World_SetEnvColor(World* world, cs_uint8 type, Color3* color) {
	if(type > WORLD_COLORS_COUNT) return false;
	world->info->modval |= MV_COLORS;
	world->modified = true;
	world->info->colors[type * 3] = *color;
	Event_Call(EVT_ONCOLOR, world);
	return true;
}

Color3* World_GetEnvColor(World* world, cs_uint8 type) {
	if(type > WORLD_COLORS_COUNT) return false;
	return &world->info->colors[type];
}

void World_UpdateClients(World* world) {
	Clients_UpdateWorldInfo(world);
	world->info->modval = MV_NONE;
}

Weather World_GetWeather(World* world) {
	return world->info->wt;
}

void World_AllocBlockArray(World* world) {
	BlockID* data = Memory_Alloc(world->size, sizeof(BlockID));
	*(cs_uint32*)data = htonl(world->size);
	world->data = data;
	world->loaded = true;
}

void World_Free(World* world) {
	Waitable_Free(world->wait);
	if(world->data) Memory_Free(world->data);
	if(world->info) Memory_Free(world->info);
	if(world->id != -1) Worlds_List[world->id] = NULL;
	Memory_Free(world);
}

static cs_bool WriteWData(FILE* fp, cs_uint8 dataType, void* ptr, cs_int32 size) {
	return File_Write(&dataType, 1, 1, fp) && (size > 0 ? File_Write(ptr, size, 1, fp) : true);
}

static cs_bool WriteInfo(World* world, FILE* fp) {
	cs_int32 magic = WORLD_MAGIC;
	if(!File_Write((char*)&magic, 4, 1, fp)) {
		Error_PrintSys(false);
		return false;
	}
	return WriteWData(fp, DT_DIM, &world->info->dimensions, sizeof(SVec)) &&
	WriteWData(fp, DT_SV, &world->info->spawnVec, sizeof(Vec)) &&
	WriteWData(fp, DT_SA, &world->info->spawnAng, sizeof(Ang)) &&
	WriteWData(fp, DT_WT, &world->info->wt, sizeof(Weather)) &&
	WriteWData(fp, DT_PROPS, world->info->props, 4 * WORLD_PROPS_COUNT) &&
	WriteWData(fp, DT_COLORS, world->info->colors, sizeof(Color3) * WORLD_COLORS_COUNT) &&
	WriteWData(fp, DT_END, NULL, 0);
}

static cs_bool ReadInfo(World* world, FILE* fp) {
	cs_uint8 id = 0;
	cs_uint32 magic = 0;
	if(!File_Read(&magic, 4, 1, fp))
		return false;

	if(WORLD_MAGIC != magic) {
		Error_Print2(ET_SERVER, EC_MAGIC, true);
		return false;
	}

	SVec dims = {0};

	while(File_Read(&id, 1, 1, fp) == 1) {
		switch (id) {
			case DT_DIM:
				if(File_Read(&dims, sizeof(SVec), 1, fp) != 1)
					return false;
				World_SetDimensions(world, &dims);
				break;
			case DT_SV:
				if(File_Read(&world->info->spawnVec, sizeof(Vec), 1, fp) != 1)
					return false;
				break;
			case DT_SA:
				if(File_Read(&world->info->spawnAng, sizeof(Ang), 1, fp) != 1)
					return false;
				break;
			case DT_WT:
				if(File_Read(&world->info->wt, sizeof(Weather), 1, fp) != 1)
					return false;
				break;
			case DT_PROPS:
				if(File_Read(world->info->props, 4 * WORLD_PROPS_COUNT, 1, fp) != 1)
					return false;
				break;
			case DT_COLORS:
				if(File_Read(world->info->colors, sizeof(Color3) * WORLD_COLORS_COUNT, 1, fp) != 1)
					return false;
				break;
			case DT_END:
				return true;
			default:
				Error_PrintF2(ET_SERVER, EC_FILECORR, false, world->name);
				return false;
		}
	}

	return false;
}

THREAD_FUNC(WorldSaveThread) {
	World* world = param;

	cs_bool succ = false;
	char path[256];
	char tmpname[256];
	String_FormatBuf(path, 256, "worlds" PATH_DELIM "%s", world->name);
	String_FormatBuf(tmpname, 256, "worlds" PATH_DELIM "%s.tmp", world->name);

	FILE* fp = File_Open(tmpname, "w");
	if(!fp) {
		Error_PrintSys(false);
		goto world_save_done;
	}

	if(!WriteInfo(world, fp)) {
		goto world_save_done;
	}

	Bytef out[1024];
	cs_int32 ret;
	z_stream stream;
	stream.zalloc = Z_NULL;
	stream.zfree = Z_NULL;
	stream.opaque = Z_NULL;

	if((ret = deflateInit(&stream, Z_BEST_COMPRESSION)) != Z_OK) {
		Error_Print2(ET_ZLIB, ret, false);
		goto world_save_done;
	}

	stream.avail_in = (uLongf)world->size + 4;
	stream.next_in = (Bytef*)world->data;

	do {
		stream.next_out = out;
		stream.avail_out = 1024;

		if((ret = deflate(&stream, Z_FINISH)) == Z_STREAM_ERROR) {
			Error_Print2(ET_ZLIB, ret, false);
			goto world_save_done;
		}

		if(!File_Write(out, 1, 1024 - stream.avail_out, fp)) {
			Error_PrintSys(false);
			goto world_save_done;
		}
	} while(stream.avail_out == 0);

	succ = true;

	world_save_done:
	File_Close(fp);
	deflateEnd(&stream);
	world->process = WP_NOPROC;
	if(succ)
		File_Rename(tmpname, path);
	if(world->saveUnload)
		World_Unload(world);
	Waitable_Signal(world->wait);
	return 0;
}

cs_bool World_Save(World* world, cs_bool unload) {
	if(world->process != WP_NOPROC || !world->modified || !world->loaded)
		return world->process == WP_SAVING;

	world->process = WP_SAVING;
	world->saveUnload = unload;
	Waitable_Reset(world->wait);
	Thread_Create(WorldSaveThread, world, true);
	return true;
}

THREAD_FUNC(WorldLoadThread) {
	World* world = param;

	cs_bool error = true;
	char path[256];
	String_FormatBuf(path, 256, "worlds" PATH_DELIM "%s", world->name);

	FILE* fp = File_Open(path, "r");
	if(!fp) {
		Error_PrintSys(false);
		goto world_load_done;
	}

	if(!ReadInfo(world, fp))
		goto world_load_done;

	World_AllocBlockArray(world);

	cs_int32 ret;
	Bytef in[1024];
	z_stream stream;
	stream.zalloc = Z_NULL;
	stream.zfree = Z_NULL;
	stream.opaque = Z_NULL;

	if((ret = inflateInit(&stream)) != Z_OK) {
		Error_Print2(ET_ZLIB, ret, false);
		goto world_load_done;
	}

	stream.next_out = (Bytef*)world->data;

	do {
		stream.avail_in = (uLongf)File_Read(in, 1, 1024, fp);
		if(File_Error(fp)) {
			Error_PrintSys(false);
			goto world_load_done;
		}

		if(stream.avail_in == 0) break;
		stream.next_in = in;

		do {
			stream.avail_out = 1024;
			if((ret = inflate(&stream, Z_FINISH)) == Z_NEED_DICT || ret == Z_DATA_ERROR || ret == Z_MEM_ERROR) {
				Error_Print2(ET_ZLIB, ret, false);
				goto world_load_done;
			}
		} while(stream.avail_out == 0);
	} while(ret != Z_STREAM_END);
	error = false;

	world_load_done:
	File_Close(fp);
	inflateEnd(&stream);
	if(error)
		World_Unload(world);
	world->process = WP_NOPROC;
	world->saveUnload = false;
	Waitable_Signal(world->wait);
	return 0;
}

cs_bool World_Load(World* world) {
	if(world->loaded) return false;
	if(world->process != WP_NOPROC)
		return world->process == WP_LOADING;

	world->process = WP_LOADING;
	Waitable_Reset(world->wait);
	Thread_Create(WorldLoadThread, world, true);
	return true;
}

void World_Unload(World* world) {
	if(world->process != WP_NOPROC)
		Waitable_Wait(world->wait);
	if(world->data) Memory_Free(world->data);
	world->loaded = false;
	world->data = NULL;
}

cs_uint32 World_GetOffset(World* world, SVec* pos) {
	WorldInfo* wi = world->info;
	SVec* dim = &wi->dimensions;

	if(dim->x < 0 || dim->y < 0 || dim->z < 0 ||
	pos->x > dim->x || pos->y > dim->y || pos->z > dim->z)
		return 0;
	return pos->z * dim->z + pos->y * (dim->x * dim->y) + pos->x + 4;
}

cs_bool World_SetBlockO(World* world, cs_uint32 offset, BlockID id) {
	if(offset > 0) {
		world->data[offset] = id;
		world->modified = true;
	} else return false;

	return true;
}

cs_bool World_SetBlock(World* world, SVec* pos, BlockID id) {
	cs_uint32 offset = World_GetOffset(world, pos);
	return World_SetBlockO(world, offset, id);
}

BlockID World_GetBlock(World* world, SVec* pos) {
	cs_uint32 offset = World_GetOffset(world, pos);

	if(offset == 0) return 0;
	return world->data[offset];
}
