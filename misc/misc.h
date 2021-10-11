#ifndef		_MISC_H
#define		_MISC_H

#ifdef __cplusplus
 extern "C" {
#endif

#include 	"stm32f4xx_hal.h"
#include 	"io.h"
#include 	"proc.h"
	
HAL_StatusTypeDef	FLASH_Program(uint32_t, uint32_t);

_io* 	  newCom(UART_HandleTypeDef *, int, int);

#ifdef __cplusplus
}
#endif

#endif
