#include "client.h"

//static vars
static char GrayMark[9] = " .;o*8$@";
static int running;
static sigset_t signal_set;

std::mutex m;

static int session_start;
static int ctlmsg_flag;

static struct ctl_info my_info;
static struct ctl_info peer_info;

static struct winsize my_wnd;
static struct winsize peer_wnd;

/* ------------------------ Threads ------------------------ */


/* read camera and send out both ctlmsg and data */
void out_thread(struct server* s)
{
        printf("out_thread start!\n");

        int serverlen = sizeof(s->serveraddr);
        char buf[BUFSIZE];
        int n;

        /* open camera */
        VideoCapture cap;
        if(!cap.open(0)) {
                perror("No camera detected!");
                exit(0);
        }

        /* send ctlmsg and wait for ack */
        m.lock();
        session_start = 0;
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &my_wnd);
        my_info.ws_col = my_wnd.ws_col;
        my_info.ws_row = my_wnd.ws_row;

        double fps = cap.get(CV_CAP_PROP_FPS);
        my_info.frame_interval = fpstotimeval(fps);

        printf("wnd:%d, %d\nframe_interval:%llu usec\n", my_info.ws_col, my_info.ws_row, my_info.frame_interval);
        m.unlock();


        while (running)
        {
                send_ctlmsg(&my_info, s->sockfd, &s->serveraddr);
                sleep(3);
                if (session_start==1) {
                        break;
                }
        }
        printf("Running!\n");


        /* Videochat session start! */
        /* RTP sender */
        RTPsender rtpsender(256, s->sockfd, &s->serveraddr);
        //struct winsize w;

        while (running)
        {
                /* get window size changing*/
                struct winsize w;
                ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
                if (w.ws_col != my_wnd.ws_col || w.ws_row != my_wnd.ws_row)
                {
                        my_wnd.ws_col = w.ws_col;
                        my_wnd.ws_row = w.ws_row;
                        my_info.ws_col = my_wnd.ws_col;
                        my_info.ws_row = my_wnd.ws_row;
                        send_ctlmsg(&my_info, s->sockfd, &s->serveraddr);
                }

                /* get image and encode */
                Mat frame;
                cap >> frame;
                if( frame.empty() ) break; // end of video stream

                char* graylvl;//to graylvl image
                int outlen;
                mat2grayim(frame, GrayMark, w, graylvl, &outlen, GET_LVL);
                //mat2grayim(frame, GrayMark, peer_wnd, graylvl, &outlen, GET_IMAGE);

                char* encimage;//to encoded graylvl image
                int enclen;
                gray_encode(GrayMark, graylvl, outlen, encimage, &enclen);

                //TODO: split the frame and send via multiple udp packet
                rtpsender.rtpsend(encimage, enclen, VIDEO);//send a whole frame
                //rtpsender.rtpsend(graylvl, outlen, VIDEO);
        }
};

/* recv data and print screen */
void inc_thread(struct server* s)
{
        printf("inc_thread start!\n");
        char buf[BUFSIZE];
        int serverlen;
        int n;
        struct sockaddr_in serveraddr;

        char* encimage;
        int enclen;
        char* grayimage_dec;
        int outlen_dec;

        RTPrecver recver; //20ms expire

        fd_set readset;
        int ret;

        while (running)
        {
                FD_ZERO(&readset);
                FD_SET(s->sockfd, &readset);


                ret = select(s->sockfd+1, &readset, NULL, NULL, recver.getnexttime());
                //ret = select(s->sockfd+1, &readset, NULL, NULL, &tv);

                switch (ret)
                {
                case -1:
                        break;

                case  0:
                        //NOTE: behavior is not for sure if delay is high

                        // print a frame
                        if (recver.checkframe(encimage, &enclen, VIDEO))
                        {
                                gray_decode(GrayMark, encimage, enclen, grayimage_dec, &outlen_dec);
                                //printimage(encimage, enclen);
                                printimage(grayimage_dec, outlen_dec);
                                //std::cout << "image" << "\n";
                        }



                        recver.popframe();
                        recver.reset_timer();

                default:
                        if(FD_ISSET(s->sockfd, &readset)) // recv a pkt
                        {
                                n = recvfrom(s->sockfd, (void*)buf, BUFSIZE, 0,
                                             (struct sockaddr *)&serveraddr, (socklen_t *)&serverlen);

                                //handle different type of pkt
                                int recv_type = parse_recv(buf, n);

                                switch (recv_type)
                                {
                                case RTP_NO:
                                        recver.recv(buf, n);
                                        if (recver.recvfirstpkt()==0) {
                                                recver.start_timer();
                                        }
                                        break;
                                case CTL_NO:
                                        if (recv_ctlmsg(buf, n, &peer_info))
                                        {
                                                //update control infos
                                                m.lock();
                                                peer_wnd.ws_col = peer_info.ws_col;
                                                peer_wnd.ws_row = peer_info.ws_row;
                                                recver.setexpire(peer_info.frame_interval);
                                                m.unlock();
                                                //send ack
                                                send_ctlmsg_ack((struct ctlhdr* )buf, n, s->sockfd, &serveraddr);
                                        }
                                        if (recv_ctlmsg_ack(buf, n))
                                        {
                                                m.lock();
                                                session_start = 1;
                                                m.unlock();
                                        }
                                        break;
                                default:
                                        break;
                                }
                        }
                }
        }
};


/* handle signal */
void monitor_thread()
{
        int sig;
        sigwait(&signal_set, &sig);
        printf("received SIGINT!\n");

        /* end thread and cleaning-up */
        running = 0;
        exit(0);
}

/* ------------------------ Process ------------------------ */

static
void Process(struct server* s)
{
        /* Handling Ctrl+C */
        sigemptyset(&signal_set);
        sigaddset(&signal_set, SIGINT);
        sigprocmask(SIG_BLOCK, &signal_set, 0);

        running = 1;
        std::thread monitor(monitor_thread);
        std::thread inc(inc_thread, s);
        std::thread out(out_thread, s);

        inc.join();
        out.join();

};

/* ----------------------- main() ----------------------- */
int main(int argc, char const *argv[])
{
        int n;
        char buf[BUFSIZE];

        int serverlen;
        struct hostent *server;
        struct server s;

        /* check command line arguments */
        if (argc != 3) {
                fprintf(stderr,"usage: %s <hostname> <port>\n", argv[0]);
                exit(0);
        }
        s.hostname = argv[1];
        s.portno = atoi(argv[2]);

        /* get socket to server and build the server's Internet address */
        s.sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (s.sockfd < 0)
                error("ERROR opening socket");

        server = gethostbyname(s.hostname);
        if (server == NULL) {
                fprintf(stderr,"ERROR, no such host as %s\n", s.hostname);
                exit(0);
        }

        bzero((char *) &s.serveraddr, sizeof(s.serveraddr));
        s.serveraddr.sin_family = AF_INET;
        bcopy((char *)server->h_addr,
              (char *)&s.serveraddr.sin_addr.s_addr, server->h_length);
        s.serveraddr.sin_port = htons(s.portno);


        /* confirm with server */
        bzero(buf, BUFSIZE);
        printf("Please enter msg: ");
        fgets(buf, BUFSIZE, stdin);

        serverlen = sizeof(s.serveraddr);
        n = sendto(s.sockfd, (const void*)buf, (size_t)strlen(buf), 0,
                   (struct sockaddr *)&s.serveraddr, (socklen_t)serverlen);

        if (n < 0)
                error("ERROR in sendto");

        n = recvfrom(s.sockfd, (void*)buf, BUFSIZE, 0,
                     (struct sockaddr *)&s.serveraddr, (socklen_t *)&serverlen);
        if (n < 0)
                error("ERROR in recvfrom");
        printf("Echo from server: %s\n", buf);


        /* After server tell u ready, go to the main proc */
        printf("Start Videochat!\n");
        Process(&s);

        return 0;
}










//end
