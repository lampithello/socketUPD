/*
 * server.c
 *
 * Author: Andrea Lampitelli
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined WIN32
#include <winsock.h>
#else
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#define closesocket close
#endif


#include "protocolServer.h"

// Server and client socket addresses and length of the client address
struct sockaddr_in server_address;
struct sockaddr_in client_address;
unsigned int client_address_length = sizeof(client_address);

// Function to handle errors
void errorHandler(char *error_message){
    printf("%s", error_message);
}

// Function to clean up Winsock on Windows
void clearWinsock(){
    #if defined WIN32
        WSACleanup();
    #endif
}

// Function to perform addition
int add(int a, int b) {
    return a + b;
}

// Function to perform multiplication
int mult(int a, int b) {
    return a * b;
}

// Function to perform subtraction
int sub(int a, int b) {
    return a - b;
}

// Function to perform division
float division(int a, int b) {
    return (float)a / b;
}

// Function to send the response to the client
void sendResponseToClient(int socket, struct sockaddr_in* client_address, int a, char operation, int b, float result) {
    char responseMessage[128];

    // Use a different format for division
    if (operation == '/') {
        sprintf(responseMessage, "Received result from server %s, IP %s: %d %c %d = %.2f\n",
                SERVER_NAME, "127.0.0.1", a, operation, b, result);
    } else {
        sprintf(responseMessage, "Received result from server %s, IP %s: %d %c %d = %.0f\n",
                SERVER_NAME, "127.0.0.1", a, operation, b, result);
    }

    // Send the response to the client
    if (sendto(socket, responseMessage, strlen(responseMessage), 0,
               (struct sockaddr *)client_address, sizeof(*client_address)) != strlen(responseMessage)) {
        errorHandler("sendto() sent a different number of bytes than expected");
        closesocket(socket);
        clearWinsock();
    }
}

// Function to handle client communication
void handle_client(int my_socket){

    char buffer[BUFFMAX];

    char operation;
    int a, b;

    while (1){
        // Print a message indicating that the server is listening
        puts("\nServer listening...");
        // clean buffer
        memset(buffer, 0, BUFFMAX);
        // receive message from client
        if ((recvfrom(my_socket, buffer, BUFFMAX, 0,
                                    (struct sockaddr *)&client_address, &client_address_length)) < 0){
            errorHandler("recvfrom() failed");
            closesocket(my_socket);
            clearWinsock();
        }

        // Get the host information for the client
        struct hostent *client_host = gethostbyaddr((char *)&client_address.sin_addr.s_addr, sizeof(client_address.sin_addr.s_addr), AF_INET);
        // Print the received operation and client information
        printf("Received operation '%s' from client %s (IP %s)\n", buffer, client_host->h_name, inet_ntoa(client_address.sin_addr));

        // Parse the received buffer to extract operation and operands
        int parsed = sscanf(buffer, " %c %d %d", &operation, &a, &b);

        // Process the received operation
        if (parsed == 3) {

            float result;

            // Perform the specified operation
            switch (operation) {
                case '+':
                    result = add(a, b);
                    break;

                case 'x':
                    result = mult(a, b);
                    break;

                case '-':
                    result = sub(a, b);
                    break;

                case '/':
                    result = division(a, b);
                    break;
            }

            // Send the response back to the client
            sendResponseToClient(my_socket, &client_address, a, operation, b, result);
         }
    }
}

// Main function
int main(void){
#if defined WIN32
    // Initialize Winsock on Windows
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        errorHandler("Failed to initialize Winsock.\n");
        return 1;
    }
#endif

    int my_socket;

    // create a UDP socket
    if ((my_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0){
        errorHandler("Error creating socket");
        closesocket(my_socket);
        clearWinsock();
        return 1;
    }

    // set the server address
    struct sockaddr_in server_address;

    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_address.sin_port = htons(DEFAULT_PORT);

    if ((bind(my_socket, (struct sockaddr *)&server_address, sizeof(server_address))) < 0){
        errorHandler("bind() failed");
        closesocket(my_socket);
        clearWinsock();
        return 1;
    }

    // Handle client communication
    handle_client(my_socket);

    // Close the socket and clean up Winsock
    closesocket(my_socket);

    // Clean up Winsock on Windows
    clearWinsock();

    return 0;
}
