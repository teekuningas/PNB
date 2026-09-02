IDIR = -I./src/core -I./src/game -I./src/game/actions_pure -I./src/game/ai_pure -I./src/game/rules_pure -I./src/include -I./external -I./src/menu -I./src/cup -I./src/physics -I./src/renderer -I./tests/unit -I./tests/integration -I./tests/sim -I./tests/scripted
CC=gcc
CFLAGS=$(IDIR) -O2 -Wall
# Header-dependency tracking: -MMD emits a .d file next to each .o listing the
# headers it #included; -MP adds phony targets so deleting a header doesn't break
# the build. Kept OUT of CFLAGS on purpose: CFLAGS is reused on compile-and-link
# lines (test, scenario_runner, ...) where -MMD would drop a stray .d in the repo
# root. Only the per-object -c rules below add DEPFLAGS.
DEPFLAGS = -MMD -MP
LFLAGS = -lglfw -lGLEW -lX11 -lGL -lGLU -lm -lpthread -ldl -lmxml
ODIR=obj

# Object list shared by all builds
_OBJ_COMMON = core/fill_player_data.o core/font.o core/input.o core/loadobj.o core/render.o core/resource_manager.o core/sound.o core/fixtures.o core/platform.o core/vector_math.o core/rng.o core/field_layout.o core/state_validator.o physics/ball_physics.o physics/collision.o renderer/player_renderer.o renderer/ball_renderer.o
_OBJ_COMMON += game/execute_actions.o game/action_invocations.o game/ball.o game/common_logic.o game/game_consolidation.o game/game_manipulation.o game/game_screen.o game/immutable_world.o game/game_frame.o game/player.o game/game_setup.o game/game_reset.o game/actions/pitching_system.o game/actions/batting_system.o game/actions/throwing_system.o game/actions/fielder_movement.o game/ai/catching_ai.o game/ai/batting_ai.o game/actions_pure/batting_physics.o game/actions_pure/pitching_physics.o game/actions_pure/swing_geometry.o game/ai_pure/batting_ai_strategy.o game/ai_pure/catching_ai_strategy.o game/ai_pure/pitching_ai_strategy.o game/rules_pure/rules_outs.o game/rules_pure/rules_runs.o game/rules_pure/rules_strikes.o game/rules_pure/base_logic.o game/referee.o game/rules_pure/base_control.o game/rules_pure/player_utils.o game/rules_pure/scoring_helpers.o game/rules_pure/rules_side_change.o game/rules_pure/rules_batting_order.o
_OBJ_COMMON += menu/batting_order_menu.o menu/hutunkeitto_menu.o menu/main_menu.o menu/team_selection_menu.o menu/front_menu.o menu/game_over_menu.o menu/homerun_contest_menu.o menu/menu_helpers.o menu/help_menu.o menu/loading_screen_menu.o menu/cup_menu.o
_OBJ_COMMON += cup/cup.o

# Shared test infrastructure (used by both scenario and contract tests)
_OBJ_TEST_INFRA = tests/integration/fixtures.o tests/integration/scenario_builder.o

# Scenario test objects (full-game simulations, live in tests/scenario/)
_OBJ_SCENARIOS = tests/scenario/test_runner_scores_from_third.o \
                 tests/scenario/test_batter_forced_out_at_first.o \
                 tests/scenario/test_fly_ball_runner_wounded.o \
                 tests/scenario/test_runner_chain_reaction.o \
                 tests/scenario/test_fly_ball_double_wound.o \
                 tests/scenario/test_fly_ball_double_wound_late_arrival.o \
                 tests/scenario/test_out_of_bounds_reset.o \
                 tests/scenario/test_pitching_strike.o \
                 tests/scenario/test_pitching_ball.o \
                 tests/scenario/test_free_walk.o \
                 tests/scenario/test_run_of_honor.o \
                 tests/scenario/test_run_before_ball_lands.o \
                 tests/scenario/test_run_before_catch.o \
                 tests/scenario/test_fly_ball_out_and_wound.o \
                 tests/scenario/test_fly_ball_early_arrival.o \
                 tests/scenario/test_burnt_player_bats_again.o \
                 tests/scenario/test_last_batter_ends_half_inning.o

# Contract test objects (1-frame pipeline tests, live in tests/integration/contracts/)
_OBJ_CONTRACTS = tests/integration/contracts/test_clear_frame_events.o \
                 tests/integration/contracts/test_reset_clears_declarations.o \
                 tests/integration/contracts/test_referee_reacts_to_catch.o \
                 tests/integration/contracts/test_referee_reacts_to_pitch.o \
                 tests/integration/contracts/test_foul_detection.o \
                 tests/integration/contracts/test_end_of_inning_blocks_runs.o \
                 tests/integration/contracts/test_compound_foul_and_end_of_inning.o \
                 tests/integration/contracts/test_compound_hr_pair_and_uncatchable.o \
                 tests/integration/contracts/test_bat_outcome_promotion.o \
                                  tests/integration/contracts/test_wounded_runner_cannot_score.o \
                 tests/integration/contracts/test_ai_tactical_drop.o \
                 tests/integration/contracts/test_control_stage_ordering.o \
                 tests/integration/contracts/test_pitch_declaration.o \
                 tests/integration/contracts/test_throw_declaration.o \
                 tests/integration/contracts/test_fielder_movement.o tests/integration/contracts/test_batter_selection.o

# Simulation test objects (headless AI-vs-AI, drive the real pipeline, live in tests/sim/)
_OBJ_SIMS = tests/sim/sim_harness.o \
            tests/sim/sim_observers.o \
            tests/sim/test_ai_vs_ai_half_inning.o \
            tests/sim/test_ai_vs_ai_homerun.o \
            tests/sim/test_determinism.o \
            tests/sim/test_ai_offense_breakdown.o \
            tests/sim/test_no_batter_lock_stall.o \
            tests/sim/test_world_retick.o \
            tests/sim/test_ai_ignores_frame_events.o

# Scripted-human test objects (headless, drive the REAL action_invocations via scripted KeyStates,
# live in tests/scripted/). Reuses the sim harness + observers (boot, real rosters, validator).
_OBJ_SCRIPTED = tests/sim/sim_harness.o \
                tests/sim/sim_observers.o \
                tests/scripted/scripted_harness.o \
                tests/scripted/test_scripted_smoke.o \
                tests/scripted/test_scripted_base_run.o \
                tests/scripted/test_scripted_pitch.o \
                tests/scripted/test_scripted_throw.o \
                tests/scripted/test_scripted_move.o tests/scripted/test_scripted_batter_aim.o

OBJ_MAIN     = $(patsubst %,$(ODIR)/main/%,core/main.o $(_OBJ_COMMON))
OBJ_SCENARIO = $(patsubst %,$(ODIR)/int/%,$(_OBJ_COMMON) $(_OBJ_TEST_INFRA) $(_OBJ_SCENARIOS))
OBJ_CONTRACT = $(patsubst %,$(ODIR)/int/%,$(_OBJ_COMMON) $(_OBJ_TEST_INFRA) $(_OBJ_CONTRACTS))
OBJ_SIM      = $(patsubst %,$(ODIR)/int/%,$(_OBJ_COMMON) $(_OBJ_TEST_INFRA) $(_OBJ_SIMS))
OBJ_SCRIPTED = $(patsubst %,$(ODIR)/int/%,$(_OBJ_COMMON) $(_OBJ_TEST_INFRA) $(_OBJ_SCRIPTED))

# Unit test objects (No OpenGL)
_TEST_OBJ = core/fixtures.o core/rng.o core/vector_math.o cup/cup.o physics/collision.o game/actions_pure/batting_physics.o game/actions_pure/pitching_physics.o game/actions_pure/swing_geometry.o game/ai_pure/batting_ai_strategy.o game/ai_pure/catching_ai_strategy.o game/ai_pure/pitching_ai_strategy.o game/rules_pure/rules_outs.o game/rules_pure/rules_runs.o game/rules_pure/rules_strikes.o game/rules_pure/base_logic.o game/referee.o game/rules_pure/base_control.o game/rules_pure/player_utils.o game/rules_pure/scoring_helpers.o game/rules_pure/rules_side_change.o game/rules_pure/rules_batting_order.o core/state_validator.o tests/unit/test_cup_logic.o tests/unit/test_batting_physics.o tests/unit/test_swing_geometry.o tests/unit/test_pitching_physics.o tests/unit/test_batting_ai_strategy.o tests/unit/test_catching_ai_strategy.o tests/unit/test_pitching_ai_strategy.o tests/unit/test_rules_outs.o tests/unit/test_rules_runs.o tests/unit/test_rules_side_change.o tests/unit/test_rules_batting_order.o tests/unit/test_base_logic.o tests/unit/test_collision.o tests/unit/test_base_control.o tests/unit/test_player_utils.o tests/unit/test_scoring_helpers.o tests/unit/test_rng.o
TEST_OBJ = $(patsubst %,$(ODIR)/unit/%,$(_TEST_OBJ))

# Generic rules for each build type
$(ODIR)/main/%.o: src/%.c
	@mkdir -p $(@D)
	$(CC) -c -o $@ $< $(CFLAGS) $(DEPFLAGS)

$(ODIR)/int/%.o: src/%.c
	@mkdir -p $(@D)
	$(CC) -c -o $@ $< $(CFLAGS) $(DEPFLAGS) -DNO_RENDER

$(ODIR)/int/tests/integration/%.o: tests/integration/%.c
	@mkdir -p $(@D)
	$(CC) -c -o $@ $< $(CFLAGS) $(DEPFLAGS) -DNO_RENDER

$(ODIR)/int/tests/scenario/%.o: tests/scenario/%.c
	@mkdir -p $(@D)
	$(CC) -c -o $@ $< $(CFLAGS) $(DEPFLAGS) -DNO_RENDER

$(ODIR)/int/tests/sim/%.o: tests/sim/%.c
	@mkdir -p $(@D)
	$(CC) -c -o $@ $< $(CFLAGS) $(DEPFLAGS) -DNO_RENDER

$(ODIR)/int/tests/scripted/%.o: tests/scripted/%.c
	@mkdir -p $(@D)
	$(CC) -c -o $@ $< $(CFLAGS) $(DEPFLAGS) -DNO_RENDER

$(ODIR)/unit/%.o: src/%.c
	@mkdir -p $(@D)
	$(CC) -c -o $@ $< $(CFLAGS) $(DEPFLAGS)

$(ODIR)/unit/tests/unit/%.o: tests/unit/%.c
	@mkdir -p $(@D)
	$(CC) -c -o $@ $< $(CFLAGS) $(DEPFLAGS)

# Pull in the generated header-dependency files. The lists are derived from the
# object lists above (.o -> .d); '-include' (leading dash) ignores the ones that
# don't exist yet on a clean build. After this, touching a .h rebuilds exactly the
# .o files that #include it — no more stale-object ABI mismatches.
DEPS = $(OBJ_MAIN:.o=.d) $(OBJ_SCENARIO:.o=.d) $(OBJ_CONTRACT:.o=.d) $(OBJ_SIM:.o=.d) $(OBJ_SCRIPTED:.o=.d) $(TEST_OBJ:.o=.d)
-include $(DEPS)

.PHONY: main
main: $(OBJ_MAIN)
	$(CC) $^ -o $@ $(CFLAGS) $(LFLAGS)

# The one target to run before committing: the build, all five test tiers, and the
# guardrails. Nothing here mutates the tree — `make format` is the separate, explicit
# fixer, and guardrails only *reports* a formatting deviation. Steps run in order and
# stop at the first failure, so the first thing printed after a break is the break.
.PHONY: check
check:
	@$(MAKE) --no-print-directory main
	@$(MAKE) --no-print-directory test
	@$(MAKE) --no-print-directory integration_test
	@$(MAKE) --no-print-directory scenario_test
	@$(MAKE) --no-print-directory sim_test
	@$(MAKE) --no-print-directory scripted_test
	@$(MAKE) --no-print-directory guardrails
	@$(MAKE) --no-print-directory dead_exports
	@echo
	@echo "  make check: build + all five tiers + guardrails + the exported surface, all green."

# The architectural numbers that may only go down (globals.h includers, dead
# parameters, function-quality audit coverage, ...). The script carries the floors
# and is the definition of each measurement; IDIR is passed in so the sweep can
# never drift from the build's own include path.
.PHONY: guardrails
guardrails:
	@./tools/guardrails.sh "$(IDIR)"

# The exported surface: functions in src/ that nothing outside their own translation
# unit uses (dead code, or a header exporting an edge that does not exist). The
# guardrails sweep above is the compiler front end alone and structurally cannot see
# this — only the whole link can. The prerequisites are the same object lists the five
# tiers link, so make itself guarantees the measurement is never taken from a stale
# obj/; the cost of that is a build, which is why this row rides with `make check`
# instead of with the six-second sweep.
.PHONY: dead_exports
dead_exports: $(OBJ_MAIN) $(OBJ_SCENARIO) $(OBJ_CONTRACT) $(OBJ_SIM) $(OBJ_SCRIPTED) $(TEST_OBJ)
	@./tools/dead_exports.sh $^

.PHONY: test
test: $(TEST_OBJ) tests/unit/test_runner.c
	$(CC) tests/unit/test_runner.c $(TEST_OBJ) -o test_runner $(CFLAGS) -lm -lmxml
	./test_runner

.PHONY: scenario_runner
scenario_runner: $(OBJ_SCENARIO) tests/scenario/scenario_runner.c
	$(CC) tests/scenario/scenario_runner.c $(OBJ_SCENARIO) -o scenario_runner $(CFLAGS) $(LFLAGS)

.PHONY: scenario_test
scenario_test: scenario_runner
	./scenario_runner

.PHONY: contract_runner
contract_runner: $(OBJ_CONTRACT) tests/integration/contract_runner.c
	$(CC) tests/integration/contract_runner.c $(OBJ_CONTRACT) -o contract_runner $(CFLAGS) $(LFLAGS)

.PHONY: integration_test
integration_test: contract_runner
	./contract_runner

.PHONY: sim_runner
sim_runner: $(OBJ_SIM) tests/sim/sim_runner.c
	$(CC) tests/sim/sim_runner.c $(OBJ_SIM) -o sim_runner $(CFLAGS) $(LFLAGS)

.PHONY: sim_test
sim_test: sim_runner
	./sim_runner

.PHONY: scripted_runner
scripted_runner: $(OBJ_SCRIPTED) tests/scripted/scripted_runner.c
	$(CC) tests/scripted/scripted_runner.c $(OBJ_SCRIPTED) -o scripted_runner $(CFLAGS) $(LFLAGS)

.PHONY: scripted_test
scripted_test: scripted_runner
	./scripted_runner

.PHONY: run
run:
	./main --windowed

.PHONY: run-super-inning
run-super-inning:
	./main --windowed --fixture super-inning

.PHONY: run-homerun
run-homerun:
	./main --windowed --fixture homerun-contest

.PHONY: run-cup-final-super-inning
run-cup-final-super-inning:
	./main --windowed --fixture cup-final-super-inning

.PHONY: clean
clean:
	rm -rf $(ODIR)
	rm -f *~ core test_runner scenario_runner contract_runner integration_runner sim_runner scripted_runner main
	find . -type f -name '*.orig' -print0 | xargs -0 rm -f

.PHONY: shell
shell:
	nix develop

.PHONY: format
format:
	@find src tests \( -name "*.c" -o -name "*.h" \) ! -name "miniaudio.h" ! -name "stb_image.h" | xargs clang-format -i
