# PulseBeat
An arduino program for a finger pulse oximeter 

The program calculates the realtime heartrate and SpO2 of the user by alternately lighting up the Red and Infrared sensor attached to the board and reading the sensor's value.
A realtime bandpass and lowpass filter is applied to the readings to smoothen the waveform and make the computation of heartrate and SpO2 easier.
The program is robust and resilient to operational errors and adjusts automatically to unexpected/changing readings.

## Features:
- Supports visualization of the pleth waveform
- Continuous, realtime filtering of the readings
- Detects finger in/out
- Supports changing orientation of display with single touch
- Intelligent peak detection using sliding-window approach
- Smiley/Sad face based on readings
