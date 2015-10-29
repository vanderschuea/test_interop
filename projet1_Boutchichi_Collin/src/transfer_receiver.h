#ifndef __TRANSFER_RECEIVER_H_
#define __TRANSFER_RECEIVER_H_

void getFile(const int sfd, char *filename, int in);

void transfer_receiver(const int sfd, char *filename, int option);


#endif
