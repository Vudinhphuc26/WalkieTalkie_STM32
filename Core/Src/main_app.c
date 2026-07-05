#include "main.h"
#include "dsp.h"

/* Extern peripheral handles from STM32CubeMX generated code */
extern ADC_HandleTypeDef hadc1;
extern DAC_HandleTypeDef hdac;
extern TIM_HandleTypeDef htim2;

/* DSP Constants */
#define BLOCK_SIZE   32
#define ADC_BUF_SIZE (BLOCK_SIZE * 2 * 2) // 32 samples * 2 (double buffer) * 2 channels (Ch1, Ch4)
#define DAC_BUF_SIZE (BLOCK_SIZE * 2)     // 32 samples * 2 (double buffer)

/* States */
typedef enum {
    STATE_RX = 0,
    STATE_TX
} SystemState;

volatile SystemState current_state = STATE_RX;

/* Double buffers for ADC and DACs */
uint16_t adc_buffer[ADC_BUF_SIZE];
uint16_t dac1_buffer[DAC_BUF_SIZE]; // TX (Jack Output PA4)
uint16_t dac2_buffer[DAC_BUF_SIZE]; // RX (Speaker Output PA5)

/* DSP filter structures and states */
FIR_Instance bpf;
FIR_Instance hilbert;
FIR_Instance lpf;
DelayLine_Instance delay_line;

float bpf_state[FILTER_TAP_NUM];
float hilbert_state[HILBERT_TAP_NUM];
float lpf_state[FILTER_TAP_NUM];
float delay_line_state[HILBERT_DELAY];

/* Initialize DSP structures */
void DSP_Init(void) {
    FIR_Init(&bpf, BPF_COEFFS, bpf_state, FILTER_TAP_NUM);
    FIR_Init(&hilbert, HILBERT_COEFFS, hilbert_state, HILBERT_TAP_NUM);
    FIR_Init(&lpf, LPF_COEFFS, lpf_state, FILTER_TAP_NUM);
    DelayLine_Init(&delay_line, delay_line_state, HILBERT_DELAY);
}

/* Audio DSP processing function */
void Process_Audio(uint16_t offset) {
    static uint32_t n = 0; // Carrier phase counter
    
    if (current_state == STATE_TX) {
        /* ----------------------------------------------------
         * TX MODE: Read Mic (Ch1), Modulate, Output to DAC1 (PA4)
         * ---------------------------------------------------- */
        for (uint16_t i = 0; i < BLOCK_SIZE; i++) {
            // Read 12-bit ADC sample from MAX9814 (Ch1 is even index: 0, 2, 4...)
            uint16_t mic_raw = adc_buffer[(offset + i) * 2];
            float mic_in = ((float)mic_raw - 2048.0f) / 2048.0f; // Scale to [-1.0, 1.0]
            
            // Step 1: Bandpass filter the voice (300 Hz - 3000 Hz)
            float x_filt = FIR_Process(&bpf, mic_in);
            
            // Step 2: Generate Quadrature Q component via Hilbert transform
            float q = FIR_Process(&hilbert, x_filt);
            
            // Step 3: Delay In-phase component I to match Hilbert group delay (15 samples)
            float i_delayed = DelayLine_Process(&delay_line, x_filt);
            
            // Step 4: SSB Modulation (Upper Sideband - USB)
            // s[n] = I[n]*cos(w_c * n) - Q[n]*sin(w_c * n)
            // Fs = 40kHz, Fc = 10kHz => w_c = pi/2
            // cos(pi/2 * n) = [1,  0, -1,  0]
            // sin(pi/2 * n) = [0,  1,  0, -1]
            float s = 0.0f;
            uint8_t carrier_idx = n % 4;
            
            if (carrier_idx == 0) {
                s = i_delayed;  // cos = 1, sin = 0
            } else if (carrier_idx == 1) {
                s = -q;         // cos = 0, sin = 1
            } else if (carrier_idx == 2) {
                s = -i_delayed; // cos = -1, sin = 0
            } else if (carrier_idx == 3) {
                s = q;          // cos = 0, sin = -1
            }
            n++;
            
            // Scale SSB signal to 12-bit DAC range with 1.65V bias (2048)
            // 0.8f gain factor to prevent overflow clipping
            float tx_val = s * 0.8f * 2048.0f + 2048.0f;
            
            // Hard limiter
            if (tx_val > 4095.0f) tx_val = 4095.0f;
            if (tx_val < 0.0f) tx_val = 0.0f;
            
            // Write to DAC1 (TX) buffer
            dac1_buffer[offset + i] = (uint16_t)tx_val;
            
            // Output silence to DAC2 (Speaker) during TX
            dac2_buffer[offset + i] = 2048;
        }
    } 
    else {
        /* ----------------------------------------------------
         * RX MODE: Read Jack (Ch4), Demodulate, Output to DAC2 (PA5)
         * ---------------------------------------------------- */
        for (uint16_t i = 0; i < BLOCK_SIZE; i++) {
            // Read 12-bit ADC sample from Jack RX (Ch4 is odd index: 1, 3, 5...)
            uint16_t rx_raw = adc_buffer[(offset + i) * 2 + 1];
            float rx_in = ((float)rx_raw - 2048.0f) / 2048.0f; // Scale to [-1.0, 1.0]
            
            // Step 1: Product Demodulation (Multiply by carrier cos(pi/2 * n))
            float demod = 0.0f;
            uint8_t carrier_idx = n % 4;
            
            if (carrier_idx == 0) {
                demod = rx_in;       // cos = 1
            } else if (carrier_idx == 1) {
                demod = 0.0f;        // cos = 0
            } else if (carrier_idx == 2) {
                demod = -rx_in;      // cos = -1
            } else if (carrier_idx == 3) {
                demod = 0.0f;        // cos = 0
            }
            n++;
            
            // Step 2: Low-pass filter (3000 Hz cutoff) to remove mixing image at 20 kHz
            float audio_out = FIR_Process(&lpf, demod);
            
            // Scale and add gain (e.g. 1.6f to boost soft signals) before outputting
            float rx_val = audio_out * 1.6f * 2048.0f + 2048.0f;
            
            // Hard limiter
            if (rx_val > 4095.0f) rx_val = 4095.0f;
            if (rx_val < 0.0f) rx_val = 0.0f;
            
            // Write to DAC2 (Speaker) buffer
            dac2_buffer[offset + i] = (uint16_t)rx_val;
            
            // DAC1 DMA is stopped during RX mode, so dac1_buffer content is ignored.
        }
    }
}

/* Half-transfer DMA Callback */
void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef* hadc) {
    if (hadc->Instance == ADC1) {
        Process_Audio(0);
    }
}

/* Full-transfer DMA Callback */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc) {
    if (hadc->Instance == ADC1) {
        Process_Audio(BLOCK_SIZE);
    }
}

/* User Application Initialization (Call in main.c after peripherals init) */
void App_Init(void) {
    // 1. Initialize DSP filters
    DSP_Init();
    
    // 2. Initialize DAC buffers with DC offset (2048) to avoid transients
    for (uint16_t i = 0; i < DAC_BUF_SIZE; i++) {
        dac1_buffer[i] = 2048;
        dac2_buffer[i] = 2048;
    }
    
    // 3. Start ADC sampling via DMA (both channels scanned)
    HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc_buffer, ADC_BUF_SIZE);
    
    // 4. Start Speaker Output (DAC2) via DMA (Default mode is RX)
    HAL_DAC_Start_DMA(&hdac, DAC_CHANNEL_2, (uint32_t*)dac2_buffer, DAC_BUF_SIZE, DAC_ALIGN_12B_R);
    current_state = STATE_RX;
    
    // 5. Start Timer 2 to drive conversion triggers
    HAL_TIM_Base_Start(&htim2);
}

/* User Application Loop (Call in main.c inside the while(1) loop) */
void App_Loop(void) {
    /* Read PTT button (KEY0 on PE4, active-low) */
    GPIO_PinState ptt_state = HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_4);
    
    if (ptt_state == GPIO_PIN_RESET) {
        // Button Pressed -> Want to Transmit (TX)
        if (current_state == STATE_RX) {
            // Disable Speaker DAC2 Output DMA
            HAL_DAC_Stop_DMA(&hdac, DAC_CHANNEL_2);
            
            // Start Transmit DAC1 Output DMA (PA4 now driven by DAC)
            HAL_DAC_Start_DMA(&hdac, DAC_CHANNEL_1, (uint32_t*)dac1_buffer, DAC_BUF_SIZE, DAC_ALIGN_12B_R);
            
            current_state = STATE_TX;
        }
    } 
    else {
        // Button Released -> Want to Receive (RX)
        if (current_state == STATE_TX) {
            // Disable Transmit DAC1 Output DMA (PA4 floats so we can read it via ADC)
            HAL_DAC_Stop_DMA(&hdac, DAC_CHANNEL_1);
            
            // Start Speaker DAC2 Output DMA (PA5 driven with demodulated audio)
            HAL_DAC_Start_DMA(&hdac, DAC_CHANNEL_2, (uint32_t*)dac2_buffer, DAC_BUF_SIZE, DAC_ALIGN_12B_R);
            
            current_state = STATE_RX;
        }
    }
    
    // Insert a tiny delay to debounce PTT button
    HAL_Delay(10);
}
