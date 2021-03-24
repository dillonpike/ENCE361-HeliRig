Authors: Bailey Lissington, Dillon Pike, Joseph Ramirez
Date: 24 March 2021

Helicopter Rig Controller Project: Milestone 1

Programmed for the Tiva TM4C123GH6PM Microcontroller and Orbit Booster Board.

Accepts an analogue input on AIN9, PE4 (J1-05) up to a maximum of 3.3 volts. The analogue input is regularly sampled and the samples are stored in a circular buffer that has its mean calculated regularly. 

The mean at the initiation of the program corresponds to an altitude of 0% and increases to 100% when the input reduces by 1 volt. The mean corresponding to 0% altitude can be changed to the current mean with the left button.

The altitude can be displayed as a percentage, mean ADC value, or not displayed at all. This is toggled with the up button.