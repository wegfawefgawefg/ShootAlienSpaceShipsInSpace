cmake_minimum_required(VERSION 3.20)

project(ShootAlienSpaceShipsInSpace LANGUAGES CXX)

option(SASSIS_REQUIRE_DEPS "Fail CMake configure if deps are missing" ON)
option(SASSIS_STRICT "Enable very strict warnings" ON)
option(SASSIS_WARN_AS_ERROR "Treat warnings as errors" OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(SASSIS_SOURCES
  "${CMAKE_SOURCE_DIR}/src/main.cpp"
  "${CMAKE_SOURCE_DIR}/src/app.cpp"
  "${CMAKE_SOURCE_DIR}/src/assets.cpp"
  "${CMAKE_SOURCE_DIR}/src/battle_render.cpp"
  "${CMAKE_SOURCE_DIR}/src/damage.cpp"
  "${CMAKE_SOURCE_DIR}/src/enemy_behavior.cpp"
  "${CMAKE_SOURCE_DIR}/src/level_data.cpp"
  "${CMAKE_SOURCE_DIR}/src/level_flow.cpp"
  "${CMAKE_SOURCE_DIR}/src/player_weapons.cpp"
)

set(SASSIS_HEADERS
  "${CMAKE_SOURCE_DIR}/src/app.hpp"
  "${CMAKE_SOURCE_DIR}/src/assets.hpp"
  "${CMAKE_SOURCE_DIR}/src/battle_render.hpp"
  "${CMAKE_SOURCE_DIR}/src/battle_types.hpp"
  "${CMAKE_SOURCE_DIR}/src/damage.hpp"
  "${CMAKE_SOURCE_DIR}/src/enemy_behavior.hpp"
  "${CMAKE_SOURCE_DIR}/src/level_data.hpp"
  "${CMAKE_SOURCE_DIR}/src/level_flow.hpp"
  "${CMAKE_SOURCE_DIR}/src/math.hpp"
  "${CMAKE_SOURCE_DIR}/src/player_weapons.hpp"
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
      -Wformat=2 -Wundef -Wnull-dereference
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

find_package(PkgConfig REQUIRED)

pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)
pkg_check_modules(SDL2_IMAGE REQUIRED IMPORTED_TARGET SDL2_image)
pkg_check_modules(SDL2_MIXER REQUIRED IMPORTED_TARGET SDL2_mixer)
pkg_check_modules(SDL2_TTF REQUIRED IMPORTED_TARGET SDL2_ttf)

target_link_libraries(ShootAlienSpaceShipsInSpace PRIVATE
  PkgConfig::SDL2
  PkgConfig::SDL2_IMAGE
  PkgConfig::SDL2_MIXER
  PkgConfig::SDL2_TTF
)

target_include_directories(ShootAlienSpaceShipsInSpace PRIVATE
  "${CMAKE_SOURCE_DIR}/src"
)
