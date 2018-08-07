#include "client.h"

#include <getopt.h>
#include <assert.h>


//static vars
static char *app_name = NULL;
static char *callee_name = NULL;
static struct server s; //peer address or server address

static int wait_mode = 0;
static int caller_mode = 0;
static int p2p_mode = 0;
static int client_mode = 0;

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


        /*
        while (running)
        {
                send_ctlmsg(&my_info, s->sockfd, &s->serveraddr);
                sleep(3);
                if (session_start==1) {
                        break;
                }
        }
        */

        send_ctlmsg(&my_info, s->sockfd, &s->serveraddr);
        std::cout << "Recv ctlmsg and start streaming!" << "\n";


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

                                                session_start = 1;

                                                m.unlock();
                                                //send ack
                                                //send_ctlmsg_ack((struct ctlhdr* )buf, n, s->sockfd, &serveraddr);
                                        }
                                        if (recv_ctlmsg_ack(buf, n))
                                        {
                                                m.lock();
                                                session_start = 1;
                                                m.unlock();
                                        }
                                        if (recv_ctlmsg_end(buf, n))
                                        {
                                                m.lock();
                                                //terminate
                                                running = 0;
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
void monitor_thread(struct server* s)
{
        int sig;
        sigwait(&signal_set, &sig);
        printf("received SIGINT!\n");

        /* end thread and cleaning-up */
        running = 0;
        //send terminate msg
        send_ctlmsg_end(s->sockfd, &s->serveraddr);
        exit(0);
};


/* ------------------------ Process ------------------------ */

static
void Process(struct server* s)
{
        /* Handling Ctrl+C */
        sigemptyset(&signal_set);
        sigaddset(&signal_set, SIGINT);
        sigprocmask(SIG_BLOCK, &signal_set, 0);

        running = 1;
        std::thread monitor(monitor_thread, s);
        std::thread inc(inc_thread, s);
        std::thread out(out_thread, s);

        inc.join();
        out.join();
        std::cout << "Session End!" << "\n";
};


/* ----------------------- main() ----------------------- */
void print_help(void)
{
        std::cout << "\n Usage: " << app_name << " [OPTIONS]\n\n";
        std::cout << "  Options:" << "\n";
        std::cout << "   -h --help                             Print this help" << "\n";
        std::cout << "   p2p mode" << "\n";
        std::cout << "      -w --wait <port>                   Wait for call on certain port" << "\n";
        std::cout << "      -c --call <target_host>:<port>     Call somebody" << "\n";
        std::cout << "   client mode" << "\n";
        std::cout << "      -s --server <server_host>:<port>   STUN/TURN server address" << "\n";
        //std::cout << "   -u --username <user_name>          User name for login" << "\n";
        //std::cout << "   -p --password <pass_word>          User password for login" << "\n";
        std::cout << "      -w --wait                          Wait for call" << "\n";
        std::cout << "      -c --call <username>               Call somebody" << "\n";
        std::cout << "\n";
};


int main(int argc, char *argv[])
{
        static struct option long_options[] = {
                {"server", required_argument, 0, 's'},
                {"call", required_argument, 0, 'c'},
                {"wait", no_argument, 0, 'w'},
                {"help", no_argument, 0, 'h'},
                {0, 0, 0, 0}
        };
        int value, option_index = 0;
        opterr = 0;

        int portno;
        int local_port;
        char hostname[16];

        app_name = argv[0];

        /* Try to process all command line arguments */
        while ((value = getopt_long(argc, argv, "s:c:wh", long_options, &option_index)) != -1)
        {
                switch (value)
                {
                case 's':
                        client_mode = 1;
                        sscanf(callee_name, "%[0-9.]:%d", hostname, &portno);
                        assert(strlen(hostname)<16);
                        assert(portno<65535 || portno>0);
                        break;
                case 'c':
                        caller_mode = 1;
                        callee_name = strdup(optarg);
                        break;
                case 'w':
                        wait_mode = 1;
                        break;
                case 'h':
                        print_help();
                        return EXIT_SUCCESS;
                case '?':
                        print_help();
                        return EXIT_FAILURE;
                default:
                        break;
                }
        }


        /* initialize */
        int n;
        char buf[BUFSIZE];
        int serverlen;

        if (client_mode==0)
        {
                p2p_mode = 1;

                local_port = bind_udp(&s);
                if (local_port==-1)
                {
                        //error opening socket
                        return EXIT_FAILURE;
                }


                if (wait_mode==1 && caller_mode==0)
                {
                        std::cout << "My udp port is: " << local_port << "\n";
                        std::cout << "Wait for a call!" << "\n";
                        wait_call(&s);
                }
                else if (caller_mode==1 && wait_mode==0)
                {
                        sscanf(callee_name, "%[0-9.]:%d", hostname, &portno);
                        assert(strlen(hostname)<16 || strlen(hostname)>6);
                        assert(portno<65535 || portno>0);

                        printf("Target address: %s %d\n", hostname, portno);
                        if (build_addr(hostname, portno, &s)==-1)
                        {
                                //error hostname
                                return EXIT_FAILURE;
                        }
                        if (make_call(&s)==-1)
                        {
                                fprintf(stderr, "No answer to the call!\n");
                                return EXIT_FAILURE;
                        }
                }
                else
                {
                        print_help();
                        return EXIT_FAILURE;
                }

        }
        else
        {
                //TODO: client mode
        }

        /* After server tell u ready, go to the main proc */
        printf("Start Videochat!\n");
        Process(&s);

        return 0;
}










//end
