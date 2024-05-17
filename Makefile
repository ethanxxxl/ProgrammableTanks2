# PROGRAM: Programable Tanks 2
# AUTHOR: Ethan Smith
# DATE: 6 January 2024
#
# @file
# @version 0.1

CC = clang
CFLAGS = -Wall -Wextra -Werror -Wunused-result -pedantic -ftrapv -std=gnu2x -g3 -O0

BUILDDIR = target

COMMON_DIR = common/src
SRC_COMMON = message.c vector.c command-line.c

SERVER_DIR = server/src
SRC_SERVER = main.c scenario.c player_manager.c server-commands.c

CLIENT_DIR = client/src
SRC_CLIENT = client.c client-commands.c client-gfx.c game-manager.c

# unit tests will work differently, each unit will have a main function.
TESTER_DIR = unit-tests
SRC_TESTER = vector-test.c message-test.c

# mains included here to filter out when running tests.
MAINS = $(CLIENT_DIR)/client.c $(SERVER_DIR)/main.c
OBJ_MAINS = $(patsubst %.c,$(BUILDDIR)/%.o,$(MAINS))

# mirror the program directory structure in the build directory.
OBJ_COMMON = $(patsubst %.c,$(BUILDDIR)/$(COMMON_DIR)/%.o,$(SRC_COMMON))
OBJ_SERVER = $(patsubst %.c,$(BUILDDIR)/$(SERVER_DIR)/%.o,$(SRC_SERVER))
OBJ_CLIENT = $(patsubst %.c,$(BUILDDIR)/$(CLIENT_DIR)/%.o,$(SRC_CLIENT))
OBJ_TESTER = $(patsubst %.c,$(BUILDDIR)/$(TESTER_DIR)/%.o,$(SRC_TESTER))

OBJ = $(OBJ_COMMON) $(OBJ_SERVER) $(OBJ_CLIENT) $(OBJ_TESTER)

INC = -Icommon/include -Iclient/include -Iserver/include
LIB = -lSDL2 -lm -pthread -lreadline

SERVER_BIN = $(BUILDDIR)/server-app
CLIENT_BIN = $(BUILDDIR)/client-app
UNIT_TESTS = $(patsubst %.c,$(BUILDDIR)/$(TESTER_DIR)/%,$(SRC_TESTER))

.PHONY: clean test

all: $(SERVER_BIN) $(CLIENT_BIN) $(TESTS)

$(SERVER_BIN): $(OBJ_SERVER) $(OBJ_COMMON)
	@echo -e "\033[0;33mbuilding executable: \033[1m$@\033[0m\033[0m"
	@$(CC) $(CFLAGS) $(LIB) $(INC) -o $@ $(OBJ_SERVER) $(OBJ_COMMON)

$(CLIENT_BIN): $(OBJ_CLIENT) $(OBJ_COMMON)
	@echo -e "\033[0;33mbuilding executable: \033[1m$@\033[0m\033[0m"
	@$(CC) $(CFLAGS) $(LIB) $(INC) -o $@ $(OBJ_CLIENT) $(OBJ_COMMON)

# Static substitution. The filestructure of the source code is mirrored in the
# build directory. This allows us to derive the .c file paths from the .o file
# paths.
$(OBJ): $(BUILDDIR)/%.o : %.c \
			  $(TESTER_DIR)/unit-test.h # unit-test header-only lib
	@mkdir -p $(@D)
	@echo -e "\033[32mcompiling \033[1m$<\033[0m\033[0m"
	@$(CC) $(CFLAGS) $(INC) -c $< -o $@

test: $(UNIT_TESTS)
	@echo -e "\033[1m---RUNNING TESTS---\033[0m\n"
	@$(patsubst %,./%;,$(UNIT_TESTS))
	@echo -e "\n\033[1m---TESTS FINISHED---\033[0m"

# compile tests. since each test has its own main function, all but the target
# main must be filtered out in the recipe.
#
# for the prerequisites, we filter out the mains from the other normal targets.
$(UNIT_TESTS): $(filter-out $(OBJ_MAINS),$(OBJ))
	@echo -e "\033[33mcompling test \033[1m$@\033[0m\033[0m"
	@$(CC) $(CFLAGS) $(LIB) $(INC) -o $@ \
		$@.o $(filter-out $(OBJ_TESTER) $(OBJ_MAINS),$(OBJ))

clean:
	rm -rf $(BUILDDIR)
