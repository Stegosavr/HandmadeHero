#include "handmade.h"

internal void
GameOutputSound(game_output_sound_buffer *SoundBuffer, int ToneHz)
{
	local_persist float tSine;
	int16 ToneVolume = 3000;
	int WavePeriod = SoundBuffer->SamplesPerSecond / ToneHz;

	int16 *SampleOut = SoundBuffer->Samples;

	for (int SampleIndex = 0; SampleIndex < SoundBuffer->SampleCount; ++SampleIndex)
	{
		float SineValue = sinf(tSine);
		int16 SampleValue = (int16)(SineValue * ToneVolume);
		*SampleOut++ = SampleValue;
		*SampleOut++ = SampleValue;

		tSine += 2.0f * Pi32  / WavePeriod;

		if (tSine > Pi32 * 2)
			tSine -= Pi32 * 2;
	}
}

internal void 
RenderWeirdGradient(game_offscreen_buffer *Buffer, int XOffset, int YOffset)
{
	uint8 *Row = (uint8 *)Buffer->Memory;
	local_persist uint8 col = 1;
	for (int Y=0; Y<Buffer->Height; Y++)
	{
		uint32 *Pixel = (uint32 *)Row;
		for (int X=0; X<Buffer->Width; X++)
		{
			uint8 B = (uint8)(X + XOffset);
			uint8 G = (uint8)(Y + YOffset);
			//uint8 R = (uint8)(X * 2 + Y * 2 + XOffset * 2 + YOffset * 2);
			uint8 R = 0;
			*Pixel++ = B | (G << 8) | (R << 16);	
			/*
			*Pixel = 0xf0ff00ff;
			++Pixel;
			*/
			col++;
		}
		Row += Buffer->Pitch;
	}
}

internal void 
GameUpdateAndRender(
					game_memory *Memory,
					game_input* Input,
					game_offscreen_buffer *Buffer
					)
{
	Assert(sizeof(game_state) <= Memory->PermanentStorageSize);
	Assert(&Input->Controllers[0].Terminator - &Input->Controllers[0].Buttons[0] ==
		   ArrayCount(Input->Controllers[0].Buttons));


	game_state *GameState = (game_state *)Memory->PermanentStorage;
	if (!Memory->IsInitialized)
	{
		char *Filename = __FILE__;
#if 0
		debug_read_file_result File = DEBUGPlatformReadEntireFile(Filename);
		if (File.Contents)
		{
			DEBUGPlatformWriteEntireFile("test.out", File.ContentsSize, File.Contents);
			DEBUGPlatformFreeFileMemory(File.Contents);
		}
#endif
		GameState->ToneHz = 256;

		// TODO(casey): This may be more appropriate to do in platform layer
		Memory->IsInitialized = true;
	}

	for (int ControllerIndex = 0; ControllerIndex < ArrayCount(Input->Controllers);
		 ++ControllerIndex)
	{
		game_controller_input *Controller = GetController(Input, ControllerIndex);
		if (Controller->IsAnalog)
		{
			// NOTE(casey): use analog movement tuning
			GameState->ToneHz = 256 + (int)(128.0f * Controller->StickAverageX);
			GameState->YOffset += (int)(4.0f * Controller->StickAverageY);
		}
		else
		{
			// NOTE(casey): use digital movement tuning
			if (Controller->MoveLeft.EndedDown)
			{
				GameState->XOffset += 1;
			}
			if (Controller->MoveRight.EndedDown)
			{
				GameState->XOffset -= 1;
			}
		}

		if (Controller->ActionDown.EndedDown)
		{
			GameState->YOffset += 1;
		}
	}

	RenderWeirdGradient(Buffer, GameState->XOffset, GameState->YOffset);	
}

internal void
GameGetSoundSamples(game_memory *Memory, game_output_sound_buffer *SoundBuffer)
{
	game_state *GameState = (game_state *)Memory->PermanentStorage;
	GameOutputSound(SoundBuffer, GameState->ToneHz);
}
