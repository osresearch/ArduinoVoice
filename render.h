#ifndef RENDER_H
#define RENDER_H

void Render();
void SetMouthThroat(unsigned char mouth, unsigned char throat);

void ProcessFrames(unsigned char mem48);
void RenderSample(unsigned char *mem66, unsigned char consonantFlag, unsigned char mem49);
unsigned char CreateTransitions();

#define PHONEME_PERIOD (1)
#define PHONEME_QUESTION (2)

#define RISING_INFLECTION (1)
#define FALLING_INFLECTION (255)

extern unsigned char pitches[];

extern unsigned char frequency1[];
extern unsigned char frequency2[];
extern unsigned char frequency3[];

extern unsigned char amplitude1[];
extern unsigned char amplitude2[];
extern unsigned char amplitude3[];

extern unsigned char sampledConsonantFlag[];

#endif

