#include "video.h"

#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>

using namespace std;
using namespace cv;

/*
 * This function take ONE cv::frame as input, transform it to a gray image and
 * print it on the screen.
 */
int mat2gray(const Mat &frame)
{
        int GrayLvl = 8;
        char GrayMark[9] = " .;o*8$@";

        //to get console window size
        struct winsize w;
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

        int col_step = frame.cols / w.ws_col + 1;
        int row_step = frame.rows / w.ws_row +1;

        //printf ("col: %d, %d, %d\n", col_step, frame.cols, w.ws_col);
        //printf ("row: %d, %d, %d\n", row_step, frame.rows, w.ws_row);

        int* buffer = new int[w.ws_col+1]();
        char screen[w.ws_col*(w.ws_row+1)+1];
        //char screen[w.ws_col*(1+frame.rows / row_step)+1];
        char* p = screen;

        //An empty line
        *(p++) = '\n';


        for(int i = 0; i < frame.rows; i++)
        {
                //access the matrix element
                const Vec3b* rowData = frame.ptr<Vec3b>(i);
                for(int j = 0; j < frame.cols; j++)
                {
                        const Vec3b& pixel = rowData[j];
                        buffer[j/col_step] += (pixel[2]*30+pixel[1]*59+pixel[0]*11)/100;
                        //buffer[j/col_step] += rowData[j][1]; //can use only green to calculate gray-level
                }


                if((i%row_step == 0 || i==frame.rows-1) && i>0)
                {
                        //print one line
                        int* b = buffer;

                        for (int index = 0; index < (frame.cols-1)/col_step; ++index)
                        {
                                //int gray = buffer[index] / (col_step * row_step);
                                int gray = *b / (col_step * row_step);
                                gray /= (256/GrayLvl);
                                gray = (gray>7) ? 7 : gray;

                                //putchar(GrayMark[gray]);
                                *(b++) = 0;
                                *(p++) = GrayMark[gray];
                        }
                        if ((frame.cols-1)/col_step -1 < w.ws_col)
                        {
                                *(p++) = '\n';
                        }
                }
        }

        *(p++) = '\0';
        printf("%s\n", screen);
        free(buffer);
        return 1;
}


/*
 * This version uses linear interploation to make real print area exactly equal to
 * the whole console
 */
int mat2gray_float(const Mat &frame)
{
        int GrayLvl = 8;
        char GrayMark[9] = " .;o*8$@";

        //to get console window size
        struct winsize w;
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

        double col_step = (double) frame.cols / w.ws_col;
        int row_step = frame.rows / w.ws_row +1;

        //printf ("col: %f, %d, %d\n", col_step, frame.cols, w.ws_col);
        //printf ("row: %d, %d, %d\n", row_step, frame.rows, w.ws_row);

        int* buffer = new int[w.ws_col+1]();
        char screen[w.ws_col*(w.ws_row+1)+1];
        //char screen[w.ws_col*(1+frame.rows / row_step)+1];
        char* p = screen;

        for(int i = 0; i < frame.rows; i++)
        {
                //access the matrix element
                const Vec3b* rowData = frame.ptr<Vec3b>(i);
                int buf = 0;
                double step = 0.0;

                for(int j = 0; j < frame.cols; j++)
                {
                        //const Vec3b& pixel = rowData[j]; //mirror direction
                        const Vec3b& pixel = rowData[frame.cols-1-j];
                        int gray = (pixel[2]*30+pixel[1]*59+pixel[0]*11)/100;

                        step += 1.0;
                        if (step >= col_step)
                        {
                                buffer[buf] += gray * (col_step-step+1);
                                step -= col_step;
                                buf += 1;
                                buffer[buf] += gray * step;
                        }
                        else
                        {
                                buffer[buf] += gray;
                        }
                }
                //printf("%d   %d\n", buf, w.ws_col);


                if((i%row_step == 0 || i==frame.rows-1) && i>0)
                {
                        //print one line
                        int* b = buffer;

                        for (int index = 0; index < w.ws_col; ++index)
                        {
                                //int gray = buffer[index] / (col_step * row_step);
                                int gray = *b / (col_step * row_step);
                                gray /= (256/GrayLvl);
                                gray = (gray>7) ? 7 : gray;

                                *(b++) = 0;
                                *(p++) = GrayMark[gray];
                        }
                }
        }

        *(p++) = '\0';
        printf("%s\n", screen);

        free(buffer);
        return 1;
}

/*
 * This version uses linear interploation to make real print area exactly equal to
 * the whole console
 * This version do not print directly, but get the grayimage data.
 * Can choose data between graylvl and graymark use *getimage*
 */
int mat2grayim(const Mat &frame, const char GrayMark[], struct winsize w,
               char*& grayimage, int* outlen, int getimage)
{
        int GrayLvl = strlen(GrayMark);

        double col_step = (double) frame.cols / w.ws_col;
        int row_step = frame.rows / w.ws_row +1;

        int* buffer = new int[w.ws_col+1]();

        *outlen =  w.ws_col*(w.ws_row)+1;
        grayimage = (char*)malloc(sizeof(char) * *outlen);

        char* p = grayimage;
        for(int i = 0; i < frame.rows; i++)
        {
                //access the matrix element
                const Vec3b* rowData = frame.ptr<Vec3b>(i);
                int buf = 0;
                double step = 0.0;

                for(int j = 0; j < frame.cols; j++)
                {
                        //const Vec3b& pixel = rowData[j]; //mirror direction
                        const Vec3b& pixel = rowData[frame.cols-1-j];
                        int gray = (pixel[2]*30+pixel[1]*59+pixel[0]*11)/100;

                        step += 1.0;
                        if (step >= col_step)
                        {
                                buffer[buf] += gray * (col_step-step+1);
                                step -= col_step;
                                buf += 1;
                                buffer[buf] += gray * step;
                        }
                        else
                        {
                                buffer[buf] += gray;
                        }
                }


                if((i%row_step == 0 || i==frame.rows-1) && i>0)
                {
                        int* b = buffer;

                        for (int index = 0; index < w.ws_col; ++index)
                        {
                                int gray = *b / (col_step * row_step);
                                gray /= (256/GrayLvl);
                                gray = (gray>GrayLvl-1) ? GrayLvl-1 : gray;

                                *(b++) = 0;
                                if (getimage)
                                        *(p++) = GrayMark[gray]; //get the image
                                else
                                        *(p++) = gray; //get the graylvl data
                        }
                }
        }

        *(p++) = '\0';
        free(buffer);
        return 1;
};

/*
 *  Encode our grayimage to reduce network bandwidth
 *  e.p.: "****..;;;;;;  \0" -> "4*2.6;2 \0" ->
 *    1 Byte   |   1 Byte
 *    count    |  graylvl
 */
int gray_encode(const char GrayMark[], char* grayimage, int length, char*& output, int* outlen)
{
        int GrayLvl = strlen(GrayMark); //graylvl 0~255, so 1 byte is enough

        char* lastp = grayimage;
        char* pixel = grayimage;
        pixel++;
        int count = 1;

        output = (char*)malloc(sizeof(char) * length);
        char* out = output;
        int len = 0; //outlen

        for (int i = 0; i < length-1; ++i)
        {
                //putchar(*lastp);

                if (*(lastp) == *(pixel) && count<255)
                {
                        count++;
                }
                else
                {
                        *(out++) = count;  //count
                        *(out++) = *lastp; //graylvl
                        len += 2;
                        count = 1;
                }

                lastp++;
                pixel++;
        }

        *out = '\0';
        *outlen = len;
        return 1;
}

/*
 * Decoder
 */
int gray_decode(const char GrayMark[], char* encimage, int length, char*& output, int* outlen)
{
        int GrayLvl = strlen(GrayMark);

        /* get total length firstly */
        char* enc = encimage;
        int len = 0; //outlen

        for (int i = 0; i < length/2; ++i)
        {
                len += *(enc++);
                enc++;
        }

        output = (char*)malloc(sizeof(char) * (len+1));
        char* out = output;

        enc = encimage;
        for (int i = 0; i < length/2; ++i)
        {
                int count = *(enc++);
                int gray  = *(enc++);

                while (count)
                {
                        *(out++) = GrayMark[gray];
                        count--;
                }

        }

        *out = '\0';
        *outlen = len+1;
        return 1;
}



/*
 * Set cursor to position (col,row)
 */
void moveCursor(std::ostream& os, int col, int row)
{
        os << "\033[" << col << ";" << row << "H";
};

/*
 * print image
 */
void printimage(char* image, int size)
{
        moveCursor(std::cout, 0,0);
        puts(image);
};






//end
