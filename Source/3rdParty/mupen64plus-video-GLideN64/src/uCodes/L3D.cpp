#include "GLideN64.h"
#include "DebugDump.h"
#include "F3D.h"
#include "L3D.h"
#include "N64.h"
#include "RSP.h"
#include "RDP.h"
#include "gSP.h"
#include "gDP.h"
#include "GBI.h"

void L3D_Line3D( u32 w0, u32 w1 )
{
	s32 wd = static_cast<s8>(_SHIFTR( w1, 0, 8 ));
	u32 v0 = _SHIFTR(w1, 16, 8) / 10;
	u32 v1 = _SHIFTR(w1, 8, 8) / 10;
	u32 flag = _SHIFTR(w1, 24, 8);
	gSPLine3D( v0, v1, wd, flag == 0 ? v0 : v1 );
}

void L3D_Tri1(u32 w0, u32 w1)
{
	u32 v0 = _SHIFTR(w1, 16, 8) / 10;
	u32 v1 = _SHIFTR(w1,  8, 8) / 10;
	u32 v2 = _SHIFTR(w0,  0, 8) / 10;
	u32 flag = _SHIFTR(w1, 24, 8);
	u32 flatShadeVtx = v0;
	switch (flag) {
		case 0x01:
			flatShadeVtx = v1;
			break;
		case 0x02:
			flatShadeVtx = v2;
			break;
	}
	if (v0 != v1)
		gSPLine3D(v0, v1, 0, flatShadeVtx);
	if (v1 != v2)
		gSPLine3D(v1, v2, 0, flatShadeVtx);
	if (v2 != v0)
		gSPLine3D(v2, v0, 0, flatShadeVtx);
}

void L3D_Init()
{
	gSPSetupFunctions();
	// Set GeometryMode flags
	GBI_InitFlags( F3D );

	GBI.PCStackSize = 10;

	//          GBI Command             Command Value			Command Function
	GBI_SetGBI( G_SPNOOP,				F3D_SPNOOP,				F3D_SPNoOp );
	GBI_SetGBI( G_MTX,					F3D_MTX,				F3D_Mtx );
	GBI_SetGBI( G_RESERVED0,			F3D_RESERVED0,			F3D_Reserved0 );
	GBI_SetGBI( G_MOVEMEM,				F3D_MOVEMEM,			F3D_MoveMem );
	GBI_SetGBI( G_VTX,					F3D_VTX,				F3D_Vtx );
	GBI_SetGBI( G_RESERVED1,			F3D_RESERVED1,			F3D_Reserved1 );
	GBI_SetGBI( G_DL,					F3D_DL,					F3D_DList );
	GBI_SetGBI( G_RESERVED2,			F3D_RESERVED2,			F3D_Reserved2 );
	GBI_SetGBI( G_RESERVED3,			F3D_RESERVED3,			F3D_Reserved3 );
	GBI_SetGBI( G_SPRITE2D_BASE,		F3D_SPRITE2D_BASE,		F3D_Sprite2D_Base );

	GBI_SetGBI( G_TRI1,					L3D_TRI1,				L3D_Tri1 );
	GBI_SetGBI( G_CULLDL,				F3D_CULLDL,				F3D_CullDL );
	GBI_SetGBI( G_POPMTX,				F3D_POPMTX,				F3D_PopMtx );
	GBI_SetGBI( G_MOVEWORD,				F3D_MOVEWORD,			F3D_MoveWord );
	GBI_SetGBI( G_TEXTURE,				F3D_TEXTURE,			F3D_Texture );
	GBI_SetGBI( G_SETOTHERMODE_H,		F3D_SETOTHERMODE_H,		F3D_SetOtherMode_H );
	GBI_SetGBI( G_SETOTHERMODE_L,		F3D_SETOTHERMODE_L,		F3D_SetOtherMode_L );
	GBI_SetGBI( G_ENDDL,				F3D_ENDDL,				F3D_EndDL );
	GBI_SetGBI( G_SETGEOMETRYMODE,		F3D_SETGEOMETRYMODE,	F3D_SetGeometryMode );
	GBI_SetGBI( G_CLEARGEOMETRYMODE,	F3D_CLEARGEOMETRYMODE,	F3D_ClearGeometryMode );
	GBI_SetGBI( G_LINE3D,				L3D_LINE3D,				L3D_Line3D );
	GBI_SetGBI( G_RDPHALF_1,			F3D_RDPHALF_1,			F3D_RDPHalf_1 );
	GBI_SetGBI( G_RDPHALF_2,			F3D_RDPHALF_2,			F3D_RDPHalf_2 );
	GBI_SetGBI( G_RDPHALF_CONT,			F3D_RDPHALF_CONT,		F3D_RDPHalf_Cont );
//	GBI_SetGBI( G_TRI4,					F3D_TRI4,				F3D_Tri4 );
}

