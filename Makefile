##
# ProgramableTanks2
#
# @file
# @version 0.1

CC = clang
CFLAGS = -Wall -Wextra -Werror -pedantic -ftrapv -std=gnu17 -g 

SRCDIR = src
BUILDDIR = target

SRC_COMMON = src/message.c src/vector.c 

SRC_SERVER = $(SRC_COMMON) src/main.c src/scenario.c src/player_manager.c 
SRC_CLIENT = $(SRC_COMMON) src/client.c

OBJ_COMMON = $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/%.o,$(SRC_COMMON))
OBJ_SERVER = $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/%.o,$(SRC_SERVER))
OBJ_CLIENT = $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/%.o,$(SRC_CLIENT))

OBJ = $(OBJ_COMMON) $(OBJ_SERVER) $(OBJ_CLIENT)

INC = -I./include/
LIB_CLIENT = -lSDL2 -lm -pthread
LIB_SERVER = -pthread -lm

all: $(BUILDDIR)/server $(BUILDDIR)/client

$(BUILDDIR)/server: $(OBJ_SERVER) 
	$(CC) $(CFLAGS) $(LIB_SERVER) $(INC) -o $@ $(OBJ_SERVER)

$(BUILDDIR)/client: $(OBJ_CLIENT)
	$(CC) $(CFLAGS) $(LIB_CLIENT) $(INC) -o $@ $(OBJ_CLIENT)

$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) $(INC) -c $< -o $@

clean:
	rm -f $(OBJ)
