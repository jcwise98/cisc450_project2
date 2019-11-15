#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MAXSIZE 80

struct packet *makepacket(short count, short pack_seq, char*data);

struct packet{
    short count;
    short pack_seq;
};


