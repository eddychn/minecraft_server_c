#ifndef WORLD_H
#define WORLD_H
#define WORLD_MAGIC 1413563479
#define MAX_WORLDS  128

enum WorldDataType {
	DT_DIM,
	DT_SV,
	DT_SA,

	DT_END = 0xFF
};

typedef struct worldDims {
	ushort width;
	ushort height;
	ushort length;
} WORLDDIMS;

typedef struct worldInfo {
	WORLDDIMS*  dim;
	VECTOR*     spawnVec;
	ANGLE*      spawnAng;
} WORLDINFO;

typedef struct world {
	const char* name;
	uint        size;
	BlockID*    data;
	WORLDINFO*  info;
} WORLD;

WORLD* World_Create(const char* name);
uint World_GetOffset(WORLD* world, ushort x, ushort y, ushort z);
void World_SetDimensions(WORLD* world, ushort width, ushort height, ushort length);
int World_SetBlock(WORLD* world, ushort x, ushort y, ushort z, BlockID id);
BlockID World_GetBlock(WORLD* world, ushort x, ushort y, ushort z);
void World_GenerateFlat(WORLD* world);
WORLD* World_FindByName(const char* name);
void World_Destroy(WORLD* world);
bool World_Load(WORLD* world);
bool World_Save(WORLD* world);
void World_AllocBlockArray(WORLD* world);

WORLD* worlds[MAX_WORLDS];
#endif
