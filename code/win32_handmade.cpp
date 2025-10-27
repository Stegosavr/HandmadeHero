//
// - You noted that a stable API is the most underrated thing in computing. 
// What are some of your favorite stable APIs and what would you recommend we use more?
//
// - So there's like one and a half out. Win32 is the stable API. I mean it's just is.
// Windows has like special code in Windows95 to to make Sim City run ...
// That is what a really really trustworthy platform holder does.
// 
// [Eskil Steenberg at BSC2025]
//

/*
 TODO(casey): THIS IS NOT A FINAL PLATFORM LAYER!!!
 
 [ ] Saved game locations
 [ ] Getting a handle to our executable file
 [ ] Asset loading path
 [ ] Threading (launch a thread)
 [ ] Raw Input (support for multiple keyboards)
 [x] Sleep/timeBeginPeriod
 [x] Fullscreen support
 [ ] ClipCursor() (for multimonitor support)
 [ ] QueryCancelAutoplay
 [ ] WM_ACTIVATEAPP (for when we are not active app)
 [ ] Blit Speed improvements (BitBlt)
 [ ] Hardware acceleration (OpenGL or Direct3D or BOTH?)
 [ ] GetKeyboardLayout (French, internetional WASD support)
 [ ] ChangeDisplaySetting option if we detect slow fullscreen blit??
 
 Just a partial list of stuff!
*/

#include "handmade_platform.h"

#include <windows.h>
#include <malloc.h>
#include <xinput.h>
#include <dsound.h>

#include "win32_handmade.h"

global_variable bool Running;
global_variable bool GlobalPause;
global_variable win32_offscreen_buffer GlobalBackbuffer;
global_variable LPDIRECTSOUNDBUFFER SecondaryBuffer;
global_variable int64 GlobalPerfCountFrequency;
global_variable bool DEBUGGlobalShowCursor;
global_variable WINDOWPLACEMENT GlobalWindowPosition = {sizeof(GlobalWindowPosition)};

//NOTE(casey): XInputGetState
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE* pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub)
{
	return ERROR_DEVICE_NOT_CONNECTED;
}
// NOTE(grigory): name ends with _ in order to not conflict with declaration
// from 'xinput.h'. We are then fixing it with macro (dont conflict with declarations).
// Macro also prevents us from calling original function decalration from windows header
global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

//NOTE(casey): XInputSetState
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub)
{
	return ERROR_DEVICE_NOT_CONNECTED;
}
global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_


#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter);
typedef DIRECT_SOUND_CREATE(direct_sound_create);


internal int 
StringLength(char *String)
{
	int Count = 0;
	while (*String++) 
	{
		++Count;
	}
	return Count;
}

internal void
CatString(size_t SourceACount, char *SourceA,
		  size_t SourceBCount, char *SourceB,
		  size_t DestCount, char *Dest)
{
	// TODO(casey): Dest bounds checking!
	for (int Index = 0; Index < SourceACount; ++Index)
	{
		*Dest++ = SourceA[Index];
	}
	for (int Index = 0; Index < SourceBCount; ++Index)
	{
		*Dest++ = SourceB[Index];
	}
	*Dest++ = 0;
}

internal void
Win32GetEXEFilename(win32_state *State)
{
	// NOTE(casey): Never use MAX_PATH in code that is user-facing, cause it
	// can be dangerous and lead to bad results
	DWORD SizeOfFilename = GetModuleFileNameA(0, State->EXEFilename,
											  sizeof(State->EXEFilename));
	char *OnePastLastSlash = State->EXEFilename;
	for (char *Scan = State->EXEFilename; *Scan; ++Scan)
	{
		if (*Scan == '\\')
		{
			State->OnePastLastEXEFilenameSlash = Scan + 1;
		}
	}

}

internal void
Win32BuildEXEPathFilename(win32_state *State, char *Filename,
						  int DestCount, char *Dest)
{
	CatString(State->OnePastLastEXEFilenameSlash - State->EXEFilename, 
			  State->EXEFilename,
			  StringLength(Filename), Filename,
			  DestCount, Dest);
}


DEBUG_PLATFORM_FREE_FILE_MEMORY(DEBUGPlatformFreeFileMemory)
{
	if (FileMemory)
	{
		VirtualFree(FileMemory, 0, MEM_RELEASE);
	}
}

DEBUG_PLATFORM_READ_ENTIRE_FILE(DEBUGPlatformReadEntireFile)
{
	debug_read_file_result Result = {};

	HANDLE FileHandle = CreateFileA(Filename, GENERIC_READ, FILE_SHARE_READ, 
								  0, OPEN_EXISTING, 0, 0);
	if (FileHandle != INVALID_HANDLE_VALUE)
	{
		LARGE_INTEGER FileSize;
		if (GetFileSizeEx(FileHandle, &FileSize))
		{
			uint32 FileSize32 = SafeTruncateUInt64(FileSize.QuadPart);
			Result.Contents = VirtualAlloc(0, FileSize32, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
			if (Result.Contents)
			{
				DWORD BytesRead;
				if (ReadFile(FileHandle, Result.Contents, FileSize32, &BytesRead, 0) &&
					(FileSize32 == BytesRead))
				{
					// NOTE(casey): File read successfully
					Result.ContentsSize = BytesRead;
				}
				else
				{
					// TODO(casey): Logging
					DEBUGPlatformFreeFileMemory(Thread, Result.Contents);
					Result.Contents = 0;
				}
			}
			else
			{
				// TODO(casey): Logging
			}
		}
		else
		{
			// TODO(casey): Logging
		}

		CloseHandle(FileHandle);
	}
	else
	{
		// TODO(casey): Logging
	}

	return Result;
}

DEBUG_PLATFORM_WRITE_ENTIRE_FILE(DEBUGPlatformWriteEntireFile)
{
	bool Result = false;

	HANDLE FileHandle = CreateFileA(Filename, GENERIC_WRITE, 0, 
								  0, CREATE_ALWAYS, 0, 0);
	if (FileHandle != INVALID_HANDLE_VALUE)
	{
		DWORD BytesWritten;
		if(WriteFile(FileHandle, Memory, MemorySize, &BytesWritten, 0))
		{
			// NOTE(casey): File written successfully
			Result = (BytesWritten == MemorySize);
		}
		else
		{
			// TODO(casey): Logging
		}

		CloseHandle(FileHandle);
	}
	else
	{
		// TODO(casey): Logging
	}

	return Result;
}

inline FILETIME
Win32GetLastWriteTime(char *Filename)
{
	FILETIME LastWriteTime = {};

	WIN32_FILE_ATTRIBUTE_DATA Data = {};
	if (GetFileAttributesExA(Filename, GetFileExInfoStandard, &Data))
	{
		LastWriteTime = Data.ftLastWriteTime;
	}

	return LastWriteTime;
}

internal win32_game_code 
Win32LoadGameCode(char *SourceDLLName, char *TempDLLName)
{
	win32_game_code Result = {};

	// TODO(casey): Need to get the proper path here!
	// TODO(casey): Automatic determination when updates are necessary.
	
	Result.DLLLastWriteTime = Win32GetLastWriteTime(SourceDLLName);
	CopyFile(SourceDLLName, TempDLLName, FALSE);
	Result.GameCodeDLL = LoadLibraryA(TempDLLName);
	if (Result.GameCodeDLL)
	{
		Result.UpdateAndRender = (game_update_and_render *)
			GetProcAddress(Result.GameCodeDLL, "GameUpdateAndRender");
		Result.GetSoundSamples = (game_get_sound_samples *)
			GetProcAddress(Result.GameCodeDLL, "GameGetSoundSamples");

		Result.IsValid = (Result.UpdateAndRender &&
						  Result.GetSoundSamples);
	}

	if (!Result.IsValid)
	{
		Result.UpdateAndRender = 0;
		Result.GetSoundSamples = 0;
	}

	return Result;
}

internal void 
Win32UnloadGameCode(win32_game_code *GameCode)
{
	if (GameCode->GameCodeDLL)
	{
		FreeLibrary(GameCode->GameCodeDLL);
		GameCode->GameCodeDLL = 0;
	}

	GameCode->IsValid = false;
	GameCode->UpdateAndRender = 0;
	GameCode->GetSoundSamples = 0;
}

// NOTE(grigory): loading dynamic libs ourselves instead of requiring direct linking
// so that we can run executable even if xinput.dll is not present on target machine
internal void Win32LoadXInput(void)
{
	HMODULE XInputLibrary = LoadLibraryA("xinput1_3.dll");
	//TODO: diagnostic
	if (!XInputLibrary)
		XInputLibrary = LoadLibraryA("xinput1_4.dll");
	if (!XInputLibrary)
		XInputLibrary = LoadLibraryA("xinput9_1_0.dll");
	if (XInputLibrary)
	{
		XInputGetState = (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
		XInputSetState = (x_input_set_state *)GetProcAddress(XInputLibrary, "XInputSetState");
	}
	else
	{
		//TODO: diagnostic
	}
}

internal void
Win32InitDSound(HWND Window, int32 SamplesPerSecond, int32 BufferSize)
{
	//NOTE: Load the library
	HMODULE DSoundLibrary = LoadLibraryA("dsound.dll");
	
	if (DSoundLibrary)
	{
		//NOTE: get a direct sound object
		direct_sound_create *DirectSoundCreate = 
			(direct_sound_create *)GetProcAddress(DSoundLibrary, "DirectSoundCreate");

		LPDIRECTSOUND	DirectSound;

		if (DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0)))
		{
			WAVEFORMATEX WaveFormat = {};
			WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
			WaveFormat.nChannels = 2;
			WaveFormat.nSamplesPerSec = SamplesPerSecond;
			WaveFormat.wBitsPerSample = 16;
			WaveFormat.nBlockAlign = (WaveFormat.nChannels*WaveFormat.wBitsPerSample) / 8;
			WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec*WaveFormat.nBlockAlign;
			WaveFormat.cbSize = 0;

			if (SUCCEEDED(DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY)))
			{
				DSBUFFERDESC BufferDescription = {};
				BufferDescription.dwSize = sizeof(BufferDescription);
				BufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;

				//NOTE: Create a primary buffer
				LPDIRECTSOUNDBUFFER PrimaryBuffer;
				if (SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &PrimaryBuffer, 0)))
				{
					if (SUCCEEDED(PrimaryBuffer->SetFormat(&WaveFormat)))
					{
						//NOTE: we set the format! yay
						OutputDebugStringA("Primaty buffer format was set\n");
					}
					else
					{
						//TODO: Diagnostic
					}
				}
				else
				{
					//TODO: Diagnostic
				}
			}
			else
			{
				//TODO: diagnostic
			}
			
			//NOTE: Create a secondary buffer
			DSBUFFERDESC BufferDescription = {};
			BufferDescription.dwSize = sizeof(BufferDescription);
			BufferDescription.dwFlags = 0;
			BufferDescription.dwBufferBytes = BufferSize;
			BufferDescription.lpwfxFormat = &WaveFormat;

			if (SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &SecondaryBuffer, 0)))
			{
				OutputDebugStringA("Secondary buffer created successfully\n");
			}
			else
			{
				//TODO: diagnostic
			}

		}
		else
		{
			//TODO: diagnostic
		}
	}
	else
	{
		//TODO: diagnostic
	}
}

internal window_size GetWindowSize(HWND Window)
{
	window_size WindowSize;

	RECT ClientRect;
	GetClientRect(Window, &ClientRect);
	WindowSize.Width = ClientRect.right - ClientRect.left;
	WindowSize.Height = ClientRect.bottom - ClientRect.top;

	return WindowSize;
}

internal void Win32ResizeDIBSection(win32_offscreen_buffer *Buffer, int Width, int Height)
{
	if (Buffer->Memory)
	{
		VirtualFree(Buffer->Memory, 0, MEM_RELEASE);	
	}

	int BytesPerPixel = 4;
	Buffer->BytesPerPixel = BytesPerPixel;
	Buffer->Width = Width;
	Buffer->Height = Height;
	
	Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
	Buffer->Info.bmiHeader.biWidth = Width;
	Buffer->Info.bmiHeader.biHeight = -Height;
	Buffer->Info.bmiHeader.biPlanes = 1;
	Buffer->Info.bmiHeader.biBitCount = 32;
	Buffer->Info.bmiHeader.biCompression = BI_RGB;
	Buffer->Pitch = Width*BytesPerPixel;

	//NOTE(casey) Chris Hecker simplified this part quite a lot, no more Device Context

	int BitmapMemorySize = Width * Height * BytesPerPixel;
	Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

	//TODO(casey) probably clear to black;
}

internal void Win32DisplayBufferInWindow(HDC DeviceContext, 
										 int WindowWidth, int WindowHeight,
										 win32_offscreen_buffer *Buffer)
{
	if ((WindowWidth  >= Buffer->Width*2) &&
		(WindowHeight >= Buffer->Height*2))
	{
		// TODO(casey): Centering / black bars?
		StretchDIBits(DeviceContext,
					  0, 0, Buffer->Width*2, Buffer->Height*2,
					  0, 0, Buffer->Width, Buffer->Height,
					  Buffer->Memory,
					  &Buffer->Info,
					  DIB_RGB_COLORS, SRCCOPY);
	}
	else
	{
		int OffsetX = 10;
		int OffsetY = 10;

		PatBlt(DeviceContext, 0, 0, OffsetX, WindowHeight, BLACKNESS);
		PatBlt(DeviceContext, 0, 0, WindowWidth, OffsetY,  BLACKNESS);
		PatBlt(DeviceContext, 0, OffsetY + Buffer->Height, WindowWidth, WindowHeight - OffsetY - Buffer->Height, BLACKNESS);
		PatBlt(DeviceContext, OffsetX + Buffer->Width, 0, WindowWidth - OffsetX - Buffer->Width, WindowHeight, BLACKNESS);

		// NOTE(casey): For prototyping purposesm we're going ro always blit
		// 1-to-1 pixels to make sure we don't introduce artifacts with
		// stretching while we are learning to code the renderer!
		StretchDIBits(DeviceContext,
					  OffsetX, OffsetY, Buffer->Width, Buffer->Height, //0, 0, WindowWidth, WindowHeight,
					  0, 0, Buffer->Width, Buffer->Height,
					  Buffer->Memory,
					  &Buffer->Info,
					  DIB_RGB_COLORS, SRCCOPY);
	}
}

LRESULT CALLBACK Win32MainWindowCallback(
	HWND Window,
	UINT Message,
	WPARAM WParam,
	LPARAM LParam )
{
	LRESULT Result = 0;

	switch (Message)
	{
		case WM_SETCURSOR:
		{
			if (DEBUGGlobalShowCursor)
			{
				Result = DefWindowProcA(Window, Message, WParam, LParam);
			}
			else
			{
				SetCursor(0);
			}
		} break;

		case WM_SIZE:
		{
			//window_size WindowSize = GetWindowSize(Window);
			//Win32ResizeDIBSection(&GlobalBackbuffer, WindowSize.Width, WindowSize.Height);
		} break;

		case WM_DESTROY:
		{
			// TODO(casey): Handle and recreate window?
			Running = false;
		} break;	

		case WM_CLOSE:
		{
			Running = false;
		} break;	

		case WM_ACTIVATEAPP:
		{
			OutputDebugStringA("ACTIVATE\n");
		} break;	

		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
		case WM_KEYDOWN:
		case WM_KEYUP:
		{
			Assert(!"Keyboard input came in through a non-dispatch msg peek!");
		} break;

		case WM_PAINT:
		{
			PAINTSTRUCT Paint;
			HDC DeviceContext = BeginPaint(Window, &Paint);
			window_size WindowSize = GetWindowSize(Window);

			Win32DisplayBufferInWindow(DeviceContext, WindowSize.Width, WindowSize.Height, &GlobalBackbuffer);
			EndPaint(Window, &Paint);
		} break;	

		default:
		{
			Result = DefWindowProcA(Window, Message, WParam, LParam);
		} break;
	}

	return Result;

}

internal void 
Win32ClearSoundBuffer(win32_sound_output *SoundOutput)
{
	VOID *Region1;
	DWORD Region1Size;
	VOID *Region2;
	DWORD Region2Size;

	if (SUCCEEDED(SecondaryBuffer->Lock(0, SoundOutput->BufferSize, 
										&Region1, &Region1Size, 
										&Region2, &Region2Size, 
										0)))
	{
		uint8 *DestByte = (uint8*)Region1;
		for (DWORD ByteIndex = 0; ByteIndex < Region1Size; ++ByteIndex)
		{
			*DestByte++ = 0;
		}

		DestByte = (uint8*)Region2;
		for (DWORD ByteIndex = 0; ByteIndex < Region2Size; ++ByteIndex)
		{
			*DestByte++ = 0;
		}

		SecondaryBuffer->Unlock(Region1, Region1Size,
								Region2, Region2Size);
	}
}

internal void 
Win32FillSoundBuffer(win32_sound_output *SoundOutput, DWORD ByteToLock, DWORD BytesToWrite,
					 game_output_sound_buffer *SourceBuffer)
{
	// day009
	VOID *Region1;
	DWORD Region1Size;
	VOID *Region2;
	DWORD Region2Size;

	if (SUCCEEDED(SecondaryBuffer->Lock(ByteToLock, BytesToWrite, 
										&Region1, &Region1Size, 
										&Region2, &Region2Size, 
										0)))
	{
		// TODO: assert that RegionSize is valid
		//
		int16 *DestSample = (int16*)Region1;
		int16 *SourceSample = SourceBuffer->Samples;
		DWORD Region1SampleCount =  Region1Size/SoundOutput->BytesPerSample;
		for (DWORD SampleIndex = 0; SampleIndex < Region1SampleCount; ++SampleIndex)
		{
			*DestSample++ = *SourceSample++;
			*DestSample++ = *SourceSample++;
			++SoundOutput->RunningSampleIndex;
		}

		DestSample = (int16*)Region2;
		DWORD Region2SampleCount =  Region2Size/SoundOutput->BytesPerSample;
		for (DWORD SampleIndex = 0; SampleIndex < Region2SampleCount; ++SampleIndex)
		{
			*DestSample++ = *SourceSample++;
			*DestSample++ = *SourceSample++;
			++SoundOutput->RunningSampleIndex;
		}
		SecondaryBuffer->Unlock(Region1, Region1Size,
								Region2, Region2Size);
	}
}

internal void 
Win32ProcessKeyboardEvent(game_button_state *NewState, bool IsDown)
{
	if (NewState->EndedDown != IsDown)
	{
		NewState->EndedDown = IsDown;
		++NewState->HalfTransitionCount;
	}
}

internal void 
Win32ProcessXInputDigitalButton(DWORD XInputButtonState, game_button_state *OldState, DWORD ButtonBit, game_button_state *NewState)
{
	NewState->EndedDown = (XInputButtonState & ButtonBit);
	NewState->HalfTransitionCount =
		(OldState->EndedDown != NewState->EndedDown) ? 1 : 0;
}

internal float 
Win32ProcessXInputStickValue(SHORT Value, SHORT DeadZoneThreshold)
{
	float Result = 0;

	if (Value < -DeadZoneThreshold)
	{
		Result = (float)(Value + DeadZoneThreshold) / (32768.0f - DeadZoneThreshold);
	}
	else if (Value > DeadZoneThreshold)
	{
		Result = (float)(Value - DeadZoneThreshold) / (32767.0f - DeadZoneThreshold);
	}

	return Result;
}

internal void
Win32GetInputFileLocation(win32_state *State, bool InputStream, int SlotIndex,
						  int DestCount, char *Dest)
{
	char Temp[64] = {};
	wsprintf(Temp, "loop_edit_%d.hm%s", SlotIndex, InputStream ? "i" : "s");
	Win32BuildEXEPathFilename(State, Temp, DestCount, Dest);
}

internal win32_replay_buffer *
Win32GetReplayBuffer(win32_state *State, int Index)
{ 
	Assert(Index > 0);
	Assert(Index < ArrayCount(State->ReplayBuffers));
	win32_replay_buffer *Result = &State->ReplayBuffers[Index];
	return Result;
}

internal void
Win32BeginRecordingInput(win32_state *State, int InputRecordingIndex)
{
	win32_replay_buffer *ReplayBuffer = Win32GetReplayBuffer(State, InputRecordingIndex);

	if (ReplayBuffer->MemoryBlock)
	{
		State->InputRecordingIndex = InputRecordingIndex;

		char Filename[WIN32_STATE_FILENAME_COUNT];
		Win32GetInputFileLocation(State, true, InputRecordingIndex,
								  sizeof(Filename), 
								  Filename);
		State->RecordingHandle = CreateFileA(Filename, GENERIC_WRITE,
											 0, 0, CREATE_ALWAYS, 0, 0);
		 
		// NOTE(grigory): it should make writing large empty files to disk faster
		//DWORD Ignored;
		// but it isnt working
		//DeviceIoControl(State->RecordingHandle, FSCTL_SET_SPARSE, 0, 0, 0, 0, &Ignored, 0);
#if 0
		LARGE_INTEGER FilePosition;
		FilePosition.QuadPart = State->TotalSize;
		SetFilePointerEx(State->RecordingHandle, FilePosition, 0, FILE_BEGIN);
#endif
		CopyMemory(ReplayBuffer->MemoryBlock, State->GameMemoryBlock, State->TotalSize);
	}
}

internal void
Win32EndRecordingInput(win32_state *State)
{
	CloseHandle(State->RecordingHandle);
	State->InputRecordingIndex = 0;
}

internal void
Win32BeginPlaybackInput(win32_state *State, int InputPlayingIndex)
{
	win32_replay_buffer *ReplayBuffer = Win32GetReplayBuffer(State, InputPlayingIndex);

	if (ReplayBuffer->MemoryBlock)
	{
		State->InputPlayingIndex = InputPlayingIndex;

		//State->PlaybackHandle = ReplayBuffer->FileHandle;
		char Filename[WIN32_STATE_FILENAME_COUNT];
		Win32GetInputFileLocation(State, true, InputPlayingIndex,
								  sizeof(Filename), 
								  Filename);
		State->PlaybackHandle = CreateFileA(Filename, GENERIC_READ,
											 0, 0, OPEN_EXISTING, 0, 0);
#if 0
		LARGE_INTEGER FilePosition;
		FilePosition.QuadPart = State->TotalSize;
		SetFilePointerEx(State->PlaybackHandle, FilePosition, 0, FILE_BEGIN);
#endif
		CopyMemory(State->GameMemoryBlock, ReplayBuffer->MemoryBlock, State->TotalSize);
	}
}

internal void
Win32EndPlaybackInput(win32_state *State)
{
	CloseHandle(State->PlaybackHandle);
	State->InputPlayingIndex = 0;
}

internal void
Win32RecordInput(win32_state *State, game_input *NewInput)
{

	DWORD BytesWritten;
	WriteFile(State->RecordingHandle, NewInput, 
			  sizeof(*NewInput), &BytesWritten, 0);
}

internal void
Win32PlaybackInput(win32_state *State, game_input *NewInput)
{

	DWORD BytesRead = 0;
	if (ReadFile(State->PlaybackHandle, NewInput, 
				 sizeof(*NewInput), &BytesRead, 0))
	{
		if (BytesRead == 0)
		{
			// NOTE(casey): We've hit the end of the stream, go back to the beginning
			int PlayingIndex = State->InputPlayingIndex;
			Win32EndPlaybackInput(State);

			Win32BeginPlaybackInput(State, PlayingIndex);
			ReadFile(State->PlaybackHandle, NewInput, 
					 sizeof(*NewInput), &BytesRead, 0);
		}
	}
}

internal void 
ToggleFullscreen(HWND Window)
{
	// NOTE(casey): This follows Raymond Chen's prescription
	// for fullscreen toggling, see:
	//  https://devblogs.microsoft.com/oldnewthing/20100412-00/?p=14353
	
	DWORD Style = GetWindowLong(Window, GWL_STYLE);
	if (Style & WS_OVERLAPPEDWINDOW) 
	{
		MONITORINFO MonitorInfo = { sizeof(MonitorInfo) };
		if (GetWindowPlacement(Window, &GlobalWindowPosition) &&
			GetMonitorInfo(MonitorFromWindow(Window, MONITOR_DEFAULTTOPRIMARY), 
						   &MonitorInfo)) 
		{
			SetWindowLong(Window, GWL_STYLE,
						  Style & ~WS_OVERLAPPEDWINDOW);
			SetWindowPos(Window, HWND_TOP,
						 MonitorInfo.rcMonitor.left, MonitorInfo.rcMonitor.top,
						 MonitorInfo.rcMonitor.right - MonitorInfo.rcMonitor.left,
						 MonitorInfo.rcMonitor.bottom - MonitorInfo.rcMonitor.top,
						 SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
		}
	} 
	else 
	{
		SetWindowLong(Window, GWL_STYLE,
					  Style | WS_OVERLAPPEDWINDOW);
		SetWindowPlacement(Window, &GlobalWindowPosition);
		SetWindowPos(Window, NULL, 0, 0, 0, 0,
					 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
					 SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
	}
}

internal void
Win32ProcessPendingMessages(win32_state *State, game_controller_input *KeyboardController)
{
	MSG Message;
	while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
	{
		switch(Message.message)			
		{		
			case WM_QUIT:
			{
				Running = false;
			} break;

			case WM_SYSKEYDOWN:
			case WM_SYSKEYUP:
			case WM_KEYDOWN:
			case WM_KEYUP:
			{
				uint32 VKCode = (uint32)Message.wParam;
				bool WasDown = Message.lParam & (1 << 30);
				bool IsDown = (Message.lParam & (1 << 31)) == 0;
				if (WasDown == IsDown)
					break;
				if (VKCode == 'W')
				{
					Win32ProcessKeyboardEvent(&KeyboardController->MoveUp,
											  IsDown);
				}
				else if (VKCode == 'A')
				{
					Win32ProcessKeyboardEvent(&KeyboardController->MoveLeft,
											  IsDown);
				}
				else if (VKCode == 'S')
				{
					Win32ProcessKeyboardEvent(&KeyboardController->MoveDown,
											  IsDown);
				}
				else if (VKCode == 'D')
				{
					Win32ProcessKeyboardEvent(&KeyboardController->MoveRight,
											  IsDown);
				}
				else if (VKCode == 'Q')
				{
					Win32ProcessKeyboardEvent(&KeyboardController->LeftShoulder,
											  IsDown);
				}
				else if (VKCode == 'E')
				{
					Win32ProcessKeyboardEvent(&KeyboardController->RightShoulder,
											  IsDown);
				}
				else if (VKCode == 'P')
				{
					if (IsDown)
						GlobalPause = !GlobalPause;
				}
				else if (VKCode == VK_UP)
				{
					Win32ProcessKeyboardEvent(&KeyboardController->ActionUp,
											  IsDown);
				}
				else if (VKCode == VK_DOWN)
				{
					Win32ProcessKeyboardEvent(&KeyboardController->ActionDown,
											  IsDown);
				}
				else if (VKCode == VK_LEFT)
				{
					Win32ProcessKeyboardEvent(&KeyboardController->ActionLeft,
											  IsDown);
				}
				else if (VKCode == VK_RIGHT)
				{
					Win32ProcessKeyboardEvent(&KeyboardController->ActionRight,
											  IsDown);
				}
				else if (VKCode == VK_ESCAPE)
				{
					Running = false;
				}
				else if (VKCode == VK_SPACE)
				{
				}
				else if (VKCode == 'L')
				{
					if (IsDown)
					{
						if (State->InputPlayingIndex == 0)
						{
							if (State->InputRecordingIndex == 0)
							{
								Win32BeginRecordingInput(State, 1);
							}
							else
							{
								Win32EndRecordingInput(State);
								Win32BeginPlaybackInput(State, 1);
							}
						}
						else
						{
							Win32EndPlaybackInput(State);
						}
					}
				}

				if (IsDown)
				{
					bool AltKeyIsDown = Message.lParam & (1 << 29);
					if (VKCode == VK_F4 && AltKeyIsDown)
					{
						Running = false;
					}
					if ((VKCode == VK_RETURN) && AltKeyIsDown)
					{
						ToggleFullscreen(Message.hwnd);
					}
				}

			} break;

			default:
			{
				TranslateMessage(&Message);
				DispatchMessageA(&Message);
			} break;
		}
	}
}

internal inline LARGE_INTEGER
Win32GetWallClock(void)
{
	LARGE_INTEGER Result;
	QueryPerformanceCounter(&Result);

	return Result;
}

internal inline float
Win32GetSecondsElapsed(LARGE_INTEGER Start, LARGE_INTEGER End)
{
				int64 CounterElapsed = End.QuadPart - Start.QuadPart;
				float Result = ((float)CounterElapsed / (float)GlobalPerfCountFrequency);

				return Result;
}

internal void
Win32DebugDrawVertical(win32_offscreen_buffer *Backbuffer,
					   int X, int Top, int Bottom, uint32 Color)
{
	if (Top <= 0)
		Top = 0;

	if (Bottom >= Backbuffer->Height)
		Bottom = Backbuffer->Height;

	if ((X >= 0) && (X < Backbuffer->Width))
	{
		uint8 *Pixel = ((uint8 *)Backbuffer->Memory + 
						X*Backbuffer->BytesPerPixel + 
						Top*Backbuffer->Pitch); 
		for (int Y = Top; Y < Bottom; ++Y)
		{
			*(uint32 *)Pixel = Color;
			Pixel += Backbuffer->Pitch;
		}
	}
}

internal void
Win32DebugDrawSoundBufferMarker(win32_offscreen_buffer *Backbuffer,
							  	win32_sound_output *SoundOutput,	
								float C, int Top, int Bottom, int PadX, uint32 Color,
								DWORD Marker)
{
	//Assert(Marker < SoundOutput->BufferSize);
	int X = (int)(C * Marker) + PadX;
	Win32DebugDrawVertical(Backbuffer, X, Top, Bottom, Color);
}

internal void
Win32DebugSyncDisplay(win32_offscreen_buffer *Backbuffer, 
					  int MarkerCount, win32_debug_time_marker *Markers, 
					  int CurrentMarkerIndex,
					  win32_sound_output *SoundOutput, float TargetSecondsPerFrame)
{
	int PadX = 16;
	int PadY = 16;
	int LineHeight = 64;

	float C = (float)(Backbuffer->Width - 2*PadX) / (float)SoundOutput->BufferSize;
	for (int MarkerIndex = 0;
		 MarkerIndex < MarkerCount;
		 ++MarkerIndex)
	{
		win32_debug_time_marker Marker = Markers[MarkerIndex];

		Assert(Marker.FlipPlayCursor < SoundOutput->BufferSize);
		Assert(Marker.FlipWriteCursor < SoundOutput->BufferSize);
		Assert(Marker.OutputPlayCursor < SoundOutput->BufferSize);
		Assert(Marker.OutputWriteCursor < SoundOutput->BufferSize);

		Assert(Marker.OutputByteCount < SoundOutput->BufferSize);
		Assert(Marker.OutputLocation < SoundOutput->BufferSize);
	
		int X = 0;
		DWORD PlayColor  = 0xFFFFFFFF;
		DWORD WriteColor = 0xFFFF0000;
		DWORD ExpectedFlipColor = 0x00FFFF00;

		int Top = PadY;
		int Bottom = Top + LineHeight;
		if (MarkerIndex == CurrentMarkerIndex)
		{
			Top += LineHeight + PadY;
			Bottom += LineHeight + PadY;

			Win32DebugDrawSoundBufferMarker(Backbuffer, SoundOutput,	
											C, Top, Bottom, PadX, PlayColor,
											Marker.OutputPlayCursor);
			Win32DebugDrawSoundBufferMarker(Backbuffer, SoundOutput,	
											C, Top, Bottom, PadX, WriteColor, 
											Marker.OutputWriteCursor);
			Win32DebugDrawSoundBufferMarker(Backbuffer, SoundOutput,	
											C, Top, Bottom + 3*LineHeight, PadX, ExpectedFlipColor,
											Marker.ExpectedFlipPlayCursor);

			Top += LineHeight + PadY;
			Bottom += LineHeight + PadY;

			Win32DebugDrawSoundBufferMarker(Backbuffer, SoundOutput,	
											C, Top, Bottom, PadX, PlayColor,
											Marker.OutputLocation);
			Win32DebugDrawSoundBufferMarker(Backbuffer, SoundOutput,	
											C, Top, Bottom, PadX, WriteColor,
											Marker.OutputByteCount + Marker.OutputLocation);

			Top += LineHeight + PadY;
			Bottom += LineHeight + PadY;
		}
		Win32DebugDrawSoundBufferMarker(Backbuffer, SoundOutput,	
										C, Top, Bottom, PadX, PlayColor, 
										Marker.FlipPlayCursor);
		Win32DebugDrawSoundBufferMarker(Backbuffer, SoundOutput,	
										C, Top, Bottom, PadX, WriteColor, 
										Marker.FlipWriteCursor);
	}
}

int CALLBACK WinMain(HINSTANCE Instance, HINSTANCE PrevInstance,
    PSTR CommandLine, int ShowCode)
{
	win32_state Win32State = {};

	LARGE_INTEGER PerfCountFrequencyRes;
	QueryPerformanceFrequency(&PerfCountFrequencyRes);
	GlobalPerfCountFrequency = PerfCountFrequencyRes.QuadPart;

	Win32GetEXEFilename(&Win32State);

	char SourceGameCodeDLLFullPath[WIN32_STATE_FILENAME_COUNT];
	Win32BuildEXEPathFilename(&Win32State, "handmade.dll",
							  sizeof(SourceGameCodeDLLFullPath), 
							  SourceGameCodeDLLFullPath);

	char TempGameCodeDLLFullPath[WIN32_STATE_FILENAME_COUNT];
	Win32BuildEXEPathFilename(&Win32State, "handmade_temp.dll",
							  sizeof(TempGameCodeDLLFullPath), 
							  TempGameCodeDLLFullPath);

	// NOTE(casey): Set the Windows scheduler granularity to 1ms
	// so that our Sleep() can be more granular.
	UINT DesiredSchedulerMS = 1;
	bool SleepIsGranular = (timeBeginPeriod(DesiredSchedulerMS) == TIMERR_NOERROR);

#if HANDMADE_INTERNAL
	DEBUGGlobalShowCursor = true;
#endif
	WNDCLASS WindowClass = {};

	// NOTE(casey): 1080p display mode is 1920x1080 -> half is 960x540
	Win32ResizeDIBSection(&GlobalBackbuffer, 960, 540);

	WindowClass.style = CS_HREDRAW|CS_VREDRAW; 

	WindowClass.lpfnWndProc = Win32MainWindowCallback; 
	WindowClass.hInstance = Instance; 
	WindowClass.hCursor = LoadCursor(0, IDC_ARROW);
	//WindowClass. hIcon; 
	WindowClass.lpszClassName = "HandmadeHeroWindowClass";

	if (RegisterClassA(&WindowClass))
	{
		HWND Window = CreateWindowExA( 
			0,//WS_EX_TOPMOST|WS_EX_LAYERED, 
			WindowClass.lpszClassName, 
			"Handmade Hero", 
			WS_OVERLAPPEDWINDOW|WS_VISIBLE, 
			CW_USEDEFAULT, 
			CW_USEDEFAULT, 
			CW_USEDEFAULT, 
			CW_USEDEFAULT, 
			0, 
			0, 
			Instance, 
			0);
		if (Window)
		{
#if 0
			// NOTE(grigory): transparency setting
			SetLayeredWindowAttributes(Window, RGB(0, 0, 0), 255, LWA_ALPHA);
#endif
			// TODO(casey): How do we reliably query on this on Windows?
			int MonitorRefreshHz = 60;

			HDC RefreshDC = GetDC(Window);
			int Win32RefreshRate = GetDeviceCaps(RefreshDC, VREFRESH);
			ReleaseDC(Window, RefreshDC);

			if (Win32RefreshRate > 1)
			{
				MonitorRefreshHz = Win32RefreshRate;
			}
			float GameUpdateHz = MonitorRefreshHz / 1.0f;
			float TargetSecondsPerFrame = 1.0f / (float)GameUpdateHz;


			Running = true;

			// NOTE: Sound test
			win32_sound_output SoundOutput = {};

			// TODO(casey): Make this like sixty seconds?
			SoundOutput.SamplesPerSecond = 48000;
			SoundOutput.BytesPerSample = sizeof(int16)*2;
			SoundOutput.BufferSize = SoundOutput.SamplesPerSecond*SoundOutput.BytesPerSample;
			// TODO(casey): Actually compute this variance and see what
			// the lowest reasonable value is
			SoundOutput.SafetyBytes =
				(int)(((float)SoundOutput.SamplesPerSecond * (float)SoundOutput.BytesPerSample / GameUpdateHz) / 3.0f);
			Win32InitDSound(Window, SoundOutput.SamplesPerSecond, SoundOutput.BufferSize);
			Win32ClearSoundBuffer(&SoundOutput);
			SecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);
			
#if 0
			// NOTE(casey): This tests the PlayCursor update frequency
			// it was 480 samples
			while (Running)
			{
				DWORD PlayCursor = 0;
				DWORD WriteCursor = 0; 
				SecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor);

					char txt[256] = {};
					wsprintf(txt,
							 "PC:%u WC:%u\n",
							 PlayCursor, WriteCursor);
					OutputDebugStringA(txt);
			}
#endif

			// TODO(casey): Pool with bitmap virtAlloc
			int16 *Samples = (int16*)VirtualAlloc(0, SoundOutput.BufferSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

#if HANDMADE_INTERNALO
			LPVOID BaseAddress = (LPVOID)Terabytes(2);
#else
			LPVOID BaseAddress = 0;
#endif


			game_memory GameMemory = {};
			GameMemory.PermanentStorageSize = Megabytes(64);
			GameMemory.TransientStorageSize = Gigabytes(1);
			GameMemory.DEBUGPlatformReadEntireFile  = DEBUGPlatformReadEntireFile;
			GameMemory.DEBUGPlatformFreeFileMemory  = DEBUGPlatformFreeFileMemory;
			GameMemory.DEBUGPlatformWriteEntireFile = DEBUGPlatformWriteEntireFile;

			// TODO(casey): Handle various memory footprints (USING SYSTEM METRICS)
			// TODO(casey): Transient storage needs to be broken up
			// into game transient andt cache transient, and only the
			// former needs to be saved for state playback
			Win32State.TotalSize = GameMemory.PermanentStorageSize + GameMemory.TransientStorageSize;

			// TODO(casey): USE MEM_LARGE_PAGES and call adjust token priveleges
			// when not on Windows XP?
			// NOTE(grigory): TLB pressure should released with large paging. 
			// TODO(grigory): Learn more about TLB in virtual memory unit
			Win32State.GameMemoryBlock = (void *)VirtualAlloc(
				BaseAddress, (size_t)Win32State.TotalSize, 
				MEM_RESERVE|MEM_COMMIT,//|MEM_LARGE_PAGES, 
				PAGE_READWRITE);

			GameMemory.PermanentStorage = Win32State.GameMemoryBlock;
			GameMemory.TransientStorage = (uint8 *)GameMemory.PermanentStorage + GameMemory.PermanentStorageSize;

			for (int ReplayIndex = 1;
				 ReplayIndex < ArrayCount(Win32State.ReplayBuffers);
				 ++ReplayIndex)
			{
				win32_replay_buffer *ReplayBuffer = 
					&Win32State.ReplayBuffers[ReplayIndex];	
				// TODO(casey): Recording system still seems too long 
				// on record start - find out what Windows is doing and if
				// we can speed up / defer some of that precessing 

				Win32GetInputFileLocation(&Win32State, false, ReplayIndex,
										  sizeof(ReplayBuffer->Filename), 
										  ReplayBuffer->Filename);

				ReplayBuffer->FileHandle = 
					CreateFileA(ReplayBuffer->Filename, GENERIC_WRITE|GENERIC_READ,
							   	0, 0, CREATE_ALWAYS, 0, 0);

				ReplayBuffer->MemoryMap =
					CreateFileMappingA(ReplayBuffer->FileHandle, 0,
									   PAGE_READWRITE, 
									   (Win32State.TotalSize >> 32),
									   (Win32State.TotalSize & 0xFFFFFFFF),
									   0
									  );
				ReplayBuffer->MemoryBlock = MapViewOfFile(ReplayBuffer->MemoryMap,
					FILE_MAP_ALL_ACCESS, 0, 0, Win32State.TotalSize);
				//ReplayBuffer->MemoryBlock = (void *)VirtualAlloc(
				//	0, (size_t)Win32State.TotalSize, 
				//	MEM_RESERVE|MEM_COMMIT,
				//	PAGE_READWRITE);
				

				if (ReplayBuffer->MemoryBlock)
				{
				
				}
				else
				{
					// TODO(casey): Change this to a log message
				}
			}

			if (!(Samples && GameMemory.PermanentStorage && GameMemory.TransientStorage))  
				// NOTE(grigory): reverse condition on casey's side. I just
				// dont like nesting big scopes (wrapping loop below), but
				// need to take into account that difference now 
				// (different codepath executed)
				return 1;

			Win32LoadXInput();

			game_input Input[2] = {};
			game_input *NewInput = &Input[0];
			game_input *OldInput = &Input[1];

			int DebugTimeMarkerIndex = 0;
			win32_debug_time_marker DebugTimeMarkers[30] = {};

			LARGE_INTEGER LastCounter = Win32GetWallClock();
			LARGE_INTEGER FlipWallClock = Win32GetWallClock();
			// NOTE(grigory): We dont want to depend on CPU's clock rate, frame timing based on QPC
			uint64 LastCycleCount = __rdtsc();

			bool SoundIsValid = false;
			DWORD AudioLatencyBytes = 0;
			float AudioLatencySeconds = 0;

			win32_game_code Game = Win32LoadGameCode(SourceGameCodeDLLFullPath,
													 TempGameCodeDLLFullPath);

			while (Running)
			{
				FILETIME NewDLLWriteTime = Win32GetLastWriteTime(SourceGameCodeDLLFullPath);
				if (CompareFileTime(&NewDLLWriteTime, &Game.DLLLastWriteTime) != 0)
				{
					Win32UnloadGameCode(&Game);
					Game = Win32LoadGameCode(SourceGameCodeDLLFullPath, 
											 TempGameCodeDLLFullPath);
				}

				NewInput->dtForFrame = TargetSecondsPerFrame;
				// TODO(casey): zeroing macro
				// TODO(casey): we can't zero everything cause the up/down state
				// will be wrong!!!
				game_controller_input *OldKeyboardController = GetController(OldInput, 0);
				game_controller_input *NewKeyboardController = GetController(NewInput, 0);
				*NewKeyboardController = {};
				NewKeyboardController->IsConnected = true;
				for (int ButtonIndex = 0; ButtonIndex < ArrayCount(NewKeyboardController->Buttons); ++ButtonIndex)
				{
					NewKeyboardController->Buttons[ButtonIndex].EndedDown = 
						OldKeyboardController->Buttons[ButtonIndex].EndedDown;
				}

				// NOTE(grigory): pulled some message processing out from win
				// event callback, kinda using to "functional" style
				// odin and elixir creators talk is still too far to comprehend
				Win32ProcessPendingMessages(&Win32State, NewKeyboardController);

				if (GlobalPause)
					continue;

				POINT MouseP;
				GetCursorPos(&MouseP);
				ScreenToClient(Window, &MouseP);
				NewInput->MouseX = MouseP.x;
				NewInput->MouseY = MouseP.y;
				NewInput->MouseZ = 0; // TODO(casey): support mousewheel?
				Win32ProcessKeyboardEvent(&NewInput->MouseButtons[0],
										  GetKeyState(VK_LBUTTON) & (1 << 15));
				Win32ProcessKeyboardEvent(&NewInput->MouseButtons[1],
										  GetKeyState(VK_MBUTTON) & (1 << 15));
				Win32ProcessKeyboardEvent(&NewInput->MouseButtons[2],
										  GetKeyState(VK_RBUTTON) & (1 << 15));
				Win32ProcessKeyboardEvent(&NewInput->MouseButtons[3],
										  GetKeyState(VK_XBUTTON1) & (1 << 15));
				Win32ProcessKeyboardEvent(&NewInput->MouseButtons[4],
										  GetKeyState(VK_XBUTTON2) & (1 << 15));

				// TODO(casey): Need to poll disconnected controllers to avoid
				// xinput frame rate hit on older libraries...
				// TODO(casey): Should we poll this more frequently
				DWORD MaxControllerCount = XUSER_MAX_COUNT;
			   	if (MaxControllerCount > ArrayCount(NewInput->Controllers) - 1)
				{
					MaxControllerCount = ArrayCount(NewInput->Controllers) - 1;
				}
				for (DWORD ControllerIndex=0; 
					 ControllerIndex < MaxControllerCount; 
					 ControllerIndex++)
				{
					int OurControllerIndex = ControllerIndex + 1;
					game_controller_input *OldController = GetController(OldInput, OurControllerIndex);
					game_controller_input *NewController = GetController(NewInput, OurControllerIndex);

					XINPUT_STATE ControllerState;
					if (XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS)
					{
						// NOTE: plugged in
						// TODO: See if ControllerState.dwPacketNumber increments too rapidly
						NewController->IsConnected = true;
						NewController->IsAnalog = OldController->IsAnalog;

						XINPUT_GAMEPAD *Gamepad = &ControllerState.Gamepad;

						// TODO(casey): This is a square deadzone, check XInput to
						// verify that the deadzone is "round" and show how to do
						// round deadzone
						NewController->StickAverageX = Win32ProcessXInputStickValue(Gamepad->sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
						NewController->StickAverageY = Win32ProcessXInputStickValue(Gamepad->sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);

						if (NewController->StickAverageY != 0 ||
							NewController->StickAverageX != 0)
						{
							NewController->IsAnalog = true;
						}

						// NOTE: DPad
						if (Gamepad->wButtons & XINPUT_GAMEPAD_DPAD_UP)
						{
							NewController->StickAverageY = 1.0f;
							NewController->IsAnalog = false;
						}
						if (Gamepad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN)
						{
							NewController->StickAverageY = -1.0f;
							NewController->IsAnalog = false;
						}
						if (Gamepad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT)
						{
							NewController->StickAverageX = 1.0f;
							NewController->IsAnalog = false;
						}
						if (Gamepad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT)
						{
							NewController->StickAverageX = -1.0f;
							NewController->IsAnalog = false;
						}

						// TODO(grigory): dont need printf for those anymore
						// replicate same in RADDbg
						
						//char buf[64] = {};
						//wsprintf(buf,"X->%d.%d\nY->%d.%d\n", 
						//		 (int32)X, (int32)(X * 1000) - (int32)X,
						//		 (int32)Y, (int32)(Y * 1000) - (int32)Y
						//		 );
						//OutputDebugStringA(buf);

						float Threshold = 0.5f;
						Win32ProcessXInputDigitalButton(
							(NewController->StickAverageY < -Threshold) ? 1 : 0,
							&OldController->MoveDown, 1,
							&NewController->MoveDown);
						Win32ProcessXInputDigitalButton(
							(NewController->StickAverageY > Threshold) ? 1 : 0,
							&OldController->MoveUp, 1,
							&NewController->MoveUp);
						Win32ProcessXInputDigitalButton(
							(NewController->StickAverageX < -Threshold) ? 1 : 0,
							&OldController->MoveLeft, 1,
							&NewController->MoveLeft);
						Win32ProcessXInputDigitalButton(
							(NewController->StickAverageX > Threshold) ? 1 : 0,
							&OldController->MoveRight, 1,
							&NewController->MoveRight);

						Win32ProcessXInputDigitalButton(Gamepad->wButtons, 
							&OldController->ActionDown, XINPUT_GAMEPAD_A, 
							&NewController->ActionDown);

						Win32ProcessXInputDigitalButton(Gamepad->wButtons, 
							&OldController->ActionRight, XINPUT_GAMEPAD_B, 
							&NewController->ActionRight);

						Win32ProcessXInputDigitalButton(Gamepad->wButtons, 
							&OldController->ActionLeft, XINPUT_GAMEPAD_X, 
							&NewController->ActionLeft);

						Win32ProcessXInputDigitalButton(Gamepad->wButtons, 
							&OldController->ActionUp, XINPUT_GAMEPAD_Y, 
							&NewController->ActionUp);

						Win32ProcessXInputDigitalButton(Gamepad->wButtons, 
							&OldController->LeftShoulder, XINPUT_GAMEPAD_LEFT_SHOULDER, 
							&NewController->LeftShoulder);

						Win32ProcessXInputDigitalButton(Gamepad->wButtons, 
							&OldController->RightShoulder, XINPUT_GAMEPAD_RIGHT_SHOULDER, 
							&NewController->RightShoulder);

						Win32ProcessXInputDigitalButton(Gamepad->wButtons, 
							&OldController->Start, XINPUT_GAMEPAD_START, 
							&NewController->Start);

						Win32ProcessXInputDigitalButton(Gamepad->wButtons, 
							&OldController->Back, XINPUT_GAMEPAD_BACK, 
							&NewController->Back);
					}
					else
					{
						//NOTE: controller unplugged
						NewController->IsConnected = false;
					}
				}

				thread_context Thread = {};

				game_offscreen_buffer Buffer = {};
				Buffer.Memory = GlobalBackbuffer.Memory;
				Buffer.Width = GlobalBackbuffer.Width;
				Buffer.Height = GlobalBackbuffer.Height;
				Buffer.Pitch = GlobalBackbuffer.Pitch;
				Buffer.BytesPerPixel = GlobalBackbuffer.BytesPerPixel;

				// NOTE(grigory): Clarify some problems of recording memory snapshot with 
				// regard to: 
				//
				// - Using main .exe fixed base address.
				//
				// *I guess game state with pointers to internal data becomes irrelevant
				// on executable restart, if there is no preservance of base address or
				// shifts from it. But it really becomes a problem only after executable
				// restart, so you cant "store" replays in a long run. Its not an issue
				// when comes to current session input replay
				//
				// - Using game code .dll fixed base address to avoid function pointers
				// corruption.
				//
				// *After reloading game code it maps process to unspecified address
				// which requires function pointers update. In current architecture
				// we avoid any allocations and memory dependencies from game code itself,
				// only main .exe manages memory and provides function pointers
				if (Win32State.InputRecordingIndex)
				{
					Win32RecordInput(&Win32State, NewInput);
				}
				if (Win32State.InputPlayingIndex)
				{
					Win32PlaybackInput(&Win32State, NewInput);
				}

				if (Game.UpdateAndRender)
				{
					Game.UpdateAndRender(&Thread,
										 &GameMemory,
										 NewInput,
										 &Buffer);
				}

				LARGE_INTEGER AudioWallClock = Win32GetWallClock();
				float FlipToAudioSeconds = Win32GetSecondsElapsed(FlipWallClock, AudioWallClock);

				DWORD TargetCursor = 0;
				DWORD BytesToWrite = 0;
				DWORD ByteToLock = 0;

				DWORD PlayCursor = 0;
				DWORD WriteCursor = 0;
				if (SecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor) == DS_OK)
				{
					//
					// NOTE(casey):
					// Here is how sound output computation works
					//
					// We define a safety value that is the number of samples 
					// we think our game update loop may vary by 
					// (let's say up to 2ms)
					//
					// When we wake up to write audio, we will look and see
					// what the play cursor position is and we will forecast
					// ahead where we think the play cursor will be on the
					// next frame boundary.
					//
					// We will then look to see if the write cursor is before that
					// by at least out safety value. 
					// If it is, the targer fill position is that frame boundary
					// plus one frame.
					// This gives us perfect audio sync in the case of a 
					// card that has low enough latency.
					//
					// If the write cursor is _after_ that safety margin,
					// then we assume we can never sync the audio
					// perfectly, so we will write one frame's worth
					// of audio plus the safety margin's worth of guard samples
					// (1ms, or something determined to be safe, whatever
					// we think the variability of our frame computation is).
					// 
					if (!SoundIsValid)
					{
						SoundOutput.RunningSampleIndex = WriteCursor / SoundOutput.BytesPerSample;
						SoundIsValid = true;
					}

					ByteToLock = (SoundOutput.RunningSampleIndex*SoundOutput.BytesPerSample) 
						% SoundOutput.BufferSize;

					DWORD ExpectedSoundBytesPerFrame = 
						(int)(SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample
						/ GameUpdateHz);
					float SecondsLeftUntilFlip = 
						TargetSecondsPerFrame - FlipToAudioSeconds;
					DWORD ExpectedSoundBytesUntilFlip = (DWORD)(ExpectedSoundBytesPerFrame *
						(SecondsLeftUntilFlip / TargetSecondsPerFrame));
					DWORD ExpectedFrameBoundaryByte = PlayCursor + ExpectedSoundBytesUntilFlip;

					DWORD SafeWriteCursor = WriteCursor;
					// NOTE(grigory): unwrapping cursor
					if (SafeWriteCursor < PlayCursor)
					{
						SafeWriteCursor += SoundOutput.BufferSize;	
					}
					Assert(SafeWriteCursor >= PlayCursor);
					SafeWriteCursor += SoundOutput.SafetyBytes;
					
					bool AudioCardIsLowLatency = SafeWriteCursor <= ExpectedFrameBoundaryByte;

					TargetCursor = 0;
					if (AudioCardIsLowLatency)
					{
						TargetCursor = 
							(ExpectedFrameBoundaryByte + ExpectedSoundBytesPerFrame);
					}
					else
					{
						TargetCursor = 
							(WriteCursor + ExpectedSoundBytesPerFrame + SoundOutput.SafetyBytes);
					}
					TargetCursor = TargetCursor % SoundOutput.BufferSize;

					if (ByteToLock > TargetCursor)
					{
						BytesToWrite = SoundOutput.BufferSize - ByteToLock + TargetCursor; 
					}
					else
					{
						BytesToWrite = TargetCursor - ByteToLock;
					}

					game_output_sound_buffer SoundBuffer = {};
					SoundBuffer.SamplesPerSecond = SoundOutput.SamplesPerSecond;
					SoundBuffer.SampleCount = BytesToWrite / SoundOutput.BytesPerSample;
					SoundBuffer.Samples = Samples;

					if (Game.GetSoundSamples)
					{
						Game.GetSoundSamples(&Thread, &GameMemory, &SoundBuffer);
					}

					Win32FillSoundBuffer(&SoundOutput, ByteToLock, BytesToWrite, &SoundBuffer);

#if HANDMADE_INTERNAL
					win32_debug_time_marker *Marker = &DebugTimeMarkers[DebugTimeMarkerIndex];
					Marker->OutputPlayCursor  = PlayCursor;
					Marker->OutputWriteCursor = WriteCursor;
					Marker->OutputLocation = ByteToLock;
					Marker->OutputByteCount = BytesToWrite;
					Marker->ExpectedFlipPlayCursor = ExpectedFrameBoundaryByte;

					DWORD UnwrappedWriteCursor = WriteCursor;
					if (UnwrappedWriteCursor < PlayCursor)
					{
						UnwrappedWriteCursor += SoundOutput.BufferSize;
					}
					AudioLatencyBytes = UnwrappedWriteCursor - PlayCursor;
					AudioLatencySeconds =
						(((float)AudioLatencyBytes / (float)SoundOutput.BytesPerSample) / 
						 (float)SoundOutput.SamplesPerSecond);

					char txt[256] = {};
					wsprintf(txt,
							 "BTL:%u TC:%u BTW:%u - PC:%u WC:%u DELTA:%u (%ums)\n",
							 ByteToLock, TargetCursor, BytesToWrite,
							 PlayCursor, WriteCursor, AudioLatencyBytes,
							 (uint32)(AudioLatencySeconds * 1000));
					OutputDebugStringA(txt);
#endif
				}
				else
				{
					SoundIsValid = false;
				}

				// NOTE(grigory): basically, we have one stable counter that on each step
				// substracts its previous value (as opposed to two counters at loop edges), so 
				// that it captures all program flow time and cant miss thread switch between
				// loop jumps or smth

				LARGE_INTEGER WorkCounter = Win32GetWallClock();
				float WorkSecondsElapsed = Win32GetSecondsElapsed(LastCounter, WorkCounter);

				// TODO(casey): NOT	TESTED YET! PROBABLY BUGGY!!!
				float SecondsElapsedForFrame = WorkSecondsElapsed;
				if (SecondsElapsedForFrame < TargetSecondsPerFrame)
				{
					DWORD SleepMS = (DWORD)1337;
					if (SleepIsGranular)
					{
						SleepMS = (DWORD)(1000.0f * (TargetSecondsPerFrame - SecondsElapsedForFrame));
						if (SleepMS)
							Sleep(SleepMS);
					}

					float SecondsElapsedAfterSleep = Win32GetSecondsElapsed(LastCounter, Win32GetWallClock());
					if (SecondsElapsedAfterSleep < TargetSecondsPerFrame)
					{
						// TODO(casey): LOG MISSED SLEEP HERE
					}

					while (SecondsElapsedForFrame < TargetSecondsPerFrame)
					{
						SecondsElapsedForFrame = 
							Win32GetSecondsElapsed(LastCounter, Win32GetWallClock());
					}
				}
				else
				{
					// TODO(casey): MISSED FRAME RATE!
					// TODO(casey): Logging
				}

				LARGE_INTEGER EndCounter = Win32GetWallClock();
				int64 CounterElapsed = EndCounter.QuadPart - LastCounter.QuadPart;
				LastCounter = EndCounter;


#if HANDMADE_INTERNAL
				// NOTE(casey): Wrong on zero'th index
				//Win32DebugSyncDisplay(&GlobalBackbuffer, ArrayCount(DebugTimeMarkers), 
				//					  DebugTimeMarkers, DebugTimeMarkerIndex - 1, &SoundOutput, TargetSecondsPerFrame);
#endif

				// NOTE(grigory): redundant?
				// NOTE(grigory): Casey returns it in day023 in order to specify
				// LAYERED window option
				HDC DeviceContext = GetDC(Window);
				window_size WindowSize = GetWindowSize(Window);
				Win32DisplayBufferInWindow(DeviceContext, WindowSize.Width, 
										   WindowSize.Height, &GlobalBackbuffer);
				// TODO(grigory): redundant? dont present in casey's code at day010
				// possibly have to do smth with Chris Hecker DC simplification
				ReleaseDC(Window, DeviceContext);

				FlipWallClock = Win32GetWallClock();

#if HANDMADE_INTERNAL
				// NOTE(casey): This is debug code
				{
					//DWORD PlayCursor = 0;
					//DWORD WriteCursor = 0; 
					if ((SecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor)) == DS_OK)
					{
						Assert(DebugTimeMarkerIndex < ArrayCount(DebugTimeMarkers));
						win32_debug_time_marker *Marker = &DebugTimeMarkers[DebugTimeMarkerIndex];
						Marker->FlipPlayCursor  = PlayCursor;
						Marker->FlipWriteCursor = WriteCursor;
					}
				}
#endif

				game_input *Temp = NewInput;
				NewInput = OldInput;
				OldInput = Temp;
				// TODO(casey): Should i clear these here?
				
				uint64 EndCycleCount = __rdtsc();
				uint64 CyclesElapsed = EndCycleCount - LastCycleCount;
				LastCycleCount = EndCycleCount;

//#if 0
				int32 MSPerFrame = (int32)(1000.f*CounterElapsed / GlobalPerfCountFrequency);
				int32 FPS = (uint32)((float)GlobalPerfCountFrequency / CounterElapsed);
				int32 MCPF /*MegaCycles per frame*/ = (int32)(CyclesElapsed / (1000 * 1000));

				char buf[256];
				wsprintf(buf, "%4dms, %4dFPS, %dmc/f\n", MSPerFrame, FPS, MCPF);
				OutputDebugStringA(buf);
//#endif


#if HANDMADE_INTERNAL
				++DebugTimeMarkerIndex;
				DebugTimeMarkerIndex = DebugTimeMarkerIndex 
					% ArrayCount(DebugTimeMarkers);
#endif
			}
		}
		else
		{
			// TODO(casey): Logging
		}
	}
	else
	{
	}

	//MessageBox(0, "This is Handmade Hero.","Handmade Hero", MB_OK|MB_ICONINFORMATION);	
    return 0;
}
