#include "GLideN64.h"
#include "DebugDump.h"
#include "F3D.h"
#include "F3DEX.h"
#include "F3DEX2.h"
#include "L3DEX2.h"
#include "N64.h"
#include "RSP.h"
#include "RDP.h"
#include "gSP.h"
#include "gDP.h"
#include "GBI.h"

void L3DEX2_Line3D( u32 w0, u32 w1 )
{
	s32 wd = static_cast<s8>(_SHIFTR( w0, 0, 8 ));
	u32 v0 = _SHIFTR(w0, 17, 7);
	u32 v1 = _SHIFTR(w0, 9, 7);
	gSPLine3D( v0, v1, wd, v0 );
}

void L3DEX2_Tri1(u32 w0, u32 w1)
{
	s32 wd = static_cast<s8>(_SHIFTR(w1, 24, 8));
	u32 v0 = _SHIFTR(w0, 17, 7);
	u32 v1 = _SHIFTR(w0,  9, 7);
	u32 v2 = _SHIFTR(w0,  1, 7);

	if (v0 != v1)
		gSPLine3D(v0, v1, wd, v0);
	if (v1 != v2)
		gSPLine3D(v1, v2, wd, v0);
	if (v2 != v0)
		gSPLine3D(v2, v0, wd, v0);
}

void L3DEX2_Tri2(u32 w0, u32 w1)
{
	s32 wd = static_cast<s8>(_SHIFTR(w1, 24, 8));
	u32 v0 = _SHIFTR(w0, 17, 7);
	u32 v1 = _SHIFTR(w0, 9, 7);
	u32 v2 = _SHIFTR(w0, 1, 7);
	u32 v3 = _SHIFTR(w1, 17, 7);
	u32 v4 = _SHIFTR(w1, 9, 7);
	u32 v5 = _SHIFTR(w1, 1, 7);

	if (v0 != v1)
		gSPLine3D(v0, v1, wd, v0);
	if (v1 != v2)
		gSPLine3D(v1, v2, wd, v0);
	if (v2 != v0)
		gSPLine3D(v2, v0, wd, v0);

	if (v3 != v4)
		gSPLine3D(v3, v4, wd, v3);
	if (v4 != v5)
		gSPLine3D(v4, v5, wd, v3);
	if (v5 != v3)
		gSPLine3D(v5, v3, wd, v3);
}

void L3DEX2_Init()
{
	gSPSetupFunctions();
	// Set GeometryMode flags
	GBI_InitFlags( F3DEX2 );

	GBI.PCStackSize = 18;

	// GBI Command						Command Value				Command Function
//	GBI_SetGBI( G_BG_COPY,				0x0A,						S2DEX_BG_Copy );
	GBI_SetGBI( G_RDPHALF_2,			F3DEX2_RDPHALF_2,			F3D_RDPHalf_2 );
	GBI_SetGBI( G_SETOTHERMODE_H,		F3DEX2_SETOTHERMODE_H,		F3DEX2_SetOtherMode_H );
	GBI_SetGBI( G_SETOTHERMODE_L,		F3DEX2_SETOTHERMODE_L,		F3DEX2_SetOtherMode_L );
	GBI_SetGBI( G_RDPHALF_1,			F3DEX2_RDPHALF_1,			F3D_RDPHalf_1 );
	GBI_SetGBI( G_SPNOOP,				F3DEX2_SPNOOP,				F3D_SPNoOp );
	GBI_SetGBI( G_ENDDL,				F3DEX2_ENDDL,				F3D_EndDL );
	GBI_SetGBI( G_DL,					F3DEX2_DL,					F3D_DList );
	GBI_SetGBI( G_LOAD_UCODE,			F3DEX2_LOAD_UCODE,			F3DEX_Load_uCode );
	GBI_SetGBI( G_MOVEMEM,				F3DEX2_MOVEMEM,				F3DEX2_MoveMem );
	GBI_SetGBI( G_MOVEWORD,				F3DEX2_MOVEWORD,			F3DEX2_MoveWord );
	GBI_SetGBI( G_MTX,					F3DEX2_MTX,					F3DEX2_Mtx );
	GBI_SetGBI( G_GEOMETRYMODE,			F3DEX2_GEOMETRYMODE,		F3DEX2_GeometryMode );
	GBI_SetGBI( G_POPMTX,				F3DEX2_POPMTX,				F3DEX2_PopMtx );
	GBI_SetGBI( G_TEXTURE,				F3DEX2_TEXTURE,				F3DEX2_Texture );
	GBI_SetGBI( G_DMA_IO,				F3DEX2_DMA_IO,				F3DEX2_DMAIO );
	GBI_SetGBI( G_SPECIAL_1,			F3DEX2_SPECIAL_1,			F3DEX2_Special_1 );
	GBI_SetGBI( G_SPECIAL_2,			F3DEX2_SPECIAL_2,			F3DEX2_Special_2 );
	GBI_SetGBI( G_SPECIAL_3,			F3DEX2_SPECIAL_3,			F3DEX2_Special_3 );

	GBI_SetGBI( G_VTX,					F3DEX2_VTX,					F3DEX2_Vtx );
	GBI_SetGBI( G_MODIFYVTX,			F3DEX2_MODIFYVTX,			F3DEX_ModifyVtx );
	GBI_SetGBI(	G_CULLDL,				F3DEX2_CULLDL,				F3DEX_CullDL );
	GBI_SetGBI( G_BRANCH_Z,				F3DEX2_BRANCH_Z,			F3DEX_Branch_Z );
	GBI_SetGBI( G_TRI1,					L3DEX2_TRI1,				L3DEX2_Tri1 );
	GBI_SetGBI( G_TRI2,					L3DEX2_TRI2,				L3DEX2_Tri2 );
//	GBI_SetGBI( G_QUAD,					F3DEX2_QUAD,				F3DEX2_Quad );
	GBI_SetGBI( G_LINE3D,				L3DEX2_LINE3D,				L3DEX2_Line3D );
}

