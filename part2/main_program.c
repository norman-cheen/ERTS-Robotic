

/*
 * FreeRTOS Kernel V10.1.1
 * Copyright (C) 2018 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */

// Author:      Mohd A. Zainol
// Date:        1 Oct 2018
// Chip:        MSP432P401R LaunchPad Development Kit (MSP-EXP432P401R) for TI-RSLK
// File:        main_program.c
// Function:    The main function of our code in FreeRTOS

/* Standard includes. */
#include <stdio.h>

/* Kernel includes. */
#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

/* TI includes. */
#include "gpio.h"

/* ARM Cortex */
#include <stdint.h>
#include "msp.h"
#include "SysTick.h"
#include "inc/CortexM.h"

#include "inc/songFile.h" // TODO: include the songFile header file
#include "inc/dcMotor.h"// TODO: include the dcMotor header file
#include "inc/bumpSwitch.h"// TODO: include the bumpSwitch header file
#include "inc/outputLED.h"// TODO: include the outputLED header file
#include "inc/SysTick.h"// TODO: include the SysTick header file

#define SW1IN ((*((volatile uint8_t *)(0x42098004)))^1) // TODO: bit-banded addresses positive logic of input switch S1
#define SW2IN ((*((volatile uint8_t *)(0x42098010)))^1) // TODO: bit-banded addresses positive logic of input switch S2
#define REDLED (*((volatile uint8_t *)(0x42098040)))

uint8_t bumpSwitch_status;   // TODO: declare a global variable to read bump switches value,
                            //       name this as bumpSwitch_status and use uint8_t

// Semaphore Declaration
SemaphoreHandle_t xSemaphore;

// static void Switch_Init
static void Switch_Init(void);

// a static void function for a task called "taskMasterThread"
static void taskMasterThread( void *pvParameters );
static void taskBumpSwitch( void *pvParameters ); // TODO: declare a static void function for a task called "taskBumpSwitch"
static void taskPlaySong( void *pvParameters );   // TODO: declare a static void function for a task called "taskPlaySong"
static void taskdcMotor( void *pvParameters );  // TODO: declare a static void function for a task called "taskdcMotor"
static void taskReadInputSwitch( void *pvParameters );// TODO: declare a static void function for a task called "taskReadInputSwitch"
static void taskDisplayOutputLED( void *pvParameters );// TODO: declare a static void function for a task called "taskDisplayOutputLED"


void main_program( void ); //Called by main() to create the main program application


static void prvConfigureClocks( void ); //The configuration of clocks for frequency.



xTaskHandle taskHandle_BlinkRedLED;  // declare an identifier of task handler called "taskHandle_BlinkRedLED"
xTaskHandle taskHandle_BumpSwitch;   // TODO: declare an identifier of task handler called "taskHandle_BumpSwitch"
xTaskHandle taskHandle_PlaySong;     // TODO: declare an identifier of task handler called "taskHandle_PlaySong"
xTaskHandle taskHandle_dcMotor;     // TODO: declare an identifier of task handler called "taskHandle_dcMotor"
xTaskHandle taskHandle_InputSwitch; // TODO: declare an identifier of task handler called "taskHandle_InputSwitch"
xTaskHandle taskHandle_OutputLED;  // TODO: declare an identifier of task handler called "taskHandle_OutputLED"

xTaskHandle taskHandle_dcMotor_interrupts; 
xTaskHandle taskHandle_OutputLED_interrupt; 


void main_program( void )
{
    prvConfigureClocks(); // initialise the clock configuration
    Switch_Init();        // TODO: initialise the switch
    SysTick_Init();       // TODO: initialise systick timer
    RedLED_Init();   // initialise the red LED
    int i;

    do{

        if (!(SW2IN | SW1IN)){
            for (i=0; i<1000000; i++);
            REDLED = 1;     // The red LED is blinking waiting for command
            continue;

        }if(SW1IN){    //Polling
               xTaskCreate(taskMasterThread, "taskT", 128, NULL, 2, &taskHandle_BlinkRedLED);
               xTaskCreate(taskBumpSwitch, "taskB", 128, NULL, 1, &taskHandle_BumpSwitch);
               xTaskCreate(taskPlaySong, "taskS", 128, NULL, 1, &taskHandle_PlaySong);
               xTaskCreate(taskdcMotor, "taskM", 128, NULL, 1, &taskHandle_dcMotor);
               xTaskCreate(taskReadInputSwitch, "taskR", 128, NULL, 1, &taskHandle_InputSwitch);
               xTaskCreate(taskDisplayOutputLED, "taskD", 128, NULL, 1, &taskHandle_OutputLED);
            break;
        }

        if(SW2IN){    //Interrupt
             xSemaphore =  xSemaphoreCreateBinary(); 
             
             EnableInterrupts();

             xTaskCreate(taskMasterThread, "taskT", 128, NULL, 2, &taskHandle_BlinkRedLED);
             xTaskCreate(taskPlaySong, "taskS", 128, NULL, 1, &taskHandle_PlaySong);
             xTaskCreate(taskReadInputSwitch, "taskR", 128, NULL, 1, &taskHandle_InputSwitch);

             xTaskCreate(taskBumpSwitch, "taskB", 128, NULL, 1, &taskHandle_BumpSwitch);

             xTaskCreate(taskdcMotor, "taskM", 128, NULL, 1, &taskHandle_dcMotor);

             xTaskCreate(taskDisplayOutputLED, "taskD", 128, NULL, 1, &taskHandle_OutputLED);
            break;
        }
    }while(1);



     vTaskStartScheduler(); // TODO: start the scheduler

    /* INFO: If everything is fine, the scheduler will now be running,
    and the following line will never be reached.  If the following line
    does execute, then there was insufficient FreeRTOS heap memory
    available for the idle and/or timer tasks to be created. See the
    memory management section on the FreeRTOS web site for more details. */
    for( ;; );
}

void PORT4_IRQHandler(void){
    bumpSwitch_status = P4->IV;

    P4->IFG &= ~0xED; // clear flag
}
/*-----------------------------------------------------------------*/
/*------------------- FreeRTOS configuration ----------------------*/
/*-------------   DO NOT MODIFY ANYTHING BELOW HERE   -------------*/
/*-----------------------------------------------------------------*/
// The configuration clock to be used for the board
static void prvConfigureClocks( void )
{
    // Set Flash wait state for high clock frequency
    FlashCtl_setWaitState( FLASH_BANK0, 2 );
    FlashCtl_setWaitState( FLASH_BANK1, 2 );

    // This clock configuration uses maximum frequency.
    // Maximum frequency also needs more voltage.

    // From the datasheet: For AM_LDO_VCORE1 and AM_DCDC_VCORE1 modes,
    // the maximum CPU operating frequency is 48 MHz
    // and maximum input clock frequency for peripherals is 24 MHz.
    PCM_setCoreVoltageLevel( PCM_VCORE1 );
    CS_setDCOCenteredFrequency( CS_DCO_FREQUENCY_48 );
    CS_initClockSignal( CS_HSMCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1 );
    CS_initClockSignal( CS_SMCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1 );
    CS_initClockSignal( CS_MCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1 );
    CS_initClockSignal( CS_ACLK, CS_REFOCLK_SELECT, CS_CLOCK_DIVIDER_1 );
}

// The sleep processing for MSP432 board
void vPreSleepProcessing( uint32_t ulExpectedIdleTime ){}

#if( configCREATE_SIMPLE_TICKLESS_DEMO == 1 )
    void vApplicationTickHook( void )
    {
        /* This function will be called by each tick interrupt if
        configUSE_TICK_HOOK is set to 1 in FreeRTOSConfig.h.  User code can be
        added here, but the tick hook is called from an interrupt context, so
        code must not attempt to block, and only the interrupt safe FreeRTOS API
        functions can be used (those that end in FromISR()). */
        /* Only the full demo uses the tick hook so there is no code is
        executed here. */
    }
#endif
/*-----------------------------------------------------------------*/
/*-------------   DO NOT MODIFY ANYTHING ABOVE HERE   -------------*/
/*--------------------------- END ---------------------------------*/
/*-----------------------------------------------------------------*/
/// --------------------------------------Shared Tasks -------------------------------------------
static void Switch_Init(void){
    // negative logic built-in Button 1 connected to P1.1
    // negative logic built-in Button 2 connected to P1.4
    P1->SEL0 &= ~0x12;
    P1->SEL1 &= ~0x12;      // configure P1.4 and P1.1 as GPIO
    P1->DIR &= ~0x12;       // make P1.4 and P1.1 in
    P1->REN |= 0x12;        // enable pull resistors on P1.4 and P1.1
    P1->OUT |= 0x12;        // P1.4 and P1.1 are pull-up
}
// a static void function for taskReadInputSwitch
static void taskReadInputSwitch( void *pvParameters ){ //STOP MUSIC

    char i_SW1=0;
    int i;

    for( ;; )
    {
        if (SW1IN == 1) {
            i_SW1 ^= 1;                 // toggle the variable i_SW1 (0,1)
            for (i=0; i<1000000; i++);  // this waiting loop is used
                                        // to prevent the switch bounce.
        }



        if (i_SW1 == 1) {
            REDLED = 1;     // turn on the red LED
            vTaskSuspend(taskHandle_PlaySong); // TODO: suspend the task taskHandle_PlaySong
        }
        else {
            REDLED = 0;     // turn off the red LED
            vTaskResume (taskHandle_PlaySong); // TODO: resume the task taskHandle_PlaySong
        }

    }
}

// TODO: create a static void function for taskPlaySong
static void taskPlaySong( void *pvParameters ){

    for( ;; )
    {
        init_song_pwm();   // TODO: initialise the song
        play_song();      // TODO: play the song's function and run forever
    };

};


// --------------------------------------End Shared Tasks -------------------------------------------

/// --------------------------------------Poling -----------------------------------------------------
// TODO: create a static void function for taskBumpSwitch
static void taskBumpSwitch( void *pvParameters ){

    BumpSwitch_Init(); // TODO: initialise bump switches


      // TODO: Read the input of bump switches forever:
        //       Continuously read the 6 bump switches in a loop,
        //       and return it to the "bumpSwitch_status" variable.
        //       Note that the bumpSwitch_status is a global variable,
        //       so do not declare it again here locally.
        // 4.7,6,5,3,2,0


    for( ;; )
    {
        bumpSwitch_status = Bump_Read_Input();    // TODO: use bumpSwitch_status as the variable and
                                                  //       use Bump_Read_Input to read the input

    };


};


// TODO: create a static void function for taskDisplayOutputLED
static void taskDisplayOutputLED( void *pvParameters ){
    for( ;; )
    {
        outputLED_response(bumpSwitch_status);     // TODO: use outputLED_response as the function and
                                                   //       use bumpSwitch_status as the parameter
    }

};



// a static void function for taskMasterThread
static void taskMasterThread( void *pvParameters )
{
    int i;

    ColorLED_Init();  // TODO: initialise the color LED
    RedLED_Init();   // initialise the red LED

    while(!SW2IN){                  // Wait for SW2 switch
        for (i=0; i<1000000; i++);  // Wait here waiting for command
        REDLED = !REDLED;           // The red LED is blinking
    }

    REDLED = 0;     // TODO: Turn off the RED LED, we no longer need that.

    //////////////////////////////////////////////////////////////////
    // TIP: to suspend a task, use vTaskSuspend in FreeRTOS
    // URL: https://www.freertos.org/a00130.html
    //////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////
    // TIP: to delete a task, use vTaskDelete in FreeRTOS
    // URL: https://www.freertos.org/a00126.html
    //////////////////////////////////////////////////////////////////

    vTaskSuspend(taskHandle_BlinkRedLED);     // TODO: This function (taskMasterThread)is no longer needed.
                                        //       Please suspend this task itself, or maybe just delete it.
                                        //       Question: what are the difference between 'suspend' the task,
                                        //                 or 'delete' the task?
};

// TODO: create a static void function for taskdcMotor
static void taskdcMotor( void *pvParameters ){

    for(;;){
       dcMotor_Init(); // TODO: initialise the DC Motor
       dcMotor_Forward(500,1);
       if (bumpSwitch_status == 0x6D || bumpSwitch_status == 0xAD || bumpSwitch_status == 0xCD || bumpSwitch_status == 0xE5 || bumpSwitch_status == 0xE9 || bumpSwitch_status == 0xEC){ // TODO: use a polling that continuously read from the bumpSwitch_status,
           dcMotor_response(bumpSwitch_status );                                                                   //       and run this forever in a while loop.
                                                                                                                   //       use dcMotor_response and bumpSwitch_status for the parameter
       }
   }
};
/// --------------------------------------End Poling -----------------------------------------------------


/// --------------------------------------Interrupt -----------------------------------------------------

/// --------------------------------------End Interrupt -----------------------------------------------------
