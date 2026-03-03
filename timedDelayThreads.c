// Name: Fahad Arif (N01729165)
// Course: Networking & Telecomm (CPAN-226)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>        // This is the GetTickCount64
#define sleep(x) Sleep(1000 * (x))
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#define SOCKET int
#define INVALID_SOCKET (-1)
#endif

#include <pthread.h>

#define PORT 8080
#define TARGET_CLIENTS 5

// This is the helper for the timing
static unsigned long long now_ms(void) {
#ifdef _WIN32
    return (unsigned long long)GetTickCount64();
#else
    return 0;
#endif
}

//This is the original handler
void handle_client(SOCKET client_socket, int client_id) {
    char *message = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 13\r\nConnection: close\r\n\r\nHello Client!";
    
    printf("[Server] Handling client %d...\n", client_id);
    
    // Simulating "bottleneck" or heavy processing
    printf("[Server] Processing request for 5 seconds...\n");
    sleep(5); 
    
    send(client_socket, message, (int)strlen(message), 0);
    printf("[Server] Response sent to client %d. Closing connection.\n", client_id);
    
#ifdef _WIN32
    closesocket(client_socket);
#else
    close(client_socket);
#endif
}

// This is the thread argument
typedef struct {
    SOCKET client_socket;
    int client_id;
} 
client_args_t;

// This is for tracking completion to pring the "Total Elapsed Time"
static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  g_cond  = PTHREAD_COND_INITIALIZER;
static int g_completed = 0;

// This is the thread function
void* client_thread(void* arg) {
    client_args_t* args = (client_args_t*)arg;

    handle_client(args->client_socket, args->client_id);

    free(args);

    // This will mark one client as done
    pthread_mutex_lock(&g_mutex);
    g_completed++;
    pthread_cond_signal(&g_cond);
    pthread_mutex_unlock(&g_mutex);

    return NULL;
}

// This is the main method
int main() {
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

    SOCKET server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    int addr_len = sizeof(client_addr);
    int client_count = 0;

    server_socket = socket(AF_INET, SOCK_STREAM, 0);

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));
    listen(server_socket, 5);

    printf("The server is listening on port %d...\n", PORT);
    printf("NOTE: This server is MULTITHREADED. It is capable of handling clients concurrently!\n\n");

    // This will start timing the batch
    unsigned long long start = now_ms();

    // This will accept exactly TARGET_CLIENTS and create threads for them
    while (client_count < TARGET_CLIENTS) {
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_len);
        if (client_socket != INVALID_SOCKET) {
            client_count++;

            client_args_t* args = (client_args_t*)malloc(sizeof(client_args_t));
            if (!args) {
                printf("[Server] malloc failed. Closing client.\n");
#ifdef _WIN32
                closesocket(client_socket);
#else
                close(client_socket);
#endif
                continue;
            }

            args->client_socket = client_socket;
            args->client_id = client_count;

            pthread_t tid;
            int rc = pthread_create(&tid, NULL, client_thread, args);
            if (rc != 0) {
                printf("[Server] pthread_create failed (rc=%d). Closing client.\n", rc);
                free(args);
#ifdef _WIN32
                closesocket(client_socket);
#else
                close(client_socket);
#endif
                continue;
            }

            // This will detach so we don't need pthread_join
            pthread_detach(tid);
        }
    }

    // This will wait until all the TARGET_CLIENTS threads are finished
    pthread_mutex_lock(&g_mutex);
    while (g_completed < TARGET_CLIENTS) {
        pthread_cond_wait(&g_cond, &g_mutex);
    }
    pthread_mutex_unlock(&g_mutex);

    unsigned long long end = now_ms();
    printf("\nTOTAL ELAPSED TIME: %.3f seconds\n", (end - start) / 1000.0);

#ifdef _WIN32
    closesocket(server_socket);
    WSACleanup();
#else
    close(server_socket);
#endif
    return 0;
}