#include "opencv2/opencv.hpp"
#include "video.h"
#include "rtp.h"
#include "msg.h"


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>

#include <iostream>
#include <thread>
#include <mutex>


/* --------------------- Definitions ---------------------- */

//for rtp frame types
#ifndef VIDEO
#define VIDEO 0x11
#endif

#ifndef AUDIO
#define AUDIO 0x12
#endif


#ifndef BUFSIZE
#define BUFSIZE 2048
#endif

#ifndef MAX_UDPLEN
#define MAX_UDPLEN 256
#endif


//to get the first 2 byte of a packet
struct parse
{
    __u8 protocol;
};



struct server
{
        int sockfd;

        //infos of target address
        int portno;
        const char *hostname;
        struct hostent *host;

        struct sockaddr_in serveraddr;
        struct sockaddr_in localaddr;  //local address of this socket
};



/* -------------------- Functions ---------------------- */
void error(const char *msg);
int parse_recv(char* buf, int len);
__u64 fpstotimeval(double fps);

int bind_udp(struct server* s);
int build_addr(const char* hostname, int port, struct server* s);


int make_call(struct server* s);
int wait_call(struct server* s);
