# Firmware

This folder contains the sources and project files of the firmware.
It is meant to be compiled with 
[Atmel Studio / Microchip Studio for AVR](https://www.microchip.com/en-us/tools-resources/develop/microchip-studio#Downloads).
No particular framework is used, just the
[avr-libc](https://www.nongnu.org/avr-libc/user-manual/)
that comes with the AVR-GCC, so it should be possible to compile it
with other IDEs without much porting effort.

If you don't need any change, you can program the supplied
[.hex file](Release/CableShieldTester.hex)
and you don't need any compiler at all.

Many modification can be achieved by changing the definitions in
config.h. (Always use Release and not Debug builds)

Whether you use the supplied .hex file or you compile yourself,
I suggest that you set BOD fuses of the chip to a level of 2.7V.
