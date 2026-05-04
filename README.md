<h1 align="center">IV-11 VFD Tube Clock</h1>

<p align="center">
  <img src="Images/Final_Center.jpg" width="100%">
</p>

<p align="center">
  <img src="Images/Final_Zoom.jpg" width="49%">
  <img src="Images/Final_Above.jpg" width="49%">
</p>

---

## Overview

A 6-digit clock built around Soviet-era IV-11 vacuum fluorescent display tubes, vintage components manufactured in Ukraine during the 1970s-80s. The entire project was designed from scratch, including schematic design, custom PCB layout, and firmware, with no kit or reference design used beyond architectural inspiration.

---

## Hardware

**Display**
6x IV-11 7-segment VFD tubes driven in a static configuration. One dedicated 74HC595N shift register and UDN2981A high-voltage source driver per tube, all daisy-chained over SPI. Static drive means every tube is on simultaneously at full brightness with no multiplexing.

**Power Supply**
The clock runs entirely from a 5V USB source with two onboard switching converters:
- XL6009 boost converter stepping 5V → 25V for the anode and grid rails
- LM2596 buck converter stepping 5V → 1.5V for the tube filament rails

**Timekeeping**
DS3231 real-time clock module over I2C with CR2032 battery backup. Accurate to ±2ppm and loses less than one minute per year.

**Microcontroller**
Arduino Nano Every handling SPI display control, I2C RTC communication, PWM brightness via shift register OE pin, button input with hardware debounce, and EEPROM-persistent user settings.

---

## Firmware

The clock firmware implements a 7-state FSM for all user interaction:

| Mode | Function |
|---|---|
| 0 | Normal clock display |
| 1 | Set hours |
| 2 | Set minutes |
| 3 | Set seconds |
| 4 | Manual brightness (1–9) |
| 5 | Auto brightness on/off |
| 6 | 12/24hr toggle |

Features include PWM brightness control via the 74HC595 OE pin, automatic brightness scheduling by hour of day, hardware debounce capacitors on all button inputs, and full EEPROM persistence for brightness, auto-brightness, and 12/24hr preference across power cycles.

---

## PCB

2-layer board designed in KiCad, fabricated by JLCPCB. Custom symbols and footprints created for the IV-11 tubes, XL6009 boost converter, and LM2596 buck converter modules. Ground plane on the bottom copper layer. Power traces sized for current capacity: 1.0mm for the 600mA filament rail and 0.5mm for the 25V anode rail.

<p align="center">
  <img src="Images/LAYOUT_TOP.jpg" width="49%">
  <img src="Images/LAYOUT_BOTTOM.jpg" width="49%">
</p>

---

## Build Notes

- The IV-11 tubes arrived from Ukraine after ~5 weeks in transit
- A tube tester utility was written to map each shift register bit to its physical segment before finalizing the digit lookup table
- The tube footprint I made was initially mirrored due to a bottom-view vs top-view convention mismatch in an online reference. I caught this before ordering and corrected it.

---

## Cost

| Component | Source | Cost |
|---|---|---|
| IV-11 tubes × 6 | eBay | $40.09 |
| PCB × 5 | JLCPCB | $22.84 |
| Buttons × 6 | AliExpress | $6.93 |
| Screws and hex nuts | Hardware Store | $6.25 |
| UDN2981A × 10 | AliExpress | $3.39 |
| XL6009 modules × 3 | AliExpress | $2.99 |
| DS3231 RTC module | AliExpress | $2.19 |
| LM2596 modules × 2 | AliExpress | $1.59 |
| 74HC595N × 10 | AliExpress | $1.19 |
| Arduino Nano Every | University | Free |
| Capacitors, resistors, headers | University | Free |
| **Total** | | **$87.46** |

## Acknowledgements

*Inspired by [OpenVFD](https://www.instructables.com/OpenVFD-6-Digit-IV-11-VFD-Tube-Clock/) for the static drive architecture using 74HC595 shift registers and UDN2981A source drivers.*
