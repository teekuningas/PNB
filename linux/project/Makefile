IDIR =../../source
CC=gcc
CFLAGS=-I$(IDIR) -O2 -Wall
LFLAGS = -lglfw -lGLEW -lX11 -lGL -lGLU -lm
ODIR=obj
SRCDIR=../../source

_OBJ = main.o action_implementation.o action_invocations.o
_OBJ += ai_logic.o ball.o fill_player_data.o font.o
_OBJ += game_analysis.o game_manipulation.o game_screen.o
_OBJ += immutable_world.o input.o
_OBJ += loadobj.o main_menu.o mutable_world.o
_OBJ += player.o render.o simplexml.o
_OBJ += sound.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

obj/%.o: ../../source/%.c
	mkdir -p obj
	$(CC) -c -o $@ $^ $(CFLAGS)

main: $(OBJ)
	$(CC) $^ -o $@ $(CFLAGS) $(LFLAGS)

.PHONY: run
run:
	nix run --override-input nixpkgs nixpkgs/nixos-23.05 --impure github:guibou/nixGL -- ./main

.PHONY: clean
clean:
	rm -f $(ODIR)/*.o *~ core $(IDIR)/*~ 

.PHONY: shell
shell:
	nix develop
