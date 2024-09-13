#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define TCP_PORT 5100

int main(int argc, char** argv){
	
	int ssock;
	struct sockaddr_in servaddr;
	char mesg[BUFSIZ];
	char data[BUFSIZ];
	char id[20];

	if(argc < 2){
		printf("Usage : %s IP_ADRESS\n", argv[0]);
		return -1;
	}
	if((ssock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		perror("socket()");
		return -1;
	}
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;

	inet_pton(AF_INET, argv[1], &(servaddr.sin_addr.s_addr));
	servaddr.sin_port = htons(TCP_PORT);

	if(connect(ssock, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0){
		perror("connect()");
		return -1;
	}
	
	printf("ID >> ");
	fgets(id, 20, stdin);
	if(send(ssock, id, 20, MSG_DONTWAIT) <= 0){
		perror("send()");
		return -1;
	}
	while(1){
		pid_t cpid;
		cpid = fork();
		if(cpid == 0){ //child process
			printf("me >> ");
			fgets(mesg, BUFSIZ, stdin);
			if(send(ssock, mesg, BUFSIZ, MSG_DONTWAIT) <= 0){
				perror("send()");
				return -1;
			}
			if(strncmp(mesg, "...", 3) == 0){
				break;
			}
			exit(0);
		}
		else if(cpid > 0){ //parent process
			memset(data, 0, BUFSIZ);
			int c = read(ssock, data, BUFSIZ);
			if(c <= 0){
				if(c == 0)
					continue;
				perror("read()");
				return -1;
			}
			data[strlen(data)-1] = '\0';
			printf("%s\n", data);
		}
		else {
			perror("fork()");
		}
	}
	close(ssock);
	return 0;
}

