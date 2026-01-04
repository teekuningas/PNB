# TODO - Current Tasks

## Milestone 10: The Referee Architecture (PLANNED)

**Goal:** Implement the high-level logic arbitrator.

**The Vision:**
- **Referee Layer:** Analyzes physical world → Outputs Abstract State & Permissions.
- **Benefits:** Explicit permissions, synchronous breathing, replayability.

- [ ] Design Referee interface.
- [ ] Implement Referee module.

---

## Completed ✅

### Milestone 9: Type Safety & State Machines (Complete 2026-01-04)
- [x] Defined `PlayerAnimationModel` enum for animation IDs.
- [x] Defined `PitchCycleState` enum for `PlayerRelatedActionInfo`.
- [x] Defined `ActionTriggerState`, `PitchActionPhase`, `BatActionPhase`, `ChooseBatterAction`, `FreeWalkAction` enums.
- [x] Refactored `ActionFlags` structs to use strong enums.
- [x] Replaced all magic numbers in `action_implementation.c`, `action_invocations.c`, `actions_messy/*.c`.

### Milestone 8: The Great Narrowing (Complete 2026-01-04)
- [x] **Refactor atomic movement functions**
  - [x] `stopMovement(PlayerInfo*, int)`
  - [x] `stopTargetLookingPlayer(PlayerInfo*, PlayerRuntimeState*, int)`
  - [x] `setOrientation(PlayerInfo*, BallInfo*, int)`
- [x] **Refactor complex movement functions**
  - [x] `runToTarget(PlayerInfo*, int, Vector3D*)`
  - [x] `moveToTarget(PlayerInfo*, int, Vector3D*)`
- [x] **Refactor rule-adjacent movement**
  - [x] `movePlayerOut(PlayerInfo*, PlayerRuntimeState*, FieldPositions*, int)`
  - [x] `lead(PlayerInfo*, PlayerRuntimeState*, FieldPositions*, int)`
- [x] **Refactor Ball Logic**
  - [x] `genericSlingBall(BallInfo*, PlayerRelatedActionInfo*, float, float, float)`
- [x] **Verified**
  - [x] All 56 unit tests passed.
  - [x] All 5 integration tests passed.

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
