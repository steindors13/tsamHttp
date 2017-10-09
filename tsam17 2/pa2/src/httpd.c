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
	char port_nr[6];
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
	fprintf(f, "<%s> ", this_client.port_nr);
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

void writeRespond(GString *gs, client_info this_client)
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
		strcat(temp, this_client.port_nr);
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
		strcat(temp, this_client.port_nr); 
		strcat(temp, html_tail);
		strcpy(toSend, temp);
		temp[0] = '\0';
	}
	
	write_logfile(this_client);
	
		
}

void send_data(int fd_client, GString *gs, client_info this_client)
{	
	writeRespond(gs, this_client);
	
	printf("sending %s\n", toSend);
	send(fd_client, toSend, strlen(toSend), 0);
	
}
 
void handle_http_request(int fd_client, GString *gs, client_info this_client)
{
	// Determine what kind of request it is
	printf("determining request type\n");
	GString *temp_gs;
	temp_gs = g_string_new("");
	g_string_assign(temp_gs, gs->str);
	this_client.url = '\0';
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
	c = strtok(NULL, " \r\n");
	this_client.url = c;
	c = NULL;
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
	
	send_data(fd_client, gs, this_client);
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
	char buffer[80]; // Buffer to recieve message
	int compress_array = FALSE;
	int close_connection = FALSE;	
	

	printf("Starting server, %d arguments\n", argc);
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

	// Set socket to be nonblocking so we can switch between differen connected sockets. 
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
		
		int recv_poll = poll(fds, nfds, timeout);
		//printf("nfds %d\n", nfds);	
		if(recv_poll < 0)
		{
			perror("Poll failed...");
			break;
		}

		if(recv_poll == 0)
		{
			perror("Poll timedout....");
			break; 	
		}
		current_size = nfds;
	
		int i;
		for(i = 0; i < current_size; i++)
		{
			// struct to store information about client in. this info is stored to be logged later and to send back to clients browser	
			client_info this_client;
					
			if(fds[i].revents == 0)
			{
				printf("revent = 0\n");
				continue;
			}

			if(fds[i].fd == fd_server)
			{
				// We have a readable file descriptor!	
				do
				{
					printf("waiting for connection....\n");
					fd_client = accept(fd_server, (struct sockaddr *) &client_addr, &sin_len);
	 				
					// Registering clients information
					this_client.ip_addr = inet_ntoa(client_addr.sin_addr);
					sprintf(this_client.port_nr, "%d", client_addr.sin_port);
						
					if(fd_client < 0)
					{
						//perror("Connection failed.......");
						break;
					}
					
					printf("New connection!\n");			
					fds[nfds].fd = fd_client;
					fds[nfds].events = POLLIN;
					nfds++;
						
							
				} while(fd_client != -1);

			}
			else
			{	
				int rc;
				unsigned int len;
				GString *gs;
				gs = g_string_new("");				
				while(1)
				{
					rc = recv(fds[i].fd, buffer, sizeof(buffer), 0);
					
					// check if everything is al right with recieving 					
					if(rc < 0)
					{
						if( errno != EWOULDBLOCK)
						{
							perror(" recv failed...");
							close_connection = TRUE;
						}
					}

					//if zero client closed the connection
					if(rc == 0)
					{
						printf("client terminated the connection\n");
						close_connection = TRUE;
						break;
					}
					
					len = rc;
					printf(" %d bytes received\n", len);
					
					// Is buffer ready to be sent ?
					if(len < sizeof(buffer))
					{
						//Null terminating the buffer.
						buffer[len] = '\0';
						g_string_append(gs, buffer);
						// Start manipulating the request
						handle_http_request(fds[i].fd, gs, this_client);	
						g_string_free(gs, 1);	
						close_connection = TRUE;
						break;
					}
					else
					{	// Then load it up!
						g_string_append(gs, buffer);
					}
					
				}

				if(close_connection)
				{
					close_connection = FALSE;
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
