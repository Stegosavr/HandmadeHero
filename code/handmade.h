// NOTE(casey):
//
// HANDMADE_INTERNAL:
// 0 - Build for public release
// 1 - Build for developer only
//
// HANDMADE_SLOW:
// 0 - Not slow code allowed!
// 1 - Slow code welcome.

#include "handmade_platform.h"

#define internal 		static 
#define global_variable static 
#define local_persist 	static 

#define Pi32 3.14159265359f

#if HANDMADE_SLOW
// TODO(casey): Complete assertion macro - don't worry everyone!
#define Assert(Expression) if (!(Expression)) *(int *)0 = 0;
#else
#define Assert(Expression)
#endif

// TODO(casey): should we do it alwaye in 64bit?
#define Kilobytes(Value) ((Value)*1024ULL)
#define Megabytes(Value) (Kilobytes(Value)*1024ULL)
#define Gigabytes(Value) (Megabytes(Value)*1024ULL)
#define Terabytes(Value) (Gigabytes(Value)*1024ULL)

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

internal inline uint32
SafeTruncateUInt64(uint64 Value)
{
	// TODO(casey): Defines for maximum values
	Assert(Value <= 0xFFFFFFFF);
	uint32 Result = (uint32)Value;
	return Result;
}

inline internal game_controller_input *
GetController(game_input *Input, int ControllerIndex)
{
	Assert(ControllerIndex < ArrayCount(Input->Controllers));
	return &Input->Controllers[ControllerIndex];
}

// TODO(casey): swap, min, max, ... macros???
// TODO(grigory): paint NOTE, STUDY, IMPORTANT words beautifully

struct tile_map
{
	int32 CountX;
	int32 CountY;

	float UpperLeftX;
	float UpperLeftY;
	float TileWidth;
	float TileHeight;

	uint32 *Tiles;
};

struct world
{
	// TODO(casey): Beginner's sparsness
	int32 CountX;
	int32 CountY;

	tile_map *TileMaps;
};

struct game_state
{
	float PlayerX;
	float PlayerY;
};
