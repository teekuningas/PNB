IDIR = -I./src/core -I./src/game -I./src/include -I./src/menu -I./src/cup -I./src/physics -I./tests
CC=gcc
CFLAGS=$(IDIR) -O2 -Wall
LFLAGS = -lglfw -lGLEW -lX11 -lGL -lGLU -lm -lpthread -ldl -lmxml
ODIR=obj

_OBJ = core/main.o core/fill_player_data.o core/font.o core/input.o core/loadobj.o core/render.o core/resource_manager.o core/sound.o core/fixtures.o core/platform.o core/vector_math.o core/rng.o core/geometry.o core/field_layout.o physics/ball_physics.o physics/collision.o
_OBJ += game/action_implementation.o game/action_invocations.o game/ball.o game/common_logic.o game/game_analysis.o game/game_manipulation.o game/game_screen.o game/immutable_world.o game/mutable_world.o game/player.o game/game_setup.o
_OBJ += menu/batting_order_menu.o menu/hutunkeitto_menu.o menu/main_menu.o menu/team_selection_menu.o menu/front_menu.o menu/game_over_menu.o menu/homerun_contest_menu.o menu/menu_helpers.o menu/help_menu.o menu/loading_screen_menu.o menu/cup_menu.o
_OBJ += cup/cup.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

# Test objects (subset without OpenGL dependencies)
_TEST_OBJ = core/fixtures.o core/rng.o cup/cup.o tests/test_cup_logic.o
TEST_OBJ = $(patsubst %,$(ODIR)/%,$(_TEST_OBJ))

obj/%.o: src/%.c
	mkdir -p $(@D)
	$(CC) -c -o $@ $^ $(CFLAGS)

obj/tests/%.o: tests/%.c
	mkdir -p $(@D)
	$(CC) -c -o $@ $^ $(CFLAGS)

# Check if we are in a nix develop shell (indicated by $IN_NIX_SHELL)
# If not, execute make command again within a nix develop shell
ifeq ($(IN_NIX_SHELL),)
.PHONY: main
main:
	nix develop -c -- $(MAKE) $(MAKECMDGOALS)
else
# If already in nix develop shell, proceed with normal build
main: $(OBJ)
	$(CC) $^ -o $@ $(CFLAGS) $(LFLAGS)
endif

# Test target - compile and run tests without OpenGL
# Check if we are in a nix develop shell (indicated by $IN_NIX_SHELL)
# If not, execute make command again within a nix develop shell
ifeq ($(IN_NIX_SHELL),)
.PHONY: test
watch_junior:
	@echo "Starting Junior Watcher..."
	@./.dev/scripts/junior.py

test:
	nix develop -c -- $(MAKE) $(MAKECMDGOALS)
else
# If already in nix develop shell, proceed with normal build
test: $(TEST_OBJ) tests/test_runner.c
	$(CC) tests/test_runner.c $(TEST_OBJ) -o test_runner $(CFLAGS) -lm -lmxml
	./test_runner
endif

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
	rm -f *~ core test_runner

.PHONY: shell
shell:
	nix develop

.PHONY: format
format:
	@for k in $(shell find src -name "*.c" -o -name "*.h"); do astyle --style=kr --indent=tab=4 $$k ; done

