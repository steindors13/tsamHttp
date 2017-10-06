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

#define GET 1
#define POST 2
#define HEAD 3

int request;
int port_nr;
char *ip_addr;
char template[] =
"HTTP/1.1 200 OK\r\n"
"Content-type: text/html; charset=UTF -8\r\n\r\n"
"<!DOCTYPE html>\r\n"
"<html>\r\n"
"<body><h1>";

char tail[] = 
"</h1><br>\r\n"
"</body></html>\r\n";

char toSend[600];
char *url;

char* writeToBody()
{
	printf("writing to body\n");
	char temp[600];
	strcpy(temp, template);
	strcat(temp, "http://127.0.0.1");
	strcat(temp, url);
	strcat(temp, " ");
	strcat(temp, ip_addr);
	strcat(temp, ":");
	char str_port[10];
	sprintf(str_port, "%d", port_nr);

	strcat(temp, str_port); 
	strcat(temp, tail);
	strcpy(toSend, temp);
	
	printf("tosend = %s\n", toSend); 	
	return toSend;	
}


void write_logfile()
{
	FILE *f;
	
	f = fopen("x.log", "a+");
	
	time_t timer;
        char time_buff[26];
        struct tm* time_info;

        time(&timer);
        time_info = localtime(&timer);

        strftime(time_buff, 26, "%Y-%m-%d %H:%M:%S", time_info);
        puts(time_buff);
        fprintf(f, "%s : ", time_buff);

	fprintf(f, "%s : ", ip_addr);
	fprintf(f, "%d : ", port_nr);
	fprintf(f, "%d : ", request);
	fprintf(f, "%s : ", url);
	fclose(f);
}

void handle_status_request(int fd_client, FILE* f)
{
	int fd_server;
	char buff_post[10000];
	fd_server = (intptr_t) writeToBody();
	if(fd_server < 0)
	{
		printf("can't find webpage to send");
		exit(0);
	}
	 
	if(request == GET)
	{	
		int nread;
		while ( (nread = read(fd_server,toSend, sizeof(toSend) )) > 0)
		{
			write(fd_client, toSend, nread);
		}
		send(fd_client, toSend, strlen(toSend), 0);
		//close(fd_server);
	}
	else if(request == HEAD)
	{
		if(read(fd_client, toSend, sizeof(toSend)))
		{
			send(fd_client, toSend, strlen(toSend), 0);
		}
		else
			perror("Connection failed...");
	}
	else if(request == POST)
	{
		char *c  = NULL;
		//read(fd_client, buff_post, sizeof(buff_post));
		//write(fd_client, buff_post, sizeof(buff_post) - 1);
		c = fgets(buff_post, sizeof(buff_post), f);
		printf("%s", c);

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
	printf("determining\n");
	char *c = NULL;
	c = fgets(Message, sizeof(Message), fp);
	if(!c)
	{
		printf("Can't determine request!\n");
		exit(0);
	}
	
	//Get, Post or even Head ?
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
	url = NULL;
	url = strtok(NULL, " \r\n");
	if(!url)
	{
		printf("NO URL\n");
	} 

	printf("url is: %s\n", url);
	
	handle_status_request(fd_client, fp);
	//close(fd_client);
}




int main(int argc, char *argv[])
{
	struct sockaddr_in server_addr, client_addr;  // internet address
	socklen_t sin_len = sizeof(client_addr);  // size of address
	int fd_server, fd_client; 
	int on = 1;

	port_nr = strtol(argv[1], NULL, 10);
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
	server_addr.sin_port = htons(port_nr);

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
		printf("waiting for client\n");
		fd_client = accept(fd_server, (struct sockaddr *) &client_addr, &sin_len);
	 	ip_addr = inet_ntoa(client_addr.sin_addr);	
		if(fd_client < 0)
		{
			perror("Connection failed.......");
			continue;
		}
		char buff_post[10000];
		printf("Got client connection.......\n");
		read(fd_client, buff_post, 9999);
		for(int i = 0; i < 1; i++)
		{
			printf("%s\n", buff_post);
		}		
		//Log it
		///write_logfile(fd_client, ip_addr, port_nr);
		
		printf("Got client connection.......\n");	
	
		// Determine what to do with the request
		handle_http_request(fd_client); 	
		
		//Log it
		write_logfile();	
	
	} 
	
	return 0;
}
