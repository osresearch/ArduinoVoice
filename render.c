//#include <stdio.h>
//#include <string.h>
//#include <stdlib.h>

#include "sam.h"
#include "render.h"
#include "rendertabs.h"

unsigned char wait1 = 7;
unsigned char wait2 = 6;

unsigned char pitches[256]; // tab43008

unsigned char frequency1[256];
unsigned char frequency2[256];
unsigned char frequency3[256];

unsigned char amplitude1[256];
unsigned char amplitude2[256];
unsigned char amplitude3[256];

unsigned char sampledConsonantFlag[256]; // tab44800


void AddInflection(unsigned char mem48, unsigned char phase1, unsigned char X);

//return = hibyte(mem39212*mem39213) <<  1
unsigned char trans(unsigned char a, unsigned char b)
{
    unsigned int ai = a;
    unsigned int bi = b;

    return ((ai*bi) >> 8) << 1;
}





//timetable for more accurate c64 simulation
const int timetable[5][5] =
{
    {162, 167, 167, 127, 128},
    {226, 60, 60, 0, 0},
    {225, 60, 59, 0, 0},
    {200, 0, 0, 54, 55},
    {199, 0, 0, 54, 54}
};

void Output(int index, unsigned char A)
{
    static unsigned oldtimetableindex = 0;
    int k;
    bufferpos += timetable[oldtimetableindex][index];
    oldtimetableindex = index;
    // write a little bit in advance
    for(k=0; k<5; k++)
        buffer[bufferpos/50 + k] = (A & 0xF)*16;
}


static unsigned char
RenderVoicedSample(
	unsigned short hi,
	unsigned char off,
	unsigned char phase1
)
{
    for(phase1 ; phase1 != 0 ; phase1++, off++)
    {
        unsigned char sample = sampleTable[hi+off];
        for(unsigned bit = 0 ; bit < 8 ; bit++, sample <<= 1)
            if ((sample & 0x80) != 0)
	            Output(3, 26);
            else
                Output(4, 6);
    }

    return off;
}

static void
RenderUnvoicedSample(
    unsigned short hi,
    unsigned char off,
    unsigned char val
)
{
    for(off ; off != 0 ; off++)
    {
        unsigned char sample = sampleTable[hi+off];
        for(unsigned bit = 0 ; bit < 8 ; bit++, sample <<= 1)
            if ((sample & 0x80) != 0)
                Output(2, 5);
            else
                Output(1, val);
    }
}



// -------------------------------------------------------------------------
//Code48227
// Render a sampled sound from the sampleTable.
//
//   Phoneme   Sample Start   Sample End
//   32: S*    15             255
//   33: SH    257            511
//   34: F*    559            767
//   35: TH    583            767
//   36: /H    903            1023
//   37: /X    1135           1279
//   38: Z*    84             119
//   39: ZH    340            375
//   40: V*    596            639
//   41: DH    596            631
//
//   42: CH
//   43: **    399            511
//
//   44: J*
//   45: **    257            276
//   46: **
// 
//   66: P*
//   67: **    743            767
//   68: **
//
//   69: T*
//   70: **    231            255
//   71: **
//
// The SampledPhonemesTable[] holds flags indicating if a phoneme is
// voiced or not. If the upper 5 bits are zero, the sample is voiced.
//
// Samples in the sampleTable are compressed, with bits being converted to
// bytes from high bit to low, as follows:
//
//   unvoiced 0 bit   -> X
//   unvoiced 1 bit   -> 5
//
//   voiced 0 bit     -> 6
//   voiced 1 bit     -> 24
//
// Where X is a value from the table:
//
//   { 0x18, 0x1A, 0x17, 0x17, 0x17 };
//
// The index into this table is determined by masking off the lower
// 3 bits from the SampledPhonemesTable:
//
//        index = (SampledPhonemesTable[i] & 7) - 1;
//
// For voices samples, samples are interleaved between voiced output.


void
RenderSample(
    unsigned char *offset,
    unsigned char consonantFlag,
    unsigned char phoneme_index
)
{     
    // mask low three bits and subtract 1 get value to 
    // convert 0 bits on unvoiced samples.
    unsigned char hibyte = (consonantFlag & 7)-1;
    unsigned char pitch = consonantFlag & ~7;
    
    // determine which offset to use from table { 0x18, 0x1A, 0x17, 0x17, 0x17 }
    // T, S, Z                0          0x18
    // CH, J, SH, ZH          1          0x1A
    // P, F*, V, TH, DH       2          0x17
    // /H                     3          0x17
    // /X                     4          0x17

    unsigned short hi = hibyte*256;

    // voiced sample?
    if(pitch != 0) {
        RenderUnvoicedSample(hi, pitch^0xFF, tab48426[hibyte]);
    } else {
        // voiced phoneme: Z*, ZH, V*, DH
        pitch = pitches[phoneme_index] >> 4;
        *offset = RenderVoicedSample(hi, *offset, pitch ^ 0xFF);
    }
}



// CREATE FRAMES
//
// The length parameter in the list corresponds to the number of frames
// to expand the phoneme to. Each frame represents 10 milliseconds of time.
// So a phoneme with a length of 7 = 7 frames = 70 milliseconds duration.
//
// The parameters are copied from the phoneme to the frame verbatim.
//
static void CreateFrames()
{
    unsigned char phase1 = 0;

    unsigned char X = 0;
    for(unsigned i = 0 ; i < 256 ; i++)
    {
        // get the phoneme at the index
        unsigned char phoneme = phonemeIndexOutput[i];
    
        // if terminal phoneme, exit the loop
        if (phoneme == 0xFF)
            break;
    
        if (phoneme == PHONEME_PERIOD)
            AddInflection(RISING_INFLECTION, phase1, X);
        else
        if (phoneme == PHONEME_QUESTION)
            AddInflection(FALLING_INFLECTION, phase1, X);

        // get the stress amount (more stress = higher pitch)
        phase1 = tab47492[stressOutput[i] + 1];
    
        // get number of frames to write
        // and copy from the source to the frames list
        for(unsigned phase2 = phonemeLengthOutput[i] ; phase2 != 0 ; phase2--, X++)
        {
            frequency1[X] = freq1data[phoneme];     // F1 frequency
            frequency2[X] = freq2data[phoneme];     // F2 frequency
            frequency3[X] = freq3data[phoneme];     // F3 frequency
            amplitude1[X] = ampl1data[phoneme];     // F1 amplitude
            amplitude2[X] = ampl2data[phoneme];     // F2 amplitude
            amplitude3[X] = ampl3data[phoneme];     // F3 amplitude
            sampledConsonantFlag[X] = sampledConsonantFlags[phoneme];        // phoneme data for sampled consonants
            pitches[X] = pitch + phase1;      // pitch
        }
    }
}


// RESCALE AMPLITUDE
//
// Rescale volume from a linear scale to decibels.
//
void RescaleAmplitude() 
{
    for(unsigned i = 0 ; i < 256 ; i++)
    {
        amplitude1[i] = amplitudeRescale[amplitude1[i]];
        amplitude2[i] = amplitudeRescale[amplitude2[i]];
        amplitude3[i] = amplitudeRescale[amplitude3[i]];
    }
}



// ASSIGN PITCH CONTOUR
//
// This subtracts the F1 frequency from the pitch to create a
// pitch contour. Without this, the output would be at a single
// pitch level (monotone).

void AssignPitchContour()
{   
    for(unsigned i=0; i<256; i++)
    {
        // subtract half the frequency of the formant 1.
        // this adds variety to the voice
        pitches[i] -= (frequency1[i] >> 1);
    }
}


// RENDER THE PHONEMES IN THE LIST
//
// The phoneme list is converted into sound through the steps:
//
// 1. Copy each phoneme <length> number of times into the frames list,
//    where each frame represents 10 milliseconds of sound.
//
// 2. Determine the transitions lengths between phonemes, and linearly
//    interpolate the values across the frames.
//
// 3. Offset the pitches by the fundamental frequency.
//
// 4. Render the each frame.
extern void serial_print(int val,int width);
extern void serial_println();

void Render()
{
    if (phonemeIndexOutput[0] == 0xFF)
        return; //exit if no data

    CreateFrames();
    unsigned char len = CreateTransitions();

    if (!singmode)
        AssignPitchContour();

    RescaleAmplitude();

/*
    if (debug) {
        PrintOutput(sampledConsonantFlag, frequency1, frequency2, frequency3, amplitude1, amplitude2, amplitude3, pitches);
    }
*/
    if(0) for(unsigned i=0 ; i < len ; i++)
    {
        if (frequency1[i] == 0)
            break;
        serial_print(sampledConsonantFlag[i], 8);
        serial_print(amplitude1[i], 8);
        serial_print(frequency1[i], 8);

        serial_print(amplitude2[i], 8);
        serial_print(frequency2[i], 8);

        serial_print(amplitude3[i], 8);
        serial_print(frequency3[i], 8);
        serial_print(pitches[i], 8);
        serial_println();
    }


    ProcessFrames(len);
}


// Create a rising or falling inflection 30 frames prior to 
// index X. A rising inflection is used for questions, and 
// a falling inflection is used for statements.

void
AddInflection(
    unsigned char inflection,
    unsigned char phase1,
    unsigned char pos
)
{
    unsigned char A;
    // store the location of the punctuation
    unsigned char end = pos;

    if (pos < 30)
        pos = 0;
    else
        pos -= 30;

    // FIXME: Explain this fix better, it's not obvious
    // ML : A =, fixes a problem with invalid pitch with '.'
    while( (A = pitches[pos]) == 0x7F)
        ++pos;

    for(pos ; pos != end ; pos++)
    {
        if (pitches[pos] == 0xFF)
            continue;

        // add the inflection direction
        A += inflection;
    
        // set the inflection
        pitches[pos] = A;
    } 
}

/*
    SAM's voice can be altered by changing the frequencies of the
    mouth formant (F1) and the throat formant (F2). Only the voiced
    phonemes (5-29 and 48-53) are altered.
*/
void SetMouthThroat(unsigned char mouth, unsigned char throat)
{
    // mouth formants (F1) 5..29
    static const unsigned char mouthFormants5_29[30] = {
        0, 0, 0, 0, 0, 10,
        14, 19, 24, 27, 23, 21, 16, 20, 14, 18, 14, 18, 18,
        16, 13, 15, 11, 18, 14, 11, 9, 6, 6, 6};

    // throat formants (F2) 5..29
    static const unsigned char throatFormants5_29[30] = {
    255, 255,
    255, 255, 255, 84, 73, 67, 63, 40, 44, 31, 37, 45, 73, 49,
    36, 30, 51, 37, 29, 69, 24, 50, 30, 24, 83, 46, 54, 86,
    };

    // there must be no zeros in this 2 tables
    // formant 1 frequencies (mouth) 48..53
    static const unsigned char mouthFormants48_53[6] = {19, 27, 21, 27, 18, 13};
       
    // formant 2 frequencies (throat) 48..53
    static const unsigned char throatFormants48_53[6] = {72, 39, 31, 43, 30, 34};

    // recalculate formant frequencies 5..29 for the mouth (F1) and throat (F2)
    unsigned char newFrequency = 0;

    for(unsigned char pos = 5 ; pos < 30 ; pos++)
    {
        // recalculate mouth frequency
        unsigned char initialFrequency = mouthFormants5_29[pos];
        if (initialFrequency != 0)
            newFrequency = trans(mouth, initialFrequency);
        freq1data[pos] = newFrequency;
               
        // recalculate throat frequency
        initialFrequency = throatFormants5_29[pos];
        if(initialFrequency != 0)
            newFrequency = trans(throat, initialFrequency);
        freq2data[pos] = newFrequency;
    }

    // recalculate formant frequencies 48..53
    for(unsigned char pos = 0 ; pos < 6 ; pos++)
    {
        // recalculate F1 (mouth formant)
        unsigned char initialFrequency = mouthFormants48_53[pos];
        newFrequency = trans(mouth, initialFrequency);
        freq1data[pos+48] = newFrequency;
           
        // recalculate F2 (throat formant)
        initialFrequency = throatFormants48_53[pos];
        newFrequency = trans(throat, initialFrequency);
        freq2data[pos+48] = newFrequency;
    }
}


