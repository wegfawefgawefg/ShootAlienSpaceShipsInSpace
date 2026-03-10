#include "globals.hpp"
#include "progression.hpp"
#include "settings.hpp"
#include "step.hpp"

#include <algorithm>

void step_next_stage() {
    if (!ss)
        return;
    if (ss->score_ready_timer > 0.0f)
        ss->score_ready_timer = std::max(0.0f, ss->score_ready_timer - TIMESTEP);
    process_next_stage_enter();
}
