#include <stdint.h>

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


// NOTE(grigory): This struct should be passed to all game code calls 
// in order to know from which thread we are running. Its unused for now
// You can do that by different calls, but Casey finds this easier
struct thread_context
{
	int Placeholder;	
};


//
// NOTE(casey): Services that the platform layer provides to the game.
//

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

	#define DEBUG_PLATFORM_READ_ENTIRE_FILE(name) debug_read_file_result name(thread_context *Thread, char *Filename)
	typedef DEBUG_PLATFORM_READ_ENTIRE_FILE(debug_platform_read_entire_file);

	#define DEBUG_PLATFORM_FREE_FILE_MEMORY(name) void name(thread_context *Thread, void *FileMemory)
	typedef DEBUG_PLATFORM_FREE_FILE_MEMORY(debug_platform_free_file_memory);

	#define DEBUG_PLATFORM_WRITE_ENTIRE_FILE(name) bool name(thread_context *Thread, char *Filename, uint32 MemorySize, void* Memory)
	typedef DEBUG_PLATFORM_WRITE_ENTIRE_FILE(debug_platform_write_entire_file);

#endif


//
// NOTE(casey): Services that the game provides to the platform layer.
// (this may expand in the future - sound on separate thread, etc.)
//

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
	game_button_state MouseButtons[5];
	int32 MouseX;
	int32 MouseY;
	int32 MouseZ;

	float dtForFrame;
	game_controller_input Controllers[5];
};


struct game_memory
{
	bool IsInitialized;
	uint64 PermanentStorageSize;
	void *PermanentStorage; // NOTE(casey): REQUIRED to be cleared to zero at startup
							
	uint64 TransientStorageSize;
	void *TransientStorage; // NOTE(casey): REQUIRED to be cleared to zero at startup

	debug_platform_read_entire_file *DEBUGPlatformReadEntireFile;
	debug_platform_free_file_memory *DEBUGPlatformFreeFileMemory;
	debug_platform_write_entire_file *DEBUGPlatformWriteEntireFile;
};

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


#define GAME_UPDATE_AND_RENDER(name) void name(thread_context *Thread, game_memory *Memory, game_input* Input, game_offscreen_buffer *Buffer)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);

// NOTE(casey): At the moment, this has to be a very fast function,
// it cannot be more than a millisecond or so.
// TODO(casey): Reduce pressure on this function's performance
// by measuring it ot asking about it, etc.
#define GAME_GET_SOUND_SAMPLES(name) void name(thread_context *Thread, game_memory *Memory, game_output_sound_buffer *SoundBuffer)
typedef GAME_GET_SOUND_SAMPLES(game_get_sound_samples);
