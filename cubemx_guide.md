# STM32CubeMX Setup Guide (Acoustic SSB Walkie-Talkie)

This guide provides step-by-step instructions to configure the peripherals on your **STM32F407VET6** board using the STM32CubeMX graphical utility for acoustic transmission. Once configured, you will generate the project structure for **STM32CubeIDE**.

---

## 1. Pin Assignment Summary

| Peripheral Pin | Label | Alternate Function | Purpose |
| :--- | :--- | :--- | :--- |
| **PE4** | `PTT` | GPIO_Input | Reads state of the onboard KEY0 button (Press to toggle TX/RX) |
| **PA1** | | ADC1_IN1 | Analog input from MAX9814 Microphone |
| **PA5** | | DAC_OUT2 | Analog output to Active Speaker (via 10uF cap & 3.5mm jack) |
| **PA6** | | GPIO_Output | Onboard LED D2 (TX active indicator) |
| **PA7** | | GPIO_Output | Onboard LED D3 (RX active indicator) |

---

## 2. Step-by-Step Peripheral Configuration

### Step A: Clock Configuration (HCLK = 168 MHz)
1. Go to **System Core** -> **RCC**.
2. Set **High Speed Clock (HSE)** to **Crystal/Ceramic Resonator** (usually an 8 MHz crystal).
3. Go to the **Clock Configuration** tab.
4. Set the **Input Frequency** to `8` MHz (or matching your board's crystal).
5. Choose **PLLSourceMUX** as **HSE**.
6. Set **System Clock Mux** to **PLLCLK**.
7. In the **HCLK (MHz)** box, type `168` and press Enter. CubeMX will auto-calculate dividers.

### Step B: GPIO Configuration (PE4)
1. Go to the **Pinout & Configuration** tab.
2. In the chip search bar (bottom right), type `PE4`, left-click it, and select **GPIO_Input**.
3. In the left panel, navigate to **System Core** -> **GPIO**.
4. Select **PE4** from the table:
   * **GPIO mode**: `Input mode`
   * **GPIO Pull-up/Pull-down**: `Pull-up`
   * **User Label**: `PTT`

*(Note: Pins PA6 and PA7 for the LEDs are initialized directly in C code, so you do not need to configure them in the CubeMX interface).*

### Step C: Timer 2 (TIM2) Configuration (40 kHz Trigger)
TIM2 acts as the master sampling clock, triggering both ADC conversion and DAC output simultaneously.
1. Navigate to **Timers** -> **TIM2**.
2. Set **Clock Source** to **Internal Clock**.
3. Under **Configuration** -> **Parameter Settings**:
   * **Prescaler (PSC)**: `167` *(APB1 Timer Clock is 84 MHz. 84 MHz / 168 = 500 kHz)*
   * **Counter Mode**: `Up`
   * **Counter Period (ARR)**: `11` *(500 kHz / 12 = 40.0 kHz)*
   * **Trigger Output (TRGO) Parameters** -> **Trigger Event Selection**: Choose **Update Event**

### Step D: ADC1 Configuration (Single-Channel Sampling)
1. Navigate to **Analog** -> **ADC1**.
2. Check the box for **IN1** (Mic on PA1).
3. Under **Configuration** -> **Parameter Settings**:
   * **Resolution**: `12-bit (15 ADCCLK cycles)`
   * **Scan Conversion Mode**: `Disabled`
   * **Continuous Conversion Mode**: `Disabled` (we trigger it externally)
   * **DMA Continuous Requests**: `Enabled`
   * **External Trigger Conversion Source**: `Timer 2 Trigger Out event`
   * **External Trigger Conversion Edge**: `Trigger detection on the rising edge`
   * **Rank 1**:
     * **Channel**: `Channel 1` (PA1)
     * **Sampling Time**: `15 Cycles` (keeps impedance matching stable)
4. Under **Configuration** -> **DMA Settings**:
   * Click **Add**.
   * Select **ADC1**.
   * Set **Direction**: `Peripheral to Memory`
   * Set **Mode**: `Circular`
   * Set **Increment Address**: Memory = **Checked**, Peripheral = **Unchecked**
   * Set **Data Width**: Peripheral = **Half Word**, Memory = **Half Word**.

### Step E: DAC Configuration (Single Output to Speaker)
1. Navigate to **Analog** -> **DAC**.
2. Check the box for **OUT2 Configuration** (PA5).
3. Under **Configuration** -> **Parameter Settings**:
   * **Trigger for OUT2**: `Timer 2 Trigger Out event`
   * **Output Buffer for OUT2**: `Enable`
4. Under **Configuration** -> **DMA Settings**:
   * Click **Add**.
   * Select **DAC_OUT2**.
   * Set **Mode**: `Circular`
   * Set **Increment Address**: Memory = **Checked**, Peripheral = **Unchecked**
   * Set **Data Width**: Peripheral = **Half Word**, Memory = **Half Word**.

### Step F: NVIC (Interrupt) Check
1. Go to **System Core** -> **NVIC**.
2. Select the **NVIC** tab.
3. Ensure that **DMA2 stream0 global interrupt** (or the DMA stream assigned to ADC1) is **Checked** / Enabled.

---

## 3. Project Generation Settings
1. Go to the **Project Manager** tab.
2. **Project Name**: `WalkieTalkie`
3. **Toolchain / IDE**: Select **STM32CubeIDE**.
4. Under **Code Generator**:
   * Select **Copy only the necessary library files**.
   * Select **Generate peripheral initialization as a pair of '.c/.h' files per peripheral**.
5. Click **GENERATE CODE** in the top right corner.
