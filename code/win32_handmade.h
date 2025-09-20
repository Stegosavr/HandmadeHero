struct window_size
{
	int Width;
	int Height;
};

struct win32_offscreen_buffer
{
	// NOTE(casey): Pixels are always 32-bits wide, BB GG RR XX
	BITMAPINFO Info;
	void *Memory;
	int Width;
	int Height;
	int Pitch;
	int BytesPerPixel;
};

struct win32_sound_output
{
	int SamplesPerSecond;
	uint32 RunningSampleIndex;
	int BytesPerSample;
	DWORD BufferSize;
	DWORD SafetyBytes;
	// TODO(casey): Math gets simples if we add a "BytesPerSecond" field?
	// TODO(casey): Maybe RunningIndex should be in bytes
};

struct win32_debug_time_marker
{
	DWORD OutputPlayCursor;
	DWORD OutputWriteCursor;
	DWORD OutputLocation;
	DWORD OutputByteCount;

	DWORD ExpectedFlipPlayCursor;
	DWORD FlipPlayCursor;
	DWORD FlipWriteCursor;
};

struct win32_game_code
{
	HMODULE GameCodeDLL;
	FILETIME DLLLastWriteTime;

	// IMPORTANT(casey): Either of the callbacks can be 0!
	// You must check before calling
	game_update_and_render *UpdateAndRender;
	game_get_sound_samples *GetSoundSamples;

	bool IsValid;
};

#define WIN32_STATE_FILENAME_COUNT MAX_PATH
struct win32_replay_buffer
{
	HANDLE FileHandle;
	HANDLE MemoryMap;
	char Filename[WIN32_STATE_FILENAME_COUNT];
	void *MemoryBlock;
};

struct win32_state
{
	uint64 TotalSize;
	void* GameMemoryBlock;
	win32_replay_buffer ReplayBuffers[4];

	HANDLE RecordingHandle;
	int InputRecordingIndex;

	HANDLE PlaybackHandle;
	int InputPlayingIndex;

	char EXEFilename[WIN32_STATE_FILENAME_COUNT];
	char *OnePastLastEXEFilenameSlash;
};
