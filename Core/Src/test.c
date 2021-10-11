#include "stm32f4xx_hal.h"
#include <stdlib.h>
#include <stdio.h>
#include "io.h"
#include "misc.h"
#include "ascii.h"

#define		nBits	9
#define		nStop	1
#define		baudrate	19200

typedef enum { ON, NA, BIT, FIRE } opmode;
typedef enum { NORMAL, VERSION, COMM, COUNT } submode;
typedef enum { SECOND, FIRST } echo;

extern		TIM_HandleTypeDef htim2,htim3;

_buffer		*icbuf1,*icbuf2;				// dma buffer ic1,ic2,tim2
uint16_t	*pwmbuf;								// dma buffer tim3,pwm
uint32_t 	idle,nBaud;

static void __rearmDMA(uint32_t len)	{	
	do {
			__HAL_DMA_DISABLE(htim3.hdma[TIM_DMA_ID_UPDATE]);
			__HAL_TIM_DISABLE(&htim3);
			__HAL_TIM_SET_COUNTER(&htim3,0);
			while(htim3.hdma[TIM_DMA_ID_UPDATE]->Instance->CR & DMA_SxCR_EN);
			htim3.hdma[TIM_DMA_ID_UPDATE]->Instance->NDTR=len;
			__HAL_DMA_CLEAR_FLAG(htim3.hdma[TIM_DMA_ID_UPDATE],
						DMA_FLAG_HTIF2_6 | DMA_FLAG_TEIF2_6 | DMA_FLAG_DMEIF2_6	| DMA_FLAG_FEIF2_6 | DMA_FLAG_TCIF2_6);
			__HAL_DMA_ENABLE(htim3.hdma[TIM_DMA_ID_UPDATE]);
			__HAL_TIM_ENABLE(&htim3);
	} while(0);
}

__packed struct {
		uint8_t addr:3, spare1:5;
		uint8_t opmode:2, echo:1, spare2:3, submode:2;
		uint8_t factmode:4, minrange:4;
		uint8_t	chk;
} callW = {1,0,ON,1,0,NORMAL,8,0,0};

union  {
__packed struct	{
	uint8_t addr:3, spare1:1, commfail:1, id:1, update:1, spare2:1;  
	uint8_t r100m:4, r1000m:4;
	uint8_t r10m:4, r10000m:1, r1m:1, submode:2;
	uint8_t	opmode:2, echo:1, spare3:5;
	uint8_t	multi:1, minrange:1, laser_fail:1, general_fail:1, spare4:4;
	uint8_t	chk;	
} a482;
__packed struct	{
	uint8_t addr:3, spare1:1, commfail:1, id:1, update:1, spare2:1;  
	uint8_t SwRev:5,SwUpdt:2,spare3:1;
	uint8_t r10m:4, r10000m:1, r1m:1, submode:2;
	uint8_t	opmode:2, echo:1, spare4:5;
	uint8_t	multi:1, minrange:1, laser_fail:1, general_fail:1, spare5:4;
	uint8_t	chk;	
} a483;	
__packed struct	{
	uint8_t addr:3, count:1, CountLow:4;
	uint8_t CountMed;
	uint8_t CountHi;
	uint8_t	opmode:2, serialLo:6;
	uint8_t	serialHi;
	uint8_t	chk;	
} a485;
__packed struct	{
	uint8_t addr:3, spare1:1, commfail:1, id:1, update:1, spare2:1;  
	uint8_t SwChk:1,RAM:1,EPROM:1,FIFO:1,Sim:1,To:1,Qsw:1,HV:1;
	uint8_t BITinproc:1,spare3:6;
	uint8_t	opmode:2, echo:1, spare4:5;
	uint8_t	multi:1, minrange:1, laser_fail:1, general_fail:1, spare5:4;
	uint8_t	chk;	
} a486;
uint8_t		byte[6];
} ackW = {0}, rxW = {0};

/*******************************************************************************
* Function Name	: 
* Description		: 
* Output				:
* Return				:
*******************************************************************************/
static	uint16_t	*sendBytes(uint8_t *s, uint16_t *d, uint16_t len) {
	uint16_t dat= 0x200;
	while(len--) {
		dat |= (*s++ | (1 << nBits)) << 1;
		for (uint32_t i=1; i < (1 << (nBits + 2)); i<<=1) {
			if(dat & i) d[0] = nBaud+1; else d[0] = 0;
			if(dat & i) d[2] = 0; else d[2] = nBaud+1;
			++d; ++d; ++d;
		}
		dat=0;
	}
	return d;
}
/*******************************************************************************
* Function Name	: 
* Description		: 
* Output				:
* Return				:
*******************************************************************************/
static	void			sendCallW(void) {
	callW.chk=0;
	for(int n=0; n<sizeof(callW)-1; ++n)
		callW.chk += ((uint8_t *)&callW)[n];
	__rearmDMA(sendBytes((uint8_t *)&callW, pwmbuf, sizeof(callW))-pwmbuf);
}
/*******************************************************************************
* Function Name	: 
* Description		: 
* Output				:
* Return				:
*******************************************************************************/
static	void sendAcklW(void) {
	ackW.a482.chk=0;
	ackW.a485.count=1;
	for(int n=0; n<sizeof(ackW)-1; ++n)
		ackW.a482.chk += ackW.byte[n];
	__rearmDMA(sendBytes((uint8_t *)&ackW, pwmbuf, sizeof(ackW))-pwmbuf);
}
/*******************************************************************************
* Function Name	: 
* Description		: 
* Output				:
* Return				:
*******************************************************************************/
static	int32_t		getIc(uint32_t ic) {
	static uint32_t to, bit, dat, cnt;
	int32_t	ret=EOF;
	if (to) {
		uint32_t n = ((ic - to) + nBaud/2) / nBaud;
		while (n-- && cnt <= nBits + nStop) {
			dat = (dat | bit) >> 1;
			++cnt;
		}
		idle=HAL_GetTick()+5;		
		bit ^= 1 << (nBits + nStop);
		if (cnt > nBits + nStop) {
			ret=dat & ((1<<nBits)-1);
			dat = cnt = 0;
		}
	} else {
			bit = dat = cnt = 0;
	}
	to = ic;
	return ret;
}
/*******************************************************************************
* Function Name	: 
* Description		: 
* Output				:
* Return				:
*******************************************************************************/
static void		parse(int32_t cword) {
static	uint32_t	idx=0, len=4;
	if(cword & 0x100) {
		idx=rxW.a482.chk=0;
	}
	
	if(idx == len-1) {
		if(cword ==  rxW.a482.chk) {
			if(len==4)
				sendAcklW();
		}
		else
			_print("err ");
		return;
	}
	
	rxW.byte[idx++] = cword & 0xff;
	rxW.a482.chk += cword & 0xff;
	
	if(rxW.a482.id || rxW.a485.count)
		len=6;
	else
		len=4;
	
}
/*******************************************************************************
* Function Name	: 
* Description		: 
* Output				:
* Return				:
*******************************************************************************/
void		app(void) {

	if(!pwmbuf)	{
		nBaud=SystemCoreClock/baudrate-1;
		_print("ESLRF test\r\n");

		pwmbuf=malloc(1024*sizeof(uint16_t));
		memcpy(pwmbuf,"\0xffff",sizeof(uint16_t));	//dummy edge, first CCR1 !!!
		htim3.Instance->ARR=nBaud;
		
		HAL_TIM_PWM_Start(&htim3,TIM_CHANNEL_1);
		HAL_TIM_PWM_Start(&htim3,TIM_CHANNEL_3);
		HAL_TIM_DMABurst_WriteStart(&htim3,TIM_DMABASE_CCR1,TIM_DMA_UPDATE,(uint32_t *)pwmbuf,TIM_DMABURSTLENGTH_3TRANSFERS);
		
		icbuf1=_buffer_init(1024);
		HAL_TIM_IC_Start_DMA(&htim2,TIM_CHANNEL_1, (uint32_t *)icbuf1->_buf, icbuf1->size/sizeof(uint32_t));
		
		icbuf2=_buffer_init(1024);
		HAL_TIM_IC_Start_DMA(&htim2,TIM_CHANNEL_2,  (uint32_t *)icbuf2->_buf, icbuf2->size/sizeof(uint32_t));
	}
	
	switch (getchar()) {
		case __F1:
			callW.opmode=ON;
			callW.submode=NORMAL;
			sendCallW();
			break;
		case __F2:
			callW.opmode=ON;
			callW.submode=VERSION;
			sendCallW();
			break;
		case __F3:
			callW.opmode=ON;
			callW.submode=COMM;
			sendCallW();
			break;
		case __F4:
			callW.opmode=ON;
			callW.submode=COUNT;
			sendCallW();
			break;
		case __F5:
			callW.opmode=BIT;
			callW.submode=NORMAL;
			sendCallW();
			break;
		case __F12:
			callW.opmode=FIRE;
			callW.submode=NORMAL;
			sendCallW();
			break;
		case __CtrlY:
			HAL_NVIC_SystemReset();
		case EOF:
		break;
		default:
		break;
	}
	
	if(idle && HAL_GetTick() > idle) {
		idle=0;
		_buffer_put(icbuf1,&idle,sizeof(uint32_t));
		_buffer_put(icbuf2,&idle,sizeof(uint32_t));
	}

	icbuf1->_push = &icbuf1->_buf[(icbuf1->size - htim2.hdma[TIM_DMA_ID_CC1]->Instance->NDTR*sizeof(uint32_t))];
	icbuf2->_push = &icbuf2->_buf[(icbuf2->size - htim2.hdma[TIM_DMA_ID_CC2]->Instance->NDTR*sizeof(uint32_t))];

	if(_buffer_count(icbuf1) && _buffer_count(icbuf2)) {
		uint32_t	t1,t2;
		_buffer_pull(icbuf1,&t1,sizeof(uint32_t));
		_buffer_pull(icbuf2,&t2,sizeof(uint32_t));
		if(t1-t2 < 72) {
			int32_t ch=getIc(t1);
			if(ch != EOF)
				parse(ch);
			if(ch != EOF) {
				if(ch & 0x100)
					_print("\r\n%03X ",ch);
				else
					_print("%03X ",ch);
			}
		}
	}
} 

