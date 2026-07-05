#include "dsp.h"

#if CONFIG_FREQ_SEL == 1
/* BPF Coefficients (300-3000Hz, Fs=12kHz, 31 taps) */
const float BPF_COEFFS[FILTER_TAP_NUM] = {
      -0.0029f,   -0.0017f,    0.0003f,   -0.0042f,   -0.0134f,   -0.0099f,    0.0002f,   -0.0187f,
      -0.0507f,   -0.0294f,    0.0144f,   -0.0398f,   -0.1412f,   -0.0474f,    0.2667f,    0.4514f,
       0.2667f,   -0.0474f,   -0.1412f,   -0.0398f,    0.0144f,   -0.0294f,   -0.0507f,   -0.0187f,
       0.0002f,   -0.0099f,   -0.0134f,   -0.0042f,    0.0003f,   -0.0017f,   -0.0029f
};

/* Hilbert Transform Coefficients (Fs=12kHz, 31 taps) */
const float HILBERT_COEFFS[HILBERT_TAP_NUM] = {
      -0.0034f,    0.0000f,   -0.0059f,    0.0000f,   -0.0134f,    0.0000f,   -0.0281f,    0.0000f,
      -0.0535f,    0.0000f,   -0.0980f,    0.0000f,   -0.1936f,    0.0000f,   -0.6302f,    0.0000f,
       0.6302f,    0.0000f,    0.1936f,    0.0000f,    0.0980f,    0.0000f,    0.0535f,    0.0000f,
       0.0281f,    0.0000f,    0.0134f,    0.0000f,    0.0059f,    0.0000f,    0.0034f
};

/* LPF Coefficients (3000Hz cutoff, Fs=12kHz, 31 taps) */
const float LPF_COEFFS[FILTER_TAP_NUM] = {
      -0.0017f,    0.0000f,    0.0029f,   -0.0000f,   -0.0067f,    0.0000f,    0.0141f,   -0.0000f,
      -0.0268f,    0.0000f,    0.0491f,   -0.0000f,   -0.0969f,    0.0000f,    0.3156f,    0.5008f,
       0.3156f,    0.0000f,   -0.0969f,   -0.0000f,    0.0491f,    0.0000f,   -0.0268f,   -0.0000f,
       0.0141f,    0.0000f,   -0.0067f,   -0.0000f,    0.0029f,    0.0000f,   -0.0017f
};

#elif CONFIG_FREQ_SEL == 2
/* BPF Coefficients (300-3000Hz, Fs=24kHz, 31 taps) */
const float BPF_COEFFS[FILTER_TAP_NUM] = {
      -0.0028f,   -0.0039f,   -0.0046f,   -0.0036f,   -0.0004f,    0.0029f,    0.0008f,   -0.0116f,
      -0.0331f,   -0.0530f,   -0.0538f,   -0.0210f,    0.0462f,    0.1298f,    0.1995f,    0.2266f,
       0.1995f,    0.1298f,    0.0462f,   -0.0210f,   -0.0538f,   -0.0530f,   -0.0331f,   -0.0116f,
       0.0008f,    0.0029f,   -0.0004f,   -0.0036f,   -0.0046f,   -0.0039f,   -0.0028f
};

/* Hilbert Transform Coefficients (Fs=24kHz, 31 taps) */
const float HILBERT_COEFFS[HILBERT_TAP_NUM] = {
      -0.0034f,    0.0000f,   -0.0059f,    0.0000f,   -0.0134f,    0.0000f,   -0.0281f,    0.0000f,
      -0.0535f,    0.0000f,   -0.0980f,    0.0000f,   -0.1936f,    0.0000f,   -0.6302f,    0.0000f,
       0.6302f,    0.0000f,    0.1936f,    0.0000f,    0.0980f,    0.0000f,    0.0535f,    0.0000f,
       0.0281f,    0.0000f,    0.0134f,    0.0000f,    0.0059f,    0.0000f,    0.0034f
};

/* LPF Coefficients (3000Hz cutoff, Fs=24kHz, 31 taps) */
const float LPF_COEFFS[FILTER_TAP_NUM] = {
      -0.0012f,   -0.0021f,   -0.0021f,    0.0000f,    0.0048f,    0.0099f,    0.0100f,   -0.0000f,
      -0.0190f,   -0.0363f,   -0.0348f,    0.0000f,    0.0686f,    0.1533f,    0.2235f,    0.2507f,
       0.2235f,    0.1533f,    0.0686f,    0.0000f,   -0.0348f,   -0.0363f,   -0.0190f,   -0.0000f,
       0.0100f,    0.0099f,    0.0048f,    0.0000f,   -0.0021f,   -0.0021f,   -0.0012f
};

#elif CONFIG_FREQ_SEL == 3
/* BPF Coefficients (300-3000Hz, Fs=67.2kHz, 31 taps) */
const float BPF_COEFFS[FILTER_TAP_NUM] = {
      -0.0031f,   -0.0032f,   -0.0035f,   -0.0035f,   -0.0024f,    0.0007f,    0.0065f,    0.0155f,
       0.0276f,    0.0423f,    0.0586f,    0.0753f,    0.0905f,    0.1028f,    0.1108f,    0.1136f,
       0.1108f,    0.1028f,    0.0905f,    0.0753f,    0.0586f,    0.0423f,    0.0276f,    0.0155f,
       0.0065f,    0.0007f,   -0.0024f,   -0.0035f,   -0.0035f,   -0.0032f,   -0.0031f
};

/* Hilbert Transform Coefficients (Fs=67.2kHz, 31 taps) */
const float HILBERT_COEFFS[HILBERT_TAP_NUM] = {
      -0.0034f,    0.0000f,   -0.0059f,    0.0000f,   -0.0134f,    0.0000f,   -0.0281f,    0.0000f,
      -0.0535f,    0.0000f,   -0.0980f,    0.0000f,   -0.1936f,    0.0000f,   -0.6302f,    0.0000f,
       0.6302f,    0.0000f,    0.1936f,    0.0000f,    0.0980f,    0.0000f,    0.0535f,    0.0000f,
       0.0281f,    0.0000f,    0.0134f,    0.0000f,    0.0059f,    0.0000f,    0.0034f
};

/* LPF Coefficients (3000Hz cutoff, Fs=67.2kHz, 31 taps) */
const float LPF_COEFFS[FILTER_TAP_NUM] = {
      -0.0016f,   -0.0015f,   -0.0015f,   -0.0010f,    0.0004f,    0.0034f,    0.0085f,    0.0160f,
       0.0259f,    0.0377f,    0.0507f,    0.0637f,    0.0757f,    0.0853f,    0.0915f,    0.0936f,
       0.0915f,    0.0853f,    0.0757f,    0.0637f,    0.0507f,    0.0377f,    0.0259f,    0.0160f,
       0.0085f,    0.0034f,    0.0004f,   -0.0010f,   -0.0015f,   -0.0015f,   -0.0016f
};
#endif

void FIR_Init(FIR_Instance *fir, const float *coeffs, float *state, uint16_t numTaps) {
    fir->coeffs = coeffs;
    fir->state = state;
    fir->numTaps = numTaps;
    fir->stateIndex = 0;
    
    // Clear state buffer
    for (uint16_t i = 0; i < numTaps; i++) {
        state[i] = 0.0f;
    }
}

float FIR_Process(FIR_Instance *fir, float input) {
    // Insert new sample into circular buffer
    fir->state[fir->stateIndex] = input;
    
    float accum = 0.0f;
    uint16_t stateIdx = fir->stateIndex;
    uint16_t numTaps = fir->numTaps;
    
    // Perform convolution (multiply-accumulate)
    for (uint16_t i = 0; i < numTaps; i++) {
        accum += fir->coeffs[i] * fir->state[stateIdx];
        if (stateIdx == 0) {
            stateIdx = numTaps - 1; // Wrap around to end
        } else {
            stateIdx--;
        }
    }
    
    // Advance state index
    fir->stateIndex++;
    if (fir->stateIndex >= numTaps) {
        fir->stateIndex = 0;
    }
    
    return accum;
}

void DelayLine_Init(DelayLine_Instance *dl, float *state, uint16_t delaySamples) {
    dl->state = state;
    dl->delaySamples = delaySamples;
    dl->writeIndex = 0;
    
    // Clear delay state buffer
    for (uint16_t i = 0; i < delaySamples; i++) {
        state[i] = 0.0f;
    }
}

float DelayLine_Process(DelayLine_Instance *dl, float input) {
    // Get the oldest sample (which is at the current write position)
    float output = dl->state[dl->writeIndex];
    
    // Overwrite it with the new sample
    dl->state[dl->writeIndex] = input;
    
    // Advance write pointer
    dl->writeIndex++;
    if (dl->writeIndex >= dl->delaySamples) {
        dl->writeIndex = 0;
    }
    
    return output;
}
