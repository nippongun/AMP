#include <stdio.h>
#include <stdlib.h>
#include "portaudio.h"

#define SAMPLE_RATE 44100
#define FRAMES_PER_BUFFER 512
#define NUM_SECONDS 5
// Every second sample corresponds right side of stereo.g
#define NUM_CHANNELS 2

#define DITHER_FLAG 0
#define WRITE_TO_FILE 0

#define PA_SAMPLE_TYPE paFloat32
typedef float SAMPLE;
#define SAMPLE_SILENCE 0.0f
#define PRINT_S_FORMAT "%.8f"

typedef struct 
{
    int frameIndex;
    int maxFrameIndex;
    SAMPLE *recordSamples;
} paTestData;
/* inputCallback handles the inward stream and assigns the incoming data to individuals buffers.
** Also it is made sure that the data is verified and under circumstances nulled (silenced)
*/
static int inputCallback( const void *input,
                        void *output,
                        unsigned long framesPerBuffer,
                        const PaStreamCallbackTimeInfo* timeInfo,
                        void *userData ) {
    paTestData *data = (paTestData*) userData;
    // Reading and writing get their own buffer to be processed and not mixed for other threads
    const SAMPLE *readSample = (const SAMPLE*) input;
    SAMPLE *writeSample = &data->recordSamples[data->frameIndex * NUM_CHANNELS];
    long framesToCalc;
    long i;
    int finished;
    unsigned long framesLeft = data->maxFrameIndex - data->frameIndex;
    if(framesLeft < framesPerBuffer){
        framesToCalc = framesLeft;
        finished = paComplete;
    } else {
        framesToCalc = framesPerBuffer;
        finished = paContinue;
    }

    // In case the input is empty, this prevents the data to be 
    // misused
    if (input == NULL)
    {
        for ( i = 0; i < framesToCalc; i++)
        {

            *writeSample++ = SAMPLE_SILENCE;
            if (NUM_CHANNELS == 2)
            {
                *writeSample++ = SAMPLE_SILENCE;
            }
               
        }
    } else {
        for ( i = 0; i < framesToCalc; i++)
        {
            *writeSample++ = *readSample++;
            if(NUM_CHANNELS == 2) {
                *writeSample++ = *readSample++;
            }
        }
    }

    data->frameIndex += framesToCalc;
    return finished;   
}