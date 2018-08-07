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
#include <sys/time.h>
#include <time.h>


/*
 * Send a ctl message with info
 */
int send_ctlmsg(struct ctl_info* info, int sock, struct sockaddr_in* serveraddr)
{
        int len = sizeof(struct ctlhdr)+sizeof(struct ctl_info);
        char* buf = (char*)malloc(sizeof(char)*len);

        struct ctlhdr* c_hdr = (struct ctlhdr*) buf;
        c_hdr->protocol = CTL_NO;
        c_hdr->type = CTL_SND;

        struct ctl_info* c_info = (struct ctl_info*)(buf+sizeof(struct ctlhdr));
        c_info->ws_col = info->ws_col;
        c_info->ws_row = info->ws_row;
        c_info->frame_interval = info->frame_interval;

        int serverlen = sizeof(struct sockaddr_in);
        int n = sendto(sock, (const void*)buf, (size_t)len, 0,
                       (struct sockaddr *)serveraddr, (socklen_t)serverlen);
        return 1;
};

/*
 * recv a ctl message with info
 * and parse info into struct ctl_info* info
 */
int recv_ctlmsg(char* buf, int len, struct ctl_info* info)
{
        struct ctlhdr* c_hdr = (struct ctlhdr*) buf;
        struct ctl_info* c_info = (struct ctl_info*)(buf+sizeof(struct ctlhdr));

        if (c_hdr->protocol != CTL_NO || c_hdr->type != CTL_SND)
        {
                return -1;
        }

        info->ws_col = c_info->ws_col;
        info->ws_row = c_info->ws_row;
        info->frame_interval = c_info->frame_interval;

        return 1;
};

/*
 * Send ctlmsg ack
 */
int send_ctlmsg_ack(struct ctlhdr* c_hdr, int n, int sock, struct sockaddr_in* serveraddr)
{
        if (n != sizeof(struct ctlhdr)+sizeof(struct ctl_info)) {
                return -1;
        }

        int len = sizeof(struct ctlhdr);
        char* buf = (char*)malloc(sizeof(char)*len);

        struct ctlhdr* snd_chdr = (struct ctlhdr*) buf;
        snd_chdr->protocol = CTL_NO;
        snd_chdr->type = CTL_ACK;

        int serverlen = sizeof(struct sockaddr_in);
        int tmp = sendto(sock, (const void*)buf, (size_t)len, 0,
                         (struct sockaddr *)serveraddr, (socklen_t)serverlen);

        return 1;
};


/*
 * control msg header
 */
int recv_ctlmsg_ack(char* buf, int len)
{
        if (len != sizeof(struct ctlhdr)) {
                return -1;
        }

        struct ctlhdr* c_hdr = (struct ctlhdr*) buf;

        if (c_hdr->protocol != CTL_NO || c_hdr->type != CTL_ACK)
        {
                return -1;
        }
        return 1;
};


/*
 * Send ctlmsg end to terminate
 */
int send_ctlmsg_end(int sock, struct sockaddr_in* serveraddr)
{
        int len = sizeof(struct ctlhdr);
        char* buf = (char*)malloc(sizeof(char)*len);

        struct ctlhdr* c_hdr = (struct ctlhdr*) buf;
        c_hdr->protocol = CTL_NO;
        c_hdr->type = CTL_END;

        int serverlen = sizeof(struct sockaddr_in);
        int n = sendto(sock, (const void*)buf, (size_t)len, 0,
                       (struct sockaddr *)serveraddr, (socklen_t)serverlen);

        return 1;
};


/*
 * recv ctlmsg end
 */
int recv_ctlmsg_end(char* buf, int len)
{
        if (len != sizeof(struct ctlhdr)) {
                return -1;
        }

        struct ctlhdr* c_hdr = (struct ctlhdr*) buf;

        if (c_hdr->protocol != CTL_NO || c_hdr->type != CTL_END)
        {
                return -1;
        }
        return 1;
};










//
