#pragma once

#include "audio.hpp"
#include "graphics.hpp"
#include "input.hpp"
#include "luamgr.hpp"
#include "mods.hpp"
#include "runtime_settings.hpp"
#include "sprites.hpp"
#include "state.hpp"

// Shared pointers to systems used across modules.
extern State* ss;
extern Graphics* gg;
extern Audio* aa;

extern ModManager* mm;
extern LuaManager* luam;
