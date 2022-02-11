#include<stdio.h>
#include<string.h>	//strlen
#include<sys/socket.h>
#include<arpa/inet.h>	//inet_addr
#include<unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>

#define BUFFER_SIZE 512

int main(int argc , char *argv[])
{
	int socket_desc;
	struct sockaddr_in server;
	char buf[BUFFER_SIZE];
	int port;
	
	//Create socket
	socket_desc = socket(AF_INET , SOCK_STREAM , 0);
	if (socket_desc == -1)
	{
		printf("Could not create socket");
	}
	
	server.sin_addr.s_addr = inet_addr(argv[1]);
	server.sin_family = AF_INET;
	if (sscanf(argv[2], "%d", &port) != 1) {
		puts("Erreur: Le paramètre NOMBRE doit être un nombre entier !");
		return EXIT_FAILURE;
	}
	server.sin_port = htons( port );

	//Connect to remote server
	if (connect(socket_desc , (struct sockaddr *)&server , sizeof(server)) < 0)
	{
		puts("connect error");
		return 1;
	}
	
	puts("Connected\n");
	puts("Enter nickname :\n");
	bzero(buf,BUFFER_SIZE);
	scanf("%s",buf);
	if( send(socket_desc, buf , BUFFER_SIZE , 0) < 0)
        {
                puts("recv failed");
        }


	// SOCKET AND STDIN non-blocking mode
	fcntl(socket_desc, F_SETFL, fcntl(socket_desc, F_GETFL) | O_NONBLOCK);
	fcntl(STDIN_FILENO, F_SETFL, fcntl(socket_desc, F_GETFL) | O_NONBLOCK);
	
	while(1){
		bzero(buf, BUFFER_SIZE);
		//Receive a reply from the server
		if( recv(socket_desc, buf , BUFFER_SIZE -1 , 0) < 0)
		{	
			if (errno != EAGAIN)
				puts("recv failed");
		}else{
			printf("%s", buf);
		}
	
		// STDIN read	
		bzero(buf, BUFFER_SIZE);
		if( read(STDIN_FILENO, buf , BUFFER_SIZE -1) < 0)
		{	
			if (errno != EAGAIN)
				puts("recv failed");
		}else{
			send(socket_desc, buf, strlen(buf), 0);
		}
		

		usleep(100000);
	}

	return 0;
}
