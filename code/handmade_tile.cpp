inline uint32
GetTileValueUnchecked(tile_map *TileMap, tile_chunk *TileChunk, uint32 TileX, uint32 TileY)
{
	Assert(TileChunk);
	Assert(TileX < TileMap->ChunkDim);
	Assert(TileY < TileMap->ChunkDim);

	uint32 TileChunkValue = TileChunk->Tiles[TileX + TileY * TileMap->ChunkDim];
	return TileChunkValue;
}

inline void 
SetTileValueUnchecked(tile_map *TileMap, tile_chunk *TileChunk, 
					  uint32 TileX, uint32 TileY, uint32 Value)
{
	Assert(TileChunk);
	Assert(TileX < TileMap->ChunkDim);
	Assert(TileY < TileMap->ChunkDim);

	TileChunk->Tiles[TileX + TileY * TileMap->ChunkDim] = Value;
}

inline tile_chunk *
GetTileChunk(tile_map *TileMap, uint32 TileChunkX, uint32 TileChunkY, uint32 TileChunkZ)
{
	tile_chunk *TileChunk = 0;

	if ((TileChunkX < TileMap->TileChunkCountX) && (TileChunkX >= 0) &&
		(TileChunkY < TileMap->TileChunkCountY) && (TileChunkY >= 0) &&
		(TileChunkZ < TileMap->TileChunkCountZ) && (TileChunkZ >= 0))
	{
		TileChunk = &TileMap->TileChunks[
			TileChunkX + 
			TileChunkY * TileMap->TileChunkCountX + 
			TileChunkZ * TileMap->TileChunkCountX * TileMap->TileChunkCountY];
	}

	return TileChunk;
}

internal uint32
GetTileValue(tile_map *TileMap, tile_chunk *TileChunk, uint32 TestTileX, uint32 TestTileY)
{
	uint32 TileChunkValue = 0;

	if (TileChunk && TileChunk->Tiles)
	{
		TileChunkValue = GetTileValueUnchecked(TileMap, TileChunk, TestTileX, TestTileY);
	}

	return TileChunkValue;
}

internal void
SetTileValue(tile_map *TileMap, tile_chunk *TileChunk, 
			 uint32 TestTileX, uint32 TestTileY, uint32 Value)
{
	if (TileChunk && TileChunk->Tiles)
	{
		SetTileValueUnchecked(TileMap, TileChunk, 
							  TestTileX, TestTileY, Value);
	}
}


inline tile_chunk_position
GetChunkPositionFor(tile_map *TileMap, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ)
{
	tile_chunk_position Result = {};
	Result.TileChunkX = AbsTileX >> TileMap->ChunkShift;
	Result.TileChunkY = AbsTileY >> TileMap->ChunkShift;
	Result.TileChunkZ = AbsTileZ;
	Result.RelTileX = AbsTileX & TileMap->ChunkMask;
	Result.RelTileY = AbsTileY & TileMap->ChunkMask;

	return Result;
}

internal uint32
GetTileValue(tile_map *TileMap, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ)
{
	tile_chunk_position ChunkPos = GetChunkPositionFor(TileMap, 
													   AbsTileX, AbsTileY, AbsTileZ);
	tile_chunk *TileChunk = GetTileChunk(TileMap, ChunkPos.TileChunkX, ChunkPos.TileChunkY, ChunkPos.TileChunkZ);
	uint32 TileChunkValue = GetTileValue(TileMap, TileChunk, ChunkPos.RelTileX, ChunkPos.RelTileY);

	return TileChunkValue;
}

internal uint32
GetTileValue(tile_map *TileMap, tile_map_position Pos)
{
	uint32 TileChunkValue = GetTileValue(TileMap, Pos.AbsTileX, Pos.AbsTileY, Pos.AbsTileZ);

	return TileChunkValue;
}

internal bool
IsTileMapPointEmpty(tile_map *TileMap, tile_map_position Pos)
{
	uint32 TileChunkValue = GetTileValue(TileMap, Pos);
	bool Empty = ((TileChunkValue == 1) ||
				  (TileChunkValue == 3) ||
				  (TileChunkValue == 4));

	return Empty;
}

internal void
SetTileValue(memory_arena *Arena, tile_map *TileMap, 
			 uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ, 
			 uint32 Value)
{
	tile_chunk_position ChunkPos = GetChunkPositionFor(TileMap, 
													   AbsTileX, AbsTileY, AbsTileZ);
	tile_chunk *TileChunk = GetTileChunk(TileMap, ChunkPos.TileChunkX, ChunkPos.TileChunkY, ChunkPos.TileChunkZ);

	Assert(TileChunk);
	if (!TileChunk->Tiles)
	{
		uint32 TileCount = TileMap->ChunkDim*TileMap->ChunkDim;
		uint32 *Tiles = PushArray(Arena, TileCount, uint32); 
		TileChunk->Tiles = Tiles;

		for (uint32 TileIndex = 0;
			 TileIndex < TileCount;
			 ++TileIndex)
		{
			Tiles[TileIndex] = 1;	
		}
	}

	SetTileValue(TileMap, TileChunk, ChunkPos.RelTileX, ChunkPos.RelTileY, Value);
}

//
// TODO(casey): Do there really belong in more of a "positioning"
// or "geometry" file?
//

inline void
RecanonicalizeCoord(tile_map *TileMap, uint32 *Tile, float *TileRel)
{
	// TODO(casey): need to do something that doesn't use the divide/mult method
	// for recanonicalizing cause this can end up rounding bach on to the tile 
	// you just came from

	// NOTE(casey): TileMap is assumed to be toroidal, if you step off of one 
	// end you come back on the other!
	int32 TileOffset = RoundReal32ToInt32(*TileRel / TileMap->TileSideInMeters);
	*Tile += TileOffset;
	*TileRel -= TileOffset * TileMap->TileSideInMeters;

	Assert(*TileRel >= -0.5f*TileMap->TileSideInMeters);
	// TODO(casey): Fix floating point math so this can be <
	Assert(*TileRel <= 0.5f*TileMap->TileSideInMeters);
}

inline tile_map_position 
RecanonicalizePosition(tile_map *TileMap, tile_map_position Pos)
{
	tile_map_position Result = Pos;

	RecanonicalizeCoord(TileMap, &Result.AbsTileX, &Result.OffsetX);
	RecanonicalizeCoord(TileMap, &Result.AbsTileY, &Result.OffsetY);

	return Result;
}

inline bool 
AreOnSameTile(tile_map_position A, tile_map_position B)
{
	bool Result = ((A.AbsTileX == B.AbsTileX) &&
				   (A.AbsTileY == B.AbsTileY) &&
				   (A.AbsTileZ == B.AbsTileZ));

	return Result;
}
