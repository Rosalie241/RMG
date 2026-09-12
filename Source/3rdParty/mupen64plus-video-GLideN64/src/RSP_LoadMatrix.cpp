#include <string.h>
#include "RSP.h"
#include "3DMath.h"

// An N64 matrix is 16 s16 integer parts followed by 16 u16 fraction parts.
static const u32 N64_MATRIX_SIZE = 64;
static const u32 N64_MATRIX_FRACTION_OFFSET = 32;

bool RSP_LoadMatrix( f32 mtx[4][4], u32 address )
{
	// Written to avoid overflowing the addition; address comes from a display
	// list and is not trustworthy.
	if (address > RDRAMSize || RDRAMSize - address < N64_MATRIX_SIZE)
		return false;

	// Read each element with memcpy instead of overlaying a struct on RDRAM.
	// Casting u8* to a struct pointer violates strict aliasing, and it assumes
	// an alignment that an arbitrary display list address does not guarantee.
	// A 2-byte memcpy compiles to a single load.
	const u8* src = RDRAM + address;
	for (u32 i = 0; i < 4; i++) {
		for (u32 j = 0; j < 4; j++) {
			// Halfwords are swapped within each 32-bit word of RDRAM, hence j ^ 1.
			const u32 element = (i * 4 + (j ^ 1)) * 2;
			s16 integer;
			u16 fraction;
			memcpy(&integer, src + element, sizeof(integer));
			memcpy(&fraction, src + N64_MATRIX_FRACTION_OFFSET + element, sizeof(fraction));
			mtx[i][j] = GetFloatMatrixElement(integer, fraction);
		}
	}

	return true;
}
