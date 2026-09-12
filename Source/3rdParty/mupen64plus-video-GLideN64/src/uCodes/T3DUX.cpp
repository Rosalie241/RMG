#include <string.h>
#include "T3DUX.h"
#include "N64.h"
#include "RSP.h"
#include "RDP.h"
#include "gSP.h"
#include "gDP.h"
#include "DisplayWindow.h"
#include "Log.h"

/******************T3DUX microcode*************************/

struct T3DUXGlobState
{
	u16 pad0;
	u16 perspNorm;
	u32 flag;
	u32 othermode0;
	u32 othermode1;
	u32 segBases[16];
	/* the viewport to use */
	s16 vsacle1;
	s16 vsacle0;
	s16 vsacle3;
	s16 vsacle2;
	s16 vtrans1;
	s16 vtrans0;
	s16 vtrans3;
	s16 vtrans2;
	u32 rdpCmds;
};

struct T3DUXState
{
	u32 renderState;	/* render state */

	u8 dmemVtxAddr;
	u8 vtxCount;	/* number of verts */
	u8 texmode;
	u8 geommode;

	u8 dmemVtxAttribsAddr;
	u8 attribsCount;	/* number of colors and texture coords */
	u8 matrixFlag;
	u8 triCount;	/* how many tris? */

	u32 rdpCmds;	/* ptr (segment address) to RDP DL */
	u32 othermode0;
	u32 othermode1;
};


struct T3DUXTriN
{
	u8	flag, v2, v1, v0; /* flag is which one for flat shade */
	u8	pal, v2tex, v1tex, v0tex; /* indexes in texture coords list */
};

static u32 t32uxSetTileW0 = 0;
static u32 t32uxSetTileW1 = 0;

static
void T3DUX_ProcessRDP(u32 _cmds)
{
	// addr indexes RDRAM as 32-bit words. RDRAMSize is the index of the last
	// valid byte, so RDRAM holds (RDRAMSize + 1) / 4 whole words.
	const u32 rdramWords = (RDRAMSize + 1) >> 2;
	u32 addr = RSP_SegmentToPhysical(_cmds) >> 2;
	if (addr != 0) {
		if (addr + 2 > rdramWords) {
			LOG(LOG_ERROR, "T3DUX_ProcessRDP: command list at 0x%08x lies outside RDRAM", _cmds);
			return;
		}
		bool truncated = false;
		RSP.LLE = true;
		u32 w0 = ((u32*)RDRAM)[addr++];
		u32 w1 = ((u32*)RDRAM)[addr++];
		RSP.cmd = _SHIFTR( w0, 24, 8 );
		while (w0 + w1 != 0) {
			GBI.cmd[RSP.cmd]( w0, w1 );
			// A malformed list has no terminator, so every read has to be
			// checked against the end of RDRAM.
			if (addr + 2 > rdramWords) {
				truncated = true;
				break;
			}
			w0 = ((u32*)RDRAM)[addr++];
			w1 = ((u32*)RDRAM)[addr++];
			RSP.cmd = _SHIFTR( w0, 24, 8 );
			switch (RSP.cmd) {
			case G_TEXRECT:
			case G_TEXRECTFLIP:
				if (addr + 2 > rdramWords) {
					truncated = true;
					break;
				}
				RDP.w2 = ((u32*)RDRAM)[addr++];
				RDP.w3 = ((u32*)RDRAM)[addr++];
				break;
			case G_SETTILE:
				t32uxSetTileW0 = w0;
				t32uxSetTileW1 = w1;
				break;
			}
			if (truncated)
				break;
		}
		RSP.LLE = false;
		if (truncated)
			LOG(LOG_ERROR, "T3DUX_ProcessRDP: command list at 0x%08x ran past the end of RDRAM", _cmds);
	}
}

static
void T3DUX_LoadGlobState(u32 pgstate)
{
	const u32 addr = RSP_SegmentToPhysical(pgstate);
	if (!isRDRAMRangeValid(addr, sizeof(T3DUXGlobState))) {
		LOG(LOG_ERROR, "T3DUX_LoadGlobState: state at 0x%08x lies outside RDRAM", pgstate);
		return;
	}
	// Copy out rather than casting RDRAM to the struct type: the cast
	// violates strict aliasing and assumes an alignment a display list
	// address does not guarantee.
	T3DUXGlobState gstate;
	memcpy(&gstate, RDRAM + addr, sizeof(gstate));
	const u32 w0 = gstate.othermode0;
	const u32 w1 = gstate.othermode1;
	gDPSetOtherMode( _SHIFTR( w0, 0, 24 ),	// mode0
					 w1 );					// mode1

	for (int s = 0; s < 16; ++s)
		gSPSegment(s, gstate.segBases[s] & 0x00FFFFFF);

	gSPViewport(pgstate + 80);

	T3DUX_ProcessRDP(gstate.rdpCmds);
}

static
void T3DUX_LoadObject(u32 pstate, u32 pvtx, u32 ptri, u32 pcol)
{
	const u32 stateAddr = RSP_SegmentToPhysical(pstate);
	if (!isRDRAMRangeValid(stateAddr, sizeof(T3DUXState))) {
		LOG(LOG_ERROR, "T3DUX_LoadObject: state at 0x%08x lies outside RDRAM", pstate);
		return;
	}
	T3DUXState ostateStorage;
	memcpy(&ostateStorage, RDRAM + stateAddr, sizeof(ostateStorage));
	const T3DUXState * ostate = &ostateStorage;
	// TODO: fix me
	const u32 tile = 0;
	gSP.texture.tile = tile;
	gSP.textureTile[0] = &gDP.tiles[tile];
	gSP.textureTile[1] = &gDP.tiles[(tile + 1) & 7];
	gSP.texture.scales = 1.0f;
	gSP.texture.scalet = 1.0f;

	{
		const u32 w0 = ostate->othermode0;
		const u32 w1 = ostate->othermode1;
		gDPSetOtherMode(
			_SHIFTR(w0, 0, 24),	// mode0
			w1);				// mode1
	}

	if ((ostate->matrixFlag & 1) == 0) //load matrix
		gSPForceMatrix(pstate + sizeof(T3DUXState));

	gSPClearGeometryMode(G_LIGHTING | G_FOG);
	gSPSetGeometryMode(ostate->renderState | G_SHADING_SMOOTH | G_SHADE | G_ZBUFFER | G_CULL_BACK);

	if (pvtx != 0) //load vtx
		gSPT3DUXVertex(pvtx, ostate->vtxCount, pcol);

	T3DUX_ProcessRDP(ostate->rdpCmds);

	if (ptri == 0)
		return;

	GraphicsDrawer & drawer = dwnd().getDrawer();
	const u32 coladdr = RSP_SegmentToPhysical(pcol);
	const u32 triaddr = RSP_SegmentToPhysical(ptri);
	u8 pal = _SHIFTR(t32uxSetTileW1, 20, 4);
	t32uxSetTileW1 &= 0xFF0FFFFF;
	const bool flatShading = (ostate->geommode & 0x0F) == 0;
	const bool texturing = ostate->texmode != 1;
	f32 flatr, flatg, flatb, flata;

	// triCount is a u8, so the triangle list is at most 255 * 8 bytes.
	if (!isRDRAMRangeValid(triaddr, ostate->triCount * sizeof(T3DUXTriN))) {
		LOG(LOG_ERROR, "T3DUX_LoadObject: triangle list at 0x%08x lies outside RDRAM", ptri);
		return;
	}

	// The colour and texture coordinate lookups below index from coladdr with
	// a u8 scaled by 4 and masked to 0x03FC, so at most the 0x400 bytes at
	// coladdr can be touched.
	static const u32 COLOR_TABLE_SIZE = 0x400;
	if ((flatShading || texturing) && !isRDRAMRangeValid(coladdr, COLOR_TABLE_SIZE)) {
		LOG(LOG_ERROR, "T3DUX_LoadObject: colour table at 0x%08x lies outside RDRAM", pcol);
		return;
	}

	drawer.setDMAVerticesSize(ostate->triCount * 3);
	SPVertex * pVtx = drawer.getDMAVerticesData();
	for (int t = 0; t < ostate->triCount; ++t) {
		T3DUXTriN triStorage;
		memcpy(&triStorage, RDRAM + triaddr + t * sizeof(T3DUXTriN), sizeof(triStorage));
		const T3DUXTriN * tri = &triStorage;

		if (texturing && tri->pal != 0) {
			const u32 w1 = t32uxSetTileW1 | (tri->pal << 20);
			const u32 newPal = _SHIFTR(w1, 20, 4);
			if (pal != newPal) {
				drawer.drawDMATriangles(static_cast<u32>(pVtx - drawer.getDMAVerticesData()));
				pVtx = drawer.getDMAVerticesData();
				pal = newPal;
				RDP_SetTile(t32uxSetTileW0, w1);
			}
		}

		if (tri->v0 >= ostate->vtxCount || tri->v1 >= ostate->vtxCount || tri->v2 >= ostate->vtxCount)
			continue;

		if (drawer.isClipped(tri->v0, tri->v1, tri->v2))
			continue;

		if (flatShading) {
			struct T3DUXColor
			{
				u8 a;
				u8 b;
				u8 g;
				u8 r;
			} color;
			memcpy(&color, RDRAM + coladdr + ((tri->flag << 2) & 0x03FC), sizeof(color));
			flata = _FIXED2FLOAT(color.a, 8);
			flatb = _FIXED2FLOAT(color.b, 8);
			flatg = _FIXED2FLOAT(color.g, 8);
			flatr = _FIXED2FLOAT(color.r, 8);
		}

		u32 vtxIdx[3] = { tri->v0, tri->v1, tri->v2 };
		u32 texIdx[3] = { tri->v0tex, tri->v1tex, tri->v2tex };
		for (u32 v = 0; v < 3; ++v) {
			*pVtx = drawer.getVertex(vtxIdx[v]);

			if (texturing) {
				u32 texcoords;
				memcpy(&texcoords, RDRAM + coladdr + (texIdx[v] << 2), sizeof(texcoords));
				pVtx->s = _FIXED2FLOAT(_SHIFTR(texcoords, 16, 16), 5);
				pVtx->t = _FIXED2FLOAT(_SHIFTR(texcoords, 0, 16), 5);
			} else {
				pVtx->s = 0.0f;
				pVtx->t = 0.0f;
			}

			if (flatShading) {
				pVtx->r = flatr;
				pVtx->g = flatg;
				pVtx->b = flatb;
				pVtx->a = flata;
			}

			++pVtx;
		}
	}

	drawer.drawDMATriangles(static_cast<u32>(pVtx - drawer.getDMAVerticesData()));
}

void RunT3DUX()
{
	// Five words are read per command; the PC advances by 24 with nothing
	// else bounding it, so it has to be checked every iteration the way
	// _ProcessDList does for the normal microcodes.
	static const u32 T3DUX_COMMAND_READ_SIZE = 20;

	while (true) {
		if (!isRDRAMRangeValid(RSP.PC[RSP.PCi], T3DUX_COMMAND_READ_SIZE)) {
			LOG(LOG_ERROR, "RunT3DUX: display list ran past the end of RDRAM at 0x%08x", RSP.PC[RSP.PCi]);
			RSP.halt = true;
			break;
		}
		u32 addr = RSP.PC[RSP.PCi] >> 2;
		const u32 pgstate = ((u32*)RDRAM)[addr++];
		const u32 pstate = ((u32*)RDRAM)[addr++];
		const u32 pvtx = ((u32*)RDRAM)[addr++];
		const u32 ptri = ((u32*)RDRAM)[addr++];
		const u32 pcol = ((u32*)RDRAM)[addr++];
		//const u32 pstore = ((u32*)RDRAM)[addr];
		if (pstate == 0) {
			RSP.halt = true;
			break;
		}
		if (pgstate != 0)
			T3DUX_LoadGlobState(pgstate);
		T3DUX_LoadObject(pstate, pvtx, ptri, pcol);
		// Go to the next instruction
		RSP.PC[RSP.PCi] += 24;
	};
}
