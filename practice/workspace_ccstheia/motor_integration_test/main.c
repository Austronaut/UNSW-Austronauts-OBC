#include <msp430.h>
#include <stdint.h>

/*
 * ============================================================
 * A4988 + NEMA-8 STEPPER MOTOR TEST
 * ============================================================
 *
 * MSP430FR5994        A4988
 * --------------------------------
 * P3.4  ------------> STEP
 * P3.5  ------------> DIR
 * P3.6  ------------> ENABLE
 * P7.3  ------------> SLEEP
 *
 * MS1, MS2 and MS3 are NOT connected to the MSP430.
 *
 * The A4988's internal pull-down resistors cause:
 *
 * MS1 = LOW
 * MS2 = LOW
 * MS3 = LOW
 *
 * Therefore the driver operates in FULL-STEP mode.
 */


/*
 * ============================================================
 * PIN DEFINITIONS
 * ============================================================
 */

#define STEP_PIN       BIT4    // P3.4 -> STEP
#define DIR_PIN        BIT5    // P3.5 -> DIR
#define ENABLE_PIN     BIT6    // P3.6 -> ENABLE

#define SLEEP_PIN      BIT3    // P7.3 -> SLEEP


/*
 * ============================================================
 * MOTOR CONFIGURATION
 * ============================================================
 */

// NEMA-8:
// 1.8 degrees per full step
// 200 steps = 360 degrees
#define STEPS_PER_REV      200

// Intended CPU frequency
#define CPU_FREQ_HZ        1000000UL

/*
 * Delay between steps.
 *
 * At ~1 MHz:
 *
 * 10000 cycles ~= 10 ms
 * ~100 steps/sec
 *
 * With 200 steps/revolution:
 *
 * ~0.5 rev/sec
 * ~30 RPM
 *
 * Increase this if the motor is struggling/jittering.
 */
#define STEP_DELAY_CYCLES  10000UL


/*
 * ============================================================
 * INITIALISE MOTOR GPIO
 * ============================================================
 */

void motor_init(void)
{
    /*
     * --------------------------------
     * PORT 3
     * --------------------------------
     *
     * P3.4 -> STEP
     * P3.5 -> DIR
     * P3.6 -> ENABLE
     */

    // Configure pins as GPIO
    P3SEL0 &= ~(STEP_PIN |
                DIR_PIN |
                ENABLE_PIN);

    P3SEL1 &= ~(STEP_PIN |
                DIR_PIN |
                ENABLE_PIN);

    // STEP starts LOW
    P3OUT &= ~STEP_PIN;

    // DIR starts LOW
    P3OUT &= ~DIR_PIN;

    /*
     * ENABLE is active LOW:
     *
     * LOW  = enabled
     * HIGH = disabled
     *
     * Start disabled.
     */
    P3OUT |= ENABLE_PIN;

    // Configure as outputs
    P3DIR |= (STEP_PIN |
              DIR_PIN |
              ENABLE_PIN);


    /*
     * --------------------------------
     * PORT 7
     * --------------------------------
     *
     * P7.3 -> SLEEP
     */

    P7SEL0 &= ~SLEEP_PIN;
    P7SEL1 &= ~SLEEP_PIN;

    /*
     * SLEEP is active LOW:
     *
     * LOW  = sleeping
     * HIGH = awake
     *
     * Start asleep.
     */
    P7OUT &= ~SLEEP_PIN;

    P7DIR |= SLEEP_PIN;
}


/*
 * ============================================================
 * ENABLE / DISABLE
 * ============================================================
 */

void motor_enable(void)
{
    // ENABLE is active LOW
    P3OUT &= ~ENABLE_PIN;
}


void motor_disable(void)
{
    // HIGH disables A4988 outputs
    P3OUT |= ENABLE_PIN;
}


/*
 * ============================================================
 * SLEEP / WAKE
 * ============================================================
 */

void motor_wake(void)
{
    // SLEEP HIGH -> A4988 awake
    P7OUT |= SLEEP_PIN;

    /*
     * Give the A4988 time to wake before
     * sending STEP pulses.
     */
    __delay_cycles(1000);
}


void motor_sleep(void)
{
    __delay_cycles(1000);
    // SLEEP LOW -> A4988 asleep
    P7OUT &= ~SLEEP_PIN;
}


/*
 * ============================================================
 * DIRECTION
 * ============================================================
 *
 * direction = 0 -> one direction
 * direction = 1 -> opposite direction
 */

void motor_set_direction(uint8_t direction)
{
    if (direction)
    {
        P3OUT |= DIR_PIN;
    }
    else
    {
        P3OUT &= ~DIR_PIN;
    }
}


/*
 * ============================================================
 * GENERATE ONE FULL STEP
 * ============================================================
 */

void motor_step(void)
{
    /*
     * A rising edge on STEP causes the
     * A4988 to advance one full step.
     */

    P3OUT |= STEP_PIN;

    // Keep STEP HIGH long enough
    __delay_cycles(10);

    P3OUT &= ~STEP_PIN;

    // Delay controls motor speed
    __delay_cycles(STEP_DELAY_CYCLES);
}


/*
 * ============================================================
 * ROTATE MOTOR
 * ============================================================
 */

void motor_rotate(uint16_t steps, uint8_t direction)
{
    uint16_t i;

    motor_set_direction(direction);

    // Give DIR time to settle
    __delay_cycles(100);

    for (i = 0; i < steps; i++)
    {
        motor_step();
    }
}


/*
 * ============================================================
 * MAIN
 * ============================================================
 */

int main(void)
{
    /*
     * Stop watchdog timer
     */
    WDTCTL = WDTPW | WDTHOLD;


    /*
     * ========================================================
     * CLOCK CONFIGURATION
     * ========================================================
     */

    CSCTL0_H = CSKEY_H;

    CSCTL1 = DCOFSEL_0;

    CSCTL2 = SELA__VLOCLK |
             SELS__DCOCLK |
             SELM__DCOCLK;

    CSCTL3 = DIVA__1 |
             DIVS__1 |
             DIVM__1;

    CSCTL0_H = 0;


    /*
     * ========================================================
     * INITIALISE GPIO
     * ========================================================
     */

    motor_init();

    // Unlock GPIO from default high-impedance mode
    PM5CTL0 &= ~LOCKLPM5;


    /*
     * Enable A4988 outputs.
     *
     * Driver is still asleep at this point.
     */
    motor_enable();


    /*
     * ========================================================
     * MOTOR TEST
     * ========================================================
     */
    motor_wake();

    while (1)
    {
        /*
         * Wake A4988
         */


        /*
         * Rotate one revolution forward.
         *
         * Full-step mode:
         *
         * 200 steps * 1.8 degrees = 360 degrees
         */
        motor_rotate(STEPS_PER_REV, 1);


        /*
         * Put A4988 to sleep.
         */
        // motor_sleep();


        /*
         * Wait approximately 2 seconds.
         */
        __delay_cycles(2 * CPU_FREQ_HZ);


        /*
         * Wake A4988 again.
         */
        // motor_wake();


        /*
         * Rotate one revolution backwards.
         */
        motor_rotate(STEPS_PER_REV, 0);


        /*
         * Sleep again.
         */
        // motor_sleep();


        /*
         * Wait approximately 2 seconds.
         */
        __delay_cycles(2 * CPU_FREQ_HZ);
    }
}
