//
// TODO(casey): Convert all of these to platform-efficient versions
// and rmove math.h
//
#include "math.h"

inline uint32 
RoundReal32ToUint32(float Real32)
{
	uint32 Result = (uint32)roundf(Real32);
	return Result;
}

inline int32 
RoundReal32ToInt32(float Real32)
{
	int32 Result = (int32)roundf(Real32);
	return Result;
}

inline int32 
FloorReal32ToInt32(float Real32)
{
	int32 Result = (int32)floorf(Real32);
	return Result;
}

inline int32 
TruncateReal32ToInt32(float Real32)
{
	int32 Result = (int32)(Real32);
	return Result;
}

inline float
Sin(float Angle)
{
	float Result = sinf(Angle);
	return Result;
}

inline float
Cos(float Angle)
{
	float Result = cosf(Angle);
	return Result;
}

inline float
ATan2(float Y, float X)
{
	float Result = atan2f(Y, X);
	return Result;
}

struct bit_scan_result
{
	bool Found;
	uint32 Index;
};
internal bit_scan_result
FindLeastSignificantBit(uint32 Value)
{
	bit_scan_result Result = {};
#if COMPILER_MSVC
	Result.Found = _BitScanForward((unsigned long *)&Result.Index, Value);
#else
	for (uint32 Bit = 0;
		 Bit < 32;
		 ++Bit)
	{
		if (Value & (1 << Bit))
		{
			Result.Index = Bit;
			Result.Found = true;
			break;
		}
	}
#endif
	return Result;
}
