#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

int main (int argc, char * argv[]) {
    int sockfd, newsockfd, portnumber;
    socklen_t client;
    char buffer[256];
    struct sockaddr_in serv_addr, cli_addr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);


    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    portnumber = atoi(argv[1]);
    serv_addr.sin_port = htons(portnumber);

    bind(sockfd, (struct sockaddr *)& serv_addr, sizeof(serv_addr));
    listen(sockfd, 5);

    client = sizeof(cli_addr);
    newsockfd = accept(sockfd, (struct sockaddr*) & cli_addr, &client);

    bzero(buffer, 256);
    while(1) {
        read(newsockfd, buffer, 255);
        printf("Client -> %s\n", buffer);
    }
    
    close(newsockfd);
    close(sockfd);
}
