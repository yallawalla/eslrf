#ifndef		_MISC_H
#define		_MISC_H

#ifdef __cplusplus
 extern "C" {
#endif

#include 	"stm32f4xx_hal.h"
#include 	"io.h"
#include 	"proc.h"
	
HAL_StatusTypeDef	FLASH_Program(uint32_t, uint32_t);
HAL_StatusTypeDef	FLASH_Erase(uint32_t, uint32_t);
char		*trim(char **);

void		dumpHex(int, int);
void		flushVCP(const void *);
void		flushUART(const void *);
void		printVersion(void);

typedef	enum {
	_HW_INIT=1,
	_HW_V1,
	_HW_V2,
	_HW_V3,
	_HW_V4,
	_HW_V5,
	_HW_V6,
	_HW_V7
} hwVersion;
extern hwVersion hw;

extern	uint32_t	__Vectors[];
	      
extern	UART_HandleTypeDef	huart1;

_io* 	  ioUsart(UART_HandleTypeDef *, int, int);
void		pollVcp(void *);
void		pollLed(void);
void		pollUsart(_io *);

void 		__RED1(int32_t);
void 		__GREEN1(int32_t);
void 		__YELLOW1(int32_t);
void 		__BLUE1(int32_t);
void 		__RED2(int32_t);
void 		__GREEN2(int32_t);
void 		__YELLOW2(int32_t);
void 		__BLUE2(int32_t);

#define __time__		HAL_GetTick()

#define	cwbarOn()		HAL_GPIO_WritePin(cwbOVRD_GPIO_Port, cwbOVRD_Pin, GPIO_PIN_RESET)
#define	cwbarOff()	HAL_GPIO_WritePin(cwbOVRD_GPIO_Port, cwbOVRD_Pin, GPIO_PIN_SET)

#define	floatLow		(FSW0_GPIO_Port->IDR & FSW1_Pin)

#define	fsw2DL()		HAL_GPIO_WritePin(FSW_DL_GPIO_Port, FSW_DL_Pin, GPIO_PIN_RESET); \
										HAL_GPIO_WritePin(FSW_EC_GPIO_Port, FSW_EC_Pin, GPIO_PIN_SET)
#define	fsw2EC()		HAL_GPIO_WritePin(FSW_DL_GPIO_Port, FSW_DL_Pin, GPIO_PIN_SET); \
										HAL_GPIO_WritePin(FSW_EC_GPIO_Port, FSW_EC_Pin, GPIO_PIN_RESET)
#define	fsw2None()	HAL_GPIO_WritePin(FSW_DL_GPIO_Port, FSW_DL_Pin, GPIO_PIN_SET); \
										HAL_GPIO_WritePin(FSW_EC_GPIO_Port, FSW_EC_Pin, GPIO_PIN_SET)

#ifdef __cplusplus
}
#endif

#endif
