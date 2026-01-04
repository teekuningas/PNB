# TODO - Current Tasks

## Milestone 10: Stabilization & Cleanup (COMPLETE ✅)

**Goal:** Eliminate globals, enforce const-correctness, and complete enum-ification to prepare for the Pipeline Architecture.

### Phase 1: Eliminate Action Globals (COMPLETE ✅)
- [x] **Audit:** Identify all static variables in `src/game/actions_messy/action_state.c`.
- [x] **Struct Definition:** Create `PendingActionState` struct in `LocalGameInfo` (in `globals.h`).
- [x] **Migration:** Move variables to `PendingActionState`.
- [x] **Refactor:** Update all references in `actions_messy/` to use `stateInfo->localGameInfo->pendingActionState`.
- [x] **Cleanup:** Delete `src/game/actions_messy/action_state.c` and `.h`.

### Phase 2: Enum-ify Remaining States (COMPLETE ✅)
- [x] Define `TeamControlMode`, `TeamSide`, `ReplacementState`, `BattingMode`, `AILockType`, `JokerStatus` enums.
- [x] Update structures to use these enums.

### Phase 3: Const-Correctness Sweep (COMPLETE ✅)
- [x] Update `draw*` functions in `src/renderer` and `src/game` to take `const` pointers.

---

## Milestone 11: The State Consolidation (Logic)

**Goal:** Eliminate ALL logic-related `static` and global variables from `src/game`, moving them into `StateInfo`.

### Phase 1: Struct Definitions
- [ ] Define `AIState` in `globals.h` (to house AI statics).
- [ ] Define `GameFlowState` in `globals.h` (to house analysis counters).
- [ ] Update `PendingActionState` in `globals.h` (to include action system statics).

### Phase 2: AI State Migration
- [ ] **Catching AI:** Move `aiMoveCounter`, `aiThrowStage`, `aiDropStage` from `catching_ai.c`.
- [ ] **Batting AI:** Move 20+ statics (`aiBattingKeyDown`, `aiActionKeyLock`, etc.) from `batting_ai.c`.

### Phase 3: Action System Migration
- [ ] **Pitching:** Move `pitchPower`, `aiPitchStage`, etc. from `pitching_system.c` to `AIState` or `PendingActionState`.
- [ ] **Throwing:** Move `throwDistance`, `throwDirection` from `throwing_system.c` to `PendingActionState`.
- [ ] **Batting:** Move `pitchFrameTime` from `batting_system.c` to `PendingActionState`.
- [ ] **Implementation:** Move `doubleClickCounter` from `action_implementation.c` to `PendingActionState`.

### Phase 4: Game Logic Migration
- [ ] **Analysis:** Move `woundingCatchCounter`, `outOfBoundsCounter`, etc. from `game_analysis.c` to `GameFlowState`.
- [ ] **Manipulation:** Move `closeToGround` from `game_manipulation.c` to `GameFlowState` (or similar).

---

## Milestone 12: The Rendering Unification

**Goal:** Modernize in-game rendering to match the Menu system's architecture.

- [ ] **Resource Manager:** Adopt `ResourceManager` for `game_screen.c` and `immutable_world.c` textures/meshes.
- [ ] **State Cleanup:** Eliminate `static` GLuints and MeshObjects from `src/game`.
- [ ] **Context Setup:** Unify 2D/3D context setup with the menu system.

---

## Milestone 13: The Great Decoupling (Read vs. Write)

**Goal:** Split logic into "Query" and "Apply" halves.

- [ ] Split `game_analysis` into `check_rules()` (Read-only) and `apply_rules()` (Write).
- [ ] Split `action_implementation` into `calculate_physics()` (Read-only) and `apply_physics()` (Write).

---

## Future Milestones (The Pipeline)

- **Milestone 14:** Implement Intent Phase.
- **Milestone 15:** Implement Referee Phase.
- **Milestone 16:** Implement Resolver Phase.
- **Milestone 17:** Pipeline Integration.
