#include "opencv2/opencv.hpp"
#include "video.h"

#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>

#ifndef GET_IMAGE
#define GET_IMAGE 1
#endif

#ifndef GET_LVL
#define GET_LVL 0
#endif

using namespace std;
using namespace cv;


int main(int argc, char** argv)
{
        VideoCapture cap;
        // open the default camera, use something different from 0 otherwise;
        // Check VideoCapture documentation.
        if(!cap.open(0))
                return 0;

        int rate = 2; //reduce frame rate to 1/rate
        int tmp = 0;

        char GrayMark[9] = " .;o*8$@";
        for(;;)
        {
                Mat frame;
                cap >> frame;
                if( frame.empty() ) break; // end of video stream
                //imshow("this is you, smile! :)", frame);

                tmp += 1;
                if (tmp%rate == 0)
                {
                    struct winsize w;
                    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

                    char* grayimage;
                    int outlen;
                    mat2grayim(frame, GrayMark, w, grayimage, &outlen, GET_IMAGE);

                    /*
                    TODO: encode and decode have bug
                    */
                    
                    char* encimage;
                    int enclen;
                    gray_encode(GrayMark, grayimage, outlen, encimage, &enclen);


                    char* grayimage_dec;
                    int outlen_dec;
                    gray_decode(GrayMark, encimage, enclen, grayimage_dec, &outlen_dec);


                    //printf("%s\n", grayimage_dec);
                    moveCursor(std::cout, 0, 0);
                    puts(grayimage);

                    tmp = 0;
                }
                //if( waitKey(10) == 27 ) break; // stop capturing by pressing ESC
                //break;
        }

        // the camera will be closed automatically upon exit
        return 0;
}
