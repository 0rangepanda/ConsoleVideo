#include "rtp.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/time.h>
#include <time.h>



__u64 timeval_to_u64(struct timeval* tv)
{
        __u64 sec = (__u64)tv->tv_sec*1000*1000;
        return sec+tv->tv_usec;
};

/*
 * print packet to hex
 */
char* packet_str(char* buffer, int len)
{
        //char ret[len*2+1];
        char* ret = (char*)malloc(sizeof(char)*(len*2+1));

        for (size_t i = 0; i < len; i++) {
                sprintf(&ret[i*2], "%02X", (unsigned char) *(buffer+i));
        }
        ret[len*2] = '\0';
        return ret;
};


/*
 * RTP sender
 */
RTPsender::RTPsender(int max_udplen, int sock, struct sockaddr_in* serveraddr)
{
        this->max_udplen = max_udplen;
        this->sock = sock;

        bzero((char *) &this->serveraddr, sizeof(this->serveraddr));
        this->serveraddr.sin_family = serveraddr->sin_family;
        this->serveraddr.sin_addr.s_addr = serveraddr->sin_addr.s_addr;
        this->serveraddr.sin_port = serveraddr->sin_port;

        this->pktlen = sizeof(struct rtphdr) + max_udplen;
        this->buf = (char*)malloc(sizeof(char) * pktlen);
};


/*
 * NOTE: seqno start from 0, endflag is 1
 */
int RTPsender::rtpsend(char* msg, int msglen, __u8 type)
{
        if (msglen > 256 * this->max_udplen)
        {
                return -1; //should not split to more than 256 pieces (2 Bytes)
        }//can increase limit to 4 Bytes

        /* get time */
        struct timeval tv;
        gettimeofday(&tv, NULL);
        //__u64 frametime = tv.tv_sec*1000*1000+tv.tv_usec; //in usec
        __u64 frametime = timeval_to_u64(&tv);

        int n;
        int seqno = 0;
        int serverlen = sizeof(serveraddr);

        //int pktlen = sizeof(struct rtphdr) + max_udplen;
        //char* buf = (char*)malloc(sizeof(char) * pktlen); //TODO: use one buf

        /* rtshdr */
        struct rtphdr* r_hdr = (struct rtphdr*) this->buf;

        r_hdr->protocol = 233;
        r_hdr->type = type;
        r_hdr->endflag = 0X00;
        r_hdr->timestamp = frametime;
        r_hdr->total_len = msglen;
        //TODO: r_hdr->src_id = 0xFFFF;

        char* content = (char*)(this->buf + sizeof(struct rtphdr));

        /* split frame to pkts */
        while (msglen > max_udplen)
        {
                msglen -= max_udplen;
                r_hdr->seqno = seqno & 0xFF;
                memcpy(content, msg, max_udplen);

                n = sendto(sock, (const void*)this->buf, (size_t)this->pktlen, 0,
                           (struct sockaddr *)&serveraddr, (socklen_t)serverlen);

                msg += max_udplen;
                seqno++;
        }

        /* last pkt of this frame */
        r_hdr->endflag = 0X01;
        r_hdr->seqno = seqno & 0xFF;
        memcpy(content, msg, max_udplen);

        n = sendto(sock, (const void*)this->buf, (size_t)(sizeof(struct rtphdr)+msglen), 0,
                   (struct sockaddr *)&serveraddr, (socklen_t)serverlen);

        return 1;
};



/*
 * RTP recver
 */
RTPrecver::RTPrecver()
{
        this->expire = 1000*1000; //1 sec
        this->recv_firstpkt = 0;
};


RTPrecver::RTPrecver(__u64 expire)
{
        this->expire = expire;
        this->recv_firstpkt = 0;
};

void RTPrecver::setexpire(__u64 expire)
{
        this->expire = expire;
};


/*
 * Recv a RTP pkt and try to rebuild the frame
 * should parse protocol number before call this function firstly
 */
int RTPrecver::recv(char* buf, int buflen)
{
        struct rtphdr* r_hdr = (struct rtphdr*) buf;
        if (r_hdr->protocol != 233)
        {
                return -1;
        }

        __u8 type = r_hdr->type;
        __u16 seqno = r_hdr->seqno;
        __u64 timestamp = r_hdr->timestamp;
        char* content = (char*)(buf + sizeof(struct rtphdr));

        struct framepiece* p = (struct framepiece*)malloc(sizeof(struct framepiece));
        p->seqno = r_hdr->seqno;
        p->next = NULL;
        p->status = 0;
        p->len = buflen-sizeof(struct rtphdr);
        p->content = (char*)malloc(sizeof(char)*p->len);
        memcpy(p->content, buf+sizeof(struct rtphdr), p->len);

        //fprintf(stdout, "%ld, %d, %d\n", timestamp, seqno, r_hdr->endflag);

        if (this->frameMap.find(timestamp) != this->frameMap.end())
        {
                /* existing frame */
                struct frame* f = this->frameMap[timestamp];
                f->count += 1;

                /* insert into the linked list */
                //if (f->headerMap.find(type) == f->headerMap.end())
                if (f->headerMap[type] == NULL)
                {
                        /* new type */
                        struct framepiece* header =
                                (struct framepiece*)malloc(sizeof(struct framepiece));
                        header->seqno = -1;
                        header->status = 1;

                        f->headerMap[type] = header;
                        f->headerMap[type]->next = p;
                }
                else
                {
                        /* existing type */
                        struct framepiece* h = f->headerMap[type];
                        struct framepiece* last = f->headerMap[type];
                        struct framepiece* tmp = f->headerMap[type]->next;

                        while (tmp != NULL)
                        {
                                if (tmp->seqno > p->seqno)
                                {
                                        last->next = p;
                                        p->next = tmp;
                                        break;
                                }
                                last = last->next;
                                tmp = tmp->next;
                        }

                        if (tmp == NULL)
                        {
                                last->next = p;
                        }
                }
        }
        else
        {
                /* create a fake node as the linked list header */

                /* new frame */
                struct frame* f =
                        (struct frame*)malloc(sizeof(struct frame));

                f->timestamp = timestamp;
                f->count = 1;
                f->status = 0;
                //f->total_len = r_hdr->total_len;
                gettimeofday(&f->recv_time, NULL);

                //NOTE: NOTE: NOTE: NOTE: bug is here
                f->headerMap =
                        (struct framepiece**)malloc(sizeof(struct framepiece*)*256);

                struct framepiece* header =
                        (struct framepiece*)malloc(sizeof(struct framepiece));
                header->seqno = -1;
                header->status = 1;
                header->len = r_hdr->total_len;

                f->headerMap[type] = header;
                f->headerMap[type]->next = p;

                /* enqueue */
                this->frameMap[timestamp] = f;
                this->framesqueue.push(timestamp);
        }

        /* check if is the last frame */
        if (r_hdr->endflag !=0)
                return 1;
        else
                return 0;
};



int RTPrecver::printframe(struct frame* f)
{
        /*
           struct framepiece* p = f->header->next;
           printf("Print Frame!\n");
           while (p != NULL)
           {
                printf("%d: %s\n", p->seqno, p->content);
                p = p->next;
           }
         */
        return 1;
};


/*
 * Go through a frame to see if it is complete
 * NOTE: For efficiency, only do this once:
 *  if success, will return the whole msg
 *  else, just discard it
 * Do this at the exact expected timepoint!
 */
int RTPrecver::checkframe(char*& buf, int* buflen, __u8 type)
{
        if (this->framesqueue.empty())
                return -1;

        __u64 top = this->framesqueue.top();
        struct frame* f = this->frameMap[top];

        struct framepiece* tmp = f->headerMap[type]->next;
        int seqno = 0;
        *buflen = f->headerMap[type]->len+1;
        buf = (char*)malloc(sizeof(char)*(*buflen));
        char* b = buf;

        while (tmp != NULL)
        {
                /* if fail */
                if (tmp->seqno > seqno)
                {
                        return -1;
                }

                memcpy(b, tmp->content, tmp->len);
                b += tmp->len;

                seqno++;
                tmp = tmp->next;
        }
        *(b++) = '\0';

        /* if success, pop out and return 1 */
        return 1;
};

/*
 * pop out the top frame in queue
 */
int RTPrecver::popframe()
{
        __u64 top = this->framesqueue.top();
        free(this->frameMap[top]);
        this->frameMap.erase(top);
        this->framesqueue.pop();
        return 1;
}


/*
 * After recv first pkt, start timer
 */

int RTPrecver::recvfirstpkt()
{
        return this->recv_firstpkt;
}

int RTPrecver::start_timer()
{
        //if this is the first pkt, start timer here
        if (this->recv_firstpkt==0)
        {
                this->recv_firstpkt = 1;
                struct timeval cur;
                gettimeofday(&cur, NULL);

                this->next_timestamp = timeval_to_u64(&cur) + this->expire;
                return 1;
        }
        return 0;
};

/*
 * After check a frame, update the next_timestamp
 */
int RTPrecver::reset_timer()
{
        this->next_timestamp += this->expire;
        return 1;
};

/*
 * Use this function to print at actual timepoint
 */
struct timeval* RTPrecver::getnexttime()
{
        if (this->recv_firstpkt==0)
        {
                return NULL;
        }

        /* get current time */
        struct timeval cur;
        gettimeofday(&cur, NULL);
        __u64 lefttime;
        if (this->next_timestamp<timeval_to_u64(&cur))
        {
                lefttime = 0;
        }
        else
        {
                lefttime = this->next_timestamp
                           - timeval_to_u64(&cur);
        }



        //while (lefttime<0)
        //{
        //    lefttime += this->expire;
        //}

        struct timeval* tv = (struct timeval*)malloc(sizeof(struct timeval));
        tv->tv_sec = lefttime / (1000*1000);
        tv->tv_usec = lefttime - lefttime / (1000*1000);
        //tv->tv_usec = this->expire;;
        return tv;
};


//end
