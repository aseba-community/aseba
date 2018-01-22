Thymio Programming Interface
============================

This page describes the programming capabilities of Thymio. It lists
the different variables and functions and indicates to which elements
of the robot they refer.

Standard library
----------------

The Thymio comes with the Aseba `standard library of native functions,
documented on its own page <aseba-std-natives.rst>`__.

Buttons
-------

Thymio holds 5 capacitive buttons corresponding to the arrows and to the
central button. Five variables hold the state of these buttons (1 =
pressed, 0 = released):

-  ``button.backward`` : backward arrow
-  ``button.left`` : left arrow
-  ``button.center`` : central button
-  ``button.forward`` : forward arrow
-  ``button.right`` : right arrow

Thymio updates this array at a frequency of 20 Hz and generates the
``buttons`` event after every update. Moreover, for each of these
buttons, when it is pressed or released, a corresponding event with the
same name is fired.

Distance sensors
----------------

Horizontal
~~~~~~~~~~

Thymio has 7 distance sensors around its periphery. An array of 7
variables, ``prox.horizontal``, holds the values of these sensors:

-  ``prox.horizontal[0]`` : front left
-  ``prox.horizontal[1]`` : front middle-left
-  ``prox.horizontal[2]`` : front middle
-  ``prox.horizontal[3]`` : front middle-right
-  ``prox.horizontal[4]`` : front right
-  ``prox.horizontal[5]`` : back left
-  ``prox.horizontal[6]`` : back right

The values in this array vary from 0 (the robot does not see anything)
to several thousand (the robot is very close to an obstacle). Thymio
updates this array at a frequency of 10 Hz, and generates the ``prox``
event after every update.

Ground
~~~~~~

Thymio holds 2 ground distance sensors. These sensors are located at the
front of the robot. As black grounds appear like no ground at all (black
absorbs the infrared light), these sensors can be used to follow a line
on the ground. Three arrays hold the values of these sensors:

-  ``prox.ground.ambiant`` : ambient light intensity at the ground,
   varies between 0 (no light) and 1023 (maximum light)
-  ``prox.ground.reflected`` : amount of light received when the sensor
   emits infrared, varies between 0 (no reflected light) and 1023
   (maximum reflected light)
-  ``prox.ground.delta`` : difference between reflected light and
   ambient light, linked to the distance and to the ground colour.

For each array, the index 0 corresponds to the left sensor and the index
1 to the right sensor. As with the distance sensors, Thymio updates this
array at a frequency of 10 Hz and generates the (same) ``prox`` event
after every update.

Local communication
-------------------

Thymio can use its horizontal infrared distance sensors to communicate a
value to peer robots within a range of about 15 cm. This value is sent
at 10 Hz while processing the distance sensors. Thymio sends an 11-bit
value (but future firmware could use one of the bits for internal use,
thus it is better to stay within 10 bits). To use the communication,
call the ``prox.comm.enable(state)`` function, with 1 in ``state`` to
enable communication or 0 to turn it off. If the communication is
enabled, the value in the ``prox.comm.tx`` variable is transmitted every
100 ms. When Thymio receives a value, the event ``prox.comm`` is fired
and the value is in the ``prox.comm.rx`` variable.

Accelerometer
-------------

Thymio contains a 3-axes accelerometer. An array of 3 variables,
``acc``, holds the values of the acceleration along these 3 axes:

-  ``acc[0]`` : x-axis (from right to left, positive towards left)
-  ``acc[1]`` : y-axis (from front to back, positive towards the rear)
-  ``acc[2]`` : z-axis (from top to bottom, positive towards ground)

The values in this array vary from -32 to 32, with 1 g (`the
acceleration of the earth's
gravity <http://en.wikipedia.org/wiki/Earth%27s_gravity>`__)
corresponding to the value 23. Thymio updates this array at a frequency
of 16 Hz, and generates the ``acc`` event after every update. Moreover,
when a shock is detected, a ``tap`` event is emitted.

Temperature sensor
------------------

The ``temperature`` variable holds the current temperature in tenths of
a degree Celsius. Thymio updates this value at 1 Hz and generates the
``temperature`` event after every update.

Timers
------

Thymio provides two user-defined timers. An array of 2 values,
``timer.period``, allows to specify the period of the timers in ms:

-  ``timer.period[0]`` : period of timer 0 in milliseconds
-  ``timer.period[1]`` : period of timer 1 in milliseconds

The timer starts the countdown when it is initialized.

When the period expires, the timer generates a ``timer0`` respectively
``timer1`` event. These events are managed in the same way as all the
others and cannot interrupt an already executing event handler.

If you restart a program with a timer, the timer could still be counting
down and an event can occur before you expect it. This is not usually a
problem if you use a timer that expires repeatedly at short intervals,
because you can set a state variable to ignore timer events until you
are ready. It is recommended that you *not* use the timer for a *single
(long) interval* because the results can be inconsistent.

LEDs
----

Thymio holds many LEDs spread around its body. Most of them are
associated with sensors and can highlight their activations: by default,
the intensity of the LED is linked to the sensor value. However, once
LEDs are used in the code, the programmer takes over control and they no
longer reflect the sensor values.

Native functions allow the various LEDs to be controlled. For all LEDs,
their intensity values range from 0 (off) to 32 (fully lit).

The LED circle on top of the robot
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

8 yellow LEDs make up a circle on top of the robot, around the buttons.

*Default activation*: reflects the values of the accelerometer. All LEDs
are off when the robot is horizontal. When the robot tilts, a single LED
shows the lowest point, with an intensity proportional to the tilt
angle.

-  ``leds.circle(led 0, led 1, led 2, led 3, led 4, led 5, led 6, led 7)``
   where ``led 0`` sets the intensity of the LED at the front of the
   robot, the others are numbered clockwise.

The RGB LEDs
~~~~~~~~~~~~

There are two RGB LEDs on the top of robot, driven together. These are
the LEDs that show the behaviour of the robot. There are two other RGB
LEDs on the bottom of the robot, which can be driven separately.

*Default activation*: off when in Aseba mode.

-  ``leds.top(red, green, blue)`` sets the intensities of the top LEDs.
-  ``leds.bottom.left(red, green, blue)`` sets the intensities of the
   bottom-left LED.
-  ``leds.bottom.right(red, green, blue)`` sets the intensities of the
   bottom-right LED.

The LEDs of proximity sensors
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Every proximity sensor has a companion red LED on its side (the front
sensor has two LEDs, one on each side).

*Default activation*: on when an object is close to the associated
sensor, with an intensity inversely proportional to the distance.

-  ``leds.prox.h(led 1, led 2, led 3, led 4, led 5, led 6, led 7, led 8)``
   sets the LEDs of the front and back horizontal sensors. ``led 1`` to
   ``led 6`` correspond to the front LEDs, from left to right, while
   ``led 7`` and ``led 8`` correspond to the left and right back LEDs.
-  ``leds.prox.v(led 1, led 2)`` sets the LEDs associated with the
   bottom sensors, left and right.

The Button LEDs
~~~~~~~~~~~~~~~

Four red LEDs are placed between the buttons.

*Default activation*: For each arrow button, one LED lights up when it
is pressed. When the centre button is pressed, all four LEDs light up.

-  ``leds.buttons(led 1, led 2, led 3, led 4)`` control these LEDs, with
   ``led 1`` corresponding to the front LED, then clockwise numbering.

The LED of the RC receiver
~~~~~~~~~~~~~~~~~~~~~~~~~~

This red LED is located close to the remote-control (infrared) receiver.

*Default activation*: blinks when the robot receives an
`RC5 <http://en.wikipedia.org/wiki/RC-5>`__ code.

-  ``leds.rc(led)`` controls this LED.

The LEDs of the temperature sensor
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

These two LEDs (one red and one blue) are located close to the
temperature sensor.

*Default activation*: red if the temperature is over 28°C, red and blue
between 28° and 15°, blue if the temperature is below 15°.

-  ``leds.temperature(red, blue)`` controls this LED.

The microphone LED
~~~~~~~~~~~~~~~~~~

This blue LED is located close to the microphone.

*Default activation*: off.

-  ``leds.sound(led)`` controls this LED.

There are also other LEDs that the user cannot control:

-  3 green LEDs on the top of the robot show the battery voltage
-  a blue and a red LED on the back of the robot show the charge status
-  a red LED on the back of the robot shows the SD-card status

Motors
------


You can change the wheel speeds by writing in these variables:

-  ``motor.left.target``: requested speed for left wheel
-  ``motor.right.target``: requested speed for right wheel

You can read the real wheel speeds from these variables:

-  ``motor.left.speed`` : real speed of left wheel
-  ``motor.right.speed`` : real speed of right wheel

The values range from -500 to 500. A value of 500 approximately
corresponds to a linear speed of 20 cm/s. You can read the value of the
motor commands from the variables ``motor.left.pwm`` and
``motor.right.pwm``.

Sound
-----

Sound-intensity detection
~~~~~~~~~~~~~~~~~~~~~~~~~

| The Thymio can detect when the ambient sound is above a given
  intensity and emit an event.
| The variable ``mic.intensity`` shows the current microphone intensity
  (in the range 0 to 255), while variable ``mic.threshold`` contains the
  limit intensity for the event. If ``mic.intensity`` is above
  ``mic.threshold``, then the event ``mic`` is generated.

Playing and recording sounds
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

You can play synthetic or system sounds. Moreover, if you have installed
a `micro-SD <http://en.wikipedia.org/wiki/MicroSD#microSD>`__ card
formatted as `FAT <http://en.wikipedia.org/wiki/Fat16>`__, you can
record and play your own sounds. The files are stored in the micro-SD
card, in `wave <http://en.wikipedia.org/wiki/Wav>`__ format, 8-bit
unsigned, 8 kHz. When Thymio finishes playing a sound requested through
Aseba, it fires the event ``sound.finished``. It does not fire an event
if playing is interrupted or if a new sound is played.

Synthetic sound
~~~~~~~~~~~~~~~

The native function ``sound.freq`` plays a frequency, specified in Hz,
for a certain duration, specified in 1/60 s. Specifying a 0 duration
plays the sound continuously and specifying a -1 duration stops the
sound.

Changing the primary wave
~~~~~~~~~~~~~~~~~~~~~~~~~

Synthetic sound generation works by re-sampling a primary wave. By
default, it is a triangular wave, but you can define your own wave using
the ``sound.wave`` native function. This function takes as input an
array of 142 samples, with values from -128 to 127. This buffer should
represent one wave of the tonic frequency specified in ``sound.freq``.
As Thymio plays sounds at 7812.5 Hz, this array is played completely at
the frequency of 7812.5/142 = ~55 Hz. Playing a sound of a higher
frequency skips samples in the array.

Recording
~~~~~~~~~

You can record sounds using the ``sound.record`` native function. This
function takes as parameter a record number from 0 to 32767. Files are
stored on the micro-SD card under the name ``Rx.wav`` where ``x`` is the
parameter passed to the ``sound.record`` function. To stop a recording,
call the ``sound.record`` function with the value of -1.

Replaying
~~~~~~~~~

You can replay a recorded sound using the ``sound.replay`` native
function. This function takes as parameter a record number from 0 to
32767 and will replay file ``Rx.wav`` from the SD card where ``x`` is
the parameter passed to the ``sound.replay`` function. To stop a replay,
call the ``sound.replay`` function with the value of -1.

Duration (from firmware version 11)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

You can retrieve the duration of a recorded sound using the
``sound.duration(x,duration)`` native function. Its first parameter,
``x``, is a number from 0 to 32767 which is the index of file ``Rx.wav``
from the SD card. The result in 1/10 of seconds is put in the variable
``duration`` as second parameter.

Creating sound on your computer
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

You can create sounds for Thymio using your computer. An efficient way
to do so is to use the `Audacity <http://audacity.sourceforge.net/>`__
software, version 1.3, which exists for various operating systems. Here
are the steps to create a sound compatible with the Thymio:

-  Once Audacity has started, change the *project rate* from 44100 Hz
   (default) to 8000 Hz. This setting is located at the bottom-left of
   Audacity's window.
-  Record your sound with the red record key in the top-left part of the
   window. You should see the cursor advancing and the wave changing.
   Stop with the stop button.
-  Your sound should be in mono (Tracks->Stereo to Mono)
-  Go to the *File* menu under *Export…*
-  Give a file name, for instance ``P0.wav`` for the first file to play
   using the ``sound.play`` native function.
-  Choose *other uncompressed files* as format *format*.
-  Under *options*, choose a *WAV (Microsoft)* header and as *Encoding*,
   choose *Unsigned 8 bit PCM*.
-  Make sure that no metadata values ares set.
-  Save or copy the file to the micro-SD card.

Here's an `instructional
video <http://www.youtube.com/watch?v=aWtPvnLYMps>`__ on how to do the
above.

Play
~~~~

You can play a user-defined sound using the ``sound.play`` native
function, which takes a record number from 0 to 32767 as parameter. The
file must be available on the micro-SD card under the name ``Px.wav``
where ``x`` is the parameter passed to the ``sound.play`` function. To
stop playing a sound, call the ``sound.play`` function with the value
-1.

System sound
~~~~~~~~~~~~


You can play a system sound using the ``sound.system`` native function,
which takes a record number from 0 to 32767 as parameter. Some sounds
are available in the firmware (see below), but you can overwrite these
sounds and add new ones using the SD-card. In this case, the file must
be named ``Sx.wav`` where ``x`` is the parameter passed to the
``sound.system`` function. To stop playing a sound, call the
``sound.system`` function with the value -1.

System sound library
~~~~~~~~~~~~~~~~~~~~

The following sounds are available:

+-------------+-----------------------------------------------------+
| parameter   | description                                         |
+=============+=====================================================+
| ``-1``      | stop playing sound                                  |
+-------------+-----------------------------------------------------+
| ``0``       | startup sound                                       |
+-------------+-----------------------------------------------------+
| ``1``       | shutdown sound (this sound is not reconfigurable)   |
+-------------+-----------------------------------------------------+
| ``2``       | arrow button sound                                  |
+-------------+-----------------------------------------------------+
| ``3``       | central button sound                                |
+-------------+-----------------------------------------------------+
| ``4``       | free-fall (scary) sound                             |
+-------------+-----------------------------------------------------+
| ``5``       | collision sound                                     |
+-------------+-----------------------------------------------------+
| ``6``       | target ok for friendly behaviour                    |
+-------------+-----------------------------------------------------+
| ``7``       | target detect for friendly behaviour                |
+-------------+-----------------------------------------------------+


Remote control
--------------

Thymio contains a receiver for infrared remote controls compatible with
the `RC5 <http://en.wikipedia.org/wiki/RC-5>`__ protocol. When Thymio
receives an RC5 code, it generates the ``rc5`` event. In this case, the
variables ``rc5.address`` and ``rc5.command`` are updated.


Read and write data from the SD card
------------------------------------

If an SD card is present, the variable ``sd.present`` is set to 1
(otherwise 0), and Thymio can read and write data to files. Only a
single file can be open at any given time. The unit of reading/writing
is a signed 16-bit binary value. The functions provided are:

-  ``sd.open(x,status)``: opens the file ``Ux.DAT``. The value ``x``
   should be a number between [0:32767], using -1 closes the currently
   open file. A value of 0 is written in the ``status`` variable if the
   operation was successful, -1 if the operation has failed.
-  ``sd.write(data,written)``: attempts to write the complete ``data``
   array in the currently opened file. The number of values written is
   returned in the ``written`` parameter. It should be equal to the size
   of ``data``, except if the card was full, or if the file was larger
   than 4 Gb, or no file was open.
-  ``sd.read(data,read)``: reads and fills the ``data`` array from the
   currently opened file. The number of values read is returned in the
   ``read`` parameter. It should be equal to the size of ``data``,
   except when the end of the file is encountered or no file was open.
-  ``sd.seek(position,status)``: moves the current read and write
   pointers in the currently opened file. The cursor is moved to the
   absolute ``position`` in the opened file. The valid range is
   [0:65535]. It is currently not possible to seek to a position after
   65535. A value of 0 is written in the ``status`` variable if the
   operation was successful, -1 if the operation has failed.

The format consist of a simple concatenation of the signed 16-bit binary
values.

**Note: do not remove the SD card while the robot is turned on. Always
power-off the robot before removing the SD card.**

Loading a program from the SD card
----------------------------------

Thymio can load a program from the SD card. When it boots, Thymio loads
the file ``vmcode.abo`` from the SD card if present.

To obtain the ``vmcode.abo`` file from your .aesl file, open Aseba
Studio and open your program (let's call it ``myprogram.aesl``). Then
click on (**1**) "Tool", then (**2**) "Save binary code…", then (**3**)
"…of thymio". You will see a dialog box opening (**4**). Choose a place
where to save your file and that's it, you saved ``myprogram.aesl`` with
the .abo format. Don't forget to call it ``vmcode.abo`` if you want your
Thymio to read it when it starts.

Table of local events
---------------------

+-----------------------+-----------------------------------------------------------+--------------------------+-------------------------------------------------------------------------------------------------------------------------+
| event                 | description                                               | frequency (Hz)           | result                                                                                                                  |
+=======================+===========================================================+==========================+=========================================================================================================================+
| ``button.backward``   | back arrow was pressed or released                        | upon action              | ``button.backward``                                                                                                     |
+-----------------------+-----------------------------------------------------------+--------------------------+-------------------------------------------------------------------------------------------------------------------------+
| ``button.left``       | left arrow was pressed or released                        | upon action              | ``button.left``                                                                                                         |
+-----------------------+-----------------------------------------------------------+--------------------------+-------------------------------------------------------------------------------------------------------------------------+
| ``button.center``     | central button was pressed or released                    | upon action              | ``button.center``                                                                                                       |
+-----------------------+-----------------------------------------------------------+--------------------------+-------------------------------------------------------------------------------------------------------------------------+
| ``button.forward``    | front arrow was pressed or released                       | upon action              | ``button.forward``                                                                                                      |
+-----------------------+-----------------------------------------------------------+--------------------------+-------------------------------------------------------------------------------------------------------------------------+
| ``button.right``      | right arrow was pressed or released                       | upon action              | ``button.right``                                                                                                        |
+-----------------------+-----------------------------------------------------------+--------------------------+-------------------------------------------------------------------------------------------------------------------------+
| ``buttons``           | button values have been probed                            | 50                       | ``buttons.backward``, ``buttons.left``, ``buttons.center``, ``buttons.forward``, ``buttons.right``                      |
+-----------------------+-----------------------------------------------------------+--------------------------+-------------------------------------------------------------------------------------------------------------------------+
| ``prox``              | proximity sensors were read                               | 10                       | ``prox.horizontal[0-7]``, ``prox.ground.ambiant[0-1]``, ``prox.ground.reflected[0-1]`` and ``prox.ground.delta[0-1]``   |
+-----------------------+-----------------------------------------------------------+--------------------------+-------------------------------------------------------------------------------------------------------------------------+
| ``prox.comm``         | value received from IR sensors                            | upon value reception     | ``prox.comm.rx``                                                                                                        |
+-----------------------+-----------------------------------------------------------+--------------------------+-------------------------------------------------------------------------------------------------------------------------+
| ``tap``               | a shock was detected                                      | upon shock               | ``acc[0-2]``                                                                                                            |
+-----------------------+-----------------------------------------------------------+--------------------------+-------------------------------------------------------------------------------------------------------------------------+
| ``acc``               | the accelerometer was read                                | 16                       | ``acc[0-2]``                                                                                                            |
+-----------------------+-----------------------------------------------------------+--------------------------+-------------------------------------------------------------------------------------------------------------------------+
| ``mic``               | ambient sound intensity was above threshold               | when condition is true   | ``mic.intensity``                                                                                                       |
+-----------------------+-----------------------------------------------------------+--------------------------+-------------------------------------------------------------------------------------------------------------------------+
| ``sound.finished``    | a sound started by aseba has finished playing by itself   | when sound finishes      |                                                                                                                         |
+-----------------------+-----------------------------------------------------------+--------------------------+-------------------------------------------------------------------------------------------------------------------------+
| ``temperature``       | temperature was read                                      | 1                        | ``temperature``                                                                                                         |
+-----------------------+-----------------------------------------------------------+--------------------------+-------------------------------------------------------------------------------------------------------------------------+
| ``rc5``               | the infrared remote-control receiver got a signal         | upon signal reception    | ``rc5.address`` and ``rc5.command``                                                                                     |
+-----------------------+-----------------------------------------------------------+--------------------------+-------------------------------------------------------------------------------------------------------------------------+
| ``motor``             | PID is executed                                           | 100                      | ``motor.left/right.speed``, ``motor.left/right.pwm``                                                                    |
+-----------------------+-----------------------------------------------------------+--------------------------+-------------------------------------------------------------------------------------------------------------------------+
| ``timer0``            | when timer 0 period expires                               | user-defined             |                                                                                                                         |
+-----------------------+-----------------------------------------------------------+--------------------------+-------------------------------------------------------------------------------------------------------------------------+
| ``timer1``            | when timer 1 period expires                               | user-defined             |                                                                                                                         |
+-----------------------+-----------------------------------------------------------+--------------------------+-------------------------------------------------------------------------------------------------------------------------+
