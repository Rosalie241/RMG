set(D3D12MA_LIBRARY_SOURCE_FILES
    D3D12MemAlloc.cpp
    "${PROJECT_SOURCE_DIR}/include/D3D12MemAlloc.h"
)

if(WIN32 AND ${CMAKE_GENERATOR} MATCHES "Visual Studio.*")
    set(D3D12MA_LIBRARY_SOURCE_FILES ${D3D12MA_LIBRARY_SOURCE_FILES} D3D12MemAlloc.natvis)
endif()

add_library(D3D12MemoryAllocator ${D3D12MA_LIBRARY_SOURCE_FILES})

set_target_properties(
    D3D12MemoryAllocator PROPERTIES

    CXX_EXTENSIONS OFF
    # Use C++14
    CXX_STANDARD 14
    CXX_STANDARD_REQUIRED ON

    OUTPUT_NAME "D3D12MA"
    # Postfix for different profiles
    DEBUG_POSTFIX "d"
    RELWITHDEBINFO_POSTFIX "rd"
    MINSIZEREL_POSTFIX "s"
)

target_include_directories(D3D12MemoryAllocator PUBLIC
    "${PROJECT_SOURCE_DIR}/include"
)

target_link_libraries(D3D12MemoryAllocator PUBLIC
    d3d12.lib
    dxgi.lib
    dxguid.lib
)

if(BUILD_SHARED_LIBS)
    target_compile_definitions(D3D12MemoryAllocator PRIVATE
        D3D12MA_EXPORTS
    )

    target_compile_definitions(D3D12MemoryAllocator INTERFACE
        D3D12MA_IMPORTS
    )
endif()

install(TARGETS D3D12MemoryAllocator
		RUNTIME DESTINATION "bin"
        	ARCHIVE DESTINATION "lib"
        	LIBRARY DESTINATION "lib")
install(FILES "${PROJECT_SOURCE_DIR}/include/D3D12MemAlloc.h" DESTINATION "include")

if(D3D12MA_BUILD_SAMPLE)
    if(WIN32)
        set(SHADER_DIR "Shaders")
        
        set(D3D12_SAMPLE_SOURCE_FILES
            Common.cpp
            Common.h
            Tests.cpp
            Tests.h
            D3D12Sample.cpp
        )
		
		set(VERTEX_SHADERS 
			"${SHADER_DIR}/VS.hlsl"
		)
				
		set(PIXEL_SHADERS
			"${SHADER_DIR}/PS.hlsl"
		)

		set( SHADERS
			${VERTEX_SHADERS}
			${PIXEL_SHADERS}
		)

		source_group("Resources\\Shaders" FILES ${SHADERS})

		set_source_files_properties(${VERTEX_SHADERS}
			PROPERTIES 
				VS_SHADER_TYPE Vertex
				VS_SETTINGS "ExcludedFromBuild=true"
		)

		set_source_files_properties( ${PIXEL_SHADERS}
			PROPERTIES 
				VS_SHADER_TYPE Pixel
				VS_SETTINGS "ExcludedFromBuild=true"
		)	

        add_executable(D3D12Sample ${D3D12_SAMPLE_SOURCE_FILES} ${SHADERS})

		add_dependencies(D3D12Sample D3D12MemoryAllocator)

        # Visual Studio specific settings
        if(${CMAKE_GENERATOR} MATCHES "Visual Studio.*")
            # Use Unicode instead of multibyte set
            add_compile_definitions(UNICODE _UNICODE)
            
            # Set VmaSample as startup project
            set_property(DIRECTORY ${PROJECT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT "D3D12Sample")
            
            # Enable multithreaded compiling
            target_compile_options(D3D12Sample PRIVATE "/MP")

            # Set working directory for Visual Studio debugger
            set_target_properties(
                D3D12Sample
                PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/bin"
            )
        endif()

        set_target_properties(
            D3D12Sample PROPERTIES

            CXX_EXTENSIONS OFF
            # Use C++14
            CXX_STANDARD 14
            CXX_STANDARD_REQUIRED ON
        )

        target_link_libraries(
            D3D12Sample

            PRIVATE D3D12MemoryAllocator
			PUBLIC d3d12.lib
			PUBLIC dxgi.lib
			PUBLIC dxguid.lib
			PUBLIC Shlwapi.lib
        )
    else()
        message(STATUS "D3D12Sample application is not supported to Linux")
    endif()
endif()

set(D3D12MA_AGILITY_SDK_DIRECTORY "" CACHE STRING "Path to unpacked DX12 Agility SDK. Leave empty to compile without it.")
option(D3D12MA_AGILITY_SDK_PREVIEW "Set if DX12 Agility SDK is preview version." OFF)
if(D3D12MA_AGILITY_SDK_DIRECTORY)
    if(EXISTS "${D3D12MA_AGILITY_SDK_DIRECTORY}/build/native/include/d3d12.h")
        message(STATUS "DX12 Agility SDK used from \"${D3D12MA_AGILITY_SDK_DIRECTORY}\".")
        target_compile_definitions(D3D12MemoryAllocator PRIVATE D3D12MA_USE_AGILITY_SDK=1)
        target_include_directories(D3D12MemoryAllocator BEFORE PRIVATE "${D3D12MA_AGILITY_SDK_DIRECTORY}/build/native/include")
        if(D3D12MA_AGILITY_SDK_PREVIEW)
            target_compile_definitions(D3D12MemoryAllocator PRIVATE D3D12MA_USE_AGILITY_SDK_PREVIEW=1)
        endif()
        if(${D3D12MA_BUILD_SAMPLE} AND ${WIN32})
            target_compile_definitions(D3D12Sample PRIVATE D3D12MA_USE_AGILITY_SDK=1)
            target_include_directories(D3D12Sample BEFORE PRIVATE "${D3D12MA_AGILITY_SDK_DIRECTORY}/build/native/include")
            if(D3D12MA_AGILITY_SDK_PREVIEW)
                target_compile_definitions(D3D12Sample PRIVATE D3D12MA_USE_AGILITY_SDK_PREVIEW=1)
            endif()
        endif()
    else()
        message(FATAL_ERROR "DX12 Agility SDK not found - cannot find file \"${D3D12MA_AGILITY_SDK_DIRECTORY}/build/native/include/d3d12.h\".")
    endif()
else()
    message(STATUS "DX12 Agility SDK not used.")
endif()
