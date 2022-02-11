/**
	Handle multiple socket connections with select and fd_set on Linux
*/
 
#include <stdio.h>
#include <string.h>   //strlen
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>   //close
#include <arpa/inet.h>    //close
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros
#include<time.h> 
#define TRUE   1
#define FALSE  0
#define PORT 8888

#define MAXCLI 30
#define BUFSIZE 1024
#define LOGSIZE 140

#define TRUE 1
#define FALSE 0

struct nick_mdp{
        int cli_sock;
        char nickname[LOGSIZE];
        char mdp[LOGSIZE];
        int display_date;
        char color[LOGSIZE];
};

struct log{
        char nickname[LOGSIZE];
        char password[LOGSIZE];
};

char* date_msg(char* mydate){
    time_t actTime;
    struct tm *timeComp;
 
    time(&actTime);
    timeComp = localtime(&actTime);
    /*creating string yyyy-MM-dd hh:mm:ss*/
 
    sprintf(mydate,"<%02d/%02d %02d:%02d> ",               timeComp->tm_mday,
                                                    timeComp->tm_mon + 1,
                                                    timeComp->tm_hour,
                                                    timeComp->tm_min);
    
    return mydate;
}

char* color_msg(char* color_selected){
    if(strstr(color_selected, "red"))
            return "\033[31m";
    if(strstr(color_selected, "blue"))
            return "\033[34m";
    if(strstr(color_selected, "off"))
            return "";
    else
        return "";
}

int is_command(char* buffer){
    if(strstr(buffer,"/nickname") != NULL)
        return 1;
    if(strstr(buffer,"/register") != NULL)
        return 1;
    if(strstr(buffer,"/login") != NULL)
        return 1;
    if(strstr(buffer,"/exit") != NULL)
        return 1;
    if(strstr(buffer,"/unregister") != NULL)
        return 1;
    if(strstr(buffer,"/mp") != NULL)
        return 1;
    if(strstr(buffer,"/date") != NULL)
        return 1;
    if(strstr(buffer,"/color") != NULL)
        return 1;
    if(strstr(buffer,"/alert") != NULL)
        return 1;
    else
        return 0;

}

int main(int argc , char *argv[])
{
    int opt = TRUE;
    int master_socket , addrlen , new_socket , client_socket[30] , max_clients = 30 , activity, i , valread , sd;
	int max_sd;
    struct sockaddr_in address;
    struct nick_mdp nicktable[MAXCLI];
    for(i=0;i<MAXCLI;i++){
        nicktable[i].cli_sock = 0;
        strcpy(nicktable[i].nickname, "");
        strcpy(nicktable[i].mdp, "");
        nicktable[i].display_date = FALSE;
        strcpy(nicktable[i].color, "");

    }

    struct log registered_log[MAXCLI];
    char datemsg[25];
    


    char buffer[1025];  //data buffer of 1K
     
    //set of socket descriptors
    fd_set readfds;
     
    //a message
    char *message = "ECHO Daemon v1.0 \r\n";
 
    //initialise all client_socket[] to 0 so not checked
    for (i = 0; i < max_clients; i++) 
    {
        client_socket[i] = 0;
    }
     
    //create a master socket
    if( (master_socket = socket(AF_INET , SOCK_STREAM , 0)) == 0) 
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
 
    //set master socket to allow multiple connections , this is just a good habit, it will work without this
    if( setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 )
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
 
    //type of socket created
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( PORT );
     
    //bind the socket to localhost port 8888
    if (bind(master_socket, (struct sockaddr *)&address, sizeof(address))<0) 
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
	printf("Listener on port %d \n", PORT);
	
    //try to specify maximum of 3 pending connections for the master socket
    if (listen(master_socket, 3) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }
     
    //accept the incoming connection
    addrlen = sizeof(address);
    puts("Waiting for connections ...");
    
	while(TRUE) 
    {
        //clear the socket set
        FD_ZERO(&readfds);
 
        //add master socket to set
        FD_SET(master_socket, &readfds);
        max_sd = master_socket;
		
        //add child sockets to set
        for ( i = 0 ; i < max_clients ; i++) 
        {
            //socket descriptor
			sd = client_socket[i];
            
			//if valid socket descriptor then add to read list
			if(sd > 0)
				FD_SET( sd , &readfds);
            
            //highest file descriptor number, need it for the select function
            if(sd > max_sd)
				max_sd = sd;
        }
 
        //wait for an activity on one of the sockets , timeout is NULL , so wait indefinitely
        activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);
   
        if ((activity < 0) && (errno!=EINTR)) 
        {
            printf("select error");
        }
         
        //If something happened on the master socket , then its an incoming connection
        if (FD_ISSET(master_socket, &readfds)) 
        {
            if ((new_socket = accept(master_socket, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)
            {
                perror("accept");
                exit(EXIT_FAILURE);
            }
         
            //inform user of socket number - used in send and receive commands
            printf("New connection , socket fd is %d , ip is : %s , port : %d \n" , new_socket , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));
       
            //send new connection greeting message
            if( send(new_socket, message, strlen(message), 0) != strlen(message) ) 
            {
                perror("send");
            }
             
            puts("Welcome message sent successfully");
             
            //add new socket to array of sockets
            for (i = 0; i < max_clients; i++) 
            {
                //if position is empty
				if( client_socket[i] == 0 )
                {
                    client_socket[i] = new_socket;
                    nicktable[i].cli_sock = new_socket;
                    strcpy(nicktable[i].nickname,"");
                    strcpy(nicktable[i].mdp,"");
                    printf("Adding to list of sockets as %d\n" , i);
					
					break;
                }
            }
        }
         
        //else its some IO operation on some other socket :)
        for (i = 0; i < max_clients; i++) 
        {
            sd = client_socket[i];
             
            if (FD_ISSET( sd , &readfds)) 
            {
                //Check if it was for closing , and also read the incoming message
                if ((valread = read( sd , buffer, 1024)) == 0)
                {
                    //Somebody disconnected , get his details and printi
                    getpeername(sd , (struct sockaddr*)&address , (socklen_t*)&addrlen);
                    printf("Host disconnected , ip %s , port %d \n" , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));
                     
                    //Close the socket and mark as 0 in list for reuse
                    close( sd );
                    client_socket[i] = 0;
                    nicktable[i].cli_sock = 0;
                }
                 
                //Echo back the message that came in
                else
                {
                    //set the string terminating NULL byte on the end of the data read
                    int j;
                    char* a;
                    buffer[valread] = '\0';
                    char msg[1024] = "";
                    char bufnick[1024] = "", bufmdp[1024] = "", bufmsg[140]="";
                    //gestion des commandes
                    if(is_command(buffer)){
                            if((strstr(buffer, "/nickname") != NULL) && (strlen(buffer)>11)){
                                for(i=0;i<max_clients;i++){
                                    if(nicktable[i].cli_sock == sd){
                                            if(strcmp(nicktable[i].nickname, "")){
                                                    strcat(msg, "You already have a nickname\n");
                                                    send(sd, msg, strlen(msg), 0);
                                                    break;
                                            }
                                            strncpy(bufnick,buffer+10, strlen(buffer+10)-2);

                                            for(j=0;j<MAXCLI;j++){
                                                if(!strcmp(nicktable[j].nickname,bufnick)){
                                                    strcat(msg, "Nickname already used\n");
                                                    break;
                                                }
                                            }
                                            if(strchr(bufnick, ' ')){
                                                strcat(msg, "No space(s) in nickname please\n");
                                                j=MAXCLI-1;
                                            }
                                            if(j>=MAXCLI){
                                                strcpy(nicktable[i].nickname, bufnick);

                                                //msg to display
                                                strcat(msg, nicktable[i].nickname);
                                                strcat(msg, " is your current nickname\n");
                                            }
                                            send(sd, msg, strlen(msg), 0);
                                    }
                                }
                                }

                                if(strstr(buffer, "/unregister") != NULL){ //for unregister
                                    for(i=0;i<MAXCLI;i++){
                                        if(nicktable[i].cli_sock == sd){
                                            strcpy(nicktable[i].nickname, "");
                                            strcpy(nicktable[i].mdp, "");
                                            strcat(msg, "Login clear\n");
                                            send(sd, msg, strlen(msg), 0);

                                        }
                                    }
                                }
                            if(strstr(buffer, "/register") != NULL){
                                 //register
                                    for(i=0;i<MAXCLI;i++){
                                        if(nicktable[i].cli_sock == sd){
                                            if((a = strchr(buffer+10, ' '))!= NULL){
                                                strncpy(bufmdp, a+1, strlen(a+1)-2);
                                                if(strchr(bufmdp, ' ')){
                                                    strcat(msg, "No space(s) in password please\n");
                                                    send(sd,msg,strlen(msg),0);
                                                    break;
                                                }
                                                if(strcmp(nicktable[i].mdp, "")){
                                                        strcat(msg, "You are already registered, unregister to be able to change your nickname\n");
                                                         send(sd, msg, strlen(msg), 0);
                                                        break;
                                                }
                                                else{
                                                    strncpy(bufnick, buffer+10, strlen(buffer)-strlen(bufmdp)-13);
                                        
                                                    for(j=0;j<MAXCLI;j++){
                                                        if(!strcmp(nicktable[j].nickname,bufnick) && !strcmp(nicktable[j].nickname, registered_log[j].nickname)){
                                                            strcat(msg, "Nickname already used\n");
                                                            break;
                                                        }
                                                    }
                                                    if(j>=MAXCLI){
                                                        for(j=0;j<MAXCLI;j++){
                                                            if(!strcmp(registered_log[j].nickname,""))
                                                                break;
                                                        }
                                                        if(j>=MAXCLI){
                                                            strcat(msg, "No more registered users allowed\n");
                                                            send(sd,msg,strlen(msg),0);
                                                            break;
                                                        }
                                                        strcpy(registered_log[j].password, bufmdp);
                                                        strcpy(registered_log[j].nickname, bufnick);
                                                        strcpy(nicktable[i].nickname,bufnick);
                                                        strcpy(nicktable[i].mdp, bufmdp);
                                                        //msg to display
                                                        strcat(msg, registered_log[j].nickname);
                                                        strcat(msg, " ");
                                                        strcat(msg, registered_log[j].password);
                                                        strcat(msg, " registered\n");
                                                    }
                                                }
                                            }
                                            else
                                                strcat(msg, "Add a password please\n");
                                            send(sd, msg, strlen(msg), 0);
                                        }
                                    }
                            }
                            if(strstr(buffer, "/mp") != NULL){
                                    for(i=0;i<MAXCLI;i++){
                                        if(nicktable[i].cli_sock == sd){
                                            if((a = strchr(buffer+4, ' '))!= NULL){
                                                strncpy(bufmsg, a+1, strlen(a+1));
                                                strncpy(bufnick, buffer+4, strlen(buffer)-strlen(bufmsg)-5);
                                                for(j=0;j<MAXCLI;j++){
                                                    if(!strcmp(nicktable[j].nickname, bufnick)){
                                                        strcpy(msg, date_msg(datemsg));
                                                        strcat(msg, "[MP] ");
                                                        if(strcmp(nicktable[i].mdp, ""))
                                                            strcat(msg, "\033[1m");
                                                        strcat(msg, nicktable[i].nickname);
                                                        if(strcmp(nicktable[i].mdp, ""))
                                                            strcat(msg, "\033[0m");
                                                        if(strcmp(nicktable[i].nickname, ""))
                                                            strcat(msg, " : ");
                                                        if(strcmp(nicktable[i].color, ""))
                                                            strcat(msg, color_msg(nicktable[i].color));
                                                        strcat(msg, bufmsg);
                                                        strcat(msg, "\033[0m");
                                                        if(nicktable[j].display_date == FALSE)
                                                            send(nicktable[j].cli_sock , msg+12, strlen(msg+12), 0 );
                                                        else
                                                            send(nicktable[j].cli_sock , msg, strlen(msg), 0 );
                                                        break;
                                                        }
                                                }
                                                if(j>=MAXCLI){
                                                    strcat(msg, "User not found\n");
                                                    send(sd, msg, strlen(msg),0);
                                                }
                                            }
                                            else{
                                                strcat(msg, "Add a message please\n");
                                                send(sd,msg,strlen(msg),0);
                                            }
                                        }
                                    }
                            }
                            if(strstr(buffer, "/exit") != NULL){
                                for(i=0;i<MAXCLI;i++){
                                    if(nicktable[i].cli_sock == sd){
                                       if(!strcmp(nicktable[i].mdp, "")){
                                           strcpy(nicktable[i].nickname, ""); 
                                       }
                                        close(sd);
                                        client_socket[i]=0;
                                        nicktable[i].cli_sock = 0;
                                    }
                                }

                            }

                            if(strstr(buffer, "/login") != NULL){
                                    for(i=0;i<MAXCLI;i++){
                                        if(nicktable[i].cli_sock == sd){
                                            if((a = strchr(buffer+7, ' '))!= NULL){
                                                strncpy(bufmdp, a+1, strlen(a+1)-2);
                                                strncpy(bufnick, buffer+7, strlen(buffer)-strlen(bufmdp)-10);
                                                
                                                for(j=0;j<BUFSIZE;j++){
                                                    if(!strcmp(registered_log[j].nickname,bufnick))
                                                        break;
                                                }
                                                if(j>=MAXCLI){
                                                    strcat(msg, "Nickname not already registered\n");
                                                    send(sd, msg, strlen(msg),0);
                                                }
                                                else{
                                                    if(strcmp(bufmdp, registered_log[j].password)){
                                                        strcat(msg, "Wrong password\n");
                                                        send(sd,msg,strlen(msg),0);
                                                        break;
                                                    }
                                                        strcpy(nicktable[i].nickname, bufnick);
                                                        strcpy(nicktable[i].mdp, bufmdp);

                                                        strcat(msg, "Connected as ");
                                                        strcat(msg,nicktable[i].nickname);
                                                        strcat(msg,"\n");
                                                        send(sd,msg,strlen(msg),0);
                                                }
                                                    
                                            }
                                            else{
                                                strcat(msg, "Add a password please\n");
                                                send(sd,msg,strlen(msg),0);
                                            }
                                        }
                                    }
                            }
                            if(strstr(buffer, "/date")!= NULL){
                                for(i=0;i<MAXCLI;i++){
                                    if(nicktable[i].cli_sock == sd){
                                        if(nicktable[i].display_date == TRUE){
                                            nicktable[i].display_date = FALSE;
                                            strcpy(msg, "Date off\n");
                                            send(sd, msg, strlen(msg),0);
                                        }
                                        else{
                                            nicktable[i].display_date = TRUE;
                                            strcpy(msg, "Date on\n");
                                            send(sd, msg, strlen(msg),0);
                                        }
                                    }
                                }

                            }
                            if(strstr(buffer, "/color") != NULL){
                                for(i=0;i<MAXCLI;i++){
                                    if(nicktable[i].cli_sock == sd){
                                        strncpy(nicktable[i].color,buffer+7, strlen(buffer+7)-2);
                                        strcpy(msg, "Color set\n");
                                        send(sd, msg,strlen(msg),0);
                                    }
                                }
                            }
                            
                            if(strstr(buffer, "/alert") != NULL){
                                    for(i=0;i<MAXCLI;i++){
                                        if(nicktable[i].cli_sock == sd){
                                            if((a = strchr(buffer+7, ' '))!= NULL){
                                                strncpy(bufmsg, a+1, strlen(a+1));
                                                strncpy(bufnick, buffer+7, strlen(buffer)-strlen(bufmsg)-8);
                                                for(j=0;j<MAXCLI;j++){
                                                    if(!strcmp(nicktable[j].nickname, bufnick)){
                                                        strcat(msg, date_msg(datemsg));
                                                        strcat(msg, "\033[31m\a[MP] ");
                                                        if(strcmp(nicktable[i].mdp, ""))
                                                            strcat(msg, "\033[1m");
                                                        strcat(msg, nicktable[i].nickname);
                                                        if(strcmp(nicktable[i].nickname, ""))
                                                            strcat(msg, " : ");
                                                        if(strcmp(nicktable[i].color, ""))
                                                            strcat(msg, color_msg(nicktable[i].color));
                                                        strcat(msg, bufmsg);
                                                        strcat(msg, "\033[0m");
                                                        if(nicktable[j].display_date == FALSE)
                                                            send(nicktable[j].cli_sock , msg+12, strlen(msg+12), 0 );
                                                        else
                                                            send(nicktable[j].cli_sock , msg, strlen(msg), 0 );
                                                        break;
                                                        }
                                                }
                                                if(j>=MAXCLI){
                                                for(j=0;j<max_clients;j++){
                                                    if((nicktable[j].cli_sock != 0) && (nicktable[j].cli_sock != sd)){
                                                        if(nicktable[j].display_date == TRUE)
                                                            strcpy(msg, date_msg(datemsg));
                                                        strcat(msg, "\033[31m\a");
                                                        if(strcmp(nicktable[i].mdp, ""))
                                                            strcat(msg, "\033[1m> ");
                                                        strcat(msg, nicktable[i].nickname);
                                                        if(strcmp(nicktable[i].nickname, ""))
                                                            strcat(msg, " : ");
                                                        if(strcmp(nicktable[i].color, ""))
                                                           strcat(msg, color_msg(nicktable[i].color));
                                                        strcat(msg, buffer+7);
                                                        strcat(msg, "\033[0m");
                                                        send(nicktable[j].cli_sock , msg, strlen(msg), 0 );
                                                    }
                                            
                                                }
                                                }
                                            }
                                            else{
                                                strcpy(msg, "Enter a message please\n");
                                                send(sd, msg, strlen(msg),0);
                                            }
                                        }
                                    }
                            }
                        }
                    else if(buffer[0] == '/'){
                        strcat(msg, "Not a command\n");
                        send(sd,msg,strlen(msg),0);
                    }
                    else{
                         //envoie le message à tous le monde sauf l'envoyeur
                        for(j=0; j< MAXCLI; j++){
                            if(nicktable[j].cli_sock == sd){
                                for(i=0;i<max_clients;i++){
                                    if((nicktable[i].cli_sock != 0) && (nicktable[i].cli_sock != sd)){
                                        if(nicktable[i].display_date == TRUE)
                                            strcpy(msg, date_msg(datemsg));
                                        if(strcmp(nicktable[j].mdp, ""))
                                            strcat(msg, "\033[1m");
                                        strcat(msg, nicktable[j].nickname);
                                        if(strcmp(nicktable[j].mdp, ""))
                                            strcat(msg, "\033[0m");
                                        if(strcmp(nicktable[j].nickname, ""))
                                            strcat(msg, " : ");
                                        if(strcmp(nicktable[j].color, ""))
                                           strcat(msg, color_msg(nicktable[j].color));
                                        strcat(msg, buffer);
                                        strcat(msg, "\033[0m");
                                        send(nicktable[i].cli_sock , msg, strlen(msg), 0 );
                                    }
                            
                            }
                            }
                        }
                    }
                }
            }
        }
    }
     
    return 0;
}
