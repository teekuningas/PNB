// The version-specific half of the golden projection: how this tree's structs answer the semantic
// questions golden.c asks. Edit THIS file when a refactor reshapes a struct, keeping every line it prints
// the same. Values a refactor legitimately leaves stale (a union half nobody is using, a scratch field the
// next action overwrites) are printed only while they mean something, so a trace records play and not
// leftovers.

// 0 none, 1 throwing, 2 pitching
static int catching_action_code(const MatchSession* m)
{
    switch (m->pendingActionState.catchingAction.kind) {
    case CATCHING_ACTION_THROWING: return 1;
    case CATCHING_ACTION_PITCHING: return 2;
    case CATCHING_ACTION_NONE: return 0;
    default: return 9;
    }
}

static void project_pitch(const MatchSession* m)
{
    const CatchingAction* a = &m->pendingActionState.catchingAction;
    if (catching_action_code(m) == 2) {
        put(5, "winding t=%d pw=%a aimed=%d", a->timer, (double)a->as.pitch.power, a->as.pitch.aimed);
        if (a->as.pitch.aimed) put(5, " d=%a", (double)a->as.pitch.direction);
    } else {
        put(5, "-");
    }
}

static void project_throw(const MatchSession* m)
{
    const CatchingAction* a = &m->pendingActionState.catchingAction;
    if (catching_action_code(m) == 1) {
        put(6, "gathering t=%d tgt=%d powered=%d", a->timer, (int)a->as.throw.target, a->as.throw.powered);
        if (a->as.throw.powered) put(6, " pw=%a", (double)a->as.throw.power);
        put(6, " dist=%a dir=", (double)a->as.throw.distance);
        put(6, "%a,%a;", (double)a->as.throw.direction.x, (double)a->as.throw.direction.z);
    } else {
        put(6, "-");
    }
    put(6, " going=%d", m->pRAI.throw_going_to_base);
}

static void project_widget(const char* name, const InputWidget* w)
{
    put(9, "%s=%d,%d,%d,%d;", name, w->counter, w->counter_max, w->dir, (int)w->mode);
}

static void project(const StateInfo* st)
{
    const MatchSession* m = st->match;
    const GameRulesState* r = st->rules;

    for (int i = 0; i < 2 * PLAYERS_IN_TEAM + JOKER_COUNT; i++) {
        const PlayerInfo* p = &m->playerInfo[i];
        put(0, "[%d]", i);
        V(0, p->tPI.location);
        V(0, p->tPI.lastLocation);
        V(0, p->tPI.targetLocation);
        V(0, p->tPI.velocity);
        V(0, p->tPI.orientation);
        put(0, "c%d,%d,%d,%d,%d,%d,%d,%d,%d;", p->cPI.team, p->cPI.moving, p->cPI.running, p->cPI.looksForTarget,
            p->cPI.lastLastLocationUpdate, (int)p->cPI.model, p->cPI.animationStage, p->cPI.animationStageCount,
            p->cPI.animationFrequency);
        put(0, "f%d,%d,%d,%d,%d,%d;", p->cTPI.position, p->cTPI.isNearHomeLocation, (int)p->cTPI.replacingStage,
            (int)p->cTPI.replacingBase, p->cTPI.busyCatching, p->cTPI.throwRecoil);
        put(0, "b%d,%d,%d,%d,%d,%d;", p->bTPI.speed, p->bTPI.power, p->bTPI.number, (int)p->bTPI.joker,
            (int)p->bTPI.state, (int)p->bTPI.baseId);
        const PlayerRuntimeState* rt = &m->playerRuntime[i];
        put(0, "r%d,%d,%d,%d;", rt->arrivedToBase, rt->passedPathPoint, rt->goingForward, rt->hasMadeRunOnThirdBase);
    }

    const BallInfo* b = &m->ballInfo;
    V(1, b->location);
    V(1, b->lastLocation);
    V(1, b->velocity);
    put(1, "%d,%d,%d,%d,%d,%d,%d", b->visible, b->moving, b->currentFlightHasHitGround, b->onGround,
        b->hitsGroundToUnWound, b->needsMoveUpdate, b->lastLastLocationUpdate);

    const PlayerIndexInfo* x = &m->pII;
    for (int i = 0; i < 4; i++) put(2, "%d,%d;", x->catcherOnBaseIndex[i], x->catcherReplacerOnBaseIndex[i]);
    for (int i = 0; i < RANKED_FIELDERS_COUNT; i++) put(2, "%d,", x->fielderRankedIndices[i]);
    for (int i = 0; i < 3; i++) put(2, "j%d,", x->jokerIndices[i]);
    put(2, "has=%d last=%d ctl=%d chg=%d", x->hasBallIndex, x->lastHadBallIndex, x->controlIndex,
        x->changePlayerArrayIndex);

    const PlayerRelatedActionInfo* a = &m->pRAI;
    put(3, "air=%d ", a->pitch_state == PITCH_STAGE_AIRBORNE);
    for (int i = 0; i < BASE_COUNT; i++) put(3, "w%d,", a->will_start_running[i]);
    put(3, "ib=%d br=%d bgo=%d bca=%d rcc=%d ips=%d", a->init_batter, a->batter_ready, a->batting_going_on,
        a->batter_can_advance, a->refresh_catch_and_change, a->init_player_selection);

    const FlowControl* fc = &m->flowControl;
    put(4, "pause=%d wbd=%d wfw=%d fwc=%d fwi=%d fwb=%d;", fc->pause, fc->waitingForBatterDecision,
        fc->waitingForFreeWalkDecision, fc->freeWalkCalculationMade, fc->freeWalkIndex, (int)fc->freeWalkBase);
    put(4, "gfs=%d,%d,%d;", m->gameFlowState.closeToGround, m->gameFlowState.homeRunCameraCounter,
        m->gameFlowState.ballHome);
    put(4, "cam=%d;", m->cameraState.homeRunCameraFlag);
    V(4, m->catchingState.controlledMoveTarget);
    put(4, "%d,%d;", m->catchingState.controlledMoveTargetActive, m->catchingState.controlledMoveTargetFor);
    V(4, m->catchingState.ballTargetPoint);
    put(4, "cca=%d", catching_action_code(m));

    project_pitch(m);
    project_throw(m);

    const PendingActionState* p = &m->pendingActionState;
    const SwingActualization* s = &p->swing;
    put(7, "cf=%d pa=%d p=%a va=%d v=%a c=%d;", s->contactFrame, s->powerActive, (double)s->power, s->verticalActive,
        (double)s->vertical, s->consumed);
    put(7, "rbf=%d bfc=%d ibfc=%d ang=%a as=%a adv=%a lim=%a stop=%d mov=%d upd=%d", p->run_bat_flag,
        p->batting_frame_count, p->increase_batting_frame_count, (double)p->batter_angle,
        (double)p->batter_advance_speed, (double)p->batter_advance, (double)p->batter_advance_limit,
        p->batting_stopped, p->batter_moving, p->update_batter_location_and_orientation);

    put(8, "%016llx", fnv(r, sizeof(*r), 1469598103934665603ULL));

    const ClientInputState* c = st->clientInput;
    for (int i = 0; i < BASE_COUNT; i++) put(9, "rp%d,", c->run_press_window[i]);
    project_widget("pw", &c->pitchWidget.meter);
    project_widget("tw", &c->throwWidget);
    put(9, "mv=%d,%d,%d,%d;", c->moveWidget.held, c->moveWidget.controlIndex, c->moveWidget.declared,
        c->moveWidget.framesSinceDeclared);
    V(9, c->moveWidget.point);
    put(9, "bs=%d,%d;", c->batterWidget.highlight, c->batterWidget.selected);
    project_widget("sw", &c->swingWidget.meter);
    put(9, "swf=%d,%d,%a;", c->swingWidget.flightFrames, (int)c->swingWidget.beat, (double)c->swingWidget.power);
    put(9, "aim=%a;", (double)c->batterAim);
    put(9, "cview=%a;", (double)catching_widget_view(c));
    SwingMeterView sv = swing_widget_view(&c->swingWidget);
    put(9, "sview=%d,%a,%d,%a", sv.cursorLive, (double)sv.cursor, sv.powerCommitted, (double)sv.power);

    const AIControllerState* ai = st->aiController;
    put(10, "seed=%u brt=%d;", ai->rngSeed, ai->batterReadyTimer);
    put(10, "sw=%d,%d,%a,%a,%a,%d;", ai->swing.decided, ai->swing.style, (double)ai->swing.power,
        (double)ai->swing.vertical, (double)ai->swing.angle, ai->swing.wrongPitch);
    V(10, ai->lastDeclaredMoveTarget);
    put(10, "%d,%d,%d", ai->hasDeclaredMoveTarget, ai->framesSinceMoveDeclared, ai->lastSteeredFielder);

    put(11, "ann=%d snd=%d stop=%d rng=%u", (int)st->announcement, st->playSoundEffect, st->stopSoundEffect,
        m->rngSeed);
}
