#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<netdb.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<pthread.h>
#include<errno.h>
#include <ctype.h>

#define MAX_LINE 8192

char board[9];
char user_name[MAX_LINE];
int server_fd;

int open_clientfd();
void* recv_msg();
void send_msg();
void show_func();
void print_board();
void *server_connection(struct sockaddr_in *address);
int main(int argc, char **argv)
{
	char* server_addr;
	struct sockaddr_in address;
	pthread_t thread;
	long port = strtol(argv[2], NULL, 10);
	if(argc < 3){
		printf("Information missed: [IP] [PORT]\n");
		return -1;
	}
	
	// connection setting
	server_addr = argv[1];
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = inet_addr(server_addr);
	address.sin_port = htons(port);
	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	server_connection(&address);
	
	printf("Enter your name: ");
	scanf("%s", user_name);
	fflush(stdout);
	for(int i=0;i<9;i++)
		board[i] = '?';
	// create thread to receive message
	pthread_create(&thread, NULL, (void*)recv_msg,NULL);
	
	send_msg();
	// exit
	printf("closed\n");
	close(server_fd);
	pthread_exit(NULL);
	return 0;
}
void* recv_msg()
{
	char msg[MAX_LINE];
	int recv_length;
	char* j;
	int game_stat = 0;// 1 is gaming, 0 is wait
	while(1){
		for(int i=0;i<MAX_LINE;i++)
			msg[i] = '\0';
		fflush(stdout);
		recv_length = recv(server_fd, msg, MAX_LINE, 0);
		if(recv_length == -1){
			fprintf(stderr, "recv() failed: %s\n", strerror(errno));
			break;
		}
		else if(recv_length == 0){
			printf("\nPeer discoonected\n");
			break;
		}
		else{
			if(game_stat == 1 || strstr(msg,"<board>") != NULL || strstr(msg,"GAME") != NULL){
				game_stat = 1;
				if((j = strstr(msg,"<board>") ) != NULL){
					for(int i=0;i<9;i++)
						board[i] = *(j+i+7);
					print_board();
					printf("%s>",user_name);
					continue;
				}
				else if(strncmp(msg,"<GAME>",6) == 0){
					printf("%s", msg);
					printf("%s>",user_name);
				}
				// game terminated
				if(strstr(msg,"Win") != NULL || strstr(msg,"Lose") != NULL || strstr(msg,"Draw") != NULL){
					game_stat = 0;
					memset(board, -1, sizeof(board));
				}
			}
			else{		
					printf("%s", msg);
					if(strncmp(msg,"<Private chat>",14) != 0)
					printf("%s>",user_name);
			}
		}
	}
}
void send_msg()
{
	char message[MAX_LINE];
	memset(message, '\0', sizeof(message));
	sprintf(message, "%s", user_name);
	send(server_fd, user_name, MAX_LINE, 0);
	memset(message, '\0', sizeof(message));
	while(fgets(message, MAX_LINE, stdin) != NULL){
		printf("%s>",user_name);
		if((strncmp(message, "/quit", 5) ==0) || (strncmp(message, "/q", 2) == 0)){
			send(server_fd, message, MAX_LINE, 0);
			printf("Close connection...\n");
			exit(0);
		}
		else if(strncmp(message, "/help", 5) == 0 || strncmp(message, "/h" , 2) == 0){
			show_func();
			memset(message, '\0', sizeof(message));	// don't need to send
			continue;
		}
		send(server_fd, message, MAX_LINE, 0);
		memset(message, '\0', sizeof(message));	// reset after sending
  	}
}
void show_func()
{
	printf("\nFunction Intro:\n");
	printf("/q, /quit: quit");
	printf("/list : list online user\n");
	printf("/chat : chat to specific user\n");
	printf("/chess : play tic tac toe to specific user\n");
	printf("%s>",user_name);
}
void print_board()
{
	printf("\n");	
	printf("┌───┬───┬───┐        ┌───┬───┬───┐\n");   
	printf("│ 1 │ 2 │ 3 │        │ %c │ %c │ %c │\n", board[0], board[1], board[2]);   
	printf("├───┼───┼───┤        ├───┼───┼───┤\n");   
	printf("│ 4 │ 5 │ 6 │        │ %c │ %c │ %c │\n", board[3], board[4], board[5]);  
    printf("├───┼───┼───┤        ├───┼───┼───┤\n"); 
    printf("│ 7 │ 8 │ 9 │        │ %c │ %c │ %c │\n", board[6], board[7], board[8]);  
	printf("└───┴───┴───┘        └───┴───┴───┘\n\n");
}
void *server_connection(struct sockaddr_in *address)
{
	int response = connect(server_fd, (struct sockaddr *) address, sizeof *address);
	if(response < 0){
		fprintf(stderr, "connect() failed: %s\n", strerror(errno));
		exit(1);
	}else{
		printf("Server connected\n");
	}
}
