# Acoustic SSB Walkie-Talkie on STM32F4

A real-time, DSP-based acoustic Single Sideband (SSB - Upper Sideband) walkie-talkie transceiver implemented on an **STM32F407VET6** development board. 

The project samples voice from a microphone, performs real-time digital signal processing (DSP) to modulate the signal to an acoustic carrier frequency of **10 kHz**, and transmits it acoustically. On the receiving end, the device captures the acoustic signal, performs product demodulation and low-pass filtering, and outputs the audio to an active speaker. A MATLAB helper script is also included for real-time demodulation on a PC.

---

## 🛠 Features & DSP Pipeline

### Transmit (TX) Pipeline
1. **Sampling**: External Timer 2 triggers ADC1 Channel 1 (connected to a MAX9814 microphone) at a sample rate of **$F_s = 40\text{ kHz}$**.
2. **Pre-filtering**: Staged samples are scaled to $[-1.0, 1.0]$ and passed through a 31-tap FIR Bandpass Filter (BPF) with a passband of **$300\text{ Hz} - 3000\text{ Hz}$** to restrict the voice spectrum.
3. **Phase Shifting**:
   * The filtered signal is passed through a **31-tap FIR Hilbert Transform** filter to generate the Quadrature ($Q$) component, introducing a $-90^\circ$ phase shift.
   * The In-phase ($I$) component is delayed by **15 samples** using a circular delay line to match the group delay of the Hilbert filter:
     $$\text{Group Delay} = \frac{N - 1}{2} = \frac{31 - 1}{2} = 15 \text{ samples}$$
4. **SSB Modulation**: Upper Sideband (USB) modulation is achieved using the Hartley modulator method with a carrier frequency of **$F_c = 10\text{ kHz}$** ($w_c = \frac{\pi}{2}$):
   $$s[n] = I[n]\cos(w_c n) - Q[n]\sin(w_c n)$$
   Because $F_s = 40\text{ kHz}$ and $F_c = 10\text{ kHz}$, the carrier terms simplify to:
   * $\cos(\frac{\pi}{2} n) = [1, 0, -1, 0]$
   * $\sin(\frac{\pi}{2} n) = [0, 1, 0, -1]$
5. **Output**: The modulated signal is offset by a 1.65V bias (12-bit DAC value `2048`), limited to prevent clipping, and output to DAC Channel 1 (PA4).

### Receive (RX) Pipeline
1. **Demodulation**: The incoming modulated signal is read from ADC1 Channel 4 and demodulated by multiplying with the local carrier $\cos(w_c n)$.
2. **Post-filtering**: The mixed signal is passed through a 31-tap FIR Low-Pass Filter (LPF) with a **3.0 kHz cutoff** to suppress the mixing image centered at 20 kHz.
3. **Playback**: The resulting audio is scaled, biased, and output directly to the active speaker via DAC Channel 2 (PA5).

---

## 🔌 Hardware Connections & Pinout

| Peripheral Pin | Board Label | Purpose | Type |
| :--- | :--- | :--- | :--- |
| **PE4** | `KEY0` | Push-to-Talk (PTT) Button (Active-Low) | GPIO_Input (Pull-up) |
| **PA1** | | Analog Input from MAX9814 Microphone | ADC1_IN1 |
| **PA4** | | Analog Output for TX (SSB Output Jack) | DAC_OUT1 |
| **PA5** | | Analog Output to Active Speaker (via 10uF cap) | DAC_OUT2 |
| **PA6** | `D2` | Onboard LED: Indicates Active Transmission (TX) | GPIO_Output |
| **PA7** | `D3` | Onboard LED: Indicates Active Reception (RX) | GPIO_Output |

---

## ⚙️ STM32CubeMX Configuration Summary

Peripherals are configured as follows:
* **System Clock**: $168\text{ MHz}$ using an external crystal (HSE).
* **TIM2**: Internal Clock with Prescaler `167` and ARR `11` to generate a trigger event at **$40\text{ kHz}$** (Trigger Output: Update Event).
* **ADC1**:
  * Scans Channel 1 (Mic) and Channel 4 (Aux/RX).
  * Triggered by `TIM2 Trigger Out Event` on the rising edge.
  * DMA configured in **Circular Mode** with Half-Word width.
* **DAC**:
  * Outputs enabled for OUT1 (TX) and OUT2 (RX).
  * Triggered by `TIM2 Trigger Out Event`.
  * DMA configured in **Circular Mode** with Half-Word width.
* **NVIC**: Enable global DMA interrupts for ADC/DAC streams.

For a detailed step-by-step setup guide, refer to [cubemx_guide.md](file:///Users/phucvu/WalkieTalkie/cubemx_guide.md).

---

## 💻 MATLAB Debugging Tool (`demodulator.m`)

The repository includes [demodulator.m](file:///Users/phucvu/WalkieTalkie/demodulator.m), a MATLAB script that captures audio from your PC microphone in real-time, demodulates the AM/SSB signal at a user-selected carrier frequency, and outputs the audio. 

### Supported carrier configurations:
* **Config 1**: $F_s = 12\text{ kHz}, F_c = 3.0\text{ kHz}$ (Default)
* **Config 2**: $F_s = 24\text{ kHz}, F_c = 6.0\text{ kHz}$
* **Config 3**: $F_s = 67.2\text{ kHz}, F_c = 16.8\text{ kHz}$

---

## 🚀 How to Run the Project

1. Import the `WalkieTalkie` project into **STM32CubeIDE**.
2. Open `WalkieTalkie.ioc` to review or regenerate the configuration code (if needed).
3. Connect your **STM32F407VET6** board to your debugger (e.g., ST-LINK).
4. Build the project (Ctrl+B) and Flash it onto the target MCU.
5. Connect your microphone (MAX9814) to pin `PA1` and an active speaker to `PA5` through a coupling capacitor.
6. Press and hold the onboard **KEY0 (PE4)** button to transmit, or release it to receive.
