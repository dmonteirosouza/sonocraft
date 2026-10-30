# Build the codec for the actual device/simulator architecture, rather than
# linking legacy mobile binaries built for a different SDK.
include(FetchContent)
set(OPUS_BUILD_PROGRAMS OFF CACHE BOOL "" FORCE)
set(OPUS_BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(OPUS_INSTALL_PKG_CONFIG_MODULE OFF CACHE BOOL "" FORCE)
set(OPUS_INSTALL_CMAKE_CONFIG_MODULE OFF CACHE BOOL "" FORCE)
FetchContent_Declare(opus
    URL https://downloads.xiph.org/releases/opus/opus-1.5.2.tar.gz
    URL_HASH SHA256=65c1d2f78b9f2fb20082c38cbe47c951ad5839345876e46941612ee87f9a7ce1)
FetchContent_MakeAvailable(opus)
set_target_properties(opus PROPERTIES POSITION_INDEPENDENT_CODE ON)

# AOO uses installed-style <opus/...> includes.
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/mobile-opus-include/opus")
file(GLOB opus_public_headers "${opus_SOURCE_DIR}/include/*.h")
file(COPY ${opus_public_headers} DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/mobile-opus-include/opus")
