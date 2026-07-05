#ifndef CORE_INC_DSP_H_
#define CORE_INC_DSP_H_

#include <stdint.h>

#define FILTER_TAP_NUM 31
#define HILBERT_TAP_NUM 31
#define HILBERT_DELAY  15  // Group delay of a 31-tap Hilbert filter ( (31-1)/2 )

/* FIR Filter instance structure */
typedef struct {
    const float *coeffs;    /* Pointer to filter coefficients */
    float *state;           /* Pointer to state/history buffer of size numTaps */
    uint16_t numTaps;       /* Number of coefficients/taps in the filter */
    uint16_t stateIndex;    /* Index of the oldest sample in the circular buffer */
} FIR_Instance;

/* Simple Delay Line structure for I-path matching delay */
typedef struct {
    float *state;           /* Pointer to delay buffer of size delaySamples */
    uint16_t delaySamples;  /* Number of samples to delay */
    uint16_t writeIndex;    /* Write pointer in circular buffer */
} DelayLine_Instance;

/* Function prototypes */

/**
  * @brief Initializes an FIR filter instance.
  * @param fir: Pointer to the FIR instance structure.
  * @param coeffs: Pointer to the coefficient array.
  * @param state: Pointer to the state/history buffer.
  * @param numTaps: Number of filter taps.
  */
void FIR_Init(FIR_Instance *fir, const float *coeffs, float *state, uint16_t numTaps);

/**
  * @brief Processes a single input sample through the FIR filter.
  * @param fir: Pointer to the FIR instance structure.
  * @param input: The new input sample.
  * @return The filtered output sample.
  */
float FIR_Process(FIR_Instance *fir, float input);

/**
  * @brief Initializes a delay line.
  * @param dl: Pointer to the delay line structure.
  * @param state: Pointer to the delay state buffer.
  * @param delaySamples: The number of samples of delay.
  */
void DelayLine_Init(DelayLine_Instance *dl, float *state, uint16_t delaySamples);

/**
  * @brief Pushes a new sample into the delay line and returns the oldest sample.
  * @param dl: Pointer to the delay line structure.
  * @param input: The new sample to push.
  * @return The delayed output sample.
  */
float DelayLine_Process(DelayLine_Instance *dl, float input);

/* Pre-designed filter coefficients */
extern const float BPF_COEFFS[FILTER_TAP_NUM];
extern const float HILBERT_COEFFS[HILBERT_TAP_NUM];
extern const float LPF_COEFFS[FILTER_TAP_NUM];

#endif /* CORE_INC_DSP_H_ */
