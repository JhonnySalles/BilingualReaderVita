cmake_minimum_required(VERSION 3.10)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

project(BilingualReaderVita)
include("${VITASDK}/share/vita.cmake" REQUIRED)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wall -std=c++17")

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR}/libs/mupdf/include
  ${CMAKE_CURRENT_SOURCE_DIR}/libs/unrar
)

link_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/libs/mupdf/lib
)

set(UNRAR_SOURCES
  libs/unrar/rar.cpp libs/unrar/strlist.cpp libs/unrar/strfn.cpp libs/unrar/pathfn.cpp
  libs/unrar/smallfn.cpp libs/unrar/global.cpp libs/unrar/file.cpp libs/unrar/filefn.cpp
  libs/unrar/filcreat.cpp libs/unrar/archive.cpp libs/unrar/arcread.cpp libs/unrar/unicode.cpp
  libs/unrar/system.cpp libs/unrar/crypt.cpp libs/unrar/crc.cpp libs/unrar/rawread.cpp
  libs/unrar/encname.cpp libs/unrar/resource.cpp libs/unrar/match.cpp libs/unrar/timefn.cpp
  libs/unrar/rdwrfn.cpp libs/unrar/consio.cpp libs/unrar/options.cpp libs/unrar/errhnd.cpp
  libs/unrar/rarvm.cpp libs/unrar/secpassword.cpp libs/unrar/rijndael.cpp libs/unrar/getbits.cpp
  libs/unrar/sha1.cpp libs/unrar/sha256.cpp libs/unrar/blake2s.cpp libs/unrar/hash.cpp
  libs/unrar/extinfo.cpp libs/unrar/extract.cpp libs/unrar/volume.cpp libs/unrar/list.cpp
  libs/unrar/find.cpp libs/unrar/unpack.cpp libs/unrar/headers.cpp libs/unrar/threadpool.cpp
  libs/unrar/rs16.cpp libs/unrar/cmddata.cpp libs/unrar/ui.cpp libs/unrar/largepage.cpp
  libs/unrar/filestr.cpp libs/unrar/scantree.cpp libs/unrar/dll.cpp libs/unrar/qopen.cpp
)

add_library(unrar STATIC ${UNRAR_SOURCES})
target_compile_definitions(unrar PRIVATE LITTLE_ENDIAN RARDLL SILENT _FILE_OFFSET_BITS=64 _LARGEFILE_SOURCE RAR_SMP)
target_compile_options(unrar PRIVATE -Wno-switch)


set(SOURCES
  src/main.cpp
  src/ui_components.cpp
  src/file_browser.cpp
  src/file_utils.cpp
  src/cache_manager.cpp
  src/parses/parse_tar.cpp
  src/parses/parse_rar.cpp
  src/reader_txt.cpp
  src/reader_cbz.cpp
  src/reader_epub.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME}
  unrar
  mupdf
  mupdf-third
  vita2d
  freetype
  png
  jpeg
  z
  m
  pthread
  SceDisplay_stub
  SceGxm_stub
  SceCtrl_stub
  SceTouch_stub
  ScePower_stub
  SceSysmodule_stub
  SceAppMgr_stub
  SceCommonDialog_stub
  ScePgf_stub
  ScePvf_stub
  SceIme_stub
)

vita_create_self(eboot.bin ${PROJECT_NAME} UNSAFE)
vita_create_vpk(${PROJECT_NAME}.vpk "BREADPSV" eboot.bin
  VERSION "01.00"
  NAME "Bilingual Reader"
  FILE sce_sys/icon0.png sce_sys/icon0.png
  FILE assets/images/app_icon.png assets/images/app_icon.png
  FILE assets/images/app_icon_small.png assets/images/app_icon_small.png
)
