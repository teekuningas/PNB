#ifndef HOME_RUN_VIEW_H
#define HOME_RUN_VIEW_H

#include "tick_outputs.h"

typedef struct _HomeRunView {
    bool active;
    int hold_ticks; // -1 while not counting
} HomeRunView;

void home_run_view_after_tick(HomeRunView* view, const TickFacts* facts);

#endif
