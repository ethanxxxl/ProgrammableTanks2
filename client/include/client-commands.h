#ifndef CLIENT_COMMANDS_H
#define CLIENT_COMMANDS_H

#include "message.h"

void connect_serv(int argc, char** argv);
void start_gfx(int argc, char** argv);
void list_tanks(int argc, char** argv);
void propose_update(int argc, char** argv);
void update_tank(int argc, char** argv);
void request_server_update(int argc, char** argv);
void authenticate(int argc, char** argv);
void change_state(int argc, char** argv);
void list_scenarios(int argc, char** argv);

void message_server(int argc, char** argv);
void change_bg_color(int argc, char** argv);

void enable_print_messages(int argc, char** argv);

void dummy(int argc, char** argv);
void quit(int argc, char** argv);
void quit(int argc, char** argv);
void quit(int argc, char **argv);

int debug_send_msg(struct message msg);

#endif
