#include "sam.h"
#include "render.h"
#include "rendertabs.h"

static void CombineGlottalAndFormants(unsigned char phase1, unsigned char phase2, unsigned char phase3, unsigned char Y)
{
    unsigned int tmp;

    tmp   = multtable[sinus[phase1]     | amplitude1[Y]];
    tmp  += multtable[sinus[phase2]     | amplitude2[Y]];
    tmp  += tmp > 255 ? 1 : 0; // if addition above overflows, we for some reason add one;
    tmp  += multtable[rectangle[phase3] | amplitude3[Y]];
    tmp  += 136;
    tmp >>= 4; // Scale down to 0..15 range of C64 audio.
            
    Output(0, tmp & 0xf);
}

// PROCESS THE FRAMES
//
// In traditional vocal synthesis, the glottal pulse drives filters, which
// are attenuated to the frequencies of the formants.
//
// SAM generates these formants directly with sin and rectangular waves.
// To simulate them being driven by the glottal pulse, the waveforms are
// reset at the beginning of each glottal pulse.
//
void ProcessFrames(unsigned char frame_count)
{

    unsigned char speedcounter = 72;
    unsigned char phase1 = 0;
    unsigned char phase2 = 0;
    unsigned char phase3 = 0;
    unsigned char mem66;
    
    unsigned char Y = 0;

    unsigned char glottal_pulse = pitches[0];
    unsigned char glottal_pulse_75 = glottal_pulse - (glottal_pulse >> 2); // mem44 * 0.75

    while(frame_count) {
        unsigned char flags = sampledConsonantFlag[Y];
        
        // unvoiced sampled phoneme?
        if(flags & 248) {
            RenderSample(&mem66, flags,Y);
            // skip ahead two in the phoneme buffer
            Y += 2;
            frame_count -= 2;
            speedcounter = speed;
        } else {
            CombineGlottalAndFormants(phase1, phase2, phase3, Y);

            speedcounter--;
            if (speedcounter == 0) { 
                Y++; //go to next amplitude
                // decrement the frame count
                frame_count--;
                if(frame_count == 0)  return;
                speedcounter = speed;
            }
         
            --glottal_pulse;
        
            if(glottal_pulse != 0) {
                // not finished with a glottal pulse

                --glottal_pulse_75;
                // within the first 75% of the glottal pulse?
                // is the count non-zero and the sampled flag is zero?
                if((glottal_pulse_75 != 0) || (flags == 0)) {
                    // reset the phase of the formants to match the pulse
                    phase1 += frequency1[Y];
                    phase2 += frequency2[Y];
                    phase3 += frequency3[Y];
                    continue;
                }
                
                // voiced sampled phonemes interleave the sample with the
                // glottal pulse. The sample flag is non-zero, so render
                // the sample for the phoneme.
                RenderSample(&mem66, flags, Y);
            }
        }

        glottal_pulse = pitches[Y];
	glottal_pulse_75 = glottal_pulse - (glottal_pulse >> 2); // mem44 * 0.75

        // reset the formant wave generators to keep them in 
        // sync with the glottal pulse
        phase1 = 0;
        phase2 = 0;
        phase3 = 0;
    }
}

