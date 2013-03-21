#include "zb_transport.h"
#include "stm32f4_discovery.h"

/*
 * zb_transport_embedded.h
 *
 * Implementation of the serial receive buffer and send methods using interrupts
 * and the STM32F4's USART peripherals.
 *
 * Currently hard-coded for USART3 peripheral - should be more flexible through #defines.
 *
 * implements an interrupt-safe queue for holding received bytes.
 *
 * Author: Kristian Hentschel
 * Team Project 3. University of Glasgow. 2013.
 */

#define QUEUE_SIZE 72

typedef struct queue {
	int head;
	int tail;
	int count;
	char elements[QUEUE_SIZE];
} *Queue;

static int q_is_empty(Queue q);
static void q_put(Queue q, char c);
static char q_take(Queue q);

struct queue RX;

/* synchronously sends a single character, by busy-waiting until send buffer is empty. */
static void zb_putc(unsigned char c) {
	while (!(USART3->SR & USART_FLAG_TXE))
		;
	USART_SendData(USART3, c);
}

/* initialise USART Peripheral and set up GPIO pins for its use. Enable interrupts. */
void zb_transport_init() {
	GPIO_InitTypeDef	GPIO_InitStructure;
	USART_InitTypeDef	USART_InitStructure;
	NVIC_InitTypeDef	NVIC_InitStructure;
	/* init data structures */
	RX.head = 0;
	RX.tail = 0;
	RX.count = 0;
	
	/* init uart */
	
	/* 1, 2 Enable USART3 and corresponding GPIO clocks */
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);

	/* 3 Set alternate functions for GPIO pins PC10, PC11 to be used for RX, TX */
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource10, GPIO_AF_USART3);
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource11, GPIO_AF_USART3);

	GPIO_StructInit(&GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF;

	GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	/* 4 baud rate, word length, stop bit, parity, flow control */
	USART_StructInit(&USART_InitStructure);
	USART_InitStructure.USART_BaudRate				= 9600;
	USART_InitStructure.USART_WordLength			= USART_WordLength_8b;
	USART_InitStructure.USART_StopBits				= USART_StopBits_1;
	USART_InitStructure.USART_Parity				= USART_Parity_No;
	USART_InitStructure.USART_Mode	 				= USART_Mode_Rx | USART_Mode_Tx;
	USART_InitStructure.USART_HardwareFlowControl	= USART_HardwareFlowControl_None;

	USART_Init(USART3, &USART_InitStructure);

	/* 5 enable nvic and these interrupts if interrupts required */
	//USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);

	/* 6 enable DMA if using this? */

	/* 7 enable USART */
	USART_Cmd(USART3, ENABLE);
	
	
	/* init NVIC */
	//NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0); /* do not need preemption of interrupts as we are only using one. */
	//NVIC_Init(&NVIC_InitStructure);
	
}

/* disable interrupts for the usart peripheral (?) */
void zb_transport_stop() {
	/* this space intentionally left empty:
	 * closing the connection does not really apply for embedded system. */
}

/* blocking write, sending character by character */
void zb_send(unsigned char *buf, unsigned char len) {
	int i;
	for (i = 0; i < len; i++) {
		zb_putc(buf[i]);
	}
}

/* disable interrupts, take character from buffer, re-enable interrupts */
char zb_getc() {
	/* TODO temporary hack - uses polling from usart peripheral rather than interrupts. */
		while(! (USART3->SR & USART_FLAG_RXNE) )
		;
	return USART_ReceiveData(USART3) & 0xff;
	
	/* wait until item has been added to queue */
	while (q_is_empty(&RX))
		;

	/* take item from queue */
	return q_take(&RX);
}

/* delay for one second through SysTick or busy loop. */
void zb_guard_delay() {
	/* TODO */	
}

/* Interrupt handler for USART.
 * 	- Check if interrupt source is RX buffer full
 * 	- read from RX buffer
 * 	- store in software RX buffer
 * 	- clear interrupt flag
 */
void USART3_IRQHandler(void) {
	char c;

	if (USART_GetITStatus(USART3, USART_IT_RXNE) == SET) {
		c = USART_ReceiveData(USART3) & 0xff; /* also clears RXNE interrupt flag */
		q_put(&RX, c);
	} else {
		/* TODO may have to clear other interrupt flags, see peripheral driver source comments. */
	}
	
}

/* take an item from an interrupt-safe queue.
 * this is only called from non-interrupt code. */
char q_take(Queue q) {
	char c;

	while (q_is_empty(q))
		; /* do nothing */

	/* TODO disable interrupts */

	c = q->elements[q->head];
	q->count -= 1;
	q->head = (q->head + 1) % QUEUE_SIZE;

	/* TODO enable interrupts */

	return c;
}

/* put an item into the queue.
 * this is only called from the irq handler
 * and discards items if the queue is full. tough. */
void q_put(Queue q, char c) {
	if (q->count == QUEUE_SIZE) {
		return;
	}
	q->elements[q->tail] = c;
	q->count++;
	q->tail = (q->tail + 1) % QUEUE_SIZE;
}

/* return 1 if queue is empty, 0 otherwise. */
int q_is_empty(Queue q) {
	return (q->count == 0);
}
