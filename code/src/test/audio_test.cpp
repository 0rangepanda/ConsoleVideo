#include "portaudio/portaudio.h"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include <iostream>
#include <thread>
#include <mutex>



//#define SAMPLE_RATE  (44100)
#define SAMPLE_RATE  (10000)
#define FRAMES_PER_BUFFER (1024)
#define NUM_SECONDS     (5)
#define NUM_CHANNELS    (2)

#define DITHER_FLAG     (0) /**/

#if 1
#define PA_SAMPLE_TYPE  paFloat32
typedef float SAMPLE;
#define SAMPLE_SILENCE  (0.0f)
#define PRINTF_S_FORMAT "%.8f"
#endif 1


static int running;
static sigset_t signal_set;
std::mutex m;

static PaStreamParameters inputParameters, outputParameters;
static PaStream *stream_in;
static PaStream *stream_out;
static PaError err;
static SAMPLE *recordedSamples;
static int totalFrames;
static int numSamples;
static int numBytes;

/* record sound */
void inc_thread()
{
        err = Pa_OpenStream(
                &stream_in,
                &inputParameters,
                NULL, /* &outputParameters, */
                SAMPLE_RATE,
                FRAMES_PER_BUFFER,
                paClipOff, /* we won't output out of range samples so don't bother clipping them */
                NULL, /* no callback, use blocking API */
                NULL ); /* no callback, so no callback userData */
        if( err != paNoError )
                fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( err ) );

        err = Pa_StartStream( stream_in );
        if( err != paNoError )
                fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( err ) );
        printf("Now recording!!\n"); fflush(stdout);

        while (running)
        {
                m.lock();
                err = Pa_ReadStream( stream_in, recordedSamples, totalFrames );
                if( err != paNoError )
                        fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( err ) );
                m.unlock();

        }
        err = Pa_CloseStream( stream_in );
        if( err != paNoError )
                fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( err ) );
};

/* play sound */
void out_thread()
{
        err = Pa_OpenStream(
                &stream_out,
                NULL, /* no input */
                &outputParameters,
                SAMPLE_RATE,
                FRAMES_PER_BUFFER,
                paClipOff, /* we won't output out of range samples so don't bother clipping them */
                NULL, /* no callback, use blocking API */
                NULL ); /* no callback, so no callback userData */
        if( err != paNoError )
                fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( err ) );

        err = Pa_StartStream( stream_out );
        if( err != paNoError )
                fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( err ) );
        printf("Waiting for playback to finish.\n"); fflush(stdout);

        while (running)
        {
                m.lock();
                err = Pa_WriteStream( stream_out, recordedSamples, totalFrames );
                if( err != paNoError )
                        fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( err ) );
                m.unlock();
        }

        err = Pa_CloseStream( stream_out );
        if( err != paNoError )
                fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( err ) );
        printf("Done.\n"); fflush(stdout);
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

static
void Process()
{
        //totalFrames = NUM_SECONDS * SAMPLE_RATE; /* Record for a few seconds. */
        totalFrames = SAMPLE_RATE / 10;
        numSamples = totalFrames * NUM_CHANNELS;

        numBytes = numSamples * sizeof(SAMPLE);
        recordedSamples = (SAMPLE *) malloc( numBytes );
        if( recordedSamples == NULL )
        {
                printf("Could not allocate record array.\n");
                exit(1);
        }
        for(int i=0; i<numSamples; i++ ) recordedSamples[i] = 0;

        err = Pa_Initialize();
        if( err != paNoError )
                fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( err ) );

        inputParameters.device = Pa_GetDefaultInputDevice(); /* default input device */
        if (inputParameters.device == paNoDevice) {
                fprintf(stderr,"Error: No default input device.\n");
                fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( err ) );
        }
        inputParameters.channelCount = NUM_CHANNELS;
        inputParameters.sampleFormat = PA_SAMPLE_TYPE;
        inputParameters.suggestedLatency = Pa_GetDeviceInfo( inputParameters.device )->defaultLowInputLatency;
        inputParameters.hostApiSpecificStreamInfo = NULL;

        outputParameters.device = Pa_GetDefaultOutputDevice(); /* default output device */
        if (outputParameters.device == paNoDevice) {
                fprintf(stderr,"Error: No default output device.\n");
                fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( err ) );
        }
        outputParameters.channelCount = NUM_CHANNELS;
        outputParameters.sampleFormat =  PA_SAMPLE_TYPE;
        outputParameters.suggestedLatency = Pa_GetDeviceInfo( outputParameters.device )->defaultLowOutputLatency;
        outputParameters.hostApiSpecificStreamInfo = NULL;

        /* Handling Ctrl+C */
        sigemptyset(&signal_set);
        sigaddset(&signal_set, SIGINT);
        sigprocmask(SIG_BLOCK, &signal_set, 0);

        running = 1;
        std::thread monitor(monitor_thread);
        std::thread inc(inc_thread);
        std::thread out(out_thread);

        inc.join();
        out.join();

};

int main(int argc, char* argv[])
{

        Process();
        return 0;
}
