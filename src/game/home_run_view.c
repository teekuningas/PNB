#include "home_run_view.h"

void home_run_view_after_tick(HomeRunView* view, const TickFacts* facts)
{
    if (facts->pitch_released) view->active = 0;
    if (facts->homeward_run_started) view->active = 1;
    if (view->active && view->hold_ticks == -1 && facts->ball_home &&
        facts->third_base_control != THIRD_CONTROLLER_OFF_BASE) {
        view->hold_ticks = 0;
    }

    if (facts->world_reset) {
        *view = (HomeRunView){.hold_ticks = -1};
        return;
    }

    if (facts->enforcement_completed) {
        if (view->hold_ticks >= 0 && ++view->hold_ticks > 50) {
            *view = (HomeRunView){.hold_ticks = -1};
        }
        if (facts->homeward_run_started_in_enforcement) view->active = 1;
    }
}
