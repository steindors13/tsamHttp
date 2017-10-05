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
#include <time.h>

char webpage[] =
"HTTP/1.1 200 OK\r\n"
"Content-type: text/html; charset=UTF -8\r\n\r\n"
"<!DOCTYPE html>\r\n"
"<html>\r\n"
"<body><h1>An IP address should be here, plus the port number</h1><br>\r\n"
"</body></html>\r\n";

void write_logfile(int client, char* ip_addr, int port_nr)
{
	FILE *fp;
	FILE *f;
        printf("Got client connection.......\n");
        char buff_cli[8192];
	f = fopen("x.log", "a+");
	fprintf(f, "client ip: %s ", ip_addr);
	fprintf(f, "client port: %d ", port_nr);

        fp = fdopen(client, "r");
        char* data = fgets(buff_cli, sizeof(buff_cli), fp);
	data = strtok(data, " \r\n");
	fprintf(f, "Request is: %s ", data);

	time_t timer;
    	char time_buff[26];
    	struct tm* time_info;

    	time(&timer);
    	time_info = localtime(&timer);

    	strftime(time_buff, 26, "%Y-%m-%d %H:%M:%S", time_info);
    	puts(time_buff);
	fprintf(f, "Timestamp: %s ", time_buff);
	fclose(f);
	
}

/*void printClientAddr(&struct sockaddr_in address)
{
	
	unsigned char *ip = (unsigned char *)&client_addr;
	printf("hahah %d.%d.%d.%d\n", ip[0], ip[1], ip[2], ip[3]);
}*/
 

int main(int argc, char *argv[])
{
	struct sockaddr_in server_addr, client_addr;  // internet address
	socklen_t sin_len = sizeof(client_addr);  // size of address
	int fd_server, fd_client; 
	//char buff1[10000];
	//char buff2[200];
	int on = 1;
	char* ip_addr;
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
		ip_addr = inet_ntoa(client_addr.sin_addr);
		if(fd_client < 0)
		{
			perror("Connection failed.......");
			continue;
		}
		
		printf("Got client connection.......\n");	
		write_logfile(fd_client, ip_addr, port);
		
		printf("closing...\n");
		
		close(fd_client);
		close(fd_server);
		exit(0);
	} 
	
	return 0;
}
