# Basic-control-system-for-autonomous-catamaran-Embedded-final-exam
 Implementation of  a basic control system for an autonomous catamaran using a microcontroller board, Microchip dsPIC30F.
 
 The project was the final exam of Embedded Systems, and was aimed to satisfy all the requirements listed below.

# Project 
### Goal: Implementing a basic control system for an autonomous catamaran. 
<p>A microcontroller board is connected to two outboard motors. The outboard motors are composed by a DC motor and a propeller installed
at the end of its shaft. Together, the two outboard motors allow the catamaran to move and rotate in the water. 
The microcontroller receives desired reference values for the rotation speed of the motors from a control PC,
in terms of motor RPMs (rounds per minute). These reference signals are sent through a serial interface.
The microcontroller sends a feedback messages back to the control PC to report a few status information.</p>

### Hardware specifications 
<ul>
<li>Each motor can run from -10000 to +10000 RPMs </li>
<li>The RPM are controlled through a PWM signal. </li>
<ul>
<li>The frequency should be at least 1 kHz. </li>
<li> 50% duty cycle corresponds to 0 RPM, 0% corresponds to -10000 RPM and 100% corresponds to 10000 RPMs</li>
</ul>
<li>The propeller installed on the shaft of each motor is rated for maximum RPMs of +-8000. Running the motor above +-8000 RPMs might damage the propeller and must be avoided.</li>
</ul>

### Firmware requirements
<ul>
<li> The control system must never generate PWM signals outside of the specifications of the motor and its propeller.</li> 
<ul>
<li> If any reference value is given outside the specifications, the system should saturate it to the minimum/maximum allowed value </li>
</ul>
<li> If no references are received for more than 5 seconds, the firmware should enter a timeout mode: </li>
<ul>
<li> Both motors velocity should be set to zero.</li>
<li>Led D4 should blink to signal timeout.</li>
<li> When a new reference is read, then the led D4 should stop blinking and commands should be given again to the motor. </li>
</ul>
<li> The firmware must support receiving references at least at 10 Hz frequency.</li>
<li> The firmware must refresh the PWM value at least at 10 Hz frequency </li>
<li> The firmware must acquire the temperature sensor at 10 Hz frequency and average the last 10 readings. The averaged value is sent to the PC at 1 Hz frequency with the MCTEM message </li>
<li> The firmware must send the feedback message MCFBK at 5 Hz frequency. </li>
<li> The control system should blink led D3 at 1 Hz to signal a correct functioning of the main loop. </li>
<li> The user can set new minimum and maximum values through a dedicated command (HLSAT): </li>
<ul>
<li> The firmware must check that these values are within the allowed range of the propeller. </li>
<li> The firmware must check that the min, max values are correctly set (i.e., min < max). </li>
<li> The zero value should be always allowed. </li>
<ul>
<li> If the above conditions are not met, the new values are not applied, and the firmware sends a negative ack message. </li>
<li> Otherwise, the new values are stored, the PWM is refreshed to comply with the new saturation values, and a positive ack is sent. </li>
</ul>
</ul>
<li>If any of the buttons (S5/S6) is pressed, the firmware should enter a safe mode: </li>
<ul>
<li> Motors are stopped immediately and reference signals are ignored until the microcontroller receives an enable message (HLENA).   </li>
<li> After exiting safe mode, the motors should be set to zero. Motors should move only after receiving a new reference. Once an enable command is received, the firmware should send a positive ack to the PC.  </li>
</ul>
<li> The firmware should write on the LCD   </li>
<ul>
<li>  First row: “STA: x  TEM: y”, where x = H/T/C  (halt/timeout/controlled), and y is the temperature (e.g. “STA: C   TEM: 22.3”) </li>
<li> Second row: “RPM: n1,n2”, where n1 and n2 are the applied RPM (e.g. “RPM: -1000,-2000”)  </li>
</ul>
</ul>
</ul>

### Messages from the PC 

<li> $HLREF,n1,n2* where n1 and n2 are the RPMs for the left and right motors respectively.  </li>
<li> $HLSAT,min,max* where min and max represent the minimum and maximum RPMs allowed.   </li>
<li> $HLENA* enables the firmware to send references to the motors (to exit safe mode)   </li>

### Messages to the PC 

<li> $MCFBK,n1,n2,state* where n1 and n2 are the applied reference signals and state is 2 if the microcontroller is in safe mode, 1 if it is in timeout mode, 0 otherwise  </li>
<li> $MCTEM,temp* where temp is the temperature   </li>
<li> $MCACK,msg_type,value* where msg_type is the command (e.g. ENA, SAT), and value is 1 if the message was applied and 0 otherwise (example: $MCACK,SAT,0* for a negative ack to the saturation command)   </li>


![dsPIC30F microcontrollerte](/Figures/board.jpg)
