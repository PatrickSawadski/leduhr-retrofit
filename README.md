# leduhr-retrofit
This project is about updating my old selfbuild discrete-CMOS-logic led clock with some intelligent brain to add some neat animations.

There is some light documentation here:
http://www.patrick-sawadski.de/projekte/1-Leduhr

I made the clock back when I was about 14 years old. At this time, I was learning the basics of digital electronics with a breadboard and some 4000 series CMOS ICs. The clock consists of 4015 shift registers in a simple configuration, so that the led circles for hours, minutes and seconds fill clockwise starting from 12 o'clock until the circle is full, then reset.

When the clock got into my hands a few days ago, I got this brilliant idea of speeding things up from 1 Hz to a few kHz and use the existing shift registers to put in serialized data, so it is possible to play patterns and animations instead of the "hardcoded" fill-from-12-o'clock function.
