#include <netdb.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "real_address.h"
/* Resolve the resource name to an usable IPv6 address
 * @address: The name to resolve
 * @rval: Where the resulting IPv6 address descriptor should be stored
 * @return: NULL if it succeeded, or a pointer towards
 *          a string describing the error if any.
 *          (const char* means the caller cannot modify or free the return value,
 *           so do not use malloc!)
 */
const char * real_address(const char *address, struct sockaddr_in6 *rval){
	struct addrinfo hints, *res;	
	memset(&hints, 0, sizeof(hints));
	hints.ai_family= AF_INET6;
	hints.ai_socktype= SOCK_DGRAM;
	hints.ai_flags |= AI_PASSIVE;
	if(getaddrinfo(address, NULL, &hints, &res)<0)
		return "getaddrinfo\n";
	rval= (struct sockaddr_in6 *) res;
	if(rval==NULL)
		return "rval\n";
	return NULL;
}
