#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <wait.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <string.h>
#include "commands.h"
#include "built_in.h"





void * client(void *command) //client socket 생성 //ipc 통신
{

	char **com = (char **)command;
	
	int client_sockfd;
	int status;
	pid_t pid,ppid;
	FILE *fp_in;
	char *pathname="./filetest";
	
	

	struct sockaddr_un clientaddr;

	int fd;



	
	client_sockfd = socket(AF_UNIX,SOCK_STREAM,0);
	
	if(client_sockfd == -1)
	{
		perror("client socket error:");	
		exit(1);

	}
	memset(&clientaddr,0, sizeof(clientaddr));
	clientaddr.sun_family = AF_UNIX;
	strcpy(clientaddr.sun_path, pathname);
		
	if(connect(client_sockfd, (struct sockaddr *)&clientaddr, sizeof(clientaddr))<0)
	{
		perror("Connect error: ");
		exit(0);
	}

	pid = fork();

	if(pid==0)
	{	
		dup2(STDOUT_FILENO,fd); //fd는 stdout의 디스크립터를 복사
		dup2(client_sockfd, STDOUT_FILENO); // stdout의 디스크립터는 client소켓 디스크립터와 같음
		execv(com[0],com);
	//	fprintf(stderr,"%s: command not found\n",com[0]);
		exit(0);
	
	}
	else
	{
		ppid = wait(&status);
		dup2(fd, STDOUT_FILENO); //stdout 다시 제자리로 
//		printf("클라이언트 자식 다 돔\n");
		close(fd); //fd 닫음

		if(ppid < 0)
			perror("wait..");
	
	}
	close(client_sockfd);	

	pthread_exit(0);




}





static struct built_in_command built_in_commands[] = {
  { "cd", do_cd, validate_cd_argv },
  { "pwd", do_pwd, validate_pwd_argv },
  { "fg", do_fg, validate_fg_argv }
};

static int is_built_in_command(const char* command_name)
{
  static const int n_built_in_commands = sizeof(built_in_commands) / sizeof(built_in_commands[0]);

  for (int i = 0; i < n_built_in_commands; ++i) {
    if (strcmp(command_name, built_in_commands[i].command_name) == 0) {
      return i;
    }
  }

  return -1; // Not found
}

/*
 * Description: Currently this function only handles single built_in commands. You should modify this structure to launch process and offer pipeline functionality.
 */
int evaluate_command(int n_commands, struct single_command (*commands)[512])
{
  if (n_commands > 0) {
    struct single_command* com = (*commands);

    assert(com->argc != 0);

    int built_in_pos = is_built_in_command(com->argv[0]);
    if (built_in_pos != -1) {
      if (built_in_commands[built_in_pos].command_validate(com->argc, com->argv)) {
        if (built_in_commands[built_in_pos].command_do(com->argc, com->argv) != 0) {
          fprintf(stderr, "%s: Error occurs\n", com->argv[0]);
        }
      } else {
        fprintf(stderr, "%s: Invalid arguments\n", com->argv[0]);
        return -1;
      }
    } else if (strcmp(com->argv[0], "") == 0) {
      return 0;
    } else if (strcmp(com->argv[0], "exit") == 0) {
      return 1;
    } else {

	if(n_commands ==2)   //server socket  생성  //ipc 통신 
	{   
		pthread_t newthread;
		int server_sockfd, client_sockfd;
		int state, client_len;
		pid_t pid, ppid;
		int status;
		FILE *fp;
		char **commandline;
		int fd= dup(STDIN_FILENO);
		commandline = com->argv ;
	


		pthread_create(&newthread, NULL, client, commandline);
		
		struct sockaddr_un clientaddr, serveraddr;

	
    		char *pathname = "./filetest";
		
   		 state = 0;
	
		 if (access(pathname, F_OK) == 0)
		 {
		        unlink(pathname);
		 }
		
		 client_len = sizeof(clientaddr);
		server_sockfd = socket(AF_UNIX, SOCK_STREAM,0);

		if(server_sockfd==-1)
		{
			perror("socket erro:");	
			exit(0);
		}	
		

		memset(&serveraddr,0x00, sizeof(serveraddr));
		serveraddr.sun_family = AF_UNIX;
		strcpy(serveraddr.sun_path, pathname);
		
		state = bind(server_sockfd, (struct sockaddr *)&serveraddr,sizeof(serveraddr));
		
		if(state ==-1)
		{
			perror("bind error:");
			exit(0);
		}
		
		state = listen(server_sockfd,2);
		if(state == -1)
		{
			perror("listen error:");
			exit(0);
		}

		client_sockfd = accept(server_sockfd, (struct sockaddr *)&clientaddr,&client_len);
		pthread_join(newthread,NULL);	
		pid = fork();
		if(pid == 0)
		{
			if(client_sockfd ==-1)
			{
				perror("accept error:");
				exit(0);
			}
			

			dup2(STDIN_FILENO, fd); // fd는 stdin의 디스크립터 복사
			dup2(client_sockfd, STDIN_FILENO); // stdin은 client디스크립터 복사
		
			execv((com+1)->argv[0], (com+1)->argv);
			fprintf(stderr,"%s : command not found\n", (com+1)->argv[0]);
			

		
		
			exit(0);

			
		}

		else 
		{
			ppid = wait(&status);
		
			dup2(fd, STDIN_FILENO); //stdin 원상복귀
			close(fd);	
			close(client_sockfd);
		}

	}
	else  // process creation
	{
 	
		pid_t child_pid, parent_pid;
		int child_status; 
	
		child_pid=fork();
		if(child_pid ==0)
		{
			execv(com->argv[0],com->argv);
			fprintf(stderr, "%s: command not found\n", com->argv[0]);
			exit(-1);
		}
		else
		{
			parent_pid = wait(&child_status);
			if(parent_pid<0)
			{
				perror("wait..\n");
			}
		}
   
  	  }
  }
}
  return 0;
}

void free_commands(int n_commands, struct single_command (*commands)[512])
{
  for (int i = 0; i < n_commands; ++i) {
    struct single_command *com = (*commands) + i;
    int argc = com->argc;
    char** argv = com->argv;

    for (int j = 0; j < argc; ++j) {
      free(argv[j]);
    }

    free(argv);
  }

  memset((*commands), 0, sizeof(struct single_command) * n_commands);
}
