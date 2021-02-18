# Arduino-T003-Laser-Tank-Pulse-Simulation
This tutorial demonstrates how to use a Software Driven Interrupt to simulate/duplicate the TTL control pulses for a NEJE 20W (5.5W Output) 450nm Blue Light Laser Module.  My Laser Tank's (LT) laser came with a power supply and controller, however I wanted to power it with a Lipo Battery and control the laser's targeting and firing via my webpage interface.  

### The Laser, Controller and Power Supply

![Laser](/Images/Laser.JPG)

> #### Goals
> - Replace the +12v Power Supply with a Lipo Battery (previous tutorial)
> - Replace the controller (in TEST MODE it generated the control pulses the laser needed) and use my Arduino to target and fire the laser

### The Laser Tank with Laser

![Laser Tank](/Images/LaserTank.JPG)

### Test Setup

I used the test setup below to identify what the controller was doing, so I could simulate it.  Basically the controller generated a Pulse Width Modulation (PWM) signal from 0 to 5V.  At the lowest setting of 001 it generated a 50 microsecond pulse every 5 milliseconds, at the highest setting of 100 it generated a solid 5 volt level for the whole 5-millisecond period (no pulse).  Each increment from 1 to 100 added 50 microseconds to the pulse length.  So a setting of 20 would generate a 1 msec pulse putting the laser at 20% of full power.

![Test Setup](/Images/TestSetup.jpg)

This video [Controller Settings](https://youtu.be/tcf7VLcQio8) walks through changing the settings on the controller and observing the output. The observations will allow me to simulate/duplicate the controllers output using an Arduino microcontroller.

> #### Arduino Requirements
> - For Targeting
>   - a 50 microsecond pulse every 5 milliseconds, enough to generate a good beam for targeting purposes
> - Firing
>   - Adjust the output power via software control and fire the Laser at that power level, to do this I created the ability to specify in millisecond increments, the amount of time the laser is on for example
>       - Command - L1 = 1 millisecond, 20% of full power
>       - Command - L4 = 4 milliseconds, 80% of full power
>       - Command - L5 = 5 milliseconds, 100% full power
>       - Command - L20 = 20 milliseconds, 100% full power for 20 milliseconds

Based on these requirements I created the following [Arduino Code](/T003_LaserTankPulseSimulation.ino) to simulate the controllers output.










## Next Up
> - A bottle rocket launcher attachment for a Drone
> - A Paratrooper dropping device an RC airplane / Drone
> - A cool GEO cache device (my grandkids love to find them)
> - Several cool Arduino tutorials for Software Interrupts, Background Processing, Board to Board communication and Fun stuff TBD

Please follow me on Please follow me on [YouTube](https://www.youtube.com/channel/UClwcP7ByE6Ia9DmKfP0C-UQ) to catch the "Next Up Stuff"



