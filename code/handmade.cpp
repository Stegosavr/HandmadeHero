#include "handmade.h"

internal void
GameOutputSound(game_output_sound_buffer *SoundBuffer, game_state *GameState)
{
	int16 ToneVolume = 3000;
	//int WavePeriod = SoundBuffer->SamplesPerSecond / GameState->ToneHz;

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

#if 0
		GameState->tSine += 2.0f * Pi32  / WavePeriod;

		if (GameState->tSine > Pi32 * 2)
			GameState->tSine -= Pi32 * 2;
#endif
	}
}

inline uint32 
RoundReal32ToUint32(float Real32)
{
	uint32 Result = (uint32)(Real32 + 0.5f);
	// TODO(casey): Intrinsic????
	return Result;
}

inline int32 
RoundReal32ToInt32(float Real32)
{
	int32 Result = (int32)(Real32 + 0.5f);
	// TODO(casey): Intrinsic????
	return Result;
}

inline int32 
TruncateReal32ToInt32(float Real32)
{
	int32 Result = (int32)(Real32);
	return Result;
}

internal void
DrawRectangle(game_offscreen_buffer *Buffer,
			  float RealMinX, float RealMinY,
			  float RealMaxX, float RealMaxY,
			  float R, float G, float B)
{
	int32 MinX = RoundReal32ToInt32(RealMinX);
	int32 MinY = RoundReal32ToInt32(RealMinY);
	int32 MaxX = RoundReal32ToInt32(RealMaxX);
	int32 MaxY = RoundReal32ToInt32(RealMaxY);
	if (MinX < 0)
	{
		MinX = 0;	
	}
	if (MinY < 0)
	{
		MinY = 0;	
	}
	if (MaxX > Buffer->Width)
	{
		MaxX = Buffer->Width;	
	}
	if (MaxY > Buffer->Height)
	{
		MaxY = Buffer->Height;	
	}

	uint8 *EndOfBuffer = (uint8 *)Buffer->Memory + Buffer->BytesPerPixel *
		Buffer->Width * Buffer->Height;

	// BIT PATTERN: 0x AA RR GG BB
	uint32 Color = (RoundReal32ToInt32(R * 255.0f) << 16) |
				   (RoundReal32ToInt32(G * 255.0f) <<  8) |
				   (RoundReal32ToInt32(B * 255.0f) <<  0);

	uint8 *Row = ((uint8 *)Buffer->Memory + 
					MinX*Buffer->BytesPerPixel + 
					MinY*Buffer->Pitch); 
	for (int Y = MinY; Y < MaxY; ++Y)
	{
		uint32 *Pixel = (uint32 *)Row;
		for (int X = MinX; X < MaxX; ++X)
		{
			*Pixel++ = Color;
		}
		Row += Buffer->Pitch;
	}
}

inline uint32
GetTileValueUnchecked(tile_map *TileMap, int32 TileX, int32 TileY)
{
	uint32 TileMapValue = TileMap->Tiles[TileX + TileY * TileMap->CountX];
	return TileMapValue;
}

inline tile_map *
GetTileMap(world *World, int32 TileMapX, int32 TileMapY)
{
	tile_map *TileMap = 0;

	if ((TileMapX < World->CountX) && (TileMapX >= 0) &&
		(TileMapY < World->CountY) && (TileMapY >= 0))
	{
		TileMap = &World->TileMaps[TileMapX + TileMapY * World->CountX];
	}

	return TileMap;
}

internal bool
IsTileMapPointEmpty(tile_map *TileMap, float TestX, float TestY)
{
	bool Empty = false;

	int32 TestTileX = TruncateReal32ToInt32(
		(TestX - TileMap->UpperLeftX) / TileMap->TileWidth);
	int32 TestTileY = TruncateReal32ToInt32(
		(TestY - TileMap->UpperLeftY) / TileMap->TileHeight);

	if ((TestTileX >= 0) && (TestTileX < TileMap->CountX) &&
		(TestTileY >= 0) && (TestTileY < TileMap->CountY))
	{
		uint32 TileMapValue = GetTileValueUnchecked(TileMap, TestTileX, TestTileY);
		Empty = (TileMapValue == 0);
	}

	return Empty;
}

internal bool
IsWorldPointEmpty(world *World, int32 TileMapX, int32 TileMapY, float TestX, float TestY)
{
	bool Empty = false;

	tile_map *TileMap = GetTileMap(World, TileMapX, TileMapY);
	if (TileMap)
	{
		int32 TestTileX = TruncateReal32ToInt32(
			(TestX - TileMap->UpperLeftX) / TileMap->TileWidth);
		int32 TestTileY = TruncateReal32ToInt32(
			(TestY - TileMap->UpperLeftY) / TileMap->TileHeight);

		if ((TestTileX >= 0) && (TestTileX < TileMap->CountX) &&
			(TestTileY >= 0) && (TestTileY < TileMap->CountY))
		{
			uint32 TileMapValue = GetTileValueUnchecked(TileMap, TestTileX, TestTileY);
			Empty = (TileMapValue == 0);
		}
	}

	return Empty;
}

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
	Assert(sizeof(game_state) <= Memory->PermanentStorageSize);
	Assert(&Input->Controllers[0].Terminator - &Input->Controllers[0].Buttons[0] ==
		   ArrayCount(Input->Controllers[0].Buttons));

#define TILE_MAP_COUNT_X 17
#define TILE_MAP_COUNT_Y 9
	uint32 Tiles00[TILE_MAP_COUNT_Y][TILE_MAP_COUNT_X] =
	{
		{ 1, 1, 1, 1,   1, 1, 1, 1,   1,   1, 1, 1, 1,   1, 1, 1, 1, },
		{ 1, 0, 0, 1,   0, 0, 0, 0,   0,   0, 0, 0, 0,   0, 0, 0, 1, },
		{ 1, 0, 0, 1,   0, 0, 0, 0,   0,   0, 0, 0, 0,   0, 0, 0, 1, },
		{ 1, 0, 0, 0,   0, 0, 0, 0,   0,   0, 0, 0, 0,   0, 0, 0, 1, },
		{ 1, 0, 0, 0,   0, 0, 0, 0,   0,   0, 0, 0, 0,   0, 0, 0, 0, },
		{ 1, 0, 0, 0,   0, 0, 0, 0,   0,   0, 0, 0, 0,   0, 0, 0, 1, },
		{ 1, 0, 0, 1,   0, 0, 0, 0,   0,   0, 0, 0, 0,   0, 0, 0, 1, },
		{ 1, 0, 0, 1,   0, 0, 0, 0,   0,   0, 0, 0, 0,   0, 0, 0, 1, },
		{ 1, 1, 1, 1,   1, 1, 1, 1,   0,   1, 1, 1, 1,   1, 1, 1, 1, },
	};
	uint32 Tiles01[TILE_MAP_COUNT_Y][TILE_MAP_COUNT_X] =
	{
		{ 1, 1, 1, 1,   1, 1, 1, 1,   0,   1, 1, 1, 1,   1, 1, 1, 1, },
		{ 1, 0, 0, 0,   0, 0, 0, 0,   0,   0, 0, 0, 0,   0, 0, 0, 1, },
		{ 1, 0, 0, 0,   0, 0, 0, 0,   0,   0, 0, 0, 0,   0, 0, 0, 1, },
		{ 1, 0, 0, 0,   0, 0, 0, 0,   0,   0, 0, 0, 0,   0, 0, 0, 1, },
		{ 1, 0, 0, 0,   0, 0, 0, 0,   1,   0, 0, 0, 0,   0, 0, 0, 0, },
		{ 1, 0, 0, 0,   0, 0, 0, 0,   0,   0, 0, 0, 0,   0, 0, 0, 1, },
		{ 1, 0, 0, 0,   0, 0, 0, 0,   0,   0, 0, 0, 0,   0, 0, 0, 1, },
		{ 1, 0, 0, 0,   0, 0, 0, 0,   0,   0, 0, 0, 0,   0, 0, 0, 1, },
		{ 1, 1, 1, 1,   1, 1, 1, 1,   1,   1, 1, 1, 1,   1, 1, 1, 1, },
	};
	uint32 Tiles10[TILE_MAP_COUNT_Y][TILE_MAP_COUNT_X] =
	{
		{ 1, 1, 1, 1,   1, 1, 1, 1,   1,   1, 1, 1, 1,   1, 1, 1, 1, },
		{ 1, 0, 0, 0,   0, 0, 0, 0,   0,   0, 0, 0, 0,   0, 0, 0, 1, },
		{ 1, 0, 0, 0,   0, 0, 0, 0,   0,   0, 0, 0, 0,   0, 0, 0, 1, },
		{ 1, 0, 0, 0,   0, 0, 0, 0,   0,   0, 0, 0, 0,   0, 0, 0, 1, },
		{ 0, 0, 0, 0,   0, 0, 0, 0,   1,   0, 0, 0, 0,   0, 0, 0, 1, },
		{ 1, 0, 0, 0,   0, 0, 0, 0,   0,   0, 0, 0, 0,   0, 0, 0, 1, },
		{ 1, 0, 0, 0,   0, 0, 0, 0,   0,   0, 0, 0, 0,   0, 0, 0, 1, },
		{ 1, 0, 0, 0,   0, 0, 0, 0,   0,   0, 0, 0, 0,   0, 0, 0, 1, },
		{ 1, 1, 1, 1,   1, 1, 1, 1,   0,   1, 1, 1, 1,   1, 1, 1, 1, },
	};
	uint32 Tiles11[TILE_MAP_COUNT_Y][TILE_MAP_COUNT_X] =
	{
		{ 1, 1, 1, 1,   1, 1, 1, 1,   0,   1, 1, 1, 1,   1, 1, 1, 1, },
		{ 1, 0, 0, 0,   0, 0, 0, 0,   0,   0, 0, 0, 0,   0, 0, 0, 1, },
		{ 1, 0, 0, 0,   0, 0, 0, 0,   0,   0, 0, 0, 0,   0, 0, 0, 1, },
		{ 1, 0, 0, 0,   0, 0, 0, 0,   0,   0, 0, 0, 0,   0, 0, 0, 1, },
		{ 0, 0, 0, 0,   0, 0, 0, 0,   1,   0, 0, 0, 0,   0, 0, 0, 1, },
		{ 1, 0, 0, 0,   0, 0, 0, 0,   0,   0, 0, 0, 0,   0, 0, 0, 1, },
		{ 1, 0, 0, 0,   0, 0, 0, 0,   0,   0, 0, 0, 0,   0, 0, 0, 1, },
		{ 1, 0, 0, 0,   0, 0, 0, 0,   0,   0, 0, 0, 0,   0, 0, 0, 1, },
		{ 1, 1, 1, 1,   1, 1, 1, 1,   1,   1, 1, 1, 1,   1, 1, 1, 1, },
	};

	tile_map TileMaps[2][2];
	TileMaps[0][0].CountX = TILE_MAP_COUNT_X;
	TileMaps[0][0].CountY = TILE_MAP_COUNT_Y;
	TileMaps[0][0].UpperLeftX = -30;
	TileMaps[0][0].UpperLeftY = 0;
	TileMaps[0][0].TileWidth  = 60;
	TileMaps[0][0].TileHeight = 60;
	TileMaps[0][0].Tiles = (uint32 *)Tiles00;

	TileMaps[0][1] = TileMaps[0][0];
	TileMaps[0][1].Tiles = (uint32 *)Tiles01;

	TileMaps[1][0] = TileMaps[0][0];
	TileMaps[1][0].Tiles = (uint32 *)Tiles10;

	TileMaps[1][1] = TileMaps[0][0];
	TileMaps[1][1].Tiles = (uint32 *)Tiles11;

	world World;
	World.CountX = 2;
	World.CountY = 2;
	World.TileMaps = (tile_map *)TileMaps; 

	tile_map *TileMap = &TileMaps[0][0];

	float PlayerWidth  = TileMap->TileWidth*0.75f;
	float PlayerHeight = TileMap->TileHeight;

	game_state *GameState = (game_state *)Memory->PermanentStorage;
	if (!Memory->IsInitialized)
	{
		GameState->PlayerX = 100.0f;
		GameState->PlayerY = 100.0f;
		Memory->IsInitialized = true;
	}


	for (int ControllerIndex = 0; ControllerIndex < ArrayCount(Input->Controllers);
		 ++ControllerIndex)
	{
		game_controller_input *Controller = GetController(Input, ControllerIndex);
		if (Controller->IsAnalog)
		{
			// NOTE(casey): use analog movement tuning
		}
		else
		{
			// NOTE(casey): use digital movement tuning
			float dPlayerX = 0.0f;
			float dPlayerY = 0.0f;

			if (Controller->MoveUp.EndedDown)
			{
				dPlayerY = -1.0f;
			}
			if (Controller->MoveDown.EndedDown)
			{
				dPlayerY = 1.0f;
			}
			if (Controller->MoveLeft.EndedDown)
			{
				dPlayerX = -1.0f;
			}
			if (Controller->MoveRight.EndedDown)
			{
				dPlayerX = 1.0f;
			}
			dPlayerX *= 64.0f;
			dPlayerY *= 64.0f;
			// TODO(casey): Diagonal will be faster! Fix once we have vectors :)
			float NewPlayerX = GameState->PlayerX + Input->dtForFrame*dPlayerX;
			float NewPlayerY = GameState->PlayerY + Input->dtForFrame*dPlayerY;

			if (IsTileMapPointEmpty(TileMap, NewPlayerX, NewPlayerY) &&
				IsTileMapPointEmpty(TileMap, NewPlayerX - 0.5f*PlayerWidth, NewPlayerY) &&
				IsTileMapPointEmpty(TileMap, NewPlayerX + 0.5f*PlayerWidth, NewPlayerY))
			{
				GameState->PlayerX = NewPlayerX;
				GameState->PlayerY = NewPlayerY;		
			}
		}

	}

	DrawRectangle(Buffer, 0.0f, 0.0f, (float)Buffer->Width, (float)Buffer->Height,
				  1.0f, 0.0f, 1.0f);
	
	for (int Row = 0;
		 Row < TileMap->CountY;
		 ++Row)
	{
		for (int Column = 0;
			 Column < TileMap->CountX;
			 ++Column)
		{
			uint32 TileID = GetTileValueUnchecked(TileMap, Column, Row);
			float Gray = 0.5f;
			if (TileID == 1)
			{
				Gray = 1.0f;
			}
			float MinX = TileMap->UpperLeftX + (float)Column * TileMap->TileWidth;
			float MinY = TileMap->UpperLeftY + (float)Row * TileMap->TileHeight;
			float MaxX = MinX + TileMap->TileWidth;
			float MaxY = MinY + TileMap->TileWidth;

			DrawRectangle(Buffer, MinX, MinY, MaxX, MaxY,
						  Gray, Gray, Gray);
		}
	}
	float PlayerR = 1.0f;
	float PlayerG = 1.0f;
	float PlayerB = 0.0f;

	float PlayerLeft = GameState->PlayerX - 0.5f*PlayerWidth;
	float PlayerTop  = GameState->PlayerY - PlayerHeight;
	DrawRectangle(Buffer,
				  PlayerLeft, PlayerTop,
				  PlayerLeft+PlayerWidth, PlayerTop+PlayerHeight,
				  PlayerR, PlayerG, PlayerB);
}

extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
{
	game_state *GameState = (game_state *)Memory->PermanentStorage;
	GameOutputSound(SoundBuffer, GameState);
}

/*
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
			*Pixel++ = B | (G << 8) | (R << 16);	
		}
		Row += Buffer->Pitch;
	}
}
*/
