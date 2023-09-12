// stre.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>

#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"


int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        std::cout << "usage: stre.exe infile.wav outfile.wav" << std::endl;
        return -1;
    }
    drwav inwav, outwav;
    if (!drwav_init_file(&inwav, argv[1], NULL)) 
    {
        std::cout << "can't open input wav file: " << argv[1] << std::endl;
        return -1;
    }

    drwav_data_format format;
    format.container = drwav_container_riff;     // <-- drwav_container_riff = normal WAV files, drwav_container_w64 = Sony Wave64.
    format.format = DR_WAVE_FORMAT_PCM;          // <-- Any of the DR_WAVE_FORMAT_* codes.
    format.channels = inwav.channels;
    format.sampleRate = inwav.sampleRate / 2;
    format.bitsPerSample = inwav.bitsPerSample;
    if (!drwav_init_file_write(&outwav, argv[2], &format, NULL))
    {
        drwav_uninit(&inwav);
        std::cout << "can't open output wav file: " << argv[2] << std::endl;
        return -1;
    }

    unsigned int frameBytes = (inwav.bitsPerSample * inwav.channels) / 8;
    unsigned char* pInBuf = new (std::nothrow) unsigned char[frameBytes * 8192];
    if (pInBuf != nullptr)
    {
        while (true)
        {
            drwav_uint64 framesRead = drwav_read_pcm_frames(&inwav, 8192, pInBuf);
            if (framesRead > 0)
            {
                drwav_uint64 framesWritten = drwav_write_pcm_frames(&outwav, framesRead, pInBuf);
                if (framesWritten != framesRead)
                {
                    std::cout << "fail to write out data!" << std::endl;
                    break;
                }
            }
            if (framesRead < 8192)
            {
                std::cout << "trans format successfully!" << std::endl;
                break;
            }
        }

        delete[] pInBuf;
    }

    drwav_uninit(&inwav);
    drwav_uninit(&outwav);
}

