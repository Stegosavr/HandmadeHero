#include "handmade.h"
#include "handmade_random.h"

#include "handmade_tile.cpp"

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

#pragma pack(push, 1)
struct bitmap_header
{
	uint16 FileType;
	uint32 FileSize;
	uint16 Reserved1;
	uint16 Reserved2;
	uint32 BitmapOffset;

	uint32 Size;
	int32 Width;
	int32 Height;
	uint16 Planes;
	uint16 BitsPerPixel;
	uint32 Compression;
	uint32 SizeOfBitmap;
	int32 HorzResolution;
	int32 VertResolution;
	uint32 ColorsUsed;
	uint32 ColorsImportant;

	uint32 RedMask;
	uint32 GreenMask;
	uint32 BlueMask;
};
#pragma pack(pop)


internal loaded_bitmap 
DEBUGLoadBMP(thread_context *Thread, debug_platform_read_entire_file *ReadEntireFile, char *FileName)
{
	loaded_bitmap Result = {};

	debug_read_file_result ReadResult = ReadEntireFile(Thread, FileName);
	if (ReadResult.ContentsSize != 0)
	{
		bitmap_header *Header = (bitmap_header *)ReadResult.Contents;
		uint32 *Pixels = (uint32 *)((uint8 *)ReadResult.Contents + Header->BitmapOffset);
		Result.Pixels = Pixels;
		Result.Width = Header->Width;
		Result.Height = Header->Height;

		Assert(Header->Compression == 3);

		// NOTE(casey): If you are using this generically for some reason,
		// please remember that BMP files can go in either direction and
		// the height will be negative for top-down
		// (Also, there can be compression, etc., etc... DONT think this
		// is complete BMP loading code because it isnt!!)
		
		// NOTE(casey): Byte order in memory is determined by the header itself,
		// so we have to read out the masks and convert pixels ourselves.
		uint32 RedMask = Header->RedMask;
		uint32 GreenMask = Header->GreenMask;
		uint32 BlueMask = Header->BlueMask;
		uint32 AlphaMask = ~(RedMask | GreenMask | BlueMask);

		bit_scan_result RedShift   = FindLeastSignificantBit(RedMask);
		bit_scan_result GreenShift = FindLeastSignificantBit(GreenMask);
		bit_scan_result BlueShift  = FindLeastSignificantBit(BlueMask);
		bit_scan_result AlphaShift = FindLeastSignificantBit(AlphaMask);

		Assert(RedShift.Found && GreenShift.Found && BlueShift.Found && AlphaShift.Found);

		uint32 *Data = Pixels;
		for (int32 Y = 0;
			 Y < Header->Height;
			 ++Y)
		{
			for (int32 X = 0;
				 X < Header->Width;
				 ++X)
			{
				uint32 C = *Data;
				*Data++ = ((((C >> AlphaShift.Index) & 0xFF) << 24) |
						   (((C >> RedShift.Index)   & 0xFF) << 16) |
						   (((C >> GreenShift.Index) & 0xFF) <<  8) |
						   (((C >> BlueShift.Index)  & 0xFF) <<  0));
			}
		}
	}

	return Result;
}

internal void
DrawBitmap(game_offscreen_buffer *Buffer, 
		   loaded_bitmap *Bitmap, float RealX, float RealY)
{
	int32 MinX = RoundReal32ToInt32(RealX);
	int32 MinY = RoundReal32ToInt32(RealY);
	int32 MaxX = RoundReal32ToInt32(RealX + (float)Bitmap->Width);
	int32 MaxY = RoundReal32ToInt32(RealY + (float)Bitmap->Height);

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

	// TODO(casey): SourceRow needs to be changed based on clipping
	uint32 *SourceRow = Bitmap->Pixels + Bitmap->Width*(Bitmap->Height - 1);
	uint8 *DestRow = ((uint8 *)Buffer->Memory + 
					MinX*Buffer->BytesPerPixel + 
					MinY*Buffer->Pitch); 

	for (int32 Y = MinY;
		 Y < MaxY;
		 ++Y)
	{
	
		uint32 *Source = SourceRow;
		uint32 *Dest = (uint32 *)DestRow;
		for (int32 X = MinX;
			 X < MaxX;
			 ++X)
		{
			float A =  (float)((*Source >> 24) & 0xFF) / 255.0f;
			float SR = (float)((*Source >> 16) & 0xFF);
			float SG = (float)((*Source >>  8) & 0xFF);
			float SB = (float)((*Source >>  0) & 0xFF);

			float DR = (float)((*Dest >> 16) & 0xFF);
			float DG = (float)((*Dest >>  8) & 0xFF);
			float DB = (float)((*Dest >>  0) & 0xFF);

			// TODO(casey): Someday, we need to takl about premultiplied alpha!
			// (this is not premultiplied alpha)
			float R = (1.0f-A)*DR + A*SR;
			float G = (1.0f-A)*DG + A*SG;
			float B = (1.0f-A)*DB + A*SB;

			*Dest = (((uint32)(R+0.5f) << 16) | 
					 ((uint32)(G+0.5f) <<  8) | 
					 ((uint32)(B+0.5f) <<  0));

			++Dest;
			++Source;
		}
		DestRow += Buffer->Pitch;
		SourceRow -= Bitmap->Width;
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
		GameState->Backdrop = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_background.bmp");
		GameState->HeroHead = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_front_head.bmp");
		GameState->HeroCape = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_front_cape.bmp");
		GameState->HeroTorso = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_front_torso.bmp");

		GameState->PlayerP.AbsTileX = 2;
		GameState->PlayerP.AbsTileY = 3;
		GameState->PlayerP.OffsetX = 0.0f;
		GameState->PlayerP.OffsetY = 0.0f;

		InitializeArena(&GameState->WorldArena, 
						Memory->PermanentStorageSize - sizeof(game_state),
						(uint8 *)Memory->PermanentStorage + sizeof(game_state));

		GameState->World = PushStruct(&GameState->WorldArena, world);
		world *World = GameState->World;
		World->TileMap = PushStruct(&GameState->WorldArena, tile_map);
		tile_map *TileMap = World->TileMap;

		TileMap->ChunkShift = 4;
		TileMap->ChunkMask = (1 << TileMap->ChunkShift) - 1;
		TileMap->ChunkDim = 1 << TileMap->ChunkShift;

		TileMap->TileChunkCountX = 64;
		TileMap->TileChunkCountY = 64;
		TileMap->TileChunkCountZ = 2;

		TileMap->TileChunks = PushArray(&GameState->WorldArena,
										TileMap->TileChunkCountX*
										TileMap->TileChunkCountY*
										TileMap->TileChunkCountY, 
										tile_chunk);


		TileMap->TileSideInMeters = 1.4f;

		// map generation
		
		uint32 RandomNumberIndex = 0;
		uint32 TilesPerWidth = 17;
		uint32 TilesPerHeight = 9;

		uint32 ScreenX = 0;
		uint32 ScreenY = 0;

		bool DoorLeft = false;
		bool DoorTop = false;
		bool DoorBottom = false;
		bool DoorRight = false;

		bool DoorUp = false;
		bool DoorDown = false;
		uint32 AbsTileZ = 0;

		for (uint32 ScreenIndex = 0;
			 ScreenIndex < 100;
			 ++ScreenIndex)
		{
			// TODO(casey): Random number generator!
			Assert(RandomNumberIndex < ArrayCount(RandomNumberTable));
			uint32 RandomChoice;
			if (DoorUp || DoorDown)
			{
				RandomChoice = RandomNumberTable[RandomNumberIndex++] % 2;
			}
			else
			{
				RandomChoice = RandomNumberTable[RandomNumberIndex++] % 3;
			}

			bool CreatedZDoor = false;
			if (RandomChoice == 2)
			{
				CreatedZDoor = true;
				if (AbsTileZ == 0)
				{
					DoorUp = true;
				}
				else
				{
					DoorDown = true;
				}
			}
			else if (RandomChoice == 1)
			{
				DoorRight = true;
			}
			else
			{
				DoorTop = true;
			}

			for (uint32 TileY = 0;
				 TileY < TilesPerHeight;
				 ++TileY)
			{
				for (uint32 TileX = 0;
					 TileX < TilesPerWidth;
					 ++TileX)
				{
					uint32 AbsTileX = ScreenX * TilesPerWidth + TileX;
					uint32 AbsTileY = ScreenY * TilesPerHeight + TileY;

					uint32 TileValue = 1;
					if ((TileX == 0) && (!DoorLeft || (TileY != TilesPerHeight / 2)) )
					{
						TileValue = 2;
					}
					if ((TileX == TilesPerWidth - 1) && (!DoorRight || (TileY != TilesPerHeight / 2)))
					{
						TileValue = 2;
					}
					if ((TileY == 0) && (!DoorBottom || (TileX != TilesPerWidth / 2)))
					{
						TileValue = 2;
					}
					if ((TileY == TilesPerHeight - 1) && (!DoorTop || (TileX != TilesPerWidth / 2)))
					{
						TileValue = 2;
					}

					if ((TileX == 8) & (TileY == 4))
					{
						if (DoorUp)
						{
							TileValue = 3;
						}
						if (DoorDown)
						{
							TileValue = 4;
						}
					}

					SetTileValue(&GameState->WorldArena, World->TileMap, AbsTileX, AbsTileY, AbsTileZ, TileValue);
				}
			}

			DoorLeft = DoorRight;
			DoorBottom = DoorTop;
			DoorRight = false;
			DoorTop = false;

			//DoorUp = DoorDown;
			//DoorDown = DoorUp;
			if (CreatedZDoor)
			{
				DoorUp = !DoorUp;
				DoorDown = !DoorDown;
			}
			else
			{
				DoorUp = false;
				DoorDown = false;
			}

			if (RandomChoice == 2)
			{
				AbsTileZ = 1 - AbsTileZ;
			}
			else if (RandomChoice == 1)
			{
				ScreenX += 1;
			}
			else
			{
				ScreenY += 1;
			}
		}

		Memory->IsInitialized = true;
	}

	world *World = GameState->World;
	tile_map *TileMap = World->TileMap;

	int32 TileSideInPixels = 60;
	float PixelsPerMeter = (float)TileSideInPixels / TileMap->TileSideInMeters;

	float PlayerHeight = 1.4f;
	float PlayerWidth  = PlayerHeight * 0.75f;
	float LowerLeftX = -(float)TileSideInPixels / 2;
	float LowerLeftY = 0;

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
				dPlayerY = 1.0f;
			}
			if (Controller->MoveDown.EndedDown)
			{
				dPlayerY = -1.0f;
			}
			if (Controller->MoveLeft.EndedDown)
			{
				dPlayerX = -1.0f;
			}
			if (Controller->MoveRight.EndedDown)
			{
				dPlayerX = 1.0f;
			}
			float PlayerSpeed = 3.0f;
			if (Controller->ActionUp.EndedDown)
			{
				PlayerSpeed = 10.0f;
			}
			dPlayerX *= PlayerSpeed;
			dPlayerY *= PlayerSpeed;
			// TODO(casey): Diagonal will be faster! Fix once we have vectors :)
			float NewPlayerX = GameState->PlayerP.OffsetX + Input->dtForFrame*dPlayerX;
			float NewPlayerY = GameState->PlayerP.OffsetY + Input->dtForFrame*dPlayerY;
			tile_map_position NewPlayerP = GameState->PlayerP;
			NewPlayerP.OffsetX = NewPlayerX;
			NewPlayerP.OffsetY = NewPlayerY;

			tile_map_position PlayerLeft = NewPlayerP;
			PlayerLeft.OffsetX -= 0.5f*PlayerWidth;
			tile_map_position PlayerRight = NewPlayerP;
			PlayerRight.OffsetX += 0.5f*PlayerWidth;

			// TODO(casey): Delta function that auto-recanonicalizes
			NewPlayerP = RecanonicalizePosition(TileMap, NewPlayerP);	
			PlayerLeft = RecanonicalizePosition(TileMap, PlayerLeft);	
			PlayerRight = RecanonicalizePosition(TileMap, PlayerRight);	

			if (IsTileMapPointEmpty(TileMap, NewPlayerP) &&
				IsTileMapPointEmpty(TileMap, PlayerLeft) &&
				IsTileMapPointEmpty(TileMap, PlayerRight))
			{
				if (!AreOnSameTile(GameState->PlayerP, NewPlayerP))
				{
					uint32 NewTileValue = GetTileValue(TileMap, NewPlayerP);
					if (NewTileValue == 3)
					{
						NewPlayerP.AbsTileZ += 1;
					}
					else if (NewTileValue == 4)
					{
						NewPlayerP.AbsTileZ -= 1;
					}
				}
				GameState->PlayerP = NewPlayerP;
			}
		}

	}

#if 1
	DrawBitmap(Buffer, &GameState->Backdrop, 0, 0);
#else
	DrawRectangle(Buffer, 0.0f, 0.0f, (float)Buffer->Width, (float)Buffer->Height,
				  1.0f, 0.0f, 1.0f);
#endif

	float ScreenCenterX = Buffer->Width / 2.0f;
	float ScreenCenterY = Buffer->Height / 2.0f;
	
	for (int32 RelRow = -10;
		 RelRow < 10;
		 ++RelRow)
	{
		for (int32 RelColumn = -20;
			 RelColumn < 20;
			 ++RelColumn)
		{
			uint32 Row = GameState->PlayerP.AbsTileY + RelRow;
			uint32 Column = GameState->PlayerP.AbsTileX + RelColumn;

			uint32 TileID = GetTileValue(TileMap, Column, Row, GameState->PlayerP.AbsTileZ);

			if (TileID < 2)
			{
				continue;
			}

			float Gray = 0.5f;
			if (TileID == 2)
			{
				Gray = 1.0f;
			}
			if (TileID > 2)
			{
				Gray = .25f;
			}

			if ((Row == GameState->PlayerP.AbsTileY) && 
				(Column == GameState->PlayerP.AbsTileX))
			{
				Gray = 0.0f;
			}
			/*
			float MinX = LowerLeftX + (float)Column * TileMap->TileSideInPixels;
			float MinY = LowerLeftY + (float)Row * TileMap->TileSideInPixels;
			*/
			float CenX = ScreenCenterX + (float)RelColumn * TileSideInPixels - GameState->PlayerP.OffsetX * PixelsPerMeter;
			float CenY = ScreenCenterY + (float)RelRow * TileSideInPixels - GameState->PlayerP.OffsetY * PixelsPerMeter;
			float MinX = CenX - 0.5f*TileSideInPixels;
			float MinY = CenY - 0.5f*TileSideInPixels;
			float MaxX = MinX + TileSideInPixels;
			float MaxY = MinY + TileSideInPixels;

			DrawRectangle(Buffer, MinX, Buffer->Height-(MaxY), 
						  MaxX, Buffer->Height-(MinY),
						  Gray, Gray, Gray);
		}
	}
	float PlayerR = 1.0f;
	float PlayerG = 1.0f;
	float PlayerB = 0.0f;

	float PlayerLeft = ScreenCenterX - 0.5f*PlayerWidth * PixelsPerMeter;
	float PlayerTop = ScreenCenterY - PixelsPerMeter*PlayerHeight;
	
	
	DrawRectangle(Buffer,
				  PlayerLeft, PlayerTop,
				  PlayerLeft+PlayerWidth*PixelsPerMeter, 
				  PlayerTop + PixelsPerMeter*PlayerHeight,
				  PlayerR, PlayerG, PlayerB);
	
	DrawBitmap(Buffer, &GameState->HeroHead, PlayerLeft, PlayerTop);

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
