=== littleKernel ===
This is an attempt to write a little kernel for the AVR platform. Currently this works on a ATMEGA328P. It could be modified to support others like the ATMEGA2560 (which I will do eventually, because I am running out RAM on this guy) by porting the task stack pointer logic and a few other minute changes.

My setup is runnning using a 16MHz xtal, and the task quantum is programmed via TIMER1 to be ~500us. This seems to work decently well. I have a pin toggling at what is programmatically 5Hz and my oscope says it's either dead on 5Hz or slightly below at 4.97Hz. 

![O-scope of output signal at 5Hz](images/IMG_000.BMP?raw=true "O-scope of output signal at 5Hz")