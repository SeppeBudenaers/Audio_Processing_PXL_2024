/******************************************************************************
* Copyright (C) 2023 Advanced Micro Devices, Inc. All Rights Reserved.
* SPDX-License-Identifier: MIT
******************************************************************************/
/*
 * helloworld.c: simple test application
 *
 * This application configures UART 16550 to baud rate 9600.
 * PS7 UART (Zynq) is not initialized by this application, since
 * bootrom/bsp configures it to baud rate 115200
 *
 * ------------------------------------------------
 * | UART TYPE   BAUD RATE                        |
 * ------------------------------------------------
 *   uartns550   9600
 *   uartlite    Configurable only in HW design
 *   ps7_uart    115200 (configured by bootrom/bsp)
 */

#include "audio.h"
#include "xscutimer.h"
#include "xscugic.h"
#include "xgpiops.h"
#include <math.h>
#include <stdint.h>
#define TIMER_DEVICE_ID		XPAR_XSCUTIMER_0_DEVICE_ID
#define INTC_DEVICE_ID		XPAR_SCUGIC_SINGLE_DEVICE_ID
#define TIMER_IRPT_INTR		XPAR_SCUTIMER_INTR
#define TIMER_LOAD_VALUE	0xFFFF

#define AMPLITUDE 200000
#define SAMPLE_RATE 48000
#define M_PI 3.1415

#define UINT32_MAX_AS_FLOAT 4294967295.0f //(2^32 - 1
#define UINT_SCALED_MAX_VALUE 0xFFFFF // 2^24 =>24 bits audio codec maximum value is 0xFF FFFF

volatile unsigned long u32DataL, u32DataR;
volatile unsigned long u32Temp;
volatile int Timer_Intr_rcvd;
char DTMF = '0';

static void Timer_ISR(void * CallBackRef) { /* ***** I2S Interruption Handler **** */
	int frequency_L = 0;
	int frequency_R = 0;
	switch (DTMF){
		case '1':
			frequency_L = 697;
			frequency_R = 1209;
			break;
		case '2':
			frequency_L = 697;
			frequency_R = 1336;
			break;
		case '3':
			frequency_L = 697;
			frequency_R = 1477;
			break;
		case 'a':
			frequency_L = 697;
			frequency_R = 1633;
			break;
		case '4':
			frequency_L = 770;
			frequency_R = 1209;
			break;
		case '5':
			frequency_L = 770;
			frequency_R = 1336;
			break;
		case '6':
			frequency_L = 770;
			frequency_R = 1477;
			break;
		case 'b':
			frequency_L = 770;
			frequency_R = 1633;
			break;
		case '7':
			frequency_L = 852;
			frequency_R = 1209;
			break;
		case '8':
			frequency_L = 852;
			frequency_R = 1336;
			break;
		case '9':
			frequency_L = 852;
			frequency_R = 1477;
			break;
		case 'c':
			frequency_L = 852;
			frequency_R = 1633;
			break;
		case '*':
			frequency_L = 941;
			frequency_R = 1209;
			break;
		case '0':
			frequency_L = 941;
			frequency_R = 1336;
			break;
		case '#':
			frequency_L = 941;
			frequency_R = 1477;
			break;
		case 'd':
			frequency_L = 941;
			frequency_R = 1633;
			break;
		default:
			frequency_L = 0;
			frequency_R = 0;
	}

	float theta_increment_L = 2* M_PI* frequency_L / SAMPLE_RATE ;
	float theta_increment_R = 2* M_PI* frequency_R / SAMPLE_RATE ;
	static float theta_L = 0.0f;
	static float theta_R = 0.0f;

	XScuTimer *TimerInstancePtr = (XScuTimer *) CallBackRef;
	XScuTimer_ClearInterruptStatus(TimerInstancePtr);

	theta_L += theta_increment_L ;
	theta_R += theta_increment_R ;
	if ( theta_L > 2* M_PI){
		theta_L -= 2* M_PI;
	}

	if ( theta_R > 2* M_PI){
		theta_R -= 2* M_PI;
	}

	float sine_value_L = sinf(theta_L);
	float sine_value_R = sinf(theta_R);
	uint32_t scaled_value_L = (uint32_t)(((sine_value_L + 1.0f) * 0.5f) * (UINT_SCALED_MAX_VALUE/2));
	uint32_t scaled_value_R = (uint32_t)(((sine_value_R + 1.0f) * 0.5f) * (UINT_SCALED_MAX_VALUE/2));

	Xil_Out32(I2S_DATA_TX_L_REG, scaled_value_L);
	Xil_Out32(I2S_DATA_TX_R_REG, scaled_value_R);
}

static int Timer_Intr_Setup(XScuGic * IntcInstancePtr, XScuTimer *TimerInstancePtr, u16 TimerIntrId)
{
	int Status;
	XScuGic_Config *IntcConfig;
	IntcConfig = XScuGic_LookupConfig(INTC_DEVICE_ID);
	Status = XScuGic_CfgInitialize(IntcInstancePtr, IntcConfig, IntcConfig->CpuBaseAddress);
	// Step 1: Interrupt Setup
	Xil_ExceptionInit();
	// Step 2: Interrupt Setup
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_IRQ_INT, (Xil_ExceptionHandler)XScuGic_InterruptHandler,IntcInstancePtr);
	// Step 3: Interrupt Setup
	Status = XScuGic_Connect(IntcInstancePtr, TimerIntrId, (Xil_ExceptionHandler)Timer_ISR, (void *)TimerInstancePtr);
	// Step 4: Interrupt Setup
	XScuGic_Enable(IntcInstancePtr, TimerIntrId);
	// Step 5:
	XScuTimer_EnableInterrupt(TimerInstancePtr);
	// Step 6: Interrupt Setup
	Xil_ExceptionEnable();
	return XST_SUCCESS;
}

int main ( void )
{
	int Status;
    init_platform();
	//Configure the IIC data structure
	IicConfig(XPAR_XIICPS_0_DEVICE_ID);

	//Configure the Audio Codec's PLL
	AudioPllConfig();

	//Configure the Line in and Line out ports.
	//Call LineInLineOutConfig() for a configuration that
	//enables the HP jack too.
	AudioConfigureJacks();
	LineinLineoutConfig();

	XScuTimer Scu_Timer;
	XScuTimer_Config *Scu_ConfigPtr;
	XScuGic IntcInstance;

	Scu_ConfigPtr = XScuTimer_LookupConfig(XPAR_PS7_SCUTIMER_0_DEVICE_ID);
	Status = XScuTimer_CfgInitialize(&Scu_Timer, Scu_ConfigPtr, Scu_ConfigPtr->BaseAddr);
	Status = Timer_Intr_Setup(&IntcInstance, &Scu_Timer, XPS_SCU_TMR_INT_ID);
	XScuTimer_LoadTimer(&Scu_Timer,(XPAR_PS7_CORTEXA9_0_CPU_CLK_FREQ_HZ / 2)/SAMPLE_RATE);
	XScuTimer_EnableAutoReload(&Scu_Timer);

//	ConfigPtr = XGpioPs_LookupConfig(XPAR_PS7_GPIO_0_DEVICE_ID);
//	XGpioPs_CfgInitialize(&GpioPs, ConfigPtr, ConfigPtr->BaseAddr);
//
//	XGpioPs_SetDirection(&GpioPs, GPIOPS_BANK, 0x80);
//	XGpioPs_SetOutputEnable(&GpioPs, GPIOPS_BANK, 0x80);

	XScuTimer_Start(&Scu_Timer);
	printf("Welcome enter numbers for DTMF tones\n\r");
  for(;;){
	  scanf("%c",DTMF);
  }
  cleanup_platform();
  return 0;
}