# TODO - Current Tasks

## Milestone 10: Stabilization & Cleanup (CURRENT)

**Goal:** Eliminate globals, enforce const-correctness, and complete enum-ification to prepare for the Pipeline Architecture.

### Phase 1: Eliminate Action Globals (High Priority)
- [ ] **Audit:** Identify all static variables in `src/game/actions_messy/action_state.c`.
- [ ] **Struct Definition:** Create `PendingActionState` struct in `LocalGameInfo` (in `globals.h`).
- [ ] **Migration:** Move variables to `PendingActionState`.
    - [ ] `meterCounter`, `meterCounterMax`
    - [ ] `throwGoingOn`, `throwTarget`
    - [ ] `runBatFlag`
    - [ ] `aiWrongPitch`
    - [ ] AI Lock variables
- [ ] **Refactor:** Update all references in `actions_messy/` to use `stateInfo->localGameInfo->pendingActionState`.
- [ ] **Cleanup:** Delete `src/game/actions_messy/action_state.c` and `.h`.

### Phase 2: Enum-ify Remaining States
- [ ] Define `TeamControlMode` enum (Human P1, Human P2, AI).
- [ ] Define `TeamSide` enum (Batting, Catching).
- [ ] Define `ReplacementState` enum for `replacingStage`.
- [ ] Update `TeamInfo`, `CommonPlayerInfo`, and `CatchingTeamPlayerInfo` to use these enums.

### Phase 3: Const-Correctness Sweep
- [ ] Update `draw*` functions in `src/renderer` to take `const` pointers.
- [ ] Update `rules_pure` functions to take `const` pointers.

---

## Future Milestones (The Pipeline)

- **Milestone 11:** Split Read/Write logic.
- **Milestone 12:** Implement Intent Phase.
- **Milestone 13:** Implement Referee Phase.
- **Milestone 14:** Implement Resolver Phase.
- **Milestone 15:** Pipeline Integration.