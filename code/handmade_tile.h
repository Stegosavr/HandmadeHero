struct tile_map_difference
{
	v2 dXY;
	float dZ;
};

struct tile_map_position
{
	// NOTE(casey): these are fixed point tile locations.
	// the high bits are the tile chunk index, and the low bits
	// are the tile index in the chunk
	uint32 AbsTileX;
	uint32 AbsTileY;
	uint32 AbsTileZ;

	// NOTE(casey): These are the offsets from tile center 
	v2 Offset;
};

struct tile_chunk_position
{
	uint32 TileChunkX;
	uint32 TileChunkY;
	uint32 TileChunkZ;

	uint32 RelTileX;
	uint32 RelTileY;
};

struct tile_chunk
{
	uint32 *Tiles;
};

struct tile_map
{
	uint32 ChunkShift;
	uint32 ChunkMask;
	uint32 ChunkDim;

	float TileSideInMeters;

	int32 CountX;
	int32 CountY;

	// TODO(casey): REAL sparseness so anywhere in the world can be
	// represented without giant pointer array
	uint32 TileChunkCountX;
	uint32 TileChunkCountY;
	uint32 TileChunkCountZ;

	tile_chunk *TileChunks;
};
