# TODO - Pending Tasks

## 🚧 Milestone 15: Action System Decoupling (CURRENT)

**Goal:** Apply the Query/Apply pattern to action system (same as referee)

### Phase 1: Audit actions_messy/ (NEXT)
- [ ] Audit `batting_system.c` (473 LOC) - identify pure calculations
- [ ] Audit `pitching_system.c` (385 LOC) - identify pure calculations
- [ ] Audit `throwing_system.c` (250 LOC) - identify pure calculations
- [ ] Document: what can be pure? what must mutate state?

### Phase 2: Action Implementation (PENDING)
- [ ] Create `ActionDecisions` struct (similar to `RefereeDecisions`)
- [ ] Move pure logic to `actions_pure/` or create new files
- [ ] Create `Action_Apply()` functions for state mutation
- [ ] Update coordinators to call Analyze → Apply

### Phase 3: Testing (PENDING)
- [ ] Create unit tests for pure action logic
- [ ] Verify integration tests still pass
- [ ] Run `make test && make integration_test`

---

## ✅ Milestone 14: The Great Decoupling (COMPLETE)

### Phase 1: Game Analysis Split ✅
- [x] Created `rules_pure/referee.c` with pure `Referee_Analyze`
- [x] Created `referee_apply.c` with `Referee_Apply`
- [x] Created `RefereeDecisions` struct

### Phase 2: State Validation ✅
- [x] Created `state_validator.c` with invariant checks
- [x] Eliminated `baseControlIndex` → `currentSafetyBase`
- [x] JSON dump on validation failure

### Phase 3: Verification ✅
- [x] Created `test_rules_referee.c`
- [x] 67 tests passing (53 unit + 14 integration)

---

## 📅 Upcoming Milestones

### Milestone 16: User Intent Phase
- Decouple input from execution
- Create Intent structs

### Milestone 17: Comprehensive Rule Verification
- 100% audit of SAANNOT.md
- Implement §31 (Fielder Positioning)

### Milestone 18: Full Functional Pipeline
- Linear game loop: Input → Intent → Physics → Referee → Apply → Render