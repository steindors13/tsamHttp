#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/poll.h>
#include <fcntl.h>
#include <glib.h>
#include <glib/gprintf.h>
#include <time.h>
#include <errno.h>

#define GET 1
#define POST 2
#define HEAD 3
#define UNKNOWN 4

typedef struct 
{
	char *ip_addr;
	char *url;
	int port_nr;
	int request;
} client_info;

char ok_header_template[] =
"HTTP/1.1 200 OK\r\n"
"Content-type: text/html; charset=UTF -8\r\n\r\n";
char accepted_header_template[] =
"HTTP/1.1 202 Accepted\r\n"
"Content-type: text/html; charset=UTF -8\r\n\r\n";
char bad_header_template[] =
"HTTP/1.1 400 Bad Request\r\n"
"Content-type: text/html; charset=UTF -8\r\n\r\n";
char html[] =
"<!DOCTYPE html>\r\n"
"<html>\r\n"
"<body><h1>";
char html_tail[] = 
"</h1><br>\r\n"
"</body></html>\r\n";

char toSend[600];

void write_logfile(client_info this_client)
{
	FILE *f;
	
	f = fopen("httpd.log", "a+");
	
	time_t timer;
        char time_buff[26];
        struct tm* time_info;

        time(&timer);
        time_info = localtime(&timer);

        strftime(time_buff, 26, "%Y-%m-%d %H:%M:%S", time_info);
        puts(time_buff);
        fprintf(f, "<%s> ", time_buff);
	fprintf(f, "<%s> ", this_client.ip_addr);
	char str_port[10];
	sprintf(str_port, "%d", this_client.port_nr);


	fprintf(f, "<%s> ", str_port);
	fprintf(f, "<%s> ", this_client.url);
	if(this_client.request == GET)
	{
		fprintf(f, "<GET REQUEST> ");
	}
	else if(this_client.request == POST)
	{
		fprintf(f, "<POST REQUEST> ");
	}
	else if(this_client.request == HEAD)
	{
		fprintf(f, "<HEAD REQUEST> ");
	}
	else
	{
		fprintf(f, "<UNKNOWN REQUEST> ");
	}
	fprintf(f, "\n");
	fclose(f);
}

char* writeRespond(GString *gs, client_info this_client)
{
	memset(toSend, 0, strlen(toSend));
	printf("writing to respond\n");
	
	if(this_client.request == UNKNOWN)
	{
		strcpy(toSend, bad_header_template);
	}
	else if(this_client.request == HEAD)
	{
		strcpy(toSend, ok_header_template);
	}
	else if(this_client.request == POST)
	{
		char temp[600];
		strcpy(temp, accepted_header_template);
		strcat(temp, html);
		strcat(temp, "http://127.0.0.1");
		strcat(temp, this_client.url);
		strcat(temp, " ");
		strcat(temp, this_client.ip_addr);
		strcat(temp, ":");
		char str_port[10];
		sprintf(str_port, "%d", this_client.port_nr);
		strcat(temp, str_port);
		//Isolate the Body of request	
		char *token;
		token = strstr(gs->str, "\r\n\r\n");
		printf("token = %s\n", token);
		strcat(temp, token);
		strcat(temp, html_tail);
		strcpy(toSend, temp);
	}	
	else
	{
		char temp[600];
		strcpy(temp, ok_header_template);
		strcat(temp, html);
		strcat(temp, "http://127.0.0.1");
		strcat(temp, this_client.url);
		strcat(temp, " ");
		strcat(temp, this_client.ip_addr);
		strcat(temp, ":");
		char str_port[10];
		sprintf(str_port, "%d", this_client.port_nr);

		strcat(temp, str_port); 
		strcat(temp, html_tail);
		strcpy(toSend, temp);
		temp[0] = '\0';
	}
	write_logfile(this_client);
	
	return toSend;	
}

void handle_status_request(int fd_client, GString *gs, client_info this_client)
{	
	int fd_server;
	fd_server = (intptr_t) writeRespond(gs, this_client);
	//printf("toSend: %s\n", toSend);
	if(fd_server < 0)
	{
		printf("can't find webpage to send");
		exit(0);
	}
	
	int nread;
	while ( (nread = read(fd_server,toSend, sizeof(toSend) )) > 0)
	{
		write(fd_client, toSend, nread);
	}
	printf("sending %s\n", toSend);
	send(fd_client, toSend, strlen(toSend), 0);
	
}
/*
void set_keepalive(FILE *f, int fd_server)
{
	int on = 1;
        socklen_t optlen = sizeof(on);
	char post_buffer[5000];
        fseek(f, 0, SEEK_END);
        long length = ftell(f);
        fseek(f, 0, SEEK_SET);
        fread(post_buffer, 1, length, f);
        fclose(f);

        char *c;
        c = strstr(post_buffer, "HTTP");
        c = strtok(c, "\r\n");
        if(strcmp(c, "HTTP/1.1") == 0)
        {
                if(setsockopt(fd_server, SOL_SOCKET, SO_KEEPALIVE, &on, optlen) < 0)
                {
                	perror("TCP keep alive error");
                        close(fd_server);
                        exit(1);
                }
                printf("SO_KEEPALIVE is %s\n", (on ? "ON" : "OFF"));
        }
}
*/
 
void handle_http_request(int fd_client, GString *gs, client_info this_client)
{
	// Determine what kind of request it is
	printf("determining request type\n");
	GString *temp_gs;
	temp_gs = g_string_new("");
	g_string_assign(temp_gs, gs->str);

	//Get, Post or even Head ?i
	char *c = NULL;
	c = strtok(temp_gs->str, " \r\n");
	printf("%s requested \n", c);
	
	if(strcmp(c, "GET") == 0)
	{
		this_client.request = GET;
	}
	else if(strcmp(c, "POST") == 0)
	{
		this_client.request = POST;
	}
	else if(strcmp(c, "HEAD") == 0)
	{
		this_client.request = HEAD;
	}
	else
	{
		this_client.request = UNKNOWN;
	}
		
	//requested url
	
	this_client.url = strtok(NULL, " \r\n");
	if(!this_client.url)
	{
		printf("NO URL\n");
	} 
	if(strcmp(this_client.url, "/favicon.ico") == 0)
	{
		this_client.request = UNKNOWN;
	}

	g_string_free(temp_gs, 1);
	printf("url is: %s\n", this_client.url);
	
	handle_status_request(fd_client, gs, this_client);
}




int main(int argc, char *argv[])
{
	struct sockaddr_in server_addr, client_addr;  // internet address
	socklen_t sin_len = sizeof(client_addr);  // size of address
	int fd_server, fd_client = 0, timeout;
	int on = 1;
	socklen_t optlen = sizeof(on);
	struct pollfd fds[200]; //file descriptors line
	int nfds = 1; // number of file descriptors
	int port_nr = strtol(argv[1], NULL, 10);
	int close_conn;
	printf("Starting server, %d arguments\n", argc);
	char buffer[80];
	int compress_array = FALSE;	
	// create and bind a TCP socket
	fd_server = socket(AF_INET, SOCK_STREAM, 0); // returns positive if success
	
	if(fd_server < 0)
	{
		perror("socket error");
		close(fd_server);
		exit(-1);
	}
	
	// Allow socket to be reuseable
	if(setsockopt(fd_server, SOL_SOCKET, SO_REUSEADDR, &on, optlen) < 0)
	{
		perror("setting socket option to be reuseable failed");
		close(fd_server);
		exit(-1);
	}

	// Set socket to be nonblocking.
	if(ioctl(fd_server, FIONBIO, &on, optlen) < 0)
	{
		perror("setting socket to be nonblocking failed");
		close(fd_server);
		exit(-1);
	}
		
	// Bind to socket
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(port_nr);
	
	if(bind(fd_server, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0)
	{
		perror("binding Failed");
		close(fd_server);
		exit(-1);
	}
	
	// Listen for a connection
	if(listen(fd_server, 10) < 0)
	{
		perror("listen failed");
		close(fd_server);
		exit(-1);
	}
	// Initialize the pollfd
	memset(fds, 0, sizeof(fds));
	fds[0].fd = fd_server;
	fds[0].events = POLLIN;
	
	// set timeout to 30sec
	timeout = 30000;
	int current_size = 0;
	printf("starting.....\n");
	while(1)
	{
	
		// Wait for incoming connected sockets.
		//printf("POLL...\n");
		int recv_poll = poll(fds, nfds, timeout);
		//printf("nfds %d\n", nfds);	
		if(recv_poll < 0)
		{
			perror("..Poll failed");
			break;
		}
		if(recv_poll == 0)
		{
			perror("..Poll timedout");
			break; 	
		}
		current_size = nfds;
		
		client_info this_client;
		int i;
		for(i = 0; i < current_size; i++)
		{
			//printf("i: %d\n", i);	
			if(fds[i].revents == 0)
			{
				printf("revent = 0\n");
				continue;
			}
			if(fds[i].fd == fd_server)
			{
				
				do
				{
					printf("waiting for client\n");
					fd_client = accept(fd_server, (struct sockaddr *) &client_addr, &sin_len);
	 				
					// Registering clients information
					this_client.ip_addr = inet_ntoa(client_addr.sin_addr);
					this_client.port_nr = client_addr.sin_port;
						
					if(fd_client < 0)
					{
						//perror("Connection failed.......");
						break;
					}
					printf("New connection!\n");			
					fds[nfds].fd = fd_client;
					fds[nfds].events = POLLIN;
					nfds++;
					printf("Client Connected.....\n");	
							
				} while(fd_client != -1);

			}
			else
			{	
				int rc;
				unsigned int len;
				close_conn = FALSE;
				GString *gs;
				gs = g_string_new("");				
				while(1)
				{
					rc = recv(fds[i].fd, buffer, sizeof(buffer), 0);
					if(rc < 0)
					{
						if( errno != EWOULDBLOCK)
						{
							perror(" recv failed...");
							close_conn = TRUE;
						}
						break;
					}
					
					if(rc == 0)
					{
						printf(" closing connection\n");
						close_conn = TRUE;
						break;
					}
					len = rc;
					printf(" %d bytes received\n", len);
					
					//printf("string %s\n", gs->str);
					if(len < sizeof(buffer))
					{
						buffer[len] = '\0';
						g_string_append(gs, buffer);
						printf("sending data\n");
						handle_http_request(fds[i].fd, gs, this_client);
						close_conn = TRUE;
						g_string_free(gs, 1);	
						break;
					}
					else
					{
						g_string_append(gs, buffer);
					}
					
				}
				
				if(close_conn)
				{
					printf("closing connections\n");
					compress_array = TRUE;
					close(fds[i].fd);
					fds[i].fd = -1;
				}				
			}			
		}
		
		if(compress_array)
		{
			printf("compressing.....\n");
			compress_array = FALSE;
			int j, k;
			for(j = 0; j < nfds; j++)
			{
				if(fds[j].fd == -1)
				{
					for(k = j; k < nfds; k++)
					{
						fds[k].fd = fds[k+1].fd;
					}
					
					nfds--;
					
				}
				
			}
			printf("done compressing\n");	
		}
	}
	// Cleaning up open sockets
	int i;
	for(i = 0; i < nfds; i++)
	{
		if(fds[i].fd >= 0)
			close(fds[i].fd);
	}
	printf("Exiting...\n");
	
}
