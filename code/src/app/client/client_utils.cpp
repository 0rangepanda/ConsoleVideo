#include "client.h"

/* ------------------------ Utils ------------------------ */

/*
 * error handler
 */
void error(const char *msg) {
        perror(msg);
        exit(0);
};


/*
 * return the first 2 Byte of a pkt,
 * which in this app is used as protocol number
 */
int parse_recv(char* buf, int len)
{
        if (len < 2)
                return -1;
        struct parse* p = (struct parse*) buf;
        __u8 ret = p->protocol;
        return ret;
};


/*
 * calculate the time interval between two frames
 * according to the fps value
 */
__u64 fpstotimeval(double fps)
{
        __u64 timeval = 1000*1000 / fps; //in usec
        return timeval;
};


/*
 * bind socket and return the local port bind to this socket
 */
int bind_udp(struct server* s)
{
        /* get socket to server and build the server's Internet address */
        s->sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (s->sockfd < 0)
        {
                perror("ERROR opening socket");
                return -1;
        }
        int optval = 1;
        setsockopt(s->sockfd, SOL_SOCKET, SO_REUSEADDR,
                   (const void *)&optval, sizeof(int));

        /* local addr */
        socklen_t addrlen = sizeof(struct sockaddr_in);

        bzero((char *) &s->localaddr, sizeof(s->localaddr));
        s->localaddr.sin_family = AF_INET;
        s->localaddr.sin_addr.s_addr = htonl(INADDR_ANY);
        s->localaddr.sin_port = htons(0);
        if (::bind(s->sockfd, (struct sockaddr*) &s->localaddr, addrlen) < 0)
                error("ERROR on binding");

        getsockname(s->sockfd, (struct sockaddr*) &s->localaddr, &addrlen);
        return ntohs(s->localaddr.sin_port);
};

/*
 * build the peer network address
 */
int build_addr(const char* hostname, int port, struct server* s)
{
        s->hostname = hostname;
        s->portno = port;

        printf("%s\n", s->hostname);
        s->host = gethostbyname(s->hostname);
        if (s->host == NULL)
        {
                fprintf(stderr,"ERROR, no such host as %s\n", s->hostname);
                return -1;
        }

        bzero((char *) &s->serveraddr, sizeof(s->serveraddr));
        s->serveraddr.sin_family = AF_INET;
        bcopy((char *)s->host->h_addr,
              (char *)&s->serveraddr.sin_addr.s_addr, s->host->h_length);
        s->serveraddr.sin_port = htons(s->portno);
        return 1;
};



/*
 * Make call to address s and wait for ack
 * retry every 1 sec if not recv ack
 * Max retry time is 5
 */
int make_call(struct server* s)
{
        int n, serverlen;
        char buf[1024];

        fd_set readset;
        int ret;

        struct timeval tv;
        tv.tv_sec = 1;
        tv.tv_usec = 0;   //retry every 1 sec

        strcpy(buf, "I want to call you!\0");
        int retry = 5;
        while (retry>0)
        {
                FD_ZERO(&readset);
                FD_SET(s->sockfd, &readset);

                ret = select(s->sockfd+1, &readset, NULL, NULL, &tv);

                switch (ret)
                {
                case -1:
                        break;
                case  0:
                        serverlen = sizeof(struct sockaddr_in);
                        n = sendto(s->sockfd, (const void*)buf, (size_t)strlen(buf), 0,
                                   (struct sockaddr *)&s->serveraddr, (socklen_t)serverlen);
                        break;
                default:
                        if(FD_ISSET(s->sockfd, &readset))
                        {
                                n = recvfrom(s->sockfd, (void*)buf, 1024, 0,
                                             (struct sockaddr *)&s->serveraddr, (socklen_t *)&serverlen);

                                if (strncmp(buf, "I am ready!", n)==0)
                                {
                                        return 1;
                                }
                        }
                }
                retry--;
        }
        return -1;
};


/*
 * wait for somebody call me
 * send ack when recv a call
 * and fill the struct server* s (this is automatically)
 */
int wait_call(struct server* s)
{
        int n, serverlen;
        char buf[1024];

        while (1)
        {
                n = recvfrom(s->sockfd, (void*)buf, 1024, 0,
                             (struct sockaddr *)&s->serveraddr, (socklen_t *)&serverlen);
                //after this, s->serveraddr will be the peer address

                if (strncmp(buf, "I want to call you!\0", n)==0)
                {
                        //somebody call me
                        strcpy(buf, "I am ready!\0");
                        n = sendto(s->sockfd, (const void*)buf, (size_t)strlen(buf), 0,
                                   (struct sockaddr *)&s->serveraddr, (socklen_t)serverlen);
                        return 1;
                }
        }
        return -1;
};
