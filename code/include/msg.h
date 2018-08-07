/*
 *
 */

#ifndef MSG_H
#define MSG_H

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <iostream>

using namespace std;

//my rtp protocol number = 233
#ifndef CTL_NO
#define CTL_NO 213
#endif

#ifndef CTL_ACK
#define CTL_ACK 0x01
#endif

#ifndef CTL_SND
#define CTL_SND 0x02
#endif

#ifndef CTL_END
#define CTL_END 0x02
#endif

typedef uint8_t __u8;        //1 Byte
typedef uint16_t __u16;      //2 Byte
typedef uint32_t __u32;      //4 Byte
typedef uint64_t __u64;      //4 Byte

/*
 * control msg header
 */
struct ctlhdr
{
        __u8 protocol; //my ctl protocol number = 213
        __u8 type; //ack
        __u16 src_id;
        __u16 session_id;
} __attribute__ ((packed));

//wrapper of ctl infos
struct ctl_info {
        __u16 ws_col;
        __u16 ws_row;
        __u64 frame_interval;
} __attribute__ ((packed));


int send_ctlmsg(struct ctl_info* info, int sock, struct sockaddr_in* serveraddr);
int recv_ctlmsg(char* buf, int len, struct ctl_info* info);
int send_ctlmsg_ack(struct ctlhdr* c_hdr, int n, int sock, struct sockaddr_in* serveraddr);
int recv_ctlmsg_ack(char* buf, int len);
int send_ctlmsg_end(int sock, struct sockaddr_in* serveraddr);
int recv_ctlmsg_end(char* buf, int len);


#endif
