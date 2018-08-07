#include "opencv2/opencv.hpp"
#include "video.h"

#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>


using namespace std;
using namespace cv;

int main(int argc, char** argv)
{
        VideoCapture cap;
        // open the default camera, use something different from 0 otherwise;
        // Check VideoCapture documentation.
        if(!cap.open(0))
                return 0;


        Mat frame;
        cap >> frame;

        char GrayMark[9] = " .;o*8$@";
        struct winsize w;
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

        char* graylvl;
        int outlen;
        mat2grayim(frame, GrayMark, w, graylvl, &outlen, GET_LVL);
        printf("mat2grayim success!\n");

        //test encode and decode
        char* encimage;
        int enclen;
        gray_encode(GrayMark, graylvl, outlen, encimage, &enclen);
        printf("gray_encode success!\n");

        char* grayimage_dec;
        int outlen_dec;
        gray_decode(GrayMark, encimage, enclen, grayimage_dec, &outlen_dec);
        printf("gray_decode success!\n");

        printf("%s\n", grayimage_dec);
        printf("short to %d\n", enclen);
        printf("%d == %d\n", outlen, outlen_dec);


        // the camera will be closed automatically upon exit
        return 0;
}
