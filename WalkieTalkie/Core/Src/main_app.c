#include "main.h"
#include "dsp.h"

/* Set to 1 to test Real-time Mic-to-Speaker loopback, 0 for Walkie-Talkie mode */
#define HARDWARE_TEST_MODE 0

/* Extern peripheral handles from STM32CubeMX generated code */
extern ADC_HandleTypeDef hadc1;
extern DAC_HandleTypeDef hdac;
extern TIM_HandleTypeDef htim2;

/* DSP Constants */
#define BLOCK_SIZE   32
#define ADC_BUF_SIZE (BLOCK_SIZE * 2)     // 32 samples * 2 (double buffer) - Single Channel (PA1)
#define DAC_BUF_SIZE (BLOCK_SIZE * 2)     // 32 samples * 2 (double buffer) - Single Channel (PA5)

/* States */
typedef enum {
    STATE_RX = 0,
    STATE_TX
} SystemState;

volatile SystemState current_state = STATE_RX;

/* Volume Control Variables (Fixed constant volume) */
#if HARDWARE_TEST_MODE
volatile float volume_scale = 0.02f; // Keep low in test mode to prevent feedback
#else
volatile float volume_scale = 1.00f; // 100% constant volume for normal walkie-talkie mode
#endif

/* Double buffers for ADC (Mic) and DAC (Speaker) */
uint16_t adc_buffer[ADC_BUF_SIZE];
uint16_t dac2_buffer[DAC_BUF_SIZE]; // RX/TX Speaker Output (PA5)

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
#if HARDWARE_TEST_MODE
    /* Direct loopback: copy samples from PA1 Mic to Speaker consecutively */
    for (uint16_t i = 0; i < BLOCK_SIZE; i++) {
        uint16_t mic_sample = adc_buffer[offset + i];
        
        // Convert to float, apply volume scaling
        float raw_val = (float)mic_sample - 2048.0f;
        float val = raw_val * volume_scale + 2048.0f;
        
        // Hard limiter
        if (val > 4095.0f) val = 4095.0f;
        if (val < 0.0f) val = 0.0f;
        
        dac2_buffer[offset + i] = (uint16_t)val;
    }
    return;
#endif

    static uint32_t n = 0; // Carrier phase counter
    
    if (current_state == STATE_TX) {
        /* ----------------------------------------------------
         * TX MODE: Read Mic (PA1), AM Modulate, Play to Speaker (PA5)
         * ---------------------------------------------------- */
        for (uint16_t i = 0; i < BLOCK_SIZE; i++) {
            // Read 12-bit ADC sample from MAX9814 (Ch1 on PA1)
            uint16_t mic_raw = adc_buffer[offset + i];
            float mic_in = ((float)mic_raw - 2048.0f) / 2048.0f; // Scale to [-1.0, 1.0]
            
            // Step 1: Bandpass filter the voice (300 Hz - 3000 Hz)
            float x_filt = FIR_Process(&bpf, mic_in);
            
            // Step 2: AM Modulation with 80% modulation index (mu = 0.8)
            // s[n] = [1.0 + 0.8 * x_filt[n]] * cos(w_c * n)
            // Scaled down by 2 to prevent DAC clipping/overflow:
            // s_scaled[n] = [0.5 + 0.4 * x_filt[n]] * cos(w_c * n)
            // Since Fs = 4 * Fc, w_c = pi/2
            // cos(pi/2 * n) = [1,  0, -1,  0]
            float s = 0.0f;
            uint8_t carrier_idx = n % 4;
            float envelope = 0.5f + 0.4f * x_filt;
            
            if (carrier_idx == 0) {
                s = envelope;   // cos = 1
            } else if (carrier_idx == 1) {
                s = 0.0f;        // cos = 0
            } else if (carrier_idx == 2) {
                s = -envelope;  // cos = -1
            } else if (carrier_idx == 3) {
                s = 0.0f;        // cos = 0
            }
            n++;
            
            // Scale AM signal to 12-bit DAC range with 1.65V bias (2048)
            float tx_val = s * 2048.0f + 2048.0f;
            
            // Hard limiter
            if (tx_val > 4095.0f) tx_val = 4095.0f;
            if (tx_val < 0.0f) tx_val = 0.0f;
            
            // Output modulated AM signal to speaker
            dac2_buffer[offset + i] = (uint16_t)tx_val;
        }
    } 
    else {
        /* ----------------------------------------------------
         * RX MODE: Read AM from Mic (PA1), Demodulate, Play to Speaker (PA5)
         * ---------------------------------------------------- */
        for (uint16_t i = 0; i < BLOCK_SIZE; i++) {
            // Read 12-bit ADC sample from Mic (PA1) containing incoming AM carrier
            uint16_t rx_raw = adc_buffer[offset + i];
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
            
            // Step 2: Low-pass filter (3000 Hz cutoff) to remove mixing image at 2 * fc
            float lpf_out = FIR_Process(&lpf, demod);
            
            // Step 3: Band-pass filter (300-3000 Hz) to remove DC offset and out-of-band noise
            float audio_clean = FIR_Process(&bpf, lpf_out);
            
            // Step 4: Scale and add gain, scaled by digital volume_scale
            // Demodulated baseband peak is 0.2, scaling by 4.0f matches the previous 0.8f gain.
            float rx_val = audio_clean * 4.0f * volume_scale * 2048.0f + 2048.0f;
            
            // Hard limiter
            if (rx_val > 4095.0f) rx_val = 4095.0f;
            if (rx_val < 0.0f) rx_val = 0.0f;
            
            // Write demodulated audio to speaker
            dac2_buffer[offset + i] = (uint16_t)rx_val;
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
        
#if HARDWARE_TEST_MODE
        static uint32_t blink_counter = 0;
        // Diagnostic: blink TX LED (PA6) once per second (1250 callbacks per second / 2)
        blink_counter++;
        if (blink_counter >= 625) {
            HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_6);
            blink_counter = 0;
        }
#endif
    }
}

/* User Application Initialization (Call in main.c after peripherals init) */
void App_Init(void) {
    // 1. Initialize GPIOs for LEDs and PTT/Volume buttons
    // PA6 (TX LED), PA7 (RX LED) as outputs (for onboard LEDs D2 & D3)
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    // Initialize PE4 (KEY0) as input with Pull-Up
    __HAL_RCC_GPIOE_CLK_ENABLE();
    GPIO_InitStruct.Pin = GPIO_PIN_4;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
    
    // Initialize PA0 (WKUP Button) as input with Pull-Down
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // Initialize PB0 (External PTT Button) as input with Pull-Up
    __HAL_RCC_GPIOB_CLK_ENABLE();
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    // Default LED states
#if HARDWARE_TEST_MODE
    // In test mode, turn both LEDs OFF initially
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6 | GPIO_PIN_7, GPIO_PIN_SET);
#else
    // Default to RX: Turn OFF TX LED (PA6) and Turn ON RX LED (PA7) - Active Low (0=ON, 1=OFF)
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_RESET);
#endif

    // 2. Set static TIM2 configuration registers based on configuration selection
    htim2.Init.Prescaler = TIM2_PRESCALER;
    htim2.Init.Period = TIM2_PERIOD;
    __HAL_TIM_SET_PRESCALER(&htim2, TIM2_PRESCALER);
    __HAL_TIM_SET_AUTORELOAD(&htim2, TIM2_PERIOD);
    
    // Force register shadow update
    htim2.Instance->EGR = TIM_EGR_UG;

    // 3. Initialize DSP instances
    DSP_Init();
    
    // 4. Initialize DAC buffer with DC offset (2048) to avoid transients
    for (uint16_t i = 0; i < DAC_BUF_SIZE; i++) {
        dac2_buffer[i] = 2048;
    }
    
    // 5. Start ADC sampling via DMA (Single channel PA1)
    HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc_buffer, ADC_BUF_SIZE);
    
    // 6. Start Speaker Output (DAC2 on PA5) via DMA
    HAL_DAC_Start_DMA(&hdac, DAC_CHANNEL_2, (uint32_t*)dac2_buffer, DAC_BUF_SIZE, DAC_ALIGN_12B_R);
    current_state = STATE_RX;
    
    // 7. Start Timer 2 to drive conversion triggers
    HAL_TIM_Base_Start(&htim2);
}

/* User Application Loop (Call in main.c inside the while(1) loop) */
void App_Loop(void) {
    /* Read on-board PTT button (KEY0 on PE4, active-low) */
    GPIO_PinState ptt_pe4 = HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_4);
    
    /* Read on-board WKUP button (active-high on PA0) */
    GPIO_PinState ptt_pa0 = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0);
    
    /* Read External PTT button (PB0, active-low) */
    GPIO_PinState ext_ptt = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0);
    
#if HARDWARE_TEST_MODE
    /* Blink LED D2 (PA6) once per second to show the CPU is active and running */
    HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_6);
    HAL_Delay(500);

#else
    /* Determine if PTT button is pressed (KEY0 PE4 active-low, ext_ptt PB0 active-low, or WKUP PA0 active-high) */
    uint8_t ptt_pressed = (ptt_pe4 == GPIO_PIN_RESET) || 
                          (ext_ptt == GPIO_PIN_RESET) ||
                          (ptt_pa0 == GPIO_PIN_SET);
    
    if (ptt_pressed) {
        // PTT Button is currently HELD DOWN -> Transmit (TX) Mode
        if (current_state == STATE_RX) {
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_SET);
            current_state = STATE_TX;
        }
    } 
    else {
        // PTT Buttons are RELEASED -> Receive (RX) Mode
        if (current_state == STATE_TX) {
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_SET);
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_RESET);
            current_state = STATE_RX;
        }
    }
    
    HAL_Delay(10);
#endif
}
