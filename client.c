#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

#define PORT "8080"
#define SERVER "127.0.0.1"
#define NUM_REQUESTS 5

void* make_request(void* arg) {
    int id = *(int*)arg;
    free(arg);

    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(SERVER, PORT, &hints, &res) != 0) return NULL;
    SOCKET sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

    printf("[Request %d] Connecting to server...\n", id);
    if (connect(sock, res->ai_addr, (int)res->ai_addrlen) == SOCKET_ERROR) {
        printf("[Request %d] Connection failed.\n", id);
        freeaddrinfo(res);
#ifdef _WIN32
        closesocket(sock);
#else
        close(sock);
#endif
        return NULL;
    }

    printf("[Request %d] Connected! Waiting for response...\n", id);
    
    char buffer[1024];
    int bytes = recv(sock, buffer, sizeof(buffer) - 1, 0);
    if (bytes > 0) {
        printf("[Request %d] Server response received!\n", id);
    }

#ifdef _WIN32
    closesocket(sock);
#else
    close(sock);
#endif
    freeaddrinfo(res);
    return NULL;
}

int main() {
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

    pthread_t threads[NUM_REQUESTS];
    time_t total_start, total_end;
    
    printf("Starting %d PARALLEL requests to test server concurrency...\n", NUM_REQUESTS);
    time(&total_start);

    for (int i = 0; i < NUM_REQUESTS; i++) {
        int* id = malloc(sizeof(int));
        *id = i + 1;
        pthread_create(&threads[i], NULL, make_request, id);
    }

    for (int i = 0; i < NUM_REQUESTS; i++) {
        pthread_join(threads[i], NULL);
    }

    time(&total_end);
    double total_elapsed = difftime(total_end, total_start);
    printf("\n>>> TOTAL ELAPSED TIME FOR ALL REQUESTS: %.2f seconds <<<\n", total_elapsed);

#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
