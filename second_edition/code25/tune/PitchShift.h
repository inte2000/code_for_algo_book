#pragma once

#define M_PI 3.14159265358979323846
#define MAX_FRAME_LENGTH 8192

typedef struct tagPitchContext
{
	float gInFIFO[MAX_FRAME_LENGTH];
	float gOutFIFO[MAX_FRAME_LENGTH];
	float gFFTworksp[2 * MAX_FRAME_LENGTH];
	float gLastPhase[MAX_FRAME_LENGTH / 2 + 1];
	float gSumPhase[MAX_FRAME_LENGTH / 2 + 1];
	float gOutputAccum[2 * MAX_FRAME_LENGTH];
	float gAnaFreq[MAX_FRAME_LENGTH];
	float gAnaMagn[MAX_FRAME_LENGTH];
	float gSynFreq[MAX_FRAME_LENGTH];
	float gSynMagn[MAX_FRAME_LENGTH];
	long gRover;
}PITCH_CONTEXT;

void InitPitchContext(PITCH_CONTEXT* ctx);

void smbPitchShift(PITCH_CONTEXT* ctx, float pitchShift, long numSampsToProcess, long fftFrameSize, long osamp, float sampleRate, float* indata, float* outdata);
