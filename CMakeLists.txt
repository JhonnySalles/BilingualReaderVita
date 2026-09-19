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
)

set(SOURCES
  src/main.cpp
  src/ui_components.cpp
  src/file_browser.cpp
  src/reader_txt.cpp
  src/reader_cbz.cpp
  src/reader_epub.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME}
  vita2d
  freetype
  png
  jpeg
  z
  m
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
)
