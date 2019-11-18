/* udp_server.c */
/* Programmed by Adarsh Sethi */
/* Sept. 19, 2019 */

#include <ctype.h>      /* for toupper */
#include <stdio.h>      /* for standard I/O functions */
#include <stdlib.h>     /* for exit */
#include <string.h>     /* for memset */
#include <sys/socket.h> /* for socket, sendto, and recvfrom */
#include <netinet/in.h> /* for sockaddr_in */
#include <unistd.h>     /* for close */
#include "packet.h"

#define STRING_SIZE 1024
#define BUFFERSIZE 81

/* SERV_UDP_PORT is the port number on which the server listens for
   incoming messages from clients. You should change this to a different
   number to prevent conflicts with others in the class. */

#define SERV_UDP_PORT 65332

int main(void)
{

   int sock_server; /* Socket on which server listens to clients */

   struct sockaddr_in server_addr; /* Internet address structure that
                                        stores server address */
   unsigned short server_port;     /* Port number used by server (local port) */

   struct sockaddr_in client_addr; /* Internet address structure that
                                        stores client address */
   unsigned int client_addr_len;   /* Length of client address structure */

   char sentence[STRING_SIZE];         /* receive message */
   char filename[STRING_SIZE];         /* recieve file*/
   char modifiedSentence[STRING_SIZE]; /* send message */
   unsigned int msg_len;               /* length of message */
   int bytes_sent, bytes_recd;         /* number of bytes sent or received */
   unsigned int i;                     /* temporary loop variable */

   /* open a socket */

   if ((sock_server = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
   {
      perror("Server: can't open datagram socket\n");
      exit(1);
   }

   /* initialize server address information */

   memset(&server_addr, 0, sizeof(server_addr));
   server_addr.sin_family = AF_INET;
   server_addr.sin_addr.s_addr = htonl(INADDR_ANY); /* This allows choice of
                                        any host interface, if more than one
                                        are present */
   server_port = SERV_UDP_PORT;                     /* Server will listen on this port */
   server_addr.sin_port = htons(server_port);

   /* bind the socket to the local server port */

   if (bind(sock_server, (struct sockaddr *)&server_addr,
            sizeof(server_addr)) < 0)
   {
      perror("Server: can't bind to local address\n");
      close(sock_server);
      exit(1);
   }

   /* wait for incoming messages in an indefinite loop */

   printf("Waiting for incoming messages on port %hu\n\n",
          server_port);

   client_addr_len = sizeof(client_addr);

   struct packet *file_pkt = malloc(sizeof(struct packet));

   for (;;)
   {

      bytes_recd = recvfrom(sock_server, file_pkt, sizeof(struct packet), 0,
                            (struct sockaddr *)&client_addr, &client_addr_len);

      strcpy(filename, file_pkt->data);

      if (bytes_recd > 0)
      {
         printf("Received file is: ");
         printf("%s\n", filename);

         //prepare to send the packets to client

         FILE *received_file = fopen(filename, "r"); /* attempt to open the file */
         short validFile = 1;                        //flag to know if file is found or not - sends back to client
         while (received_file == NULL)
         {
            printf("Unable to find file\n");
            validFile = 0;
            short flag = htons(validFile);
            bytes_sent = sendto(sock_server, &flag, sizeof(short), 0,
                                (struct sockaddr *)&client_addr, client_addr_len); //send to client flag that file wasn't found
            bytes_recd = recvfrom(sock_server, filename, STRING_SIZE, 0,
                                  (struct sockaddr *)&client_addr, &client_addr_len); //receive new file from client

            if (bytes_recd <= 0)
            { //check for packet error
               perror("Packet error");
            }

            received_file = fopen(filename, "r"); //attempt to open new file
         }
         validFile = 1;
         short flag = htons(validFile);
         bytes_sent = sendto(sock_server, &flag, sizeof(short), 0,
                             (struct sockaddr *)&client_addr, client_addr_len); //send to client flag that file was found.

         struct packet *pkt = malloc(sizeof(struct packet)); //allocate space for packet struct
         struct ack *ACK_pkt = malloc(sizeof(struct ack));
         //char *data;
         char buffer[BUFFERSIZE]; //to read data segment from file

         int sequence = 0; //keep track of packets
         int packetsSent = 0;
         int bytes_trans = 0; //keep track of data bytes

         while (fgets(buffer, BUFFERSIZE - 1, received_file) != NULL)
         { /* read 80 characters into buffer and enter loop */
            int len = (int)strlen(buffer);

            buffer[80] = '\0';

            pkt->pack_seq = htons(sequence);
            pkt->count = htons(len);

            //pkt->data = malloc(strlen(buffer));
            strcpy(pkt->data, buffer);

            bytes_sent = sendto(sock_server, pkt, sizeof(struct packet), 0,
                                (struct sockaddr *)&client_addr, client_addr_len); //send packet header to client

            printf("Packet %d generated for transmission with %d data bytes", sequence, len);

            //thing for timeout/loss shit here

            printf("Packet %d successfully transmitted with %d data bytes", sequence, len);

            while (1)
            {
               recvfrom(sock_server, ACK_pkt, sizeof(struct ack), 0,
                        (struct sockaddr *)&client_addr, &client_addr_len); //recieve ACK
               int ackSeq = ACK_pkt->ack_seq;
               printf("ACK %d received\n", ackSeq);
               if(ackSeq != sequence)
               {
                  bytes_sent = sendto(sock_server, pkt, sizeof(struct packet), 0,
                                      (struct sockaddr *)&client_addr, client_addr_len); //send packet header to client
                  printf("Packet %d generated for re-transmission with %d data bytes", sequence, len);

                  //thing for timeout/loss shit here

                  printf("Packet %d successfully transmitted with %d data bytes", sequence, len);
               }
               else {
                  break;
               }
            }
            sequence = !sequence; //increment sequence tracker
            packetsSent++;
            //free(pkt->data);
         }
         fclose(received_file); //finish file reading

         struct packet *eot = malloc(sizeof(struct packet)); //allocate space for End of Transmission packet
         eot->count = htons(0);
         eot->pack_seq = htons(sequence);

         bytes_sent = sendto(sock_server, eot, sizeof(struct packet), 0,
                             (struct sockaddr *)&client_addr, client_addr_len); //send End of Transmission packet to client

         printf("\nEnd of Transmission Packet with sequence number %d transmitted", sequence);

         //free memory
         free(pkt);
         //free(data);
         free(eot);

         //statistics
         printf("\n\n****************************************************************\n");
         printf("Number of data packets transmitted: %d\n", packetsSent);
         printf("Total number of data bytes transmitted: %d\n", bytes_trans);
         printf("\n****************************************************************\n");
      }
      else
      {
         perror("Packet error");
      }

      /* prepare the message to send

      msg_len = bytes_recd;
      for (i=0; i<msg_len; i++)
         modifiedSentence[i] = toupper (sentence[i]);
      */

      /* send message
 
      bytes_sent = sendto(sock_server, modifiedSentence, msg_len, 0,
               (struct sockaddr*) &client_addr, client_addr_len);
      */
   }
}
