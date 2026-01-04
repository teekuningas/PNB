# TODO - Current Tasks

## Milestone 10: Stabilization & Cleanup (COMPLETE ✅)

**Goal:** Eliminate globals, enforce const-correctness, and complete enum-ification to prepare for the Pipeline Architecture.

### Phase 1: Eliminate Action Globals (COMPLETE ✅)
- [x] **Audit:** Identify all static variables in `src/game/actions_messy/action_state.c`.
- [x] **Struct Definition:** Create `PendingActionState` struct in `LocalGameInfo` (in `globals.h`).
- [x] **Migration:** Move variables to `PendingActionState`.
    - [x] `meterCounter`, `meterCounterMax`
    - [x] `throwGoingOn`, `throwTarget`
    - [x] `runBatFlag`
    - [x] `aiWrongPitch`
    - [x] AI Lock variables
- [x] **Refactor:** Update all references in `actions_messy/` to use `stateInfo->localGameInfo->pendingActionState`.
- [x] **Cleanup:** Delete `src/game/actions_messy/action_state.c` and `.h`.

### Phase 2: Enum-ify Remaining States (COMPLETE ✅)
- [x] Define `TeamControlMode` enum (Human P1, Human P2, AI).
- [x] Define `TeamSide` enum (Batting, Catching).
- [x] Define `ReplacementState` enum for `replacingStage`.
- [x] Define `BattingMode`, `AILockType`, and `JokerStatus` enums.
- [x] Update `TeamInfo`, `CommonPlayerInfo`, `CatchingTeamPlayerInfo`, and `BattingTeamPlayerInfo` to use these enums.

### Phase 3: Const-Correctness Sweep (COMPLETE ✅)
- [x] Update `draw*` functions in `src/renderer` and `src/game` to take `const` pointers.
- [x] Verify pure logic functions in `src/game/rules_pure` already take `const` pointers.

---

## Milestone 11: The Great Decoupling (Read vs. Write)

**Goal:** Split logic into "Query" and "Apply" halves.

- [ ] Split `game_analysis` into `check_rules()` (Read-only) and `apply_rules()` (Write).
- [ ] Split `action_implementation` into `calculate_physics()` (Read-only) and `apply_physics()` (Write).

---

## Future Milestones (The Pipeline)

- **Milestone 12:** Implement Intent Phase.
- **Milestone 13:** Implement Referee Phase.
- **Milestone 14:** Implement Resolver Phase.
- **Milestone 15:** Pipeline Integration.