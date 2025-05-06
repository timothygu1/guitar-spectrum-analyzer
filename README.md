# ESP32 Guitar Spectrum Analyzer

## Overview
This project implements a real-time spectrum analyzer for a guitar input signal, visualized on a 16-band LED matrix. The system uses an **ESP32 microcontroller**, an **op-amp gain stage**, and a **Fast Fourier Transform (FFT)** to process and display the frequency content of the guitar signal. It captures audio at **44.1 kHz** using **I2S** with **DMA buffering** for low CPU overhead and non-blocking ADC sampling. The result is a responsive and intuitive visualization of the guitar signal's frequency spectrum.

<div style="display: flex;">
<img src="https://github.com/user-attachments/assets/abff05b4-6354-42e2-9571-048bfe3518cf" width="300"/>
<img src="https://github.com/user-attachments/assets/1fab0fdd-0ec2-48a3-b46b-022798ae2d8b" width="600"/>
</div>

## Hardware
- **ESP32-WROVER Module**
- **16x16 WS2812B LED Matrix**
- **TL071 Operational Amplifier**
- **Acoustic Guitar with Fishman Neo-D Magnetic Pickup**

## Software/Library Dependencies
- **PlatformIO (VSCode Extension)**
- **FastLED**
- **ArduinoFFT**

