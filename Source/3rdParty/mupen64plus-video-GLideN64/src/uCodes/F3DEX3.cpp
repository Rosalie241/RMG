#include "GLideN64.h"
#include "F3D.h"
#include "F3DEX.h"
#include "F3DEX2.h"
#include "F3DZEX2.h"
#include "F3DEX3.h"
#include "gSP.h"

#define	F3DEX3_BRANCH_WZ	0x04

#define F3DEX3_TRISTRIP          0x08
#define F3DEX3_TRIFAN            0x09
#define F3DEX3_LIGHTTORDP        0x0A
#define F3DEX3_RELSEGMENT        0x0B

#define F3DEX3_G_MW_FX		     0x00
#define F3DEX3_G_MW_LIGHTCOL     0x0A

#define F3DEX3_G_MV_MMTX 2

#define F3DEX3_G_MW_HALFWORD_FLAG 0x8000

#define F3DEX3_G_MWO_AO_AMBIENT         0x00
#define F3DEX3_G_MWO_AO_DIRECTIONAL     0x02
#define F3DEX3_G_MWO_AO_POINT           0x04
#define F3DEX3_G_MWO_PERSPNORM          0x06
#define F3DEX3_G_MWO_FRESNEL_SCALE      0x0C
#define F3DEX3_G_MWO_FRESNEL_OFFSET     0x0E
#define F3DEX3_G_MWO_ATTR_OFFSET_S      0x10
#define F3DEX3_G_MWO_ATTR_OFFSET_T      0x12
#define F3DEX3_G_MWO_ATTR_OFFSET_Z      0x14
#define F3DEX3_G_MWO_ALPHA_COMPARE_CULL 0x16
#define F3DEX3_G_MWO_NORMALS_MODE       0x18
#define F3DEX3_G_MWO_LAST_MAT_DL_ADDR   0x1A

#define F3DEX3_G_MAX_LIGHTS 9

struct F3DEX3_Ambient
{
	u8 pad0, b, g, r;
	u8 pad1, b2, g2, r2;
};

struct F3DEX3_Light
{
	u8 pad0, b, g, r;
	u8 pad1, b2, g2, r2;
	s8 pad2, z, y, x;
	u8 size, pad3[3];
};

// Notice how it looks like F3DEX3_Light but the ending so there are 8 bytes of difference
struct F3DEX3_LookAt
{
	s8 pad, z, y, x;
};

struct F3DEX3_LookAtOld
{
	u8 pad0, b, g, r;
	u8 pad1, b2, g2, r2;
	s8 pad2, z, y, x;
};

// TODO: This seems incorrect to me?
static void F3DZEX3_Branch_W(u32 w0, u32 w1)
{
	gSPBranchLessW(gDP.half_1, _SHIFTR(w0, 1, 7), w1);
}

#define _LIGHT_TO_OFFSET(n) (((n) - 1) * 0x10 + 0x10) /* The + 0x10 skips cam pos and lookat */

static void writeLight(int off, u32 w)
{
	if (0 == off)
	{
		// CameraWorld not supported
	}
	if (0x8 == off)
	{
		// gSPLookAt would like to have a F3DEX3_LookAtOld struct, so we need to do some math magic
		gSPLookAt(w - (sizeof(F3DEX3_LookAtOld) - sizeof(F3DEX3_LookAt)), 0);
		gSPLookAt(w - (sizeof(F3DEX3_LookAtOld) - sizeof(F3DEX3_LookAt)) + sizeof(F3DEX3_LookAt), 1);
	}

	for (u32 i = 1; i <= gSP.numLights; i++)
	{
		if (_LIGHT_TO_OFFSET(i) == off)
		{
			gSPLight(w, i);
		}
	}

	if (_LIGHT_TO_OFFSET(gSP.numLights + 1) == off)
	{
		// TODO: Write ambient lights
	}
	if ((F3DEX3_G_MAX_LIGHTS * 0x10) + 0x18 == off)
	{
		// TODO: OcclusionPlane not supported
	}
}

void F3DEX3_MoveMem(u32 w0, u32 w1)
{
	switch (_SHIFTR(w0, 0, 8))
	{
	case F3DEX3_G_MV_MMTX:
		// TODO: Not supported!
		break;
	case F3DEX2_MV_VIEWPORT:
		gSPViewport(w1);
		break;
	case G_MV_LIGHT:
	{
		const u32 ofs = _SHIFTR(w0, 8, 8) * 8;
		const u32 len = (1 + _SHIFTR(w0, 19, 5)) * 8;
		for (u32 i = 0; i < len; i += 4)
		{
			writeLight(ofs + i, w1 + i);
		}
	}
	break;
	}
}

void F3DEX3_MoveWord(u32 w0, u32 w1)
{
	switch (_SHIFTR(w0, 16, 8))
	{
	case F3DEX3_G_MW_FX:
	{
		const u32 value = _SHIFTR(w0, 0, 16);
		u32 what = w1;
		if (value & F3DEX3_G_MW_HALFWORD_FLAG)
			what &= 0xffff;

		switch (value & ~F3DEX3_G_MW_HALFWORD_FLAG)
		{
			case F3DEX3_G_MWO_AO_AMBIENT:
				break;
			case F3DEX3_G_MWO_AO_DIRECTIONAL:
				break;
			case F3DEX3_G_MWO_AO_POINT:
				break;
			case F3DEX3_G_MWO_PERSPNORM:
				gSPPerspNormalize(what);
				break;
			case F3DEX3_G_MWO_FRESNEL_SCALE:
				break;
			case F3DEX3_G_MWO_FRESNEL_OFFSET:
				break;
			case F3DEX3_G_MWO_ATTR_OFFSET_S:
				break;
			case F3DEX3_G_MWO_ATTR_OFFSET_T:
				break;
			case F3DEX3_G_MWO_ATTR_OFFSET_Z:
				break;
			case F3DEX3_G_MWO_ALPHA_COMPARE_CULL:
				break;
			case F3DEX3_G_MWO_NORMALS_MODE:
				break;
			case F3DEX3_G_MWO_LAST_MAT_DL_ADDR:
				break;
		}
	}
		break;
	case G_MW_NUMLIGHT:
		gSPNumLights(w1 / 0x10);
		break;
	case G_MW_SEGMENT:
		gSPSegment(_SHIFTR(w0, 2, 4), w1 & 0x00FFFFFF);
		break;
	case G_MW_FOG:
		gSPFogFactor((s16)_SHIFTR(w1, 16, 16), (s16)_SHIFTR(w1, 0, 16));
		break;
	case G_MW_LIGHTCOL:
		int off = _SHIFTR(w0, 0, 16);
		gSPLightColor((off / 0x10) + 1, w1);
		break;
	}
}

struct Vertices7
{
	u8 v[7];

	inline bool valid(u8 i) const
	{ return v[i] < 64; }
};

static inline Vertices7 unpackVertices7(u32 w0, u32 w1)
{
	Vertices7 v;
	v.v[0] = _SHIFTR(w0, 17, 7);
	v.v[1] = _SHIFTR(w0, 9, 7);
	v.v[2] = _SHIFTR(w0, 1, 7);
	v.v[3] = _SHIFTR(w1, 25, 7);
	v.v[4] = _SHIFTR(w1, 17, 7);
	v.v[5] = _SHIFTR(w1, 9, 7);
	v.v[6] = _SHIFTR(w1, 1, 7);
	return v;
}

static void F3DEX3_TriStrip(u32 w0, u32 w1)
{
	Vertices7 vertices = unpackVertices7(w0, w1);
	// *v1 - v2 - v3, v3 - v2 - v4, v3 - v4 - v5, v5 - v4 - v6, v5 - v6 - v7
	if (!vertices.valid(0) || !vertices.valid(1) || !vertices.valid(2)) return;
	gSPTriangle(vertices.v[0], vertices.v[1], vertices.v[2]);

	if (!vertices.valid(3)) return;
	gSPTriangle(vertices.v[2], vertices.v[1], vertices.v[3]);

	if (!vertices.valid(4)) return;
	gSPTriangle(vertices.v[2], vertices.v[3], vertices.v[4]);

	if (!vertices.valid(5)) return;
	gSPTriangle(vertices.v[4], vertices.v[3], vertices.v[5]);

	if (!vertices.valid(6)) return;
	gSPTriangle(vertices.v[4], vertices.v[5], vertices.v[6]);
}

static void F3DEX3_TriFan(u32 w0, u32 w1)
{
	Vertices7 vertices = unpackVertices7(w0, w1);
	// *v1 - v2 - v3, v1 - v3 - v4, v1 - v4 - v5, v1 - v5 - v6, v1 - v6 - v7
	if (!vertices.valid(0) || !vertices.valid(1) || !vertices.valid(2)) return;
	gSPTriangle(vertices.v[0], vertices.v[1], vertices.v[2]);

	if (!vertices.valid(3)) return;
	gSPTriangle(vertices.v[0], vertices.v[2], vertices.v[3]);

	if (!vertices.valid(4)) return;
	gSPTriangle(vertices.v[0], vertices.v[3], vertices.v[4]);

	if (!vertices.valid(5)) return;
	gSPTriangle(vertices.v[0], vertices.v[3], vertices.v[5]);

	if (!vertices.valid(6)) return;
	gSPTriangle(vertices.v[0], vertices.v[5], vertices.v[6]);
}

static void F3DEX3_LightToRDP(u32 w0, u32 w1)
{
	// unsupported, i do not know what this is for
}

static void F3DEX3_RelSegment(u32 w0, u32 w1)
{
	gSPRelSegment(_SHIFTR(w0, 2, 4), w1 & 0x00FFFFFF);
}

void F3DEX3_Init()
{
	gSPSetupFunctions();
	// Set GeometryMode flags
	GBI_InitFlags(F3DEX2);

	GBI.PCStackSize = 18;

	// GBI Command						Command Value				Command Function
	GBI_SetGBI(G_RDPHALF_2, F3DEX2_RDPHALF_2, F3D_RDPHalf_2);
	GBI_SetGBI(G_SETOTHERMODE_H, F3DEX2_SETOTHERMODE_H, F3DEX2_SetOtherMode_H);
	GBI_SetGBI(G_SETOTHERMODE_L, F3DEX2_SETOTHERMODE_L, F3DEX2_SetOtherMode_L);
	GBI_SetGBI(G_RDPHALF_1, F3DEX2_RDPHALF_1, F3D_RDPHalf_1);
	GBI_SetGBI(G_SPNOOP, F3DEX2_SPNOOP, F3D_SPNoOp);
	GBI_SetGBI(G_ENDDL, F3DEX2_ENDDL, F3D_EndDL);
	GBI_SetGBI(G_DL, F3DEX2_DL, F3D_DList);
	GBI_SetGBI(G_LOAD_UCODE, F3DEX2_LOAD_UCODE, F3DEX_Load_uCode);
	GBI_SetGBI(G_MOVEMEM, F3DEX2_MOVEMEM, F3DEX3_MoveMem);
	GBI_SetGBI(G_MOVEWORD, F3DEX2_MOVEWORD, F3DEX3_MoveWord);
	GBI_SetGBI(G_MTX, F3DEX2_MTX, F3DEX2_Mtx);
	GBI_SetGBI(G_GEOMETRYMODE, F3DEX2_GEOMETRYMODE, F3DEX2_GeometryMode);
	GBI_SetGBI(G_POPMTX, F3DEX2_POPMTX, F3DEX2_PopMtx);
	GBI_SetGBI(G_TEXTURE, F3DEX2_TEXTURE, F3DEX2_Texture);
	GBI_SetGBI(G_DMA_IO, F3DEX2_DMA_IO, F3DEX2_DMAIO);
	GBI_SetGBI(G_SPECIAL_1, F3DEX2_SPECIAL_1, F3DEX2_Special_1);
	GBI_SetGBI(G_SPECIAL_2, F3DEX2_SPECIAL_2, F3DEX2_Special_2);
	GBI_SetGBI(G_SPECIAL_3, F3DEX2_SPECIAL_3, F3DEX2_Special_3);

	GBI_SetGBI(G_VTX, F3DEX2_VTX, F3DEX2_Vtx);
	GBI_SetGBI(G_MODIFYVTX, F3DEX2_MODIFYVTX, F3DEX_ModifyVtx);
	GBI_SetGBI(G_CULLDL, F3DEX2_CULLDL, F3DEX_CullDL);
	GBI_SetGBI(G_BRANCH_W, F3DEX3_BRANCH_WZ, F3DZEX3_Branch_W);
	GBI_SetGBI(G_TRI1, F3DEX2_TRI1, F3DEX2_Tri1);
	GBI_SetGBI(G_TRI2, F3DEX2_TRI2, F3DEX_Tri2);
	GBI_SetGBI(G_QUAD, F3DEX2_QUAD, F3DEX2_Quad);
	GBI_SetGBI(G_TRISTRIP, F3DEX3_TRISTRIP, F3DEX3_TriStrip);
	GBI_SetGBI(G_TRIFAN, F3DEX3_TRIFAN, F3DEX3_TriFan);
	GBI_SetGBI(G_LIGHTTORDP, F3DEX3_LIGHTTORDP, F3DEX3_LightToRDP);
	GBI_SetGBI(G_RELSEGMENT, F3DEX3_RELSEGMENT, F3DEX3_RelSegment);
}
