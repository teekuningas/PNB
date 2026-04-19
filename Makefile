IDIR = -I./src/core -I./src/game -I./src/game/actions_pure -I./src/game/ai_pure -I./src/game/rules_pure -I./src/include -I./src/menu -I./src/cup -I./src/physics -I./src/renderer -I./tests/unit -I./tests/integration
CC=gcc
CFLAGS=$(IDIR) -O2 -Wall
LFLAGS = -lglfw -lGLEW -lX11 -lGL -lGLU -lm -lpthread -ldl -lmxml
ODIR=obj

# Object list shared by all builds
_OBJ_COMMON = core/fill_player_data.o core/font.o core/input.o core/loadobj.o core/render.o core/resource_manager.o core/sound.o core/fixtures.o core/platform.o core/vector_math.o core/rng.o core/geometry.o core/field_layout.o core/state_validator.o physics/ball_physics.o physics/collision.o renderer/player_renderer.o renderer/ball_renderer.o
_OBJ_COMMON += game/action_implementation.o game/action_invocations.o game/ball.o game/common_logic.o game/game_consolidation.o game/game_manipulation.o game/game_screen.o game/immutable_world.o game/mutable_world.o game/player.o game/game_setup.o game/actions_messy/pitching_system.o game/actions_messy/batting_system.o game/actions_messy/throwing_system.o game/ai_messy/catching_ai.o game/ai_messy/batting_ai.o game/actions_pure/batting_physics.o game/actions_pure/pitching_physics.o game/ai_pure/batting_ai_strategy.o game/ai_pure/catching_ai_strategy.o game/ai_pure/pitching_ai_strategy.o game/rules_pure/rules_outs.o game/rules_pure/rules_runs.o game/rules_pure/rules_strikes.o game/rules_pure/base_logic.o game/referee.o game/rules_pure/base_control.o game/rules_pure/player_utils.o game/rules_pure/scoring_helpers.o
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
                 tests/scenario/test_fly_ball_early_arrival.o

# Contract test objects (1-frame pipeline tests, live in tests/integration/contracts/)
_OBJ_CONTRACTS = tests/integration/contracts/test_clear_frame_events.o \
                 tests/integration/contracts/test_referee_reacts_to_catch.o \
                 tests/integration/contracts/test_referee_reacts_to_pitch.o \
                 tests/integration/contracts/test_foul_detection.o \
                 tests/integration/contracts/test_end_of_inning_blocks_runs.o \
                 tests/integration/contracts/test_compound_foul_and_end_of_inning.o \
                 tests/integration/contracts/test_compound_hr_pair_and_uncatchable.o

OBJ_MAIN     = $(patsubst %,$(ODIR)/main/%,core/main.o $(_OBJ_COMMON))
OBJ_SCENARIO = $(patsubst %,$(ODIR)/int/%,$(_OBJ_COMMON) $(_OBJ_TEST_INFRA) $(_OBJ_SCENARIOS))
OBJ_CONTRACT = $(patsubst %,$(ODIR)/int/%,$(_OBJ_COMMON) $(_OBJ_TEST_INFRA) $(_OBJ_CONTRACTS))

# Unit test objects (No OpenGL)
_TEST_OBJ = core/fixtures.o core/rng.o core/vector_math.o cup/cup.o physics/collision.o game/actions_pure/batting_physics.o game/actions_pure/pitching_physics.o game/ai_pure/batting_ai_strategy.o game/ai_pure/catching_ai_strategy.o game/ai_pure/pitching_ai_strategy.o game/rules_pure/rules_outs.o game/rules_pure/rules_runs.o game/rules_pure/rules_strikes.o game/rules_pure/base_logic.o game/referee.o game/rules_pure/base_control.o game/rules_pure/player_utils.o game/rules_pure/scoring_helpers.o core/state_validator.o tests/unit/test_cup_logic.o tests/unit/test_batting_physics.o tests/unit/test_pitching_physics.o tests/unit/test_batting_ai_strategy.o tests/unit/test_catching_ai_strategy.o tests/unit/test_pitching_ai_strategy.o tests/unit/test_rules_outs.o tests/unit/test_rules_runs.o tests/unit/test_base_logic.o tests/unit/test_collision.o tests/unit/test_base_control.o tests/unit/test_player_utils.o tests/unit/test_scoring_helpers.o
TEST_OBJ = $(patsubst %,$(ODIR)/unit/%,$(_TEST_OBJ))

# Generic rules for each build type
$(ODIR)/main/%.o: src/%.c
	@mkdir -p $(@D)
	$(CC) -c -o $@ $< $(CFLAGS)

$(ODIR)/int/%.o: src/%.c
	@mkdir -p $(@D)
	$(CC) -c -o $@ $< $(CFLAGS) -DNO_RENDER

$(ODIR)/int/tests/integration/%.o: tests/integration/%.c
	@mkdir -p $(@D)
	$(CC) -c -o $@ $< $(CFLAGS) -DNO_RENDER

$(ODIR)/int/tests/scenario/%.o: tests/scenario/%.c
	@mkdir -p $(@D)
	$(CC) -c -o $@ $< $(CFLAGS) -DNO_RENDER

$(ODIR)/unit/%.o: src/%.c
	@mkdir -p $(@D)
	$(CC) -c -o $@ $< $(CFLAGS)

$(ODIR)/unit/tests/unit/%.o: tests/unit/%.c
	@mkdir -p $(@D)
	$(CC) -c -o $@ $< $(CFLAGS)

.PHONY: main
main: $(OBJ_MAIN)
	$(CC) $^ -o $@ $(CFLAGS) $(LFLAGS)

.PHONY: watch_task_agent
watch_task_agent:
	@echo "Starting Task Agent Watcher (Gemini)..."
	@./.dev/scripts/task_agent.py

.PHONY: watch_task_agent_copilot
watch_task_agent_copilot:
	@echo "Starting Task Agent Watcher (Copilot)..."
	@./.dev/scripts/task_agent_copilot.py

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
	rm -f *~ core test_runner scenario_runner contract_runner integration_runner main
	find . -type f -name '*.orig' -print0 | xargs -0 rm -f

.PHONY: shell
shell:
	devenv shell

.PHONY: format
format:
	@find src tests \( -name "*.c" -o -name "*.h" \) ! -name "miniaudio.h" ! -name "stb_image.h" | xargs clang-format -i
