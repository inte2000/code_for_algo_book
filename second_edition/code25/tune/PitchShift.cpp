#include <string.h>
#include <math.h>
#include <stdio.h>
#include "PitchShift.h"


void smbFft(float *fftBuffer, long fftFrameSize, long sign);
double smbAtan2(double x, double y);


void InitPitchContext(PITCH_CONTEXT * ctx)
{
	memset(ctx->gInFIFO, 0, MAX_FRAME_LENGTH * sizeof(float));
	memset(ctx->gOutFIFO, 0, MAX_FRAME_LENGTH * sizeof(float));
	memset(ctx->gFFTworksp, 0, 2 * MAX_FRAME_LENGTH * sizeof(float));
	memset(ctx->gLastPhase, 0, (MAX_FRAME_LENGTH / 2 + 1) * sizeof(float));
	memset(ctx->gSumPhase, 0, (MAX_FRAME_LENGTH / 2 + 1) * sizeof(float));
	memset(ctx->gOutputAccum, 0, 2 * MAX_FRAME_LENGTH * sizeof(float));
	memset(ctx->gAnaFreq, 0, MAX_FRAME_LENGTH * sizeof(float));
	memset(ctx->gAnaMagn, 0, MAX_FRAME_LENGTH * sizeof(float));
	ctx->gRover = 0;
}

void smbPitchShift(PITCH_CONTEXT* ctx, float pitchShift, long numSampsToProcess, long fftFrameSize, long osamp, float sampleRate, float *indata, float *outdata)
/*
	Routine smbPitchShift(). See top of file for explanation
	Purpose: doing pitch shifting while maintaining duration using the Short
	Time Fourier Transform.
	Author: (c)1999-2009 Stephan M. Bernsee <smb [AT] dspdimension [DOT] com>
*/
{
	double magn, phase, tmp, window, real, imag;
	double freqPerBin, expct;
	long i,k, qpd, index, inFifoLatency, stepSize, fftFrameSize2;

	/* set up some handy variables */
	fftFrameSize2 = fftFrameSize/2;
	stepSize = fftFrameSize/osamp;
	freqPerBin = sampleRate/(double)fftFrameSize;
	expct = 2.*M_PI*(double)stepSize/(double)fftFrameSize;
	inFifoLatency = fftFrameSize-stepSize;
	if (ctx->gRover == 0) ctx->gRover = inFifoLatency;

	/* main processing loop */
	for (i = 0; i < numSampsToProcess; i++){

		/* As long as we have not yet collected enough data just read in */
		ctx->gInFIFO[ctx->gRover] = indata[i];
		outdata[i] = ctx->gOutFIFO[ctx->gRover-inFifoLatency];
		ctx->gRover++;

		/* now we have enough data for processing */
		if (ctx->gRover >= fftFrameSize) {
			ctx->gRover = inFifoLatency;

			/* do windowing and re,im interleave */
			for (k = 0; k < fftFrameSize;k++) {
				window = -.5*cos(2.*M_PI*(double)k/(double)fftFrameSize)+.5;
				ctx->gFFTworksp[2*k] = ctx->gInFIFO[k] * window;
				ctx->gFFTworksp[2*k+1] = 0.;
			}


			/* ***************** ANALYSIS ******************* */
			/* do transform */
			smbFft(ctx->gFFTworksp, fftFrameSize, -1);

			/* this is the analysis step */
			for (k = 0; k <= fftFrameSize2; k++) {

				/* de-interlace FFT buffer */
				real = ctx->gFFTworksp[2*k];
				imag = ctx->gFFTworksp[2*k+1];

				/* compute magnitude and phase */
				magn = 2.*sqrt(real*real + imag*imag);
				phase = atan2(imag,real);

				/* compute phase difference */
				tmp = phase - ctx->gLastPhase[k];
				ctx->gLastPhase[k] = phase;

				/* subtract expected phase difference */
				tmp -= (double)k*expct;

				/* map delta phase into +/- Pi interval */
				qpd = tmp/M_PI;
				if (qpd >= 0) qpd += qpd&1;
				else qpd -= qpd&1;
				tmp -= M_PI*(double)qpd;

				/* get deviation from bin frequency from the +/- Pi interval */
				tmp = osamp*tmp/(2.*M_PI);

				/* compute the k-th partials' true frequency */
				tmp = (double)k*freqPerBin + tmp*freqPerBin;

				/* store magnitude and true frequency in analysis arrays */
				ctx->gAnaMagn[k] = magn;
				ctx->gAnaFreq[k] = tmp;

			}

			/* ***************** PROCESSING ******************* */
			/* this does the actual pitch shifting */
			memset(ctx->gSynMagn, 0, fftFrameSize*sizeof(float));
			memset(ctx->gSynFreq, 0, fftFrameSize*sizeof(float));
			for (k = 0; k <= fftFrameSize2; k++) { 
				index = k*pitchShift;
				if (index <= fftFrameSize2) { 
					ctx->gSynMagn[index] += ctx->gAnaMagn[k];
					ctx->gSynFreq[index] = ctx->gAnaFreq[k] * pitchShift;
				} 
			}
			
			/* ***************** SYNTHESIS ******************* */
			/* this is the synthesis step */
			for (k = 0; k <= fftFrameSize2; k++) {

				/* get magnitude and true frequency from synthesis arrays */
				magn = ctx->gSynMagn[k];
				tmp = ctx->gSynFreq[k];

				/* subtract bin mid frequency */
				tmp -= (double)k*freqPerBin;

				/* get bin deviation from freq deviation */
				tmp /= freqPerBin;

				/* take osamp into account */
				tmp = 2.*M_PI*tmp/osamp;

				/* add the overlap phase advance back in */
				tmp += (double)k*expct;

				/* accumulate delta phase to get bin phase */
				ctx->gSumPhase[k] += tmp;
				phase = ctx->gSumPhase[k];

				/* get real and imag part and re-interleave */
				ctx->gFFTworksp[2*k] = magn*cos(phase);
				ctx->gFFTworksp[2*k+1] = magn*sin(phase);
			} 

			/* zero negative frequencies */
			for (k = fftFrameSize+2; k < 2*fftFrameSize; k++) ctx->gFFTworksp[k] = 0.;

			/* do inverse transform */
			smbFft(ctx->gFFTworksp, fftFrameSize, 1);

			/* do windowing and add to output accumulator */ 
			for(k=0; k < fftFrameSize; k++) {
				window = -.5*cos(2.*M_PI*(double)k/(double)fftFrameSize)+.5;
				ctx->gOutputAccum[k] += 2.*window* ctx->gFFTworksp[2*k]/(fftFrameSize2*osamp);
			}
			for (k = 0; k < stepSize; k++) ctx->gOutFIFO[k] = ctx->gOutputAccum[k];

			/* shift accumulator */
			memmove(ctx->gOutputAccum, ctx->gOutputAccum+stepSize, fftFrameSize*sizeof(float));

			/* move input FIFO */
			for (k = 0; k < inFifoLatency; k++) ctx->gInFIFO[k] = ctx->gInFIFO[k+stepSize];
		}
	}
}

// -----------------------------------------------------------------------------------------------------------------


void smbFft(float *fftBuffer, long fftFrameSize, long sign)
/* 
	FFT routine, (C)1996 S.M.Bernsee. Sign = -1 is FFT, 1 is iFFT (inverse)
	Fills fftBuffer[0...2*fftFrameSize-1] with the Fourier transform of the
	time domain data in fftBuffer[0...2*fftFrameSize-1]. The FFT array takes
	and returns the cosine and sine parts in an interleaved manner, ie.
	fftBuffer[0] = cosPart[0], fftBuffer[1] = sinPart[0], asf. fftFrameSize
	must be a power of 2. It expects a complex input signal (see footnote 2),
	ie. when working with 'common' audio signals our input signal has to be
	passed as {in[0],0.,in[1],0.,in[2],0.,...} asf. In that case, the transform
	of the frequencies of interest is in fftBuffer[0...fftFrameSize].
*/
{
	float wr, wi, arg, *p1, *p2, temp;
	float tr, ti, ur, ui, *p1r, *p1i, *p2r, *p2i;
	long i, bitm, j, le, le2, k;

	for (i = 2; i < 2*fftFrameSize-2; i += 2) {
		for (bitm = 2, j = 0; bitm < 2*fftFrameSize; bitm <<= 1) {
			if (i & bitm) j++;
			j <<= 1;
		}
		if (i < j) {
			p1 = fftBuffer+i; p2 = fftBuffer+j;
			temp = *p1; *(p1++) = *p2;
			*(p2++) = temp; temp = *p1;
			*p1 = *p2; *p2 = temp;
		}
	}
	for (k = 0, le = 2; k < (long)(log(fftFrameSize)/log(2.)+.5); k++) {
		le <<= 1;
		le2 = le>>1;
		ur = 1.0;
		ui = 0.0;
		arg = M_PI / (le2>>1);
		wr = cos(arg);
		wi = sign*sin(arg);
		for (j = 0; j < le2; j += 2) {
			p1r = fftBuffer+j; p1i = p1r+1;
			p2r = p1r+le2; p2i = p2r+1;
			for (i = j; i < 2*fftFrameSize; i += le) {
				tr = *p2r * ur - *p2i * ui;
				ti = *p2r * ui + *p2i * ur;
				*p2r = *p1r - tr; *p2i = *p1i - ti;
				*p1r += tr; *p1i += ti;
				p1r += le; p1i += le;
				p2r += le; p2i += le;
			}
			tr = ur*wr - ui*wi;
			ui = ur*wi + ui*wr;
			ur = tr;
		}
	}
}


// -----------------------------------------------------------------------------------------------------------------

/*

    12/12/02, smb
    
    PLEASE NOTE:
    
    There have been some reports on domain errors when the atan2() function was used
    as in the above code. Usually, a domain error should not interrupt the program flow
    (maybe except in Debug mode) but rather be handled "silently" and a global variable
    should be set according to this error. However, on some occasions people ran into
    this kind of scenario, so a replacement atan2() function is provided here.
    
    If you are experiencing domain errors and your program stops, simply replace all
    instances of atan2() with calls to the smbAtan2() function below.
    
*/


double smbAtan2(double x, double y)
{
  double signx;
  if (x > 0.) signx = 1.;  
  else signx = -1.;
  
  if (x == 0.) return 0.;
  if (y == 0.) return signx * M_PI / 2.;
  
  return atan2(x, y);
}


// -----------------------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------------------
