##
# ProgramableTanks2
#
# @file
# @version 0.1

CC = gcc
CFLAGS = -Wall -g

SRC_COMMON = src/tank.c src/message.c

SRC_SERVER = $(SRC_COMMON) src/main.c
SRC_CLIENT = $(SRC_COMMON) src/client.c

OBJ_COMMON = $(SRC_COMMON:.c=.o)
OBJ_SERVER = $(SRC_SERVER:.c=.o)
OBJ_CLIENT = $(SRC_CLIENT:.c=.o)

OBJ = $(OBJ_COMMON) $(OBJ_SERVER) $(OBJ_CLIENT)

INC = -I./include/
LIB = -lSDL2 -lm -pthread

all: server client

server: $(OBJ_SERVER) 
	$(CC) $(CFLAGS) $(LIB) $(INC) -o server $(OBJ_SERVER)

client: $(OBJ_CLIENT)
	$(CC) $(CFLAGS) $(LIB) $(INC) -o client $(OBJ_CLIENT)

%.o: %.c
	$(CC) $(CFLAGS) $(LIB) $(INC) -c $< -o $@

clean:
	rm -f $(OBJ)
