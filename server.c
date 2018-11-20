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
#include "socket.h"

#define PORT_NUM 8000
#define BUFFER_SIZE 4096
#define BACKLOG_NUM 10

void write_Protocol(char *http_header);
char *get_time(char *);
int get_Request(char *response);

void bind_Server(int, struct sockaddr_in *);
void listen_Server(int);

void init_Selector(int, fd_set *);
void select_Client(int, fd_set *);
int accept_Client(int, struct sockaddr_in *);
void socket_Handler(int, char *, int);
void add_SocketFD(int, fd_set *, int *, int *);
void clear_SocketFD(int, fd_set *, int *, int *);

int main(int argc, char *argv[]){
	int serverSocket, clientSocket;
	int minSocket, maxSocket;
	int replyLength = 0;
	char response[BUFFER_SIZE];
	struct sockaddr_in serverInfo, clientInfo;

	fd_set initFDSet, readFDSet;

	serverSocket = getSocketFD();

	replyLength = get_Request(response);

	init_Socket(&serverInfo, NULL);

	bind_Server(serverSocket, &serverInfo);

	listen_Server(serverSocket);

	init_Selector(serverSocket, &initFDSet);

	minSocket = maxSocket = serverSocket;

	while(1){
		int s;

		readFDSet = initFDSet;

		select_Client(maxSocket, &readFDSet);

		for(s = minSocket; s <= maxSocket; s++){

			if(FD_ISSET(s, &readFDSet)){
				if(s == serverSocket){
					clientSocket = accept_Client(serverSocket, &clientInfo);
					add_SocketFD(clientSocket, &initFDSet, &minSocket, &maxSocket);
				}
				else{
					socket_Handler(s, response, replyLength);
					clear_SocketFD(s, &initFDSet, &minSocket, &maxSocket);
					close(s);
				}
			}
		}
	}

	close(serverSocket);
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

void write_Protocol(char *http_header){
	char header[] = "HTTP/1.1 200 OK\r\n";
	char host[] = "Host:localhost\r\n";
	char port[] = "Port:";
	char time[512];
	char *time_Reg = get_time(time);

	sprintf(http_header, "%s%s%s%d\r\n%s\r\n\n", header, host, port, PORT_NUM, time_Reg);
}

int get_Request(char *response){
	int htmlFD = open("index.html", O_RDONLY);
	char htmlContent[BUFFER_SIZE];
	int length = read(htmlFD, htmlContent, BUFFER_SIZE);

	htmlContent[length] = '\0';
	write_Protocol(response);

	length += strlen(response);
	strcat(response, htmlContent);

	return length;
}

void bind_Server(int socketFD, struct sockaddr_in *serverInfo){

	if(bind(socketFD, (struct sockaddr *) serverInfo, sizeof(struct sockaddr_in)) == -1){
		perror("bind");
		close(socketFD);
		exit(EXIT_FAILURE);
	}
}

void listen_Server(int socketFD){

	if(listen(socketFD, BACKLOG_NUM) == -1){
		perror("listen");
		close(socketFD);
		exit(EXIT_FAILURE);
	}
}


void init_Selector(int socketFD, fd_set *initFDSet){

	FD_ZERO(initFDSet);
	FD_SET(socketFD, initFDSet);
}

void select_Client(int socketFD, fd_set *readFDSet){

	if(select(socketFD + 1, readFDSet, NULL, NULL, NULL) == -1){
		perror("select");
		exit(EXIT_FAILURE);
	}
}

int accept_Client(int serverSocket, struct sockaddr_in *clientInfo){
	socklen_t length = sizeof(struct sockaddr_in);

	int clientSocket = accept(serverSocket, (struct sockaddr *)clientInfo, &length);

	if(clientSocket == -1){
		perror("accept");
		exit(EXIT_FAILURE);
	}
	return clientSocket;
}

void socket_Handler(int socketFD, char *response, int messageLength){

	char recieve[BUFFER_SIZE];

	recv(socketFD, recieve, BUFFER_SIZE, 0);
	send(socketFD, response, messageLength, 0);

	return;
}

void add_SocketFD(int index, fd_set *initFDSet, int *minSocket, int *maxSocket){

	FD_SET(index, initFDSet);
	
	if(index > *maxSocket){
		*maxSocket = index;
	}
	else if(index < *minSocket){
		*minSocket = index;
	}

}

void clear_SocketFD(int index, fd_set *initFDSet, int *minSocket, int *maxSocket){
	int replaceIndex = index;

	FD_CLR(index, initFDSet);

	if(index == *maxSocket){
		while((replaceIndex > *minSocket) && (!FD_ISSET(replaceIndex, initFDSet))){
			replaceIndex--;
		}
		*maxSocket = replaceIndex;
	}
	else if(index == *minSocket){
		while((replaceIndex < *maxSocket) && (!FD_ISSET(replaceIndex, initFDSet))){
			replaceIndex++;
		}
		*minSocket = replaceIndex;
	}
}
