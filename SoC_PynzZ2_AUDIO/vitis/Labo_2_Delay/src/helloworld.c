#include "audio.h"
#include "xscutimer.h"
#include "xscugic.h"
#include "xgpiops.h"
#define TIMER_DEVICE_ID		XPAR_XSCUTIMER_0_DEVICE_ID
#define INTC_DEVICE_ID		XPAR_SCUGIC_SINGLE_DEVICE_ID
#define TIMER_IRPT_INTR		XPAR_SCUTIMER_INTR
#define TIMER_LOAD_VALUE	0xFFFF
#define AMPLITUDE 200000
#define SAMPLE_RATE 48000

#define DELAY_BUF_SIZE 24000

volatile unsigned long u32DataL, u32DataR;
volatile unsigned long u32Temp;
volatile int Timer_Intr_rcvd;

int32_t buffer_L [ SAMPLE_RATE];
int32_t buffer_R [ SAMPLE_RATE];

int32_t i = 0;

static void Timer_ISR(void * CallBackRef)
{
	int32_t u32DataL = Xil_In32(I2S_DATA_RX_L_REG);
	int32_t u32DataR = Xil_In32(I2S_DATA_RX_R_REG);

	int32_t delayed_sample_L = 0 ;
	int32_t delayed_sample_R = 0 ;
	int32_t buffer_L[i];
	//int32_t audio_out_chR = 0;



	delayed_sample_L = buffer_L[i];
	//delayed_sample_R = buffer_R[i];

	//audio_out_chR = delayed_sample_R + u32DataR;



	buffer_L[i] = u32DataL;
	buffer_R[i] = u32DataR;
	i = ((i+1) % SAMPLE_RATE);


	Xil_Out32(I2S_DATA_TX_L_REG, delayed_sample_L);
	Xil_Out32(I2S_DATA_TX_R_REG, u32DataL);
}

static int Timer_Intr_Setup(XScuGic * IntcInstancePtr, XScuTimer *TimerInstancePtr, u16 TimerIntrId)
{
	XScuGic_Config *IntcConfig;
	IntcConfig = XScuGic_LookupConfig(INTC_DEVICE_ID);
	XScuGic_CfgInitialize(IntcInstancePtr, IntcConfig, IntcConfig->CpuBaseAddress);
	// Step 1: Interrupt Setup
	Xil_ExceptionInit();
	// Step 2: Interrupt Setup
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_IRQ_INT, (Xil_ExceptionHandler)XScuGic_InterruptHandler,IntcInstancePtr);
	// Step 3: Interrupt Setup
	XScuGic_Connect(IntcInstancePtr, TimerIntrId, (Xil_ExceptionHandler)Timer_ISR, (void *)TimerInstancePtr);
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
  XScuTimer_CfgInitialize(&Scu_Timer, Scu_ConfigPtr, Scu_ConfigPtr->BaseAddr);
  Timer_Intr_Setup(&IntcInstance, &Scu_Timer, XPS_SCU_TMR_INT_ID);
  XScuTimer_LoadTimer(&Scu_Timer,(XPAR_PS7_CORTEXA9_0_CPU_CLK_FREQ_HZ / 2)/48000);
  XScuTimer_EnableAutoReload(&Scu_Timer);

  XScuTimer_Start(&Scu_Timer);

  for(;;){

  }
  cleanup_platform();
  return 0;
}
