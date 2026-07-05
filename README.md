# Acoustic SSB Walkie-Talkie on STM32F4

A real-time, DSP-based acoustic Single Sideband (SSB - Upper Sideband) walkie-talkie transceiver implemented on an **STM32F407VET6** development board. 

The project samples voice from a microphone, performs real-time digital signal processing (DSP) to modulate the signal to an acoustic carrier frequency of **10 kHz**, and transmits it acoustically. On the receiving end, the device captures the acoustic signal, performs product demodulation and low-pass filtering, and outputs the audio to an active speaker. A MATLAB helper script is also included for real-time demodulation on a PC.

---
