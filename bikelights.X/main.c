/*
 * File:   main.c
 * Author: hughe
 *
 * Created on 08 February 2016, 20:35
 */

#define _XTAL_FREQ 500000

#include <xc.h>
#include <pic12f1572.h>

/***************************************************************************
     * Configurations
***************************************************************************/
// CONFIG1
#pragma config FOSC = INTOSC    //  (INTOSC oscillator; I/O function on CLKIN pin)
#pragma config WDTE = OFF       // Watchdog Timer Enable (WDT disabled)
#pragma config PWRTE = OFF      // Power-up Timer Enable (PWRT disabled)
#pragma config MCLRE = OFF      // MCLR Pin Function Select (MCLR/VPP pin function is digital input)
#pragma config CP = OFF         // Flash Program Memory Code Protection (Program memory code protection is disabled)
#pragma config BOREN = OFF      // Brown-out Reset Enable (Brown-out Reset disabled)
#pragma config CLKOUTEN = OFF   // Clock Out Enable (CLKOUT function is disabled. I/O or oscillator function on the CLKOUT pin)

// CONFIG2
#pragma config WRT = OFF        // Flash Memory Self-Write Protection (Write protection off)
#pragma config PLLEN = OFF      // PLL Enable (4x PLL disabled)
#pragma config STVREN = OFF     // Stack Overflow/Underflow Reset Enable (Stack Overflow or Underflow will not cause a Reset)
#pragma config BORV = HI        // Brown-out Reset Voltage Selection (Brown-out Reset Voltage (Vbor), high trip point selected.)
#pragma config LPBOREN = OFF    // Low Power Brown-out Reset enable bit (LPBOR is disabled)
#pragma config LVP = OFF        // Low-Voltage Programming Enable (High-voltage on MCLR/VPP must be used for programming)


#define LEFT 0
#define RIGHT 1

#define DEBOUNCE 100

#define BRAKE_TOTAL 100 // total cycle
#define BRAKE_DUTY 40 // on time
#define BRAKE_SOLID 90 // solid on time

#define FLASH_TOTAL 600 // total cycle
#define FLASH_DUTY 300  // on time

#define BRAKE_BTN PORTAbits.RA3
#define LEFT_BTN PORTAbits.RA0
#define RIGHT_BTN PORTAbits.RA1

#define BRAKE_LIGHT(v) PORTAbits.RA2 = v;
#define LEFT_LIGHT(v) PORTAbits.RA5 = v;
#define RIGHT_LIGHT(v) PORTAbits.RA4 = v;

//
unsigned char left_down = 0;
unsigned char right_down = 0;
bit left_flash = 0;
bit right_flash = 0;

unsigned char brake_counter = 0;
unsigned short flash_counter = 0;

void reset() {
    left_flash = 0;
    right_flash = 0;
    flash_counter = 0;
    brake_counter = 0;
    left_down = 0;
    right_down = 0;
    BRAKE_LIGHT(0);
    LEFT_LIGHT(0);
    RIGHT_LIGHT(0);
}

void sleepy() {
    BRAKE_LIGHT(0);
    LEFT_LIGHT(1);
    RIGHT_LIGHT(1);

    // buttons must be down for 0.5 seconds
    __delay_ms(500);
    if(!LEFT_BTN || !RIGHT_BTN) return;

    // turn lights off to let user know it was successful
    LEFT_LIGHT(0);
    RIGHT_LIGHT(0);

    // avoid bounce
    while(LEFT_BTN || RIGHT_BTN);
    __delay_ms(100);

    // sleep loop
    while(1) {
        reset();
        SLEEP();
        // wake up
        NOP();
        // check that both buttons are down for 1.5 seconds
        if(!LEFT_BTN || !RIGHT_BTN) continue;
        __delay_ms(500);
        if(!LEFT_BTN || !RIGHT_BTN) continue;
        __delay_ms(500);
        if(!LEFT_BTN || !RIGHT_BTN) continue;
        __delay_ms(500);
        if(!LEFT_BTN || !RIGHT_BTN) continue;

        break;
    }

    // turn lights on to let user know it was successful
    LEFT_LIGHT(1);
    RIGHT_LIGHT(1);

    // avoid bounce
    while(LEFT_BTN || RIGHT_BTN);
    __delay_ms(100);

    reset();
}

void turn(const unsigned char way) {
    flash_counter = 0;

    // cancel flash if currently flashing
    if(left_flash || right_flash) {
        left_flash = right_flash = 0;
        return;
    }

    // enable flash
    if(way == LEFT) {
        left_flash = 1;
    }
    if(way == RIGHT) {
        right_flash = 1;
    }
    return;
}

void main(void) {

    /***************************************************************************
     * Configure registers
    ***************************************************************************/
    OSCCON = 0b00111010; // 500khz
    // while(!OSCSTATbits.MFIOFR);
    NOP();
    NOP();

    INTCONbits.GIE = 0;
    INTCONbits.PEIE = 0;
    INTCONbits.IOCIE = 1;
    IOCAP = 0x00000011;

    ANSELA = 0x00;
    TRISA = 0b11001011;
    WPUA = 0x00;
    OPTION_REGbits.nWPUEN = 1;
    ODCONA = 0x00;
    SLRCONA = 0x00;
    INLVLA = 0x00;
    APFCON = 0x00;
    PWM1CON = 0x00;
    PWM2CON = 0x00;
    PWM3CON = 0x00;

    reset();

    while(1) {
        __delay_ms(1);

        // check sleep command
        if(LEFT_BTN && RIGHT_BTN) {
            sleepy();
            continue;
        }

        
        // animate brake light
        if(BRAKE_BTN && brake_counter < BRAKE_SOLID) {
            BRAKE_LIGHT(1);
        }
        else if(brake_counter < BRAKE_DUTY) {
            BRAKE_LIGHT(1);
        } else {
            BRAKE_LIGHT(0);
        }
        brake_counter++;
        if(brake_counter >= BRAKE_TOTAL) {
            brake_counter = 0;
        }


        // detect left button rising edge with debounce
        if(LEFT_BTN && !left_down) {
            left_down = DEBOUNCE;
            turn(LEFT);
        }
        if(!LEFT_BTN && left_down) {
            left_down--;
        }

        
        // detect right button rising edge with debounce
        if(RIGHT_BTN && !right_down) {
            right_down = DEBOUNCE;
            turn(RIGHT);
        }
        if(!RIGHT_BTN && right_down) {
            right_down--;
        }

        
        // animate left light
        if(left_flash && flash_counter < FLASH_DUTY) {
            LEFT_LIGHT(1);
        } else {
            LEFT_LIGHT(0);
        }
            
        // animate right light
        if(right_flash && flash_counter < FLASH_DUTY) {
            RIGHT_LIGHT(1);
        } else {
            RIGHT_LIGHT(0);
        }

        // increment flash counter
        if(left_flash || right_flash) {
            flash_counter++;
            if(flash_counter >= FLASH_TOTAL) {
                flash_counter = 0;
            }
        }
           
    }
}


    
    
