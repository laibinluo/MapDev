/*
*	filename   :mynet.h
*	author     :luolaibin
*	create     :2012-4-20
*
*/

#include<sys/types.h>
#include<ctype.h>
#include<string.h>
#include<unistd.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<netdb.h>
#include<arpa/inet.h>
#include<ctype.h>
#include<errno.h>
#include<sys/time.h>
#include<stdio.h>
#include<strings.h>
#include<sys/select.h>
#include<stdlib.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <time.h>

#define SERVER_PORT    6666
#define LENGTH_OF_LISTEN_QUEUE 20
#define BUFFER_SIZE 1024
#define FILE_NAME_MAX_SIZE 512


void main(int argc, char **argv)
{
	struct sockaddr_in server_addr;
	bzero(&server_addr, sizeof(server_addr));

	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons((uint16_t)SERVER_PORT);

	int server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if(server_socket < 0)
	{
		printf("Create Socket Failed!");
		exit(1);
	}

	if(bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
	{
		perror("bind error");
		exit(1);
	}

	if(listen(server_socket, LENGTH_OF_LISTEN_QUEUE))
	{
		perror("Listen error");
		exit(1);
	}

	while(1)
	{
		struct sockaddr_in client_addr;
		socklen_t length = sizeof(client_addr);

		int request_socket = accept(server_socket, (struct sockaddr *)&client_addr, &length);

		if(request_socket < 0)
		{
			perror("server accept failed!");
			break;
		}
        
        char buffer[BUFFER_SIZE] = "test server android";
		length = send(request_socket, buffer, strlen(buffer), 0);
		if(length < 0)
		{
			perror("Server send Data Failed!\n");
			break;
		}

		close(request_socket);
	}
	close(server_socket);
}

