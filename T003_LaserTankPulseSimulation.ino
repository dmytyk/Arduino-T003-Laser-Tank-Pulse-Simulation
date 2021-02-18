// Laser
#ifndef LASERDIGITALPWM
    // D5 - PWM Out to TTL of Laser
    #define LASERDIGITALPWM  5
#endif

// ISR
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

// ISR vars
volatile boolean ISR_LaserOn = false;
volatile int ISR_LaserTargetCount = 0;
volatile boolean ISR_LaserFire = false;
volatile int ISR_LaserFireLength = 0;
volatile int ISR_LaserFireCount = 0;

// Background
boolean BackgroundHearBeat = false;

// Start MKR1010 software interrupt functions **********
uint16_t next_pow2(uint16_t v_)
{
    // the next power-of-2 of the value (if v_ is pow-of-2 returns v_)
    --v_;
    v_|=v_>>1;
    v_|=v_>>2;
    v_|=v_>>4;
    v_|=v_>>8;
    return v_+1;
}
uint16_t get_clk_div(uint32_t freq_)
{
    float ideal_clk_div=48000000.0f/(256.0f*float(freq_));
    uint16_t clk_div=next_pow2(uint16_t(ceil(ideal_clk_div)));
    switch(clk_div)
    {
        case 32: clk_div=64; break;
        case 128: clk_div=256; break;
        case 512: clk_div=1024; break;
    }
    return clk_div;
}
void setup_timer4(uint32_t freq_)
{
    uint16_t clk_div=get_clk_div(freq_);
    uint8_t clk_cnt=(48000000/clk_div)/freq_;
    setup_timer4(clk_div, clk_cnt);
}

void setup_timer4(uint16_t clk_div_, uint8_t count_)
{
    // Set up the generic clock (GCLK4) used to clock timers
    REG_GCLK_GENDIV = GCLK_GENDIV_DIV(1) |          // Divide the 48MHz clock source by divisor 1: 48MHz/1=48MHz
                    GCLK_GENDIV_ID(4);              // Select Generic Clock (GCLK) 4
    while (GCLK->STATUS.bit.SYNCBUSY);              // Wait for synchronization

    REG_GCLK_GENCTRL = GCLK_GENCTRL_IDC |           // Set the duty cycle to 50/50 HIGH/LOW
                     GCLK_GENCTRL_GENEN |           // Enable GCLK4
                     GCLK_GENCTRL_SRC_DFLL48M |     // Set the 48MHz clock source
                     GCLK_GENCTRL_ID(4);            // Select GCLK4
    while (GCLK->STATUS.bit.SYNCBUSY);              // Wait for synchronization

    // Feed GCLK4 to TC4 and TC5
    REG_GCLK_CLKCTRL = GCLK_CLKCTRL_CLKEN |         // Enable GCLK4 to TC4 and TC5
                     GCLK_CLKCTRL_GEN_GCLK4 |       // Select GCLK4
                     GCLK_CLKCTRL_ID_TC4_TC5;       // Feed the GCLK4 to TC4 and TC5
    while (GCLK->STATUS.bit.SYNCBUSY);              // Wait for synchronization

    REG_TC4_CTRLA |= TC_CTRLA_MODE_COUNT8;          // Set the counter to 8-bit mode
    while (TC4->COUNT8.STATUS.bit.SYNCBUSY);        // Wait for synchronization

    REG_TC4_COUNT8_CC0 = count_;                    // Set the TC4 CC0 register to some arbitary value
    while (TC4->COUNT8.STATUS.bit.SYNCBUSY);        // Wait for synchronization

    NVIC_SetPriority(TC4_IRQn, 0);                  // Set the Nested Vector Interrupt Controller (NVIC) priority for TC4 to 0 (highest)
    NVIC_EnableIRQ(TC4_IRQn);                       // Connect TC4 to Nested Vector Interrupt Controller (NVIC)

    REG_TC4_INTFLAG |= TC_INTFLAG_OVF;              // Clear the interrupt flags
    REG_TC4_INTENSET = TC_INTENSET_OVF;             // Enable TC4 interrupts

    uint16_t prescale=0;
    switch(clk_div_)
    {
        case 1:    prescale=TC_CTRLA_PRESCALER(0); break;
        case 2:    prescale=TC_CTRLA_PRESCALER(1); break;
        case 4:    prescale=TC_CTRLA_PRESCALER(2); break;
        case 8:    prescale=TC_CTRLA_PRESCALER(3); break;
        case 16:   prescale=TC_CTRLA_PRESCALER(4); break;
        case 64:   prescale=TC_CTRLA_PRESCALER(5); break;
        case 256:  prescale=TC_CTRLA_PRESCALER(6); break;
        case 1024: prescale=TC_CTRLA_PRESCALER(7); break;
    }
    REG_TC4_CTRLA |= prescale | TC_CTRLA_WAVEGEN_MFRQ | TC_CTRLA_ENABLE;  // Enable TC4
    while (TC4->COUNT8.STATUS.bit.SYNCBUSY);                              // Wait for synchronization
}
// End MKR1010 software interrupt functions **********

// Interrupt Service Routine (ISR)
void TC4_Handler()
{
  if (TC4->COUNT16.INTFLAG.bit.OVF && TC4->COUNT16.INTENSET.bit.OVF)
  {
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

    // keep - clear interrupt
    REG_TC4_INTFLAG = TC_INTFLAG_OVF;
  }
}

void setup()
{
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
}

void loop()
{
    String S_input;
    while(Serial.available())
    {
        S_input = Serial.readString();// read the incoming data as string
        S_input.trim();
        Serial.println(S_input);
    }

    // Target LED to on
    if(S_input == "on") {
        ISR_LaserOn = true;
        S_input = "";
    } else if (S_input == "off") {
        ISR_LaserOn = false;
        delay(10);
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

    // Background Process 5
    // show the background heart
    BackgroundHearBeat = !BackgroundHearBeat;
    if(BackgroundHearBeat) {
        digitalWrite(LED_BUILTIN, LOW);
        delay(500);  
    } else {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(500);  
    }
}
