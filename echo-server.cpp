#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>

#define MSG_MAX 65536
#define SOCKET_MAX 1000
#define CLIENT_MAX 1000

int mode;
int new_socket;
int sockets[SOCKET_MAX];
int socket_cnt;
pthread_mutex_t mutex;

void usage() {
	printf("syntax : echo-server <port> [-e[-b]]\n");
	printf("sample : echo-server 1234 -e -b\n");
}

void *handler(void *pSocket) {
	int socket = *(int *)pSocket;
	char msg[MSG_MAX];
	char msg_len = 0;
	
	while((msg_len = recv(socket, msg, MSG_MAX-1, 0))!=0) {
		msg[msg_len]='\0';
		printf("%s", msg);
		
		if(mode == 3) {
			pthread_mutex_lock(&mutex);
			for(int i=0; i<socket_cnt; i++) {
				if(sockets[i]==socket) {
					send(socket, msg, msg_len, 0);
					break;
				}
			}
			pthread_mutex_unlock(&mutex);
		}
		else if(mode == 4) {
			pthread_mutex_lock(&mutex);
			for(int i=0; i<socket_cnt; i++) {
				send(sockets[i], msg, msg_len, 0);
			}
			pthread_mutex_unlock(&mutex);
		}
		
	}
	
	pthread_mutex_lock(&mutex);
	for(int i=0; i<socket_cnt; i++) {
		if(sockets[i]==socket) {
			socket_cnt--;
			while(i<socket_cnt) {
				sockets[i] = sockets[i+1];
				i++;
			}
			break;
		}
	}
	pthread_mutex_unlock(&mutex);
	
	close(socket);
	return NULL;
}

int main(int argc, char **argv) {
	if(argc < 2 || argc > 4 || argc >= 3 && strcmp(argv[2], "-e") != 0 || argc == 4 && strcmp(argv[3], "-b") != 0) {
		usage();
		return -1;
	}
	mode = argc;
	
	int port = atoi(argv[1]);
	if(port < 0 || port > 65535) {
		printf("Port num out of range\n");
		return -1;
	}
	
	struct sockaddr_in address;
	
	int opt = 1;
	int addrlen = sizeof(address);
	pthread_t t_id;
	pthread_mutex_init(&mutex, NULL);
	
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(port);
	
	int sock_server = socket(AF_INET, SOCK_STREAM, 0);
	if (sock_server < 0) {
		printf("Socket error\n"); 
		return -1;
	}

	if (bind(sock_server, (struct sockaddr *)&address, sizeof(address))<0) {
		printf("Bind error\n");
		return -1;
	}

	if (listen(sock_server, CLIENT_MAX) < 0) {
		printf("Listen error\n");
		return -1;
	}

	while(1) {
		if ((new_socket = accept(sock_server, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0) {
			printf("Accept error\n");
			return -1;
		}
		
		pthread_mutex_lock(&mutex);
		sockets[socket_cnt++] = new_socket;
		pthread_mutex_unlock(&mutex);
		
		pthread_create(&t_id, NULL, handler, (void *)&new_socket);
		pthread_detach(t_id);
		
		printf("CONNECT WITH %s\n", inet_ntoa(address.sin_addr));
	}
	
	pthread_mutex_destroy(&mutex);
	close(sock_server);
	return 0;
}
