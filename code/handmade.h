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

struct canonical_position
{
	// TODO(casey):
	// Take the tile map x and y
	// and tile x and y
	//
	// and pack them into single 32-bit values for x and y
	// where there is some low bits for the tile index
	// and the high bits are tile "page"
	int32 TileMapX;
	int32 TileMapY;

	int32 TileX;
	int32 TileY;

	// TODO(casey):
	// convert this to math-friendly, resolution independent representation of
	// world units relative to a tile
	float TileRelX;
	float TileRelY;
};

// TODO(casey): Is this ever necessary?
struct raw_position
{
	int32 TileMapX;
	int32 TileMapY;

	// NOTE(casey): tile-map relative X and Y
	float X;
	float Y;
};

struct tile_map
{
	uint32 *Tiles;
};

struct world
{
	float TileSideInMeters;
	int32 TileSideInPixels;

	int32 CountX;
	int32 CountY;

	float UpperLeftX;
	float UpperLeftY;

	// TODO(casey): Beginner's sparsness
	int32 TileMapCountX;
	int32 TileMapCountY;

	tile_map *TileMaps;
};

struct game_state
{
	// TODO(casey): Should be canonical pos now?
	int32 PlayerTileMapX;
	int32 PlayerTileMapY;

	float PlayerX;
	float PlayerY;
};
