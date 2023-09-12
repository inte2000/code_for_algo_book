// tune.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <cmath>

#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"

#include "PitchShift.h"

class CWavData;

bool OpenWavFile(const char* filename, CWavData& wavData);
bool WriteWavFile(const char* filename, const CWavData& wavData);

class CWavData
{
	bool OpenWavFile(const char* filename, CWavData& wavData);
	bool WriteWavFile(const char* filename, const CWavData& wavData);
public:
	CWavData()
	{
		m_pDataBuf = NULL;
		m_totalSamples = 0;
		m_sampleRate = 0;
		m_channels = 0;
	}
	CWavData(drwav* wav)
	{
		InitFromDrWave(wav);
	}
	CWavData(size_t totalSamples, int sampleRate, int channels)
	{
		size_t totalSize = totalSamples * sizeof(float) * channels;
		m_pDataBuf = new float[totalSize];
		if (m_pDataBuf != NULL)
		{
			memset(m_pDataBuf, 0, totalSize);
			m_totalSamples = totalSamples;
			m_sampleRate = sampleRate;
			m_channels = channels;
		}
	}
	virtual ~CWavData()
	{
		Release();
	}
	bool IsValid() const { return (m_pDataBuf != NULL); }

	void InitFromDrWave(drwav* wav)
	{
		assert(m_pDataBuf == NULL);

		size_t totalSize = (size_t)wav->totalPCMFrameCount * sizeof(float) * wav->channels;
		m_pDataBuf = new float[totalSize];
		if (m_pDataBuf != NULL)
		{
			memset(m_pDataBuf, 0, totalSize);
			m_totalSamples = (size_t)wav->totalPCMFrameCount;
			m_sampleRate = wav->sampleRate;
			m_channels = wav->channels;
		}
	}
	//channel 0: first channel, 1: second channel
	size_t ReadFramess(size_t pos, size_t framesToRead, int channel, float* outBuf, size_t bufSzie)
	{
		if ((m_pDataBuf == NULL) || ((framesToRead * sizeof(float)) > bufSzie))
			return 0;

		size_t thisRead = (m_totalSamples - pos) > framesToRead ? framesToRead : (m_totalSamples - pos);
		float* pStart = m_pDataBuf + (pos * m_channels);
		if (m_channels == 1)
		{
			memcpy(outBuf, pStart, thisRead * sizeof(float));
		}
		else
		{
			for (size_t i = 0; i < thisRead; i++)
			{
				*outBuf++ = *(pStart + channel);
				pStart += m_channels;
			}
		}

		return thisRead;
	}
	size_t WriteFramess(size_t pos, size_t framesToWrite, int channel, float* inBuf)
	{
		if (m_pDataBuf == NULL)
			return 0;

		size_t thisWrite = (m_totalSamples - pos) > framesToWrite ? framesToWrite : (m_totalSamples - pos);
		float* pStart = m_pDataBuf + (pos * m_channels);
		if (m_channels == 1)
		{
			memcpy(pStart, inBuf, thisWrite * sizeof(float));
		}
		else
		{
			for (size_t i = 0; i < thisWrite; i++)
			{
				*(pStart + channel) = *inBuf++;
				pStart += m_channels;
			}
		}

		return thisWrite;
	}

	void Release()
	{
		if (m_pDataBuf != NULL)
		{
			delete[] m_pDataBuf;
		}
		m_sampleRate = 0;
		m_channels = 0;
		m_totalSamples = 0;
	}
	size_t GetTotalSamples() const { return m_totalSamples; }
	int GetSampleRate() const { return m_sampleRate; }
	int GetBitsPerSample() const { return 32; }
	int GetChannels() const { return m_channels; }
	const float* GetRawBuffer() const { return m_pDataBuf; }
	float* GetRawBuffer() { return m_pDataBuf; }

protected:
	float* m_pDataBuf;
	int m_sampleRate;
	int m_channels;
	size_t m_totalSamples;
};

bool OpenWavFile(const char* filename, CWavData& wavData)
{
	drwav wav;
	if (!drwav_init_file(&wav, filename, NULL))
		return false;

	wavData.InitFromDrWave(&wav);
	drwav_uint64 readSamples = drwav_read_pcm_frames_f32(&wav, wavData.GetTotalSamples(), wavData.GetRawBuffer());
	drwav_uninit(&wav);
	return (readSamples == wavData.GetTotalSamples());
}

bool WriteWavFile(const char* filename, const CWavData& wavData)
{
	drwav_data_format format;
	format.container = drwav_container_riff;     // <-- drwav_container_riff = normal WAV files, drwav_container_w64 = Sony Wave64.
	format.format = DR_WAVE_FORMAT_IEEE_FLOAT;          // <-- Any of the DR_WAVE_FORMAT_* codes.
	format.channels = wavData.GetChannels();
	format.sampleRate = (drwav_uint32)wavData.GetSampleRate();
	format.bitsPerSample = wavData.GetBitsPerSample();

	drwav wav;
	if (drwav_init_file_write(&wav, filename, &format, NULL))
	{
		drwav_uint64 samplesWritten = drwav_write_pcm_frames(&wav, wavData.GetTotalSamples(), wavData.GetRawBuffer());
		drwav_uninit(&wav);
		if (samplesWritten == wavData.GetTotalSamples())
		{
			return true;
		}
	}

	return false;
}

int main(int argc, char* argv[])
{
	if (argc < 3)
	{
		std::cout << "usage: tune.exe infile.wav outfile.wav" << std::endl;
		return -1;
	}

	int numChannels = 1;
	size_t bufferLengthFrames = MAX_FRAME_LENGTH;
	int ret = 0;
	long semitones = 8;							// shift up by 3 semitones
	float pitchShift = (float)std::pow(2.0, semitones / 12.0);	// convert semitones to factor

	CWavData inWav;
	bool bLoaded = OpenWavFile(argv[1], inWav);

	CWavData outWav(inWav.GetTotalSamples(), inWav.GetSampleRate(), inWav.GetChannels());

	PITCH_CONTEXT pitch_ctx_left, pitch_ctx_right;
	InitPitchContext(&pitch_ctx_left);
	InitPitchContext(&pitch_ctx_right);

	int nSampleRate = inWav.GetSampleRate();
	float* DataBuf = new (std::nothrow) float[bufferLengthFrames];
	if (DataBuf != nullptr)
	{
		size_t thisPos = 0;
		while (true)
		{
			size_t framesRead = inWav.ReadFramess(thisPos, bufferLengthFrames, 0, DataBuf, bufferLengthFrames * sizeof(float));
			//framesRead = inWav.ReadFramess(thisPos, bufferLengthFrames, 1, DataBuf, bufferLengthFrames * sizeof(float));
			if (framesRead > 0)
			{
				smbPitchShift(&pitch_ctx_left, pitchShift, framesRead, 2048, 4, nSampleRate, DataBuf, DataBuf);
				size_t framesWritten = outWav.WriteFramess(thisPos, framesRead, 0, DataBuf);
				if (framesWritten != framesRead)
				{
					std::cout << "fail to write out data!" << std::endl;
					break;
				}

				thisPos += framesRead;
			}
			if (framesRead < bufferLengthFrames)
			{
				std::cout << "trans format successfully!" << std::endl;
				WriteWavFile(argv[2], outWav);
				break;
			}
		}

		delete[] DataBuf;
	}

	return 0;
}

