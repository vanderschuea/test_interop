#include <netdb.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "wait_for_client.h"

/* Block the caller until a message is received on sfd,
 * and connect the socket to the source addresse of the received message.
 * @sfd: a file descriptor to a bound socket but not yet connected
 * @return: 0 in case of success, -1 otherwise
 * @POST: This call is idempotent, it does not 'consume' the data of the message,
 * and could be repeated several times blocking only at the first call.
 */
int wait_for_client(int sfd){
	struct sockaddr_in6 client;
	socklen_t addrlen= sizeof(client);
	//ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklent_t *addrlen);
	if((recvfrom(sfd, NULL, 0, MSG_PEEK, (struct sockaddr *) &client, &addrlen))==-1){
		fprintf(stderr, "recvfrom()\n");
		return -1;
	}
	int err= connect(sfd, (struct sockaddr *) &client, addrlen);//msglen);
	if(err!=0){
		fprintf(stderr, "connect()\n");
		return -1;
	}
	return 0;
}
