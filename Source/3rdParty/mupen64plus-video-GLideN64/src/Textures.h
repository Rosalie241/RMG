#ifndef TEXTURES_H
#define TEXTURES_H

#include <array>
#include <list>
#include <map>
#include <unordered_map>
#include <vector>
#include <stddef.h> // for size_t

#include "CRC.h"
#include "convert.h"
#include "Graphics/ObjectHandle.h"
#include "Graphics/Parameter.h"

typedef u32 (*GetTexelFunc)(u16 offset, u16 x, u16 i, u8 palette);
typedef u32 (*GetTexelFuncBG)(u64 *src, u16 x, u16 i, u8 palette);
struct GHQTexInfo;

struct CachedTexture
{
	CachedTexture(graphics::ObjectHandle _name)
		: name(_name)
	{}

	graphics::ObjectHandle name{ 0 };
	u64		crc{ 0 };
	float	offsetS{ 0 }, offsetT{ 0 };
	u8		maskS{ 0 }, maskT{ 0 };
	u8		clampS{ 0 }, clampT{ 0 };
	u8		mirrorS{ 0 }, mirrorT{ 0 };
	u16		line{ 0 };
	u16		size{ 0 };
	u16		format{ 0 };
	u32		tMem{ 0 };
	u32		palette{ 0 };
	u16		width{ 0 }, height{ 0 };			// N64 width and height
	u16		clampWidth{ 0 }, clampHeight{ 0 };  // Size to clamp to
	f32		scaleS{ 0 }, scaleT{ 0 };			// Scale to map to 0.0-1.0
	f32     hdRatioS{ 0 }, hdRatioT{ 0 };       // HD / N64 width and height
	f32		shiftScaleS{ 0 }, shiftScaleT{ 0 }; // Scale to shift
	u32		textureBytes{ 0 };

	u32		address{ 0 };
	u8		max_level{ 0 };
	u16		mipmapAtlasWidth{ 0 };
	u16		mipmapAtlasHeight{ 0 };
	enum {
		fbNone = 0,
		fbOneSample = 1,
		fbMultiSample = 2
	} frameBufferTexture{ fbNone };
	bool bHDTexture{ false };
};

struct TextureCache
{
	CachedTexture * current[2];

	void init();
	void destroy();
	void clear();
	CachedTexture * addFrameBufferTexture(graphics::Parameter _target);
	void removeFrameBufferTexture(CachedTexture * _pTexture);
	void activateTexture(u32 _t, CachedTexture *_pTexture);
	void activateDummy(u32 _t);
	void activateMSDummy(u32 _t);
	void update(u32 _t);
	void toggleDumpTex();

	static TextureCache & get();

private:
	TextureCache()
		: m_pDummy(nullptr)
		, m_pMSDummy(nullptr)
		, m_hits(0)
		, m_misses(0)
		, m_curUnpackAlignment(4)
		, m_toggleDumpTex(false)
	{
		current[0] = nullptr;
		current[1] = nullptr;
		CRC_Init();
	}
	TextureCache(const TextureCache &) = delete;

	void _checkCacheSize();
	void _checkHdTexLimit();
	CachedTexture * _addTexture(u64 _crc64);
	void _loadFast(u32 _tile, CachedTexture *_pTexture);
	void _loadAccurate(u32 _tile, CachedTexture *_pTexture);
	bool _loadHiresTexture(u32 _tile, CachedTexture *_pTexture, u64 & _ricecrc, u64 & _strongcrc);
	void _loadBackground(CachedTexture *pTexture);
	bool _loadHiresBackground(CachedTexture *_pTexture, u64 & _ricecrc);
	void _loadDepthTexture(CachedTexture * _pTexture, u16* _pDest);
	void _updateBackground();
	void _initDummyTexture(CachedTexture * _pDummy);
	void _getTextureDestData(CachedTexture& tmptex, u32* pDest, graphics::Parameter glInternalFormat, GetTexelFunc GetTexel, u16* pLine);
	void _updateCachedTexture(const GHQTexInfo & _info, CachedTexture *_pTexture, u16 widthOrg, u16 heightOrg);

	typedef std::list<CachedTexture> Textures;
	typedef std::unordered_map<u64, Textures::iterator> Texture_Locations;
	typedef std::unordered_map<u32, CachedTexture> FBTextures;
	Textures m_textures;
	Texture_Locations m_lruTextureLocations;
	FBTextures m_fbTextures;
	CachedTexture * m_pDummy;
	CachedTexture * m_pMSDummy;
	u32 m_hits, m_misses;
	s32 m_curUnpackAlignment;
	bool m_toggleDumpTex;
	std::vector<u32> m_tempTextureHolder;

#ifdef VC
	const size_t m_maxCacheSize = 1500u;
#else
	const size_t m_maxCacheSize = 8000u;
#endif
	u64 m_hdTexCacheSize = 0u;
};

void getTextureShiftScale(u32 tile, const TextureCache & cache, f32 & shiftScaleS, f32 & shiftScaleT);

// Check for situation when Tex0 is used instead of Tex1
bool needReplaceTex1ByTex0();

inline TextureCache & textureCache()
{
	return TextureCache::get();
}

#endif
