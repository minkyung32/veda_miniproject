#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>

#define TCP_PORT 5100
#define MAX_CLIENT 50

void handler(int signo){
	waitpid(0, NULL, WNOHANG);
}
void set_nonblocking(int sock) {
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);
}
void child_rnw(int csock, int pfd[MAX_CLIENT][2], int cli_count){
    int n;
    char buf[BUFSIZ];
    char mesg[BUFSIZ];
    char id[20];

    //id를 읽어와 사용자 접속을 알림
    if((n = read(csock, id, 20)) <= 0){
        if(n == 0)
            close(csock);
        else perror("id read");
    }
    int len = strlen(id);
    id[len - 1] = '\0';
    printf("%s is connected\n", id);
    while(1){
        //client 메세지 read
        if((n = read(csock, mesg, BUFSIZ)) <= 0){
            if(n==0){
                close(csock);
                break;
            }
            perror("mesg read");
        }
        if(strncmp(mesg, "...", 3) == 0){
            printf("%s is disconnected\n", id);
            close(csock);
            return;
        }              
        printf("[ %s ] : %s", id, mesg);

        close(pfd[cli_count][0]); // 읽기 끝 close
        write(pfd[cli_count][1], mesg, n);
        close(pfd[cli_count][1]); // 쓰기 끝 close
    }
}
int main(int argc, char **argv)
{
    int yes = 1;
	int ssock;
    socklen_t clen;
    struct sockaddr_in servaddr, cliaddr;
    int pfd[MAX_CLIENT][2];
    char line[BUFSIZ];
	int status;

	struct sigaction sigact;
	sigact.sa_handler = handler;
	sigfillset(&sigact.sa_mask);
	sigact.sa_flags = SA_RESTART;
	sigaction(SIGCHLD, &sigact, NULL);

    // 서버 소켓 생성
    if((ssock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket()");
        return -1;
    }

    // 주소 구조체에 주소 지정
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(TCP_PORT);

 	if(setsockopt(ssock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1){
		perror("setsockopt");
		exit(1);
	}
    set_nonblocking(ssock); //서버 소켓 non-block

 	//서버 소켓의 주소 설정
    if(bind(ssock, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind()");
        return -1;
    }

    // 동시에 접속하는 클라이언트의 처리를 위한 대기 큐를 설정
    if(listen(ssock, 8) < 0) {
        perror("listen()");
        return -1;
    }

    clen = sizeof(cliaddr);
    int cli_count = 0;
    int cs_list[MAX_CLIENT];
    while(1) {
		// 클라이언트가 접속하면 접속을 허용하고 클라이언트 소켓 생성
        int n, csock = accept(ssock, (struct sockaddr *)&cliaddr, &clen);
		if (csock < 0) {
                if (errno == EWOULDBLOCK || errno == EAGAIN) {
                    ssize_t m;
                    for (int i = 0; i <= cli_count; i++) { //client의 수
                        if ((m = read(pfd[i][0], line, sizeof(line))) > 0) { //자식 프로세스에서 쓴 데이터를 읽어옴
                            for (int j = 0; j <= cli_count; j++) {
                                if (j != i) {
                                    printf("write to all other clients\n");
                                    char tmp[BUFSIZ] = "[ from parent ] : ";
                                    strcat(tmp, line);
                                    //if(j != cli_count) : 보낸 클라이언트가 아닐 때만 write
                                    write(cs_list[i], tmp, m); //클라이언트에게 메세지를 보냄
                                }
                            }
                        } else if (m == 0) {
                            // Pipe closed
                            close(pfd[i][0]);
                        } else {
                            perror("pipe read");
                        }
                        usleep(100000);
                    }
                    continue;
                }
            else {
                perror("accept() failed");
                continue;
            }
        }
        else{
            if(pipe(pfd[cli_count]) < 0){
            perror("pipe");
            return -1;
            }
            pid_t cpid;
            cpid = fork();
            cs_list[cli_count] = csock;
            set_nonblocking(csock);
            if(cpid == 0){ //child process
                child_rnw(csock, pfd, cli_count);
                exit(0);
            }
            else if(cpid > 0){ //parent process
                close(pfd[cli_count][1]); // 부모 읽기 끝 닫음
                cli_count++;
                fcntl(pfd[cli_count][0], F_SETFL, O_NONBLOCK);
            }
            else {
                perror("fork()");
                close(csock);
                close(pfd[cli_count][0]);
                close(pfd[cli_count][1]);
                continue;
            }
        }
	}
    close(ssock);
    return 0;
}
