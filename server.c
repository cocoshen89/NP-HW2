#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<time.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<string.h>
#include<pthread.h>

#define MAX_LINE 8192
#define MAX_People 10
#define NAME_LIEN 20
#define SERV_PORT 8080

int connect_fd[MAX_People];
char user[MAX_People][NAME_LIEN];
int game_state[MAX_People];

int open_listenfd();
void init_connectFd_user();
void server_control(int listen_fd);
void show_setting();
void server_receive_send(int n);
void game(int p1,int p2);
int check_win(char board[9],char piece);
int main()
{
	int listen_fd,i;
	pthread_t serv_ctrl;
	struct sockaddr_in client_addr;
	socklen_t clientlen;
	
	// create server socket and connect
	listen_fd = open_listenfd();
	// create thread to control server
	pthread_create(&serv_ctrl, NULL, (void*)(&server_control), (void*)listen_fd );
	// init connect_fd and user
	init_connectFd_user();
	// wait for client
	fprintf(stderr,"\nWaiting Client...\n");
	while(1){
		clientlen = sizeof(client_addr);
		for(i = 0;i< MAX_People && connect_fd[i] != -1; i++);	// look for free connect_fd
		// accept client connect
		connect_fd[i] = accept(listen_fd, (struct sockaddr*)&client_addr, &clientlen);
		
		// create threads for client to control connect_fd
		printf("\nWaiting for user name...\n");
		pthread_create(malloc(sizeof(pthread_t)), NULL, (void*)(&server_receive_send), (void *)i);
	}
	return 0;
}
void init_connectFd_user()
{
	for(int i=0;i<MAX_People;i++)
	{
		connect_fd[i] = -1;
		strcmp(user[i],"\0");
		game_state[i] = -1;
	}
	fprintf(stderr,"Initial connect_fd and user done...\n");
}
int open_listenfd() 
{
	int listenfd;
	static struct sockaddr_in serv_addr;
	// create server socket 
	if ((listenfd=socket(AF_INET, SOCK_STREAM,0))<0){
		fprintf(stderr,"Socket create Error!!!\n");
		exit(1);
	}
	// Internet connection setting
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(8080);
	// notify by bind
	if (bind(listenfd,(struct sockaddr*)&serv_addr,sizeof(serv_addr))<0)
	{
		fprintf(stderr,"Bind Error!!!\n");
		exit(2);
	}
	// listen
	fprintf(stderr,"Server Listening...\n");
	if (listen(listenfd,64)<0)
	{
		fprintf(stderr,"Listen Error!!!\n");
		exit(2);
	}
	return listenfd;
}
void server_control(int listen_fd){
	char msg[10];

	while(1){
		scanf("%s", msg);
		if(strncmp(msg, "/quit", 5) == 0 || strncmp(msg, "/q" , 2) == 0){
			printf("Server closed\n");
			close(listen_fd);
			exit(0);
		}
		else if(strncmp(msg, "/help" , 5) == 0 || strncmp(msg, "/h" , 2) == 0)
			show_setting();
		else
		{
			fprintf(stderr,"Unknown common...\n");
			fprintf(stderr,"use /h, /help to see server common\n\n");
		}
	}
}
void show_setting()
{
	fprintf(stderr,"MAX number of user is %d\n", MAX_People);
	fprintf(stderr,"Maximum Length of user name is %d\n", NAME_LIEN);
	fprintf(stderr,"Server port is set to be: %d", SERV_PORT);
	fprintf(stderr,"Maximum message long is %d\n\n", MAX_LINE);

	fprintf(stderr,"Function Intro:\n");
	fprintf(stderr,"/q, /quit :quit the server\n");
	fprintf(stderr,"/h, /help :show server setting and function Intro\n\n");
}
void server_receive_send(int n)
{
	char msg_notify[MAX_LINE];
	char msg_rcv[MAX_LINE];
	char msg_send[MAX_LINE];
	char target_user[NAME_LIEN];
	char user_name[NAME_LIEN];
	char message[MAX_LINE];
	

	char serv_msg1[]="";
	char serv_msg2[]=" <SERVER> Who do you want to chat? \0";
	char serv_msg3[]=" <SERVER> Which Player do you want to play with: \0";
	char serv_msg4[]=" <SERVER> Enter Y or y to accept the game with\0";
	char serv_msg5[]=" <SERVER> Target Player rejected\n\0";
	char serv_msg6[]=" <SERVER> Target Player accept, Game Start\n\0";

	char game_msg[]="<GAME> Game Start!!!\n";
	
	int i = 0;
	int target_idx;
	int retval;
	int recv_length;
	int check;
	memset(user_name, '\0', sizeof(user_name));
	
	// receiving user name from client
	recv_length = recv(connect_fd[n], user_name, NAME_LIEN, 0);
	if(recv_length > 0){	// receiving succeeded
		user_name[recv_length-1] = '\0';
		strcpy(user[n], user_name);
		fprintf(stderr,"Cilent:%d Create User name: %s\n",connect_fd[n],user[n]);
	}
	// receiving message from client
	while(1){
		memset(msg_rcv, '\0', sizeof(msg_rcv));
		memset(msg_send, '\0', sizeof(msg_send));
		memset(message,'\0',sizeof(message));
		target_idx = -1;

		recv_length = recv(connect_fd[n], msg_rcv, MAX_LINE+MAX_LINE, 0);
		if(recv_length > 0){	//Message received
			msg_rcv[recv_length] = 0;
			// quit
			if(strncmp(msg_rcv, "/quit", 5) == 0 || strncmp(msg_rcv, "/q" , 2) == 0){
				fprintf(stderr,"%s quitted\n", user_name);
				close(connect_fd[n]);
				connect_fd[n] = -1;
				pthread_exit(&retval);
			}
			// list client name
			else if(strncmp(msg_rcv, "/list", 5) == 0){
				strcpy(msg_send, "\n <SERVER> Online gamer:\n");
				for(i = 0; i < MAX_People; i++){
					if(connect_fd[i] != -1){
						strcat(msg_send, user[i]);
						strcat(msg_send, "\n");
					}
				}
				send(connect_fd[n], msg_send, MAX_LINE, 0);
			}
			// talk to specific user
			else if(strncmp(msg_rcv, "/chat", 5) == 0){
				fprintf(stderr,"\nPrivate message from %s ...\n", user_name);
				// ask for target user's name
				send(connect_fd[n], serv_msg2, MAX_LINE, 0);
				recv_length = recv(connect_fd[n], target_user, NAME_LIEN, 0);
				// check target user is Online ?
				check = 0;
				for(i=0;i<MAX_People;i++)
				{
					if(connect_fd[i] != -1 && strncmp(target_user, user[i], strlen(user[i])) == 0){
						check = 1;
						break;
					}
				}
				if(check != 1)
				{
					sprintf(msg_send, " <SERVER> This user %s not Online...\n", target_user);
					send(connect_fd[n], msg_send, MAX_LINE, 0);
					continue;
				}
				target_user[recv_length - 1] = '\0';
				sprintf(msg_send, "<Private chat>%s : ", user[i]);
				send(connect_fd[n], msg_send, MAX_LINE, 0);

				// get the private message
				memset(message,'\0',sizeof(message));
				recv_length = recv(connect_fd[n], message, MAX_LINE, 0);
				message[recv_length] = '\0';
				sprintf(msg_send, "<Private chat> %s : %s", user_name, message);
				//send private message
				send(connect_fd[i], msg_send, MAX_LINE, 0);
			}
			// tic tac toe
			else if(strncmp(msg_rcv, "/chess", 6) == 0){
				
				fprintf(stderr,"\n %s create a game with ", user_name);
				// ask for target user's name
				send(connect_fd[n], serv_msg3, MAX_LINE, 0);
				recv_length = recv(connect_fd[n], target_user, MAX_LINE, 0);
				target_user[recv_length - 1] = '\0';
				fprintf(stderr,"%s...\n", target_user);
					
				// wait for target user acception
				
				
				for(i = 0; i < MAX_People; i++){
					if(connect_fd[i] != -1 && strncmp(target_user, user[i], strlen(user[i])) == 0){
						// check game state
						if(game_state[n] == -1 && game_state[i] == -1){
							fprintf(stderr,"Target player founding...\n");
							memset(msg_send,'\0',sizeof(msg_send));
							sprintf(msg_send, " <SERVER> Enter Y or y to accept the game with %s?\n", user[n]);
							send(connect_fd[i], msg_send, MAX_LINE, 0);
							target_idx = i;
							game_state[n] = target_idx;
							break;
						}
						else{
							fprintf(stderr,"Player is playing now...\n");
							sprintf(msg_send, "Player is playing now...\n");
							send(connect_fd[n], msg_send, MAX_LINE, 0);
							break;
						}
					}
				}
				if(game_state[n] == -1){
					fprintf(stderr,"Game failed\n");
					continue;
				}
				// reply from target user
				recv_length = recv(connect_fd[target_idx], message, MAX_LINE, 0);	
				message[recv_length] = '\0';

				// reply response to user
				if(strncmp(message, "y", 1) != 0 && strncmp(message, "Y", 1) != 0){
					fprintf(stderr,"Player %s reject...\n", target_user);
					game_state[n] = -1;
					send(connect_fd[n], serv_msg5, MAX_LINE, 0);
					continue;
				}
				fprintf(stderr,"GAME Game start ... %s vs %s\n", user_name, target_user);
				send(connect_fd[n], game_msg, MAX_LINE, 0);
				send(connect_fd[target_idx], game_msg, MAX_LINE, 0);
				game_state[n] = target_idx;
				game_state[target_idx] = n;
				game(n, target_idx);
				fprintf(stderr,"%s vs %s Game terminated ...\n", user_name, target_user);
			}
		}
	}
}
void game(int p1,int p2)
{
	int i,recv_length,move;
	char OX[MAX_People];
	char board[9];
	char msg_send[MAX_LINE];
	char msg_rcv[MAX_LINE];
	char msg_game0[]="<GAME> Wait for another player...\n";
	char msg_game1[]="<GAME> It's your turn. Your piece is Please choose number.\n";
	char msg_game2[]="<GAME> You Lose.\n";
	char msg_game3[]="<GAME> You Win.\n";
	char msg_game4[]="<GAME> Draw\n";
	char msg_game5[]="<GAME> Invalid number!! Please enter number again...\n";
	for(int i=0;i<9;i++)
		board[i] = '?';
	memset(msg_rcv, '\0', sizeof(msg_rcv));
	memset(msg_send, '\0', sizeof(msg_send));
	OX[p1] = 'O';
	OX[p2] = 'X';
	memset(msg_send, '\0', sizeof(msg_send));
	sprintf(msg_send, "<board>         \n");
	send(connect_fd[p1], msg_send, MAX_LINE, 0);
	send(connect_fd[p2], msg_send, MAX_LINE, 0);
	for(i=0;i<9;i++)
	{		
		//send game message
		memset(msg_send, '\0', sizeof(msg_send));
		send(connect_fd[p1], msg_send, MAX_LINE, 0);
		send(connect_fd[p2], msg_send, MAX_LINE, 0);
		sprintf(msg_send, "<GAME> It's your turn. Your piece is %c ,Please choose number.\n",OX[p1]);
		send(connect_fd[p1], msg_send, MAX_LINE, 0);
		send(connect_fd[p2], msg_game0, MAX_LINE, 0);
		
		//recive p1 choose
		recv_length = recv(connect_fd[p1], msg_rcv, MAX_LINE, 0);
		move = (int)(msg_rcv[0]-'0')-1;
		board[move] = OX[p1];
		
		//send board
			memset(msg_send, '\0', sizeof(msg_send));
			sprintf(msg_send, "<board>%s\n",board);
			send(connect_fd[p1], msg_send, MAX_LINE, 0);
			send(connect_fd[p2], msg_send, MAX_LINE, 0);
		
		//check winer
		if(check_win(board,OX[p1]) == 1)
		{
			send(connect_fd[p1], msg_game3, strlen(msg_game3), 0);
			send(connect_fd[p2], msg_game2, strlen(msg_game2), 0);
			break;
		}
		//Draw
		if(i == 8){
			send(connect_fd[p1], msg_game4, strlen(msg_game4), 0);
			send(connect_fd[p2], msg_game4, strlen(msg_game4), 0);
			break;
		}
		//change turn
		int tmp = p1;
		p1 = p2;
		p2 = tmp;
	}
	game_state[p1] = -1;
	game_state[p2] = -1;
	fprintf(stderr,"%s v.s %s game terminated...\n",user[p1],user[p2]);
}
int check_win(char board[9],char piece)
{
	if(board[0] == piece && board[1] == piece && board[2] == piece) //first row
		return 1;
	else if(board[3] == piece && board[4] == piece && board[5] == piece) //second row
		return 1;
	else if(board[6] == piece && board[7] == piece && board[8] == piece) //third row
		return 1;
	else if(board[0] == piece && board[3] == piece && board[6] == piece) // first col
		return 1;
	else if(board[1] == piece && board[4] == piece && board[7] == piece) // second col
		return 1;
	else if(board[2] == piece && board[5] == piece && board[8] == piece) // third col
		return 1;
	else if(board[0] == piece && board[4] == piece && board[8] == piece) // left slash
		return 1;
	else if(board[2] == piece && board[4] == piece && board[6] == piece) // right slash
		return 1;
	else
		return 0;
}
