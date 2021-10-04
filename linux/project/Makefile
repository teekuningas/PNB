IDIR =../../source
CC=gcc
CFLAGS=-I$(IDIR) -O2 -Wall
LFLAGS = -lglfw -lGLEW -lX11 -lGL -lGLU -lfmodex64
ODIR=obj
SRCDIR=../../source

#_OBJ = %.o
#OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

_OBJ = main.o action_implementation.o action_invocations.o
_OBJ += ai_logic.o ball.o fill_player_data.o font.o
_OBJ += game_analysis.o game_manipulation.o game_screen.o
_OBJ += immutable_world.o input.o
_OBJ += loadobj.o main_menu.o mutable_world.o
_OBJ += player.o render.o simplexml.o
_OBJ += sound.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

obj/%.o: ../../source/%.c
	$(CC) -c -o $@ $^ $(CFLAGS)

#obj/main.o: ../../source/main.c $(DEPS)
#	$(CC) -c -o obj/main.o ../../source/main.c $(DEPS) $(CFLAGS)

main: $(OBJ)
	$(CC) $^ -o $@ $(CFLAGS) $(LFLAGS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(IDIR)/*~ 
