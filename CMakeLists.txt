cmake_minimum_required(VERSION 3.20)

project(ShootAlienSpaceShipsInSpace LANGUAGES CXX)

option(SASSIS_REQUIRE_DEPS "Fail CMake configure if deps are missing" ON)
option(SASSIS_STRICT "Enable very strict warnings" ON)
option(SASSIS_WARN_AS_ERROR "Treat warnings as errors" OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB_RECURSE SASSIS_SOURCES CONFIGURE_DEPENDS
  "${CMAKE_SOURCE_DIR}/src/*.cpp"
  "${CMAKE_SOURCE_DIR}/src/*.cxx"
  "${CMAKE_SOURCE_DIR}/src/*.cc"
)
file(GLOB_RECURSE SASSIS_HEADERS CONFIGURE_DEPENDS
  "${CMAKE_SOURCE_DIR}/src/*.hpp"
  "${CMAKE_SOURCE_DIR}/src/*.h"
)

add_executable(ShootAlienSpaceShipsInSpace ${SASSIS_SOURCES} ${SASSIS_HEADERS})

function(target_enable_strict_warnings tgt)
  if (MSVC)
    target_compile_options(${tgt} PRIVATE
      /W4
      /permissive-
      $<$<BOOL:${SASSIS_WARN_AS_ERROR}>:/WX>
    )
  else()
    target_compile_options(${tgt} PRIVATE
      -Wall -Wextra -Wpedantic
      -Wshadow -Wconversion -Wsign-conversion -Wdouble-promotion
      -Wformat=2 -Wundef -Wnull-dereference -Wnon-virtual-dtor
      -Woverloaded-virtual -Wimplicit-fallthrough
      $<$<BOOL:${SASSIS_WARN_AS_ERROR}>:-Werror>
    )
  endif()
endfunction()

if (SASSIS_STRICT)
  target_enable_strict_warnings(ShootAlienSpaceShipsInSpace)
else()
  if (MSVC)
    target_compile_options(ShootAlienSpaceShipsInSpace PRIVATE /W3)
  else()
    target_compile_options(ShootAlienSpaceShipsInSpace PRIVATE -Wall -Wextra)
  endif()
endif()

find_package(PkgConfig QUIET)

set(SDL2_FOUND_LOCAL OFF)
find_package(SDL2 CONFIG QUIET)
if (SDL2_FOUND)
  set(SDL2_FOUND_LOCAL ON)
  target_link_libraries(ShootAlienSpaceShipsInSpace PRIVATE SDL2::SDL2)
elseif(PkgConfig_FOUND)
  pkg_check_modules(SDL2 QUIET IMPORTED_TARGET sdl2)
  if (SDL2_FOUND)
    set(SDL2_FOUND_LOCAL ON)
    target_link_libraries(ShootAlienSpaceShipsInSpace PRIVATE PkgConfig::SDL2)
  endif()
endif()
if (NOT SDL2_FOUND_LOCAL)
  message(FATAL_ERROR "SDL2 not found. Install libsdl2-dev.")
endif()

set(GLM_FOUND_LOCAL OFF)
find_package(glm CONFIG QUIET)
if (glm_FOUND)
  set(GLM_FOUND_LOCAL ON)
  target_link_libraries(ShootAlienSpaceShipsInSpace PRIVATE glm::glm)
elseif(PkgConfig_FOUND)
  pkg_check_modules(GLM QUIET IMPORTED_TARGET glm)
  if (GLM_FOUND)
    set(GLM_FOUND_LOCAL ON)
    target_link_libraries(ShootAlienSpaceShipsInSpace PRIVATE PkgConfig::GLM)
  endif()
endif()
if (NOT GLM_FOUND_LOCAL AND SASSIS_REQUIRE_DEPS)
  message(FATAL_ERROR "glm not found.")
endif()

set(SDL2_TTF_FOUND_LOCAL OFF)
find_package(SDL2_ttf CONFIG QUIET)
if (SDL2_ttf_FOUND)
  set(SDL2_TTF_FOUND_LOCAL ON)
  target_link_libraries(ShootAlienSpaceShipsInSpace PRIVATE SDL2_ttf::SDL2_ttf)
elseif(PkgConfig_FOUND)
  pkg_check_modules(SDL2_TTF QUIET IMPORTED_TARGET SDL2_ttf)
  if (SDL2_TTF_FOUND)
    set(SDL2_TTF_FOUND_LOCAL ON)
    target_link_libraries(ShootAlienSpaceShipsInSpace PRIVATE PkgConfig::SDL2_TTF)
  endif()
endif()
if (NOT SDL2_TTF_FOUND_LOCAL)
  message(WARNING "SDL2_ttf not found. Text rendering may fail at runtime.")
endif()

set(SDL2_IMAGE_FOUND_LOCAL OFF)
if (PkgConfig_FOUND)
  pkg_check_modules(SDL2_IMAGE QUIET IMPORTED_TARGET SDL2_image)
  if (SDL2_IMAGE_FOUND)
    set(SDL2_IMAGE_FOUND_LOCAL ON)
    target_link_libraries(ShootAlienSpaceShipsInSpace PRIVATE PkgConfig::SDL2_IMAGE)
  endif()
endif()
if (NOT SDL2_IMAGE_FOUND_LOCAL AND SASSIS_REQUIRE_DEPS)
  message(FATAL_ERROR "SDL2_image not found. Install libsdl2-image-dev.")
endif()

set(SDL2_MIXER_FOUND_LOCAL OFF)
if (PkgConfig_FOUND)
  pkg_check_modules(SDL2_MIXER QUIET IMPORTED_TARGET SDL2_mixer)
  if (SDL2_MIXER_FOUND)
    set(SDL2_MIXER_FOUND_LOCAL ON)
    target_link_libraries(ShootAlienSpaceShipsInSpace PRIVATE PkgConfig::SDL2_MIXER)
  endif()
endif()
if (NOT SDL2_MIXER_FOUND_LOCAL AND SASSIS_REQUIRE_DEPS)
  message(FATAL_ERROR "SDL2_mixer not found. Install libsdl2-mixer-dev.")
endif()

set(LUA_FOUND_LOCAL OFF)
if (PkgConfig_FOUND)
  pkg_check_modules(LUA54 QUIET IMPORTED_TARGET lua5.4)
  if (LUA54_FOUND)
    set(LUA_FOUND_LOCAL ON)
    target_link_libraries(ShootAlienSpaceShipsInSpace PRIVATE PkgConfig::LUA54)
  else()
    pkg_check_modules(LUA_ALT QUIET IMPORTED_TARGET lua-5.4 lua54 lua)
    if (LUA_ALT_FOUND)
      set(LUA_FOUND_LOCAL ON)
      target_link_libraries(ShootAlienSpaceShipsInSpace PRIVATE PkgConfig::LUA_ALT)
    endif()
  endif()
endif()
if (NOT LUA_FOUND_LOCAL)
  message(FATAL_ERROR "Lua 5.4 not found. Install liblua5.4-dev.")
endif()

if (EXISTS "${CMAKE_SOURCE_DIR}/sol/sol.hpp")
  target_compile_definitions(ShootAlienSpaceShipsInSpace PRIVATE GUB_USE_SOL2=1)
  target_include_directories(ShootAlienSpaceShipsInSpace SYSTEM PRIVATE "${CMAKE_SOURCE_DIR}")
endif()

target_include_directories(ShootAlienSpaceShipsInSpace PRIVATE
  "${CMAKE_SOURCE_DIR}/src"
)
