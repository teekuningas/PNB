IDIR = -I./src/core -I./src/game -I./src/include -I./src/menu
CC=gcc
CFLAGS=$(IDIR) -O2 -Wall
LFLAGS = -lglfw -lGLEW -lX11 -lGL -lGLU -lm -lpthread -ldl -lmxml
ODIR=obj

_OBJ = core/main.o core/fill_player_data.o core/font.o core/input.o core/loadobj.o core/render.o core/save.o core/sound.o
_OBJ += game/action_implementation.o game/action_invocations.o game/ball.o game/common_logic.o game/game_analysis.o game/game_manipulation.o game/game_screen.o game/immutable_world.o game/mutable_world.o game/player.o
_OBJ += menu/batting_order_menu.o menu/hutunkeitto_menu.o menu/main_menu.o menu/team_selection_menu.o menu/front_menu.o menu/game_over_menu.o menu/homerun_contest_menu.o menu/menu_helpers.o menu/help_menu.o menu/loading_screen_menu.o menu/cup_menu.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

obj/%.o: src/%.c
	mkdir -p $(@D)
	$(CC) -c -o $@ $^ $(CFLAGS)

main: $(OBJ)
	$(CC) $^ -o $@ $(CFLAGS) $(LFLAGS)

.PHONY: run
run:
	nix run --override-input nixpkgs nixpkgs/nixos-25.05 --impure github:guibou/nixGL -- ./main --windowed

.PHONY: clean
clean:
	rm -rf $(ODIR)
	rm -f *~ core

.PHONY: shell
shell:
	nix develop

.PHONY: format
format:
	@for k in $(shell find src -name "*.c" -o -name "*.h"); do astyle --style=kr --indent=tab=4 $$k ; done
