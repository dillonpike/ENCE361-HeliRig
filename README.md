# ENCE361 Helicopter Project

## Introduction
The ENCE361 Embedded Systems 1 course requires the design of a scale helicopter controller using the Texas Instruments TM4C123GXL Launchpad development board, and the Digilent Orbit BoosterPack 
## Authors
- Bailey Lissington
- Dillon Pike
- Joseph Ramirez

## Running Modes
Uncomment the respective lines in main.c to alter the running mode of the helicopter controller
### Testing Mode
```
//#define TESTING
```
Uses the built-in potentiometer for voltage measurement rather than PE4. Useful for testing without an adjustable power supply.
### Debug Mode
```
//#define DEBUG
```
Outputs debugging information to the virtual serial port of the TivaBoard.
## Changelog

| Version | Due Date | Description
| ----------- | ----------- | ----------- |
| Milestone 1 | 26/03/2021 | Accepts an analogue input on AIN9, PE4 (J1-05) up to a maximum of 3.3 volts. The analogue input is regularly sampled and the samples are stored in a circular buffer that has its mean calculated regularly. The mean at the initiation of the program corresponds to an altitude of 0% and increases to 100% when the input reduces by 1 volt. The mean corresponding to 0% altitude can be changed to the current mean with the left button.  The altitude can be displayed as a percentage, mean ADC value, or not displayed at all. This is toggled with the up button. |
