#include <stdbool.h>
#include <stddef.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <message.h>

const char HELP_STRING[] =
    "available commands:\n"
    "    help    :: display this help page\n"
    "    q       :: quit this program\n"
    "    update  :: proposes an update to the server\n"
    "    request :: requests a map update from the server\n";

/* GLOBAL VARIABLES
 */
bool g_run_program;
int g_server_sock;

/*
 * Command Callback Functions
 */

void propose_update(int argc, char **argv) {
    printf("proposing update...\n");
    printf("COMMAND FAILED: NOT IMPLEMENTED\n");
}
void request_server_update(int argc, char **argv) {
    printf("requesting info from server...\n");
    printf("COMMAND FAILED: NOT IMPLEMENTED\n");
}
void add_tank(int argc, char **argv) {
    
}
void help_page(int argc, char **arg) {
    printf("%s", HELP_STRING);
}
void message_server(int argc, char **argv) {
    struct client_message msg;
    msg.msg_type = CLIENT_DEBUG_MESSAGE;
    msg.debug_msg[0] = 0;

    char* str_end = msg.debug_msg;
    for (int i = 1; i < argc; i++) {
	// --BUG-- there is a segfault here which needs fixed.
	strncpy(str_end, argv[i], 50-(str_end-msg.debug_msg));
	str_end += strlen(argv[i]);
	*str_end++ = ' ';
    }
    *(str_end-1) = '\0'; // get rid of final space

    printf("sending \"%s\" to server.\n", msg.debug_msg);

    send_client_message(g_server_sock, &msg);
}
void command_error(int argc, char **argv) {
    printf("ERROR: Unknown Command \"%s\"\n", argv[0]);
}
void quit(int argc, char **argv) {
    g_run_program = false;
}

/*
 * Command Master List
 */
// master list of all commands in the client program. In the future, I
// may integrate the documentation into this structure as well, and
// make the help function generate the help text automatically.
const struct command {
    char name[20];
    void (*callback)(int, char**);
} COMMANDS[] = {
    {"update", &propose_update},
    {"add-tank", &add_tank},
    {"request", &request_server_update},
    {"help", &help_page},
    {"msg", &message_server},
    {"q", &quit},
    {"quit", &quit},
    {"exit", &quit},
};


// call the appropriate callback function for the command passed in.
void manage_commands(int argc, char** argv) {
    struct command const* cmd = COMMANDS;
    const size_t N_ELEM = sizeof(COMMANDS)/sizeof(struct command);

    while (strcmp((cmd)->name, argv[0]) != 0
	   && (cmd - COMMANDS) < N_ELEM) cmd++;
    
    if (cmd - COMMANDS >= N_ELEM) {
	command_error(argc, argv);
	return;
    }

    cmd->callback(argc, argv);
}


int main(int argc, char** argv) {    
    // create a socket
    g_server_sock = socket(PF_INET, SOCK_STREAM, 0);
    if (socket < 0) {
        perror("ERROR: failed to open network socket\n");
        exit(EXIT_FAILURE);
    }

    // give the socket a name
    struct sockaddr_in name =
        { .sin_family = AF_INET,
          .sin_addr = {inet_addr("127.0.0.1")},
          .sin_port = 4444};

    int status = connect(g_server_sock,
			 (struct sockaddr*) &name,
			 sizeof(name));
    if (status < 0) {
        perror("ERROR: couldn't connect to server.\n");
        exit(EXIT_FAILURE);
    }

    struct client_message authenticate_msg = {
	.msg_type = CLIENT_USER_CREDENTIALS,
    };

    if (argc >= 2) {
	strcpy(authenticate_msg.user_credentials.username,
	       argv[1]);
    } else {
	strcpy(authenticate_msg.user_credentials.username,
	       "default");
    }

    send_client_message(g_server_sock, &authenticate_msg);

    g_run_program = true;
    char* buff = malloc(50);
    size_t buff_size = 50;
    while (g_run_program) {
        printf("> ");
        fflush(stdout);

        getline(&buff, &buff_size, stdin);

        char* argv[10];
	int i = 0;
	for (char* token = strtok(buff, " \n");
	     token != NULL;
	     token = strtok(NULL, " \n")) {
	    argv[i++] = token;
	}

	manage_commands(i, argv);
    }

    printf("quitting program...\n");
    exit(EXIT_SUCCESS);
}
