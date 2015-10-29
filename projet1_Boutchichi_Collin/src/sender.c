#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> 

#include "real_address.h"
#include "create_socket.h"
#include "transfer_sender.h"
#include "wait_for_client.h"

/*void test(char *filename){
	int op= open(filename, O_RDONLY); //| O_NONBLOCK);
	if(op<0){
		fprintf(stderr, "test failed\n");
		return;
	}
	char *buf;
	buf= malloc(sizeof(char));
	ssize_t len= read(op,(void *) buf,sizeof(buf));
	if(len>0)
		write(fileno(stdin),(void *) buf,len);
}*/

int main(int argc, char *argv[]){
	if(argc<3){
		fprintf(stderr, "Parameters missing (hostname or port)\n");
		return EXIT_FAILURE;
	}
	static struct option long_options[]= {
                   {"filename", required_argument, 0, 0},
                   {0, 0, 0, 0}
               };
	int option_index= 0;
	int c, isFile= 0;
	char *filename;
	while(1){
		c= getopt_long_only(argc, argv, "f:", long_options, &option_index);
		if (c == -1)
			break;
		switch (c){
			case 0: //--filename optarg
				filename= optarg;
				isFile= 1;
				break;
			case 'f': //-f optarg
				filename= optarg;
				isFile= 1;
				break;
			case '?':
				fprintf(stderr, "Invalid option. Usage: -f filename\n"
												 "\t\t       --filename filename\n");
				return EXIT_FAILURE; //par securite, afin de ne pas prendre de mauvais arguments
				break;
			default:
				fprintf(stderr, "Usage: -f filename\n"
									 "\t --filename filename\n");
				return EXIT_FAILURE;
				break;
		}
	} //optind = premier argument n'étant pas une option
	//hostname
	struct sockaddr_in6 addr;
	char *host= argv[optind++];
	const char *err= real_address(host, &addr);
	if(err){
		fprintf(stderr, "Could not resolve hostname %s: %s\n", host, err);
		return EXIT_FAILURE;
	}
	//socket
	int sfd;
	int port= atoi(argv[optind++]);
	sfd= create_socket(NULL, -1, &addr, port);
	if(sfd<0){
		fprintf(stderr, "Failed to create the socket!\n");
		return EXIT_FAILURE;
	}
	//chatprintf("Avant la boucle\n"); 
	transfer_sender(sfd, filename, isFile);
	//printf("Close Socket\n");
	close(sfd);
	return EXIT_SUCCESS;
}
