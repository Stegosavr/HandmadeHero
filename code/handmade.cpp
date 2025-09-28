#include "handmade.h"
#include "handmade_intrinsics.h"

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
GetTileValue(world *World, tile_map *TileMap, int32 TileX, int32 TileY)
{
	Assert(TileMap);
	Assert((TileX >= 0) && (TileX < World->CountX) &&
		   (TileY >= 0) && (TileY < World->CountY));
	uint32 TileMapValue = TileMap->Tiles[TileX + TileY * World->CountX];
	return TileMapValue;
}

inline tile_map *
GetTileMap(world *World, int32 TileMapX, int32 TileMapY)
{
	tile_map *TileMap = 0;

	if ((TileMapX < World->TileMapCountX) && (TileMapX >= 0) &&
		(TileMapY < World->TileMapCountY) && (TileMapY >= 0))
	{
		TileMap = &World->TileMaps[TileMapX + TileMapY * World->TileMapCountX];
	}

	return TileMap;
}

internal bool
IsTileMapPointEmpty(world *World, tile_map *TileMap, int32 TestTileX, int32 TestTileY)
{
	bool Empty = false;

	if (TileMap)
	{
		if ((TestTileX >= 0) && (TestTileX < World->CountX) &&
			(TestTileY >= 0) && (TestTileY < World->CountY))
		{
			uint32 TileMapValue = GetTileValue(World, TileMap, TestTileX, TestTileY);
			Empty = (TileMapValue == 0);
		}
	}

	return Empty;
}


inline canonical_position 
GetCanonicalPosition(world *World, raw_position Pos)
{
	canonical_position Result;

	Result.TileMapX = Pos.TileMapX;
	Result.TileMapY = Pos.TileMapY;

	float X = Pos.X - World->UpperLeftX;
	float Y = Pos.Y - World->UpperLeftY;

	Result.TileX = FloorReal32ToInt32(
		(X) / World->TileSideInPixels);
	Result.TileY = FloorReal32ToInt32(
		(Y) / World->TileSideInPixels);

	Result.TileRelX = X - Result.TileX * World->TileSideInPixels;
	Result.TileRelY = Y - Result.TileY * World->TileSideInPixels;

	Assert(Result.TileRelX >= 0);
	Assert(Result.TileRelY >= 0);
	Assert(Result.TileRelX < World->TileSideInPixels);
	Assert(Result.TileRelX < World->TileSideInPixels);

	if (Result.TileX < 0)
	{
		--Result.TileMapX;
		Result.TileX += World->CountX;
	}
	if (Result.TileY < 0)
	{
		--Result.TileMapY;
		Result.TileY += World->CountY;
	}
	if (Result.TileX >= World->CountX)
	{
		++Result.TileMapX;
		Result.TileX -= World->CountX;
	}
	if (Result.TileY >= World->CountY)
	{
		++Result.TileMapY;
		Result.TileY -= World->CountY;
	}

	return Result;
}

internal bool
IsWorldPointEmpty(world *World, raw_position TestPos)
{
	bool Empty = false;

	canonical_position CanPos = GetCanonicalPosition(World, TestPos);
	tile_map *TileMap = GetTileMap(World, CanPos.TileMapX, CanPos.TileMapY);
	Empty = IsTileMapPointEmpty(World, TileMap, CanPos.TileX, CanPos.TileY);

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
	TileMaps[0][0].Tiles = (uint32 *)Tiles00;
	TileMaps[1][0].Tiles = (uint32 *)Tiles01;
	TileMaps[0][1].Tiles = (uint32 *)Tiles10;
	TileMaps[1][1].Tiles = (uint32 *)Tiles11;

	world World;

	World.CountX = TILE_MAP_COUNT_X;
	World.CountY = TILE_MAP_COUNT_Y;

	// TODO(casey): begin using tile side in meters
	World.TileSideInMeters = 1.4f;
	World.TileSideInPixels = 60;

	World.UpperLeftX = -(float)World.TileSideInPixels / 2;
	World.UpperLeftY = 0;

	World.TileMapCountX = 2;
	World.TileMapCountY = 2;
	World.TileMaps = (tile_map *)TileMaps; 

	float PlayerWidth  = (float)World.TileSideInPixels*0.75f;
	float PlayerHeight = (float)World.TileSideInPixels;

	game_state *GameState = (game_state *)Memory->PermanentStorage;
	if (!Memory->IsInitialized)
	{
		GameState->PlayerX = 100.0f;
		GameState->PlayerY = 100.0f;
		Memory->IsInitialized = true;
	}

	tile_map *TileMap = GetTileMap(&World, GameState->PlayerTileMapX, GameState->PlayerTileMapY);
	Assert(TileMap);

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

			raw_position PlayerPos = 
				{GameState->PlayerTileMapX, GameState->PlayerTileMapY,
				 NewPlayerX, NewPlayerY};
			raw_position PlayerLeft  = PlayerPos;
			PlayerLeft.X -= 0.5f*PlayerWidth;
			raw_position PlayerRight = PlayerPos;
			PlayerRight.X += 0.5f*PlayerWidth;

			if (IsWorldPointEmpty(&World, PlayerPos) &&
				IsWorldPointEmpty(&World, PlayerLeft) &&
				IsWorldPointEmpty(&World, PlayerRight))
			{
				canonical_position CanPos = GetCanonicalPosition(&World, PlayerPos);

				GameState->PlayerTileMapX = CanPos.TileMapX;
				GameState->PlayerTileMapY = CanPos.TileMapY;
				GameState->PlayerX = World.UpperLeftX + CanPos.TileRelX + World.TileSideInPixels  * CanPos.TileX;
				GameState->PlayerY = World.UpperLeftY + CanPos.TileRelY + World.TileSideInPixels * CanPos.TileY;
			}
		}

	}

	DrawRectangle(Buffer, 0.0f, 0.0f, (float)Buffer->Width, (float)Buffer->Height,
				  1.0f, 0.0f, 1.0f);
	
	for (int Row = 0;
		 Row < World.CountY;
		 ++Row)
	{
		for (int Column = 0;
			 Column < World.CountX;
			 ++Column)
		{
			uint32 TileID = GetTileValue(&World, TileMap, Column, Row);
			float Gray = 0.5f;
			if (TileID == 1)
			{
				Gray = 1.0f;
			}
			float MinX = World.UpperLeftX + (float)Column * World.TileSideInPixels;
			float MinY = World.UpperLeftY + (float)Row * World.TileSideInPixels;
			float MaxX = MinX + World.TileSideInPixels;
			float MaxY = MinY + World.TileSideInPixels;

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
