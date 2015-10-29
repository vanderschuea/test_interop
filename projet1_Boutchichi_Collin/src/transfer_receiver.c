#include <fcntl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <time.h>

#include "packet_implem.h"
#include "transfer_receiver.h"

#define WINSIZE	32 //taille de la fenetre
#define BUFSIZE 520 //taille maximale des segments envoyes ou recus en octets
#define MAXRTT 4 //2*2 sec de latence max

#include <sys/time.h>

void waitFor (unsigned int secs){
    unsigned int retTime= time(0) + secs;
    while (time(0) < retTime);
}

double getRtt(struct timeval rtt){
	struct timeval curTime, res;
	struct timezone zone;
	gettimeofday(&curTime, &zone);
	timersub(&curTime, &rtt, &res);
	return res.tv_sec;
}
void getFile(const int sfd, char *filename, int out){
	//Ouverture fichier pour ecriture
	int fichier;
	if(out)
		fichier= fileno(stdin);
	else{
		fichier= open(filename, O_WRONLY|O_CREAT|O_APPEND|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
		if(fichier==-1){
			fprintf(stderr, "%s coudn't be opened.\n", filename);
			return;
		}
	}
	//Initialisations
	fd_set readfds,writefds;
	fcntl(sfd, F_SETFL, fcntl(sfd, F_GETFL, 0) | O_NONBLOCK);
	char *buffer;
	struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    pkt_t **stock;
    stock= malloc(sizeof(pkt_t)*WINSIZE);
    if(stock==NULL){
    	fprintf(stderr, "window allocation\n");
    	if(out!=1)
    		close(fichier);
    	return;
    }
    int k;
	for(k=0; k<=WINSIZE; k++)
		stock[k]= pkt_new();
    uint8_t seq= 0, win= (uint8_t) WINSIZE-1;
    int ack= 0, err= 0, gotEof= 0;
	ssize_t bytes_read= 0;
	pkt_t *acquis;
	struct timeval rtt;
	struct timezone zone;
	while(1){
		int ret= 0;
		FD_ZERO(&readfds);
		FD_ZERO(&writefds);
		FD_SET(sfd, &readfds);
		FD_SET(sfd, &writefds);
		if((ret=select(sfd+1,&readfds,&writefds,NULL,&timeout))<0){
			fprintf(stderr, "select()\n");
			if(out!=1)
				close(fichier);
			return;
		}
		double tmp= getRtt(rtt);
		if(gotEof && win==WINSIZE-1 && tmp>MAXRTT) 
			return;
		acquis= pkt_new();
		//Socket vers fichier
		if(FD_ISSET(sfd,&readfds) && win>=0){ //Paquet recu
			buffer= malloc(sizeof(char)*BUFSIZE);
			if(buffer==NULL){
				fprintf(stderr, "buffer allocation\n");
			}
			bytes_read= read(sfd, (void *) buffer, BUFSIZE);
			if(bytes_read==0){ //Fin. si buffer encore rempli, attend d'avoir tout recu
				if(win==WINSIZE-1)
					return;
				gotEof= 1;
			}
			else if(bytes_read<0){
				fprintf(stderr, "read(socket)\n");
			}
			else{
				pkt_t *paquet= pkt_new();
				pkt_status_code state= pkt_decode(buffer, bytes_read, paquet);
				if(state!=PKT_OK){ //normalement decode contient le header si != E_NOHEADER
					fprintf(stderr, "corrupted packet\n");
					if(bytes_read<=4 && state!=E_NOHEADER){ //Congestion
						//pkt_copy(acquis, paquet);
						acquis= paquet;
						acquis->type= PTYPE_NACK;
						acquis->window= win;
						ack= 2;
					} else //n'envoie rien
						pkt_del(paquet);
				} else{
					if(paquet->length==0)
						gotEof= 1;
					if(paquet->seqnum==seq){ //bon paquet recu
						err= write(fichier, (void *) paquet->payload, paquet->length);
						if(err==-1){
							fprintf(stderr, "write(fichier)\n");
							return;
						}
						seq++;
						pkt_del(paquet);
					} else{ //stocke le paquet en attendant le bon
						stock[paquet->seqnum%WINSIZE]= paquet;
						win--;
					}
					ack= 1;
				}
			}
			free(buffer);
		}
		// vider le buffer contenant les paquets recus et ecrire le contenu dans un fichier SI le paquet a le bon numero de sequence
		int j, nomiss= 1;
		for(j=seq%WINSIZE; j<WINSIZE && nomiss; j= (j+1)%WINSIZE){
			if(stock[j]->payload!=NULL){
				err= write(fichier, (void *) stock[j]->payload, stock[j]->length);
				if(err==-1){
					fprintf(stderr, "write(fichier)\n");
					return;
				}
				fprintf(stderr, "\tn°%d ecrit.\n", stock[j]->seqnum);
				pkt_del(stock[j]);
				stock[j]= pkt_new();
				seq++;
				win++;
			} else nomiss= 0;
		}
		//Confirmation vers Socket
		if(FD_ISSET(sfd,&writefds) && ack>0){ //Envoi acquis
			if(ack){ //acquis cumulatif
				acquis->type= PTYPE_ACK;
				acquis->window= win;
				acquis->seqnum= seq;
			}
			char *p;
			p= malloc(bytes_read*sizeof(char));
			pkt_encode(acquis, p, (size_t *) &bytes_read);
			write(sfd, (void *) p, bytes_read);
			pkt_del(acquis);
			if(gotEof && win==WINSIZE-1) //dernier acquis à envoyer
				gettimeofday(&rtt,&zone);
		}
		ack= 0;
		waitFor(0);
	}
	if(out)
		close(fichier);
	return;
}

void transfer_receiver(const int sfd, char *filename, int option){
	if(option)
		getFile(sfd, filename, 0);
	else
		getFile(sfd, filename, 1);
}
