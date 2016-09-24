/*************************************************************/
/* Chanakya Pallapolu				        				 */
/* 															 */
/* Implementation: Using select 							 */
/* Usage:  SUMChatClient <shost> <sport> <s|u|m>			 */
/*************************************************************/ 

#include "client.h"


void exit(int);
void selectCall();
void connectServer();
void sendSctp();
void sendUnicast();
void sendMulticast();
void sendMessage(int inSock, int outSock, int identity);
void recvMessage(int inSock, int outSock);
void recvList(int inSock, int outSock);
void intHandler(int);	
void intHandler2(int);	

struct sockaddr_in LocalHost;
struct sockaddr_in tcpSock;
struct sockaddr_in tcp_server;
struct sockaddr_in client;
struct  hostent *hp, *gethostbyname();
struct sockaddr_in from;
struct sockaddr_in addr;
struct sockaddr_in GroupAddress;

fd_set assignedFDSet, readFDSet;

int fromlen, length;
int pres_cli,pres_cli2;
int recv_spres_cli,pres_cli3;
int i;
int recvCount = 0;
int identity=0, rc, tcp_socket_fd, u=0;;
int udp_socket_fd, UDPport, udp_sock_recv, udp_sock_send, sctp_seq_fd;
int conn_fd;

int recv_upres_cli,recv_mpres_cli;
int bytes = 0;


char sendBuf[MAX_LEN];
char ThisHost[80];
char num_cli[1024],num_cli2[1024],num_cli3[1024];
char chatBuf[1024];
char chatBuf1[1024];
char chatBuf3[1024];
char *username;
char buf[BUFSIZE];
char rbuf[BUFSIZE];
char listcode[10], endcode[10];
char mcastip[20];
char mcastGreeting[100] = "I am a NEW MULTICAST Client\n";
char *client_type;

u_char TimeToLive;
u_char loop;

main(int argc, char *argv[]){


    signal(SIGINT, intHandler);
    signal(SIGQUIT, intHandler);
    signal(SIGPIPE, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);	
	signal(SIGTERM, SIG_IGN);


	if(argc!=4){
		printf("Usage: SUMChatClient <host> <port> <u|m|s>\n");
		exit(-1);
	}

	client_type = argv[3];

	username = getenv("USER");

	printf("\n");
	printf("********************************************************************\n");

	printf("Welcome! %s\n",username);

	hp = gethostbyname(argv[1]);

	tcp_server.sin_family = AF_INET;
	tcp_server.sin_port = htons(atoi(argv[2]));	
	bcopy ( hp->h_addr, &(tcp_server.sin_addr.s_addr), hp->h_length);

	tcp_socket_fd = socket(AF_INET,SOCK_STREAM,0);

	if (tcp_socket_fd < 0){
		perror("Error in creating TCP Socket");
	}
	
	if (strcmp(client_type,"s")==0){
		if ( (tcp_socket_fd = socket (AF_INET,SOCK_STREAM,IPPROTO_SCTP)) < 0){
			perror("opening SCTP stream socket");
			exit(-1);
		}		
	}
	else{
		if ( (tcp_socket_fd = socket (AF_INET,SOCK_STREAM,0)) < 0){
			perror("opening TCP stream socket");
			exit(-1);
		}		
	}
	reusePort(tcp_socket_fd);	

	conn_fd = connect(tcp_socket_fd, (SA *)&tcp_server,sizeof(tcp_server));

	if (conn_fd < 0){
		perror("Error in connecting to Server");
	}

	/** get port information of the client and  prints it out */
	length = sizeof(client);
	client.sin_family = AF_INET;
	client.sin_addr.s_addr = htonl(INADDR_ANY);
	if ( getsockname (tcp_socket_fd, (SA *)&client,&length)  && client.sin_family == AF_INET && length == sizeof(client)) 
	{
		perror("getting socket name");
		exit(0);
	}
	tcpSock.sin_port = client.sin_port;	

	printf("\n");
	if(strcmp(client_type,"s")==0){
		printf("Client Port is : %d\n\n", ntohs(client.sin_port)+1);  	
	}
	else {
		printf("Client Port is : %d\n\n", ntohs(client.sin_port));  	
	}
		
	printf("Connected to Server: %s : %d\n",  inet_ntoa(tcp_server.sin_addr),ntohs(tcp_server.sin_port));
    printf("Name is : %s@cs.odu.edu\n\n",argv[1]);	

	if( (rc=send(tcp_socket_fd,username,strlen(username),0)) < 0){
		perror("sending stream  message");
		exit(-1);
	}
	usleep(5000);
	
	// Check if the type of client is Unicast or Multicast and send '0' or '1' accordingly.
	if (strcmp(client_type,"u")==0){
		if( (rc=send(tcp_socket_fd,"1",1,0)) < 0){
			perror("sending stream  message");
			exit(-1);
		}
		identity = 1;
	}
	else if (strcmp(client_type,"m")==0){
		if((rc=send(tcp_socket_fd,"0",1,0)) < 0){
			perror("sending stream  message");
			exit(-1);
		}		
		identity = 0;
	}	
	else{
		identity = 3;
	}
		
	FD_ZERO (&assignedFDSet);
	FD_SET (0, &assignedFDSet);	
	FD_SET (tcp_socket_fd, &assignedFDSet);	
	selectCall();
}
	
// Select() is used to handle all the asynchronous I/Os. 
// This includes TCP listener socket, TCP Comm socket and UDP comm socket.	
void selectCall(){
	int activeReads;
	for(;;){

		memcpy(&readFDSet, &assignedFDSet, sizeof(assignedFDSet));
		activeReads = select(FD_SETSIZE, &readFDSet, NULL, NULL, NULL);
		//printf("FD_SETSIZE is %d\n", FD_SETSIZE);
		//printf("select returns %d\n", activeReads);
		if (activeReads < 0){
			perror ("Error in select call\n");
			exit(-1);
        }	
		
		for (i = 0; i< FD_SETSIZE; i++){
			if (FD_ISSET(i, &readFDSet)){
				if (i == tcp_socket_fd){
					connectServer();
				}
				else if (i == udp_socket_fd && identity == 1){
					recvMessage(udp_socket_fd, 1);
				}
				else if (i == sctp_seq_fd && identity == 3){
					recvMessage(sctp_seq_fd, 1);
				}				
				else if (i == 0 && identity == 1){
					sendMessage(0, udp_socket_fd, 1);
				}
				else if (i == 0 && identity == 3){
					sendMessage(0, sctp_seq_fd, 1);
				}				
				else if (i == udp_sock_recv && identity == 0){
					recvMessage(udp_sock_recv, 1);
				}
				else if (i == udp_sock_send && identity == 0){
					recvList(udp_sock_send, 1);
				}
				else if (i == 0 && identity == 0){
					sendMessage(0, udp_sock_send, 0);
				}
			}
		}
	}
}

void connectServer(){
	fflush(stdout);
	cleanup(rbuf);

	if( (rc=recv(tcp_socket_fd, rbuf, sizeof(rbuf), 0)) < 0){
		perror("receiving stream  message ");
		exit(-1);
	}

	if (rc > 0){
		recvCount++;		

		if (identity == 0){
			if (recvCount == 1){
				strcpy(mcastip,rbuf);
			}
			else if (recvCount == 2){
				UDPport = htons(atoi(rbuf));
			}
			else if (recvCount == 3){
				strcpy(listcode,rbuf);
			}
			else if (recvCount == 4){
				strcpy(endcode,rbuf);
				printf("Received Multicast IP Address: %s\n",mcastip);
				printf("Received Multicast Port Number: %d\n",ntohs(UDPport));
				printf("Received List Code: %s\n",listcode);
				printf("Received End code: %s\n",endcode);

				printf("\n\n");
			}			
		}		
		else{
			if (recvCount == 1){
				UDPport = htons(atoi(rbuf));
			}
			else if (recvCount == 2){
				strcpy(listcode,rbuf);
			}
			else if (recvCount == 3){
				strcpy(endcode,rbuf); 
				printf("Received Multicast Port Number: %d\n",ntohs(UDPport));
				printf("Received List Code: %s\n",listcode);
				printf("Received End code: %s\n",endcode); 
				printf("\n\n");                                      
				
			}
		}
	}
	else {
		printf("SUMChatServer Disconnected. No more Chat Available\n");
		exit(0);
	}
	
	if (recvCount == 4 && identity == 0){
		sendMulticast();		
	}
	else if (recvCount == 3 && identity == 1){
		sendUnicast();	
	}
	else if (recvCount == 3 && identity == 3){
		sendSctp();
	}
}

void sendSctp(){

	LocalHost.sin_family = AF_INET;
	LocalHost.sin_port = htons(tcpSock.sin_port)+1;
	LocalHost.sin_addr.s_addr = htonl(INADDR_ANY);
	
	char helloMsg[MAX_LEN];
	memset(helloMsg, '\0', MAX_LEN);	

	// Create a single UDP Socket for sending & receiving chat messages

	
	if ((sctp_seq_fd = socket(AF_INET, SOCK_SEQPACKET,IPPROTO_SCTP)) < 0) {
		printf("can't create UDP socket: \n");
		exit(-1);
	}
	reusePort(sctp_seq_fd);

	// bind this UDP socket to local IP address and the TCPport that was used in the TCP socket.	
	if (bind(sctp_seq_fd, (SA *) & LocalHost, sizeof(LocalHost)) < 0) {

		printf("error in sctp_seq_fd bind\n");
		exit(-1);
	}
	listen(sctp_seq_fd, 1);
	FD_SET (sctp_seq_fd, &assignedFDSet);	

	// send a Hello Message to the server which will then forward it to all OTHER clients.
	tcp_server.sin_family = AF_INET;
	tcp_server.sin_port = UDPport;
	sprintf(helloMsg, "I am a NEW SCTP Client\n");
	if (sendto(sctp_seq_fd, helloMsg, strlen(helloMsg), 0, (SA *) & tcp_server, sizeof(tcp_server)) < 0) 
	{
		printf("error in sending sctp greeting\n");
		exit(-1);
	}

	printf("********************************************************************\n");

	printf("\nType in your message to be sent, followed by ENTER.\n");
	printf("Hit CTRL+D or CTRL+\\ to quit from the group \n\n");
}

void sendUnicast(){

	
	LocalHost.sin_family = AF_INET;
	LocalHost.sin_port = htons(tcpSock.sin_port);
	LocalHost.sin_addr.s_addr = htonl(INADDR_ANY);
	
	char helloMsg[MAX_LEN];
	memset(helloMsg, '\0', MAX_LEN);	

	// Create a single UDP Socket for sending & receiving chat messages

	
	if ((udp_socket_fd= socket(AF_INET, SOCK_DGRAM,0)) < 0) {
		printf("can't create UDP socket: \n");
		exit(-1);
	}
	reusePort(udp_socket_fd);

	// bind this UDP socket to local IP address and the TCPport that was used in the TCP socket.	
	if (bind(udp_socket_fd, (SA *) & LocalHost, sizeof(LocalHost)) < 0) {

		printf("error in TCP UDP Socket bind\n");
		exit(-1);
	}
	listen(udp_socket_fd, 1);
	FD_SET (udp_socket_fd, &assignedFDSet);	

	// send a Hello Message to the server which will then forward it to all OTHER clients.
	tcp_server.sin_family = AF_INET;
	tcp_server.sin_port = UDPport;
	sprintf(helloMsg, "I am a NEW UNICAST Client\n");
	if (sendto(udp_socket_fd, helloMsg, strlen(helloMsg), 0, (SA *) & tcp_server, sizeof(tcp_server)) < 0){
		printf("error in sending weolcome message from unicast\n");
		exit(-1);
	}
	printf("********************************************************************\n");

	printf("\nType in your message to be sent, followed by ENTER.\n");
	printf("Hit CTRL+D or CTRL+\\ to quit from the group \n\n");
}

void sendMulticast(){
	
	
	LocalHost.sin_family = AF_INET;
	LocalHost.sin_port = UDPport;
	LocalHost.sin_addr.s_addr = htonl(INADDR_ANY);
	
	char helloMsg[MAX_LEN];
	memset(helloMsg, '\0', MAX_LEN);		

	// Create a UDP Socket to receive messages.
	if ((udp_sock_recv = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		printf("can't create UDP socket: \n");
		exit(-1);
	}
	reusePort(udp_sock_recv);

	// bind the Receive UDP socket to local IP and Multicast Port.
	if (bind(udp_sock_recv, (SA *) & LocalHost, sizeof(LocalHost)) < 0) {

		printf("error in bind\n");
		exit(-1);
	}

	LocalHost.sin_port = htons(tcpSock.sin_port);

	// Create a second UDP Socket to send messages.
	if ((udp_sock_send = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		printf("can't create UDP socket: \n");
		exit(-1);
	}
	reusePort(udp_sock_send);

	// bind the send UDP socket to local IP and the TCPport that was used for local TCP socket.
	if (bind(udp_sock_send, (SA *) & LocalHost, sizeof(LocalHost)) < 0) {

		printf("error in bind\n");
		exit(-1);
	}		

	FD_SET (udp_sock_recv, &assignedFDSet);	
	FD_SET (udp_sock_send, &assignedFDSet);		
	/** allow multicast datagrams to be transmitted to a distance
			according to the value put in "TimeToLive" variable */

	TimeToLive = 2;
	setTTLvalue(udp_sock_recv, &TimeToLive);

	loop = 1;		/* enable loopback */
	setLoopback(udp_sock_recv, loop);

	joinGroup(udp_sock_recv, mcastip);
	
	// send a Hello Message to the server which will then forward it to all OTHER clients.		

	tcp_server.sin_family = AF_INET;
	tcp_server.sin_port = UDPport;
	if (sendto(udp_sock_send, mcastGreeting, strlen(mcastGreeting), 0, (SA *) & tcp_server, sizeof(tcp_server)) < 0){
		printf("error in sending Mcast greeting\n");
		exit(-1);
	}

	printf("********************************************************************\n");

	printf("\nType in your message to be sent, followed by ENTER.\n");
	printf("Hit CTRL+D or CTRL+\\ to quit from the group \n\n");		
}

void recvMessage(int inSock, int outSock){
	int             bytes = 0;
	char            recvBuf[1024];
	struct   sockaddr_in client;
	int len = sizeof(client);


	memset(recvBuf, '\0', MAX_LEN);
	bytes = recvfrom(inSock, recvBuf, MAX_LEN, 0, (struct sockaddr *)&client, (socklen_t *)&len);
	//printf("Received message------------->%s\n",recvBuf);
	if (bytes < 0){
		printf("error in reading from multicast socket\n");
		exit(0);
	} 
	else{	
		// if an End code was received from the server, then kill all child processes and exit.
		if (strcmp(recvBuf, endcode)==0){
			printf("Server disconnected. No more Chat!!\n");
			exit(0);
		}
		else{
			fflush(stdout);

			/* print the message to STDOUT */
			if (write(outSock, recvBuf, MAX_LEN) < 0){
				printf("error in write to STDOUT \n");
				exit(-1);
			}
		}
	}
}

void recvList(int inSock, int outSock){
	int             bytes = 0;
	char            recvBuf[MAX_LEN];
	struct   sockaddr_in client;
	int len = sizeof(client);

	memset(recvBuf, '\0', MAX_LEN);
	bytes = recvfrom(inSock, recvBuf, MAX_LEN, 0, (struct sockaddr *)&client, (socklen_t *)&len);
	//printf("Received message==================>%s\n",recvBuf);
	if (bytes < 0){
		printf("error in reading from socket\n");
		exit(-1);
	} 
	else{	
		// if an End code was received from the server, then kill all child processes and exit.		
		if (strcmp(recvBuf, endcode)==0){
			printf(">>>>> %s <<<<<\n", recvBuf);
			printf("Server Disconnected. No More Chat!!\n");		
			exit(0);
		}
		else{
			fflush(stdout);

			/* print the message to STDOUT */
			if (write(outSock, recvBuf, MAX_LEN) < 0) 
			{
				printf("error in write to STDOUT \n");
				exit(-1);
			}
		}
	}
}

void sendMessage(int inSock, int outSock, int identity){

	int socket;

	GroupAddress.sin_family = AF_INET;
	GroupAddress.sin_port = UDPport;
	GroupAddress.sin_addr.s_addr = inet_addr(mcastip);

	memset(sendBuf, '\0', MAX_LEN);
	bytes = read(inSock, sendBuf, MAX_LEN);

	if (bytes < 0){
		printf("error in reading from STDIN \n");
		exit(-1);
	} 
	// Handling CTRL+D
	// Send End Code to the server, kill Parent Process and exit.
	else if (bytes == 0) 
	{
		tcp_server.sin_family = AF_INET;
		tcp_server.sin_port = UDPport;
		if (udp_socket_fd == 0)
			socket = udp_sock_send;
		else
			socket = udp_socket_fd;
		if (sendto(socket, endcode, sizeof(endcode), 0, (SA *) & tcp_server, sizeof(tcp_server)) < 0) 
		{
			printf("error in sending endcode to server\n");
			exit(-1);
		}			
		//kill(getptid(), 9);
		exit(0);
	} 
	else 
	{
		// if the Client is a Multicast client, send the chat message to the Multicast Group
		// The server will then forward the message to all the Unicast clients
		/*if (identity==0)
		{
			if (sendto(outSock, sendBuf, MAX_LEN, 0, (SA *) & GroupAddress, sizeof(GroupAddress)) < 0) 
			{
				printf("error in sendto 04\n");
				exit(-1);
			}
		}*/
		// if the Client is a Unicast client, send the chat message to the Server
		// the server will then forward the message to the Multicast clients and the OTHER unicast clients
		/*else*/
		{
			tcp_server.sin_family = AF_INET;
			tcp_server.sin_port = UDPport;
			if (sendto(outSock, sendBuf, MAX_LEN, 0, (SA *) & tcp_server, sizeof(tcp_server)) < 0) 
			{
				printf("error in sending message to server\n");
				exit(-1);
			}			
		}
	}
}

void intHandler(int sig){
	signal(sig, intHandler);
	int socket;

	// CTRL+C SIGINT interrupt intHandler
	
	if(sig == SIGINT)
	{
		tcp_server.sin_family = AF_INET;
		tcp_server.sin_port = UDPport;
		
		if (identity == 0){
			socket = udp_sock_send;
		}
		else if (identity == 1){
			socket = udp_socket_fd;
		}
		else if (identity == 3){
			socket = sctp_seq_fd;
		}

		// Send L code to the UMChat Server who in turn will send the list of all Unicast & Multicast clients
		if (sendto(socket, listcode, sizeof(listcode), 0, (SA *) & tcp_server, sizeof(tcp_server)) < 0){
			printf("error in sending ListCode to server\n");
			exit(-1);
		}	
		printf("\n\n");
		/*usleep(500);
		//printf("hi-------------->\n");

		recv_upres_cli = recv(socket,num_cli,1024,0);
		//pres_cli = atoi(num_cli);
		printf("U No. of clients :*%s*\n",num_cli);*/

		/*if(pres_cli == 0){
			printf("List of Unicast Connected Clients\n");
			printf("**************************************\n");
			printf("No UniCast Clients Connected\n");
			printf("**************************************\n");
		}
		else if(pres_cli > 0){
			printf("List of Unicast Connected Clients\n");
			printf("**************************************\n");
	       	for(i=0;i<pres_cli;i++){
				recv_upres_cli = recv(socket,chatBuf,sizeof(chatBuf),0);
				printf("%s\n",chatBuf);
			}		
			//printf("\n");
			printf("**************************************\n");
		}

		printf("\n");

		memset(chatBuf1, '\0', MAX_LEN);
		recv_mpres_cli = recv(socket,num_cli2,sizeof(num_cli2),0);
		pres_cli2 = atoi(num_cli2);
		//printf("M No. of clients : %d  *%s*\n", pres_cli2, num_cli2);

		if(pres_cli2 == 0){
			printf("List of Multicast Connected Clients\n");
			printf("**************************************\n");
			printf("No MultiCast Clients Connected\n");
			printf("**************************************\n");
		}
		else if(pres_cli2 > 0){
			printf("List of Multicast Connected Clients\n");
			printf("**************************************\n");
		   	for(i=0;i<pres_cli2;i++){
				recv_mpres_cli = recv(socket,chatBuf1,sizeof(chatBuf1),0);
				printf("%s\n",chatBuf1);		
			}	
			//printf("\n");		
			printf("**************************************\n");		
		}

		printf("\n");
		//printf("about to print sctp list - CLIENT SIDE MSG\n");
		memset(chatBuf3, '\0', MAX_LEN);
		recv_spres_cli = recv(socket,num_cli3,sizeof(num_cli3),0);
		pres_cli3 = atoi(num_cli3);
		//printf("S No. of clients : %d  *%s*\n", pres_cli3, num_cli3);

		if(pres_cli3 == 0){
			printf("List of SCTP Connected Clients\n");
			printf("**************************************\n");
			printf("No SCTP Clients Connected\n");
			printf("**************************************\n");
		}
		else if(pres_cli3 > 0){
			printf("List of SCTP Connected Clients\n");
			printf("**************************************\n");
		   	for(i=0;i<pres_cli3;i++){
				recv_spres_cli = recv(socket,chatBuf3,sizeof(chatBuf3),0);
				printf("%s\n",chatBuf3);		
			}	
					
			printf("**************************************\n");		
		}
		printf("\n");
*/
		selectCall();
	} 

	else if(sig == SIGQUIT)
	{
		tcp_server.sin_family = AF_INET;
		tcp_server.sin_port = UDPport;
		if (identity == 0){
			socket = udp_sock_send;
		}
		else if (identity == 1){
			socket = udp_socket_fd;
		}
		else if (identity == 3){
			socket = sctp_seq_fd;
		}

		if (sendto(socket, endcode, sizeof(endcode), 0, (SA *) & tcp_server, sizeof(tcp_server)) < 0){
			printf("error in sending end code to server\n");
			exit(-1);
		}	
		exit(0);
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
  for(i=0; i<BUFSIZE; i++) buf[i]=0;
}