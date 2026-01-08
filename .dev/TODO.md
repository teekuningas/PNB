# TODO

## Milestone 15: Referee Architecture V2 (Sequential Update)

- [ ] Design `updateReferee` signature and internal flow (create header prototype)
- [ ] Migrate "Force Out" logic from `Referee_Analyze` to a standalone pure function called by `updateReferee`
- [ ] Migrate "Run" logic from `Referee_Analyze` to a standalone pure function
- [ ] Migrate "Wounding" logic
- [ ] Remove `RefereeDecisions` struct and `Referee_Analyze`/`Referee_Apply` functions
- [ ] Verify 100% test coverage with `make test` and `integration_runner`

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
- [x] Fix referee system integration: Add `reconcile` logic to `mutable_world.c` after `Referee_Execute`
- [x] Eliminate `baseControlIndex` array (Milestone 14)
- [x] Create `Referee_Analyze` (pure) and `Referee_Apply` (impure) (Milestone 14)
- [x] Fix "frame-off" bug where player runs automatically due to delayed safety grant
