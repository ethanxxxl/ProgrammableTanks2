#ifndef CLIENT_COMMANDS_H
#define CLIENT_COMMANDS_H

#include "sexp.h"
#include "command-line.h"

command_fn connect_serv;
command_fn start_gfx;
command_fn list_tanks;
command_fn propose_update;
command_fn update_tank;
command_fn request_server_update;
command_fn authenticate;
command_fn change_state;
command_fn list_scenarios;

command_fn message_server;
command_fn change_bg_color;

command_fn enable_print_messages;

command_fn dummy;
command_fn quit;

struct result_s32 debug_send_msg(sexp *msg);

#endif
