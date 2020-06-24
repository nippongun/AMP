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
static int inputCallback(const void *input,
                         void *output,
                         unsigned long framesPerBuffer,
                         const PaStreamCallbackTimeInfo *timeInfo,
                         PaStreamCallbackFlags statusFlags,
                         void *userData)
{
    paTestData *data = (paTestData *)userData;
    // Reading and writing get their own buffer to be processed and not mixed for other threads
    const SAMPLE *readSample = (const SAMPLE *)input;
    SAMPLE *writeSample = &data->recordSamples[data->frameIndex * NUM_CHANNELS];
    long framesToCalc;
    long i;
    int finished;
    unsigned long framesLeft = data->maxFrameIndex - data->frameIndex;
    if (framesLeft < framesPerBuffer)
    {
        framesToCalc = framesLeft;
        finished = paComplete;
    }
    else
    {
        framesToCalc = framesPerBuffer;
        finished = paContinue;
    }

    // In case the input is empty, this prevents the data to be
    // misused
    if (input == NULL)
    {
        for (i = 0; i < framesToCalc; i++)
        {

            *writeSample++ = SAMPLE_SILENCE;
            if (NUM_CHANNELS == 2)
            {
                *writeSample++ = SAMPLE_SILENCE;
            }
        }
    }
    else
    {
        for (i = 0; i < framesToCalc; i++)
        {
            *writeSample++ = *readSample++;
            if (NUM_CHANNELS == 2)
            {
                *writeSample++ = *readSample++;
            }
        }
    }

    data->frameIndex += framesToCalc;
    return finished;
}

static int outputCallback(const void *input,
                          void *output,
                          unsigned long framesPerBuffer,
                          const PaStreamCallbackTimeInfo* timeInfo,
                          PaStreamCallbackFlags statusFlags,
                          void *userData)
{
    paTestData *data = (paTestData *)userData;
    SAMPLE *readSample = &data->recordSamples[data->frameIndex * NUM_CHANNELS];
    SAMPLE *writeSample = (SAMPLE *)output;
    unsigned int i;
    int finished;
    unsigned int framesLeft = data->maxFrameIndex - data->frameIndex;

    if (framesLeft < framesPerBuffer)
    {
        for (i = 0; i < framesLeft; i++)
        {
            *writeSample++ = *readSample++;
            if (NUM_CHANNELS == 2)
            {
                *writeSample++ = *readSample++;
            }
        }
        for (; i < framesPerBuffer; i++)
        {
            *writeSample++ = 0;
            if (NUM_CHANNELS == 2)
            {
                *writeSample++ = 0;
            }
        }
        data->frameIndex += framesLeft;
        finished = paComplete;
    }
    else
    {
        for (i = 0; i < framesPerBuffer; i++)
        {
            *writeSample++ = *readSample++;
            if (NUM_CHANNELS == 2)
            {
                *writeSample++ = *readSample++;
            }
        }
        data->frameIndex += framesPerBuffer;
        finished = paContinue;
    }
    return finished;
}

int main (void);
int main (void){
    PaStreamParameters inputParameters, outputParameters;
    PaStream* stream;

    PaError err = paNoError;
    paTestData data;
    int i;
    int totalFrames;
    int numSamples;
    int numBytes;
    SAMPLE max, val;
    double average;
    fflush(stdout);

    data.maxFrameIndex = totalFrames = NUM_SECONDS * SAMPLE_RATE;
    data.frameIndex = 0;
    numSamples = totalFrames * NUM_CHANNELS;
    numBytes = numSamples * sizeof(SAMPLE);
    data.recordSamples = (SAMPLE *) malloc(numBytes);
    if (data.recordSamples == NULL){
        printf("Could not allocate memory.\n");
        goto done;
    }
    for ( i = 0; i < numSamples; i++)
    {
        data.recordSamples[i] = 0;
    }

    err = Pa_Initialize();

    if(err != paNoError){
        goto done;
    }

    inputParameters.device = Pa_GetDefaultInputDevice();
    if(inputParameters.device == paNoDevice){
        fprintf(stderr,"Error: No default device found");
        goto done;
    }    

    inputParameters.channelCount = 2;
    inputParameters.sampleFormat = PA_SAMPLE_TYPE;
    inputParameters.suggestedLatency = Pa_GetDeviceInfo(inputParameters.device)->defaultLowInputLatency;
    inputParameters.hostApiSpecificStreamInfo = NULL;


    err = Pa_OpenStream(&stream,
        &inputParameters,
        NULL,
        SAMPLE_RATE,
        FRAMES_PER_BUFFER,
        paClipOff,
        inputCallback,
        &data);

    if(err != paNoError){
        goto done;
    }
    err = Pa_StartStream( stream );

    if(err != paNoError){
        goto done;
    }

    fflush(stdout);

    while((err = Pa_IsStreamActive(stream))== 1){
        Pa_Sleep(1000);
        printf("index = %d\n",data.frameIndex);
        fflush(stdout);   
    }

    if( err < 0 ) goto done;

    err = Pa_CloseStream(stream);
    if ( err != paNoError) {
        goto done;
    }

    max = 0;
    average = 0.0;
    for ( i = 0; i < numSamples; i++)
    {
        val = data.recordSamples[i];
        if(val < 0 ){
            val = -val;
        }
        if( val > max){
            max = val;
        }
        average += val;
    }
    average = average / (double)numSamples;

#if WRITE_TO_FILE
    {
        FILE *file;
        file = fopen("record.raw","wb");
        if(file == NULL){
            printf("Could not open file");
        } else {
            fwrite (data.recordSamples,NUM_CHANNELS * sizeof(SAMPLE),totalFrames, file);
            fclose(file);
            printf("Wrote data to file");
        }
    }
#endif

    data.frameIndex = 0;

    outputParameters.device = Pa_GetDefaultOutputDevice();
    if(outputParameters.device == paNoDevice){
        fprintf(stderr,"Error: No default output device found");
        goto done;
    }

    outputParameters.channelCount = 2;
    outputParameters.sampleFormat = PA_SAMPLE_TYPE;
    outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;

    printf("Playback");
    fflush(stdout);

    err = Pa_OpenStream(&stream,
    NULL,
    &outputParameters,
    SAMPLE_RATE,
    FRAMES_PER_BUFFER,paClipOff,
    outputCallback,
    &data);

    if (err != paNoError){
        goto done;
    }

    if(stream){
        err = Pa_StartStream(stream);
        if(err != paNoError){
            goto done;
        }

        while((err = Pa_IsStreamActive(stream)== 1)){
            Pa_Sleep(1000);
        }

        if(err < 0){
            goto done;
        }

        err = Pa_CloseStream(stream);
        if(err != paNoError){
            goto done;
        }

        printf("Done");
        fflush(stdout);
    }
done:
    Pa_Terminate();
    if(data.recordSamples){
        free(data.recordSamples);
    }
    if( err != paNoError){
        fprintf( stderr, "An error occured while using portaudio\n");
        fprintf(stderr, "Error number %d\n", err);
        fprintf(stderr, "Erro message: %s\n", Pa_GetErrorText(err));
        err = 1;
    }

    return err;
}