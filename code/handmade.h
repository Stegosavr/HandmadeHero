// NOTE(casey):
//
// HANDMADE_INTERNAL:
// 0 - Build for public release
// 1 - Build for developer only
//
// HANDMADE_SLOW:
// 0 - Not slow code allowed!
// 1 - Slow code welcome.

#include <stdint.h>
// TODO(casey): implement sinf ourselves
#include <math.h>

#define internal 		static 
#define global_variable static 
#define local_persist 	static 

#define Pi32 3.14159265359f

// TODO(grigory): get rid of 'float' type, so we can know actual size of variables
// btw do it using vim replace or smth
typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

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

// TODO(casey): swap, min, max, ... macros???

//
// NOTE(casey): Services that the platform layer provides to the game.
//

// TODO(grigory): paint NOTE, STUDY, IMPORTANT words beautifully
#if HANDMADE_INTERNAL
// IMPORTANT(casey):
//
// These are NOT for doing anything in the shipping game - they are
// blocking and the write doesn't protect against lost data!

struct debug_read_file_result
{
	uint32 ContentsSize;
	void *Contents;
};

#define DEBUG_PLATFORM_READ_ENTIRE_FILE(name) debug_read_file_result name(char *Filename)
typedef DEBUG_PLATFORM_READ_ENTIRE_FILE(debug_platform_read_entire_file);

#define DEBUG_PLATFORM_FREE_FILE_MEMORY(name) void name(void *FileMemory)
typedef DEBUG_PLATFORM_FREE_FILE_MEMORY(debug_platform_free_file_memory);

#define DEBUG_PLATFORM_WRITE_ENTIRE_FILE(name) bool name(char *Filename, uint32 MemorySize, void* Memory)
typedef DEBUG_PLATFORM_WRITE_ENTIRE_FILE(debug_platform_write_entire_file);

#endif

//
// NOTE(casey): Services that the game provides to the platform layer.
// (this may expand in the future - sound on separate thread, etc.)
//

// timing, input, bitmap, sound	

// TODO(casey): In the future, rendering _specifically_ will become a three-tiered abstraction!!!
struct game_offscreen_buffer
{
	// NOTE(casey): Pixels are always 32-bits wide, BB GG RR XX
	void *Memory;
	int Width;
	int Height;
	int Pitch;
	int BytesPerPixel;
};

struct game_output_sound_buffer
{
	int SamplesPerSecond;
	int SampleCount;
	int16 *Samples;
};

struct game_button_state
{
	int HalfTransitionCount;
	bool EndedDown;
};

struct game_controller_input
{
	bool IsConnected;
	bool IsAnalog;

	float StickAverageX;
	float StickAverageY;

	union
	{
		game_button_state Buttons[12];
		struct
		{
			game_button_state MoveUp;
			game_button_state MoveDown;
			game_button_state MoveLeft;
			game_button_state MoveRight;

			game_button_state ActionUp;
			game_button_state ActionDown;
			game_button_state ActionLeft;
			game_button_state ActionRight;

			game_button_state LeftShoulder;
			game_button_state RightShoulder;

			game_button_state Start;
			game_button_state Back;

			// NOTE(casey): All buttons must be added above this line
			
			game_button_state Terminator;
		};
	};
};

struct game_input
{
	// TODO(casey): insert clock value here
	// float SecondsElapsed;
	game_controller_input Controllers[5];
};

inline internal game_controller_input *
GetController(game_input *Input, int ControllerIndex)
{
	Assert(ControllerIndex < ArrayCount(Input->Controllers));
	return &Input->Controllers[ControllerIndex];
}

struct game_memory
{
	bool IsInitialized;
	uint64 PermanentStorageSize;
	void *PermanentStorage; // NOTE(casey): REQUIRED to be cleared to zero at startup
							//
	uint64 TransientStorageSize;
	void *TransientStorage; // NOTE(casey): REQUIRED to be cleared to zero at startup

	debug_platform_read_entire_file *DEBUGPlatformReadEntireFile;
	debug_platform_free_file_memory *DEBUGPlatformFreeFileMemory;
	debug_platform_write_entire_file *DEBUGPlatformWriteEntireFile;
};

#define GAME_UPDATE_AND_RENDER(name) void name(game_memory *Memory, game_input* Input, game_offscreen_buffer *Buffer)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);
GAME_UPDATE_AND_RENDER(GameUpdateAndRenderStub)
{
	return;
}

// NOTE(casey): At the moment, this has to be a very fast function,
// it cannot be more than a millisecond or so.
// TODO(casey): Reduce pressure on this function's performance
// by measuring it ot asking about it, etc.
#define GAME_GET_SOUND_SAMPLES(name) void name(game_memory *Memory, game_output_sound_buffer *SoundBuffer)
typedef GAME_GET_SOUND_SAMPLES(game_get_sound_samples);
GAME_GET_SOUND_SAMPLES(GameGetSoundSamplesStub)
{
	return;
}

//
// NOTE(grigory): stuff that platform layer dont care about 
//

struct game_state
{
	int ToneHz;
	int XOffset;
	int YOffset;

	float tSine;

	int PlayerX;
	int PlayerY;
	float tJump;
};

