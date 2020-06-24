#include "portaudio.h"
#include <stdio.h>
#include <string.h>

#define FRAME_SIZE 1024
#define BUFFERSIZE (FRAME_SIZE*4)
#define SAMPLE_RATE = 100
float buffer[BUFFERSIZE];
float samplesPerFrame = FRAME_SIZE;                    

int main(void);
int main (void){
    fflush(stdout);

    PaStream *stream;
    PaError err;

    err = Pa_Initialize();
    if(err != paNoError){
        goto error;
    }
    // Here goes: err = Pa_OpenDefaultStream
    if( err != paNoError ) goto error;

    err = Pa_StopStream( stream );
    if( err != paNoError ) goto error;
    err = Pa_CloseStream( stream );
    if( err != paNoError ) goto error;
    Pa_Terminate();

    printf("Test finished.\n");

    return err;
error:
    Pa_Terminate();
    fprintf( stderr, "An error occured while using the portaudio stream\n" );
    fprintf( stderr, "Error number: %d\n", err );
    fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( err ) );
    return err;
}