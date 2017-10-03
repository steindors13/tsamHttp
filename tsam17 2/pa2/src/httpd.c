#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>

char webpage[] =
"HTTP/1.1 200 OK\r\n"
"Content-type: text/html; charset=UTF -8\r\n\r\n"
"<!DOCTYPE html>\r\n"
"<html>\r\n"
"<body><h1>An IP address should be here, plus the port number</h1><br>\r\n"
"</body></html>\r\n";
 

int main(int argc, char *argv[])
{
	struct sockaddr_in server_addr, client_addr;  // internet address
	socklen_t sin_len = sizeof(client_addr);  // size of address
	int fd_server, fd_client; 
	char buff_cli[2048];
	char buff_serv[2048];
	int on = 1;
	int port = strtol(argv[1], NULL, 10);
	printf("Starting server, %d arguments\n", argc);
		
	// create and bind a TCP socket
	fd_server = socket(AF_INET, SOCK_STREAM, 0); // returns positive if success
	if(fd_server < 0)
	{
		perror("socket error");
		exit(1);
	}
	
	memset(&server_addr, 0, sizeof(server_addr));
	setsockopt(fd_server, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int));
	
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(port);
	
	if(bind(fd_server, (struct sockaddr *) &server_addr, sizeof(server_addr)) == -1)
	{
		perror("bind");
		close(fd_server);
		exit(1);
	}

	if(listen(fd_server, 10) == -1)
	{
		perror("listen");
		close(fd_server);
		exit(1);
	}

	while(1)
	{
		fd_client = accept(fd_server, (struct sockaddr *) &client_addr, &sin_len);

		if(fd_client == -1)
		{
			perror("Connection failed.......");
			continue;
		}

		printf("Got client connection.......\n");
		//FILE *f;
		//f = fopen("x.log", "a+");

		//close(fd_server);
		memset(buff_cli, 0, 2048);
		memset(buff_serv, 0, 2048);
		read(fd_client, buff_cli, 2047);
		//read(fd_server, buff_serv, 2047);
		
		char *pch = strtok(buff_cli, " \r\n");
		while(&pch != "\r\n")
		{
			printf("%s\n", pch);
			pch = strtok(NULL, " \r\n");
		}
		//for(int i = 0; i < 50; i++)
		//{
			//printf("%s\n", pch);
			//pch = strtok(NULL, " \r\n");
		//}
		printf("%s\n", buff_cli);
		//printf("%s\n", buff_serv);
		//fprintf(f, "Im logging somethig ..\n");

		write(fd_client, webpage, sizeof(webpage) - 1);
		//write(fd_server, webpage, sizeof(webpage) - 1);
			
		printf("closing...\n");
		
		close(fd_client);
		close(fd_server);
		exit(0);
	} 
	
	return 0;
}
