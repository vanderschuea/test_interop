#ifndef __TRANSFER_SENDER_H_
#define __TRANSFER_SENDER_H_

void sendFile(const int sfd, char *filename);

void transfer_sender(const int sfd, char *filename, int option);

#endif
