#include <math.h>
#include <stdio.h>

#define PI 3.14159265358979323846
#define AMPLITUDE 1.0
#define FREQUENCY 5.0
#define SAMPLES 100
#define PHASE_SHIFT 0.0

int main()
{
	double t;
	double freq;
	double phase;
	double sample;

	freq = 2 * PI * FREQUENCY / SAMPLES;
	phase = PHASE_SHIFT;

	for (t = 0; t < SAMPLES; t++)
	{
		sample = AMPLITUDE * sin(freq * t + phase);
		printf("%f\n", sample);
	}

	return 0;
}
