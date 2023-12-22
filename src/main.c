#include "message.h"
#include "player_manager.h"
#include <SDL2/SDL.h>
#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;

//int main(int argc, char** argv) {
//    SDL_Window* window = NULL;
//    SDL_Surface* screenSurface = NULL;
//
//    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
//        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
//    } else {
//        window = SDL_CreateWindow("SDL Tutorial",
//                                  SDL_WINDOWPOS_UNDEFINED,
//                                  SDL_WINDOWPOS_UNDEFINED,
//                                  SCREEN_WIDTH,
//                                  SCREEN_HEIGHT,
//                                  SDL_WINDOW_SHOWN);
//        if (window == NULL) {
//            printf("SDL could not create a window! SDL_Error: %s\n",
//                   SDL_GetError());
//        } else {
//            screenSurface = SDL_GetWindowSurface(window);
//            SDL_FillRect(screenSurface, NULL, SDL_MapRGB(screenSurface->format,
//                                                         0xff, 0xff, 0xff));
//            SDL_UpdateWindowSurface(window);
//            SDL_Event e; bool quit = false; while( quit == false ){ while( SDL_PollEvent( &e ) ){ if( e.type == SDL_QUIT ) quit = true; } }
//
//            SDL_DestroyWindow(window);
//            SDL_Quit();
//        }}
//
//    return 0;
//}
#include <scenario.h>
#include <message.h>
#include <pthread.h>
#include <fcntl.h>

bool g_run;
extern struct scenario g_scenario;

struct {
    struct player_manager* client;
    struct vector msg_buf;
} g_connections[50];

int g_connections_len;

// TODO: should eventually support multiple scenarios
struct scenario g_scenario;

void *accept_connections_thread(void* arg) {
    // create a socket
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("ERROR: failed to open network socket");
        exit(EXIT_FAILURE);
    }

    // give the socket a name
    struct sockaddr_in name =
        { .sin_family = AF_INET,
          .sin_addr = {inet_addr("127.0.0.1")},
          .sin_port = 4444};

    int status = bind(sock, (struct sockaddr*) &name, sizeof(name));
    if (status < 0) {
        perror("ERROR: couldn't bind socket.");
        exit(EXIT_FAILURE);
    }
 
    // enable listening
    listen(sock, 1);
    g_connections_len = 0;

    while (g_run) {
	struct sockaddr client_addr;
	socklen_t client_size;

	int client_fd = accept(sock, &client_addr, &client_size);

	// set the connection to nonblocking
	fcntl(client_fd, F_SETFL, O_NONBLOCK);

	struct player_manager *new_player;
	new_player = malloc(sizeof(struct player_manager));
	if (new_player == NULL) {
	    perror("ERROR! COULDN'T MALLOC FOR NEW CLIENT");
	    continue;
	}

	new_player->size = client_size;
	new_player->address = client_addr;
	new_player->socket = client_fd;
	new_player->state = STATE_IDLE;

	// FIXME: allocated memory never freed!
    	printf("recieved a new connection!\n");
	g_connections[g_connections_len].client = new_player;
	make_vector(&g_connections[g_connections_len].msg_buf,
		    sizeof(char), 10);
	g_connections_len += 1;
    }

    shutdown(sock, SHUT_RDWR);
    return NULL;
}


// BUG: there is a potential race condition here. If a client with the same
// credentials attempts to enter the game while the old client is being handled,
// the old clients memory will be freed out from under it!

/// top level client handling function that recieves all messages from the
/// clients, and passes them to the appropriate handler (depends on the state of
/// the client)
void handle_client(struct player_manager* p, struct vector *msg_buf) {
    struct message msg;
    int status = recv_message(p->socket, &msg, msg_buf);
    
    // return if nothing was read.
    if (status == -1) {
        return;
    }
    
    print_player(p);
    printf("--RECEIVED--\n");
    // XXX: debug message.
    print_message(msg);
    
    switch (p->state) {
    case STATE_DISCONNECTED:
	break;
    case STATE_IDLE:
	player_idle_handler(p, msg);
	break;
    case STATE_LOBBY:
	player_lobby_handler(p, msg);
	break;
    case STATE_SCENARIO:
	player_scenario_handler(p, msg);
	break;
    }
    
    if (msg.type == MSG_REQUEST_DEBUG) {
	printf("%s: %s\n", p->username, msg.debug_msg);
	
        if (strcmp(msg.debug_msg, "kill-serv") == 0) {
            g_run = false;
        }
    }

    free_message(msg);
    return;
}

// this function handles all requests from clients.
void* client_request_thread(void *arg) {
    while (g_run) {	
	// you will handle stuff here, like printing debug messages!
	for (int i = 0; i < g_connections_len; i++) {
	    handle_client(g_connections[i].client,
			  &g_connections[i].msg_buf);
	}

	/* TEMPORARY (probably) SCENE HANDLING */
	scenario_handler(&g_scenario);
    }

    return NULL;
}

int main(int argc, char** argv) {
    make_scenario(&g_scenario);
    
    // start networking thread
    g_run = true;

    pthread_t network_thread_pid;
    pthread_create(&network_thread_pid,
		   NULL,
		   &accept_connections_thread,
		   NULL);
    
    pthread_t client_thread_pid;
    pthread_create(&client_thread_pid,
		   NULL,
		   &client_request_thread,
		   NULL);

    // run updates.
    while (g_run);
}
