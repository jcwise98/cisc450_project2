/* udp_client.c */
/* Programmed by Adarsh Sethi */
/* Sept. 19, 2019 */

#include <stdio.h>      /* for standard I/O functions */
#include <stdlib.h>     /* for exit */
#include <string.h>     /* for memset, memcpy, and strlen */
#include <netdb.h>      /* for struct hostent and gethostbyname */
#include <sys/socket.h> /* for socket, sendto, and recvfrom */
#include <netinet/in.h> /* for sockaddr_in */
#include <unistd.h>     /* for close */
#include "packet.h"

#define STRING_SIZE 1024

int main(void) {

   int sock_client; /* Socket used by client */

   struct sockaddr_in client_addr; /* Internet address structure that
                                        stores client address */
   unsigned short client_port;     /* Port number used by client (local port) */

   struct sockaddr_in server_addr;    /* Internet address structure that
                                        stores server address */
   struct hostent *server_hp;         /* Structure to store server's IP
                                        address */
   char server_hostname[STRING_SIZE]; /* Server's hostname */
   unsigned short server_port;        /* Port number used by server (remote port) */

   char filename[STRING_SIZE];              /* send message */
   char data[STRING_SIZE];                  /* receive message */
   unsigned int msg_len;                    /* length of message */
   int bytes_sent, bytes_recd, bytes_recd1; /* number of bytes sent or received */
   struct packet *header = malloc(sizeof(struct packet));
   //header->data = malloc(81);

   /* open a socket */

   if ((sock_client = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
   {
      perror("Client: can't open datagram socket\n");
      exit(1);
   }

   /* Note: there is no need to initialize local client address information
            unless you want to specify a specific local port.
            The local address initialization and binding is done automatically
            when the sendto function is called later, if the socket has not
            already been bound. 
            The code below illustrates how to initialize and bind to a
            specific local port, if that is desired. */

   /* initialize client address information */

   client_port = 0; /* This allows choice of any available local port */

   /* Uncomment the lines below if you want to specify a particular 
             local port: */
   /*
   printf("Enter port number for client: ");
   scanf("%hu", &client_port);
   */

   /* clear client address structure and initialize with client address */
   memset(&client_addr, 0, sizeof(client_addr));
   client_addr.sin_family = AF_INET;
   client_addr.sin_addr.s_addr = htonl(INADDR_ANY); /* This allows choice of
                                        any host interface, if more than one 
                                        are present */
   client_addr.sin_port = htons(client_port);

   /* bind the socket to the local client port */

   if (bind(sock_client, (struct sockaddr *)&client_addr,
            sizeof(client_addr)) < 0)
   {
      perror("Client: can't bind to local address\n");
      close(sock_client);
      exit(1);
   }

   /* end of local address initialization and binding */

   /* initialize server address information */

   //printf("Enter hostname of server: ");
   //scanf("%s", server_hostname);
   strcpy(server_hostname, "cisc450.cis.udel.edu");
   if ((server_hp = gethostbyname(server_hostname)) == NULL)
   {
      perror("Client: invalid server hostname\n");
      close(sock_client);
      exit(1);
   }
   printf("Enter port number for server: ");
   scanf("%hu", &server_port);

   /* Clear server address structure and initialize with server address */
   memset(&server_addr, 0, sizeof(server_addr));
   server_addr.sin_family = AF_INET;
   memcpy((char *)&server_addr.sin_addr, server_hp->h_addr,
          server_hp->h_length);
   server_addr.sin_port = htons(server_port);

   /* user interface */

   // valid file flag
   short valid_file = 0;

   struct packet *file_pkt = malloc(sizeof(struct packet));

   while (valid_file == 0)
   {
      printf("Enter name of file for transfer:");
      scanf("%s", filename);

      file_pkt->count = htons(strlen(filename));
      file_pkt->pack_seq = htons(0);

      strcpy(file_pkt->data, filename);

      bytes_sent = sendto(sock_client, file_pkt, sizeof(struct packet), 0,
                          (struct sockaddr *)&server_addr, sizeof(server_addr));

      // tmp flag
      short tmpFlag;
      bytes_recd = recvfrom(sock_client, &tmpFlag, sizeof(short), 0,
                            (struct sockaddr *)0, (int *)0);

      if (bytes_recd <= 0)
      {
         perror("Packet Error!");
         close(sock_client);
         exit(1);
      }

      valid_file = ntohs(tmpFlag);
      printf("valid file flag = %d\n", valid_file);
      if (valid_file == 0)
      {
         printf("File not found try again\n");
      }
      else
      {
      }
   }

   int total_bytes = 0;
   int packs_rec = 0;
   int transmission_flag = 1;
   int AckSeq = 0;
   short tmp;
   short tmp2;
   struct ack *ACK_pkt = malloc(sizeof(struct ack));
   // receive initial header packet for file transmission

   while (1)
   {
      bytes_recd = recvfrom(sock_client, header, sizeof(struct packet), 0,
                            (struct sockaddr *)0, (int *)0);

      // tmp variables for decoding network to host data

      tmp = ntohs(header->count);
      tmp2 = ntohs(header->pack_seq);

      if (tmp2 == AckSeq)
      {
         printf("Packet %d received with %d data bytes\n", tmp2, tmp);
         ACK_pkt->ack_seq = htons(AckSeq);
         printf("ACK %d generated for transmission\n", AckSeq);
         sendto(sock_client, ACK_pkt, sizeof(struct ack), 0,
                (struct sockaddr *)&server_addr, sizeof(server_addr));
         printf("ACK %d successfully transmitted\n", AckSeq);
         AckSeq = !AckSeq;
         break;
      }
      else
      {
         printf("Duplicate packet %d received with %d data bytes\n", tmp2, tmp);
         int oldAck = !AckSeq;
         ACK_pkt->ack_seq = htons(oldAck);
         printf("ACK %d generated for transmission\n", oldAck);
         sendto(sock_client, ACK_pkt, sizeof(struct ack), 0,
                (struct sockaddr *)&server_addr, sizeof(server_addr));
         printf("ACK %d successfully transmitted\n", oldAck);
      }
   }
   strcpy(data, header->data);
   // increment total bytes

   total_bytes += tmp;

   // receive data line
   //bytes_recd1 = recvfrom(sock_client, data, tmp, 0,
   //                       (struct sockaddr *)0, (int *)0);

   // check if invalid packet

   if (bytes_recd <= 0)
   {
      perror("Invalid packet(s)");
      free(header);
      close(sock_client);
      exit(1);
   }

   // increment packets recieved
   packs_rec++;

   // declare file to write to
   FILE *f;

   // open file in write mode
   f = fopen("out.txt", "w");

   // malloc char array for count + 1 bytes
   char *buff1 = malloc(tmp + 1);

   /// cpy for tmp characters
   strncpy(buff1, data, tmp);

   // clean out ^M ascii character that are in the data buffer
   buff1[tmp] = '\0';

   // free allocated space
   fputs(buff1, f);
   printf("Packet %d delivered to user\n", tmp2);
   free(buff1);
   // loop while tranmission_flag is high

   while (transmission_flag)
   {
      while (1)
      {
         bytes_recd = recvfrom(sock_client, header, sizeof(struct packet), 0,
                               (struct sockaddr *)0, (int *)0);

         // tmp variables for decoding network to host data

         tmp = ntohs(header->count);
         tmp2 = ntohs(header->pack_seq);

         if (tmp2 == AckSeq)
         {
            printf("Packet %d received with %d data bytes\n", tmp2, tmp);
            ACK_pkt->ack_seq = htons(AckSeq);
            printf("ACK %d generated for transmission\n", AckSeq);
            sendto(sock_client, ACK_pkt, sizeof(struct ack), 0,
                   (struct sockaddr *)&server_addr, sizeof(server_addr));
            printf("ACK %d successfully transmitted\n", AckSeq);
            AckSeq = !AckSeq;
            break;
         }
         else
         {
            printf("Duplicate packet %d recieved with %d data bytes\n", tmp2, tmp);
            int oldAck = !AckSeq;
            ACK_pkt->ack_seq = htons(oldAck);
            printf("ACK %d generated for transmission\n", oldAck);
            sendto(sock_client, ACK_pkt, sizeof(struct ack), 0,
                   (struct sockaddr *)&server_addr, sizeof(server_addr));
            printf("ACK %d successfully transmitted\n", oldAck);
         }
      }
      strcpy(data, header->data);
      total_bytes += tmp;

      // check for EOT packet
      if (tmp == 0)
      {

         transmission_flag = 0;
         break;
      }
      else
      {
         //bytes_recd1 = recvfrom(sock_client, data, tmp, 0,
         //                       (struct sockaddr *)0, (int *)0);
         if (bytes_recd <= 0)
         {
            perror("Invalid packets");
            free(header);
            // close file
            fclose(f);
            // close socket connection
            close(sock_client);
            exit(1);
         }
         packs_rec++;
         // malloc tmp + 1
         char *buff = malloc(tmp + 1);
         // write up to data[tmp] buffer
         strncpy(buff, data, tmp);

         // clear out any characters in data buffer
         buff[tmp] = '\0';
         // print mesage
	 // printf("Recieved packet %d with %d bytes\n", tmp2, tmp);
         // write to file with f puts
         fputs(buff, f);
         printf("Packet %d delivered to user\n", tmp2);
         // free buff
         free(buff);
      }
   }

   fclose(f);
   // print statistics
   printf("End of Transmission packet received with sequence number %d transmitted with %d bytes\n", tmp2, tmp);
   printf("\n************************************************************************\n");
   printf("		Transmission Statistics\n");
   printf("\n");
   printf("Total Bytes Received: %d\n", total_bytes);
   printf("\n");
   printf("# of Data Packets Received: %d\n", packs_rec);
   printf("\n************************************************************************\n");

   // free allocated mem
   /* close the socket */

   free(header);

   /*
   printf("Please input a sentence:\n");
   scanf("%s", sentence);
   msg_len = strlen(sentence) + 1;

   /* send message */

   /*
   bytes_sent = sendto(sock_client, sentence, msg_len, 0,
                       (struct sockaddr *)&server_addr, sizeof(server_addr));

   /* get response from server */

   /*
   printf("Waiting for response from server...\n");
   bytes_recd = recvfrom(sock_client, modifiedSentence, STRING_SIZE, 0,
                         (struct sockaddr *)0, (int *)0);
   printf("\nThe response from server is:\n");
   printf("%s\n\n", modifiedSentence);

   /* close the socket */

   close(sock_client);
}
