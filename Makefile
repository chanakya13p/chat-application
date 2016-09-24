CC              =       gcc 
SOLARISHOME     = /usr/dt
RM              = rm -rf 

# libraries and includes
CFLAGS          = $(NORMCFLAGS)
#Solaris
#LIBS       = -lsocket -lnsl 
LIBS       = -lpthread -lnsl  -lm -lsctp

PROGS = SUMChatClient SUMChatServer 

all: $(PROGS)

SUMChatClient:  SUMChatClient.o
	$(CC) $(CFLAGS) -o SUMChatClient SUMChatClient.c $(LIBS)
SUMChatServer:  SUMChatServer.o
	$(CC) $(CFLAGS) -o SUMChatServer SUMChatServer.c $(LIBS)
clean:;
	rm -f $(PROGS) core *.o
