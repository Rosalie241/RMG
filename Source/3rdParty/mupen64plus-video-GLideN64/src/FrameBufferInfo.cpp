#include "FrameBufferInfoAPI.h"
#include "FrameBufferInfo.h"
#include "Config.h"
#include "gSP.h"
#include "FrameBuffer.h"
#include "DepthBuffer.h"
#include "RSP.h"
#include "VI.h"
#include "Log.h"

namespace FBInfo {

	FBInfo fbInfo;

	FBInfo::FBInfo()
	{
		reset();
	}

	void FBInfo::reset() {
		m_supported = false;
		m_writeBuffers.fill(nullptr);
		m_readBuffers.fill(nullptr);
	}

	// Searches _buffers (a densely packed, nullptr-terminated prefix) for _buf.
	//
	// Returns (true, index of the entry) when _buf is present.
	// Returns (false, index of the first free slot) when it is not.
	// Returns (false, _buffers.size()) when it is not present and the array is
	// full - a legitimate runtime condition, not a programming error. Callers
	// must check the index against _buffers.size() before using it for a write.
	FBInfo::BufferSearchResult FBInfo::_findBuffer(const BuffersArray& _buffers, const FrameBuffer* _buf) const
	{
		const u32 size = static_cast<u32>(_buffers.size());
		for (u32 i = 0; i < size; ++i) {
			if (_buffers[i] == nullptr)
				return BufferSearchResult(false, i);
			if (_buffers[i] == _buf)
				return BufferSearchResult(true, i);
		}
		return BufferSearchResult(false, size);
	}


	void FBInfo::Write(u32 addr, u32 size)
	{
		const u32 address = RSP_SegmentToPhysical(addr);
		const FrameBuffer* writeBuffer = frameBufferList().findBuffer(address);
		if (writeBuffer == nullptr)
			return;
		const auto findRes = _findBuffer(m_writeBuffers, writeBuffer);
		if (!findRes.first && findRes.second < m_writeBuffers.size())
			m_writeBuffers[findRes.second] = writeBuffer;
		FrameBuffer_AddAddress(address, size);
	}

	void FBInfo::WriteList(FrameBufferModifyEntry *plist, u32 size)
	{
		LOG(LOG_WARNING, "FBWList size=%u", size);
	}

	void FBInfo::Read(u32 addr)
	{
		const u32 address = RSP_SegmentToPhysical(addr);
		FrameBuffer * pBuffer = frameBufferList().findBuffer(address);

		if (pBuffer == nullptr || _findBuffer(m_writeBuffers, pBuffer).first)
			return;

		const auto findRes = _findBuffer(m_readBuffers, pBuffer);
		if (pBuffer->m_isDepthBuffer) {
			if (config.frameBufferEmulation.fbInfoReadDepthChunk != 0)
				FrameBuffer_CopyDepthBufferChunk(address);
			else if (!findRes.first)
				FrameBuffer_CopyDepthBuffer(address);
		} else {
			if (config.frameBufferEmulation.fbInfoReadColorChunk != 0)
				FrameBuffer_CopyChunkToRDRAM(address);
			else if (!findRes.first)
				FrameBuffer_CopyToRDRAM(address, true);
		}

		if (!findRes.first && findRes.second < m_readBuffers.size())
			m_readBuffers[findRes.second] = pBuffer;
	}

	void FBInfo::GetInfo(void *pinfo)
	{
		if (pinfo == nullptr) {
			LOG(LOG_ERROR, "FBGetInfo called with a null buffer");
			return;
		}

		FrameBufferInfo * pFBInfo = static_cast<FrameBufferInfo*>(pinfo);
		memset(pFBInfo, 0, sizeof(FrameBufferInfo) * numFrameBufferInfos);

		if (config.frameBufferEmulation.fbInfoDisabled != 0)
			return;

		u32 idx = 0;
		DepthBuffer * pDepthBuffer = depthBufferList().getCurrent();
		if (pDepthBuffer != nullptr) {
			pFBInfo[idx].addr = pDepthBuffer->m_address;
			pFBInfo[idx].width = pDepthBuffer->m_width;
			pFBInfo[idx].height = VI.real_height;
			pFBInfo[idx++].size = 2;
		}
		frameBufferList().fillBufferInfo(&pFBInfo[idx], numFrameBufferInfos - idx);

		m_writeBuffers.fill(nullptr);
		m_readBuffers.fill(nullptr);
		m_supported = true;
	}
}
