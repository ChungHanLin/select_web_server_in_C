#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/select.h>
#include <time.h>

#define Port_Num 8099
#define Background 10

void write_protocol(char *http_header);
char *get_time(char *);

void socket_handler(int socket_fd, char *recieve, char *response, int length, fd_set *set){
	send(socket_fd, response, length, 0);
	recv(socket_fd, recieve, 1024, 0);
	close(socket_fd);
	FD_CLR(socket_fd, set);
}

int main(int argc, char *argv[]){
	
	//open html file
	int htmlFD = open("index.html", O_RDONLY);
	char htmlContent[4096];
	char recieve[1024];
	int length = read(htmlFD, htmlContent, 4096);
	
	htmlContent[length] = '\0';

	char response[2048];
	write_protocol(response);

	length += strlen(response);
	strcat(response, htmlContent);

	int network_socket, new_socket, maxsock;

	network_socket = socket(AF_INET, SOCK_STREAM, 0);

	if(network_socket == -1){
		perror("socket error");
	}

	struct sockaddr_in serverInfo, clientInfo;
	int client_addrlen = sizeof(clientInfo);
	fd_set socks, readsocks;

	serverInfo.sin_family = AF_INET;
	serverInfo.sin_addr.s_addr = INADDR_ANY;
	serverInfo.sin_port = htons(Port_Num);

	if(bind(network_socket, (struct sockaddr *) &serverInfo, sizeof(serverInfo)) == -1){
		perror("bind error");
	}

	if(listen(network_socket, Background) == -1){
		perror("listen error");
	}

	FD_ZERO(&socks);
	FD_SET(network_socket, &socks);
	maxsock = network_socket;

	while(1){
		unsigned int s;
		readsocks = socks;
		if(select(maxsock + 1, &readsocks, NULL, NULL, NULL) == -1){
			perror("select");
		}
		
		for(s = 0; s <= maxsock; s++){
			if(FD_ISSET(s, &readsocks)){
				printf("socket %d was ready\n", s);
				if(s == network_socket){
					// New Connection
					new_socket = accept(network_socket, (struct sockaddr *) &clientInfo, (unsigned int *) &client_addrlen);
					if(new_socket == -1){
						perror("client_socket error");
						exit(EXIT_FAILURE);
					}
					else{
						FD_SET(new_socket, &socks);
						if(new_socket > maxsock){
							maxsock = new_socket;
						}
					}
				}
				else{
					socket_handler((unsigned int)s, recieve, response, length, &socks);
				}
			}
		}
	}

	close(network_socket);
	printf("done\n");
	return 0;
}

char *get_time(char *c_time_string){
	time_t current_time;

	current_time = time(NULL);

	if (current_time == ((time_t)-1)){
        	fprintf(stderr, "Failure to obtain the current time.\n");
		exit(EXIT_FAILURE);    	
	}

   	/* Convert to local time format. */
    	c_time_string = ctime(&current_time);

    	if (c_time_string == NULL){
        	fprintf(stderr, "Failure to convert the current time.\n");
        	exit(EXIT_FAILURE);
    	}
	return c_time_string;
}

void write_protocol(char *http_header){
	char header[] = "HTTP/1.1 200 OK\r\n";
	char host[] = "Host:localhost\r\n";
	char port[] = "Port:8001\r\n";
	char time[512];
	char *time_Reg = get_time(time);

	strcat(http_header, header);
	strcat(http_header, host);
	//strcat(http_header, Port_Num);
	strcat(http_header, time_Reg);
	strcat(http_header, "\r\n\n");
}
