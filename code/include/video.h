#ifndef VIDEO_H
#define VIDEO_H

#include "opencv2/opencv.hpp"


using namespace cv;


#ifndef GET_IMAGE
#define GET_IMAGE 1
#endif

#ifndef GET_LVL
#define GET_LVL 0
#endif


int mat2gray(const Mat &frame);
int mat2gray_float(const Mat &frame);
int mat2grayim(const Mat &frame, const char GrayMark[], struct winsize w,
               char*& grayimage, int* outlen, int getimage);

int gray_encode(const char GrayMark[], char* grayimage, int length, char*& output, int* outlen);
int gray_decode(const char GrayMark[], char* encimage, int length, char*& output, int* outlen);


void moveCursor(std::ostream& os, int col, int row);
void printimage(char* image, int size);


#endif
