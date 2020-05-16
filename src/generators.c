#include "core.h"
#include "block.h"
#include "world.h"
#include "generators.h"
#include "random.h"

// Генератор плоского мира

void Generator_Flat(World *world) {
	WorldInfo *wi = &world->info;
	SVec *dims = &wi->dimensions;

	BlockID *data = World_GetBlockArray(world, NULL);
	cs_int32 dirtEnd = dims->x * dims->z * (dims->y / 2 - 1);
	for(cs_int32 i = 0; i < dirtEnd + dims->x * dims->z; i++) {
		if(i < dirtEnd)
			data[i] = 3;
		else
			data[i] = 2;
	}

	World_SetProperty(world, PROP_CLOUDSLEVEL, dims->y + 2);
	World_SetProperty(world, PROP_EDGELEVEL, dims->y / 2);

	wi->spawnVec.x = (cs_float)dims->x / 2;
	wi->spawnVec.y = (cs_float)(dims->y / 2) + 1.59375f;
	wi->spawnVec.z = (cs_float)dims->z / 2;
}

/*
** Генератор обычного мира.
** Когда-нибудь он точно будет
** готов, но явно не сегодня.
*/

static cs_bool gen_enable_caves = true,
gen_enable_trees = true,
gen_enable_ores = true,
gen_enable_houses = true;

static cs_uint16 gen_cave_radius = 3,
gen_cave_min_length = 100,
gen_cave_max_length = 500,
gen_ore_vein_size = 3,
gen_gravel_vein_size = 14,
gen_biome_step = 20,
gen_biome_radius = 5,
gen_biome_radius2 = 25; // Менять в зависимости от предыдущего значения

static cs_float gen_trees_count_mult = 0.007f,
gen_ores_count_mult = 1.0f / 2000.0f,
gen_caves_count_mult = 2.0f / 700000.0f,
gen_houses_count_mult = 1.0f / 70000.0f,
gen_gravel_count_mult = 1.0f / 500000;

#define MAX_THREADS 16
cs_int32 cfgMaxThreads = 2;

static struct DefGenContext {
	RNGState rnd;
	World *world;
	BlockID *data;
	SVec *dims;
	Thread threads[MAX_THREADS];
	cs_uint32 wsize, lvlSize;
	cs_uint16 *biomes, *heightMap,
	biomeSizeX, biomeSizeZ, biomesNum,
	heightGrass, heightWater, heightStone,
	heightLava, gravelVeinSize, biomeSize;
} ctx;

enum gen_biomes {
	BIOME_NORMAL,
	BIOME_HIGH,
	BIOME_TREES,
	BIOME_SAND,
	BIOME_WATER,
	BIOMES_COUNT
};

static cs_int32 addThread(TFUNC func, TARG arg) {
	for(cs_int32 i = 0; i < MAX_THREADS; i++) {
		if(i > cfgMaxThreads) {
			i = 0;
			if(Thread_IsValid(ctx.threads[i])) {
				Thread_Join(ctx.threads[i]);
				ctx.threads[i] = NULL;
			}
		}
		if(!Thread_IsValid(ctx.threads[i])) {
			ctx.threads[i] = Thread_Create(func, arg, false);
			return i;
		}
	}

	return -1;
}

static void waitAll(void) {
	for(cs_int32 i = 0; i < MAX_THREADS; i++) {
		if(Thread_IsValid(ctx.threads[i]))
			Thread_Join(ctx.threads[i]);
	}
}

static void doCleanUp(void) {
	Memory_Free(ctx.biomes);
	Memory_Free(ctx.heightMap);
	Memory_Fill(&ctx, sizeof(ctx), 0);
}

static void genBiomes(void) {
	ctx.biomeSizeX = ((cs_uint16)ctx.dims->x / gen_biome_step) + 1,
	ctx.biomeSizeZ = ((cs_uint16)ctx.dims->z / gen_biome_step) + 2,
	ctx.biomeSize = ctx.biomeSizeX * ctx.biomeSizeZ,
	ctx.biomesNum = ctx.dims->x * ctx.dims->z / gen_biome_step / gen_biome_radius / 64 + 1;

	ctx.biomes = Memory_Alloc(2, ctx.biomeSize);
	for(cs_int16 i = 0; i < ctx.biomesNum; i++) {
		cs_uint16 x = (cs_uint16)Random_Range(&ctx.rnd, 0, ctx.biomeSizeX),
		z = (cs_uint16)Random_Range(&ctx.rnd, 0, ctx.biomeSizeZ),
		biome = (cs_uint16)Random_Range(&ctx.rnd, 0, BIOMES_COUNT);

		for(cs_int16 dx = -gen_biome_radius; dx < gen_biome_radius; dx++) {
			for(cs_int16 dz = -gen_biome_radius; dz < gen_biome_radius; dz++) {
				cs_int16 nx = x + dx,
				nz = z + dz;

				if(dx * dx + dz * dz < gen_biome_radius2 &&
				0 <= nx && nx < ctx.biomeSizeX &&
				0 <= nz && nz < ctx.biomeSizeZ) {
					cs_uint16 offset = nx + nz * ctx.biomeSizeX;
					if(offset >= 0 && offset <= ctx.biomeSize)
						ctx.biomes[offset] = biome;
				}
			}
		}
	}
}

static void genHeightMap(void) {
	ctx.heightMap = Memory_Alloc(2, ctx.biomeSizeX * ctx.biomeSizeZ);

	for(cs_uint16 x = 0; x < ctx.biomeSizeX; x++) {
		for(cs_uint16 z = 0; z < ctx.biomeSizeZ; z++) {
			cs_uint16 offset = x + z * ctx.biomeSizeX,
			biome = ctx.biomes[offset];

			switch (biome) {
				case BIOME_NORMAL:
					if(Random_Range(&ctx.rnd, 0, 6) == 0)
						ctx.heightMap[offset] = ctx.heightGrass + (cs_uint16)Random_Range(&ctx.rnd, -3, -1);
					else
						ctx.heightMap[offset] = ctx.heightGrass + (cs_uint16)Random_Range(&ctx.rnd, 1, 3);
					break;
				case BIOME_HIGH:
					if(Random_Range(&ctx.rnd, 0, 30) == 0)
						ctx.heightMap[offset] = ctx.heightGrass +
						(cs_uint16)Random_Range(&ctx.rnd, 20, min(ctx.dims->y - ctx.heightGrass - 1, 40));
					else
						ctx.heightMap[offset] = ctx.heightGrass + (cs_int16)Random_Range(&ctx.rnd, -2, 20);
					break;
				case BIOME_TREES:
					ctx.heightMap[offset] = ctx.heightGrass + (cs_uint16)Random_Range(&ctx.rnd, 1, 5);
					break;
				case BIOME_SAND:
					ctx.heightMap[offset] = ctx.heightGrass + (cs_uint16)Random_Range(&ctx.rnd, 1, 4);
					break;
				case BIOME_WATER:
					if(Random_Range(&ctx.rnd, 0, 10) == 0)
						ctx.heightMap[offset] = ctx.heightGrass + (cs_int16)Random_Range(&ctx.rnd, -20, -3);
					else
						ctx.heightMap[offset] = ctx.heightGrass + (cs_int16)Random_Range(&ctx.rnd, -10, -3);
					break;
				default:
					ctx.heightMap[offset] = ctx.heightGrass;
			}

			ctx.heightMap[offset] = max(min(ctx.heightMap[offset], ctx.dims->y - 2), 3);
		}
	}
}

#define setBlock(x, y, z, id) \
ctx.data[(y * ctx.dims->z + z) * ctx.dims->x + x] = id

#define getBlock(x, y, z) \
ctx.data[(y * ctx.dims->z + z) * ctx.dims->x + x]

THREAD_FUNC(terrainThread) {
	(void)param;

	cs_uint16 height1, heightStone1;
	enum gen_biomes biome;
	for(cs_uint16 x = 0; x < ctx.dims->x; x++) {
		cs_uint16 hx = x / gen_biome_step,
		biomePosZOld = (cs_uint16)-1,
		b0 = hx,
		b1 = b0 + 1,
		b00 = 0,
		b01 = ctx.biomes[b0],
		b10 = 0,
		b11 = ctx.biomes[b1];
		cs_float percentPosX = (cs_float)x / (float)gen_biome_step - (float)hx,
		percentNegX = 1.0f - percentPosX;
		for(cs_uint16 z = 0; z < ctx.dims->z; z++) {
			cs_uint16 hz = z / gen_biome_step;
			cs_float percentZ = (cs_float)z / (cs_float)gen_biome_step - (float)hz,
			percentZOp = 1.0f - percentZ;
			height1 = (cs_uint16)(((cs_float)ctx.heightMap[hx + hz * ctx.biomeSizeX] * percentNegX +
			(cs_float)ctx.heightMap[(hx + 1) + hz * ctx.biomeSizeX] * percentPosX) * (1 - percentZ) +
			((cs_float)ctx.heightMap[hx + (hz + 1) * ctx.biomeSizeX] * percentNegX +
			(cs_float)ctx.heightMap[(hx + 1) + (hz + 1) * ctx.biomeSizeX] * percentPosX) *
			percentZ + 0.5f);
			heightStone1 = max(height1 - (cs_uint16)Random_Range(&ctx.rnd, 4, 6), 1);

			if(hz != biomePosZOld) {
				biomePosZOld = hz;
				b00 = b01;
				b01 = ctx.biomes[b0 + (hz + 1) * ctx.biomeSizeX];
				b10 = b11;
				b11 = ctx.biomes[b1 + (hz + 1) * ctx.biomeSizeX];
				if(b01 == BIOME_TREES) b01 = BIOME_NORMAL;
				if(b11 == BIOME_TREES) b11 = BIOME_NORMAL;
			}

			if(b11 == b01 && b11 == b10) {
				if(percentPosX * percentPosX + percentZ * percentZ > 0.25f)
					biome = b11;
				else
					biome = b00;
			} else if(b00 == b11 && b00 == b10) {
				if(percentPosX * percentPosX + percentZOp * percentZOp > 0.25f)
					biome = b00;
				else
					biome = b01;
			} else if(b00 == b01 && b00 == b11) {
				if(percentNegX * percentNegX + percentZ * percentZ > 0.25f)
					biome = b00;
				else
					biome = b10;
			} else if(b00 == b01 && b00 == b10) {
				if(percentNegX * percentNegX + percentZOp * percentZOp > 0.25f)
					biome = b00;
				else
					biome = b11;
			} else {
				cs_uint16 bx = (cs_uint16)((cs_float)hx + 0.5f),
				bz = (cs_uint16)((cs_float)hz + 0.5f);
				biome = ctx.biomes[bx + bz * ctx.biomeSizeX];
			}

			cs_uint32 offset = z * ctx.dims->x + x;
			cs_uint16 y; // For iterators

			for(y = ctx.heightStone; y < heightStone1; y++)
				ctx.data[offset + y * ctx.lvlSize] = BLOCK_STONE;

			switch (biome) {
				case BIOME_NORMAL:
				case BIOME_TREES:
					for(y = heightStone1; y < height1 - 1; y++)
						ctx.data[offset + y * ctx.lvlSize] = BLOCK_DIRT;

					if(height1 > ctx.heightWater) {
						ctx.data[offset + (height1 - 1) * ctx.lvlSize] = BLOCK_DIRT;
						ctx.data[offset + height1 * ctx.lvlSize] = BLOCK_GRASS;
					} else {
						ctx.data[offset + (height1 - 1) * ctx.lvlSize] = BLOCK_DIRT;
						ctx.data[offset + height1 * ctx.lvlSize] = BLOCK_SAND;
					}

					for(y = height1 + 1; y <= ctx.heightWater; y++)
						ctx.data[offset + y * ctx.lvlSize] = BLOCK_WATER;
					break;
				case BIOME_HIGH:
					for(y = heightStone1; y <= height1; y++)
						ctx.data[offset + y * ctx.lvlSize] = BLOCK_STONE;
					for(y = height1 + 1; y <= ctx.heightWater; y++)
						ctx.data[offset + y * ctx.lvlSize] = BLOCK_WATER;
					break;
				case BIOME_SAND:
					for(y = heightStone1; y <= height1; y++)
						ctx.data[offset + y * ctx.lvlSize] = BLOCK_SAND;
					for(y = height1 + 1; y <= ctx.heightWater; y++)
						ctx.data[offset + y * ctx.lvlSize] = BLOCK_WATER;
					break;
				case BIOME_WATER:
					for(y = heightStone1; y <= min(height1, ctx.heightGrass - 2); y++)
						ctx.data[offset + y * ctx.lvlSize] = BLOCK_DIRT;
					for(y = max(heightStone1, ctx.heightGrass - 2); y <= height1; y++)
						ctx.data[offset + y * ctx.lvlSize] = BLOCK_SAND;
					for(y = height1 + 1; y <= ctx.heightWater; y++)
						ctx.data[offset + y * ctx.lvlSize] = BLOCK_WATER;
					break;
				default:
					ctx.data[offset + (height1 - 1) * ctx.lvlSize] =
					(BlockID)Random_Range(&ctx.rnd, BLOCK_RED, BLOCK_BLACK);
					ctx.data[offset + height1 * ctx.lvlSize] =
					(BlockID)Random_Range(&ctx.rnd, BLOCK_RED, BLOCK_BLACK);
					break;
			}
		}
	}

	return 0;
}

void Generator_Default(World *world) {
	ctx.world = world;
	ctx.dims = &world->info.dimensions;
	ctx.lvlSize = ctx.dims->x * ctx.dims->z;
	ctx.heightGrass = ctx.dims->y / 2;
	ctx.heightWater = ctx.heightGrass;
	ctx.heightStone = ctx.heightGrass - 3;
	ctx.data = World_GetBlockArray(world, &ctx.wsize);
	ctx.gravelVeinSize = min(gen_gravel_vein_size, ctx.heightGrass / 3);
	Random_Seed(&ctx.rnd, 1337);
	genBiomes();
	genHeightMap();
	Memory_Fill(ctx.data, ctx.lvlSize, BLOCK_BEDROCK);
	Memory_Fill(ctx.data + ctx.lvlSize, ctx.lvlSize * (ctx.heightStone - 1), BLOCK_STONE);
	addThread(terrainThread, NULL);
	waitAll();
	doCleanUp();
}
