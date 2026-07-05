#include "dsp.h"

/* BPF Coefficients (300 Hz - 3000 Hz bandpass, Fs = 40 kHz, 31 taps) */
const float BPF_COEFFS[FILTER_TAP_NUM] = {
    -0.0021f, -0.0031f, -0.0042f, -0.0041f, -0.0011f,  0.0062f,  0.0169f,  0.0289f,
     0.0389f,  0.0435f,  0.0400f,  0.0270f,  0.0048f, -0.0232f, -0.0505f, -0.0711f,
    -0.0805f, -0.0759f, -0.0573f, -0.0279f,  0.0055f,  0.0347f,  0.0543f,  0.0609f,
     0.0535f,  0.0349f,  0.0105f, -0.0125f, -0.0274f, -0.0326f, -0.0284f
};

/* Hilbert Transform Coefficients (90 degree phase shifter, Fs = 40 kHz, 31 taps) */
/* The even coefficients are exactly zero to reduce calculation overhead. */
const float HILBERT_COEFFS[HILBERT_TAP_NUM] = {
    -0.0424f,  0.0000f, -0.0495f,  0.0000f, -0.0594f,  0.0000f, -0.0742f,  0.0000f,
    -0.0990f,  0.0000f, -0.1485f,  0.0000f, -0.2971f,  0.0000f, -0.8913f,  0.0000f,
     0.8913f,  0.0000f,  0.2971f,  0.0000f,  0.1485f,  0.0000f,  0.0990f,  0.0000f,
     0.0742f,  0.0000f,  0.0594f,  0.0000f,  0.0495f,  0.0000f,  0.0424f
};

/* LPF Coefficients (3000 Hz cutoff lowpass, Fs = 40 kHz, 31 taps) */
const float LPF_COEFFS[FILTER_TAP_NUM] = {
     0.0022f,  0.0039f,  0.0069f,  0.0113f,  0.0171f,  0.0242f,  0.0321f,  0.0404f,
     0.0487f,  0.0563f,  0.0626f,  0.0673f,  0.0701f,  0.0710f,  0.0713f,  0.0713f,
     0.0713f,  0.0710f,  0.0701f,  0.0673f,  0.0626f,  0.0563f,  0.0487f,  0.0404f,
     0.0321f,  0.0242f,  0.0171f,  0.0113f,  0.0069f,  0.0039f,  0.0022f
};

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
