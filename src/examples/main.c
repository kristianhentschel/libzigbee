#include "stm32f4_discovery.h"
#include <stdio.h>
#include "zb_packets.h"
#include "zb_transport.h"

/* You can monitor the converted value by adding the variable "ADC3ConvertedValue"
 * to the debugger watch window
 * (taken from (c) STMicro example file) */
#define ADC3_DR_ADDRESS     ((uint32_t)0x4001224C)
__IO uint16_t ADCConvertedValue = 0;

static void ADC_Config(void);
static void USART_Config(void);
static void respond();
static char hexToChar(char h);

int main(void)
{
	unsigned char c;
	
	/* set up ADC3 for continuous DMA mode */
	ADC_Config();

	/* Start ADC3 Software Conversions */ 
	ADC_SoftwareStartConv(ADC3);

	/* Setup packet layer / transport layer UART */
	zb_packets_init();

	zb_set_broadcast_mode(0);
	zb_set_device_id(2);

	zb_send_packet(OP_PONG, "", 0);
	
	while (1)
	{
		c = zb_getc();
		
		switch (zb_parse(c)) {
			case ZB_VALID_PACKET:
				respond();
				break;
			case ZB_START_PACKET:
				1 + 1;
				break;
			default:
				break;
		}	   
	}
}


static void respond() {
	unsigned char buf[4];
	uint16_t val;

	switch (zb_packet_op) {
		case OP_PING:
			zb_send_packet(OP_PONG, NULL, 0);
			break;
		case OP_MEASURE_REQUEST:
			val = ADCConvertedValue;
			buf[3] = hexToChar(val & 0x0f);
			buf[2] = hexToChar(val >> 4  & 0x0f);
			buf[1] = hexToChar(val >> 8  & 0x0f);
			buf[0] = hexToChar(val >> 12 & 0x0f);
			zb_send_packet(OP_MEASURE_RESPONSE, buf, 4);
			break;
		default:
			break;
	}
}


/**
  * @brief  ADC3 channel1 with DMA configuration
  * @param  None
  * @retval None
  *
  * taken from (c) STMicroelectronics example file.
  */
void ADC_Config(void)
{
  ADC_InitTypeDef       ADC_InitStructure;
  ADC_CommonInitTypeDef ADC_CommonInitStructure;
  DMA_InitTypeDef       DMA_InitStructure;
  GPIO_InitTypeDef      GPIO_InitStructure;

  /* Enable ADC3, DMA2 and GPIO clocks ****************************************/
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2 | RCC_AHB1Periph_GPIOA, ENABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC3, ENABLE);

  /* DMA2 Stream0 channel0 configuration **************************************/
  DMA_InitStructure.DMA_Channel = DMA_Channel_2;
  DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)ADC3_DR_ADDRESS;
  DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)&ADCConvertedValue;
  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
  DMA_InitStructure.DMA_BufferSize = 1;
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Disable;
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
  DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
  DMA_InitStructure.DMA_Priority = DMA_Priority_High;
  DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;         
  DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_HalfFull;
  DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
  DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
  DMA_Init(DMA2_Stream0, &DMA_InitStructure);
  DMA_Cmd(DMA2_Stream0, ENABLE);

  /* Configure ADC3 Channel1 pin as analog input ******************************/
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL ;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  /* ADC Common Init **********************************************************/
  ADC_CommonInitStructure.ADC_Mode = ADC_Mode_Independent;
  ADC_CommonInitStructure.ADC_Prescaler = ADC_Prescaler_Div2;
  ADC_CommonInitStructure.ADC_DMAAccessMode = ADC_DMAAccessMode_Disabled;
  ADC_CommonInitStructure.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_5Cycles;
  ADC_CommonInit(&ADC_CommonInitStructure);

  /* ADC3 Init ****************************************************************/
  ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b;
  ADC_InitStructure.ADC_ScanConvMode = DISABLE;
  ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;
  ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;
  ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
  ADC_InitStructure.ADC_NbrOfConversion = 1;
  ADC_Init(ADC3, &ADC_InitStructure);

  /* ADC3 regular channel1 configuration *************************************/
  ADC_RegularChannelConfig(ADC3, ADC_Channel_1, 1, ADC_SampleTime_3Cycles);

 /* Enable DMA request after last transfer (Single-ADC mode) */
  ADC_DMARequestAfterLastTransferCmd(ADC3, ENABLE);

  /* Enable ADC3 DMA */
  ADC_DMACmd(ADC3, ENABLE);

  /* Enable ADC3 */
  ADC_Cmd(ADC3, ENABLE);
}

static char hexToChar(char h) {
	static const chars[17] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
	if(h >= 0 && h <= 15) {
		return chars[h];
	} else {
		return 'X';
	}
}
