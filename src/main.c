#include "message.h"
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

#include <tank.h>
#include <message.h>
#include <pthread.h>
#include <fcntl.h>

struct client {
    int socket;
    socklen_t size;
    struct sockaddr address;
    
    bool authenticated;
    char username[50];
};

// requests username and determines if the user should be allowed to
// enter the game. If the user is valid, then a new client structure
// is allocated. Otherwise, NULL is returned.
bool authenticate_connection(struct client* client, struct client_message *credentials) {
    // TODO add some sort of credential validation.
    // for now, just set the username and set the authenticated flag.
    if (credentials->msg_type != CLIENT_USER_CREDENTIALS)
	return false;
    
    client->authenticated = true;
    strcpy(client->username, credentials->user_credentials.username);

    return true;
}

bool g_run;
struct client *g_connections[50];
int g_connections_len;


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

	struct client *new_client;
	new_client = malloc(sizeof(struct client));
	if (new_client == NULL) {
	    perror("ERROR! COULDN'T MALLOC FOR NEW CLIENT");
	    continue;
	}

	new_client->size = client_size;
	new_client->address = client_addr;
	new_client->socket = client_fd;

	//FIXME allocated memory never freed!
    
	printf("recieved a new connection!\n");
	g_connections[g_connections_len] = new_client;
	g_connections_len += 1;
    }

    shutdown(sock, SHUT_RDWR);
    return NULL;
}


// BUG!!! there is a potential race condition here. If a client with
// the same credentials attempts to enter the game while the old
// client is being handled, the old clients memory will be freed out
// from under it!
void handle_client(struct client* client) {
    struct client_message msg;
    // FIXME this should probably be changed to a non-blocking read.
    int status = recv_client_message(client->socket, &msg);

    // return if nothing was read.
    if (status < 0)
	return;

    // TODO handle all cases of the message
    if (msg.msg_type != CLIENT_USER_CREDENTIALS &&
	!client->authenticated) {
	// can't do anything unless the client is authenticated.
	return;
    }

    switch (msg.msg_type) {
    case CLIENT_USER_CREDENTIALS: {
	bool result = authenticate_connection(client, &msg);
	
	if (result)
	    printf("%s: authenticated\n", client->username);
	
	break;
    }
    case CLIENT_DEBUG_MESSAGE: {
	printf("%s: %s\n", client->username, msg.debug_msg);
	
       	if (strcmp(msg.debug_msg, "kill-serv") == 0) {
	    g_run = false;
	}
	break;
    }
    case CLIENT_REQUEST_WORLD: 
    case CLIENT_PROPOSE_UPDATE:
	printf("ERROR: UNSUPPORTED OPERATION\n");
    }
}

// this function handles all requests from clients.
void* client_request_thread(void *arg) {
    while (g_run) {	
	// you will handle stuff here, like printing debug messages!
	for (int i = 0; i < g_connections_len; i++) {
	    handle_client(g_connections[i]);
	}
    }

    return NULL;
}
int main(int argc, char** argv) {
    // structure initialization
    init_tank_list(50);

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
