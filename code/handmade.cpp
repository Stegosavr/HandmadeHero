#include "handmade.h"

internal void
GameOutputSound(game_output_sound_buffer *SoundBuffer, game_state *GameState)
{
	int16 ToneVolume = 3000;
	int WavePeriod = SoundBuffer->SamplesPerSecond / GameState->ToneHz;

	int16 *SampleOut = SoundBuffer->Samples;

	for (int SampleIndex = 0; SampleIndex < SoundBuffer->SampleCount; ++SampleIndex)
	{
#if 0
		float SineValue = sinf(GameState->tSine);
		int16 SampleValue = (int16)(SineValue * ToneVolume);
#else
		int16 SampleValue = 0;
#endif
		*SampleOut++ = SampleValue;
		*SampleOut++ = SampleValue;

		GameState->tSine += 2.0f * Pi32  / WavePeriod;

		if (GameState->tSine > Pi32 * 2)
			GameState->tSine -= Pi32 * 2;
	}
}

internal void 
RenderWeirdGradient(game_offscreen_buffer *Buffer, int XOffset, int YOffset)
{
	uint8 *Row = (uint8 *)Buffer->Memory;
	for (int Y=0; Y<Buffer->Height; Y++)
	{
		uint32 *Pixel = (uint32 *)Row;
		for (int X=0; X<Buffer->Width; X++)
		{
			uint8 B = (uint8)(X + XOffset);
			uint8 G = (uint8)(Y + YOffset);
			//uint8 R = (uint8)(X * 2 + Y * 2 + XOffset * 2 + YOffset * 2);
			uint8 R = 0;
			*Pixel++ = B | (G << 16) | (R << 16);	
			/*
			*Pixel = 0xf0ff00ff;
			++Pixel;
			*/
		}
		Row += Buffer->Pitch;
	}
}

internal void
RenderPlayer(game_offscreen_buffer *Buffer, int PlayerX, int PlayerY)
{
	uint8 *EndOfBuffer = (uint8 *)Buffer->Memory + Buffer->BytesPerPixel *
		Buffer->Width * Buffer->Height;
	uint32 Color = 0xFFFFFFFF;
	int Top = PlayerY;
	int Bottom = PlayerY + 10;
	for (int X = PlayerX; X < PlayerX+10; ++X)
	{
		uint8 *Pixel = ((uint8 *)Buffer->Memory + 
						X*Buffer->BytesPerPixel + 
						Top*Buffer->Pitch); 
		for (int Y = Top; Y < Bottom; ++Y)
		{
			if ((Pixel >= Buffer->Memory) &&
				(Pixel < EndOfBuffer))
			{
				*(uint32 *)Pixel = Color;
			}
			Pixel += Buffer->Pitch;
		}
	}
}

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
	Assert(sizeof(game_state) <= Memory->PermanentStorageSize);
	Assert(&Input->Controllers[0].Terminator - &Input->Controllers[0].Buttons[0] ==
		   ArrayCount(Input->Controllers[0].Buttons));


	game_state *GameState = (game_state *)Memory->PermanentStorage;
	if (!Memory->IsInitialized)
	{
		char *Filename = __FILE__;
#if 1
		debug_read_file_result File = Memory->DEBUGPlatformReadEntireFile(Filename);
		if (File.Contents)
		{
			Memory->DEBUGPlatformWriteEntireFile("test.out", File.ContentsSize, File.Contents);
			Memory->DEBUGPlatformFreeFileMemory(File.Contents);
		}
#endif
		GameState->ToneHz = 256;
		GameState->tSine = 0.0f;

		GameState->PlayerX = 100;
		GameState->PlayerY = 100;

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

		GameState->PlayerX += (int)(4.0f * Controller->StickAverageX);
		GameState->PlayerY -= (int)(4.0f * Controller->StickAverageY);
		float t = Pi32*2 / 90; 
		if (GameState->tJump > 0)
		{
			GameState->PlayerY += (int)(5*sinf(GameState->tJump));
		}
		if (Controller->ActionDown.EndedDown)
		{
			GameState->tJump = 2 * Pi32;
		}
		GameState->tJump -= t;

	}

	RenderWeirdGradient(Buffer, GameState->XOffset, GameState->YOffset);	
	RenderPlayer(Buffer, GameState->PlayerX, GameState->PlayerY);
}

extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
{
	game_state *GameState = (game_state *)Memory->PermanentStorage;
	GameOutputSound(SoundBuffer, GameState);
}
