/*
 * An implememntation of real-time portocol
 * Sender and recver can do udp split and rebuild.
 */

#ifndef RTP_H
#define RTP_H

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <iostream>
#include <queue>
#include <unordered_map>

using namespace std;


//my rtp protocol number = 233
#ifndef RTP_NO
#define RTP_NO 233
#endif


typedef uint8_t __u8;        //1 Byte
typedef uint16_t __u16;      //2 Byte
typedef uint32_t __u32;      //4 Byte
typedef uint64_t __u64;      //4 Byte


/*
 * type
 * control:0x1 video:0x11 audio::0x12 windowsize::0x21
 */
struct rtphdr {
        __u8 protocol;       //my rtp protocol number = 233
        __u16 type : 8,      //audio or video...
              endflag : 8;   //indicating if this is the end of one frame
        __u16 total_len;     //the total len of this frame
        __u16 seqno;         //sequence number (in one frame)
        __u64 timestamp;
        //__u32 src_id;        //unique source identifier
} __attribute__ ((packed));


__u64 timeval_to_u64(struct timeval* tv);
char* packet_str(char* buffer, int len);


/*
 * Sender
 */
class RTPsender
{
public:
        RTPsender(int max_udplen, int sock, struct sockaddr_in* serveraddr);
        int rtpsend(char* msg, int msglen, __u8 type);//send a whole frame

private:
        int max_udplen;
        int sock;
        struct sockaddr_in serveraddr;

        int pktlen;
        char* buf;
};

/*
 * Recver
 */
struct frame {
        __u64 timestamp;
        //__u16 total_len; store total_len in the fake header
        int status;  //status = 1 means the frame is expired
        int count;

        struct framepiece* header; // a singly linked list of framepiece
        struct timeval recv_time;
        /*
         * NOTE: there could be more than one msg type for one frame (video, audio)
         * this hashmap deal with this problem
         */
        //unordered_map<__u8, struct framepiece*>& headerMap;
        struct framepiece** headerMap;



        bool operator<(const frame &rhs) const
        {
                return timestamp < rhs.timestamp;
        }
};

struct framepiece {
        int seqno;
        int status;
        struct framepiece* next;

        char* content;
        int len;

        bool operator<(const framepiece &rhs) const
        {
                return seqno < rhs.seqno;
        }
};

class RTPrecver
{
public:
        RTPrecver();
        RTPrecver(__u64 expire);

        void setexpire(__u64 expire);

        __u64 gettimestamp(){
                return this->framesqueue.top();
        };
        int recv(char* buf, int buflen); //recv one udp packet
        int printframe(struct frame* f);
        int checkframe(char*& buf, int* buflen, __u8 type);
        int popframe();

        int recvfirstpkt();
        int start_timer();
        int reset_timer();
        struct timeval* getnexttime();


        //public var
        __u64 next_timestamp;

private:
        __u64 expire; //discard a frame after this amount of time (in usec)
        int recv_firstpkt;
        //__u64 next_timestamp;

        priority_queue<__u64> framesqueue; //use timestamp as unique id
        unordered_map<__u64, struct frame*> frameMap; //timestamp->seqno: to get each frame's sequence

};



#endif
