#ifndef GLOBALS_H
#define GLOBALS_H

// TRANSITIONAL — an umbrella over the headers the old monolith was split into, so that its remaining
// includers compile while they migrate to the headers they actually name. It is deleted when the last one has.

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <malloc.h>
#include <math.h>

#include "foundation.h"
#include "field.h"
#include "display.h"
#include "key_states.h"
#include "tick_outputs.h"
#include "team_data.h"
#include "rules_state.h"
#include "intents.h"
#include "world.h"
#include "session.h"
#include "client_input.h"
#include "ai/ai_memory.h"

#endif /* GLOBALS_H */
