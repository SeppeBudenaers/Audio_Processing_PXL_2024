#include <stdio.h>
#include "platform.h"
#include "xil_printf.h"
#include "arm_math.h"
#include "arm_const_structs.h"
#include "audio.h"
#include "xscutimer.h"
#include "xscugic.h"
#include "xgpiops.h"
#include <stdint.h>

#define TIMER_DEVICE_ID		XPAR_XSCUTIMER_0_DEVICE_ID
#define INTC_DEVICE_ID		XPAR_SCUGIC_SINGLE_DEVICE_ID
#define TIMER_IRPT_INTR		XPAR_SCUTIMER_INTR
#define TIMER_LOAD_VALUE	0xFFFF

volatile int Timer_Intr_rcvd;



#define SAMPLE_RATE 48000
#define TEST_LENGTH_SAMPLES 2048

/* -------------------------------------------------------------------
* External Input and Output buffer Declarations for FFT Bin Example
* ------------------------------------------------------------------- */
extern float32_t testInput_f32_10khz[TEST_LENGTH_SAMPLES];

/* ------------------------------------------------------------------
* Global variables for FFT Bin Example
* ------------------------------------------------------------------- */
uint32_t fftSize = 1024;
uint32_t ifftFlag = 0;
uint32_t doBitReverse = 1;
arm_cfft_instance_f32 varInstCfftF32;

/* Reference index at which max energy of bin ocuurs */
uint32_t refIndex = 213, testIndex = 0;


uint8_t Buffer_Flag = 0;
uint8_t FFT_Ready_Flag = 0;
float32_t Buffer1[TEST_LENGTH_SAMPLES],Buffer2[TEST_LENGTH_SAMPLES];

float32_t * FFT_Buffer_Ptr;
float32_t FFT_Output[TEST_LENGTH_SAMPLES/2];

static void Timer_ISR(void * CallBackRef) { /* ***** I2S Interruption Handler **** */
	static int  counter;
	switch (Buffer_Flag) {
		case 0:
			Buffer1[counter] = Xil_In32(I2S_DATA_RX_L_REG);
//			Buffer1[counter] = testInput_f32_10khz[counter];
			break;
		case 1:
			Buffer2[counter] = Xil_In32(I2S_DATA_RX_L_REG);
//			Buffer2[counter] = testInput_f32_10khz[counter];
			break;
		default:
			break;
	}

	counter++;
	if(counter >= TEST_LENGTH_SAMPLES){
		switch (Buffer_Flag) {
			case 0:
				FFT_Buffer_Ptr = Buffer1;
				Buffer_Flag = 1;
				break;
			case 1:
				FFT_Buffer_Ptr = Buffer2;
				Buffer_Flag = 0;
				break;
			default:
				break;
		}
		FFT_Ready_Flag = 1;
		counter = 0;

	}

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


int main()
{


	init_platform();

    IicConfig(XPAR_XIICPS_0_DEVICE_ID);

    AudioPllConfig();
    AudioConfigureJacks();
    LineinLineoutConfig();

    XScuTimer Scu_Timer;
    XScuTimer_Config *Scu_ConfigPtr;
    XScuGic IntcInstance;

    Scu_ConfigPtr = XScuTimer_LookupConfig(XPAR_PS7_SCUTIMER_0_DEVICE_ID);
    XScuTimer_CfgInitialize(&Scu_Timer, Scu_ConfigPtr, Scu_ConfigPtr->BaseAddr);
    Timer_Intr_Setup(&IntcInstance, &Scu_Timer, XPS_SCU_TMR_INT_ID);
    XScuTimer_LoadTimer(&Scu_Timer,(XPAR_PS7_CORTEXA9_0_CPU_CLK_FREQ_HZ / 2)/SAMPLE_RATE);
    XScuTimer_EnableAutoReload(&Scu_Timer);

	arm_status status;
	float32_t maxValue;

	XScuTimer_Start(&Scu_Timer);
	while(1){
		if(FFT_Ready_Flag == 1){
			status = ARM_MATH_SUCCESS;

			status=arm_cfft_init_1024_f32(&varInstCfftF32);

			/* Process the data through the CFFT/CIFFT module */
			arm_cfft_f32(&varInstCfftF32, FFT_Buffer_Ptr, ifftFlag, doBitReverse);

			/* Process the data through the Complex Magnitude Module for calculating the magnitude at each bin */
			arm_cmplx_mag_f32(FFT_Buffer_Ptr, FFT_Output, fftSize);

			/* Calculates maxValue and returns corresponding BIN value */
			arm_max_f32(FFT_Output, fftSize, &maxValue, &testIndex);

			xil_printf("Index : %d\n\r",testIndex);

			FFT_Ready_Flag = 0;
		}
	}
    cleanup_platform();
    return 0;
}
