CPP      = g++
CC       = gcc
OBJ      = client.o server.o sockets.o
LINKOBJ  = server.o client.o sockets.o
LIBS     = 
INCS     = 
CXXINCS  = -I"../sockets-lumify"
CXXFLAGS = $(CXXINCS) -std=c++11 -g3 -ggdb3
CFLAGS   = $(INCS) -std=c11 -ggdb3
RM       = rm -f

.PHONY: all all-before all-after clean clean-custom client server

all: clean client server 

tcp: clean tcp-client tcp-server

clean: clean-custom
	${RM} $(OBJ) client server clientudp serverudp

tcp-client: client.o sockets.o
	$(CPP) client.o sockets.o -o tcp-client $(LIBS)

tcp-server: server.o sockets.o
	$(CPP) server.o sockets.o -o tcp-server $(LIBS)

client.o:
	$(CPP) -c tcp/client.cpp -o client.o $(CXXFLAGS)

server.o:
	$(CPP) -c tcp/server.cpp -o server.o $(CXXFLAGS)

sockets.o: 
	$(CPP) -c ../sockets-lumify/sockets.cpp -o sockets.o $(CXXFLAGS)