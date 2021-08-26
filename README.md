System Architecture

* High Level Application:
        
       This device was specifically created for electric guitars. In fact, the 5khz cutoff frequency
for the antialiasing filter was chosen considering that electric guitar frequencies vary between
80Hz to 2kHz. The system recorded up to 4 minutes and 30 seconds of a sound. The absolute
maximum duration was not tested, but we deemed this to be a sufficient metric. Most of the
testing was done using the push button since most of the artists use the device by choosing when
to start and stop the recording. The reaction time of the recording is seemingly instantaneous. No
perceptible latency due to the pushbutton was noticed or measured. The mixing of the recorded
sound with the incoming signal is smooth and reliable. Little distortion can be noticed. The
reproduction speed of the recorded input is similar to the real time sound played. This allows the
recorded sound and the incoming sound to feel synchronized to outside hears. The recorded loop
is synchronized but has a slightly lower volume than the incoming sound. This is likely due to
our mixing algorithm, but this is more

*Low Level Implementation:
The sampling rate was measured to be around 35kHz. This was determined
experimentally through checking the loop index values after a set duration. The SPI clock
frequencies were tuned to produce the best loop audio quality (speed and minimal distortion). We
used the SPI0 and SPI6 interfaces on the Raspberry Pi 4 by modifying the config.text file under
the \boot directory. We had to specify that SPI6 was enabled, as well as set the CS pin (GPIO16)
using dtoverlay and dtparam respectively. We used polling to determine check for new inputs
from the foot switch to start and stop recording the loop. This was because it was easier to
implement than interrupts and did not appear to degrade the system performance. The hardware
implementation is described in the hardware schematic section and refers to what each stage is
doing.

Hardware Schematic:

![alt text](https://github.com/FilippoCheein/PedalPi/blob/master/Documentation/Guitar_Loop_Schematic.PNG?raw=true)

Software Flowchart:

![alt text](https://github.com/FilippoCheein/PedalPi/blob/master/Documentation/GUitar_Pedal_Software_Flowchart.PNG?raw=true)

