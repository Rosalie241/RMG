//
// Copyright (c) 2019-2022 Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include "Common.h"
#include "Tests.h"
#include <thread>

// Define to the same value as you did for D3D12MemAlloc.cpp.
#ifndef D3D12MA_DEBUG_MARGIN
    #define D3D12MA_DEBUG_MARGIN 0
#endif

extern ID3D12GraphicsCommandList* BeginCommandList();
extern DXGI_ADAPTER_DESC1 g_AdapterDesc;
extern void EndCommandList(ID3D12GraphicsCommandList* cmdList);

enum CONFIG_TYPE
{
    CONFIG_TYPE_MINIMUM,
    CONFIG_TYPE_SMALL,
    CONFIG_TYPE_AVERAGE,
    CONFIG_TYPE_LARGE,
    CONFIG_TYPE_MAXIMUM,
    CONFIG_TYPE_COUNT
};

enum class FREE_ORDER { FORWARD, BACKWARD, RANDOM, COUNT };

static const char* CODE_DESCRIPTION = "D3D12MA Tests";
static constexpr UINT64 KILOBYTE = 1024;
static constexpr UINT64 MEGABYTE = 1024 * KILOBYTE;
static constexpr CONFIG_TYPE ConfigType = CONFIG_TYPE_AVERAGE;
static const char* FREE_ORDER_NAMES[] = { "FORWARD", "BACKWARD", "RANDOM", };

static void CurrentTimeToStr(std::string& out)
{
    time_t rawTime; time(&rawTime);
    struct tm timeInfo; localtime_s(&timeInfo, &rawTime);
    char timeStr[128];
    strftime(timeStr, _countof(timeStr), "%c", &timeInfo);
    out = timeStr;
}

static float ToFloatSeconds(duration d)
{
    return std::chrono::duration_cast<std::chrono::duration<float>>(d).count();
}

static const char* AlgorithmToStr(D3D12MA::POOL_FLAGS algorithm)
{
    switch (algorithm)
    {
    case D3D12MA::POOL_FLAG_ALGORITHM_LINEAR:
        return "Linear";
    case 0:
        return "TLSF";
    default:
        assert(0);
        return "";
    }
}

static const char* VirtualAlgorithmToStr(D3D12MA::VIRTUAL_BLOCK_FLAGS algorithm)
{
    switch (algorithm)
    {
    case D3D12MA::VIRTUAL_BLOCK_FLAG_ALGORITHM_LINEAR:
        return "Linear";
    case 0:
        return "TLSF";
    default:
        assert(0);
        return "";
    }
}

static const wchar_t* DefragmentationAlgorithmToStr(UINT32 algorithm)
{
    switch (algorithm)
    {
    case D3D12MA::DEFRAGMENTATION_FLAG_ALGORITHM_BALANCED:
        return L"Balanced";
    case D3D12MA::DEFRAGMENTATION_FLAG_ALGORITHM_FAST:
        return L"Fast";
    case D3D12MA::DEFRAGMENTATION_FLAG_ALGORITHM_FULL:
        return L"Full";
    case 0:
        return L"Default";
    default:
        assert(0);
        return L"";
    }
}

struct ResourceWithAllocation
{
    ComPtr<ID3D12Resource> resource;
    ComPtr<D3D12MA::Allocation> allocation;
    UINT64 size = UINT64_MAX;
    UINT dataSeed = 0;

    void Reset()
    {
        resource.Reset();
        allocation.Reset();
        size = UINT64_MAX;
        dataSeed = 0;
    }
};

template<typename D3D12_RESOURCE_DESC_T>
static void FillResourceDescForBuffer(D3D12_RESOURCE_DESC_T& outResourceDesc, UINT64 size)
{
    outResourceDesc = {};
    outResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    outResourceDesc.Alignment = 0;
    outResourceDesc.Width = size;
    outResourceDesc.Height = 1;
    outResourceDesc.DepthOrArraySize = 1;
    outResourceDesc.MipLevels = 1;
    outResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
    outResourceDesc.SampleDesc.Count = 1;
    outResourceDesc.SampleDesc.Quality = 0;
    outResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    outResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
}

static void FillData(void* outPtr, const UINT64 sizeInBytes, UINT seed)
{
    UINT* outValues = (UINT*)outPtr;
    const UINT64 sizeInValues = sizeInBytes / sizeof(UINT);
    UINT value = seed;
    for(UINT i = 0; i < sizeInValues; ++i)
    {
        outValues[i] = value++;
    }
}

static void FillAllocationsData(const ComPtr<D3D12MA::Allocation>* allocs, size_t allocCount, UINT seed)
{
    std::for_each(allocs, allocs + allocCount, [seed](const ComPtr<D3D12MA::Allocation>& alloc)
        {
            D3D12_RANGE range = {};
            void* ptr;
            CHECK_HR(alloc->GetResource()->Map(0, &range, &ptr));
            FillData(ptr, alloc->GetSize(), seed);
            alloc->GetResource()->Unmap(0, nullptr);
        });
}

static void FillAllocationsDataGPU(const TestContext& ctx, const ComPtr<D3D12MA::Allocation>* allocs, size_t allocCount, UINT seed)
{
    D3D12MA::ALLOCATION_DESC allocDesc = {};
    allocDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;
    allocDesc.ExtraHeapFlags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;
    allocDesc.Flags = D3D12MA::ALLOCATION_FLAGS::ALLOCATION_FLAG_COMMITTED;

    std::vector<D3D12_RESOURCE_BARRIER> barriers;
    std::vector<ComPtr<D3D12MA::Allocation>> uploadAllocs;
    barriers.reserve(allocCount);
    uploadAllocs.reserve(allocCount);

    // Move resource into right state
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;

    ID3D12GraphicsCommandList* cl = BeginCommandList();
    std::for_each(allocs, allocs + allocCount, [&](const ComPtr<D3D12MA::Allocation>& alloc)
        {
            // Copy only buffers for now
            D3D12_RESOURCE_DESC resDesc = alloc->GetResource()->GetDesc();
            if (resDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
            {
                ComPtr<D3D12MA::Allocation> uploadAlloc;
                CHECK_HR(ctx.allocator->CreateResource(&allocDesc, &resDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
                    nullptr, &uploadAlloc, IID_NULL, nullptr));

                D3D12_RANGE range = {};
                void* ptr;
                CHECK_HR(uploadAlloc->GetResource()->Map(0, &range, &ptr));
                FillData(ptr, resDesc.Width, seed);
                uploadAlloc->GetResource()->Unmap(0, nullptr);

                cl->CopyResource(alloc->GetResource(), uploadAlloc->GetResource());
                uploadAllocs.emplace_back(std::move(uploadAlloc));
            }

            barrier.Transition.pResource = alloc->GetResource();
            barrier.Transition.StateAfter = (D3D12_RESOURCE_STATES)(uintptr_t)alloc->GetPrivateData();
            barriers.emplace_back(barrier);
        });
    cl->ResourceBarrier(static_cast<UINT>(allocCount), barriers.data());
    EndCommandList(cl);
}

static bool ValidateData(const void* ptr, const UINT64 sizeInBytes, UINT seed)
{
    const UINT* values = (const UINT*)ptr;
    const UINT64 sizeInValues = sizeInBytes / sizeof(UINT);
    UINT value = seed;
    for(UINT i = 0; i < sizeInValues; ++i)
    {
        if(values[i] != value++)
        {
            //CHECK_BOOL(0 && "ValidateData failed.");
            return false;
        }
    }
    return true;
}

static bool ValidateDataZero(const void* ptr, const UINT64 sizeInBytes)
{
    const UINT* values = (const UINT*)ptr;
    const UINT64 sizeInValues = sizeInBytes / sizeof(UINT);
    for(UINT i = 0; i < sizeInValues; ++i)
    {
        if(values[i] != 0)
        {
            //CHECK_BOOL(0 && "ValidateData failed.");
            return false;
        }
    }
    return true;
}

static void ValidateAllocationsData(const ComPtr<D3D12MA::Allocation>* allocs, size_t allocCount, UINT seed)
{
    std::for_each(allocs, allocs + allocCount, [seed](const ComPtr<D3D12MA::Allocation>& alloc)
        {
            D3D12_RANGE range = {};
            void* ptr;
            CHECK_HR(alloc->GetResource()->Map(0, &range, &ptr));
            CHECK_BOOL(ValidateData(ptr, alloc->GetSize(), seed));
            alloc->GetResource()->Unmap(0, nullptr);
        });
}

static void ValidateAllocationsDataGPU(const TestContext& ctx, const ComPtr<D3D12MA::Allocation>* allocs, size_t allocCount, UINT seed)
{
    D3D12MA::ALLOCATION_DESC allocDesc = {};
    allocDesc.HeapType = D3D12_HEAP_TYPE_READBACK;
    allocDesc.ExtraHeapFlags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;
    allocDesc.Flags = D3D12MA::ALLOCATION_FLAGS::ALLOCATION_FLAG_COMMITTED;

    std::vector<D3D12_RESOURCE_BARRIER> barriers;
    std::vector<ComPtr<D3D12MA::Allocation>> downloadAllocs;
    barriers.reserve(allocCount);
    downloadAllocs.reserve(allocCount);

    // Move resource into right state
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;

    ID3D12GraphicsCommandList* cl = BeginCommandList();
    size_t resCount = allocCount;
    std::for_each(allocs, allocs + allocCount, [&](const ComPtr<D3D12MA::Allocation>& alloc)
        {
            // Check only buffers for now
            D3D12_RESOURCE_DESC resDesc = alloc->GetResource()->GetDesc();
            if (resDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
            {
                ComPtr<D3D12MA::Allocation> downloadAlloc;
                CHECK_HR(ctx.allocator->CreateResource(&allocDesc, &resDesc, D3D12_RESOURCE_STATE_COPY_DEST,
                    nullptr, &downloadAlloc, IID_NULL, nullptr));

                barrier.Transition.pResource = alloc->GetResource();
                barrier.Transition.StateBefore = (D3D12_RESOURCE_STATES)(uintptr_t)alloc->GetPrivateData();
                barriers.emplace_back(barrier);
                downloadAllocs.emplace_back(std::move(downloadAlloc));
            }
            else
                --resCount;
        });

    cl->ResourceBarrier(static_cast<UINT>(resCount), barriers.data());
    for (size_t i = 0, j = 0; i < resCount; ++j)
    {
        if (allocs[j]->GetResource()->GetDesc().Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
        {
            cl->CopyResource(downloadAllocs.at(i)->GetResource(), allocs[j]->GetResource());
            barriers.at(i).Transition.StateAfter = barriers.at(i).Transition.StateBefore;
            barriers.at(i).Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
            ++i;
        }
    }
    cl->ResourceBarrier(static_cast<UINT>(resCount), barriers.data());
    EndCommandList(cl);

    for (auto& alloc : downloadAllocs)
    {
        D3D12_RANGE range = {};
        void* ptr;
        CHECK_HR(alloc->GetResource()->Map(0, &range, &ptr));
        CHECK_BOOL(ValidateData(ptr, alloc->GetResource()->GetDesc().Width, seed));
        alloc->GetResource()->Unmap(0, nullptr);
    }
}

static void SaveStatsStringToFile(const TestContext& ctx, const wchar_t* dstFilePath, BOOL detailed = TRUE)
{
    WCHAR* s = nullptr;
    ctx.allocator->BuildStatsString(&s, detailed);
    SaveFile(dstFilePath, s, wcslen(s) * sizeof(WCHAR));
    ctx.allocator->FreeStatsString(s);
}


static void TestDebugMargin(const TestContext& ctx)
{
    using namespace D3D12MA;

    if(D3D12MA_DEBUG_MARGIN == 0)
    {
        return;
    }

    wprintf(L"Test D3D12MA_DEBUG_MARGIN = %u\n", (uint32_t)D3D12MA_DEBUG_MARGIN);

    ALLOCATION_DESC allocDesc = {};

    D3D12_RESOURCE_DESC resDesc = {};

    POOL_DESC poolDesc = {};
    poolDesc.HeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;

    for(size_t algorithmIndex = 0; algorithmIndex < 2; ++algorithmIndex)
    {
        switch(algorithmIndex)
        {
        case 0: poolDesc.Flags = POOL_FLAG_NONE; break;
        case 1: poolDesc.Flags = POOL_FLAG_ALGORITHM_LINEAR; break;
        default: assert(0);
        }
        ComPtr<Pool> pool;
        CHECK_HR(ctx.allocator->CreatePool(&poolDesc, &pool));

        allocDesc.CustomPool = pool.Get();

        // Create few buffers of different size.
        const size_t BUF_COUNT = 10;
        ComPtr<Allocation> buffers[BUF_COUNT];
        for(size_t allocIndex = 0; allocIndex < 10; ++allocIndex)
        {
            const bool isLast = allocIndex == BUF_COUNT - 1;
            FillResourceDescForBuffer(resDesc, (UINT64)(allocIndex + 1) * 0x10000);

            CHECK_HR(ctx.allocator->CreateResource(
                &allocDesc,
                &resDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                &buffers[allocIndex],
                IID_NULL, nullptr));
        }

        // JSON dump
        wchar_t* json = nullptr;
        ctx.allocator->BuildStatsString(&json, TRUE);
        int I = 1; // Put breakpoint here to manually inspect json in a debugger.

        // Check if their offsets preserve margin between them.
        std::sort(buffers, buffers + BUF_COUNT, [](const ComPtr<Allocation>& lhs, const ComPtr<Allocation>& rhs) -> bool
            {
                if(lhs->GetHeap() != rhs->GetHeap())
                {
                    return lhs->GetHeap() < rhs->GetHeap();
                }
                return lhs->GetOffset() < rhs->GetOffset();
            });
        for(size_t i = 1; i < BUF_COUNT; ++i)
        {
            if(buffers[i]->GetHeap() == buffers[i - 1]->GetHeap())
            {
                const UINT64 allocStart = buffers[i]->GetOffset();
                const UINT64 prevAllocEnd = buffers[i - 1]->GetOffset() + buffers[i - 1]->GetSize();
                CHECK_BOOL(allocStart >= prevAllocEnd + D3D12MA_DEBUG_MARGIN);
            }
        }

        ctx.allocator->FreeStatsString(json);
    }
}

static void TestDebugMarginNotInVirtualAllocator(const TestContext& ctx)
{
    wprintf(L"Test D3D12MA_DEBUG_MARGIN not applied to virtual allocator\n");
    using namespace D3D12MA;
    constexpr size_t ALLOCATION_COUNT = 10;
    for(size_t algorithmIndex = 0; algorithmIndex < 2; ++algorithmIndex)
    {
        VIRTUAL_BLOCK_DESC blockDesc = {};
        blockDesc.Size = ALLOCATION_COUNT * MEGABYTE;
        switch(algorithmIndex)
        {
        case 0: blockDesc.Flags = VIRTUAL_BLOCK_FLAG_NONE; break;
        case 1: blockDesc.Flags = VIRTUAL_BLOCK_FLAG_ALGORITHM_LINEAR; break;
        default: assert(0);
        }

        ComPtr<VirtualBlock> block;
        CHECK_HR(CreateVirtualBlock(&blockDesc, &block));

        // Fill the entire block
        VirtualAllocation allocs[ALLOCATION_COUNT];
        for(size_t i = 0; i < ALLOCATION_COUNT; ++i)
        {
            VIRTUAL_ALLOCATION_DESC allocDesc = {};
            allocDesc.Size = 1 * MEGABYTE;
            CHECK_HR(block->Allocate(&allocDesc, &allocs[i], nullptr));
        }

        block->Clear();
    }
}

static void TestJson(const TestContext& ctx)
{
    wprintf(L"Test JSON\n");

    std::vector<ComPtr<D3D12MA::Pool>> pools;
    std::vector<ComPtr<D3D12MA::Allocation>> allocs;

    D3D12MA::ALLOCATION_DESC allocDesc = {};
    D3D12_RESOURCE_DESC resDesc = {};
    resDesc.Alignment = 0;
    resDesc.MipLevels = 1;
    resDesc.SampleDesc.Count = 1;
    resDesc.SampleDesc.Quality = 0;

    D3D12_RESOURCE_ALLOCATION_INFO allocInfo = {};
    allocInfo.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    allocInfo.SizeInBytes = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;

    // Select if using custom pool or default
    for (UINT8 poolType = 0; poolType < 2; ++poolType)
    {
        // Select different heaps
        for (UINT8 heapType = 0; heapType < 5; ++heapType)
        {
            D3D12_RESOURCE_STATES state;
            D3D12_CPU_PAGE_PROPERTY cpuPageType = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
            D3D12_MEMORY_POOL memoryPool = D3D12_MEMORY_POOL_UNKNOWN;
            switch (heapType)
            {
            case 0:
                allocDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
                state = D3D12_RESOURCE_STATE_COMMON;
                break;
            case 1:
                allocDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;
                state = D3D12_RESOURCE_STATE_GENERIC_READ;
                break;
            case 2:
                allocDesc.HeapType = D3D12_HEAP_TYPE_READBACK;
                state = D3D12_RESOURCE_STATE_COPY_DEST;
                break;
            case 3:
                allocDesc.HeapType = D3D12_HEAP_TYPE_CUSTOM;
                state = D3D12_RESOURCE_STATE_COMMON;
                cpuPageType = D3D12_CPU_PAGE_PROPERTY_NOT_AVAILABLE;
                memoryPool = ctx.allocator->IsUMA() ? D3D12_MEMORY_POOL_L0 : D3D12_MEMORY_POOL_L1;
                break;
            case 4:
                allocDesc.HeapType = D3D12_HEAP_TYPE_CUSTOM;
                state = D3D12_RESOURCE_STATE_GENERIC_READ;
                cpuPageType = D3D12_CPU_PAGE_PROPERTY_WRITE_COMBINE;
                memoryPool = D3D12_MEMORY_POOL_L0;
                break;
            }
            // Skip custom heaps for default pools
            if (poolType == 0 && heapType > 2)
                continue;
            const bool texturesPossible = heapType == 0 || heapType == 3;

            // Select different resource region types
            for (UINT8 resType = 0; resType < 3; ++resType)
            {
                allocDesc.ExtraHeapFlags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;
                D3D12_RESOURCE_FLAGS resFlags = D3D12_RESOURCE_FLAG_NONE;
                if (texturesPossible)
                {
                    switch (resType)
                    {
                    case 1:
                        allocDesc.ExtraHeapFlags = D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES;
                        break;
                    case 2:
                        allocDesc.ExtraHeapFlags = D3D12_HEAP_FLAG_ALLOW_ONLY_RT_DS_TEXTURES;
                        resFlags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
                        break;
                    }
                }

                switch (poolType)
                {
                case 0:
                    allocDesc.CustomPool = nullptr;
                    break;
                case 1:
                {
                    ComPtr<D3D12MA::Pool> pool;
                    D3D12MA::POOL_DESC poolDesc = {};
                    poolDesc.HeapFlags = allocDesc.ExtraHeapFlags;
                    poolDesc.HeapProperties.Type = allocDesc.HeapType;
                    poolDesc.HeapProperties.CPUPageProperty = cpuPageType;
                    poolDesc.HeapProperties.MemoryPoolPreference = memoryPool;
                    CHECK_HR(ctx.allocator->CreatePool(&poolDesc, &pool));

                    allocDesc.CustomPool = pool.Get();
                    pools.emplace_back(std::move(pool));
                    break;
                }
                }

                // Select different allocation flags
                for (UINT8 allocFlag = 0; allocFlag < 2; ++allocFlag)
                {
                    switch (allocFlag)
                    {
                    case 0:
                        allocDesc.Flags = D3D12MA::ALLOCATION_FLAG_NONE;
                        break;
                    case 1:
                        allocDesc.Flags = D3D12MA::ALLOCATION_FLAG_COMMITTED;
                        break;
                    }

                    // Select different alloc types (block, buffer, texture, etc.)
                    for (UINT8 allocType = 0; allocType < 5; ++allocType)
                    {
                        // Select different data stored in the allocation
                        for (UINT8 data = 0; data < 4; ++data)
                        {
                            ComPtr<D3D12MA::Allocation> alloc;

                            if (texturesPossible && resType != 0)
                            {
                                resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
                                resDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                                switch (allocType % 3)
                                {
                                case 0:
                                    resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
                                    resDesc.Width = 512;
                                    resDesc.Height = 1;
                                    resDesc.DepthOrArraySize = 1;
                                    resDesc.Flags = resFlags;
                                    CHECK_HR(ctx.allocator->CreateResource(&allocDesc, &resDesc, state, nullptr, &alloc, IID_NULL, nullptr));
                                    break;
                                case 1:
                                    resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
                                    resDesc.Width = 1024;
                                    resDesc.Height = 512;
                                    resDesc.DepthOrArraySize = 1;
                                    resDesc.Flags = resFlags;
                                    CHECK_HR(ctx.allocator->CreateResource(&allocDesc, &resDesc, state, nullptr, &alloc, IID_NULL, nullptr));
                                    break;
                                case 2:
                                    resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
                                    resDesc.Width = 512;
                                    resDesc.Height = 256;
                                    resDesc.DepthOrArraySize = 128;
                                    resDesc.Flags = resFlags;
                                    CHECK_HR(ctx.allocator->CreateResource(&allocDesc, &resDesc, state, nullptr, &alloc, IID_NULL, nullptr));
                                    break;
                                }
                            }
                            else
                            {
                                switch (allocType % 2)
                                {
                                case 0:
                                    CHECK_HR(ctx.allocator->AllocateMemory(&allocDesc, &allocInfo, &alloc));
                                    break;
                                case 1:
                                    FillResourceDescForBuffer(resDesc, 1024);
                                    CHECK_HR(ctx.allocator->CreateResource(&allocDesc, &resDesc, state, nullptr, &alloc, IID_NULL, nullptr));
                                    break;
                                }
                            }

                            switch (data)
                            {
                            case 1:
                                alloc->SetPrivateData((void*)16112007);
                                break;
                            case 2:
                                alloc->SetName(L"SHEPURD");
                                break;
                            case 3:
                                alloc->SetPrivateData((void*)26012010);
                                alloc->SetName(L"JOKER");
                                break;
                            }
                            allocs.emplace_back(std::move(alloc));
                        }
                    }

                }
            }
        }
    }
    SaveStatsStringToFile(ctx, L"JSON_D3D12.json");
}

static void TestCommittedResourcesAndJson(const TestContext& ctx)
{
    wprintf(L"Test committed resources and JSON\n");
    
    const UINT count = 4;
    const UINT64 bufSize = 32ull * 1024;
    const wchar_t* names[count] = {
        L"Resource\nFoo\r\nBar",
        L"Resource \"'&<>?#@!&-=_+[]{};:,./\\",
        nullptr,
        L"",
    };

    ResourceWithAllocation resources[count];

    D3D12MA::ALLOCATION_DESC allocDesc = {};
    allocDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
    allocDesc.Flags = D3D12MA::ALLOCATION_FLAG_COMMITTED;

    D3D12_RESOURCE_DESC resourceDesc;
    FillResourceDescForBuffer(resourceDesc, bufSize);

    for(UINT i = 0; i < count; ++i)
    {
        const bool receiveExplicitResource = i < 2;

        CHECK_HR( ctx.allocator->CreateResource(
            &allocDesc,
            &resourceDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            NULL,
            &resources[i].allocation,
            __uuidof(ID3D12Resource),
            receiveExplicitResource ? (void**)&resources[i].resource : NULL));

        if(receiveExplicitResource)
        {
            ID3D12Resource* res = resources[i].resource.Get();
            CHECK_BOOL(res && res == resources[i].allocation->GetResource());
            const ULONG refCountAfterAdd = res->AddRef();
            CHECK_BOOL(refCountAfterAdd == 3);
            res->Release();
        }
        
        // Make sure it has implicit heap.
        CHECK_BOOL( resources[i].allocation->GetHeap() == NULL && resources[i].allocation->GetOffset() == 0 );

        resources[i].allocation->SetName(names[i]);
    }

    // Check names.
    for(UINT i = 0; i < count; ++i)
    {
        const wchar_t* const allocName = resources[i].allocation->GetName();
        if(allocName)
        {
            CHECK_BOOL( wcscmp(allocName, names[i]) == 0 );
        }
        else
        {
            CHECK_BOOL(names[i] == NULL);
        }
    }

    WCHAR* jsonString;
    ctx.allocator->BuildStatsString(&jsonString, TRUE);
    CHECK_BOOL(wcsstr(jsonString, L"\"Resource\\nFoo\\r\\nBar\"") != NULL);
    CHECK_BOOL(wcsstr(jsonString, L"\"Resource \\\"'&<>?#@!&-=_+[]{};:,.\\/\\\\\"") != NULL);
    CHECK_BOOL(wcsstr(jsonString, L"\"\"") != NULL);
    ctx.allocator->FreeStatsString(jsonString);
}

static void TestSmallBuffers(const TestContext& ctx)
{
    wprintf(L"Test small buffers\n");

    D3D12MA::POOL_DESC poolDesc = {};
    poolDesc.HeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
    poolDesc.HeapFlags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;
    ComPtr<D3D12MA::Pool> pool;
    CHECK_HR(ctx.allocator->CreatePool(&poolDesc, &pool));

    D3D12MA::ALLOCATION_DESC allocDesc = {};
    allocDesc.CustomPool = pool.Get();

    D3D12_RESOURCE_DESC resDesc;
    FillResourceDescForBuffer(resDesc, 8 * KILOBYTE);

    D3D12_RESOURCE_DESC largeResDesc = resDesc;
    largeResDesc.Width = 128 * KILOBYTE;

    std::vector<ResourceWithAllocation> resources;

    // A large buffer placed inside the heap to allocate first block.
    {
        resources.emplace_back();
        ResourceWithAllocation& resWithAlloc = resources.back();
        CHECK_HR(ctx.allocator->CreateResource(&allocDesc, &largeResDesc, D3D12_RESOURCE_STATE_COMMON,
            nullptr, &resWithAlloc.allocation, IID_PPV_ARGS(&resWithAlloc.resource)));
        CHECK_BOOL(resWithAlloc.allocation && resWithAlloc.allocation->GetResource());
        CHECK_BOOL(resWithAlloc.allocation->GetHeap()); // Expected to be placed.
    }

    // Test 1: COMMITTED.
    {
        resources.emplace_back();
        ResourceWithAllocation& resWithAlloc = resources.back();
        allocDesc.Flags = D3D12MA::ALLOCATION_FLAG_COMMITTED;
        CHECK_HR(ctx.allocator->CreateResource(&allocDesc, &resDesc, D3D12_RESOURCE_STATE_COMMON,
            nullptr, &resWithAlloc.allocation, IID_PPV_ARGS(&resWithAlloc.resource)));
        CHECK_BOOL(resWithAlloc.allocation && resWithAlloc.allocation->GetResource());
        CHECK_BOOL(!resWithAlloc.allocation->GetHeap()); // Expected to be committed.
    }

    // Test 2: Default.
    {
        resources.emplace_back();
        ResourceWithAllocation& resWithAlloc = resources.back();
        allocDesc.Flags = D3D12MA::ALLOCATION_FLAG_NONE;
        CHECK_HR(ctx.allocator->CreateResource(&allocDesc, &resDesc, D3D12_RESOURCE_STATE_COMMON,
            nullptr, &resWithAlloc.allocation, IID_PPV_ARGS(&resWithAlloc.resource)));
        CHECK_BOOL(resWithAlloc.allocation && resWithAlloc.allocation->GetResource());
        CHECK_BOOL(!resWithAlloc.allocation->GetHeap()); // Expected to be committed.
    }

    // Test 3: NEVER_ALLOCATE.
    {
        resources.emplace_back();
        ResourceWithAllocation& resWithAlloc = resources.back();
        allocDesc.Flags = D3D12MA::ALLOCATION_FLAG_NEVER_ALLOCATE;
        CHECK_HR(ctx.allocator->CreateResource(&allocDesc, &resDesc, D3D12_RESOURCE_STATE_COMMON,
            nullptr, &resWithAlloc.allocation, IID_PPV_ARGS(&resWithAlloc.resource)));
        CHECK_BOOL(resWithAlloc.allocation && resWithAlloc.allocation->GetResource());
        CHECK_BOOL(resWithAlloc.allocation->GetHeap()); // Expected to be placed.
    }
}

static void TestCustomHeapFlags(const TestContext& ctx)
{
    wprintf(L"Test custom heap flags\n");

    // 1. Just memory heap with custom flags
    {
        D3D12MA::ALLOCATION_DESC allocDesc = {};
        allocDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
        allocDesc.ExtraHeapFlags = D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES |
            D3D12_HEAP_FLAG_SHARED; // Extra flag.

        D3D12_RESOURCE_ALLOCATION_INFO resAllocInfo = {};
        resAllocInfo.SizeInBytes = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
        resAllocInfo.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;

        ResourceWithAllocation res;
        CHECK_HR( ctx.allocator->AllocateMemory(&allocDesc, &resAllocInfo, &res.allocation) );

        // Must be created as separate allocation.
        CHECK_BOOL( res.allocation->GetOffset() == 0 );
    }

    // 2. Committed resource with custom flags
    {
        D3D12_RESOURCE_DESC resourceDesc = {};
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        resourceDesc.Alignment = 0;
        resourceDesc.Width = 1920;
        resourceDesc.Height = 1080;
        resourceDesc.DepthOrArraySize = 1;
        resourceDesc.MipLevels = 1;
        resourceDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        resourceDesc.SampleDesc.Count = 1;
        resourceDesc.SampleDesc.Quality = 0;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_CROSS_ADAPTER;

        D3D12MA::ALLOCATION_DESC allocDesc = {};
        allocDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
        allocDesc.ExtraHeapFlags = D3D12_HEAP_FLAG_SHARED | D3D12_HEAP_FLAG_SHARED_CROSS_ADAPTER; // Extra flags.

        ResourceWithAllocation res;
        CHECK_HR( ctx.allocator->CreateResource(
            &allocDesc,
            &resourceDesc,
            D3D12_RESOURCE_STATE_COMMON,
            NULL,
            &res.allocation,
            IID_PPV_ARGS(&res.resource)) );

        // Must be created as committed.
        CHECK_BOOL( res.allocation->GetHeap() == NULL );
    }
}

static void TestPlacedResources(const TestContext& ctx)
{
    wprintf(L"Test placed resources\n");

    const bool alwaysCommitted = (ctx.allocatorFlags & D3D12MA::ALLOCATOR_FLAG_ALWAYS_COMMITTED) != 0;

    const UINT count = 4;
    const UINT64 bufSize = 64ull * 1024;
    ResourceWithAllocation resources[count];

    D3D12MA::ALLOCATION_DESC allocDesc = {};
    allocDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_RESOURCE_DESC resourceDesc;
    FillResourceDescForBuffer(resourceDesc, bufSize);

    for(UINT i = 0; i < count; ++i)
    {
        CHECK_HR( ctx.allocator->CreateResource(
            &allocDesc,
            &resourceDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            NULL,
            &resources[i].allocation,
            IID_PPV_ARGS(&resources[i].resource)) );

        // Make sure it doesn't have implicit heap.
        if(!alwaysCommitted)
        {
            CHECK_BOOL( resources[i].allocation->GetHeap() != NULL );
        }
    }

    // Make sure at least some of the resources belong to the same heap, but their memory ranges don't overlap.
    bool sameHeapFound = false;
    for(size_t i = 0; i < count; ++i)
    {
        for(size_t j = i + 1; j < count; ++j)
        {
            const ResourceWithAllocation& resI = resources[i];
            const ResourceWithAllocation& resJ = resources[j];
            if(resI.allocation->GetHeap() != NULL &&
                resI.allocation->GetHeap() == resJ.allocation->GetHeap())
            {
                sameHeapFound = true;
                CHECK_BOOL(resI.allocation->GetOffset() + resI.allocation->GetSize() <= resJ.allocation->GetOffset() ||
                    resJ.allocation->GetOffset() + resJ.allocation->GetSize() <= resI.allocation->GetOffset());
            }
        }
    }
    if(!alwaysCommitted)
    {
        CHECK_BOOL(sameHeapFound);
    }

    // Additionally create a texture to see if no error occurs due to bad handling of Resource Tier.
    resourceDesc = {};
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resourceDesc.Alignment = 0;
    resourceDesc.Width = 1024;
    resourceDesc.Height = 1024;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.MipLevels = 1;
    resourceDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.SampleDesc.Quality = 0;
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    ResourceWithAllocation textureRes;
    CHECK_HR( ctx.allocator->CreateResource(
        &allocDesc,
        &resourceDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        NULL,
        &textureRes.allocation,
        IID_PPV_ARGS(&textureRes.resource)) );
    
    // Additionally create an MSAA render target to see if no error occurs due to bad handling of Resource Tier.
    resourceDesc = {};
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resourceDesc.Alignment = 0;
    resourceDesc.Width = 1920;
    resourceDesc.Height = 1080;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.MipLevels = 1;
    resourceDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    resourceDesc.SampleDesc.Count = 2;
    resourceDesc.SampleDesc.Quality = 0;
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    ResourceWithAllocation renderTargetRes;
    CHECK_HR( ctx.allocator->CreateResource(
        &allocDesc,
        &resourceDesc,
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        NULL,
        &renderTargetRes.allocation,
        IID_PPV_ARGS(&renderTargetRes.resource)) );
}

static void TestOtherComInterface(const TestContext& ctx)
{
    wprintf(L"Test other COM interface\n");

    D3D12_RESOURCE_DESC resDesc;
    FillResourceDescForBuffer(resDesc, 0x10000);

    for(uint32_t i = 0; i < 2; ++i)
    {
        D3D12MA::ALLOCATION_DESC allocDesc = {};
        allocDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
        if(i == 1)
        {
            allocDesc.Flags = D3D12MA::ALLOCATION_FLAG_COMMITTED;
        }

        ComPtr<D3D12MA::Allocation> alloc;
        ComPtr<ID3D12Pageable> pageable;
        CHECK_HR(ctx.allocator->CreateResource(
            &allocDesc,
            &resDesc,
            D3D12_RESOURCE_STATE_COMMON,
            nullptr, // pOptimizedClearValue
            &alloc,
            IID_PPV_ARGS(&pageable)));

        // Do something with the interface to make sure it's valid.
        ComPtr<ID3D12Device> device;
        CHECK_HR(pageable->GetDevice(IID_PPV_ARGS(&device)));
        CHECK_BOOL(device.Get() == ctx.device);
    }
}

static void TestCustomPools(const TestContext& ctx)
{
    wprintf(L"Test custom pools\n");

    // # Fetch global stats 1

    D3D12MA::TotalStatistics globalStatsBeg = {};
    ctx.allocator->CalculateStatistics(&globalStatsBeg);

    // # Create pool, 1..2 blocks of 11 MB
    
    D3D12MA::POOL_DESC poolDesc = {};
    poolDesc.HeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
    poolDesc.HeapFlags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;
    poolDesc.BlockSize = 11 * MEGABYTE;
    poolDesc.MinBlockCount = 1;
    poolDesc.MaxBlockCount = 2;
    poolDesc.ResidencyPriority = D3D12_RESIDENCY_PRIORITY_HIGH; // Test some residency priority, by the way.

    ComPtr<D3D12MA::Pool> pool;
    CHECK_HR( ctx.allocator->CreatePool(&poolDesc, &pool) );

    // # Validate stats for empty pool

    D3D12MA::DetailedStatistics poolStats = {};
    pool->CalculateStatistics(&poolStats);
    CHECK_BOOL( poolStats.Stats.BlockCount == 1 );
    CHECK_BOOL( poolStats.Stats.AllocationCount == 0 );
    CHECK_BOOL( poolStats.Stats.AllocationBytes == 0 );
    CHECK_BOOL( poolStats.Stats.BlockBytes - poolStats.Stats.AllocationBytes ==
        poolStats.Stats.BlockCount * poolDesc.BlockSize );

    // # SetName and GetName

    static const wchar_t* NAME = L"Custom pool name 1";
    pool->SetName(NAME);
    CHECK_BOOL( wcscmp(pool->GetName(), NAME) == 0 );

    // # Create buffers 2x 5 MB

    D3D12MA::ALLOCATION_DESC allocDesc = {};
    allocDesc.CustomPool = pool.Get();
    allocDesc.ExtraHeapFlags = (D3D12_HEAP_FLAGS)0xCDCDCDCD; // Should be ignored.
    allocDesc.HeapType = (D3D12_HEAP_TYPE)0xCDCDCDCD; // Should be ignored.

    const UINT64 BUFFER_SIZE = 5 * MEGABYTE;
    D3D12_RESOURCE_DESC resDesc;
    FillResourceDescForBuffer(resDesc, BUFFER_SIZE);

    ComPtr<D3D12MA::Allocation> allocs[4];
    for(uint32_t i = 0; i < 2; ++i)
    {
        CHECK_HR( ctx.allocator->CreateResource(&allocDesc, &resDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            NULL, // pOptimizedClearValue
            &allocs[i],
            __uuidof(ID3D12Resource), NULL) ); // riidResource, ppvResource
    }

    // # Validate pool stats now

    pool->CalculateStatistics(&poolStats);
    CHECK_BOOL( poolStats.Stats.BlockCount == 1 );
    CHECK_BOOL( poolStats.Stats.AllocationCount == 2 );
    CHECK_BOOL( poolStats.Stats.AllocationBytes == 2 * BUFFER_SIZE );
    CHECK_BOOL( poolStats.Stats.BlockBytes - poolStats.Stats.AllocationBytes ==
        poolDesc.BlockSize - poolStats.Stats.AllocationBytes );

    // # Check that global stats are updated as well

    D3D12MA::TotalStatistics globalStatsCurr = {};
    ctx.allocator->CalculateStatistics(&globalStatsCurr);

    CHECK_BOOL( globalStatsCurr.Total.Stats.AllocationCount ==
        globalStatsBeg.Total.Stats.AllocationCount + poolStats.Stats.AllocationCount );
    CHECK_BOOL( globalStatsCurr.Total.Stats.BlockCount ==
        globalStatsBeg.Total.Stats.BlockCount + poolStats.Stats.BlockCount );
    CHECK_BOOL( globalStatsCurr.Total.Stats.AllocationBytes ==
        globalStatsBeg.Total.Stats.AllocationBytes + poolStats.Stats.AllocationBytes );

    // # NEVER_ALLOCATE and COMMITTED should fail
    // (Committed allocations not allowed in this pool because BlockSize != 0.)

    for(uint32_t i = 0; i < 2; ++i)
    {
        allocDesc.Flags = i == 0 ?
            D3D12MA::ALLOCATION_FLAG_NEVER_ALLOCATE:
            D3D12MA::ALLOCATION_FLAG_COMMITTED;
        ComPtr<D3D12MA::Allocation> alloc;
        const HRESULT hr = ctx.allocator->CreateResource(&allocDesc, &resDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            NULL, // pOptimizedClearValue
            &alloc,
            __uuidof(ID3D12Resource), NULL); // riidResource, ppvResource
        CHECK_BOOL( FAILED(hr) );
    }

    // # 3 more buffers. 3rd should fail.

    allocDesc.Flags = D3D12MA::ALLOCATION_FLAG_NONE;
    for(uint32_t i = 2; i < 5; ++i)
    {
        ComPtr<D3D12MA::Allocation> alloc;
        HRESULT hr = ctx.allocator->CreateResource(&allocDesc, &resDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            NULL, // pOptimizedClearValue
            &alloc,
            __uuidof(ID3D12Resource), NULL); // riidResource, ppvResource
        if(i < 4)
        {
            CHECK_HR( hr );
            allocs[i] = std::move(alloc);
        }
        else
        {
            CHECK_BOOL( FAILED(hr) );
        }
    }

    pool->CalculateStatistics(&poolStats);
    CHECK_BOOL( poolStats.Stats.BlockCount == 2 );
    CHECK_BOOL( poolStats.Stats.AllocationCount == 4 );
    CHECK_BOOL( poolStats.Stats.AllocationBytes == 4 * BUFFER_SIZE );
    CHECK_BOOL( poolStats.Stats.BlockBytes - poolStats.Stats.AllocationBytes ==
        poolStats.Stats.BlockCount * poolDesc.BlockSize - poolStats.Stats.AllocationBytes );

    // # Make room, AllocateMemory, CreateAliasingResource

    allocs[3].Reset();
    allocs[0].Reset();

    D3D12_RESOURCE_ALLOCATION_INFO resAllocInfo = {};
    resAllocInfo.SizeInBytes = 5 * MEGABYTE;
    resAllocInfo.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;

    CHECK_HR( ctx.allocator->AllocateMemory(&allocDesc, &resAllocInfo, &allocs[0]) );

    resDesc.Width = 1 * MEGABYTE;
    ComPtr<ID3D12Resource> res;
    CHECK_HR( ctx.allocator->CreateAliasingResource(allocs[0].Get(),
        0, // AllocationLocalOffset
        &resDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        NULL, // pOptimizedClearValue
        IID_PPV_ARGS(&res)) );

    // JSON dump
    wchar_t* json = nullptr;
    ctx.allocator->BuildStatsString(&json, TRUE);
    ctx.allocator->FreeStatsString(json);
}

static void TestPoolsAndAllocationParameters(const TestContext& ctx)
{
    wprintf(L"Test pools and allocation parameters\n");

    ComPtr<D3D12MA::Pool> pool1, pool2;
    std::vector<ComPtr<D3D12MA::Allocation>> bufs;

    D3D12MA::ALLOCATION_DESC allocDesc = {};

    uint32_t totalNewAllocCount = 0, totalNewBlockCount = 0;
    D3D12MA::TotalStatistics statsBeg, statsEnd;
    ctx.allocator->CalculateStatistics(&statsBeg);

    HRESULT hr;
    ComPtr<D3D12MA::Allocation> alloc;

    // poolTypeI:
    // 0 = default pool
    // 1 = custom pool, default (flexible) block size and block count
    // 2 = custom pool, fixed block size and limited block count
    for(size_t poolTypeI = 0; poolTypeI < 3; ++poolTypeI)
    {
        if(poolTypeI == 0)
        {
            allocDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
            allocDesc.CustomPool = nullptr;
        }
        else if(poolTypeI == 1)
        {
            D3D12MA::POOL_DESC poolDesc = {};
            poolDesc.HeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
            poolDesc.HeapFlags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;
            hr = ctx.allocator->CreatePool(&poolDesc, &pool1);
            CHECK_HR(hr);
            allocDesc.CustomPool = pool1.Get();
        }
        else if(poolTypeI == 2)
        {
            D3D12MA::POOL_DESC poolDesc = {};
            poolDesc.HeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
            poolDesc.HeapFlags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;
            poolDesc.MaxBlockCount = 1;
            poolDesc.BlockSize = 2 * MEGABYTE + MEGABYTE / 2; // 2.5 MB
            hr = ctx.allocator->CreatePool(&poolDesc, &pool2);
            CHECK_HR(hr);
            allocDesc.CustomPool = pool2.Get();
        }

        uint32_t poolAllocCount = 0, poolBlockCount = 0;
        D3D12_RESOURCE_DESC resDesc;
        FillResourceDescForBuffer(resDesc, MEGABYTE);

        // Default parameters
        allocDesc.Flags = D3D12MA::ALLOCATION_FLAG_NONE;
        hr = ctx.allocator->CreateResource(&allocDesc, &resDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, &alloc, IID_NULL, nullptr);
        CHECK_BOOL(SUCCEEDED(hr) && alloc && alloc->GetResource());
        ID3D12Heap* const defaultAllocHeap = alloc->GetHeap();
        const UINT64 defaultAllocOffset = alloc->GetOffset();
        bufs.push_back(std::move(alloc));
        ++poolAllocCount;

        // COMMITTED. Should not try pool2 as it may assert on invalid call.
        if(poolTypeI != 2)
        {
            allocDesc.Flags = D3D12MA::ALLOCATION_FLAG_COMMITTED;
            hr = ctx.allocator->CreateResource(&allocDesc, &resDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, &alloc, IID_NULL, nullptr);
            CHECK_BOOL(SUCCEEDED(hr) && alloc && alloc->GetResource());
            CHECK_BOOL(alloc->GetOffset() == 0); // Committed
            CHECK_BOOL(alloc->GetHeap() == nullptr); // Committed
            bufs.push_back(std::move(alloc));
            ++poolAllocCount;
        }

        // NEVER_ALLOCATE #1
        allocDesc.Flags = D3D12MA::ALLOCATION_FLAG_NEVER_ALLOCATE;
        hr = ctx.allocator->CreateResource(&allocDesc, &resDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, &alloc, IID_NULL, nullptr);
        CHECK_BOOL(SUCCEEDED(hr) && alloc && alloc->GetResource());
        CHECK_BOOL(alloc->GetHeap() == defaultAllocHeap); // Same memory block as default one.
        CHECK_BOOL(alloc->GetOffset() != defaultAllocOffset);
        bufs.push_back(std::move(alloc));
        ++poolAllocCount;

        // NEVER_ALLOCATE #2. Should fail in pool2 as it has no space.
        allocDesc.Flags = D3D12MA::ALLOCATION_FLAG_NEVER_ALLOCATE;
        hr = ctx.allocator->CreateResource(&allocDesc, &resDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, &alloc, IID_NULL, nullptr);
        if(poolTypeI == 2)
            CHECK_BOOL(FAILED(hr));
        else
        {
            CHECK_BOOL(SUCCEEDED(hr) && alloc && alloc->GetResource());
            bufs.push_back(std::move(alloc));
            ++poolAllocCount;
        }

        // Pool stats
        switch(poolTypeI)
        {
        case 0: poolBlockCount = 1; break; // At least 1 added for dedicated allocation.
        case 1: poolBlockCount = 2; break; // 1 for custom pool block and 1 for dedicated allocation.
        case 2: poolBlockCount = 1; break; // Only custom pool, no dedicated allocation.
        }

        if(poolTypeI > 0)
        {
            D3D12MA::DetailedStatistics poolStats = {};
            (poolTypeI == 2 ? pool2 : pool1)->CalculateStatistics(&poolStats);
            CHECK_BOOL(poolStats.Stats.AllocationCount == poolAllocCount);
            CHECK_BOOL(poolStats.Stats.AllocationBytes == poolAllocCount * MEGABYTE);
            CHECK_BOOL(poolStats.Stats.BlockCount == poolBlockCount);
        }

        totalNewAllocCount += poolAllocCount;
        totalNewBlockCount += poolBlockCount;
    }

    ctx.allocator->CalculateStatistics(&statsEnd);

    CHECK_BOOL(statsEnd.Total.Stats.AllocationCount ==
        statsBeg.Total.Stats.AllocationCount + totalNewAllocCount);
    CHECK_BOOL(statsEnd.Total.Stats.BlockCount >=
        statsBeg.Total.Stats.BlockCount + totalNewBlockCount);
    CHECK_BOOL(statsEnd.Total.Stats.AllocationBytes ==
        statsBeg.Total.Stats.AllocationBytes + totalNewAllocCount * MEGABYTE);
}

static void TestCustomPool_MinAllocationAlignment(const TestContext& ctx)
{
    wprintf(L"Test custom pool MinAllocationAlignment\n");

    const UINT64 BUFFER_SIZE = 32;
    constexpr size_t BUFFER_COUNT = 4;
    const UINT64 MIN_ALIGNMENT = 128 * 1024;

    D3D12MA::POOL_DESC poolDesc = {};
    poolDesc.HeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
    poolDesc.HeapFlags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;
    poolDesc.MinAllocationAlignment = MIN_ALIGNMENT;

    ComPtr<D3D12MA::Pool> pool;
    CHECK_HR( ctx.allocator->CreatePool(&poolDesc, &pool) );

    D3D12MA::ALLOCATION_DESC allocDesc = {};
    allocDesc.CustomPool = pool.Get();

    D3D12_RESOURCE_DESC resDesc;
    FillResourceDescForBuffer(resDesc, BUFFER_SIZE);

    ComPtr<D3D12MA::Allocation> allocs[BUFFER_COUNT];
    for(size_t i = 0; i < BUFFER_COUNT; ++i)
    {
        CHECK_HR( ctx.allocator->CreateResource(&allocDesc, &resDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            NULL, // pOptimizedClearValue
            &allocs[i],
            IID_NULL, NULL) ); // riidResource, ppvResource
        CHECK_BOOL(allocs[i]->GetOffset() % MIN_ALIGNMENT == 0);
    }
}

static void TestCustomPool_Committed(const TestContext& ctx)
{
    wprintf(L"Test custom pool committed\n");

    const UINT64 BUFFER_SIZE = 32;

    D3D12MA::POOL_DESC poolDesc = {};
    poolDesc.HeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
    poolDesc.HeapFlags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;

    ComPtr<D3D12MA::Pool> pool;
    CHECK_HR( ctx.allocator->CreatePool(&poolDesc, &pool) );

    D3D12MA::ALLOCATION_DESC allocDesc = {};
    allocDesc.CustomPool = pool.Get();
    allocDesc.Flags = D3D12MA::ALLOCATION_FLAG_COMMITTED;

    D3D12_RESOURCE_DESC resDesc;
    FillResourceDescForBuffer(resDesc, BUFFER_SIZE);

    ComPtr<D3D12MA::Allocation> alloc;
    CHECK_HR( ctx.allocator->CreateResource(&allocDesc, &resDesc,
        D3D12_RESOURCE_STATE_COMMON,
        NULL, // pOptimizedClearValue
        &alloc,
        IID_NULL, NULL) ); // riidResource, ppvResource
    CHECK_BOOL(alloc->GetHeap() == NULL);
    CHECK_BOOL(alloc->GetResource() != NULL);
    CHECK_BOOL(alloc->GetOffset() == 0);
}

static HRESULT TestCustomHeap(const TestContext& ctx, const D3D12_HEAP_PROPERTIES& heapProps)
{
    D3D12MA::TotalStatistics globalStatsBeg = {};
    ctx.allocator->CalculateStatistics(&globalStatsBeg);

    D3D12MA::POOL_DESC poolDesc = {};
    poolDesc.HeapProperties = heapProps;
    poolDesc.HeapFlags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;
    poolDesc.BlockSize = 10 * MEGABYTE;
    poolDesc.MinBlockCount = 1;
    poolDesc.MaxBlockCount = 1;

    const UINT64 BUFFER_SIZE = 1 * MEGABYTE;

    ComPtr<D3D12MA::Pool> pool;
    HRESULT hr = ctx.allocator->CreatePool(&poolDesc, &pool);
    if(SUCCEEDED(hr))
    {
        D3D12MA::ALLOCATION_DESC allocDesc = {};
        allocDesc.CustomPool = pool.Get();

        D3D12_RESOURCE_DESC resDesc;
        FillResourceDescForBuffer(resDesc, BUFFER_SIZE);

        // Pool already allocated a block. We don't expect CreatePlacedResource to fail.
        ComPtr<D3D12MA::Allocation> alloc;
        CHECK_HR( ctx.allocator->CreateResource(&allocDesc, &resDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            NULL, // pOptimizedClearValue
            &alloc,
            __uuidof(ID3D12Resource), NULL) ); // riidResource, ppvResource

        D3D12MA::TotalStatistics globalStatsCurr = {};
        ctx.allocator->CalculateStatistics(&globalStatsCurr);

        // Make sure it is accounted only in CUSTOM heap not any of the standard heaps.
        CHECK_BOOL(memcmp(&globalStatsCurr.HeapType[0], &globalStatsBeg.HeapType[0], sizeof(D3D12MA::DetailedStatistics)) == 0);
        CHECK_BOOL(memcmp(&globalStatsCurr.HeapType[1], &globalStatsBeg.HeapType[1], sizeof(D3D12MA::DetailedStatistics)) == 0);
        CHECK_BOOL(memcmp(&globalStatsCurr.HeapType[2], &globalStatsBeg.HeapType[2], sizeof(D3D12MA::DetailedStatistics)) == 0);
        CHECK_BOOL( globalStatsCurr.HeapType[3].Stats.AllocationCount == globalStatsBeg.HeapType[3].Stats.AllocationCount + 1 );
        CHECK_BOOL( globalStatsCurr.HeapType[3].Stats.BlockCount == globalStatsBeg.HeapType[3].Stats.BlockCount + 1 );
        CHECK_BOOL( globalStatsCurr.HeapType[3].Stats.AllocationBytes == globalStatsBeg.HeapType[3].Stats.AllocationBytes + BUFFER_SIZE );
        CHECK_BOOL( globalStatsCurr.Total.Stats.AllocationCount == globalStatsBeg.Total.Stats.AllocationCount + 1 );
        CHECK_BOOL( globalStatsCurr.Total.Stats.BlockCount == globalStatsBeg.Total.Stats.BlockCount + 1 );
        CHECK_BOOL( globalStatsCurr.Total.Stats.AllocationBytes == globalStatsBeg.Total.Stats.AllocationBytes + BUFFER_SIZE );

        // Map and write some data.
        if(heapProps.CPUPageProperty == D3D12_CPU_PAGE_PROPERTY_WRITE_COMBINE ||
            heapProps.CPUPageProperty == D3D12_CPU_PAGE_PROPERTY_WRITE_BACK)
        {
            ID3D12Resource* const res = alloc->GetResource();

            UINT* mappedPtr = nullptr;
            const D3D12_RANGE readRange = {0, 0};
            CHECK_HR(res->Map(0, &readRange, (void**)&mappedPtr));
            
            *mappedPtr = 0xDEADC0DE;
            
            res->Unmap(0, nullptr);
        }
    }

    return hr;
}

static void TestCustomHeaps(const TestContext& ctx)
{
    wprintf(L"Test custom heap\n");

    D3D12_HEAP_PROPERTIES heapProps = {};

    // Use custom pool but the same as READBACK, which should be always available.
    heapProps.Type = D3D12_HEAP_TYPE_CUSTOM;
    heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
    heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_L0; // System memory
    HRESULT hr = TestCustomHeap(ctx, heapProps);
    CHECK_HR(hr);
}

static void TestStandardCustomCommittedPlaced(const TestContext& ctx)
{
    wprintf(L"Test standard, custom, committed, placed\n");

    static const D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_DEFAULT;
    static const UINT64 bufferSize = 1024;

    D3D12MA::POOL_DESC poolDesc = {};
    poolDesc.HeapProperties.Type = heapType;
    poolDesc.HeapFlags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;

    ComPtr<D3D12MA::Pool> pool;
    CHECK_HR(ctx.allocator->CreatePool(&poolDesc, &pool));

    std::vector<ComPtr<D3D12MA::Allocation>> allocations;
    
    D3D12MA::TotalStatistics statsBeg = {};
    D3D12MA::DetailedStatistics poolStatInfoBeg = {};
    ctx.allocator->CalculateStatistics(&statsBeg);
    pool->CalculateStatistics(&poolStatInfoBeg);

    size_t poolAllocCount = 0;

    D3D12_RESOURCE_DESC resDesc = {};
    FillResourceDescForBuffer(resDesc, bufferSize);

    for(uint32_t standardCustomI = 0; standardCustomI < 2; ++standardCustomI)
    {
        const bool useCustomPool = standardCustomI > 0;
        for(uint32_t flagsI = 0; flagsI < 3; ++flagsI)
        {
            const bool useCommitted = flagsI > 0;
            const bool neverAllocate = flagsI > 1;

            D3D12MA::ALLOCATION_DESC allocDesc = {};
            if(useCustomPool)
            {
                allocDesc.CustomPool = pool.Get();
                allocDesc.HeapType = (D3D12_HEAP_TYPE)0xCDCDCDCD; // Should be ignored.
                allocDesc.ExtraHeapFlags = (D3D12_HEAP_FLAGS)0xCDCDCDCD; // Should be ignored.
            }
            else
                allocDesc.HeapType = heapType;
            if(useCommitted)
                allocDesc.Flags |= D3D12MA::ALLOCATION_FLAG_COMMITTED;
            if(neverAllocate)
                allocDesc.Flags |= D3D12MA::ALLOCATION_FLAG_NEVER_ALLOCATE;

            ComPtr<D3D12MA::Allocation> allocPtr = NULL;
            HRESULT hr = ctx.allocator->CreateResource(&allocDesc, &resDesc,
                D3D12_RESOURCE_STATE_COMMON,
                NULL, // pOptimizedClearValue
                &allocPtr, IID_NULL, NULL);
            CHECK_BOOL(SUCCEEDED(hr) == (allocPtr != NULL));
            if(allocPtr)
            {
                allocations.push_back(allocPtr);
                if(useCustomPool)
                    ++poolAllocCount;
            }

            bool expectSuccess = !neverAllocate; // NEVER_ALLOCATE should always fail with COMMITTED.
            CHECK_BOOL(expectSuccess == SUCCEEDED(hr));
            if(SUCCEEDED(hr) && useCommitted)
            {
                CHECK_BOOL(allocPtr->GetHeap() == NULL); // Committed allocation has implicit heap.
            }
        }
    }

    D3D12MA::TotalStatistics statsEnd = {};
    D3D12MA::DetailedStatistics poolStatInfoEnd = {};
    ctx.allocator->CalculateStatistics(&statsEnd);
    pool->CalculateStatistics(&poolStatInfoEnd);

    CHECK_BOOL(statsEnd.Total.Stats.AllocationCount == statsBeg.Total.Stats.AllocationCount + allocations.size());
    CHECK_BOOL(statsEnd.Total.Stats.AllocationBytes >= statsBeg.Total.Stats.AllocationBytes + allocations.size() * bufferSize);
    CHECK_BOOL(statsEnd.HeapType[0].Stats.AllocationCount == statsBeg.HeapType[0].Stats.AllocationCount + allocations.size());
    CHECK_BOOL(statsEnd.HeapType[0].Stats.AllocationBytes >= statsBeg.HeapType[0].Stats.AllocationBytes + allocations.size() * bufferSize);
    CHECK_BOOL(poolStatInfoEnd.Stats.AllocationCount == poolStatInfoBeg.Stats.AllocationCount + poolAllocCount);
    CHECK_BOOL(poolStatInfoEnd.Stats.AllocationBytes >= poolStatInfoBeg.Stats.AllocationBytes + poolAllocCount * bufferSize);
}

static void TestAliasingMemory(const TestContext& ctx)
{
    wprintf(L"Test aliasing memory\n");

    D3D12_RESOURCE_DESC resDesc1 = {};
    resDesc1.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resDesc1.Alignment = 0;
    resDesc1.Width = 1920;
    resDesc1.Height = 1080;
    resDesc1.DepthOrArraySize = 1;
    resDesc1.MipLevels = 1;
    resDesc1.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    resDesc1.SampleDesc.Count = 1;
    resDesc1.SampleDesc.Quality = 0;
    resDesc1.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    resDesc1.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    D3D12_RESOURCE_DESC resDesc2 = {};
    resDesc2.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resDesc2.Alignment = 0;
    resDesc2.Width = 1024;
    resDesc2.Height = 1024;
    resDesc2.DepthOrArraySize = 1;
    resDesc2.MipLevels = 0;
    resDesc2.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    resDesc2.SampleDesc.Count = 1;
    resDesc2.SampleDesc.Quality = 0;
    resDesc2.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    resDesc2.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

    const D3D12_RESOURCE_ALLOCATION_INFO allocInfo1 =
        ctx.device->GetResourceAllocationInfo(0, 1, &resDesc1);
    const D3D12_RESOURCE_ALLOCATION_INFO allocInfo2 =
        ctx.device->GetResourceAllocationInfo(0, 1, &resDesc2);

    D3D12_RESOURCE_ALLOCATION_INFO finalAllocInfo = {};
    finalAllocInfo.Alignment = std::max(allocInfo1.Alignment, allocInfo2.Alignment);
    finalAllocInfo.SizeInBytes = std::max(allocInfo1.SizeInBytes, allocInfo2.SizeInBytes);

    D3D12MA::ALLOCATION_DESC allocDesc = {};
    allocDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
    allocDesc.ExtraHeapFlags = D3D12_HEAP_FLAG_ALLOW_ONLY_RT_DS_TEXTURES;

    ComPtr<D3D12MA::Allocation> alloc;
    CHECK_HR( ctx.allocator->AllocateMemory(&allocDesc, &finalAllocInfo, &alloc) );
    CHECK_BOOL(alloc != NULL && alloc->GetHeap() != NULL);

    ComPtr<ID3D12Resource> res1;
    CHECK_HR( ctx.allocator->CreateAliasingResource(
        alloc.Get(),
        0, // AllocationLocalOffset
        &resDesc1,
        D3D12_RESOURCE_STATE_COMMON,
        NULL, // pOptimizedClearValue
        IID_PPV_ARGS(&res1)) );
    CHECK_BOOL(res1 != NULL);

    ComPtr<ID3D12Resource> res2;
    CHECK_HR( ctx.allocator->CreateAliasingResource(
        alloc.Get(),
        0, // AllocationLocalOffset
        &resDesc2,
        D3D12_RESOURCE_STATE_COMMON,
        NULL, // pOptimizedClearValue
        IID_PPV_ARGS(&res2)) );
    CHECK_BOOL(res2 != NULL);

    // You can use res1 and res2, but not at the same time!
}

static void TestAliasingImplicitCommitted(const TestContext& ctx)
{
    wprintf(L"Test aliasing implicit dedicated\n");

    // The buffer will be large enough to be allocated as committed.
    // We still need it to have an explicit heap to be able to alias.

    D3D12_RESOURCE_DESC resDesc = {};
    FillResourceDescForBuffer(resDesc, 300 * MEGABYTE);

    D3D12MA::ALLOCATION_DESC allocDesc = {};
    allocDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;
    allocDesc.Flags = D3D12MA::ALLOCATION_FLAG_CAN_ALIAS;

    ComPtr<D3D12MA::Allocation> alloc;
    CHECK_HR(ctx.allocator->CreateResource(&allocDesc, &resDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, NULL,
        &alloc, IID_NULL, NULL));
    CHECK_BOOL(alloc != NULL && alloc->GetHeap() != NULL);

    resDesc.Width = 200 * MEGABYTE;
    ComPtr<ID3D12Resource> aliasingRes;
    CHECK_HR(ctx.allocator->CreateAliasingResource(alloc.Get(),
        0, // AllocationLocalOffset
        &resDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, NULL, IID_PPV_ARGS(&aliasingRes)));
    CHECK_BOOL(aliasingRes != NULL);
}

static void TestPoolMsaaTextureAsCommitted(const TestContext& ctx)
{
    wprintf(L"Test MSAA texture always as committed in pool\n");

    D3D12MA::POOL_DESC poolDesc = {};
    poolDesc.HeapFlags = D3D12_HEAP_FLAG_ALLOW_ONLY_RT_DS_TEXTURES;
    poolDesc.HeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
    poolDesc.Flags = D3D12MA::POOL_FLAG_MSAA_TEXTURES_ALWAYS_COMMITTED;

    ComPtr<D3D12MA::Pool> pool;
    CHECK_HR(ctx.allocator->CreatePool(&poolDesc, &pool));

    D3D12MA::ALLOCATION_DESC allocDesc = {};
    allocDesc.CustomPool = pool.Get();

    D3D12_RESOURCE_DESC resDesc = {};
    resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resDesc.Width = 1024;
    resDesc.Height = 512;
    resDesc.DepthOrArraySize = 1;
    resDesc.MipLevels = 1;
    resDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    resDesc.SampleDesc.Count = 2;
    resDesc.SampleDesc.Quality = 0;
    resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

    ComPtr<D3D12MA::Allocation> alloc;
    CHECK_HR(ctx.allocator->CreateResource(&allocDesc, &resDesc, D3D12_RESOURCE_STATE_RENDER_TARGET, nullptr, &alloc, IID_NULL, nullptr));
    // Committed allocation should not have explicit heap
    CHECK_BOOL(alloc->GetHeap() == nullptr);
}

static void TestMapping(const TestContext& ctx)
{
    wprintf(L"Test mapping\n");

    const UINT count = 10;
    const UINT64 bufSize = 32ull * 1024;
    ResourceWithAllocation resources[count];

    D3D12MA::ALLOCATION_DESC allocDesc = {};
    allocDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC resourceDesc;
    FillResourceDescForBuffer(resourceDesc, bufSize);

    for(UINT i = 0; i < count; ++i)
    {
        CHECK_HR( ctx.allocator->CreateResource(
            &allocDesc,
            &resourceDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            NULL,
            &resources[i].allocation,
            IID_PPV_ARGS(&resources[i].resource)) );

        void* mappedPtr = NULL;
        CHECK_HR( resources[i].resource->Map(0, &EMPTY_RANGE, &mappedPtr) );

        FillData(mappedPtr, bufSize, i);

        // Unmap every other buffer. Leave others mapped.
        if((i % 2) != 0)
        {
            resources[i].resource->Unmap(0, NULL);
        }
    }
}

static inline bool StatisticsEqual(const D3D12MA::DetailedStatistics& lhs, const D3D12MA::DetailedStatistics& rhs)
{
    return memcmp(&lhs, &rhs, sizeof(lhs)) == 0;
}

static void CheckStatistics(const D3D12MA::DetailedStatistics& stats)
{
    CHECK_BOOL(stats.Stats.AllocationBytes <= stats.Stats.BlockBytes);
    if(stats.Stats.AllocationBytes > 0)
    {
        CHECK_BOOL(stats.Stats.AllocationCount > 0);
        CHECK_BOOL(stats.AllocationSizeMin <= stats.AllocationSizeMax);
    }
    if(stats.UnusedRangeCount > 0)
    {
        CHECK_BOOL(stats.UnusedRangeSizeMax > 0);
        CHECK_BOOL(stats.UnusedRangeSizeMin <= stats.UnusedRangeSizeMax);
    }
}

static void TestStats(const TestContext& ctx)
{
    wprintf(L"Test stats\n");

    D3D12MA::TotalStatistics begStats = {};
    ctx.allocator->CalculateStatistics(&begStats);

    const UINT count = 10;
    const UINT64 bufSize = 64ull * 1024;
    ResourceWithAllocation resources[count];

    D3D12MA::ALLOCATION_DESC allocDesc = {};
    allocDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC resourceDesc;
    FillResourceDescForBuffer(resourceDesc, bufSize);

    for(UINT i = 0; i < count; ++i)
    {
        if(i == count / 2)
            allocDesc.Flags |= D3D12MA::ALLOCATION_FLAG_COMMITTED;
        CHECK_HR( ctx.allocator->CreateResource(
            &allocDesc,
            &resourceDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            NULL,
            &resources[i].allocation,
            IID_PPV_ARGS(&resources[i].resource)) );
    }

    D3D12MA::TotalStatistics endStats = {};
    ctx.allocator->CalculateStatistics(&endStats);

    CHECK_BOOL(endStats.Total.Stats.BlockCount >= begStats.Total.Stats.BlockCount);
    CHECK_BOOL(endStats.Total.Stats.AllocationCount == begStats.Total.Stats.AllocationCount + count);
    CHECK_BOOL(endStats.Total.Stats.AllocationBytes == begStats.Total.Stats.AllocationBytes + count * bufSize);
    CHECK_BOOL(endStats.Total.AllocationSizeMin <= bufSize);
    CHECK_BOOL(endStats.Total.AllocationSizeMax >= bufSize);

    CHECK_BOOL(endStats.HeapType[1].Stats.BlockCount >= begStats.HeapType[1].Stats.BlockCount);
    CHECK_BOOL(endStats.HeapType[1].Stats.AllocationCount >= begStats.HeapType[1].Stats.AllocationCount + count);
    CHECK_BOOL(endStats.HeapType[1].Stats.AllocationBytes >= begStats.HeapType[1].Stats.AllocationBytes + count * bufSize);
    CHECK_BOOL(endStats.HeapType[1].AllocationSizeMin <= bufSize);
    CHECK_BOOL(endStats.HeapType[1].AllocationSizeMax >= bufSize);

    CHECK_BOOL(StatisticsEqual(begStats.HeapType[0], endStats.HeapType[0]));
    CHECK_BOOL(StatisticsEqual(begStats.HeapType[2], endStats.HeapType[2]));

    CheckStatistics(endStats.Total);
    CheckStatistics(endStats.HeapType[0]);
    CheckStatistics(endStats.HeapType[1]);
    CheckStatistics(endStats.HeapType[2]);

    D3D12MA::Budget localBudget = {}, nonLocalBudget = {};
    ctx.allocator->GetBudget(&localBudget, &nonLocalBudget);

    CHECK_BOOL(localBudget.Stats.AllocationBytes <= localBudget.Stats.BlockBytes);
    CHECK_BOOL(endStats.HeapType[3].Stats.BlockCount == 0); // No allocation from D3D12_HEAP_TYPE_CUSTOM in this test.
    if(!ctx.allocator->IsUMA())
    {
        // Discrete GPU
        CHECK_BOOL(localBudget.Stats.AllocationBytes == endStats.HeapType[0].Stats.AllocationBytes);
        CHECK_BOOL(localBudget.Stats.BlockBytes == endStats.HeapType[0].Stats.BlockBytes);
    
        CHECK_BOOL(nonLocalBudget.Stats.AllocationBytes <= nonLocalBudget.Stats.BlockBytes);
        CHECK_BOOL(nonLocalBudget.Stats.AllocationBytes == endStats.HeapType[1].Stats.AllocationBytes + endStats.HeapType[2].Stats.AllocationBytes);
        CHECK_BOOL(nonLocalBudget.Stats.BlockBytes ==
            endStats.HeapType[1].Stats.BlockBytes + endStats.HeapType[2].Stats.BlockBytes);
    }
    else
    {
        // Integrated GPU - all memory is local
        CHECK_BOOL(localBudget.Stats.AllocationBytes == endStats.HeapType[0].Stats.AllocationBytes +
            endStats.HeapType[1].Stats.AllocationBytes +
            endStats.HeapType[2].Stats.AllocationBytes);
        CHECK_BOOL(localBudget.Stats.BlockBytes == endStats.HeapType[0].Stats.BlockBytes +
            endStats.HeapType[1].Stats.BlockBytes +
            endStats.HeapType[2].Stats.BlockBytes);

        CHECK_BOOL(nonLocalBudget.Stats.AllocationBytes == 0);
        CHECK_BOOL(nonLocalBudget.Stats.BlockBytes == 0);
    }
}

static void TestTransfer(const TestContext& ctx)
{
    wprintf(L"Test mapping\n");

    const UINT count = 10;
    const UINT64 bufSize = 32ull * 1024;
    
    ResourceWithAllocation resourcesUpload[count];
    ResourceWithAllocation resourcesDefault[count];
    ResourceWithAllocation resourcesReadback[count];

    D3D12MA::ALLOCATION_DESC allocDescUpload = {};
    allocDescUpload.HeapType = D3D12_HEAP_TYPE_UPLOAD;
    D3D12MA::ALLOCATION_DESC allocDescDefault = {};
    allocDescDefault.HeapType = D3D12_HEAP_TYPE_DEFAULT;
    D3D12MA::ALLOCATION_DESC allocDescReadback = {};
    allocDescReadback.HeapType = D3D12_HEAP_TYPE_READBACK;

    D3D12_RESOURCE_DESC resourceDesc;
    FillResourceDescForBuffer(resourceDesc, bufSize);

    // Create 3 sets of resources.
    for(UINT i = 0; i < count; ++i)
    {
        CHECK_HR( ctx.allocator->CreateResource(
            &allocDescUpload,
            &resourceDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            NULL,
            &resourcesUpload[i].allocation,
            IID_PPV_ARGS(&resourcesUpload[i].resource)) );

        CHECK_HR( ctx.allocator->CreateResource(
            &allocDescDefault,
            &resourceDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            NULL,
            &resourcesDefault[i].allocation,
            IID_PPV_ARGS(&resourcesDefault[i].resource)) );

        CHECK_HR( ctx.allocator->CreateResource(
            &allocDescReadback,
            &resourceDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            NULL,
            &resourcesReadback[i].allocation,
            IID_PPV_ARGS(&resourcesReadback[i].resource)) );
    }

    // Map and fill data in UPLOAD.
    for(UINT i = 0; i < count; ++i)
    {
        void* mappedPtr = nullptr;
        CHECK_HR( resourcesUpload[i].resource->Map(0, &EMPTY_RANGE, &mappedPtr) );

        FillData(mappedPtr, bufSize, i);

        // Unmap every other resource, leave others mapped.
        if((i % 2) != 0)
        {
            resourcesUpload[i].resource->Unmap(0, NULL);
        }
    }

    // Transfer from UPLOAD to DEFAULT, from there to READBACK.
    ID3D12GraphicsCommandList* cmdList = BeginCommandList();
    for(UINT i = 0; i < count; ++i)
    {
        cmdList->CopyBufferRegion(resourcesDefault[i].resource.Get(), 0, resourcesUpload[i].resource.Get(), 0, bufSize);
    }
    D3D12_RESOURCE_BARRIER barriers[count] = {};
    for(UINT i = 0; i < count; ++i)
    {
        barriers[i].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barriers[i].Transition.pResource = resourcesDefault[i].resource.Get();
        barriers[i].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        barriers[i].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
        barriers[i].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    }
    cmdList->ResourceBarrier(count, barriers);
    for(UINT i = 0; i < count; ++i)
    {
        cmdList->CopyBufferRegion(resourcesReadback[i].resource.Get(), 0, resourcesDefault[i].resource.Get(), 0, bufSize);
    }
    EndCommandList(cmdList);

    // Validate READBACK buffers.
    for(UINT i = count; i--; )
    {
        const D3D12_RANGE mapRange = {0, bufSize};
        void* mappedPtr = nullptr;
        CHECK_HR( resourcesReadback[i].resource->Map(0, &mapRange, &mappedPtr) );

        CHECK_BOOL( ValidateData(mappedPtr, bufSize, i) );

        // Unmap every 3rd resource, leave others mapped.
        if((i % 3) != 0)
        {
            resourcesReadback[i].resource->Unmap(0, &EMPTY_RANGE);
        }
    }
}

static void TestZeroInitialized(const TestContext& ctx)
{
    wprintf(L"Test zero initialized\n");

    const UINT64 bufSize = 128ull * 1024;

    D3D12_RESOURCE_DESC resourceDesc;
    FillResourceDescForBuffer(resourceDesc, bufSize);

    // # Create upload buffer and fill it with data.

    D3D12MA::ALLOCATION_DESC allocDescUpload = {};
    allocDescUpload.HeapType = D3D12_HEAP_TYPE_UPLOAD;

    ResourceWithAllocation bufUpload;
    CHECK_HR( ctx.allocator->CreateResource(
        &allocDescUpload,
        &resourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        NULL,
        &bufUpload.allocation,
        IID_PPV_ARGS(&bufUpload.resource)) );

    {
        void* mappedPtr = nullptr;
        CHECK_HR( bufUpload.resource->Map(0, &EMPTY_RANGE, &mappedPtr) );
        FillData(mappedPtr, bufSize, 5236245);
        bufUpload.resource->Unmap(0, NULL);
    }

    // # Create readback buffer
    
    D3D12MA::ALLOCATION_DESC allocDescReadback = {};
    allocDescReadback.HeapType = D3D12_HEAP_TYPE_READBACK;

    ResourceWithAllocation bufReadback;
    CHECK_HR( ctx.allocator->CreateResource(
        &allocDescReadback,
        &resourceDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        NULL,
        &bufReadback.allocation,
        IID_PPV_ARGS(&bufReadback.resource)) );

    auto CheckBufferData = [&](const ResourceWithAllocation& buf)
    {
        const bool shouldBeZero = buf.allocation->WasZeroInitialized() != FALSE;

        {
            ID3D12GraphicsCommandList* cmdList = BeginCommandList();
            cmdList->CopyBufferRegion(bufReadback.resource.Get(), 0, buf.resource.Get(), 0, bufSize);
            EndCommandList(cmdList);
        }

        bool isZero = false;
        {
            const D3D12_RANGE readRange{0, bufSize}; // I could pass pReadRange = NULL but it generates D3D Debug layer warning: EXECUTION WARNING #930: MAP_INVALID_NULLRANGE
            void* mappedPtr = nullptr;
            CHECK_HR( bufReadback.resource->Map(0, &readRange, &mappedPtr) );
            isZero = ValidateDataZero(mappedPtr, bufSize);
            bufReadback.resource->Unmap(0, &EMPTY_RANGE);
        }

        wprintf(L"Should be zero: %u, is zero: %u\n", shouldBeZero ? 1 : 0, isZero ? 1 : 0);

        if(shouldBeZero)
        {
            CHECK_BOOL(isZero);
        }
    };

    // # Test 1: Committed resource. Should always be zero initialized.

    {
        D3D12MA::ALLOCATION_DESC allocDescDefault = {};
        allocDescDefault.HeapType = D3D12_HEAP_TYPE_DEFAULT;
        allocDescDefault.Flags = D3D12MA::ALLOCATION_FLAG_COMMITTED;

        ResourceWithAllocation bufDefault;
        CHECK_HR( ctx.allocator->CreateResource(
            &allocDescDefault,
            &resourceDesc,
            D3D12_RESOURCE_STATE_COPY_SOURCE,
            NULL,
            &bufDefault.allocation,
            IID_PPV_ARGS(&bufDefault.resource)) );

        wprintf(L"  Committed: ");
        CheckBufferData(bufDefault);
        CHECK_BOOL( bufDefault.allocation->WasZeroInitialized() );
    }

    // # Test 2: (Probably) placed resource.

    ResourceWithAllocation bufDefault;
    for(uint32_t i = 0; i < 2; ++i)
    {
        // 1. Create buffer

        D3D12MA::ALLOCATION_DESC allocDescDefault = {};
        allocDescDefault.HeapType = D3D12_HEAP_TYPE_DEFAULT;

        CHECK_HR( ctx.allocator->CreateResource(
            &allocDescDefault,
            &resourceDesc,
            D3D12_RESOURCE_STATE_COPY_SOURCE,
            NULL,
            &bufDefault.allocation,
            IID_PPV_ARGS(&bufDefault.resource)) );

        // 2. Check it

        wprintf(L"  Normal #%u: ", i);
        CheckBufferData(bufDefault);

        // 3. Upload some data to it

        {
            ID3D12GraphicsCommandList* cmdList = BeginCommandList();

            D3D12_RESOURCE_BARRIER barrier = {};
            barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrier.Transition.pResource = bufDefault.resource.Get();
            barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
            barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
            barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            cmdList->ResourceBarrier(1, &barrier);

            cmdList->CopyBufferRegion(bufDefault.resource.Get(), 0, bufUpload.resource.Get(), 0, bufSize);
            
            EndCommandList(cmdList);
        }

        // 4. Delete it

        bufDefault.Reset();
    }
}

static void TestMultithreading(const TestContext& ctx)
{
    wprintf(L"Test multithreading\n");

    const UINT threadCount = 32;
    const UINT bufSizeMin = 1024ull;
    const UINT bufSizeMax = 1024ull * 1024;

    D3D12MA::ALLOCATION_DESC allocDesc = {};
    allocDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;

    // Launch threads.
    std::thread threads[threadCount];
    for(UINT threadIndex = 0; threadIndex < threadCount; ++threadIndex)
    {
        auto threadFunc = [&, threadIndex]()
        {
            RandomNumberGenerator rand(threadIndex);

            std::vector<ResourceWithAllocation> resources;
            resources.reserve(256);

            // Create starting number of buffers.
            const UINT bufToCreateCount = 32;
            for(UINT bufIndex = 0; bufIndex < bufToCreateCount; ++bufIndex)
            {
                ResourceWithAllocation res = {};
                res.dataSeed = (threadIndex << 16) | bufIndex;
                res.size = AlignUp<UINT>(rand.Generate() % (bufSizeMax - bufSizeMin) + bufSizeMin, 16);

                D3D12_RESOURCE_DESC resourceDesc;
                FillResourceDescForBuffer(resourceDesc, res.size);

                CHECK_HR( ctx.allocator->CreateResource(
                    &allocDesc,
                    &resourceDesc,
                    D3D12_RESOURCE_STATE_GENERIC_READ,
                    NULL,
                    &res.allocation,
                    IID_PPV_ARGS(&res.resource)) );
                
                void* mappedPtr = nullptr;
                CHECK_HR( res.resource->Map(0, &EMPTY_RANGE, &mappedPtr) );

                FillData(mappedPtr, res.size, res.dataSeed);

                // Unmap some of them, leave others mapped.
                if(rand.GenerateBool())
                {
                    res.resource->Unmap(0, NULL);
                }

                resources.push_back(std::move(res));
            }
            
            Sleep(20);

            // Make a number of random allocate and free operations.
            const UINT operationCount = 128;
            for(UINT operationIndex = 0; operationIndex < operationCount; ++operationIndex)
            {
                const bool removePossible = !resources.empty();
                const bool remove = removePossible && rand.GenerateBool();
                if(remove)
                {
                    const UINT indexToRemove = rand.Generate() % resources.size();
                    resources.erase(resources.begin() + indexToRemove);
                }
                else // Create new buffer.
                {
                    ResourceWithAllocation res = {};
                    res.dataSeed = (threadIndex << 16) | operationIndex;
                    res.size = AlignUp<UINT>(rand.Generate() % (bufSizeMax - bufSizeMin) + bufSizeMin, 16);
                    D3D12_RESOURCE_DESC resourceDesc;
                    FillResourceDescForBuffer(resourceDesc, res.size);

                    CHECK_HR( ctx.allocator->CreateResource(
                        &allocDesc,
                        &resourceDesc,
                        D3D12_RESOURCE_STATE_GENERIC_READ,
                        NULL,
                        &res.allocation,
                        IID_PPV_ARGS(&res.resource)) );

                    void* mappedPtr = nullptr;
                    CHECK_HR( res.resource->Map(0, NULL, &mappedPtr) );

                    FillData(mappedPtr, res.size, res.dataSeed);

                    // Unmap some of them, leave others mapped.
                    if(rand.GenerateBool())
                    {
                        res.resource->Unmap(0, NULL);
                    }

                    resources.push_back(std::move(res));
                }
            }

            Sleep(20);

            // Validate data in all remaining buffers while deleting them.
            for(size_t resIndex = resources.size(); resIndex--; )
            {
                void* mappedPtr = nullptr;
                CHECK_HR( resources[resIndex].resource->Map(0, NULL, &mappedPtr) );

                ValidateData(mappedPtr, resources[resIndex].size, resources[resIndex].dataSeed);

                // Unmap some of them, leave others mapped.
                if((resIndex % 3) == 1)
                {
                    resources[resIndex].resource->Unmap(0, &EMPTY_RANGE);
                } 

                resources.pop_back();
            }
        };
        threads[threadIndex] = std::thread(threadFunc);
    }

    // Wait for threads to finish.
    for(UINT threadIndex = threadCount; threadIndex--; )
    {
        threads[threadIndex].join();
    }
}

static bool IsProtectedResourceSessionSupported(const TestContext& ctx)
{
    D3D12_FEATURE_DATA_PROTECTED_RESOURCE_SESSION_SUPPORT support = {};
    CHECK_HR(ctx.device->CheckFeatureSupport(
        D3D12_FEATURE_PROTECTED_RESOURCE_SESSION_SUPPORT, &support, sizeof support));
    return support.Support > D3D12_PROTECTED_RESOURCE_SESSION_SUPPORT_FLAG_NONE;
}

static void TestLinearAllocator(const TestContext& ctx)
{
    wprintf(L"Test linear allocator\n");

    RandomNumberGenerator rand{ 645332 };

    D3D12MA::POOL_DESC poolDesc = {};
    poolDesc.HeapFlags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;
    poolDesc.HeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
    poolDesc.Flags = D3D12MA::POOL_FLAG_ALGORITHM_LINEAR;
    poolDesc.BlockSize = 64 * KILOBYTE * 300; // Alignment of buffers is always 64KB
    poolDesc.MinBlockCount = poolDesc.MaxBlockCount = 1;

    ComPtr<D3D12MA::Pool> pool;
    CHECK_HR(ctx.allocator->CreatePool(&poolDesc, &pool));

    D3D12_RESOURCE_DESC buffDesc = {};
    FillResourceDescForBuffer(buffDesc, 0);

    D3D12MA::ALLOCATION_DESC allocDesc = {};
    allocDesc.CustomPool = pool.Get();

    constexpr size_t maxBufCount = 100;
    struct BufferInfo
    {
        ComPtr<ID3D12Resource> Buffer;
        ComPtr<D3D12MA::Allocation> Allocation;
    };
    std::vector<BufferInfo> buffInfo;

    constexpr UINT64 bufSizeMin = 16;
    constexpr UINT64 bufSizeMax = 1024;
    UINT64 prevOffset = 0;

    // Test one-time free.
    for (size_t i = 0; i < 2; ++i)
    {
        // Allocate number of buffers of varying size that surely fit into this block.
        UINT64 bufSumSize = 0;
        UINT64 allocSumSize = 0;
        for (size_t i = 0; i < maxBufCount; ++i)
        {
            buffDesc.Width = AlignUp<UINT64>(bufSizeMin + rand.Generate() % (bufSizeMax - bufSizeMin), 16);
            BufferInfo newBuffInfo;
            CHECK_HR(ctx.allocator->CreateResource(&allocDesc, &buffDesc, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
                nullptr, &newBuffInfo.Allocation, IID_PPV_ARGS(&newBuffInfo.Buffer)));
            const UINT64 offset = newBuffInfo.Allocation->GetOffset();
            CHECK_BOOL(i == 0 || offset > prevOffset);
            prevOffset = offset;
            bufSumSize += buffDesc.Width;
            allocSumSize += newBuffInfo.Allocation->GetSize();
            buffInfo.push_back(std::move(newBuffInfo));
        }

        // Validate pool stats.
        D3D12MA::DetailedStatistics stats;
        pool->CalculateStatistics(&stats);
        CHECK_BOOL(stats.Stats.BlockBytes - stats.Stats.AllocationBytes == poolDesc.BlockSize - allocSumSize);
        CHECK_BOOL(allocSumSize >= bufSumSize);
        CHECK_BOOL(stats.Stats.AllocationCount == buffInfo.size());

        // Destroy the buffers in random order.
        while (!buffInfo.empty())
        {
            const size_t indexToDestroy = rand.Generate() % buffInfo.size();
            buffInfo.erase(buffInfo.begin() + indexToDestroy);
        }
    }

    // Test stack.
    {
        // Allocate number of buffers of varying size that surely fit into this block.
        for (size_t i = 0; i < maxBufCount; ++i)
        {
            buffDesc.Width = AlignUp<UINT64>(bufSizeMin + rand.Generate() % (bufSizeMax - bufSizeMin), 16);
            BufferInfo newBuffInfo;
            CHECK_HR(ctx.allocator->CreateResource(&allocDesc, &buffDesc, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
                nullptr, &newBuffInfo.Allocation, IID_PPV_ARGS(&newBuffInfo.Buffer)));
            const UINT64 offset = newBuffInfo.Allocation->GetOffset();
            CHECK_BOOL(i == 0 || offset > prevOffset);
            buffInfo.push_back(std::move(newBuffInfo));
            prevOffset = offset;
        }

        // Destroy few buffers from top of the stack.
        for (size_t i = 0; i < maxBufCount / 5; ++i)
            buffInfo.pop_back();

        // Create some more
        for (size_t i = 0; i < maxBufCount / 5; ++i)
        {
            buffDesc.Width = AlignUp<UINT64>(bufSizeMin + rand.Generate() % (bufSizeMax - bufSizeMin), 16);
            BufferInfo newBuffInfo;
            CHECK_HR(ctx.allocator->CreateResource(&allocDesc, &buffDesc, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
                nullptr, &newBuffInfo.Allocation, IID_PPV_ARGS(&newBuffInfo.Buffer)));
            const UINT64 offset = newBuffInfo.Allocation->GetOffset();
            CHECK_BOOL(i == 0 || offset > prevOffset);
            buffInfo.push_back(std::move(newBuffInfo));
            prevOffset = offset;
        }

        // Destroy the buffers in reverse order.
        while (!buffInfo.empty())
            buffInfo.pop_back();
    }

    // Test ring buffer.
    {
        // Allocate number of buffers that surely fit into this block.
        buffDesc.Width = bufSizeMax;
        for (size_t i = 0; i < maxBufCount; ++i)
        {
            BufferInfo newBuffInfo;
            CHECK_HR(ctx.allocator->CreateResource(&allocDesc, &buffDesc, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
                nullptr, &newBuffInfo.Allocation, IID_PPV_ARGS(&newBuffInfo.Buffer)));
            const UINT64 offset = newBuffInfo.Allocation->GetOffset();
            CHECK_BOOL(i == 0 || offset > prevOffset);
            buffInfo.push_back(std::move(newBuffInfo));
            prevOffset = offset;
        }

        // Free and allocate new buffers so many times that we make sure we wrap-around at least once.
        const size_t buffersPerIter = maxBufCount / 10 - 1;
        const size_t iterCount = poolDesc.BlockSize / buffDesc.Width / buffersPerIter * 2;
        for (size_t iter = 0; iter < iterCount; ++iter)
        {
            buffInfo.erase(buffInfo.begin(), buffInfo.begin() + buffersPerIter);

            for (size_t bufPerIter = 0; bufPerIter < buffersPerIter; ++bufPerIter)
            {
                BufferInfo newBuffInfo;
                CHECK_HR(ctx.allocator->CreateResource(&allocDesc, &buffDesc, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
                    nullptr, &newBuffInfo.Allocation, IID_PPV_ARGS(&newBuffInfo.Buffer)));
                buffInfo.push_back(std::move(newBuffInfo));
            }
        }

        // Allocate buffers until we reach out-of-memory.
        UINT32 debugIndex = 0;
        while (true)
        {
            BufferInfo newBuffInfo;
            HRESULT hr = ctx.allocator->CreateResource(&allocDesc, &buffDesc, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
                nullptr, &newBuffInfo.Allocation, IID_PPV_ARGS(&newBuffInfo.Buffer));
            ++debugIndex;
            if (SUCCEEDED(hr))
            {
                buffInfo.push_back(std::move(newBuffInfo));
            }
            else
            {
                CHECK_BOOL(hr == E_OUTOFMEMORY);
                break;
            }
        }

        // Destroy the buffers in random order.
        while (!buffInfo.empty())
        {
            const size_t indexToDestroy = rand.Generate() % buffInfo.size();
            buffInfo.erase(buffInfo.begin() + indexToDestroy);
        }
    }

    // Test double stack.
    {
        // Allocate number of buffers of varying size that surely fit into this block, alternate from bottom/top.
        UINT64 prevOffsetLower = 0;
        UINT64 prevOffsetUpper = poolDesc.BlockSize;
        for (size_t i = 0; i < maxBufCount; ++i)
        {
            const bool upperAddress = (i % 2) != 0;
            if (upperAddress)
                allocDesc.Flags = D3D12MA::ALLOCATION_FLAG_UPPER_ADDRESS;
            else
                allocDesc.Flags = D3D12MA::ALLOCATION_FLAG_NONE;
            buffDesc.Width = AlignUp<UINT64>(bufSizeMin + rand.Generate() % (bufSizeMax - bufSizeMin), 16);
            BufferInfo newBuffInfo;
            CHECK_HR(ctx.allocator->CreateResource(&allocDesc, &buffDesc, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
                nullptr, &newBuffInfo.Allocation, IID_PPV_ARGS(&newBuffInfo.Buffer)));
            const UINT64 offset = newBuffInfo.Allocation->GetOffset();
            if (upperAddress)
            {
                CHECK_BOOL(offset < prevOffsetUpper);
                prevOffsetUpper = offset;
            }
            else
            {
                CHECK_BOOL(offset >= prevOffsetLower);
                prevOffsetLower = offset;
            }
            CHECK_BOOL(prevOffsetLower < prevOffsetUpper);
            buffInfo.push_back(std::move(newBuffInfo));
        }

        // Destroy few buffers from top of the stack.
        for (size_t i = 0; i < maxBufCount / 5; ++i)
            buffInfo.pop_back();

        // Create some more
        for (size_t i = 0; i < maxBufCount / 5; ++i)
        {
            const bool upperAddress = (i % 2) != 0;
            if (upperAddress)
                allocDesc.Flags = D3D12MA::ALLOCATION_FLAG_UPPER_ADDRESS;
            else
                allocDesc.Flags = D3D12MA::ALLOCATION_FLAG_NONE;
            buffDesc.Width = AlignUp<UINT64>(bufSizeMin + rand.Generate() % (bufSizeMax - bufSizeMin), 16);
            BufferInfo newBuffInfo;
            CHECK_HR(ctx.allocator->CreateResource(&allocDesc, &buffDesc, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
                nullptr, &newBuffInfo.Allocation, IID_PPV_ARGS(&newBuffInfo.Buffer)));
            buffInfo.push_back(std::move(newBuffInfo));
        }

        // Destroy the buffers in reverse order.
        while (!buffInfo.empty())
            buffInfo.pop_back();

        // Create buffers on both sides until we reach out of memory.
        prevOffsetLower = 0;
        prevOffsetUpper = poolDesc.BlockSize;
        for (size_t i = 0; true; ++i)
        {
            const bool upperAddress = (i % 2) != 0;
            if (upperAddress)
                allocDesc.Flags = D3D12MA::ALLOCATION_FLAG_UPPER_ADDRESS;
            else
                allocDesc.Flags = D3D12MA::ALLOCATION_FLAG_NONE;
            buffDesc.Width = AlignUp<UINT64>(bufSizeMin + rand.Generate() % (bufSizeMax - bufSizeMin), 16);
            BufferInfo newBuffInfo;
            HRESULT hr = ctx.allocator->CreateResource(&allocDesc, &buffDesc, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
                nullptr, &newBuffInfo.Allocation, IID_PPV_ARGS(&newBuffInfo.Buffer));
            if (SUCCEEDED(hr))
            {
                const UINT64 offset = newBuffInfo.Allocation->GetOffset();
                if (upperAddress)
                {
                    CHECK_BOOL(offset < prevOffsetUpper);
                    prevOffsetUpper = offset;
                }
                else
                {
                    CHECK_BOOL(offset >= prevOffsetLower);
                    prevOffsetLower = offset;
                }
                CHECK_BOOL(prevOffsetLower < prevOffsetUpper);
                buffInfo.push_back(std::move(newBuffInfo));
            }
            else
                break;
        }

        // Destroy the buffers in random order.
        while (!buffInfo.empty())
        {
            const size_t indexToDestroy = rand.Generate() % buffInfo.size();
            buffInfo.erase(buffInfo.begin() + indexToDestroy);
        }

        // Create buffers on upper side only, constant size, until we reach out of memory.
        prevOffsetUpper = poolDesc.BlockSize;
        allocDesc.Flags = D3D12MA::ALLOCATION_FLAG_UPPER_ADDRESS;
        buffDesc.Width = bufSizeMax;
        while (true)
        {
            BufferInfo newBuffInfo;
            HRESULT hr = ctx.allocator->CreateResource(&allocDesc, &buffDesc, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
                nullptr, &newBuffInfo.Allocation, IID_PPV_ARGS(&newBuffInfo.Buffer));
            if (SUCCEEDED(hr))
            {
                const UINT64 offset = newBuffInfo.Allocation->GetOffset();
                CHECK_BOOL(offset < prevOffsetUpper);
                prevOffsetUpper = offset;
                buffInfo.push_back(std::move(newBuffInfo));
            }
            else
                break;
        }

        // Destroy the buffers in reverse order.
        while (!buffInfo.empty())
        {
            const BufferInfo& currBufInfo = buffInfo.back();
            buffInfo.pop_back();
        }
    }
}

static void TestLinearAllocatorMultiBlock(const TestContext& ctx)
{
    wprintf(L"Test linear allocator multi block\n");

    RandomNumberGenerator rand{ 345673 };

    D3D12MA::POOL_DESC poolDesc = {};
    poolDesc.HeapFlags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;
    poolDesc.HeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
    poolDesc.Flags = D3D12MA::POOL_FLAG_ALGORITHM_LINEAR;

    ComPtr<D3D12MA::Pool> pool;
    CHECK_HR(ctx.allocator->CreatePool(&poolDesc, &pool));

    D3D12_RESOURCE_DESC buffDesc = {};
    FillResourceDescForBuffer(buffDesc, 1024 * 1024);

    D3D12MA::ALLOCATION_DESC allocDesc = {};
    allocDesc.CustomPool = pool.Get();

    struct BufferInfo
    {
        ComPtr<ID3D12Resource> Buffer;
        ComPtr<D3D12MA::Allocation> Allocation;
    };
    std::vector<BufferInfo> buffInfo;

    // Test one-time free.
    {
        // Allocate buffers until we move to a second block.
        ID3D12Heap* lastHeap = nullptr;
        while (true)
        {
            BufferInfo newBuffInfo;
            CHECK_HR(ctx.allocator->CreateResource(&allocDesc, &buffDesc, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
                nullptr, &newBuffInfo.Allocation, IID_PPV_ARGS(&newBuffInfo.Buffer)));
            ID3D12Heap* heap = newBuffInfo.Allocation->GetHeap();
            buffInfo.push_back(std::move(newBuffInfo));
            if (lastHeap && heap != lastHeap)
            {
                break;
            }
            lastHeap = heap;
        }
        CHECK_BOOL(buffInfo.size() > 2);

        // Make sure that pool has now two blocks.
        D3D12MA::DetailedStatistics poolStats = {};
        pool->CalculateStatistics(&poolStats);
        CHECK_BOOL(poolStats.Stats.BlockCount == 2);

        // Destroy all the buffers in random order.
        while (!buffInfo.empty())
        {
            const size_t indexToDestroy = rand.Generate() % buffInfo.size();
            buffInfo.erase(buffInfo.begin() + indexToDestroy);
        }

        // Make sure that pool has now at most one block.
        pool->CalculateStatistics(&poolStats);
        CHECK_BOOL(poolStats.Stats.BlockCount <= 1);
    }

    // Test stack.
    {
        // Allocate buffers until we move to a second block.
        ID3D12Heap* lastHeap = nullptr;
        while (true)
        {
            BufferInfo newBuffInfo;
            CHECK_HR(ctx.allocator->CreateResource(&allocDesc, &buffDesc, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
                nullptr, &newBuffInfo.Allocation, IID_PPV_ARGS(&newBuffInfo.Buffer)));
            ID3D12Heap* heap = newBuffInfo.Allocation->GetHeap();
            buffInfo.push_back(std::move(newBuffInfo));
            if (lastHeap && heap != lastHeap)
            {
                break;
            }
            lastHeap = heap;
        }
        CHECK_BOOL(buffInfo.size() > 2);

        // Add few more buffers.
        for (UINT32 i = 0; i < 5; ++i)
        {
            BufferInfo newBuffInfo;
            CHECK_HR(ctx.allocator->CreateResource(&allocDesc, &buffDesc, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
                nullptr, &newBuffInfo.Allocation, IID_PPV_ARGS(&newBuffInfo.Buffer)));
            buffInfo.push_back(std::move(newBuffInfo));
        }

        // Make sure that pool has now two blocks.
        D3D12MA::DetailedStatistics poolStats = {};
        pool->CalculateStatistics(&poolStats);
        CHECK_BOOL(poolStats.Stats.BlockCount == 2);

        // Delete half of buffers, LIFO.
        for (size_t i = 0, countToDelete = buffInfo.size() / 2; i < countToDelete; ++i)
            buffInfo.pop_back();

        // Add one more buffer.
        BufferInfo newBuffInfo;
        CHECK_HR(ctx.allocator->CreateResource(&allocDesc, &buffDesc, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
            nullptr, &newBuffInfo.Allocation, IID_PPV_ARGS(&newBuffInfo.Buffer)));
        buffInfo.push_back(std::move(newBuffInfo));

        // Make sure that pool has now one block.
        pool->CalculateStatistics(&poolStats);
        CHECK_BOOL(poolStats.Stats.BlockCount == 1);

        // Delete all the remaining buffers, LIFO.
        while (!buffInfo.empty())
            buffInfo.pop_back();
    }
}

static void ManuallyTestLinearAllocator(const TestContext& ctx)
{
    wprintf(L"Manually test linear allocator\n");

    RandomNumberGenerator rand{ 645332 };

    D3D12MA::TotalStatistics origStats;
    ctx.allocator->CalculateStatistics(&origStats);

    D3D12MA::POOL_DESC poolDesc = {};
    poolDesc.HeapFlags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;
    poolDesc.HeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
    poolDesc.Flags = D3D12MA::POOL_FLAG_ALGORITHM_LINEAR;
    poolDesc.BlockSize = 6 * 64 * KILOBYTE;
    poolDesc.MinBlockCount = poolDesc.MaxBlockCount = 1;

    ComPtr<D3D12MA::Pool> pool;
    CHECK_HR(ctx.allocator->CreatePool(&poolDesc, &pool));

    D3D12_RESOURCE_DESC buffDesc = {};
    FillResourceDescForBuffer(buffDesc, 0);

    D3D12MA::ALLOCATION_DESC allocDesc = {};
    allocDesc.CustomPool = pool.Get();

    struct BufferInfo
    {
        ComPtr<ID3D12Resource> Buffer;
        ComPtr<D3D12MA::Allocation> Allocation;
    };
    std::vector<BufferInfo> buffInfo;
    BufferInfo newBuffInfo;

    // Test double stack.
    {
        /*
        Lower: Buffer 32 B, Buffer 1024 B, Buffer 32 B
        Upper: Buffer 16 B, Buffer 1024 B, Buffer 128 B

        Totally:
        1 block allocated
        393 216 DirectX 12 bytes
        6 new allocations
        2256 bytes in allocations (384 KB according to alignment)
        */

        buffDesc.Width = 32;
        CHECK_HR(ctx.allocator->CreateResource(&allocDesc, &buffDesc, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
            nullptr, &newBuffInfo.Allocation, IID_PPV_ARGS(&newBuffInfo.Buffer)));
        buffInfo.push_back(std::move(newBuffInfo));

        buffDesc.Width = 1024;
        CHECK_HR(ctx.allocator->CreateResource(&allocDesc, &buffDesc, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
            nullptr, &newBuffInfo.Allocation, IID_PPV_ARGS(&newBuffInfo.Buffer)));
        buffInfo.push_back(std::move(newBuffInfo));

        buffDesc.Width = 32;
        CHECK_HR(ctx.allocator->CreateResource(&allocDesc, &buffDesc, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
            nullptr, &newBuffInfo.Allocation, IID_PPV_ARGS(&newBuffInfo.Buffer)));
        buffInfo.push_back(std::move(newBuffInfo));

        allocDesc.Flags |= D3D12MA::ALLOCATION_FLAG_UPPER_ADDRESS;

        buffDesc.Width = 128;
        CHECK_HR(ctx.allocator->CreateResource(&allocDesc, &buffDesc, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
            nullptr, &newBuffInfo.Allocation, IID_PPV_ARGS(&newBuffInfo.Buffer)));
        buffInfo.push_back(std::move(newBuffInfo));

        buffDesc.Width = 1024;
        CHECK_HR(ctx.allocator->CreateResource(&allocDesc, &buffDesc, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
            nullptr, &newBuffInfo.Allocation, IID_PPV_ARGS(&newBuffInfo.Buffer)));
        buffInfo.push_back(std::move(newBuffInfo));

        buffDesc.Width = 16;
        CHECK_HR(ctx.allocator->CreateResource(&allocDesc, &buffDesc, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
            nullptr, &newBuffInfo.Allocation, IID_PPV_ARGS(&newBuffInfo.Buffer)));
        buffInfo.push_back(std::move(newBuffInfo));

        D3D12MA::TotalStatistics currStats;
        ctx.allocator->CalculateStatistics(&currStats);
        D3D12MA::DetailedStatistics poolStats;
        pool->CalculateStatistics(&poolStats);

        WCHAR* statsStr = nullptr;
        ctx.allocator->BuildStatsString(&statsStr, FALSE);

        // PUT BREAKPOINT HERE TO CHECK.
        // Inspect: currStats versus origStats, poolStats, statsStr.
        int I = 0;

        ctx.allocator->FreeStatsString(statsStr);

        // Destroy the buffers in reverse order.
        while (!buffInfo.empty())
            buffInfo.pop_back();
    }
}

static void BenchmarkAlgorithmsCase(const TestContext& ctx,
    FILE* file,
    D3D12MA::POOL_FLAGS algorithm,
    bool empty,
    FREE_ORDER freeOrder)
{
    RandomNumberGenerator rand{ 16223 };

    const UINT64 bufSize = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    const size_t maxBufCapacity = 10000;
    const UINT32 iterationCount = 10;

    D3D12MA::POOL_DESC poolDesc = {};
    poolDesc.HeapFlags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;
    poolDesc.HeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
    poolDesc.BlockSize = bufSize * maxBufCapacity;
    poolDesc.Flags |= algorithm;
    poolDesc.MinBlockCount = poolDesc.MaxBlockCount = 1;

    ComPtr<D3D12MA::Pool> pool;
    CHECK_HR(ctx.allocator->CreatePool(&poolDesc, &pool));

    D3D12_RESOURCE_ALLOCATION_INFO allocInfo = {};
    allocInfo.SizeInBytes = bufSize;

    D3D12MA::ALLOCATION_DESC allocDesc = {};
    allocDesc.CustomPool = pool.Get();

    std::vector<ComPtr<D3D12MA::Allocation>> baseAllocations;
    const size_t allocCount = maxBufCapacity / 3;
    if (!empty)
    {
        // Make allocations up to 1/3 of pool size.
        for (UINT64 i = 0; i < allocCount; ++i)
        {
            ComPtr<D3D12MA::Allocation> alloc;
            CHECK_HR(ctx.allocator->AllocateMemory(&allocDesc, &allocInfo, &alloc));
            baseAllocations.push_back(std::move(alloc));
        }

        // Delete half of them, choose randomly.
        size_t allocsToDelete = baseAllocations.size() / 2;
        for (size_t i = 0; i < allocsToDelete; ++i)
        {
            const size_t index = (size_t)rand.Generate() % baseAllocations.size();
            baseAllocations.erase(baseAllocations.begin() + index);
        }
    }

    // BENCHMARK
    std::vector<ComPtr<D3D12MA::Allocation>> testAllocations;
    duration allocTotalDuration = duration::zero();
    duration freeTotalDuration = duration::zero();
    for (uint32_t iterationIndex = 0; iterationIndex < iterationCount; ++iterationIndex)
    {
        testAllocations.reserve(allocCount);
        // Allocations
        time_point allocTimeBeg = std::chrono::high_resolution_clock::now();
        for (size_t i = 0; i < allocCount; ++i)
        {
            ComPtr<D3D12MA::Allocation> alloc;
            CHECK_HR(ctx.allocator->AllocateMemory(&allocDesc, &allocInfo, &alloc));
            testAllocations.push_back(std::move(alloc));
        }
        allocTotalDuration += std::chrono::high_resolution_clock::now() - allocTimeBeg;

        // Deallocations
        switch (freeOrder)
        {
        case FREE_ORDER::FORWARD:
            // Leave testAllocations unchanged.
            break;
        case FREE_ORDER::BACKWARD:
            std::reverse(testAllocations.begin(), testAllocations.end());
            break;
        case FREE_ORDER::RANDOM:
            std::shuffle(testAllocations.begin(), testAllocations.end(), MyUniformRandomNumberGenerator(rand));
            break;
        default: assert(0);
        }

        time_point freeTimeBeg = std::chrono::high_resolution_clock::now();
        testAllocations.clear();
        freeTotalDuration += std::chrono::high_resolution_clock::now() - freeTimeBeg;

    }

    // Delete baseAllocations
    baseAllocations.clear();

    const float allocTotalSeconds = ToFloatSeconds(allocTotalDuration);
    const float freeTotalSeconds = ToFloatSeconds(freeTotalDuration);

    printf("    Algorithm=%s %s FreeOrder=%s: allocations %g s, free %g s\n",
        AlgorithmToStr(algorithm),
        empty ? "Empty" : "Not empty",
        FREE_ORDER_NAMES[(size_t)freeOrder],
        allocTotalSeconds,
        freeTotalSeconds);

    if (file)
    {
        std::string currTime;
        CurrentTimeToStr(currTime);

        fprintf(file, "%s,%s,%s,%u,%s,%g,%g\n",
            CODE_DESCRIPTION, currTime.c_str(),
            AlgorithmToStr(algorithm),
            empty ? 1 : 0,
            FREE_ORDER_NAMES[(uint32_t)freeOrder],
            allocTotalSeconds,
            freeTotalSeconds);
    }
}

static void BenchmarkAlgorithms(const TestContext& ctx, FILE* file)
{
    wprintf(L"Benchmark algorithms\n");

    if (file)
    {
        fprintf(file,
            "Code,Time,"
            "Algorithm,Empty,Free order,"
            "Allocation time (s),Deallocation time (s)\n");
    }

    UINT32 freeOrderCount = 1;
    if (ConfigType >= CONFIG_TYPE::CONFIG_TYPE_LARGE)
        freeOrderCount = 3;
    else if (ConfigType >= CONFIG_TYPE::CONFIG_TYPE_SMALL)
        freeOrderCount = 2;

    const UINT32 emptyCount = ConfigType >= CONFIG_TYPE::CONFIG_TYPE_SMALL ? 2 : 1;

    for (UINT32 freeOrderIndex = 0; freeOrderIndex < freeOrderCount; ++freeOrderIndex)
    {
        FREE_ORDER freeOrder = FREE_ORDER::COUNT;
        switch (freeOrderIndex)
        {
        case 0: freeOrder = FREE_ORDER::BACKWARD; break;
        case 1: freeOrder = FREE_ORDER::FORWARD; break;
        case 2: freeOrder = FREE_ORDER::RANDOM; break;
        default: assert(0);
        }

        for (UINT32 emptyIndex = 0; emptyIndex < emptyCount; ++emptyIndex)
        {
            for (UINT32 algorithmIndex = 0; algorithmIndex < 2; ++algorithmIndex)
            {
                D3D12MA::POOL_FLAGS algorithm;
                switch (algorithmIndex)
                {
                case 0:
                    algorithm = D3D12MA::POOL_FLAG_NONE;
                    break;
                case 1:
                    algorithm = D3D12MA::POOL_FLAG_ALGORITHM_LINEAR;
                    break;
                default:
                    assert(0);
                }

                BenchmarkAlgorithmsCase(ctx,
                    file,
                    algorithm,
                    (emptyIndex == 0), // empty
                    freeOrder);
            }
        }
    }
}

#ifdef __ID3D12Device4_INTERFACE_DEFINED__
static void TestDevice4(const TestContext& ctx)
{
    wprintf(L"Test ID3D12Device4\n");

    if(!IsProtectedResourceSessionSupported(ctx))
    {
        wprintf(L"D3D12_FEATURE_PROTECTED_RESOURCE_SESSION_SUPPORT returned no support for protected resource session.\n");
        return;
    }

    ComPtr<ID3D12Device4> dev4;
    HRESULT hr = ctx.device->QueryInterface(IID_PPV_ARGS(&dev4));
    if(FAILED(hr))
    {
        wprintf(L"QueryInterface for ID3D12Device4 FAILED.\n");
        return;
    }

    D3D12_PROTECTED_RESOURCE_SESSION_DESC sessionDesc = {};
    ComPtr<ID3D12ProtectedResourceSession> session;
    // This fails on the SOFTWARE adapter.
    hr = dev4->CreateProtectedResourceSession(&sessionDesc, IID_PPV_ARGS(&session));
    if(FAILED(hr))
    {
        wprintf(L"ID3D12Device4::CreateProtectedResourceSession FAILED.\n");
        return;
    }

    D3D12MA::POOL_DESC poolDesc = {};
    poolDesc.HeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
    poolDesc.pProtectedSession = session.Get();
    poolDesc.MinAllocationAlignment = 0;
    poolDesc.HeapFlags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;

    ComPtr<D3D12MA::Pool> pool;
    hr = ctx.allocator->CreatePool(&poolDesc, &pool);
    if(FAILED(hr))
    {
        wprintf(L"Failed to create custom pool.\n");
        return;
    }

    D3D12_RESOURCE_DESC resourceDesc;
    FillResourceDescForBuffer(resourceDesc, 64 * KILOBYTE);

    for(UINT testIndex = 0; testIndex < 2; ++testIndex)
    {
        // Create a buffer
        D3D12MA::ALLOCATION_DESC allocDesc = {};
        allocDesc.CustomPool = pool.Get();
        if(testIndex == 0)
            allocDesc.Flags = D3D12MA::ALLOCATION_FLAG_COMMITTED;
        ComPtr<D3D12MA::Allocation> bufAlloc;
        ComPtr<ID3D12Resource> bufRes;
        CHECK_HR(ctx.allocator->CreateResource(&allocDesc, &resourceDesc,
            D3D12_RESOURCE_STATE_COMMON, NULL,
            &bufAlloc, IID_PPV_ARGS(&bufRes)));
        CHECK_BOOL(bufAlloc && bufAlloc->GetResource() == bufRes.Get());
        // Make sure it's (not) committed.
        CHECK_BOOL((bufAlloc->GetHeap() == NULL) == (testIndex == 0));

        // Allocate memory/heap
        // Temporarily disabled on NVIDIA as it causes BSOD on RTX2080Ti driver 461.40.
        if(g_AdapterDesc.VendorId != VENDOR_ID_NVIDIA)
        {
            D3D12_RESOURCE_ALLOCATION_INFO heapAllocInfo = {
                D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT * 2, // SizeInBytes
                D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT, // Alignment
            };
            ComPtr<D3D12MA::Allocation> memAlloc;
            CHECK_HR(ctx.allocator->AllocateMemory(&allocDesc, &heapAllocInfo, &memAlloc));
            CHECK_BOOL(memAlloc->GetHeap());
        }
    }
}
#endif // #ifdef __ID3D12Device4_INTERFACE_DEFINED__

#ifdef __ID3D12Device8_INTERFACE_DEFINED__
static void TestDevice8(const TestContext& ctx)
{
    wprintf(L"Test ID3D12Device8\n");

    ComPtr<ID3D12Device8> dev8;
    CHECK_HR(ctx.device->QueryInterface(IID_PPV_ARGS(&dev8)));

    D3D12_RESOURCE_DESC1 resourceDesc;
    FillResourceDescForBuffer(resourceDesc, 1024 * 1024);

    // Create a committed buffer

    D3D12MA::ALLOCATION_DESC allocDesc = {};
    allocDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
    allocDesc.Flags = D3D12MA::ALLOCATION_FLAG_COMMITTED;

    ComPtr<D3D12MA::Allocation> allocPtr0;
    ComPtr<ID3D12Resource> res0;
    CHECK_HR(ctx.allocator->CreateResource2(&allocDesc, &resourceDesc,
        D3D12_RESOURCE_STATE_COMMON, NULL,
        &allocPtr0, IID_PPV_ARGS(&res0)));
    CHECK_BOOL(allocPtr0->GetHeap() == NULL);

    // Create a heap and placed buffer in it

    allocDesc.Flags |= D3D12MA::ALLOCATION_FLAG_CAN_ALIAS;

    ComPtr<D3D12MA::Allocation> allocPtr1;
    ComPtr<ID3D12Resource> res1;
    CHECK_HR(ctx.allocator->CreateResource2(&allocDesc, &resourceDesc,
        D3D12_RESOURCE_STATE_COMMON, NULL,
        &allocPtr1, IID_PPV_ARGS(&res1)));
    CHECK_BOOL(allocPtr1->GetHeap() != NULL);

    // Create a placed buffer

    allocDesc.Flags &= ~D3D12MA::ALLOCATION_FLAG_COMMITTED;

    ComPtr<D3D12MA::Allocation> allocPtr2;
    ComPtr<ID3D12Resource> res2;
    CHECK_HR(ctx.allocator->CreateResource2(&allocDesc, &resourceDesc,
        D3D12_RESOURCE_STATE_COMMON, NULL,
        &allocPtr2, IID_PPV_ARGS(&res2)));
    CHECK_BOOL(allocPtr2->GetHeap()!= NULL);

    // Create an aliasing buffer
    ComPtr<ID3D12Resource> res3;
    CHECK_HR(ctx.allocator->CreateAliasingResource1(allocPtr2.Get(), 0, &resourceDesc,
        D3D12_RESOURCE_STATE_COMMON, NULL,
        IID_PPV_ARGS(&res3)));
}
#endif // #ifdef __ID3D12Device8_INTERFACE_DEFINED__

#ifdef __ID3D12Device10_INTERFACE_DEFINED__
static void TestDevice10(const TestContext& ctx)
{
    wprintf(L"Test ID3D12Device10\n");

    ComPtr<ID3D12Device10> dev10;
    if(FAILED(ctx.device->QueryInterface(IID_PPV_ARGS(&dev10))))
    {
        wprintf(L"QueryInterface for ID3D12Device10 failed!\n");
        return;
    }

    D3D12_RESOURCE_DESC1 resourceDesc = {};
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resourceDesc.Alignment = 0;
    resourceDesc.Width = 1920;
    resourceDesc.Height = 1080;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.MipLevels = 1;
    resourceDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.SampleDesc.Quality = 0;
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

    // Create a committed texture

    D3D12MA::ALLOCATION_DESC allocDesc = {};
    allocDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
    allocDesc.Flags = D3D12MA::ALLOCATION_FLAG_COMMITTED;

    ComPtr<D3D12MA::Allocation> allocPtr0;
    ComPtr<ID3D12Resource> res0;
    CHECK_HR(ctx.allocator->CreateResource3(&allocDesc, &resourceDesc,
        D3D12_BARRIER_LAYOUT_UNDEFINED, NULL, 0, NULL,
        &allocPtr0, IID_PPV_ARGS(&res0)));
    CHECK_BOOL(allocPtr0->GetHeap() == NULL);

    // Create a heap and placed texture in it

    allocDesc.Flags |= D3D12MA::ALLOCATION_FLAG_CAN_ALIAS;

    ComPtr<D3D12MA::Allocation> allocPtr1;
    ComPtr<ID3D12Resource> res1;
    CHECK_HR(ctx.allocator->CreateResource3(&allocDesc, &resourceDesc,
        D3D12_BARRIER_LAYOUT_UNDEFINED, NULL, 0, NULL,
        &allocPtr1, IID_PPV_ARGS(&res1)));
    CHECK_BOOL(allocPtr1->GetHeap() != NULL);

    // Create a placed texture

    allocDesc.Flags &= ~D3D12MA::ALLOCATION_FLAG_COMMITTED;

    ComPtr<D3D12MA::Allocation> allocPtr2;
    ComPtr<ID3D12Resource> res2;
    CHECK_HR(ctx.allocator->CreateResource3(&allocDesc, &resourceDesc,
        D3D12_BARRIER_LAYOUT_UNDEFINED, NULL, 0, NULL,
        &allocPtr2, IID_PPV_ARGS(&res2)));
    CHECK_BOOL(allocPtr2->GetHeap() != NULL);

    // Create an aliasing texture
    ComPtr<ID3D12Resource> res3;
    CHECK_HR(ctx.allocator->CreateAliasingResource2(allocPtr2.Get(), 0, &resourceDesc,
        D3D12_BARRIER_LAYOUT_UNDEFINED, NULL, 0, NULL,
        IID_PPV_ARGS(&res3)));
}
#endif // #ifdef __ID3D12Device10_INTERFACE_DEFINED__

static void TestVirtualBlocks(const TestContext& ctx)
{
    wprintf(L"Test virtual blocks\n");

    using namespace D3D12MA;

    const UINT64 blockSize = 16 * MEGABYTE;
    const UINT64 alignment = 256;

    // # Create block 16 MB

    ComPtr<D3D12MA::VirtualBlock> block;
    VIRTUAL_BLOCK_DESC blockDesc = {};
    blockDesc.pAllocationCallbacks = ctx.allocationCallbacks;
    blockDesc.Size = blockSize;
    CHECK_HR(CreateVirtualBlock(&blockDesc, &block));
    CHECK_BOOL(block);

    // # Allocate 8 MB

    VIRTUAL_ALLOCATION_DESC allocDesc = {};
    allocDesc.Alignment = alignment;
    allocDesc.pPrivateData = (void*)(uintptr_t)1;
    allocDesc.Size = 8 * MEGABYTE;
    VirtualAllocation alloc0;
    CHECK_HR(block->Allocate(&allocDesc, &alloc0, nullptr));

    // # Validate the allocation

    VIRTUAL_ALLOCATION_INFO alloc0Info = {};
    block->GetAllocationInfo(alloc0, &alloc0Info);
    CHECK_BOOL(alloc0Info.Offset < blockSize);
    CHECK_BOOL(alloc0Info.Size == allocDesc.Size);
    CHECK_BOOL(alloc0Info.pPrivateData == allocDesc.pPrivateData);

    // # Check SetUserData

    block->SetAllocationPrivateData(alloc0, (void*)(uintptr_t)2);
    block->GetAllocationInfo(alloc0, &alloc0Info);
    CHECK_BOOL(alloc0Info.pPrivateData == (void*)(uintptr_t)2);

    // # Allocate 4 MB

    allocDesc.Size = 4 * MEGABYTE;
    allocDesc.Alignment = alignment;
    VirtualAllocation alloc1;
    CHECK_HR(block->Allocate(&allocDesc, &alloc1, nullptr));

    VIRTUAL_ALLOCATION_INFO alloc1Info = {};
    block->GetAllocationInfo(alloc1, &alloc1Info);
    CHECK_BOOL(alloc1Info.Offset < blockSize);
    CHECK_BOOL(alloc1Info.Offset + 4 * MEGABYTE <= alloc0Info.Offset || alloc0Info.Offset + 8 * MEGABYTE <= alloc1Info.Offset); // Check if they don't overlap.

    // # Allocate another 8 MB - it should fail

    allocDesc.Size = 8 * MEGABYTE;
    allocDesc.Alignment = alignment;
    VirtualAllocation alloc2;
    CHECK_BOOL(FAILED(block->Allocate(&allocDesc, &alloc2, nullptr)));
    CHECK_BOOL(alloc2.AllocHandle == (AllocHandle)0);

    // # Free the 4 MB block. Now allocation of 8 MB should succeed.

    block->FreeAllocation(alloc1);
    UINT64 alloc2Offset;
    CHECK_HR(block->Allocate(&allocDesc, &alloc2, &alloc2Offset));
    CHECK_BOOL(alloc2Offset < blockSize);
    CHECK_BOOL(alloc2Offset + 4 * MEGABYTE <= alloc0Info.Offset || alloc0Info.Offset + 8 * MEGABYTE <= alloc2Offset); // Check if they don't overlap.

    // # Calculate statistics

    DetailedStatistics statInfo = {};
    block->CalculateStatistics(&statInfo);
    CHECK_BOOL(statInfo.Stats.AllocationCount == 2);
    CHECK_BOOL(statInfo.Stats.BlockCount == 1);
    CHECK_BOOL(statInfo.Stats.AllocationBytes == blockSize);
    CHECK_BOOL(statInfo.Stats.BlockBytes == blockSize);

    // # Generate JSON dump

    WCHAR* json = nullptr;
    block->BuildStatsString(&json);
    {
        std::wstring str(json);
        CHECK_BOOL(str.find(L"\"CustomData\": 1") != std::wstring::npos);
        CHECK_BOOL(str.find(L"\"CustomData\": 2") != std::wstring::npos);
    }
    block->FreeStatsString(json);

    // # Free alloc0, leave alloc2 unfreed.

    block->FreeAllocation(alloc0);

    // # Test alignment

    {
        constexpr size_t allocCount = 10;
        VirtualAllocation allocs[allocCount] = {};
        for (size_t i = 0; i < allocCount; ++i)
        {
            const bool alignment0 = i == allocCount - 1;
            allocDesc.Size = i * 3 + 15;
            allocDesc.Alignment = alignment0 ? 0 : 8;
            UINT64 offset;
            CHECK_HR(block->Allocate(&allocDesc, &allocs[i], &offset));
            if (!alignment0)
            {
                CHECK_BOOL(offset % allocDesc.Alignment == 0);
            }
        }

        for (size_t i = allocCount; i--; )
        {
            block->FreeAllocation(allocs[i]);
        }
    }

    // # Final cleanup

    block->FreeAllocation(alloc2);
}

static void TestVirtualBlocksAlgorithms(const TestContext& ctx)
{
    wprintf(L"Test virtual blocks algorithms\n");

    RandomNumberGenerator rand{ 3454335 };
    auto calcRandomAllocSize = [&rand]() -> UINT64 { return rand.Generate() % 20 + 5; };

    for (size_t algorithmIndex = 0; algorithmIndex < 2; ++algorithmIndex)
    {
        // Create the block
        D3D12MA::VIRTUAL_BLOCK_DESC blockDesc = {};
        blockDesc.pAllocationCallbacks = ctx.allocationCallbacks;
        blockDesc.Size = 10'000;
        switch (algorithmIndex)
        {
        case 0: blockDesc.Flags = D3D12MA::VIRTUAL_BLOCK_FLAG_NONE; break;
        case 1: blockDesc.Flags = D3D12MA::VIRTUAL_BLOCK_FLAG_ALGORITHM_LINEAR; break;
        }
        ComPtr<D3D12MA::VirtualBlock> block;
        CHECK_HR(D3D12MA::CreateVirtualBlock(&blockDesc, &block));

        struct AllocData
        {
            D3D12MA::VirtualAllocation allocation;
            UINT64 allocOffset, requestedSize, allocationSize;
        };
        std::vector<AllocData> allocations;

        // Make some allocations
        for (size_t i = 0; i < 20; ++i)
        {
            D3D12MA::VIRTUAL_ALLOCATION_DESC allocDesc = {};
            allocDesc.Size = calcRandomAllocSize();
            allocDesc.pPrivateData = (void*)(uintptr_t)(allocDesc.Size * 10);
            if (i < 10) {}
            else if (i < 20 && algorithmIndex == 1) allocDesc.Flags = D3D12MA::VIRTUAL_ALLOCATION_FLAG_UPPER_ADDRESS;

            AllocData alloc = {};
            alloc.requestedSize = allocDesc.Size;
            CHECK_HR(block->Allocate(&allocDesc, &alloc.allocation, nullptr));

            D3D12MA::VIRTUAL_ALLOCATION_INFO allocInfo;
            block->GetAllocationInfo(alloc.allocation, &allocInfo);
            CHECK_BOOL(allocInfo.Size >= allocDesc.Size);
            alloc.allocOffset = allocInfo.Offset;
            alloc.allocationSize = allocInfo.Size;

            allocations.push_back(alloc);
        }

        // Free some of the allocations
        for (size_t i = 0; i < 5; ++i)
        {
            const size_t index = rand.Generate() % allocations.size();
            block->FreeAllocation(allocations[index].allocation);
            allocations.erase(allocations.begin() + index);
        }

        // Allocate some more
        for (size_t i = 0; i < 6; ++i)
        {
            D3D12MA::VIRTUAL_ALLOCATION_DESC allocDesc = {};
            allocDesc.Size = calcRandomAllocSize();
            allocDesc.pPrivateData = (void*)(uintptr_t)(allocDesc.Size * 10);

            AllocData alloc = {};
            alloc.requestedSize = allocDesc.Size;
            CHECK_HR(block->Allocate(&allocDesc, &alloc.allocation, nullptr));

            D3D12MA::VIRTUAL_ALLOCATION_INFO allocInfo;
            block->GetAllocationInfo(alloc.allocation, &allocInfo);
            CHECK_BOOL(allocInfo.Size >= allocDesc.Size);
            alloc.allocOffset = allocInfo.Offset;
            alloc.allocationSize = allocInfo.Size;

            allocations.push_back(alloc);
        }

        // Allocate some with extra alignment
        for (size_t i = 0; i < 3; ++i)
        {
            D3D12MA::VIRTUAL_ALLOCATION_DESC allocDesc = {};
            allocDesc.Size = calcRandomAllocSize();
            allocDesc.Alignment = 16;
            allocDesc.pPrivateData = (void*)(uintptr_t)(allocDesc.Size * 10);

            AllocData alloc = {};
            alloc.requestedSize = allocDesc.Size;
            CHECK_HR(block->Allocate(&allocDesc, &alloc.allocation, nullptr));

            D3D12MA::VIRTUAL_ALLOCATION_INFO allocInfo;
            block->GetAllocationInfo(alloc.allocation, &allocInfo);
            CHECK_BOOL(allocInfo.Offset % 16 == 0);
            CHECK_BOOL(allocInfo.Size >= allocDesc.Size);
            alloc.allocOffset = allocInfo.Offset;
            alloc.allocationSize = allocInfo.Size;

            allocations.push_back(alloc);
        }

        // Check if the allocations don't overlap
        std::sort(allocations.begin(), allocations.end(), [](const AllocData& lhs, const AllocData& rhs) {
            return lhs.allocOffset < rhs.allocOffset; });
        for (size_t i = 0; i < allocations.size() - 1; ++i)
        {
            CHECK_BOOL(allocations[i + 1].allocOffset >= allocations[i].allocOffset + allocations[i].allocationSize);
        }

        // Check pPrivateData
        {
            const AllocData& alloc = allocations.back();
            D3D12MA::VIRTUAL_ALLOCATION_INFO allocInfo;
            block->GetAllocationInfo(alloc.allocation, &allocInfo);
            CHECK_BOOL((uintptr_t)allocInfo.pPrivateData == alloc.requestedSize * 10);

            block->SetAllocationPrivateData(alloc.allocation, (void*)(uintptr_t)666);
            block->GetAllocationInfo(alloc.allocation, &allocInfo);
            CHECK_BOOL((uintptr_t)allocInfo.pPrivateData == 666);
        }

        // Calculate statistics
        {
            UINT64 actualAllocSizeMin = UINT64_MAX, actualAllocSizeMax = 0, actualAllocSizeSum = 0;
            std::for_each(allocations.begin(), allocations.end(), [&](const AllocData& a) {
                actualAllocSizeMin = std::min(actualAllocSizeMin, a.allocationSize);
                actualAllocSizeMax = std::max(actualAllocSizeMax, a.allocationSize);
                actualAllocSizeSum += a.allocationSize;
                });

            D3D12MA::DetailedStatistics statInfo = {};
            block->CalculateStatistics(&statInfo);
            CHECK_BOOL(statInfo.Stats.AllocationCount == allocations.size());
            CHECK_BOOL(statInfo.Stats.BlockCount == 1);
            CHECK_BOOL(statInfo.Stats.BlockBytes == blockDesc.Size);
            CHECK_BOOL(statInfo.AllocationSizeMax == actualAllocSizeMax);
            CHECK_BOOL(statInfo.AllocationSizeMin == actualAllocSizeMin);
            CHECK_BOOL(statInfo.Stats.AllocationBytes >= actualAllocSizeSum);
        }

        // Build JSON dump string
        {
            WCHAR* json = nullptr;
            block->BuildStatsString(&json);
            int I = 0; // put a breakpoint here to debug
            block->FreeStatsString(json);
        }

        // Final cleanup
        block->Clear();
    }
}

static void TestVirtualBlocksAlgorithmsBenchmark(const TestContext& ctx)
{
    wprintf(L"Benchmark virtual blocks algorithms\n");

    const size_t ALLOCATION_COUNT = 7200;
    const UINT32 MAX_ALLOC_SIZE = 2056;

    D3D12MA::VIRTUAL_BLOCK_DESC blockDesc = {};
    blockDesc.pAllocationCallbacks = ctx.allocationCallbacks;
    blockDesc.Size = 0;

    RandomNumberGenerator rand{ 20092010 };

    UINT32 allocSizes[ALLOCATION_COUNT];
    for (size_t i = 0; i < ALLOCATION_COUNT; ++i)
    {
        allocSizes[i] = rand.Generate() % MAX_ALLOC_SIZE + 1;
        blockDesc.Size += allocSizes[i];
    }
    blockDesc.Size = static_cast<UINT64>(blockDesc.Size * 1.5); // 50% size margin in case of alignment

    for (UINT8 alignmentIndex = 0; alignmentIndex < 4; ++alignmentIndex)
    {
        UINT64 alignment;
        switch (alignmentIndex)
        {
        case 0: alignment = 1; break;
        case 1: alignment = 16; break;
        case 2: alignment = 64; break;
        case 3: alignment = 256; break;
        default: assert(0); break;
        }
        printf("    Alignment=%llu\n", alignment);

        for (UINT8 algorithmIndex = 0; algorithmIndex < 2; ++algorithmIndex)
        {
            switch (algorithmIndex)
            {
            case 0:
                blockDesc.Flags = D3D12MA::VIRTUAL_BLOCK_FLAG_NONE;
                break;
            case 1:
                blockDesc.Flags = D3D12MA::VIRTUAL_BLOCK_FLAG_ALGORITHM_LINEAR;
                break;
            default:
                assert(0);
            }

            D3D12MA::VirtualAllocation allocs[ALLOCATION_COUNT];
            ComPtr<D3D12MA::VirtualBlock> block;
            CHECK_HR(D3D12MA::CreateVirtualBlock(&blockDesc, &block));
            duration allocDuration = duration::zero();
            duration freeDuration = duration::zero();

            // Alloc
            time_point timeBegin = std::chrono::high_resolution_clock::now();
            for (size_t i = 0; i < ALLOCATION_COUNT; ++i)
            {
                D3D12MA::VIRTUAL_ALLOCATION_DESC allocCreateInfo = {};
                allocCreateInfo.Size = allocSizes[i];
                allocCreateInfo.Alignment = alignment;

                CHECK_HR(block->Allocate(&allocCreateInfo, allocs + i, nullptr));
            }
            allocDuration += std::chrono::high_resolution_clock::now() - timeBegin;

            // Free
            timeBegin = std::chrono::high_resolution_clock::now();
            for (size_t i = ALLOCATION_COUNT; i;)
                block->FreeAllocation(allocs[--i]);
            freeDuration += std::chrono::high_resolution_clock::now() - timeBegin;

            printf("        Algorithm=%s  \tallocations %g s,   \tfree %g s\n",
                VirtualAlgorithmToStr(blockDesc.Flags),
                ToFloatSeconds(allocDuration),
                ToFloatSeconds(freeDuration));
        }
        printf("\n");
    }
}

static void ProcessDefragmentationPass(const TestContext& ctx, D3D12MA::DEFRAGMENTATION_PASS_MOVE_INFO& stepInfo)
{
    std::vector<D3D12_RESOURCE_BARRIER> startBarriers;
    std::vector<D3D12_RESOURCE_BARRIER> finalBarriers;

    bool defaultHeap = false;
    for (UINT32 i = 0; i < stepInfo.MoveCount; ++i)
    {
        if (stepInfo.pMoves[i].Operation == D3D12MA::DEFRAGMENTATION_MOVE_OPERATION_COPY)
        {
            const bool isDefaultHeap = stepInfo.pMoves[i].pSrcAllocation->GetHeap()->GetDesc().Properties.Type == D3D12_HEAP_TYPE_DEFAULT;
            // Create new resource
            D3D12_RESOURCE_DESC desc = stepInfo.pMoves[i].pSrcAllocation->GetResource()->GetDesc();
            ComPtr<ID3D12Resource> dstRes;
            CHECK_HR(ctx.device->CreatePlacedResource(stepInfo.pMoves[i].pDstTmpAllocation->GetHeap(),
                stepInfo.pMoves[i].pDstTmpAllocation->GetOffset(), &desc,
                isDefaultHeap ? D3D12_RESOURCE_STATE_COPY_DEST : D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr, IID_PPV_ARGS(&dstRes)));
            stepInfo.pMoves[i].pDstTmpAllocation->SetResource(dstRes.Get());

            // Perform barriers only if not in right state
            if (isDefaultHeap)
            {
                defaultHeap = true;
                // Move new resource into previous state
                D3D12_RESOURCE_BARRIER barrier = {};
                barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
                barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
                barrier.Transition.pResource = dstRes.Get();
                barrier.Transition.StateAfter = (D3D12_RESOURCE_STATES)(uintptr_t)stepInfo.pMoves[i].pSrcAllocation->GetPrivateData();
                barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
                finalBarriers.emplace_back(barrier);

                // Move resource into right state
                barrier.Transition.pResource = stepInfo.pMoves[i].pSrcAllocation->GetResource();
                barrier.Transition.StateBefore = barrier.Transition.StateAfter;
                barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
                startBarriers.emplace_back(barrier);
            }
        }
    }

    if (defaultHeap)
    {
        ID3D12GraphicsCommandList* cl = BeginCommandList();
        cl->ResourceBarrier(static_cast<UINT>(startBarriers.size()), startBarriers.data());

        // Copy resources
        for (UINT32 i = 0; i < stepInfo.MoveCount; ++i)
        {
            if (stepInfo.pMoves[i].Operation == D3D12MA::DEFRAGMENTATION_MOVE_OPERATION_COPY)
            {
                ID3D12Resource* dstRes = stepInfo.pMoves[i].pDstTmpAllocation->GetResource();
                ID3D12Resource* srcRes = stepInfo.pMoves[i].pSrcAllocation->GetResource();

                if (stepInfo.pMoves[i].pDstTmpAllocation->GetHeap()->GetDesc().Properties.Type == D3D12_HEAP_TYPE_DEFAULT)
                {
                    cl->CopyResource(dstRes, srcRes);
                }
                else
                {
                    D3D12_RANGE range = {};
                    void* dst;
                    CHECK_HR(dstRes->Map(0, &range, &dst));
                    void* src;
                    CHECK_HR(srcRes->Map(0, &range, &src));
                    memcpy(dst, src, stepInfo.pMoves[i].pSrcAllocation->GetSize());
                    dstRes->Unmap(0, nullptr);
                    srcRes->Unmap(0, nullptr);
                }
            }
        }

        cl->ResourceBarrier(static_cast<UINT>(finalBarriers.size()), finalBarriers.data());
        EndCommandList(cl);
    }
    else
    {
        // Copy only CPU-side
        for (UINT32 i = 0; i < stepInfo.MoveCount; ++i)
        {
            if (stepInfo.pMoves[i].Operation == D3D12MA::DEFRAGMENTATION_MOVE_OPERATION_COPY)
            {
                D3D12_RANGE range = {};

                void* dst;
                ID3D12Resource* dstRes = stepInfo.pMoves[i].pDstTmpAllocation->GetResource();
                CHECK_HR(dstRes->Map(0, &range, &dst));

                void* src;
                ID3D12Resource* srcRes = stepInfo.pMoves[i].pSrcAllocation->GetResource();
                CHECK_HR(srcRes->Map(0, &range, &src));

                memcpy(dst, src, stepInfo.pMoves[i].pSrcAllocation->GetSize());
                dstRes->Unmap(0, nullptr);
                srcRes->Unmap(0, nullptr);
            }
        }
    }
}

static void Defragment(const TestContext& ctx,
    D3D12MA::DEFRAGMENTATION_DESC& defragDesc,
    D3D12MA::Pool* pool,
    D3D12MA::DEFRAGMENTATION_STATS* defragStats = nullptr)
{
    ComPtr<D3D12MA::DefragmentationContext> defragCtx;
    if (pool != nullptr)
    {
        CHECK_HR(pool->BeginDefragmentation(&defragDesc, &defragCtx));
    }
    else
        ctx.allocator->BeginDefragmentation(&defragDesc, &defragCtx);

    HRESULT hr = S_OK;
    D3D12MA::DEFRAGMENTATION_PASS_MOVE_INFO pass = {};
    while ((hr = defragCtx->BeginPass(&pass)) == S_FALSE)
    {
        ProcessDefragmentationPass(ctx, pass);

        if ((hr = defragCtx->EndPass(&pass)) == S_OK)
            break;
        CHECK_BOOL(hr == S_FALSE);
    }
    CHECK_HR(hr);
    if (defragStats != nullptr)
        defragCtx->GetStats(defragStats);
}

static void TestDefragmentationSimple(const TestContext& ctx)
{
    wprintf(L"Test defragmentation simple\n");

    RandomNumberGenerator rand(667);

    const UINT ALLOC_SEED = 20220310;
    const UINT64 BUF_SIZE = 0x10000;
    const UINT64 BLOCK_SIZE = BUF_SIZE * 8;

    const UINT64 MIN_BUF_SIZE = 32;
    const UINT64 MAX_BUF_SIZE = BUF_SIZE * 4;
    auto RandomBufSize = [&]() -> UINT64
    {
        return AlignUp<UINT64>(rand.Generate() % (MAX_BUF_SIZE - MIN_BUF_SIZE + 1) + MIN_BUF_SIZE, 64);
    };

    D3D12MA::POOL_DESC poolDesc = {};
    poolDesc.BlockSize = BLOCK_SIZE;
    poolDesc.HeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
    poolDesc.HeapFlags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;
    ComPtr<D3D12MA::Pool> pool;
    CHECK_HR(ctx.allocator->CreatePool(&poolDesc, &pool));

    D3D12MA::ALLOCATION_DESC allocDesc = {};
    allocDesc.CustomPool = pool.Get();

    D3D12_RESOURCE_DESC resDesc = {};
    FillResourceDescForBuffer(resDesc, BUF_SIZE);

    D3D12MA::DEFRAGMENTATION_DESC defragDesc = {};
    defragDesc.Flags = D3D12MA::DEFRAGMENTATION_FLAG_ALGORITHM_FAST;

    // Defragmentation of empty pool.
    {
        ComPtr<D3D12MA::DefragmentationContext> defragCtx = nullptr;
        CHECK_HR(pool->BeginDefragmentation(&defragDesc, &defragCtx));

        D3D12MA::DEFRAGMENTATION_PASS_MOVE_INFO pass = {};
        CHECK_BOOL(defragCtx->BeginPass(&pass) == S_OK);

        D3D12MA::DEFRAGMENTATION_STATS defragStats = {};
        defragCtx->GetStats(&defragStats);
        CHECK_BOOL(defragStats.AllocationsMoved == 0 && defragStats.BytesFreed == 0 &&
            defragStats.BytesMoved == 0 && defragStats.HeapsFreed == 0);
    }

    D3D12_RANGE mapRange = {};
    void* mapPtr;
    std::vector<ComPtr<D3D12MA::Allocation>> allocations;

    // persistentlyMappedOption = 0 - not persistently mapped.
    // persistentlyMappedOption = 1 - persistently mapped.
    for (UINT8 persistentlyMappedOption = 0; persistentlyMappedOption < 2; ++persistentlyMappedOption)
    {
        wprintf(L"  Persistently mapped option = %u\n", persistentlyMappedOption);
        const bool persistentlyMapped = persistentlyMappedOption != 0;

        // # Test 1
        // Buffers of fixed size.
        // Fill 2 blocks. Remove odd buffers. Defragment everything.
        // Expected result: at least 1 block freed.
        {
            for (size_t i = 0; i < BLOCK_SIZE / BUF_SIZE * 2; ++i)
            {
                ComPtr<D3D12MA::Allocation> alloc;
                CHECK_HR(ctx.allocator->CreateResource(&allocDesc, &resDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
                    nullptr, &alloc, IID_NULL, nullptr));
                if (persistentlyMapped)
                {
                    CHECK_HR(alloc->GetResource()->Map(0, &mapRange, &mapPtr));
                }

                allocations.emplace_back(std::move(alloc));
            }

            for (size_t i = 1; i < allocations.size(); ++i)
                allocations.erase(allocations.begin() + i);
            FillAllocationsData(allocations.data(), allocations.size(), ALLOC_SEED);

            // Set data for defragmentation retrieval
            for (auto& alloc : allocations)
                alloc->SetPrivateData((void*)D3D12_RESOURCE_STATE_GENERIC_READ);

            D3D12MA::DEFRAGMENTATION_STATS defragStats;
            Defragment(ctx, defragDesc, pool.Get(), & defragStats);
            CHECK_BOOL(defragStats.AllocationsMoved == 4 && defragStats.BytesMoved == 4 * BUF_SIZE);

            ValidateAllocationsData(allocations.data(), allocations.size(), ALLOC_SEED);
            allocations.clear();
        }

        // # Test 2
        // Buffers of fixed size.
        // Fill 2 blocks. Remove odd buffers. Defragment one buffer at time.
        // Expected result: Each of 4 interations makes some progress.
        {
            for (size_t i = 0; i < BLOCK_SIZE / BUF_SIZE * 2; ++i)
            {
                ComPtr<D3D12MA::Allocation> alloc;
                CHECK_HR(ctx.allocator->CreateResource(&allocDesc, &resDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
                    nullptr, &alloc, IID_NULL, nullptr));
                if (persistentlyMapped)
                {
                    CHECK_HR(alloc->GetResource()->Map(0, &mapRange, &mapPtr));
                }

                allocations.emplace_back(std::move(alloc));
            }

            for (size_t i = 1; i < allocations.size(); ++i)
                allocations.erase(allocations.begin() + i);
            FillAllocationsData(allocations.data(), allocations.size(), ALLOC_SEED);

            // Set data for defragmentation retrieval
            for (auto& alloc : allocations)
                alloc->SetPrivateData((void*)D3D12_RESOURCE_STATE_GENERIC_READ);

            defragDesc.MaxAllocationsPerPass = 1;
            defragDesc.MaxBytesPerPass = BUF_SIZE;

            ComPtr<D3D12MA::DefragmentationContext> defragCtx;
            CHECK_HR(pool->BeginDefragmentation(&defragDesc, &defragCtx));

            for (size_t i = 0; i < BLOCK_SIZE / BUF_SIZE / 2; ++i)
            {
                D3D12MA::DEFRAGMENTATION_PASS_MOVE_INFO pass = {};
                CHECK_BOOL(defragCtx->BeginPass(&pass) == S_FALSE);

                ProcessDefragmentationPass(ctx, pass);

                CHECK_BOOL(defragCtx->EndPass(&pass) == S_FALSE);
            }

            D3D12MA::DEFRAGMENTATION_STATS defragStats = {};
            defragCtx->GetStats(&defragStats);
            CHECK_BOOL(defragStats.AllocationsMoved == 4 && defragStats.BytesMoved == 4 * BUF_SIZE);

            ValidateAllocationsData(allocations.data(), allocations.size(), ALLOC_SEED);
            allocations.clear();
        }

        // # Test 3
        // Buffers of variable size.
        // Create a number of buffers. Remove some percent of them.
        // Defragment while having some percent of them unmovable.
        // Expected result: Just simple validation.
        {
            for (size_t i = 0; i < 100; ++i)
            {
                D3D12_RESOURCE_DESC localResDesc = resDesc;
                localResDesc.Width = RandomBufSize();

                ComPtr<D3D12MA::Allocation> alloc;
                CHECK_HR(ctx.allocator->CreateResource(&allocDesc, &localResDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
                    nullptr, &alloc, IID_NULL, nullptr));
                if (persistentlyMapped)
                {
                    CHECK_HR(alloc->GetResource()->Map(0, &mapRange, &mapPtr));
                }

                allocations.emplace_back(std::move(alloc));
            }

            const UINT32 percentToDelete = 60;
            const size_t numberToDelete = allocations.size() * percentToDelete / 100;
            for (size_t i = 0; i < numberToDelete; ++i)
            {
                size_t indexToDelete = rand.Generate() % (UINT32)allocations.size();
                allocations.erase(allocations.begin() + indexToDelete);
            }
            FillAllocationsData(allocations.data(), allocations.size(), ALLOC_SEED);

            // Non-movable allocations will be at the beginning of allocations array.
            const UINT32 percentNonMovable = 20;
            const size_t numberNonMovable = allocations.size() * percentNonMovable / 100;
            for (size_t i = 0; i < numberNonMovable; ++i)
            {
                size_t indexNonMovable = i + rand.Generate() % (UINT32)(allocations.size() - i);
                if (indexNonMovable != i)
                    std::swap(allocations[i], allocations[indexNonMovable]);
            }

            // Set data for defragmentation retrieval
            for (auto& alloc : allocations)
                alloc->SetPrivateData((void*)D3D12_RESOURCE_STATE_GENERIC_READ);

            defragDesc.MaxAllocationsPerPass = 0;
            defragDesc.MaxBytesPerPass = 0;

            ComPtr<D3D12MA::DefragmentationContext> defragCtx;
            CHECK_HR(pool->BeginDefragmentation(&defragDesc, &defragCtx));

            HRESULT hr = S_OK;
            D3D12MA::DEFRAGMENTATION_PASS_MOVE_INFO pass = {};
            while ((hr = defragCtx->BeginPass(&pass)) == S_FALSE)
            {
                D3D12MA::DEFRAGMENTATION_MOVE* end = pass.pMoves + pass.MoveCount;
                for (UINT32 i = 0; i < numberNonMovable; ++i)
                {
                    D3D12MA::DEFRAGMENTATION_MOVE* move = std::find_if(pass.pMoves, end, [&](D3D12MA::DEFRAGMENTATION_MOVE& move) { return move.pSrcAllocation == allocations[i].Get(); });
                    if (move != end)
                        move->Operation = D3D12MA::DEFRAGMENTATION_MOVE_OPERATION_IGNORE;
                }

                ProcessDefragmentationPass(ctx, pass);

                if ((hr = defragCtx->EndPass(&pass)) == S_OK)
                    break;
                CHECK_BOOL(hr == S_FALSE);
            }
            CHECK_BOOL(hr == S_OK);

            ValidateAllocationsData(allocations.data(), allocations.size(), ALLOC_SEED);
            allocations.clear();
        }
    }
}

static void TestDefragmentationAlgorithms(const TestContext& ctx)
{
    wprintf(L"Test defragmentation algorithms\n");

    RandomNumberGenerator rand(669);

    const UINT ALLOC_SEED = 20091225;
    const UINT64 BUF_SIZE = 0x10000;
    const UINT64 BLOCK_SIZE = BUF_SIZE * 400;

    const UINT64 MIN_BUF_SIZE = 32;
    const UINT64 MAX_BUF_SIZE = BUF_SIZE * 4;
    auto RandomBufSize = [&]() -> UINT64
    {
        return AlignUp<UINT64>(rand.Generate() % (MAX_BUF_SIZE - MIN_BUF_SIZE + 1) + MIN_BUF_SIZE, 64);
    };

    D3D12MA::POOL_DESC poolDesc = {};
    poolDesc.BlockSize = BLOCK_SIZE;
    poolDesc.HeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
    poolDesc.HeapFlags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;
    ComPtr<D3D12MA::Pool> pool;
    CHECK_HR(ctx.allocator->CreatePool(&poolDesc, &pool));

    D3D12MA::ALLOCATION_DESC allocDesc = {};
    allocDesc.CustomPool = pool.Get();

    D3D12_RESOURCE_DESC resDesc = {};
    FillResourceDescForBuffer(resDesc, BUF_SIZE);

    D3D12MA::DEFRAGMENTATION_DESC defragDesc = {};

    std::vector<ComPtr<D3D12MA::Allocation>> allocations;

    for (UINT8 i = 0; i < 3; ++i)
    {
        switch (i)
        {
        case 0:
            defragDesc.Flags = D3D12MA::DEFRAGMENTATION_FLAG_ALGORITHM_FAST;
            break;
        case 1:
            defragDesc.Flags = D3D12MA::DEFRAGMENTATION_FLAG_ALGORITHM_BALANCED;
            break;
        case 2:
            defragDesc.Flags = D3D12MA::DEFRAGMENTATION_FLAG_ALGORITHM_FULL;
            break;
        }
        wprintf(L"  Algorithm = %s\n", DefragmentationAlgorithmToStr(defragDesc.Flags));

        // 0 - Without immovable allocations
        // 1 - With immovable allocations
        for (uint8_t j = 0; j < 2; ++j)
        {
            for (size_t i = 0; i < 800; ++i)
            {
                resDesc.Width = RandomBufSize();

                ComPtr<D3D12MA::Allocation> alloc;
                CHECK_HR(ctx.allocator->CreateResource(&allocDesc, &resDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
                    nullptr, &alloc, IID_NULL, nullptr));
                allocations.emplace_back(std::move(alloc));
            }

            const UINT32 percentToDelete = 55;
            const size_t numberToDelete = allocations.size() * percentToDelete / 100;
            for (size_t i = 0; i < numberToDelete; ++i)
            {
                size_t indexToDelete = rand.Generate() % (uint32_t)allocations.size();
                allocations.erase(allocations.begin() + indexToDelete);
            }
            FillAllocationsData(allocations.data(), allocations.size(), ALLOC_SEED);

            // Non-movable allocations will be at the beginning of allocations array.
            const UINT32 percentNonMovable = 20;
            const size_t numberNonMovable = j == 0 ? 0 : (allocations.size() * percentNonMovable / 100);
            for (size_t i = 0; i < numberNonMovable; ++i)
            {
                size_t indexNonMovable = i + rand.Generate() % (UINT32)(allocations.size() - i);
                if (indexNonMovable != i)
                    std::swap(allocations[i], allocations[indexNonMovable]);
            }

            // Set data for defragmentation retrieval
            for (auto& alloc : allocations)
                alloc->SetPrivateData((void*)D3D12_RESOURCE_STATE_GENERIC_READ);

            std::wstring output = DefragmentationAlgorithmToStr(defragDesc.Flags);
            if (j == 0)
                output += L"_NoMove";
            else
                output += L"_Move";
            SaveStatsStringToFile(ctx, (output + L"_Before.json").c_str());

            ComPtr<D3D12MA::DefragmentationContext> defragCtx;
            CHECK_HR(pool->BeginDefragmentation(&defragDesc, &defragCtx));

            HRESULT hr = S_OK;
            D3D12MA::DEFRAGMENTATION_PASS_MOVE_INFO pass = {};
            while ((hr = defragCtx->BeginPass(&pass)) == S_FALSE)
            {
                D3D12MA::DEFRAGMENTATION_MOVE* end = pass.pMoves + pass.MoveCount;
                for (UINT32 i = 0; i < numberNonMovable; ++i)
                {
                    D3D12MA::DEFRAGMENTATION_MOVE* move = std::find_if(pass.pMoves, end, [&](D3D12MA::DEFRAGMENTATION_MOVE& move) { return move.pSrcAllocation == allocations[i].Get(); });
                    if (move != end)
                        move->Operation = D3D12MA::DEFRAGMENTATION_MOVE_OPERATION_IGNORE;
                }
                for (UINT32 i = 0; i < pass.MoveCount; ++i)
                {
                    auto it = std::find_if(allocations.begin(), allocations.end(), [&](const ComPtr<D3D12MA::Allocation>& alloc) { return pass.pMoves[i].pSrcAllocation == alloc.Get(); });
                    assert(it != allocations.end());
                }

                ProcessDefragmentationPass(ctx, pass);

                if ((hr = defragCtx->EndPass(&pass)) == S_OK)
                    break;
                CHECK_BOOL(hr == S_FALSE);
            }
            CHECK_BOOL(hr == S_OK);

            SaveStatsStringToFile(ctx, (output + L"_After.json").c_str());
            ValidateAllocationsData(allocations.data(), allocations.size(), ALLOC_SEED);
            allocations.clear();
        }
    }
}

static void TestDefragmentationFull(const TestContext& ctx)
{
    const UINT ALLOC_SEED = 20101220;
    std::vector<ComPtr<D3D12MA::Allocation>> allocations;

    D3D12MA::ALLOCATION_DESC allocDesc = {};
    allocDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;
    allocDesc.ExtraHeapFlags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;

    D3D12_RESOURCE_DESC resDesc = {};
    FillResourceDescForBuffer(resDesc, 0x10000);

    // Create initial allocations.
    for (size_t i = 0; i < 400; ++i)
    {
        ComPtr<D3D12MA::Allocation> alloc;
        CHECK_HR(ctx.allocator->CreateResource(&allocDesc, &resDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr, &alloc, IID_NULL, nullptr));
        allocations.emplace_back(std::move(alloc));
    }
    FillAllocationsData(allocations.data(), allocations.size(), ALLOC_SEED);

    // Delete random allocations
    const size_t allocationsToDeletePercent = 80;
    size_t allocationsToDelete = allocations.size() * allocationsToDeletePercent / 100;
    for (size_t i = 0; i < allocationsToDelete; ++i)
    {
        size_t index = (size_t)rand() % allocations.size();
        allocations.erase(allocations.begin() + index);
    }
    SaveStatsStringToFile(ctx, L"FullBefore.json");

    {
        // Set data for defragmentation retrieval
        for (auto& alloc : allocations)
            alloc->SetPrivateData((void*)D3D12_RESOURCE_STATE_GENERIC_READ);

        const UINT32 defragCount = 1;
        for (UINT32 defragIndex = 0; defragIndex < defragCount; ++defragIndex)
        {
            D3D12MA::DEFRAGMENTATION_DESC defragDesc = {};
            defragDesc.Flags = D3D12MA::DEFRAGMENTATION_FLAG_ALGORITHM_FULL;

            wprintf(L"Test defragmentation full #%u\n", defragIndex);

            time_point begTime = std::chrono::high_resolution_clock::now();

            D3D12MA::DEFRAGMENTATION_STATS stats;
            Defragment(ctx, defragDesc, nullptr, &stats);

            float defragmentDuration = ToFloatSeconds(std::chrono::high_resolution_clock::now() - begTime);

            wprintf(L"Moved allocations %u, bytes %llu\n", stats.AllocationsMoved, stats.BytesMoved);
            wprintf(L"Freed blocks %u, bytes %llu\n", stats.HeapsFreed, stats.BytesFreed);
            wprintf(L"Time: %.2f s\n", defragmentDuration);

            SaveStatsStringToFile(ctx, (L"FullAfter_" + std::to_wstring(defragIndex) + L".json").c_str());
        }
    }

    ValidateAllocationsData(allocations.data(), allocations.size(), ALLOC_SEED);
}

static void TestDefragmentationGpu(const TestContext& ctx)
{
    wprintf(L"Test defragmentation GPU\n");

    const UINT ALLOC_SEED = 20180314;
    std::vector<ComPtr<D3D12MA::Allocation>> allocations;

    // Create that many allocations to surely fill 3 new blocks of 256 MB.
    const UINT64 bufSizeMin = 5ull * 1024 * 1024;
    const UINT64 bufSizeMax = 10ull * 1024 * 1024;
    const UINT64 totalSize = 3ull * 256 * 1024 * 1024;
    const size_t bufCount = (size_t)(totalSize / bufSizeMin);
    const size_t percentToLeave = 30;
    const size_t percentNonMovable = 3;
    RandomNumberGenerator rand = { 234522 };

    D3D12_RESOURCE_DESC resDesc = {};
    FillResourceDescForBuffer(resDesc, 0x10000);

    D3D12MA::ALLOCATION_DESC allocDesc = {};
    allocDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
    allocDesc.ExtraHeapFlags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;

    // Create all intended buffers.
    for (size_t i = 0; i < bufCount; ++i)
    {
        resDesc.Width = AlignUp(rand.Generate() % (bufSizeMax - bufSizeMin) + bufSizeMin, 32ull);

        ComPtr<D3D12MA::Allocation> alloc;
        CHECK_HR(ctx.allocator->CreateResource(&allocDesc, &resDesc, D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr, &alloc, IID_NULL, nullptr));
        allocations.emplace_back(std::move(alloc));
    }

    // Destroy some percentage of them.
    {
        const size_t buffersToDestroy = RoundDiv<size_t>(bufCount * (100 - percentToLeave), 100);
        for (size_t i = 0; i < buffersToDestroy; ++i)
        {
            const size_t index = rand.Generate() % allocations.size();
            allocations.erase(allocations.begin() + index);
        }
    }

    // Set data for defragmentation retrieval
    for (auto& alloc : allocations)
        alloc->SetPrivateData((void*)D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

    // Fill them with meaningful data.
    FillAllocationsDataGPU(ctx, allocations.data(), allocations.size(), ALLOC_SEED);

    SaveStatsStringToFile(ctx, L"GPU_defragmentation_A_before.json");
    // Defragment using GPU only.
    {
        const size_t numberNonMovable = allocations.size() * percentNonMovable / 100;
        for (size_t i = 0; i < numberNonMovable; ++i)
        {
            size_t indexNonMovable = i + rand.Generate() % (UINT32)(allocations.size() - i);
            if (indexNonMovable != i)
                std::swap(allocations[i], allocations[indexNonMovable]);
        }

        D3D12MA::DEFRAGMENTATION_DESC defragDesc = {};
        D3D12MA::DEFRAGMENTATION_STATS stats;
        Defragment(ctx, defragDesc, nullptr, &stats);

        CHECK_BOOL(stats.AllocationsMoved > 0 && stats.BytesMoved > 0);
        CHECK_BOOL(stats.HeapsFreed > 0 && stats.BytesFreed > 0);
    }

    SaveStatsStringToFile(ctx, L"GPU_defragmentation_B_after.json");
    ValidateAllocationsDataGPU(ctx, allocations.data(), allocations.size(), ALLOC_SEED);
}

static void TestDefragmentationIncrementalBasic(const TestContext& ctx)
{
    wprintf(L"Test defragmentation incremental basic\n");

    const UINT ALLOC_SEED = 20210918;
    std::vector<ComPtr<D3D12MA::Allocation>> allocations;

    // Create that many allocations to surely fill 3 new blocks of 256 MB.
    const std::array<UINT32, 3> imageSizes = { 256, 512, 1024 };
    const UINT64 bufSizeMin = 5ull * 1024 * 1024;
    const UINT64 bufSizeMax = 10ull * 1024 * 1024;
    const UINT64 totalSize = 3ull * 256 * 1024 * 1024;
    const size_t imageCount = totalSize / ((size_t)imageSizes[0] * imageSizes[0] * 4) / 2;
    const size_t bufCount = (size_t)(totalSize / bufSizeMin) / 2;
    const size_t percentToLeave = 30;
    RandomNumberGenerator rand = { 234522 };

    D3D12MA::ALLOCATION_DESC allocDesc = {};
    allocDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_RESOURCE_DESC resDesc = {};
    resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resDesc.Alignment = 0;
    resDesc.DepthOrArraySize = 1;
    resDesc.MipLevels = 1;
    resDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    resDesc.SampleDesc.Count = 1;
    resDesc.SampleDesc.Quality = 0;
    resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    // Create all intended images.
    for (size_t i = 0; i < imageCount; ++i)
    {
        const UINT32 size = imageSizes[rand.Generate() % 3];
        resDesc.Width = size;
        resDesc.Height = size;

        ComPtr<D3D12MA::Allocation> alloc;
        CHECK_HR(ctx.allocator->CreateResource(&allocDesc, &resDesc, D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr, &alloc, IID_NULL, nullptr));

        alloc->SetPrivateData((void*)D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        allocations.emplace_back(std::move(alloc));
    }

    // And all buffers
    FillResourceDescForBuffer(resDesc, 0x10000);
    for (size_t i = 0; i < bufCount; ++i)
    {
        resDesc.Width = AlignUp(rand.Generate() % (bufSizeMax - bufSizeMin) + bufSizeMin, 32ull);

        ComPtr<D3D12MA::Allocation> alloc;
        CHECK_HR(ctx.allocator->CreateResource(&allocDesc, &resDesc, D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr, &alloc, IID_NULL, nullptr));

        alloc->SetPrivateData((void*)D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        allocations.emplace_back(std::move(alloc));
    }

    // Destroy some percentage of them.
    {
        const size_t allocationsToDestroy = RoundDiv<size_t>((imageCount + bufCount) * (100 - percentToLeave), 100);
        for (size_t i = 0; i < allocationsToDestroy; ++i)
        {
            const size_t index = rand.Generate() % allocations.size();
            allocations.erase(allocations.begin() + index);
        }
    }

    // Fill them with meaningful data.
    FillAllocationsDataGPU(ctx, allocations.data(), allocations.size(), ALLOC_SEED);

    SaveStatsStringToFile(ctx, L"GPU_defragmentation_incremental_basic_A_before.json");
    // Defragment using GPU only.
    {
        D3D12MA::DEFRAGMENTATION_DESC defragDesc = {};
        ComPtr<D3D12MA::DefragmentationContext> defragCtx;
        ctx.allocator->BeginDefragmentation(&defragDesc, &defragCtx);

        HRESULT hr = S_OK;
        D3D12MA::DEFRAGMENTATION_PASS_MOVE_INFO pass = {};
        while ((hr = defragCtx->BeginPass(&pass)) == S_FALSE)
        {
            // Ignore data outside of test
            for (UINT32 i = 0; i < pass.MoveCount; ++i)
            {
                auto it = std::find_if(allocations.begin(), allocations.end(), [&](const ComPtr<D3D12MA::Allocation>& alloc) { return pass.pMoves[i].pSrcAllocation == alloc.Get(); });
                if (it == allocations.end())
                    pass.pMoves[i].Operation = D3D12MA::DEFRAGMENTATION_MOVE_OPERATION_IGNORE;
            }

            ProcessDefragmentationPass(ctx, pass);

            if ((hr = defragCtx->EndPass(&pass)) == S_OK)
                break;
            CHECK_BOOL(hr == S_FALSE);
        }
        CHECK_BOOL(hr == S_OK);

        D3D12MA::DEFRAGMENTATION_STATS stats = {};
        defragCtx->GetStats(&stats);
        CHECK_BOOL(stats.AllocationsMoved > 0 && stats.BytesMoved > 0);
        CHECK_BOOL(stats.HeapsFreed > 0 && stats.BytesFreed > 0);
    }

    SaveStatsStringToFile(ctx, L"GPU_defragmentation_incremental_basic_B_after.json");
    ValidateAllocationsDataGPU(ctx, allocations.data(), allocations.size(), ALLOC_SEED);
}

void TestDefragmentationIncrementalComplex(const TestContext& ctx)
{
    wprintf(L"Test defragmentation incremental complex\n");

    const UINT ALLOC_SEED = 20180112;
    std::vector<ComPtr<D3D12MA::Allocation>> allocations;

    // Create that many allocations to surely fill 3 new blocks of 256 MB.
    const std::array<UINT32, 3> imageSizes = { 256, 512, 1024 };
    const UINT64 bufSizeMin = 5ull * 1024 * 1024;
    const UINT64 bufSizeMax = 10ull * 1024 * 1024;
    const UINT64 totalSize = 3ull * 256 * 1024 * 1024;
    const size_t imageCount = (size_t)(totalSize / (imageSizes[0] * imageSizes[0] * 4)) / 2;
    const size_t bufCount = (size_t)(totalSize / bufSizeMin) / 2;
    const size_t percentToLeave = 30;
    RandomNumberGenerator rand = { 234522 };

    D3D12MA::ALLOCATION_DESC allocDesc = {};
    allocDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_RESOURCE_DESC resDesc = {};
    resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resDesc.Alignment = 0;
    resDesc.DepthOrArraySize = 1;
    resDesc.MipLevels = 1;
    resDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    resDesc.SampleDesc.Count = 1;
    resDesc.SampleDesc.Quality = 0;
    resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    // Create all intended images.
    for (size_t i = 0; i < imageCount; ++i)
    {
        const UINT32 size = imageSizes[rand.Generate() % 3];
        resDesc.Width = size;
        resDesc.Height = size;

        ComPtr<D3D12MA::Allocation> alloc;
        CHECK_HR(ctx.allocator->CreateResource(&allocDesc, &resDesc, D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr, &alloc, IID_NULL, nullptr));

        alloc->SetPrivateData((void*)D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        allocations.emplace_back(std::move(alloc));
    }

    // And all buffers
    FillResourceDescForBuffer(resDesc, 0x10000);
    for (size_t i = 0; i < bufCount; ++i)
    {
        resDesc.Width = AlignUp(rand.Generate() % (bufSizeMax - bufSizeMin) + bufSizeMin, 32ull);

        ComPtr<D3D12MA::Allocation> alloc;
        CHECK_HR(ctx.allocator->CreateResource(&allocDesc, &resDesc, D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr, &alloc, IID_NULL, nullptr));

        alloc->SetPrivateData((void*)D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        allocations.emplace_back(std::move(alloc));
    }

    // Destroy some percentage of them.
    {
        const size_t allocationsToDestroy = RoundDiv<size_t>((imageCount + bufCount) * (100 - percentToLeave), 100);
        for (size_t i = 0; i < allocationsToDestroy; ++i)
        {
            const size_t index = rand.Generate() % allocations.size();
            allocations.erase(allocations.begin() + index);
        }
    }

    // Fill them with meaningful data.
    FillAllocationsDataGPU(ctx, allocations.data(), allocations.size(), ALLOC_SEED);

    SaveStatsStringToFile(ctx, L"GPU_defragmentation_incremental_complex_A_before.json");

    const size_t maxAdditionalAllocations = 100;
    std::vector<ComPtr<D3D12MA::Allocation>> additionalAllocations;
    additionalAllocations.reserve(maxAdditionalAllocations);

    const auto makeAdditionalAllocation = [&]()
    {
        if (additionalAllocations.size() < maxAdditionalAllocations)
        {
            resDesc.Width = AlignUp(bufSizeMin + rand.Generate() % (bufSizeMax - bufSizeMin), 16ull);
            ComPtr<D3D12MA::Allocation> alloc;
            CHECK_HR(ctx.allocator->CreateResource(&allocDesc, &resDesc, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
                nullptr, &alloc, IID_NULL, nullptr));
            alloc->SetPrivateData((void*)D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
            additionalAllocations.emplace_back(std::move(alloc));
        }
    };

    // Defragment using GPU only.
    {
        D3D12MA::DEFRAGMENTATION_DESC defragDesc = {};
        defragDesc.Flags = D3D12MA::DEFRAGMENTATION_FLAG_ALGORITHM_FULL;

        ComPtr<D3D12MA::DefragmentationContext> defragCtx;
        ctx.allocator->BeginDefragmentation(&defragDesc, &defragCtx);

        makeAdditionalAllocation();

        HRESULT hr = S_OK;
        D3D12MA::DEFRAGMENTATION_PASS_MOVE_INFO pass = {};
        while ((hr = defragCtx->BeginPass(&pass)) == S_FALSE)
        {
            makeAdditionalAllocation();

            // Ignore data outside of test
            for (UINT32 i = 0; i < pass.MoveCount; ++i)
            {
                auto it = std::find_if(allocations.begin(), allocations.end(), [&](const ComPtr<D3D12MA::Allocation>& alloc) { return pass.pMoves[i].pSrcAllocation == alloc.Get(); });
                if (it == allocations.end())
                {
                    auto it = std::find_if(additionalAllocations.begin(), additionalAllocations.end(), [&](const ComPtr<D3D12MA::Allocation>& alloc) { return pass.pMoves[i].pSrcAllocation == alloc.Get(); });
                    if (it == additionalAllocations.end())
                        pass.pMoves[i].Operation = D3D12MA::DEFRAGMENTATION_MOVE_OPERATION_IGNORE;
                }
            }

            ProcessDefragmentationPass(ctx, pass);

            makeAdditionalAllocation();

            if ((hr = defragCtx->EndPass(&pass)) == S_OK)
                break;
            CHECK_BOOL(hr == S_FALSE);
        }
        CHECK_BOOL(hr == S_OK);

        D3D12MA::DEFRAGMENTATION_STATS stats = {};
        defragCtx->GetStats(&stats);

        CHECK_BOOL(stats.AllocationsMoved > 0 && stats.BytesMoved > 0);
        CHECK_BOOL(stats.HeapsFreed > 0 && stats.BytesFreed > 0);
    }

    SaveStatsStringToFile(ctx, L"GPU_defragmentation_incremental_complex_B_after.json");
    ValidateAllocationsDataGPU(ctx, allocations.data(), allocations.size(), ALLOC_SEED);
}

static void TestGroupVirtual(const TestContext& ctx)
{
    TestVirtualBlocks(ctx);
    TestVirtualBlocksAlgorithms(ctx);
    TestVirtualBlocksAlgorithmsBenchmark(ctx);
}

static void TestGroupBasics(const TestContext& ctx)
{
#if D3D12MA_DEBUG_MARGIN
    TestDebugMargin(ctx);
    TestDebugMarginNotInVirtualAllocator(ctx);
#else
    TestJson(ctx);
    TestCommittedResourcesAndJson(ctx);
    TestSmallBuffers(ctx);
    TestCustomHeapFlags(ctx);
    TestPlacedResources(ctx);
    TestOtherComInterface(ctx);
    TestCustomPools(ctx);
    TestCustomPool_MinAllocationAlignment(ctx);
    TestCustomPool_Committed(ctx);
    TestPoolsAndAllocationParameters(ctx);
    TestCustomHeaps(ctx);
    TestStandardCustomCommittedPlaced(ctx);
    TestAliasingMemory(ctx);
    TestAliasingImplicitCommitted(ctx);
    TestPoolMsaaTextureAsCommitted(ctx);
    TestMapping(ctx);
    TestStats(ctx);
    TestTransfer(ctx);
    TestZeroInitialized(ctx);
    TestMultithreading(ctx);
    TestLinearAllocator(ctx);
    TestLinearAllocatorMultiBlock(ctx);
    ManuallyTestLinearAllocator(ctx);
#ifdef __ID3D12Device4_INTERFACE_DEFINED__
    TestDevice4(ctx);
#endif
#ifdef __ID3D12Device8_INTERFACE_DEFINED__
    TestDevice8(ctx);
#endif
#ifdef __ID3D12Device10_INTERFACE_DEFINED__
    TestDevice10(ctx);
#endif

    FILE* file;
    fopen_s(&file, "Results.csv", "w");
    assert(file != NULL);
    BenchmarkAlgorithms(ctx, file);
    fclose(file);
#endif // #if D3D12_DEBUG_MARGIN
}

static void TestGroupDefragmentation(const TestContext& ctx)
{
    TestDefragmentationSimple(ctx);
    TestDefragmentationAlgorithms(ctx);
    TestDefragmentationFull(ctx);
    TestDefragmentationGpu(ctx);
    TestDefragmentationIncrementalBasic(ctx);
    TestDefragmentationIncrementalComplex(ctx);
}

void Test(const TestContext& ctx)
{
    wprintf(L"TESTS BEGIN\n");

    if(false)
    {
        ////////////////////////////////////////////////////////////////////////////////
        // Temporarily insert custom tests here:
        return;
    }

    TestGroupVirtual(ctx);
    TestGroupBasics(ctx);
    TestGroupDefragmentation(ctx);

    wprintf(L"TESTS END\n");
}
