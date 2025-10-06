# A macro stacking rail

## Overview
This Project is structured as follows
- Hardware
  - of the shelf Mechanical Parts
    - Rail: [HiWin KK5002P](https://www.hiwin.de/de/Produkte/Pr%C3%A4zisionsachsen-%26-Pr%C3%A4zisions-Systeme/Pr%C3%A4zisionsachsen-KK-KF/KK/KK5002P150A1F0/p/10.00011)
    - Stepper Motor: [iCL Series NEMA 17 Integrated Closed Loop Stepper Motor](https://www.omc-stepperonline.com/icl-series-nema-17-integrated-closed-loop-stepper-motor-0-6nm-84-96oz-in-20-36vdc-w-14-bit-encoder-icl42-06)
    - Arca Swiss Clamp round, diameter 60mm: [link to mjkzz](https://www.mjkzz.de/collections/camera-plates/products/mjkzz-round-quick-release-system?variant=29216681427059)
    - Camera Plate 200mm: [LEOFOTO Quick Release Plate PL-200](https://www.amazon.de/dp/B081DBJ4B8)
  - Some adapters and parts are 3D printent. See [./3d-print.scad](./3d-print.scad).
    - ![rail-1.png](./3d-print.scad/rail-1.png)
    - ![rail-2.png](./3d-print.scad/rail-2.png)
- Firmware
- Electronics
  - Control of the Stepper Motor
  - Control of the Camera

## Details
### Firmware
The firmware is written with zephyr. The code is in [./app](./app).

It internally has a state machine:
![State Machine](./app/mermaid.StateMachine.svg)

It has a GUI with LVGL.

It runs on an embeded MCU which is not yet decided, potential options include:
- [STM32H747I-DISCO](https://www.st.com/en/evaluation-tools/stm32h747i-disco.html) with [-b stm32h747i_disco](https://docs.zephyrproject.org/latest/boards/st/stm32h747i_disco/doc/index.html)
- [STM32H7B3I-DK](https://www.st.com/en/evaluation-tools/stm32h7b3i-dk.html) with [-b stm32h7b3i_dk](https://docs.zephyrproject.org/latest/boards/st/stm32h7b3i_dk/doc/index.html)
- [B-U585I-IOT02A](https://www.st.com/en/evaluation-tools/b-u585i-iot02a.html) with Adafruit 2.4" TFT
- ~~[WIO Terminal](https://www.seeedstudio.com/Wio-Terminal-p-4509.html)~~ only the first three gpio-keys work and [PWM is not clear](https://github.com/zephyrproject-rtos/zephyr/issues/66547)

### Electronics / Control of the Stepper Motor
Optocopplers to drive the stepper motor

### Electronics / Control of the Camera
The Camera ~~is~~was controlled via IR. The IR LED is driven by a transistor.
