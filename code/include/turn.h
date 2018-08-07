/*
 * My TURN Protocol
 */

#ifndef TURN_H
#define TURN_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>
#include <string>
#include <ctime>
#include <unordered_map>
#include <unordered_set>

typedef uint8_t __u8;        //1 Byte
typedef uint16_t __u16;      //2 Byte
typedef uint32_t __u32;      //4 Byte


/*
 * TURN msg types (1 Byte)
 * TURN_0x01 (port request): c->s, s assign another port for it
 * TURN_0x02 (portreq response):  s->c, tell
 * TURN_0x03 (port release):  c->s, offline and release the port
 * TURN_0x04 (caller call):  c->s, s check if the callee online
 * TURN_0x05 (callee ready): s->c, tell the caller the is callee online
 * TURN_0x06 (incomming call): s->c, tell callee someone is calling
 * TURN_0x07 (answer the call): c->s, tell server if I want to answer
 */
enum MsgType {
        /* client->server */
        port_req = 0x11,
        port_rel = 0x13,
        mk_call  = 0x14,
        ans_call = 0x17,

        /* server->client */
        port_res  = 0x12,
        ready     = 0x15,
        inc_coall = 0x16,
};



/*
 * TURN msg header
 * MyTURN proto_no = 249
 */
struct turnhdr
{
        __u8 proto;
        __u8 type : 4,
             status : 4;
        __u16 port;

} __attribute__ ((packed));


/*
 * TURN server class
 */
class Turnserver
{
public:
        Turnserver();

        int parseturnmsg(char *packet, int packet_len);
        void prepare_msg(char*& buf, int* outlen);
        void TURN(int sock1, int sock2, struct sockaddr_in *addr1, struct sockaddr_in *addr2);



private:
        std::unordered_map<std::string, __u16> usrportMap; //NOTE: username to port mapping
        std::unordered_map<std::time_t, __u16> usrtimeMap; //NOTE: username to timestamp mapping
        std::unordered_set<__u16> portSet; //NOTE: already used ports

};


#endif
