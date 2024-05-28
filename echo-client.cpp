#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>

#define MSG_MAX 65536
#define SOCKET_MAX 1000

void usage();
void *receiver(void *pSocket);

int new_socket;
int sockets[SOCKET_MAX];
int socket_cnt;
pthread_mutex_t mutex;

int main(int argc, char **argv) {
	if(argc != 3) {
		usage();
		return -1;
	}
	
	int port = atoi(argv[2]);
	if(port < 0 || port > 65535) {
		printf("Port num out of range\n");
		return -1;
	}
	
	struct sockaddr_in address;
	char msg[MSG_MAX];
	int msg_len = 0;
	int ip_addr = 0;
	int client_fd;
	int opt = 1;
	int addrlen = sizeof(address);
	pthread_t t_id;
	pthread_mutex_init(&mutex, NULL);
	
	char *temp = argv[1];
	temp[strlen(temp)] = '.';
	temp[strlen(temp)+1] = '\0';
	
	for(int i=0; i<strlen(argv[1]); i++) {
		if(argv[1][i] == '.') {
			ip_addr = ip_addr << 8;
			ip_addr += atoi(temp);
			temp = argv[1] + i + 1;
		}
	}
	
	if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
		printf("Socket error\n"); 
		return -1;
	}

	// Attaching socket to the port
	if (setsockopt(client_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
		printf("Setsockopt error\n"); 
		return -1;
	}
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl(ip_addr);
	address.sin_port = htons(port);
	
	if(connect(client_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
		printf("Connect error\n");
		return -1;
	}
	printf("Connect with server\n");
	
	pthread_create(&t_id, NULL, receiver, (void *)&client_fd);
	pthread_detach(t_id);
	
	while(1) {
		fgets(msg, MSG_MAX, stdin);
		fflush(stdin);
		msg_len = strlen(msg);
		msg[msg_len-1] = '\r';
		msg[msg_len] = '\n';
		msg[msg_len+1] = '\0';
		if(send(client_fd, msg, strlen(msg), 0) <= 0) {
			printf("Send error\n");
			break;
		}
	}
	
	close(client_fd);
	return 0;
}

void usage() {
	printf("syntax : echo-client <ip> <port>\n");
	printf("sample : echo-client 192.168.10.2 1234\n");
}

void *receiver(void *pSocket) {
	int socket = *(int *)pSocket;
	char msg[MSG_MAX];
	int msg_len = 0;
	
	while((msg_len = recv(socket, msg, MSG_MAX-1, 0))!=0) {
		msg[msg_len]='\0';
		printf("%s", msg);	
	}
	
	printf("Disconnect\n");
	exit(0);
	return NULL;
}
