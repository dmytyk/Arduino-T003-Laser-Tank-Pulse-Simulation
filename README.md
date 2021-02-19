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

> #### Laser Tank's Arduino Requirements
> - For Targeting
>   - a 50 microsecond pulse every 5 milliseconds, enough to generate a good beam for targeting purposes
> - Firing
>   - Adjust the output power via software control and fire the Laser at that power level, to do this I created the ability to specify in millisecond increments, the amount of time the laser is on for example
>       - Command - L1 = 1 millisecond, 20% of full power
>       - Command - L4 = 4 milliseconds, 80% of full power
>       - Command - L5 = 5 milliseconds, 100% full power
>       - Command - L20 = 20 milliseconds, 100% full power for 20 milliseconds

Based on these requirements I created this [Arduino Code](/T003_LaserTankPulseSimulation.ino) to simulate the controllers output.

NOTE: I will not be detailing the code related to the Software Interrupt (future tutorial), just the code to generate the targeting and fire pulses. The Software Interrupt code configures the Arduino to trigger a 50 microsecond Interrupt Service Routine, that should be enough background information to detail the controllers pulse simulation code.

- Constants
    - Setup the hardware pins and create a couple of defines needed to count 50 microsecond pulses to get desired lengths of time
```sh
    // Laser Constants
    #ifndef LASERDIGITALPWM
        // D5 - PWM Out to TTL of Laser
        #define LASERDIGITALPWM  5
    #endif
    
    // ISR Constants
    // call ISR - TC4_Handler 20000 times per second
    // an interrupt is called every 50 microseconds so to get:
    // count 1 interrupts = 50us
    // count 20 interrupts = 1ms
    // count 100 interrupts = 5ms
    // count 200 interrupts = 10ms
    // count 20000 interrupts = 1s
    // count 60000 interrupts = 3s
    // count 100000 interrupts = 5s
    #ifndef ISR_1MSECS
        // number of 50usecs we need to get 1msec for the laser fire length,of the user defined time in msecs
        #define ISR_1MSECS 20
    #endif
    #ifndef ISR_5MSECS
        // pulse the laser every 5 milliseconds
        #define ISR_5MSECS 100
    #endif
```

- Vars
    - System vars needed as noted in the comments
```sh
    // ISR vars
    volatile boolean ISR_LaserOn = false;       // tells the Arduino to turn on the laser to start the targeting process  
    volatile int ISR_LaserTargetCount = 0;      // counts 50 usec pulses to trigger events related to targeting as defined in the code 
    volatile boolean ISR_LaserFire = false;     // flag used to indicate when it's time to fire the laser
    volatile int ISR_LaserFireLength = 0;       // var used to hold the users input as to how long you want to laser on
    volatile int ISR_LaserFireCount = 0;        // counts 50 usec pulses to trigger events related to targeting as defined in the code
    
    // Background Vars
    boolean BackgroundHearBeat = false;         // used to let us know the background is running
```

- ISR code
    - This code runs every 50 usecs. The ISR_LaserFire and ISR_LaserOn flags are set by the background code running in the loop code block.  For this example we replace our web interface input with input from the serial terminal.
        - When the ISR_LaserOn flag is set we start counting interrupts, the first count turns the laser on, the second (50 usecs later) we turn the laser off (i.e. our first 50 usec pulse) then we count up to 5 msec's, reset the count, so we can start over again.
        - When the ISR_LaserFire flag is set we turn the laser on for the amount of time specified by (ISR_LaserFireLength * ISR_1MSECS), user defined time in msec's times the number of 50 usec counts need to get 1 msec (20).  Once we reach the count we turn off the laser and clear the ISR_LaserFire flag because we are done we fired the laser.

Note: The laser can be targeting, told to fire and then return to targeting.   

```sh
    // see if Targeting is active and/or we were asked to Fire the Laser
    if(ISR_LaserFire) {
        // Fire
        // the length of the pulse is determined by the value of ISR_LaserFireLength (as milliseconds)
        if(ISR_LaserFireCount == 0) {
            digitalWrite(LASERDIGITALPWM, HIGH);
            ISR_LaserFireCount++;
        } else {
            ISR_LaserFireCount++;
            if(ISR_LaserFireCount == ((ISR_LaserFireLength * ISR_1MSECS))) {
                digitalWrite(LASERDIGITALPWM, LOW);
                ISR_LaserFireCount = 0;
                ISR_LaserFire = false;
            }
        }
    } else if (ISR_LaserOn) {
        // Targeting
        // generate a 50 microsecond digital pulse every 5 milliseconds
        ISR_LaserTargetCount++;
        if(ISR_LaserTargetCount == 1) {
            digitalWrite(LASERDIGITALPWM, HIGH);
        }
        // 50 microseconds each interrupt
        if(ISR_LaserTargetCount == 2) {
            digitalWrite(LASERDIGITALPWM, LOW);
        }
        // do this every 5 milliseconds
        if(ISR_LaserTargetCount == ISR_5MSECS) {
            ISR_LaserTargetCount = 0;
        }
    }
```

- Setup Code Block
    - Setup the hardware Pins
    - setup_timer4(20000) configure the software interrupt to run every 50 microseconds
```sh
    Serial.begin(9600); // opens serial port, sets data rate to 9600 bps

    // we use this led to show the background is running
    // flased on and off every time we go through the background loop
    pinMode(LED_BUILTIN, OUTPUT);
    
    // Digital PWM Pin
    pinMode(LASERDIGITALPWM, OUTPUT);
    digitalWrite(LASERDIGITALPWM, LOW); // No PWM

    // done initilization so start the interrupts
    // call ISR - TC4_Handler 20000 times per second
    // an interrupt is called every 50 microseconds
    setup_timer4(20000);
```

- Loop Code Block
    - Set up the serial port, so we can interact with the Arduino
    - "Target LED to on", tell the laser to start/stop targeting.  If off, tell the ISR to stop targeting, wait 5 msecs in case it was already targeting and then ensure the laser is off. 
    - "Fire Laser for user defined time", get the input from the user, save it to ISR_LaserFireLength, then tell the ISR to fire the laser for that amount of time 
```sh
    String S_input;
    while(Serial.available())
    {
        S_input = Serial.readString();// read the incoming data as string
        S_input.trim();
        Serial.println(S_input);
    }

    // Targeting on/off
    if(S_input == "on") {
        ISR_LaserOn = true;
        S_input = "";
    } else if (S_input == "off") {
        ISR_LaserOn = false;
        delay(5);
        digitalWrite(LASERDIGITALPWM, LOW);
        S_input = "";
    }

    // Fire Set the Fire LED to on for user defined time
    if(S_input.length() > 0) {
        // remove the L
        S_input.remove(0, 1);

        // set LASERDIGITALPWM
        Serial.println("LASERDIGITALPWM: " + S_input + " msec");
        ISR_LaserFireLength =  S_input.toInt();
        ISR_LaserFire = true;
        ISR_LaserFireCount = 0;
    }
```

This video [Arduino Simulation](https://www.youtube.com/watch?v=wl15-C5hTR0&t=5s) demonstrates the Arduino simulating the controller.

## Next Up
> - A bottle rocket launcher attachment for a Drone
> - A Paratrooper dropping device an RC airplane / Drone
> - A cool GEO cache device (my grandkids love to find them)
> - Several cool Arduino tutorials for Software Interrupts, Background Processing, Board to Board communication and Fun stuff TBD

Please follow me on [YouTube](https://www.youtube.com/channel/UClwcP7ByE6Ia9DmKfP0C-UQ) to catch the "Next Up Stuff" and thanks for Hanging out at the Shack!



