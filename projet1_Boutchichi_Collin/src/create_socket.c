#include <netdb.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "create_socket.h"
/* Creates a socket and initialize it
 * @source_addr: if !NULL, the source address that should be bound to this socket
 * @src_port: if >0, the port on which the socket is listening
 * @dest_addr: if !NULL, the destination address to which the socket should send data
 * @dst_port: if >0, the destination port to which the socket should be connected
 * @return: a file descriptor number representing the socket,
 *         or -1 in case of error (explanation will be printed on stderr)
 */
int create_socket(struct sockaddr_in6 *source_addr, int src_port, struct sockaddr_in6 *dest_addr, int dst_port){
	int newSocket= socket(AF_INET6, SOCK_DGRAM, 0); //descripteur du socket cree
	if(newSocket==-1){
		fprintf(stderr, "socket()\n");
		return -1;
	}
	int err;
	if(source_addr!=NULL){ //serveur
		memset(source_addr, 0, sizeof(*source_addr));
		source_addr->sin6_family= AF_INET6;
		//source_addr->sin6_addr= IN6ADDR_ANY_INIT;
		if(src_port>0)
			source_addr->sin6_port= htons(src_port);
		err= bind(newSocket, (struct sockaddr *) source_addr, sizeof(*source_addr));
		if(err==-1){
			close(newSocket);
			fprintf(stderr, "bind()\n");
			return -1;
		}
	}
	if(dest_addr!=NULL){ //client
		memset(dest_addr, 0, sizeof(*dest_addr));
		dest_addr->sin6_family= AF_INET6;	
		if(dst_port>0)
			dest_addr->sin6_port= htons(dst_port);
		err= connect(newSocket, (struct sockaddr *) dest_addr, sizeof(*dest_addr));
		if(err==-1){
			close(newSocket);
			fprintf(stderr, "connect()");
			return -1;
		}
	}
	return newSocket;
}
