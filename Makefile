IDIR = -I./src/core -I./src/game -I./src/game/actions_pure -I./src/game/ai_pure -I./src/game/rules_pure -I./src/include -I./src/menu -I./src/cup -I./src/physics -I./src/renderer -I./tests
CC=gcc
CFLAGS=$(IDIR) -O2 -Wall
LFLAGS = -lglfw -lGLEW -lX11 -lGL -lGLU -lm -lpthread -ldl -lmxml
ODIR=obj

# Object list shared by all builds
_OBJ_COMMON = core/fill_player_data.o core/font.o core/input.o core/loadobj.o core/render.o core/resource_manager.o core/sound.o core/fixtures.o core/platform.o core/vector_math.o core/rng.o core/geometry.o core/field_layout.o core/state_validator.o physics/ball_physics.o physics/collision.o renderer/player_renderer.o renderer/ball_renderer.o
_OBJ_COMMON += game/action_implementation.o game/action_invocations.o game/ball.o game/common_logic.o game/game_analysis.o game/game_manipulation.o game/game_screen.o game/immutable_world.o game/mutable_world.o game/player.o game/game_setup.o game/actions_messy/pitching_system.o game/actions_messy/batting_system.o game/actions_messy/throwing_system.o game/ai_messy/catching_ai.o game/ai_messy/batting_ai.o game/actions_pure/batting_physics.o game/actions_pure/pitching_physics.o game/ai_pure/batting_ai_strategy.o game/ai_pure/catching_ai_strategy.o game/ai_pure/pitching_ai_strategy.o game/rules_pure/rules_outs.o game/rules_pure/rules_runs.o game/rules_pure/rules_strikes.o game/rules_pure/base_logic.o game/referee.o game/rules_pure/base_control.o
_OBJ_COMMON += menu/batting_order_menu.o menu/hutunkeitto_menu.o menu/main_menu.o menu/team_selection_menu.o menu/front_menu.o menu/game_over_menu.o menu/homerun_contest_menu.o menu/menu_helpers.o menu/help_menu.o menu/loading_screen_menu.o menu/cup_menu.o
_OBJ_COMMON += cup/cup.o

# Build-specific object paths
OBJ_MAIN = $(patsubst %,$(ODIR)/main/%,core/main.o $(_OBJ_COMMON))
OBJ_INT  = $(patsubst %,$(ODIR)/int/%,$(_OBJ_COMMON) tests/integration/fixtures.o tests/integration/scenario_builder.o tests/integration/test_full_scenarios.o)

# Unit test objects (No OpenGL)
_TEST_OBJ = core/fixtures.o core/rng.o core/vector_math.o cup/cup.o physics/collision.o game/actions_pure/batting_physics.o game/actions_pure/pitching_physics.o game/ai_pure/batting_ai_strategy.o game/ai_pure/catching_ai_strategy.o game/ai_pure/pitching_ai_strategy.o game/rules_pure/rules_outs.o game/rules_pure/rules_runs.o game/rules_pure/rules_strikes.o game/rules_pure/base_logic.o game/referee.o game/rules_pure/base_control.o core/state_validator.o tests/test_cup_logic.o tests/test_batting_physics.o tests/test_pitching_physics.o tests/test_batting_ai_strategy.o tests/test_catching_ai_strategy.o tests/test_pitching_ai_strategy.o tests/test_rules_outs.o tests/test_rules_runs.o tests/test_rules_strikes.o tests/test_base_logic.o tests/test_collision.o tests/test_rules_referee.o tests/test_debug_logging.o
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

$(ODIR)/unit/%.o: src/%.c
	@mkdir -p $(@D)
	$(CC) -c -o $@ $< $(CFLAGS)

$(ODIR)/unit/tests/%.o: tests/%.c
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
test: $(TEST_OBJ) tests/test_runner.c
	$(CC) tests/test_runner.c $(TEST_OBJ) -o test_runner $(CFLAGS) -lm -lmxml
	./test_runner

.PHONY: integration_runner
integration_runner: $(OBJ_INT) tests/integration/integration_runner.c
	$(CC) tests/integration/integration_runner.c $(OBJ_INT) -o integration_runner $(CFLAGS) $(LFLAGS)

.PHONY: integration_test
integration_test: integration_runner
	./integration_runner

.PHONY: run
run:
	nix run --override-input nixpkgs nixpkgs/nixos-25.05 --impure github:guibou/nixGL -- ./main --windowed

.PHONY: run-super-inning
run-super-inning:
	nix run --override-input nixpkgs nixpkgs/nixos-25.05 --impure github:guibou/nixGL -- ./main --windowed --fixture super-inning

.PHONY: run-homerun
run-homerun:
	nix run --override-input nixpkgs nixpkgs/nixos-25.05 --impure github:guibou/nixGL -- ./main --windowed --fixture homerun-contest

.PHONY: run-cup-final-super-inning
run-cup-final-super-inning:
	nix run --override-input nixpkgs nixpkgs/nixos-25.05 --impure github:guibou/nixGL -- ./main --windowed --fixture cup-final-super-inning

.PHONY: clean
clean:
	rm -rf $(ODIR)
	rm -f *~ core test_runner integration_runner main
	find . -type f -name '*.orig' -print0 | xargs -0 rm -f

.PHONY: shell
shell:
	devenv shell

.PHONY: format
format:
	@for k in $(shell find src -name "*.c" -o -name "*.h"); do astyle --style=kr --indent=tab=4 $$k ; done
