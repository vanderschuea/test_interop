# definition de variables

CC       = gcc  
LD       = gcc
CFLAGS   = -Wall 
LIBS	 = -lz 
PROG1    = sender 
PROG2    = receiver
LISTEOBJ1 = \
create_socket.o real_address.o wait_for_client.o sender.o packet_implem.o transfer_sender.o
LISTEOBJ2 = \
create_socket.o real_address.o wait_for_client.o receiver.o packet_implem.o transfer_receiver.o

# compilation
.c.o :
	$(CC) $(CFLAGS) -o $@ -c $< 

# dependances
all        : $(PROG1) $(PROG2)
create_socket.o    : create_socket.c create_socket.h
real_address.o     : real_address.c real_address.h
wait_for_client.o  : wait_for_client.c wait_for_client.h
sender.o     : sender.c create_socket.h real_address.h wait_for_client.h packet_implem.h transfer_sender.h
receiver.o     : receiver.c create_socket.h real_address.h wait_for_client.h packet_implem.h transfer_receiver.h 
packet_implem.o	: packet_implem.c packet_implem.h 

# edition de lien
$(PROG1)    : $(LISTEOBJ1) 
	$(LD) -o $(PROG1) $(LISTEOBJ1) $(LIBS)

$(PROG2)    : $(LISTEOBJ2) 
	$(LD) -o $(PROG2) $(LISTEOBJ2) $(LIBS)

