/*
 * client.c
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


#include "protocolClient.h"

// Global declarations
struct sockaddr_in server_address;
unsigned int server_address_size = sizeof(server_address);
char buffer[BUFFMAX];

// Function to clean up Winsock on Windows
void clearWinsock(){
    #if defined WIN32
        WSACleanup();
    #endif
}

// Function to handle errors
void errorHandler(char *error_message) {
    printf("%s", error_message);
}

// Function to handle communication with the server
void handle_server(int my_socket){

    char operation;
    int a, b;

    while(1){

        // Asks the user to enter an operation and two numbers
        printf("Enter operation and two numbers (e.g., \"+ 4 6\" ): ");

        // Read user input
        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
            break;
        }

        buffer[strcspn(buffer, "\n")] = '\0';

        // Check if the input exceeds the maximum buffer size
        if (strlen(buffer) >= BUFFMAX - 1) {
            errorHandler("Input too long. Reduce the length of the input.\n\n");
            continue;
        }

        // Check if the user entered '=' to stop execution
        if (strcmp(buffer, "=") == 0){
            printf("\nYou stopped execution by typing '='");
            break;
        }

        // Parse the input for operation and numbers
        if (sscanf(buffer, "%c %d %d", &operation, &a, &b) != 3) {
            errorHandler("Invalid input. Please enter a valid operation and two numbers.\n\n");
            continue;
        }

        // Check for space after the operator
        if (buffer[1] != ' ') {
            errorHandler("Invalid input. After the operator put a space character.\n\n");
            continue;
        }

        // Check for a valid operator
        if (operation != '+' && operation != '-' && operation != 'x' && operation != '/') {
            errorHandler("Use a valid operator.\n\n");
            continue;
        }

        // Check for extra characters
        char extra;
        if (sscanf(buffer, "%*c %*d %*d %c", &extra) == 1) {
            errorHandler("Invalid input. Too many numbers.\n\n");
            continue;
        }

        // Check for division by zero
        if (b == 0 && operation == '/') {
            errorHandler("Invalid input. Division by 0.\n\n");
            continue;
        }

        // Send a message to the server
        if (sendto(my_socket, buffer, strlen(buffer), 0,
                    (struct sockaddr *)&server_address, sizeof(server_address)) != strlen(buffer)){
            errorHandler("Error sending data");
            closesocket(my_socket);
            clearWinsock();
        }

        memset(buffer, 0, BUFFMAX);

        // Receive a message from the server
        if ((recvfrom(my_socket, buffer, BUFFMAX, 0,
                                     (struct sockaddr *)&server_address, &server_address_size)) < 0){
            errorHandler("Error receiving data");
            closesocket(my_socket);
            clearWinsock();
        }

        // Print the received message
        printf("%s\n", buffer);

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

    // Read the name and port of the UDP server to contact
    memset(buffer, 0, BUFFMAX);
    char *delim = ":";
    puts("Enter the server name and port (in the format servername:port):");
    fgets(buffer, BUFFMAX, stdin);
    char *name;
    int port;

    // Handle the case where no address is entered from the command line
    if (strcmp(buffer, "\n") == 0){
        name = "localhost";
        port = 48000;
        printf("You choose the default option: '%s:%d'\n", name, port);
    }
    else{
        name = strtok(buffer, delim);
        port = atoi(strtok(NULL, delim));
    }

    struct hostent *host;

    host = gethostbyname(name);

    if (host == NULL) {
        errorHandler("Error: Unable to resolve the name %s.");
        exit(1);
    }

    // Create a UDP socket
    if ((my_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0){
        errorHandler("Error creating socket");
        closesocket(my_socket);
        clearWinsock();
        return 1;
    }

    // Set the server address
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr = *((struct in_addr *)host->h_addr_list[0]);
    server_address.sin_port = htons(DEFAULT_PORT);

    // Handle communication with the server
    handle_server(my_socket);

    // Close the socket
    closesocket(my_socket);

    // Clean up Winsock on Windows
    clearWinsock();

    return 0;
}
