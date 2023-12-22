#include <fcntl.h>
#include <pthread.h>
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

int debug_send_msg(struct message msg) {
    printf("--SENDING--\n");
    print_message(msg);
    return send_message(g_server_sock, msg);    
}
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

void authenticate(int argc, char **argv) {
    struct message msg = {MSG_REQUEST_AUTHENTICATE};
    
    // FIXME: hardcoded size.
    strncpy(msg.user_credentials.username, argv[1], 50);
    debug_send_msg(msg);
}
void change_state(int argc, char **argv) {
    if (argc != 2) {
	printf("ERROR: valid options are \"scene\" or \"lobby\"\n");
	return;
    }

    struct message msg;
    if (strcmp(argv[1], "scene") == 0) {
        msg = (struct message){MSG_REQUEST_JOIN_SCENARIO};
    } else if (strcmp(argv[1], "lobby") == 0) {
	msg = (struct message){MSG_REQUEST_RETURN_TO_LOBBY};
    } else {
        printf("ERROR: valid options are \"scene\" or \"lobby\"\n");
	return;
    }

    debug_send_msg(msg);
    return;
}
void list_scenarios(int argc, char **argv) {
    struct message msg = {MSG_REQUEST_LIST_SCENARIOS};
    debug_send_msg(msg);
}

void add_tank(int argc, char **argv) {}

void help_page(int argc, char **arg) { printf("%s", HELP_STRING); }


void message_server(int argc, char **argv) {
    struct message msg = {MSG_REQUEST_DEBUG, .debug_msg = {0}};

    char* str_end = msg.debug_msg;
    for (int i = 1; i < argc; i++) {
	// --BUG-- there is a segfault here which needs fixed.
	strncpy(str_end, argv[i], 50-(str_end-msg.debug_msg));
	str_end += strlen(argv[i]);
	*str_end++ = ' ';
    }
    *(str_end-1) = '\0'; // get rid of final space

    debug_send_msg(msg);
}
void command_error(int argc, char **argv) {
    printf("ERROR: Unknown Command \"%s\"\n", argv[0]);
}
void quit(int argc, char **argv) {
    g_run_program = false;
}

void dummy(int arggc, char **argv) {}

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
    {"auth", &authenticate},
    {"change-state", &change_state},
    {"list-scenarios", &list_scenarios},
    {"help", &help_page},
    {"msg", &message_server},
    {"\n", &dummy},
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

void* read_msg_thread(void* arg) {
    struct message msg = {0};
    struct vector msg_buf;
    make_vector(&msg_buf, sizeof(char), 30);
    fcntl(g_server_sock, F_SETFL, O_NONBLOCK);
    
    while (g_run_program) {
        int status = recv_message(g_server_sock, &msg, &msg_buf);

	if (status != -1) {
	    print_message(msg);
	    free_message(msg);
	}
    }

    return NULL;
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
    
    g_run_program = true;
    char* buff = malloc(50);
    size_t buff_size = 50;

    pthread_t read_msg_pid;
    pthread_create(&read_msg_pid,
		   NULL,
		   &read_msg_thread,
		   NULL);
    
    
    while (g_run_program) {
        printf("\n> ");
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
