#include "opencv2/opencv.hpp"
#include "opencv2/core/core.hpp"
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

        Mat mat;
        cap >> mat;

        //Mat mat(pCodecCtx->height, pCodecCtx->width, CV_8UC3, pFrameYUV->data[0], pFrameYUV->linesize[0]);

        imshow("frame", mat);

        if (!mat.empty()) {

                std::vector<uchar> data_encode;

                int res = imencode(".jpg", mat, data_encode);

                std::string str_encode(data_encode.begin(), data_encode.end());



//char *char_r = new char[str_encode.size()];



                char* char_r=(char *)malloc(sizeof(char) * (str_encode.size()));

                memcpy(char_r, str_encode.data(), sizeof(char) * (str_encode.size()));

                printf("length: %d\n", str_encode.size());



//strcpy(char_r, str_encode.data());



//char_r = const_cast<char*>(str_encode.data());

//char_r[str_encode.size() - 1] = '\0';

                Mat img_decode;

                std::string str_tmp(char_r, str_encode.size());



                if (str_tmp.compare(str_encode) == 0) {

                        printf("ok");

                }

                std::vector<uchar> data(str_tmp.begin(), str_tmp.end());

                img_decode = imdecode(data, CV_LOAD_IMAGE_COLOR);

                imshow("pic", img_decode);
        }

        getchar();

        // the camera will be closed automatically upon exit
        return 0;
}
