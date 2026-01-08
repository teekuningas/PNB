# TODO

## Milestone 16: Centralized Mutation & Referee Cleanup

- [ ] Refactor `update_runs` logic to remove direct dependencies on `GameControlFlags` and `PlayerCounters`
- [ ] Investigate moving `globalGameInfo` updates (runs) to `RefereeState` or `GameState`
- [ ] Migrate `EVENT_STRIKE` and `EVENT_BALL` logic from `game_manipulation.c` to `Referee_Update`
- [ ] Migrate `foulPlay` logic from `game_analysis.c` to `Referee_Update` (or at least the state restoration part)
- [ ] Audit `game_manipulation.c` for any other rule-based state changes and move to Referee

## Test Infrastructure (Ongoing)

- [ ] Migrate `test_scenario_runs.c` to full-scenario test
- [ ] Migrate `test_scenario_wounded.c` to full-scenario test
- [ ] Migrate `test_scenario_chain_reaction.c` to full-scenario test
- [ ] Migrate `test_scenario_tuplahaava.c` to full-scenario test
- [ ] Migrate `test_scenario_foul_play.c` to full-scenario test
- [ ] Migrate `test_scenario_overtaking.c` to full-scenario test
- [ ] Migrate `test_scenario_fielder_positioning.c` to full-scenario test
- [ ] Migrate `test_scenario_force_play.c` to full-scenario test

## Completed
- [x] Referee Refactor: Remove `RefereeDecisions` struct and use `Referee_Update` pipeline (Milestone 15)
- [x] Migrate `ballHome` logic out of Referee to `game_manipulation.c`
- [x] Fix referee system integration: Add `reconcile` logic to `mutable_world.c` after `Referee_Update`
- [x] Eliminate `baseControlIndex` array (Milestone 14)
- [x] Create `Referee_Analyze` (pure) and `Referee_Apply` (impure) (Milestone 14)
- [x] Fix "frame-off" bug where player runs automatically due to delayed safety grant