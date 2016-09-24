/*************************************************************/
/* Chanakya Pallapolu 				          				 */
/* 															 */
/* Implementation: Using select 							 */
/* Usage:  SUMChatServer <sport>							 */				
/*************************************************************/ 

#include "server.h"

void exit(int);
void selectCall();
void UDPBox();
void SCTPBox();
void TCPSetUp(int accept_fd);
void SCTPSetUp(int accept_fd);
void reusePort(int sock);   
void getMessage(int inSock, int outSock);
void sendMessageUtoALL(int inSock);
void sendMessageMtoALL(int inSock);
void sendMessageStoALL(int inSock);
void intHandler(int);	
void intHandler2(int);	
void generate();
void sendBye(struct sockaddr_in client);
void sendList(struct sockaddr_in client, int inSock);


struct sockaddr_in tcpSock, sctpStrSock, sctpSeqSock;
struct hostent *hp, *gethostbyname();
struct sockaddr_in from;
struct sockaddr_in LocalHost;
struct sockaddr_in tcp_server,udp_server;
struct sockaddr_in mcastGroupBye;
struct sockaddr_in client;
struct sockaddr_in ucastAddress;

fd_set assignedFDSet, readFDSet;


int bytes = 0;
int len = sizeof(client);
int accept_fd, accept_sctp_fd;
int fromlen, length;
int i, j;
int send_spres,send_slist;
int identity[MAXCLIENTS];
int accept_sd[MAXCLIENTS];
int currClient=0;
int send_upres,send_mpres;
int send_ulist,send_mlist;
int ip1,ip2,ip3,ip4,mport;
int sCount=0,mCount=0,uCount=0;
int udp_socket_fd, MCastPort, mcastPort, sctp_seq_fd;
int tcp_socket_fd,tcp_socket_bind;
int sctp_stream_fd,sctp_stream_bind;
int TCPCount[1024];
int count[4]; 
int mlist_cntr = 0;
int ulist_cntr = 0;
int slist_cntr = 0;
int send_ip,send_port;
int send_l,send_e;
int tcp_socket_fd,tcp_socket_bind;
int sctp_stream_fd,sctp_stream_bind;
int TCPCount[1024];
int send_ip,send_port;
int send_l,send_e;
char sendBuf[MAX_LEN];
char recvBuf[MAX_LEN];
char tempStr[MAX_LEN];
char welcomeMsg[MAX_LEN];
char byeMsg[MAX_LEN];


char num_cli[1024];
char chatters[2000];
char ThisHost[80];
char mcastPortString[6];
char byeMsg[1024];
char username[50];
char mcastGreeting[100] = "I am a NEW MULTICAST Client\n";
char *mcip;
char mcastip[1024];
char mcastport[1024];
char listcode[10],endcode[10];

u_char TimeToLive;
u_char loop;

struct clients 
{
	int accept;
	char ip[20];
	int port;
	char user[50];
} mlist[10], ulist[10], slist[10];

main( int argc,char *argv[] ){


	if (argc != 2){
		printf("Usage: SUMChatServer <port>\n");
		exit(-1);
	}


	signal(SIGINT, intHandler);
	signal(SIGQUIT, intHandler);
	signal(SIGCHLD, SIG_IGN);	
	signal(SIGPIPE, SIG_IGN);
	signal(SIGTERM, SIG_IGN);	

	
	for(i=0;i<MAXCLIENTS;i++){
		identity[i]=0;
	}

	generate();			/**generates multicast address, port, L and E*/


	/** Creating TCP Socket*/
	tcp_server.sin_family = AF_INET;
	tcp_server.sin_port = htons(atoi(argv[1]));
	tcp_server.sin_addr.s_addr = htonl(INADDR_ANY);

	tcp_socket_fd = socket(AF_INET,SOCK_STREAM,0);

	if (tcp_socket_fd < 0){
		perror("Error in creating TCP Socket \n");
		exit(0);
	}
	
	reusePort(tcp_socket_fd);

	tcp_socket_bind = bind(tcp_socket_fd, (SA *) &tcp_server,sizeof(tcp_server));		/** Binding TCP Socket */

	if (tcp_socket_bind < 0){
		close(tcp_socket_fd);
		perror("Error in binding TCP Socket \n");
		exit(0);
	}
	listen(tcp_socket_fd,4);

	
	
	/** Creating SCTP Socket*/
	sctp_stream_fd = socket (AF_INET,SOCK_STREAM,IPPROTO_SCTP); 
	
	if (sctp_stream_fd<0) 
	{
		perror("opening SCTP stream socket");
		exit(-1);
	}
	
	/* this allow the TCP server to re-start quickly instead of fully wait
		for TIME_WAIT which can be as large as 2 minutes */
	reusePort(sctp_stream_fd);

	sctp_stream_bind = bind( sctp_stream_fd, (SA *) &tcp_server, sizeof(tcp_server) );
	
	if ( sctp_stream_bind < 0 ) 
	{
		close(sctp_stream_fd);
		perror("Error in SCTP stream socket binding");
		exit(-1);
	}

	listen(sctp_stream_fd,1);

	/** Getting the host name to display server name and address*/

	gethostname(ThisHost,MAXHOSTNAME);
	hp = gethostbyname(ThisHost);

	bcopy ( hp->h_addr, &(tcp_server.sin_addr), hp->h_length);

	printf("Server Name is: %s.cs.odu.edu\n",ThisHost);
   // printf("Server Name is : %s\n", hp->h_name);
	printf("Server IP is: %s\n", inet_ntoa(tcp_server.sin_addr));
	printf("Server Port is: %d\n", ntohs(tcp_server.sin_port));
	printf("\n");		
	

	FD_ZERO (&assignedFDSet);
	FD_SET (tcp_socket_fd, &assignedFDSet);
	FD_SET (sctp_stream_fd, &assignedFDSet);
	UDPBox();
	SCTPBox();
	selectCall();
}
	
	
/* accept TCP connections from clients */	
void TCPAccept(){
	//printf("hi\n");
	fromlen = sizeof(from);  
	if ((accept_fd  = accept(tcp_socket_fd, (SA *)&from, &fromlen)) < 0)
		exit(-1);

	identity[count[0]] = accept_fd;
	accept_sd[count[0]] = accept_fd;
	
	count[0]++;		
	TCPSetUp(accept_fd);
	
}

void generate(){
	int L,E;
	int port_high = 11001;
	int port_low = 9999;
	int ip_start = 224;
	int ip_end = 239;

	srand ((unsigned) getpid ());
	
	ip1 = rand() % (ip_end - ip_start +1) + ip_start;
	ip2 = rand()%255+1;
	ip3 = rand()%255+1;
	ip4 = rand()%255+1;
	mport  = rand () % (port_high-port_low+1) + port_low;
	L = rand () % 1000000 + 1000000;
	E = rand () % 1000000 + 1000000;
	printf("\n");
	printf("The Multicast IP Address is: %d.%d.%d.%d\n",ip1,ip2,ip3,ip4);	
  	printf ("The Multicast Port Address is: %d\n", mport);
  	printf ("The List Code is: %d\n", L);
  	printf ("The End Code is: %d\n", E);
	printf("\n");

	sprintf(mcastip,"%d.%d.%d.%d",ip1,ip2,ip3,ip4);			/** converting ip address to char type */
	mcip = mcastip;	
	sprintf(mcastport,"%d",mport);
	sprintf(listcode,"%d",L);
    sprintf(endcode,"%d",E);
}

void SCTPAccept(){
	fromlen = sizeof(from);  
	if ((accept_sctp_fd  = accept(sctp_stream_fd, (SA *)&from, &fromlen)) < 0)
		exit(-1);

	//FD_SET (accept_sctp_fd, &assignedFDSet);
	identity[count[0]] = accept_sctp_fd;
	accept_sd[count[0]] = accept_sctp_fd;
	
	count[0]++;		
	SCTPSetUp(accept_sctp_fd);
}

// Select() is used to handle all the asynchronous I/Os. 
// This includes TCP listener socket, TCP Comm socket and UDP comm socket.
void selectCall(){
	int presReads;
	int i;

	for(;;){
		memcpy(&readFDSet, &assignedFDSet, sizeof(assignedFDSet));


		//printf("**********  SELECT  **********\n");
		presReads = select(FD_SETSIZE, &readFDSet, NULL, NULL, NULL);
		if (presReads < 0){
			perror ("Error in select Call");
			exit(-1);
        }	

		for (i = 0; i< FD_SETSIZE; i++){
			if (FD_ISSET(i, &readFDSet)){
				//printf("select triggered for %d\n", i);	
				if (i == tcp_socket_fd){
					TCPAccept();
				}
				else if (i == udp_socket_fd){
					getMessage(udp_socket_fd, 1);
				}
				else if (i == sctp_seq_fd){
					getMessage(sctp_seq_fd, 1);
				}				
				else if (i == sctp_stream_fd){
					SCTPAccept();
				}
			}
		}
	}
}

void UDPBox(){
	// Creating an UDP Socket */
	MCastPort = htons(mport);
	
	LocalHost.sin_family = AF_INET;
	LocalHost.sin_port = MCastPort;
	LocalHost.sin_addr.s_addr = htonl(INADDR_ANY);

	if ((udp_socket_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		printf("Can't create UDP socket: \n");
		exit(-1);
	}	

	reusePort(udp_socket_fd);

	if (bind(udp_socket_fd, (SA *) & LocalHost, sizeof(LocalHost)) < 0) {

		printf("Error in binding UDP Socket\n");
		exit(-1);
	}   
	FD_SET (udp_socket_fd, &assignedFDSet);		

	TimeToLive = 2;
	setTTLvalue(udp_socket_fd, &TimeToLive);

	loop = 1;		/* enable loopback */
	setLoopback(udp_socket_fd, loop);

	joinGroup(udp_socket_fd, mcip);
}

void SCTPBox(){
	// Create an SCTP Sequence Packet */
	MCastPort = htons(mport);
	LocalHost.sin_family = AF_INET;
	LocalHost.sin_port = MCastPort;
	LocalHost.sin_addr.s_addr = htonl(INADDR_ANY);

	if ((sctp_seq_fd = socket(AF_INET, SOCK_SEQPACKET,IPPROTO_SCTP)) < 0) {
		printf("Error in creating SEQPACKET socket");
		exit(-1);
	}	

	reusePort(sctp_seq_fd);

	if (bind(sctp_seq_fd, (SA *) & LocalHost, sizeof(LocalHost)) < 0) {

		printf("Error in binding sctp_seq_fd");
		exit(-1);
	}   
	listen(sctp_seq_fd,1);
	FD_SET (sctp_seq_fd, &assignedFDSet);		
}

void TCPSetUp(int accept_fd){
	char um_options_buf1[512];
	char um_options_buf2[512];
	int len;
	
		fflush(stdout);
		cleanup(um_options_buf1);
		cleanup(um_options_buf2);
		
		int um_options_rc1 = recv(accept_fd, um_options_buf1, sizeof(um_options_buf1),0);
		if (um_options_rc1 < 0){
			perror("Error in receiving UM Options");
			exit(0);
		}
		//printf("UMOPTIONS2---------------->%s\n",um_options_buf1 );

		strcpy(username,um_options_buf1);

		int um_options_rc2 = recv(accept_fd, um_options_buf2, sizeof(um_options_buf2),0);

		//printf("UMOPTIONS1---------------->%s\n",um_options_buf2 );
			/*int um_options_rc2 = recv(accept_fd, um_options_buf1, sizeof(um_options_buf1),0);*/
			
			TCPCount[accept_fd]++;

			if (strcmp(um_options_buf2,"0")==0){
				printf("New Multicast Client accepted: %s @ (%s:%d) \n\n",username,inet_ntoa(from.sin_addr), ntohs(from.sin_port));
				strcpy(mlist[mlist_cntr].ip, inet_ntoa(from.sin_addr));				
				mlist[mlist_cntr].port = ntohs(from.sin_port);
				mlist[mlist_cntr].accept = accept_fd;					
				strcpy(mlist[mlist_cntr].user, username);	
				/*
				for(i=0;i<=mlist_cntr;i++){
					printf("MUL---------->USER---->%s,IP---->%s,Port----->%d\n",mlist[mlist_cntr].user,mlist[mlist_cntr].ip,mlist[mlist_cntr].port );
				}*/
				mlist_cntr++;

				send_ip = send(accept_fd,mcastip,strlen(mcastip),0);
		    	if (send_ip < 0){
		    		perror("Error in sending IP to multicast client");
		    		exit(0);
		    	}
		    	
		    	usleep(500);						
			}
			else if(strcmp(um_options_buf2,"1")==0){
				printf("New Unicast Client accepted: %s @ (%s:%d) \n\n",username,inet_ntoa(from.sin_addr), ntohs(from.sin_port));
				strcpy(ulist[ulist_cntr].ip, inet_ntoa(from.sin_addr));
				ulist[ulist_cntr].port = ntohs(from.sin_port);
				ulist[ulist_cntr].accept = accept_fd;					
				strcpy(ulist[ulist_cntr].user, username);	

				/*for(i=0;i<=ulist_cntr;i++){
					printf("UNI----------->USER---->%s,IP---->%s,Port----->%d\n",ulist[ulist_cntr].user,ulist[ulist_cntr].ip,ulist[ulist_cntr].port );
				}*/
				ulist_cntr++;
			}
			
			// Send Multicast Port number, L code & E Code to the client.
			send_port = send(accept_fd,mcastport,strlen(mcastport),0);
	    	if (send_port < 0){
	    		perror("Error in sending PORT to multicast client");
	    		exit(0);
	    	}
	    	

            //Sending L,E to the Clients 
            usleep(500);
            
            send_l = send(accept_fd,listcode,sizeof(listcode),0);
            if (send_l < 0){
                perror("Error in sending L to client");
                exit(0);
            }
           
            usleep(500);

            send_e = send(accept_fd,endcode,sizeof(endcode),0);
            if (send_e < 0){
                perror("Error in sending E to client");
                exit(0);
            }
           
            usleep(1500);
}

void SCTPSetUp(int accept_fd){
	char sctpBuf[512];
	int len;
	
	fflush(stdout);
	cleanup(sctpBuf);

	if (recv(accept_fd, sctpBuf, sizeof(sctpBuf),0) < 0){
		perror("receiving SCTP stream message ");
	}

	strcpy(username, sctpBuf);	
	cleanup(sctpBuf);

	printf("New SCTP Client accepted: %s @ (%s:%d) \n\n",username,inet_ntoa(from.sin_addr), ntohs(from.sin_port)+1);
	strcpy(slist[slist_cntr].ip, inet_ntoa(from.sin_addr));
	slist[slist_cntr].port = ntohs(from.sin_port)+1;
	slist[slist_cntr].accept = accept_fd;					
	strcpy(slist[slist_cntr].user, username);	

	/*for(i=0;i<=slist_cntr;i++){
		printf("SCTP--------->USER---->%s,IP---->%s,Port----->%d\n",slist[slist_cntr].user,slist[slist_cntr].ip,slist[slist_cntr].port );
	}*/
	slist_cntr++;

	
	// Send Multicast Port number, L code & E Code to the client.
	send_port = send(accept_fd,mcastport,strlen(mcastport),0);
	if (send_port < 0){
		perror("Error in sending PORT to multicast client");
		exit(0);
	}
	

    //Sending L,E to the Clients 
    usleep(500);
    
    send_l = send(accept_fd,listcode,sizeof(listcode),0);
    if (send_l < 0){
        perror("Error in sending L to client");
        exit(0);
    }
   
    usleep(500);

    send_e = send(accept_fd,endcode,sizeof(endcode),0);
    if (send_e < 0){
        perror("Error in sending E to client");
        exit(0);
    }
   
    usleep(1500);
}

void intHandler(int sig){
	signal(sig, intHandler);
	int i;
	int ulist_cntr1=0,mlist_cntr1=0,slist_cntr1=0;
	char byeMsg[MAX_LEN];
	memset(byeMsg, '\0', MAX_LEN);
	struct sockaddr_in client;
	
	struct sockaddr_in mcastGroupBye;	
	mcastGroupBye.sin_family = AF_INET;
	mcastGroupBye.sin_port = MCastPort;
	mcastGroupBye.sin_addr.s_addr = inet_addr(mcip);		
	
	// intHandler for CTRL+C SIGINT
	if(sig == SIGINT){
		printf("\n*****************************************************\n");	
		printf("\nList of Chat Clients\n");
		printf("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n\n");
		
		printf("Unicast TCP Clients\n");
		printf("**************************************\n");


		for (i = 0; i<ulist_cntr; i++){
			if (ulist[i].accept > -1){
				printf("%s @ %s : %d \n",ulist[i].user,ulist[i].ip, ulist[i].port);
				ulist_cntr1++;
			}
		}		
		if (ulist_cntr1 == 0){
			printf("No Unicast Clients\n");	
		}

		printf("**************************************\n");
			
		printf("\n");
		printf("Multicast Clients\n");
		printf("**************************************\n");
	
		for (i = 0; i<mlist_cntr; i++){
			if (mlist[i].accept > -1){
				printf("%s @ %s : %d \n",mlist[i].user,mlist[i].ip, mlist[i].port);
				mlist_cntr1++;
			}
		}	
		if (mlist_cntr1 == 0){
			printf("No Multicast Clients\n");	
		}
		printf("**************************************\n");
			
		printf("\n");			
		printf("SCTP Clients\n");
		printf("**************************************\n");

		for (i = 0; i<slist_cntr; i++){
			if (slist[i].accept > -1){
				printf("%s @ %s : %d \n",slist[i].user,slist[i].ip, slist[i].port);
				slist_cntr1++;
			}
		}		
		if (slist_cntr1 == 0){
			printf("No SCTP Clients\n");	
		}
		printf("**************************************\n");	
		printf("\n*****************************************************\n");	
		selectCall();			
	} 
	
	// intHandler for CTRL + \ (SIGQUIT)
	else if(sig == SIGQUIT){
				
		// Send E code to all connected TCP Unicast Clients
		for (i = 0; i<ulist_cntr; i++){
			if (ulist[i].accept > -1){
				client.sin_addr.s_addr = inet_addr(ulist[i].ip);
				client.sin_port = ulist[i].port;
				client.sin_family = AF_INET;
				if (sendto(udp_socket_fd, endcode, strlen(endcode), 0, (SA *) & client, sizeof(client)) < 0) 
				{
					printf("error in sending endcode to unicast client\n");
					exit(-1);
				}					
			}
		}		

		// Send E code to all connected Multicast Clients
		if (ulist_cntr>0){
			if (sendto(udp_socket_fd, endcode, strlen(endcode), 0, (SA *) & mcastGroupBye, sizeof(mcastGroupBye)) < 0){
				printf("error in sending endcode to multicast clients\n");
				exit(-1);
			}			
		}
		
		// Send E code to all connected SCTP Unicast Clients
		for (i = 0; i<slist_cntr; i++){
			if (slist[i].accept > -1){
				client.sin_addr.s_addr = inet_addr(slist[i].ip);
				client.sin_port = slist[i].port;
				client.sin_family = AF_INET;
				if (sendto(sctp_seq_fd, endcode, strlen(endcode), 0, (SA *) & client, sizeof(client)) < 0){
					printf("error in sending to sctp clients\n");
					exit(-1);
				}					
			}
		}				
		
	
		exit(0);
	} 
}

void getMessage(int inSock, int outSock){
		
	mcastGroupBye.sin_family = AF_INET;
	mcastGroupBye.sin_port = MCastPort;
	mcastGroupBye.sin_addr.s_addr = inet_addr(mcip);	


	memset(recvBuf, '\0', MAX_LEN);
	memset(tempStr, '\0', MAX_LEN);
	bytes = recvfrom(inSock, recvBuf, MAX_LEN, 0, (struct sockaddr *)&client, (socklen_t *)&len);

	if (bytes < 0){
		printf("error in reading from socket\n");
		exit(-1);
	} 

	else if (strcmp(recvBuf, listcode)==0){
		sendList(client,inSock);
	}
	else if (strcmp(recvBuf, endcode)==0){
		
		memset(byeMsg, '\0', MAX_LEN);
		// Check if Unicast Client is left
		for (i = 0; i<ulist_cntr; i++){
			if (client.sin_port == ulist[i].port){	
				ulist[i].accept = -1;
				ulist_cntr = ulist_cntr - 1;				
				sprintf(byeMsg, "Unicast Client [%s @ %s:%d] has left the chat\n\n",ulist[i].user,ulist[i].ip,ulist[i].port); 
				printf("%s",byeMsg);						
				
			}
		}	
		// Check if Multicast Client is left
		for (i = 0; i<mlist_cntr; i++){
			if (client.sin_port == mlist[i].port){	
				mlist[i].accept = -1;
				mlist_cntr = mlist_cntr - 1;							
				sprintf(byeMsg, "Multicast Client [%s @ %s:%d] has left the chat\n\n",mlist[i].user,mlist[i].ip,mlist[i].port); 
				printf("%s",byeMsg);	
				
			}
		}
		// Check if SCTP Client is left		
		for (i = 0; i<slist_cntr; i++){
			if (client.sin_port == slist[i].port){
				slist[i].accept = -1;
				slist_cntr = slist_cntr - 1;
				sprintf(byeMsg, "SCTP Client [%s @ %s:%d] has left the chat\n\n",slist[i].user,slist[i].ip, slist[i].port); 
				printf("%s",byeMsg);					
				
				//printf("SCTP NUM------------>%d\n",slist_cntr );
			}
		}	

		sendBye(client);			
	}
	else{
			
		fflush(stdout);

		/*bytes = strlen(recvBuf);*/
		// Check the received message form a Unicast client.
		for (i = 0; i<ulist_cntr; i++){
			
			if (ulist[i].port == client.sin_port){

				//printf("U------------>%s", recvBuf);	
				if(strcmp(recvBuf,"I am a NEW UNICAST Client\n") == 0){
					//memset(recvBuf, '\0', MAX_LEN);
					sprintf(welcomeMsg, "Unicast Client [%s @ %s:%d] has joined the Chat\n",ulist[i].user,inet_ntoa(client.sin_addr),client.sin_port);
					//strcpy(recvBuf, welcomeMsg);
					//printf("%s", welcomeMsg);	
				}
				else if(strcmp(recvBuf,"I am a NEW UNICAST Client\n") != 0){
					sprintf(tempStr, "From Unicast Client [%s @ %s:%d] --> %s\n", ulist[i].user,inet_ntoa(client.sin_addr),client.sin_port, recvBuf);
					memset(recvBuf, '\0', MAX_LEN);
					strcpy(recvBuf, tempStr);
					printf("%s", recvBuf);	
				}

				sendMessageUtoALL(inSock);

				break;							
			}
		}
		// Check the received message form a Multicast client.		
		for (i = 0; i<mlist_cntr; i++){
		
			if (mlist[i].port == client.sin_port){

				//printf("M----------->%s\n",recvBuf);

				if(strcmp(recvBuf,"I am a NEW MULTICAST Client\n") == 0){
					//memset(recvBuf, '\0', MAX_LEN);
					sprintf(welcomeMsg, "Multicast Client [%s @ %s:%d] has joined the Chat\n",mlist[i].user,inet_ntoa(client.sin_addr),client.sin_port);
					//strcpy(recvBuf, welcomeMsg);
					//printf("%s", welcomeMsg);	
				}
				else if(strcmp(recvBuf,"I am a NEW MULTICAST Client\n") != 0){
					sprintf(tempStr, "From Multicast Client [%s @ %s:%d] --> %s\n", mlist[i].user,inet_ntoa(client.sin_addr),client.sin_port, recvBuf);
					memset(recvBuf, '\0', MAX_LEN);
					strcpy(recvBuf, tempStr);
					printf("%s", recvBuf);	
				}			
						
				sendMessageMtoALL(inSock);
				
				break;
			}
		}	
		// Check the received message form a SCTP client.
		for (i = 0; i<slist_cntr; i++){
			
			if (slist[i].port == client.sin_port){
			//	printf("S--------------->%s", recvBuf);
				if(strcmp(recvBuf,"I am a NEW SCTP Client\n") == 0){
					//memset(recvBuf, '\0', MAX_LEN);
					sprintf(welcomeMsg, "SCTP Client [%s @ %s:%d] has joined the Chat\n",slist[i].user,inet_ntoa(client.sin_addr),client.sin_port);
					//strcpy(recvBuf, welcomeMsg);
					//printf("%s", recvBuf);	
				}
				else if(strcmp(recvBuf,"I am a NEW SCTP Client\n") != 0){
					sprintf(tempStr, "From SCTP Client [%s @ %s:%d] --> %s\n", slist[i].user,inet_ntoa(client.sin_addr),client.sin_port, recvBuf);
					memset(recvBuf, '\0', MAX_LEN);
					strcpy(recvBuf, tempStr);
					printf("%s", recvBuf);	
				}

			/*	sprintf(tempStr, "From SCTP Client [%s @ %s:%d] --> %s\n", slist[i].user,inet_ntoa(client.sin_addr),client.sin_port, recvBuf);
				memset(recvBuf, '\0', MAX_LEN);
				strcpy(recvBuf, tempStr);
				printf("%s", recvBuf);	*/
				
				sendMessageStoALL(inSock);

				break;					
			}
		}			
	}
}

void sendMessageUtoALL(int inSock){
	//printf("Sending Message-------------->%d\n",inSock);

	if (mlist_cntr>0){
		if(strcmp(recvBuf,"I am a NEW UNICAST Client\n") == 0){		
			if (sendto(inSock, welcomeMsg, MAX_LEN, 0, (SA *) & mcastGroupBye, sizeof(mcastGroupBye)) < 0){
				printf("error in sending Unicast message to multicast clients\n");
				exit(-1);
			}
		}
		else if(strcmp(recvBuf,"I am a NEW UNICAST Client\n") != 0){
			if (sendto(inSock, recvBuf, MAX_LEN, 0, (SA *) & mcastGroupBye, sizeof(mcastGroupBye)) < 0){
				printf("error in sending Unicast message to multicast clients\n");
				exit(-1);
			}
		}
	}
	if (ulist_cntr>1){
		if(strcmp(recvBuf,"I am a NEW UNICAST Client\n") == 0){
				
			for (j = 0; j<ulist_cntr; j++){
				ucastAddress.sin_family = AF_INET;
				ucastAddress.sin_port = ulist[j].port;
				ucastAddress.sin_addr.s_addr = inet_addr(ulist[j].ip);
				if (sendto(inSock, welcomeMsg, MAX_LEN, 0, (SA *) & ucastAddress, sizeof(ucastAddress)) < 0){
					printf("error in sending unicast messages to unicast clients\n");
					exit(-1);
				}										
			}
		}
		else if(strcmp(recvBuf,"I am a NEW UNICAST Client\n") != 0){

			for (j = 0; j<ulist_cntr; j++){
				if (i != j){
					ucastAddress.sin_family = AF_INET;
					ucastAddress.sin_port = ulist[j].port;
					ucastAddress.sin_addr.s_addr = inet_addr(ulist[j].ip);
					if (sendto(inSock, recvBuf, MAX_LEN, 0, (SA *) & ucastAddress, sizeof(ucastAddress)) < 0){
						printf("error in sending unicast messages to unicast clients\n");
						exit(-1);
					}										
				}
			}
		}
	}
	if (slist_cntr>0){
		if(strcmp(recvBuf,"I am a NEW UNICAST Client\n") == 0){				
			for (j = 0; j<slist_cntr; j++){
				ucastAddress.sin_family = AF_INET;
				ucastAddress.sin_port = slist[j].port;
				ucastAddress.sin_addr.s_addr = inet_addr(slist[j].ip);
				if (sendto(sctp_seq_fd, welcomeMsg, MAX_LEN, 0, (SA *) & ucastAddress, sizeof(ucastAddress)) < 0){
					printf("error in sending unicast messages to sctp clients\n");
					exit(-1);
				}										
			}
		}
		else if(strcmp(recvBuf,"I am a NEW UNICAST Client\n") != 0){
			for (j = 0; j<slist_cntr; j++){
				ucastAddress.sin_family = AF_INET;
				ucastAddress.sin_port = slist[j].port;
				ucastAddress.sin_addr.s_addr = inet_addr(slist[j].ip);
				if (sendto(sctp_seq_fd, recvBuf, MAX_LEN, 0, (SA *) & ucastAddress, sizeof(ucastAddress)) < 0){
					printf("error in sending unicast messages to sctp clients\n");
					exit(-1);
				}										
			}
		}
	}							
}

void sendMessageMtoALL(int inSock){
	//printf("Sending Messagfe from M to ALL--------->%s\n",recvBuf );
	if (mlist_cntr>1){
		if(strcmp(recvBuf,"I am a NEW MULTICAST Client\n") == 0){		
			if (sendto(inSock, welcomeMsg, MAX_LEN, 0, (SA *) &mcastGroupBye, sizeof(mcastGroupBye)) < 0){
				printf("error in sending Multicast welcome message to All Multicast Clients clients\n");
				exit(-1);
			}
		}
		else if(strcmp(recvBuf,"I am a NEW MULTICAST Client\n") != 0){
			if (sendto(inSock, recvBuf, MAX_LEN, 0, (SA *) &mcastGroupBye, sizeof(mcastGroupBye)) < 0){
				printf("error in sending Multicast message to All Multicast Clients clients\n");
				exit(-1);
			}
		}
	}
	if (ulist_cntr>0){

		if(strcmp(recvBuf,"I am a NEW MULTICAST Client\n") == 0){				
			for (j = 0; j<ulist_cntr; j++){
				ucastAddress.sin_family = AF_INET;
				ucastAddress.sin_port = ulist[j].port;
				ucastAddress.sin_addr.s_addr = inet_addr(ulist[j].ip);
				if (sendto(inSock, welcomeMsg, MAX_LEN, 0, (SA *) & ucastAddress, sizeof(ucastAddress)) < 0){
					printf("error in sending Multicast welcome message to All Unicast clients\n");
					exit(-1);
				}									
			}
		}
		else if(strcmp(recvBuf,"I am a NEW MULTICAST Client\n") != 0){
			for (j = 0; j<ulist_cntr; j++){
				ucastAddress.sin_family = AF_INET;
				ucastAddress.sin_port = ulist[j].port;
				ucastAddress.sin_addr.s_addr = inet_addr(ulist[j].ip);
				if (sendto(inSock, recvBuf, MAX_LEN, 0, (SA *) & ucastAddress, sizeof(ucastAddress)) < 0){
					printf("error in sending Multicast message to All Unicast clients\n");
					exit(-1);
				}									
			}
		}
	}
	if (slist_cntr>0){
		if(strcmp(recvBuf,"I am a NEW MULTICAST Client\n") == 0){							
			for (j = 0; j<slist_cntr; j++){
				ucastAddress.sin_family = AF_INET;
				ucastAddress.sin_port = slist[j].port;
				ucastAddress.sin_addr.s_addr = inet_addr(slist[j].ip);
				if (sendto(sctp_seq_fd, welcomeMsg, MAX_LEN, 0, (SA *) & ucastAddress, sizeof(ucastAddress)) < 0){
					printf("error in sending Multicast welcome message to All SCTP clients\n");
					exit(-1);
				}										
			}
		}
		if(strcmp(recvBuf,"I am a NEW MULTICAST Client\n") != 0){		
			for (j = 0; j<slist_cntr; j++){
				ucastAddress.sin_family = AF_INET;
				ucastAddress.sin_port = slist[j].port;
				ucastAddress.sin_addr.s_addr = inet_addr(slist[j].ip);
				if (sendto(sctp_seq_fd, recvBuf, MAX_LEN, 0, (SA *) & ucastAddress, sizeof(ucastAddress)) < 0){
					printf("error in sending Multicast message to All SCTP clients\n");
					exit(-1);
				}										
			}
		}
	}	
}

void sendMessageStoALL(int inSock){
	//printf("Sending Messagfe from S to ALL--------->%d,%d\n",udp_socket_fd,inSock);
	if (mlist_cntr>0){
		if(strcmp(recvBuf,"I am a NEW SCTP Client\n") == 0){
			//printf("Sending Welcome Message from S to ALL--------->%s\n",welcomeMsg );
			if (sendto(udp_socket_fd, welcomeMsg, MAX_LEN, 0, (SA *) & mcastGroupBye, sizeof(mcastGroupBye)) < 0){
				printf("error in sending welcome Message to ALL Multicast Clients\n");
				exit(-1);
			}
		}
		else if(strcmp(recvBuf,"I am a NEW SCTP Client\n") != 0){
			if (sendto(udp_socket_fd, recvBuf, MAX_LEN, 0, (SA *) & mcastGroupBye, sizeof(mcastGroupBye)) < 0){
				printf("error in sending message to ALL Multicat Clients\n");
				exit(-1);
			}
		}
	}	
	if (ulist_cntr>0){
		if(strcmp(recvBuf,"I am a NEW SCTP Client\n") == 0){
			for (j = 0; j<=ulist_cntr; j++){
				ucastAddress.sin_family = AF_INET;
				ucastAddress.sin_port = ulist[j].port;
				ucastAddress.sin_addr.s_addr = inet_addr(ulist[j].ip);
				if (sendto(udp_socket_fd, welcomeMsg, MAX_LEN, 0, (SA *) & ucastAddress, sizeof(ucastAddress)) < 0){
					printf("error in sending welcome message to ALL UniCast Clients\n");
					exit(-1);
				}	
			}
		}
		else if(strcmp(recvBuf,"I am a NEW SCTP Client\n") != 0){
			for (j = 0; j<=ulist_cntr; j++){
				ucastAddress.sin_family = AF_INET;
				ucastAddress.sin_port = ulist[j].port;
				ucastAddress.sin_addr.s_addr = inet_addr(ulist[j].ip);
				if (sendto(udp_socket_fd, recvBuf, MAX_LEN, 0, (SA *) & ucastAddress, sizeof(ucastAddress)) < 0){
					printf("error in sending message to ALL Unicast Clients\n");
					exit(-1);
				}	
			}
		}
	}	
	if (slist_cntr>1){
		if(strcmp(recvBuf,"I am a NEW SCTP Client\n") == 0){										
			for (j = 0; j<slist_cntr; j++){					
				ucastAddress.sin_family = AF_INET;
				ucastAddress.sin_port = slist[j].port;
				ucastAddress.sin_addr.s_addr = inet_addr(slist[j].ip);
				if (sendto(inSock, welcomeMsg, MAX_LEN, 0, (SA *) & ucastAddress, sizeof(ucastAddress)) < 0){
					printf("error in sending welcome message to ALL SCTP Clients\n");
					exit(-1);
				}	
			}
		}
		else if(strcmp(recvBuf,"I am a NEW SCTP Client\n") != 0){
			for (j = 0; j<slist_cntr; j++){			
					ucastAddress.sin_family = AF_INET;
					ucastAddress.sin_port = slist[j].port;
					ucastAddress.sin_addr.s_addr = inet_addr(slist[j].ip);
					if (sendto(inSock, recvBuf, MAX_LEN, 0, (SA *) & ucastAddress, sizeof(ucastAddress)) < 0){
						printf("error in sending message to ALL SCTP Clients\n");
						exit(-1);
					}	
			}
		}	
	}			
}

void sendList(struct sockaddr_in client, int inSock){

	for (i = 0; i<=ulist_cntr; i++){
		if (ulist[i].accept > -1){
			uCount++;
		}
	}		
	for (i = 0; i<=mlist_cntr; i++){
		if (mlist[i].accept > -1){
			mCount++;
		}
	}	
	for (i = 0; i<=slist_cntr; i++){
		if (slist[i].accept > -1){
			sCount++;
		}
	}
	//unicast
	memset(tempStr, '\0', MAX_LEN);
	sprintf(tempStr, "List of Unicast Connected Clients\n");
	if (sendto(inSock, tempStr, strlen(tempStr), 0, (SA *) & client, sizeof(client)) < 0){
		printf("error in sending Heading\n");
		exit(-1);
	}	
	memset(tempStr, '\0', MAX_LEN);
	sprintf(tempStr, "**************************************\n");
	if (sendto(inSock, tempStr, strlen(tempStr), 0, (SA *) & client, sizeof(client)) < 0){
		printf("error in sending *'s\n");
		exit(-1);
	}	
	if (uCount == 0){
		memset(tempStr, '\0', MAX_LEN);
		sprintf(tempStr, "No UniCast Clients Connected\n");
		if (sendto(inSock, tempStr, strlen(tempStr), 0, (SA *) & client, sizeof(client)) < 0){
			printf("error in sending UNicast\n");
			exit(-1);
		}		
	}
	for (i = 0; i<ulist_cntr; i++){
		if (ulist[i].accept > -1){
			memset(tempStr, '\0', MAX_LEN);
			sprintf(tempStr, "%s @ (%s : %d)\n",ulist[i].user,ulist[i].ip, ulist[i].port);
			if (sendto(inSock, tempStr, strlen(tempStr), 0, (SA *) & client, sizeof(client)) < 0){
				printf("error in sending chatters Unicast\n");
				exit(-1);
			}
		}
	}
	memset(tempStr, '\0', MAX_LEN);
	sprintf(tempStr, "**************************************\n\n");
	if (sendto(inSock, tempStr, strlen(tempStr), 0, (SA *) & client, sizeof(client)) < 0){
		printf("error in sending *'s\n");
		exit(-1);
	}
	//multicast 
	memset(tempStr, '\0', MAX_LEN);
	sprintf(tempStr, "List of Multicast Connected Clients\n");
	if (sendto(inSock, tempStr, strlen(tempStr), 0, (SA *) & client, sizeof(client)) < 0){
		printf("error in sending Heading\n");
		exit(-1);
	}
	memset(tempStr, '\0', MAX_LEN);
	sprintf(tempStr, "**************************************\n");
	if (sendto(inSock, tempStr, strlen(tempStr), 0, (SA *) & client, sizeof(client)) < 0){
		printf("error in sending *'s\n");
		exit(-1);
	}	
	if (mCount == 0){
		memset(tempStr, '\0', MAX_LEN);
		sprintf(tempStr, "No Multicast Clients Connected\n");
		if (sendto(inSock, tempStr, strlen(tempStr), 0, (SA *) & client, sizeof(client)) < 0){
			printf("error in sending Multicast\n");
			exit(-1);
		}				
	}
	for (i = 0; i<mlist_cntr; i++){
		if (mlist[i].accept > -1){
			memset(tempStr, '\0', MAX_LEN);
			sprintf(tempStr, "%s @ (%s : %d)\n",mlist[i].user,mlist[i].ip, mlist[i].port);
			if (sendto(inSock, tempStr, strlen(tempStr), 0, (SA *) & client, sizeof(client)) < 0){
				printf("error in sending chatters Unicast\n");
				exit(-1);
			}
		}
	}
	memset(tempStr, '\0', MAX_LEN);
	sprintf(tempStr, "**************************************\n\n");
	if (sendto(inSock, tempStr, strlen(tempStr), 0, (SA *) & client, sizeof(client)) < 0){
		printf("error in sending *'s\n");
		exit(-1);
	}
	//SCTP
	memset(tempStr, '\0', MAX_LEN);
	sprintf(tempStr, "List of SCTP Connected Clients\n");
	if (sendto(inSock, tempStr, strlen(tempStr), 0, (SA *) & client, sizeof(client)) < 0){
		printf("error in sending Heading\n");
		exit(-1);
	}	
	memset(tempStr, '\0', MAX_LEN);
	sprintf(tempStr, "**************************************\n");
	if (sendto(inSock, tempStr, strlen(tempStr), 0, (SA *) & client, sizeof(client)) < 0){
		printf("error in sending *'s\n");
		exit(-1);
	}	
	if (sCount == 0){
		memset(tempStr, '\0', MAX_LEN);
		sprintf(tempStr, "No SCTP Clients Connected\n");
		if (sendto(inSock, tempStr, strlen(tempStr), 0, (SA *) & client, sizeof(client)) < 0){
			printf("error in sending Multicast\n");
			exit(-1);
		}			
	}
	for (i = 0; i<slist_cntr; i++){
		if (slist[i].accept > -1){
			memset(tempStr, '\0', MAX_LEN);
			sprintf(tempStr, "%s @ (%s : %d)\n",slist[i].user,slist[i].ip, slist[i].port);
			if (sendto(inSock, tempStr, strlen(tempStr), 0, (SA *) & client, sizeof(client)) < 0){
				printf("error in sending chatters Unicast\n");
				exit(-1);
			}
		}
	}
	memset(tempStr, '\0', MAX_LEN);
	sprintf(tempStr, "**************************************\n\n");
	if (sendto(inSock, tempStr, strlen(tempStr), 0, (SA *) & client, sizeof(client)) < 0){
		printf("error in sending *'s\n");
		exit(-1);
	}
}

void sendBye(struct sockaddr_in client){
	mcastGroupBye.sin_family = AF_INET;
	mcastGroupBye.sin_port = MCastPort;
	mcastGroupBye.sin_addr.s_addr = inet_addr(mcip);	

	// Send information to all connected Unicast TCP clients about the client who is exiting the chat.
	for (i = 0; i<ulist_cntr; i++){
		if (ulist[i].accept > -1){
			client.sin_addr.s_addr = inet_addr(ulist[i].ip);
			client.sin_port = ulist[i].port;
			
			if (sendto(udp_socket_fd, byeMsg, strlen(byeMsg), 0, (SA *) & client, sizeof(client)) < 0){
				printf("Error in sending to Bye Message to Unicast Clients\n");
				exit(-1);
			}			
		}
	}	
		
	// Send information to all connected Unicast SCTP clients about the client who is exiting the chat.
	for (i = 0; i<slist_cntr; i++){
		if (slist[i].accept > -1){
			client.sin_addr.s_addr = inet_addr(slist[i].ip);
			client.sin_port = slist[i].port;
			
			if (sendto(sctp_seq_fd, byeMsg, strlen(byeMsg), 0, (SA *) & client, sizeof(client)) < 0){
				printf("error in sendto 19\n");
				exit(-1);
			}			
		}
	}				

	// Send information to all connected Multicast clients about the client who is exiting the chat.
	if (mlist_cntr>0){
		if (sendto(udp_socket_fd, byeMsg, strlen(byeMsg), 0, (SA *) & mcastGroupBye, sizeof(mcastGroupBye)) < 0){
			printf("error in sendto 20\n");
			exit(-1);
		}			
	}	
}

void joinGroup(int s, char *group){
	struct sockaddr_in groupStruct;
	struct ip_mreq  mreq;	/* multicast group info structure */

	if ((groupStruct.sin_addr.s_addr = inet_addr(group)) == -1)
		printf("error in inet_addr\n");

	/* check if group address is indeed a Class D address */
	mreq.imr_multiaddr = groupStruct.sin_addr;
	mreq.imr_interface.s_addr = INADDR_ANY;

	if (setsockopt(s, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *) &mreq,
		       sizeof(mreq)) == -1) {
		printf("error in joining group \n");
		exit(-1);
	}
}

void leaveGroup(int recvSock, char *group){
	struct sockaddr_in groupStruct;
	struct ip_mreq  dreq;	/* multicast group info structure */

	if ((groupStruct.sin_addr.s_addr = inet_addr(group)) == -1)
		printf("error in inet_addr\n");

	dreq.imr_multiaddr = groupStruct.sin_addr;
	dreq.imr_interface.s_addr = INADDR_ANY;

	if (setsockopt(recvSock, IPPROTO_IP, IP_DROP_MEMBERSHIP,
		       (char *) &dreq, sizeof(dreq)) == -1) {
		printf("error in leaving group \n");
		exit(-1);
	}
	printf("process quitting multicast group %s \n", group);
}

void reusePort(int s){
	int             one = 1;

	if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *) &one, sizeof(one)) == -1) {
		printf("error in setsockopt,SO_REUSEPORT \n");
		exit(-1);
	}
}

void displayDaddr(int s){
	int             one = 1;


        if (setsockopt(s, IPPROTO_IP, IP_PKTINFO, (char *) &one, sizeof(one)) < 0)
                perror("IP_RECVDSTADDR setsockopt error");
}

void setTTLvalue(int s, u_char * ttl_value){
	if (setsockopt(s, IPPROTO_IP, IP_MULTICAST_TTL, (char *) ttl_value,
		       sizeof(u_char)) == -1) {
		printf("error in setting loopback value\n");
	}
}

void setLoopback(int s, u_char loop){
	if (setsockopt(s, IPPROTO_IP, IP_MULTICAST_LOOP, (char *) &loop,
		       sizeof(u_char)) == -1) {
		printf("error in disabling loopback\n");
	}
}

cleanup(buf)
char *buf;
{
  int i;
  for(i=0; i<512; i++) buf[i]=0;
}

