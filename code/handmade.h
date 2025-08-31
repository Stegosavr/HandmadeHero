// NOTE(casey):
//
// HANDMADE_INTERNAL:
// 0 - Build for public release
// 1 - Build for developer only
//
// HANDMADE_SLOW:
// 0 - Not slow code allowed!
// 1 - Slow code welcome.

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

#if HANDMADE_INTERNAL
// TODO(grigory): paint NOTE, STUDY, IMPORTANT words beautifully

// IMPORTANT(casey):
//
// These are NOT for doing anything in the shipping game - they are
// blocking and the write doesn't protect against lost data!

struct debug_read_file_result
{
	uint32 ContentsSize;
	void *Contents;
};
internal debug_read_file_result DEBUGPlatformReadEntireFile(char *Filename);
internal void DEBUGPlatformFreeFileMemory(void *FileMemory);
internal bool DEBUGPlatformWriteEntireFile(char *Filename, uint32 MemorySize, void* Memory);
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
};

internal void 
GameUpdateAndRender(
					game_memory *Memory,
					game_input* Input,
					game_offscreen_buffer *Buffer
					);

// NOTE(casey): At the moment, this has to be a very fast function,
// it cannot be more than a millisecond or so.
// TODO(casey): Reduce pressure on this function's performance
// by measuring it ot asking about it, etc.
internal void
GameGetSoundSamples(game_memory *Memory, game_output_sound_buffer *SoundBuffer);

//
// NOTE(grigory): stuff that platform layer dont care about 
//

struct game_state
{
	int ToneHz;
	int XOffset;
	int YOffset;
};

