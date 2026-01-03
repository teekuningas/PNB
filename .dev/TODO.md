# TODO - Current Tasks

## Milestone 8: The Great Narrowing (COMPLETE ✅)

**Philosophy:** "Write-only what you need" - Narrow function signatures to specific structs.

**Goal:** Decouple utility/physics logic from the global God Object (`StateInfo`).

---

### Phase 1: Narrow Movement Logic (common_logic.c) - COMPLETE ✅

- [x] **Refactor atomic movement functions** (Done 2026-01-03)
  - [x] `stopMovement(LocalGameInfo*, int)`
  - [x] `stopTargetLookingPlayer(LocalGameInfo*, int)`
  - [x] `setOrientation(LocalGameInfo*, int)`
- [x] **Refactor complex movement functions** (Done 2026-01-03)
  - [x] `runToTarget(LocalGameInfo*, int, Vector3D*)`
  - [x] `moveToTarget(LocalGameInfo*, int, Vector3D*)`
- [x] **Refactor rule-adjacent movement** (Done 2026-01-03)
  - [x] `movePlayerOut(LocalGameInfo*, FieldPositions*, int)`
  - [x] `runToNextBase(LocalGameInfo*, FieldPositions*, int, int)`
  - [x] `runToPreviousBase(LocalGameInfo*, FieldPositions*, int, int)`
  - [x] `lead(LocalGameInfo*, FieldPositions*, int)`

### Phase 2: Narrow Ball Logic (ball.c) - COMPLETE ✅

- [x] `updateBallStatus(LocalGameInfo*, FieldPositions*)` (Done 2026-01-03)
- [x] `updateBallToPlayer(LocalGameInfo*)` (Done 2026-01-03)
- [x] `genericSlingBall(LocalGameInfo*, float, float, float)` (Done 2026-01-03)

### Phase 3: Narrow AI Strategy (ai_pure/) - COMPLETE ✅

- [x] `calculate_batting_strategy(const GameState*, ...)` (Done 2026-01-03)
- [x] `calculate_ai_pitch_targets(..., const PlayerCounters*, const GameState*, ...)` (Done 2026-01-03)
- [x] `should_ai_drop_ball(const WoundingState*, const GameControlFlags*, ...)` (Done 2026-01-03)
- [x] `should_ai_throw(const PlayerIndexInfo*, ...)` (Done 2026-01-03)

### Phase 4: Test Suite Refinement - COMPLETE ✅

- [x] Update unit tests to use narrowed structs (Done 2026-01-03)
- [x] Verify all 51 tests pass (Done 2026-01-03)

---

## Milestone 9: Type Safety & State Machines (CURRENT 🎯)

**Goal:** Replace "int flags" with strong Enums and State Machines.

- [ ] Define `PlayerAnimationModel` enum for animation IDs.
- [ ] Define `PitchCycleState` enum for `PlayerRelatedActionInfo`.
- [ ] Investigate typing `ActionFlags`.

---

## Milestone 10: The Referee Architecture (PLANNED)

- [ ] Implement the Referee pattern.

---

## Completed ✅

### Milestone 7.5: Data Structure Cleanup (Complete 2026-01-03)
- [x] Phase 0: Data Structure Audit
- [x] Phase 1: Extract PlayerRuntimeState
- [x] Phase 2: Split GameAnalysisInfo God Object
- [x] Phase 3: Stabilize & Document (Baseline)

### Milestone 7: Data Renaissance (Complete 2026-01-01)
- [x] Eliminated ALL legacy state flags
- [x] Type-safe enums (PlayerUnitState, BaseID, GameEventType)
- [x] Deleted state_adapter.c/h
- [x] All 51 unit + 5 integration tests passing
