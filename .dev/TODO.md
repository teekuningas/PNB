# TODO - Current Tasks

## Milestone 11: The State Consolidation (Logic) (COMPLETE ✅)

**Goal:** Eliminate ALL logic-related `static` and global variables from `src/game`, moving them into `StateInfo`.

### Phase 1: Struct Definitions (COMPLETE ✅)
- [x] Define `AIState` struct in `src/include/globals.h` (to house AI statics).
- [x] Define `GameFlowState` struct in `src/include/globals.h` (to house analysis counters).
- [x] Add `AIState aiState;` and `GameFlowState gameFlowState;` to `LocalGameInfo` struct in `src/include/globals.h`.
- [x] Update `PendingActionState` in `src/include/globals.h` to include remaining action system statics.

### Phase 2: AI State Migration (COMPLETE ✅)
- [x] Move `aiMoveCounter`, `aiThrowStage`, `aiDropStage` from `src/game/ai_messy/catching_ai.c` to `stateInfo->localGameInfo->aiState`.
- [x] Move `aiBattingKeyDown`, `aiActionKeyLock` and other statics from `src/game/ai_messy/batting_ai.c` to `stateInfo->localGameInfo->aiState`.

### Phase 3: Action System Migration (COMPLETE ✅)
- [x] Move `pitchPower`, `aiPitchStage` from `src/game/actions_messy/pitching_system.c` to `AIState`.
- [x] Move `throwDistance`, `throwDirection` from `src/game/actions_messy/throwing_system.c` to `PendingActionState`.
- [x] Move `pitchFrameTime` from `src/game/actions_messy/batting_system.c` to `PendingActionState`.
- [x] Move `doubleClickCounter` from `src/game/action_implementation.c` to `PendingActionState`.

### Phase 4: Game Logic Migration (COMPLETE ✅)
- [x] Move `woundingCatchCounter`, `outOfBoundsCounter` from `src/game/game_analysis.c` to `GameFlowState`.
- [x] Move `closeToGround` from `src/game/game_manipulation.c` to `GameFlowState`.

---

## Milestone 12: The Rendering Unification (COMPLETE ✅)

**Goal:** Modernize in-game rendering to match the Menu system's architecture.

- [x] **Resource Manager:** Adopt `ResourceManager` for `game_screen.c` and `immutable_world.c` textures/meshes.
- [x] **State Cleanup:** Eliminate `static` GLuints and MeshObjects from `src/game`.
- [x] **Context Setup:** Unify 2D/3D context setup with the menu system.

---

## Milestone 13: Stabilization & Rule Decoupling (COMPLETE ✅)

**Goal:** Purify rule logic and stabilize safety mechanisms before large-scale decoupling.

- [x] **Purify Safety Logic:** Created `player_is_protected` and `player_is_safe_from_fly` helpers in `rules_pure/base_logic.c`.
- [x] **Standardize Base Indexing:** Standardized on `BaseID` enum consistently across logic.
- [x] **Pitch Result Enum:** Implemented `PitchResult` enum to replace magic numbers in analysis.
- [x] **Legacy Cleanup:** Removed `initLocals` 5-frame counter mechanism.
- [x] **Scenario Tests:** Added integration tests for "Chain Reaction" (§33), "Tuplahaava" (§36), and "Foul Play Resets".

---

## Milestone 14: The Great Decoupling (Read vs. Write) (CURRENT)

**Goal:** Split logic into "Query" and "Apply" halves.

- [ ] Split `game_analysis` into `check_rules()` (Read-only) and `apply_rules()` (Write).
- [ ] Split `action_implementation` into `calculate_physics()` (Read-only) and `apply_physics()` (Write).

---

## Future Milestones (The Pipeline)

- **Milestone 15:** Implement Intent Phase.
- **Milestone 16:** Implement Referee Phase.
- **Milestone 17:** Implement Resolver Phase.
- **Milestone 18:** Pipeline Integration.