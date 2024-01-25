set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/submodules/vcpkg/scripts/buildsystems/vcpkg.cmake CACHE PATH "vcpkg toolchain file")
set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/submodules/cmake/ios.toolchain.cmake CACHE PATH "ios toolchain file")

if (PLATFORM_MAC)
    set(PLATFORM MAC_ARM64)
elseif (PLATFORM_IOS)
    set(PLATFORM OS64)
endif ()