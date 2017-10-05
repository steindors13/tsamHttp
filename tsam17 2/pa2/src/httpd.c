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
#include <glib.h>

#define GET 1
#define POST 2

char webpage[] =
"HTTP/1.1 200 OK\r\n"
"Content-type: text/html; charset=UTF -8\r\n\r\n"
"<!DOCTYPE html>\r\n"
"<html>\r\n"
"<body><h1>An IP address should be here, plus the port number</h1><br>\r\n"
"</body></html>\r\n";
/*
void printClientAddr(&struct sockaddr_in address)
{
	
	unsigned char *ip = (unsigned char *)&client_addr;
	printf("hahah %d.%d.%d.%d\n", ip[0], ip[1], ip[2], ip[3]);
}
*/
void handle_status_request(int request, char *url, int fd_client)
{
	int fd_server;
	fd_server = *webpage;
	if(fd_server < 0)
	{
		printf("can't find webpage to send");
		exit(0);
	}
	 
	if(request == GET)
	{	
		int nread;
		while ( (nread = read(fd_server, webpage, sizeof(webpage) )) > 0)
		{
			write(fd_client, webpage, nread);
		}
		close(fd_server);
	}
}
 
void handle_http_request(int fd_client)
{
	//Message buffer to store information in
	char Message[5000];
	// Read the request
	FILE *fp;
	fp = fdopen(fd_client, "r");
	if(!fp) 
	{
		printf("Error reading file, while processing http");
		exit(0);
	} 
	// Determine what kind of request it is
	char *c = NULL;
	c = fgets(Message, sizeof(Message), fp);
	if(!c)
	{
		printf("Can't determine request!\n");
		exit(0);
	}
	printf("Processing: %s\n", c);
	
	//Get, Post or even Head ?
	int request = GET;
	c = strtok(c, " \r\n");
	printf("%s requested \n", c);
	
	if(strcmp(c, "GET") == 0)
	{
		request = GET;
	}
	
	if(strcmp(c, "POST") == 0)
	{
		request = POST;
	}
	
	//requested url
	char *url = NULL;
	url = strtok(NULL, " \r\n");
	if(!url)
	{
		printf("NO URL\n");
	}
	if(url[0] == '/')
	{
		url = &url[1];
	}	
	printf("url is: %s\n", url);

	handle_status_request(request, &url, fd_client);

}

int main(int argc, char *argv[])
{
	struct sockaddr_in server_addr, client_addr;  // internet address
	socklen_t sin_len = sizeof(client_addr);  // size of address
	int fd_server, fd_client; 
	//char buff_cli[8192];
	//char buff_serv[8192];
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

	// Bind to socket
	if(bind(fd_server, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0)
	{
		perror("bind");
		close(fd_server);
		exit(1);
	}
	
	// Listen for a connection
	if(listen(fd_server, 10) < 0)
	{
		perror("listen");
		close(fd_server);
		exit(1);
	}

	while(1)
	{
		fd_client = accept(fd_server, (struct sockaddr *) &client_addr, &sin_len);

		if(fd_client < 0)
		{
			perror("Connection failed.......");
			continue;
		}

		printf("Got client connection.......\n");
		
		// Determine what h
		handle_http_request(fd_client); 	
		printf("closing...\n");
		
		close(fd_client);
		close(fd_server);
		exit(0);
	} 
	
	return 0;
}
