#include <fcntl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>


#include "packet_implem.h"
#include "transfer_sender.h"

#define WINSIZE	32
#define BUF_SIZE 520 //taille maximale des segments envoyes ou recus en octets


/* -------------------------------------------------------------------------------------------------------------------------------------
current_time() renvoit le temps actuel avec une precision de l ordre de la milliseconde 
------------------------------------------------------------------------------------------------------------------------------------- */
long long current_time() {
    struct timeval te; 
    gettimeofday(&te, NULL); // get current time
    long long milliseconds = te.tv_sec*1000LL + te.tv_usec/1000; //  millisecondes
    return milliseconds;
}

/* -------------------------------------------------------------------------------------------------------------------------------------
optimum_rtt() renvoit une valeur optimisee pour le rtt. Celui-ci vaut au depart, 2000 milisec. Au cours de l avancement du programme cette valeur va diminuer pour tendre vers le rtt optimum. 
On tient compte de l'ancienne valeur a hauteur de 60% et de la nouvelle a hauteur de 40% (pour ne pas que des valeurs extremes changent trop la valeur du rtt). 
Ensuite on prend cette moyenne et on la multiplie par 1.1 pour se permettre une marge d erreur de 10%
------------------------------------------------------------------------------------------------------------------------------------- */

long long new_rtt = 2*2000; 
void optimum_rtt(long long timer){
	new_rtt = (0.6 * new_rtt + 0.4 * timer)*1.5;
}
/* -------------------------------------------------------------------------------------------------------------------------------------
send File() contient la boucle while qui permet d envoyer un fichier sur le socket et de lire les ack renvoyes par le receiver.
Il décide des operations qu il va effectuer en fonction de la priorite de la tache.  
------------------------------------------------------------------------------------------------------------------------------------- */
void sendFile(const int sfd, char *filename){
	// OUVERTURE DU FICHIER A LIRE 
	int fichier;
	fichier = open(filename, O_RDONLY,0);
		if(fichier==-1){
			fprintf(stderr, "%s coudn't be opened.\n", filename);
			return;
		}

	// INITIALISATION DE VARIABLES UTILES
	fd_set readfds, writefds; // descripteurs de fichiers
	fcntl(sfd, F_SETFL, fcntl(sfd, F_GETFL, 0) | O_NONBLOCK); // on ne veut pas qu un appel a select puisse bloquer le socket
	char *buffer, *buffer2;
	buffer= (char*) malloc(sizeof(char)*BUF_SIZE);
	buffer2= (char*) malloc(sizeof(char)*BUF_SIZE);

	// ETAT ACTUEL DU SENDER 
	pkt_t *dernier_ack= pkt_new(); // dernier ack recu
		dernier_ack->type = 3;
		dernier_ack->window= 0;
		dernier_ack->seqnum = 0;
	int send_seqnum = 255; // num de sequence du dernier paquet envoye
	int frame = -400; // nombre de paquets que je peux envoyer 
	int congestion = 0; // compte le nombre de paquets ayant subi de la congestion
	int last_paquet = 0; 
	pkt_t** send_paquets = malloc(32*sizeof(pkt_t*));// stocke les paquets envoyes 
	int i; 
	for(i=0; i<=31; i++) send_paquets[i]= pkt_new();
	int nack[32]; 
		int y; 
		for(y=0; y<32; y++){
			nack[y]=0; 
		}
	long long timeout [32]; // stocke l heure a laquelle un paquet a ete envoye
		for(y=0; y<32; y++){
			timeout[y]=0; 
		}


	while(1){
		FD_ZERO(&readfds);
		FD_ZERO(&writefds);
		FD_SET(sfd, &readfds);
		FD_SET(sfd, &writefds);

	/* verifier si on peut utiliser la socket */
	if((select(sfd+1,&readfds,&writefds,NULL,NULL))<0){
		fprintf(stderr, "select()\n");
		close(fichier); 
		return;
	}

	/* Verifier si je peux lire sur le socket */
	if(FD_ISSET(sfd,&readfds)){
	 
		//ssize_t length = read(sfd, (void *) buffer2, 8);
		ssize_t length= read(sfd, (void *) buffer2, 8);
		while (length!=-1) { // il y a qqchose a lire
			pkt_t *acquis = pkt_new(); 
			if(pkt_decode(buffer2, length, acquis)!=PKT_OK){ 
				fprintf(stderr, "pkt_decode()\n");
			}
			printf("acquis reçu : seq_num %d type %d length %d window %d\n", (int)acquis->seqnum, (int)acquis->type, (int)acquis->length, (int)acquis->window); 

			//if(acquis->type==PTYPE_ACK && acquis->seqnum > (send_seqnum+1)%256) break;
	
			/* Si je viens de recevoir un nack */
			if(acquis->type==PTYPE_NACK){
			int indice = (acquis->seqnum % 32); 
			/* detecter la congestion sur le reseau, diminuer la fenetre d envoi s il le faut*/
			congestion++; 
				if(congestion > 2) {
					frame = dernier_ack->window/2; 
				}
			nack[indice] = 1; 
			}

			/* si je recois le paquet de fermeture de connexion */
			if(acquis->type==PTYPE_ACK && last_paquet == 1 && (acquis->seqnum == (send_seqnum+1)%256) ) {
				close(fichier);
				return; 
			}

			/* si je recois un acquis valide qui n est pas une ouverture de connexion */
			if(acquis->type==PTYPE_ACK && frame != -401 ){ 
				if(congestion < 0) congestion--; 
				if(frame < acquis->window) frame++; 
				else if(frame > acquis->window) frame = acquis->window; 
				// statistiques pour ameliorer la veleur du rtt
				long long tmp = current_time() - timeout[(acquis->seqnum -1)%32]; 
				optimum_rtt(tmp); 

				//reinitialiser les timers
				int m; 
				for( m=(acquis->seqnum)%32; m != (dernier_ack->seqnum%32) && send_seqnum < (acquis->seqnum+1); m-- ){
					timeout[m] = 0; 
					pkt_del(send_paquets[m]); 
					if(m==0) m = 32; 
				}
			}
				dernier_ack = acquis; 
 
			/* si je recois le paquet d ouverture de connexion */
			if(acquis->type==PTYPE_ACK && frame == -401) {
				frame = acquis->window;
				send_seqnum = 0; 
				dernier_ack = acquis; // on l enverra plus bas
				// statistiques pour ameliorer la veleur du rtt
				long long tmp = current_time() - timeout[(acquis->seqnum -1)%32]; 
				optimum_rtt(tmp); 
				timeout[(acquis->seqnum -1)%32] = 0;  
			}
		usleep(1000); // on attend 1 (milisec) pour ne pas lire plus vite que ce que les acquis n arrivent
		length= read(sfd, (void *) buffer2, 8);
		}
	}

	/*Si le receiver n'a rien envoyé, je peux envoyer un paquet si le buffer du receiver n'est pas rempli */
	if(FD_ISSET(sfd,&writefds)){
		/* renvoyer les nack*/
		int l; 
		for(l=0;frame > 1 && l<31;l++){
			if( nack[l] == 1){
				char *h; 
				h = malloc(sizeof(char)*520);
				size_t len= send_paquets[l]->length+8;
				if(pkt_encode(send_paquets[l], h, &len)!=PKT_OK){ 
					fprintf(stderr, "pkt_encode()\n");
				}
				write(sfd, (void *) h, len);
				timeout[l] = current_time(); 	
				frame--; 
				nack[l]=0; 
			}
		}

		/* si un timer a expire, je renvoie le paquet */
		int k; 
		for(k=0; k<32; k++){
			if(current_time()-timeout[k] > new_rtt && timeout[k] != 0){
				char *h; 
				h = malloc(sizeof(char)*520);
				size_t len= send_paquets[k]->length+8;
				if(pkt_encode(send_paquets[k], h, &len)!=PKT_OK){ 
					fprintf(stderr, "pkt_encode()\n");
				}
				write(sfd, (void *) h, len);
				timeout[k] = current_time(); 
			}			
		}
 
		/*renvoyer un paquet non recu*/
		if(frame>0 && (dernier_ack->seqnum < (send_seqnum+1)%256)){
				int indice = dernier_ack->seqnum; 
				frame--; 
				char *h; 
				h = malloc(sizeof(char)*520);
				size_t len = send_paquets[indice%32]->length+8;
				if(pkt_encode(send_paquets[indice%32], h, &len)!=PKT_OK){ 
					fprintf(stderr, "pkt_encode()\n");
				}
				write(sfd, (void *) h, len);
				timeout[indice] = current_time(); 
		}

		while((frame > 0 || frame == -400) && last_paquet == 0){
	
			// on avance dans la lecture du fichier
				ssize_t bytes_read = read(fichier, buffer, 512);
				if(bytes_read==0) { //Fichier fini
					pkt_t *paquet= pkt_new();
					send_seqnum = (send_seqnum+1)%256;
					frame--; 
					pkt_set_payload(paquet, 0,0); 
					pkt_set_type(paquet, PTYPE_DATA); 
					pkt_set_window(paquet, 0);
					pkt_set_length(paquet, 0); 
					pkt_set_seqnum(paquet, send_seqnum); 
					last_paquet = 1; 
					char *p;
					p= malloc(sizeof(char)*520); 
					size_t len = 8; 
					if(pkt_encode(paquet, p, &len)!=PKT_OK){ 
						fprintf(stderr, "pkt_encode()\n");
					}
					send_seqnum = paquet->seqnum; 
					write(sfd, (void *) p, len);
					timeout[(send_seqnum)%32] = current_time(); 
					send_paquets[(send_seqnum)%32] = paquet; 

					//usleep(new_rtt*1000); // on attend pour etre sur qu il va attendre de lire les acquis au lieu d en voyer si frame >0 tant qu aucun acquis n est encore arrive	
break;
				}
				if(bytes_read<0){ //Erreur IO
					fprintf(stderr, "read file\n");
					return;
				}
		
				// on stoke les donnees lues dans un paquet
				pkt_t *paquet= pkt_new();
				send_seqnum = (send_seqnum+1)%256;
				frame--; 
				pkt_set_payload(paquet, buffer,(uint16_t) bytes_read); 
				pkt_set_type(paquet, PTYPE_DATA); 
				pkt_set_window(paquet, 0);
				pkt_set_length(paquet, bytes_read); 
				pkt_set_seqnum(paquet, send_seqnum); 
				send_paquets[send_seqnum%32] = paquet; 
				char *p;
				p = malloc(sizeof(char)*520); 
				size_t len = bytes_read+8; 
				if(pkt_encode(paquet, p, (size_t *) &len)!=PKT_OK){ 
					fprintf(stderr, "pkt_encode()\n");
				}
				send_seqnum = paquet->seqnum; 
				write(sfd, (void *) p, len);
				timeout[(send_seqnum)%32] = current_time(); 
				send_paquets[(send_seqnum)%32] = paquet; 
		}
	}
}//end while
close(fichier);
}// end send file


void transfer_sender(const int sfd, char *filename, int option){
	if(option){
		sendFile(sfd, filename);
		}
	/*else{ //Si pas de fichier ---> pas encore fonctionnel
		sendFile(sfd);
		
	}*/
}






