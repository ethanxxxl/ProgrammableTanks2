# PROGRAM: Programable Tanks 2
# AUTHOR: Ethan Smith
# DATE: 6 January 2024
#
# @file
# @version 0.1

CC = clang
CFLAGS = -Wall -Wextra -Werror -Wunused-result -pedantic -ftrapv -std=gnu17 -g 

BUILDDIR = target

COMMON_DIR = common/src
SRC_COMMON = message.c vector.c

SERVER_DIR = server/src
SRC_SERVER = main.c scenario.c player_manager.c

CLIENT_DIR = client/src
SRC_CLIENT = client.c

# mirror the program directory structure in the build directory.
OBJ_COMMON = $(patsubst %.c,$(BUILDDIR)/$(COMMON_DIR)/%.o,$(SRC_COMMON))
OBJ_SERVER = $(patsubst %.c,$(BUILDDIR)/$(SERVER_DIR)/%.o,$(SRC_SERVER))
OBJ_CLIENT = $(patsubst %.c,$(BUILDDIR)/$(CLIENT_DIR)/%.o,$(SRC_CLIENT))

OBJ = $(OBJ_COMMON) $(OBJ_SERVER) $(OBJ_CLIENT)

COMMON_INC = -Icommon/include
CLIENT_INC = $(COMMON_INC) -Iclient/include
SERVER_INC = $(COMMON_INC) -Iserver/include
LIB_CLIENT = -lSDL2 -lm -pthread
LIB_SERVER = -pthread -lm

SERVER_BIN = $(BUILDDIR)/server-app
CLIENT_BIN = $(BUILDDIR)/client-app

.PHONY: clean

all: $(SERVER_BIN) $(CLIENT_BIN)

$(SERVER_BIN): $(OBJ_SERVER) $(OBJ_COMMON)
	@echo -e "\033[1;33mbuilding executable: \033[1m$@\033[0m\033[0m"
	@$(CC) $(CFLAGS) $(LIB_SERVER) $(SERVER_INC) -o $@ $(OBJ_SERVER) $(OBJ_COMMON)

$(CLIENT_BIN): $(OBJ_CLIENT) $(OBJ_COMMON)
	@echo -e "\033[1;33mbuilding executable: \033[1m$@\033[0m\033[0m"
	@$(CC) $(CFLAGS) $(LIB_CLIENT) $(CLIENT_INC) -o $@ $(OBJ_CLIENT) $(OBJ_COMMON)

# Static substitution. The filestructure of the source code is mirrored in the
# build directory. This allows us to derive the .c file paths from the .o file
# paths.
$(OBJ): $(BUILDDIR)/%.o : %.c
	@mkdir -p $(@D)
	@echo -e "\033[32mcompiling \033[1m$<\033[0m\033[0m"
	@$(CC) $(CFLAGS) $(COMMON_INC) $(CLIENT_INC) $(SERVER_INC) -c $< -o $@

clean:
	rm -rf $(BUILDDIR)
