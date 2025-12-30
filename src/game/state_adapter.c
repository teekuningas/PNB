#include "state_adapter.h"

void update_player_state_from_flags(PlayerInfo* p) {
    BattingTeamPlayerInfo* b = &p->bTPI;

    // Default state
    b->state = PLAYER_STATE_IDLE;

    // 1. Terminal states override everything
    if (b->out) {
        b->state = PLAYER_STATE_OUT;
        return;
    }
    if (b->wounded) {
        b->state = PLAYER_STATE_WOUNDED;
        return;
    }

    // 2. Safe states
    if (b->isOnBase) {
        if (b->leading) {
            b->state = PLAYER_STATE_LEADING;
        } else {
            b->state = PLAYER_STATE_SAFE_ON_BASE;
        }
        return;
    }

    // 3. Special movement
    if (b->takingFreeWalk) {
        b->state = PLAYER_STATE_ADVANCING_FREELY;
        return;
    }

    // 4. Default running
    // If not on base, not out, not wounded, and not walking -> Running
    // (Assuming they are active in the play)
    if (b->base != -1) { // -1 usually implies not on field, but context matters
        b->state = PLAYER_STATE_RUNNING;
    }

    // Sync baseId from base
    b->baseId = (BaseID)b->base;
}

void update_player_flags_from_state(PlayerInfo* p) {
    BattingTeamPlayerInfo* b = &p->bTPI;

    // Reset flags controlled by state
    // We don't reset 'base' or 'originalBase' here as they are data, not state flags
    b->isOnBase = 0;
    b->out = 0;
    b->wounded = 0;
    b->takingFreeWalk = 0;
    b->leading = 0;

    switch (b->state) {
        case PLAYER_STATE_IDLE:
            break;
        case PLAYER_STATE_AT_BAT:
            b->isOnBase = 1; // Batter is technically "safe" at home until hit
            break;
        case PLAYER_STATE_SAFE_ON_BASE:
            b->isOnBase = 1;
            break;
        case PLAYER_STATE_RUNNING:
            // All flags 0
            break;
        case PLAYER_STATE_ADVANCING_FREELY:
            b->takingFreeWalk = 1;
            break;
        case PLAYER_STATE_LEADING:
            b->isOnBase = 1; // Leading is a sub-state of being on base (usually)
            b->leading = 1;
            break;
        case PLAYER_STATE_OUT:
            b->out = 1;
            break;
        case PLAYER_STATE_WOUNDED:
            b->wounded = 1;
            break;
        case PLAYER_STATE_SCORED:
            // Scored players are usually removed from field arrays, so flags might not matter
            // but we ensure they aren't marked as "out"
            break;
    }

    // Sync base from baseId
    b->base = (int)b->baseId;
}

void update_global_state_from_flags(StateInfo* state) {
    // Sync GameMode
    if (state->globalGameInfo->period >= 4) {
        state->globalGameInfo->mode = GAME_MODE_HOMERUN_CONTEST;
    } else if (state->globalGameInfo->period >= 2) {
        state->globalGameInfo->mode = GAME_MODE_SUPER_INNING;
    } else {
        state->globalGameInfo->mode = GAME_MODE_NORMAL;
    }

    // Sync GameEvent
    state->localGameInfo->gAI.event = (GameEventType)state->localGameInfo->gAI.gameInfoEvent;
}

void update_global_flags_from_state(StateInfo* state) {
    // Sync period from mode
    switch (state->globalGameInfo->mode) {
        case GAME_MODE_NORMAL:
            // We don't force period to 0 if it's already 1, as both are GAME_MODE_NORMAL
            if (state->globalGameInfo->period > 1) state->globalGameInfo->period = 0;
            break;
        case GAME_MODE_SUPER_INNING:
            if (state->globalGameInfo->period < 2) state->globalGameInfo->period = 2;
            break;
        case GAME_MODE_HOMERUN_CONTEST:
            if (state->globalGameInfo->period < 4) state->globalGameInfo->period = 4;
            break;
    }

    // Sync gameInfoEvent from event
    state->localGameInfo->gAI.gameInfoEvent = (int)state->localGameInfo->gAI.event;
}
