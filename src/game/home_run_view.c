#include "home_run_view.h"

void home_run_view_after_tick(HomeRunView* view, const TickFacts* facts)
{
    if (facts->pitch_released) {
        view->active = 0;
        view->pending = 0;
    }

    // Confirm or cancel a homeward start from the previous tick before recording a new start.
    // A runner who broke on pitch release and snapped straight back onto third (e.g. a väärä
    // recall within one tick) never visibly left third, so the camera stays level.
    if (view->pending) {
        view->pending = 0;
        if (facts->third_base_control != THIRD_CONTROLLER_ON_BASE) {
            view->active = 1;
        }
    }

    if (facts->homeward_run_started) {
        if (!view->active) {
            view->pending = 1;
        }
        view->hold_ticks = -1;
    }

    if (facts->world_reset) {
        *view = (HomeRunView){.hold_ticks = -1};
        return;
    }

    if (facts->enforcement_completed) {
        // Once the runner safely returns onto third base (with no new run starting), drop the
        // elevated framing immediately instead of holding it through the rest of the play.
        if (view->active && !facts->homeward_run_started && facts->third_base_control == THIRD_CONTROLLER_ON_BASE) {
            view->active = 0;
            view->hold_ticks = -1;
        }

        // After the runner has resolved off third (scored, burnt, or wounded) and the ball is back
        // home, hold the elevated framing for 50 ticks so the finish at home stays framed.
        if (view->active && view->hold_ticks == -1 && facts->ball_home &&
            facts->third_base_control == THIRD_NO_CONTROLLER) {
            view->hold_ticks = 0;
        }

        if (view->hold_ticks >= 0 && ++view->hold_ticks > 50) {
            *view = (HomeRunView){.hold_ticks = -1};
        }
        if (facts->homeward_run_started_in_enforcement) {
            if (!view->active) {
                view->pending = 1;
            }
            view->hold_ticks = -1;
        }
    }
}
