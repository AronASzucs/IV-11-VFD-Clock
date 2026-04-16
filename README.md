
<p align="center">
  <h1>IV-11 VFD Clock</h1>
  <img src="Images/Front.jpg" width="100%">
</p>

A 6-digit clock built around Soviet-era IV-11 vacuum fluorescent display tubes, driven by 6x 74HC595 shift registers and 6x UDN2981A high-voltage source drivers in a static drive configuration. The power supply uses an XL6009 boost converter (5V→25V) for the anode rail and an LM2596 buck converter (5V→1.5V) for the filament rail. A DS3231 RTC module handles timekeeping with battery backup, and an Arduino Nano Every runs the firmware — including SPI display control, PWM brightness adjustment via the shift register OE pin, and button-based time setting with EEPROM persistence.

Currently in progress. The circuit has been fully prototyped and verified on a breadboard. PCB schematic is underway in KiCad, with fabrication at JLCPCB planned once the design is finalized.

*Inspired by [OpenVFD](https://www.instructables.com/OpenVFD-6-Digit-IV-11-VFD-Tube-Clock/) for the static drive architecture using 74HC595 shift registers and UDN2981A source drivers.*
