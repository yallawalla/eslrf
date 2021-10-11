#include	"stm32f4xx_hal.h"
#include 	"misc.h"
#include 	"ascii.h"
/*******************************************************************************
* Function Name	: 
* Description		: 
* Output				:
* Return				:
*******************************************************************************/
HAL_StatusTypeDef	FLASH_Program(uint32_t Address, uint32_t Data) {
	HAL_StatusTypeDef status;
	__HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_OPERR  | FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR );
	if(*(uint32_t *)Address !=  Data) {
		HAL_FLASH_Unlock();
		do
			status=HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,Address,Data);
		while(status == HAL_BUSY);
		HAL_FLASH_Lock();
	}	
	return status;
}
/*******************************************************************************
* Function Name	: 
* Description		: 
* Output				:
* Return				:
*******************************************************************************/
HAL_StatusTypeDef	FLASH_Erase(uint32_t sector, uint32_t n) {
FLASH_EraseInitTypeDef EraseInitStruct;
HAL_StatusTypeDef ret;
uint32_t	SectorError;
	HAL_FLASH_Unlock();
  EraseInitStruct.TypeErase = FLASH_TYPEERASE_SECTORS;
  EraseInitStruct.VoltageRange = FLASH_VOLTAGE_RANGE_3;
  EraseInitStruct.Sector = sector;
  EraseInitStruct.NbSectors = n;
  ret=HAL_FLASHEx_Erase(&EraseInitStruct, &SectorError);
  HAL_FLASH_Lock(); 
	return ret;
}
/*******************************************************************************
* Function Name	: 
* Description		: see https://community.st.com/s/question/0D50X00009XkfN8/restore-circular-dma-rx-after-uart-error
* Output				:
* Return				:
*******************************************************************************/
UART_HandleTypeDef	*huart_err=NULL;
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
		huart_err=huart;
}
/*******************************************************************************
* Function Name	: 
* Description		: 
* Output				:
* Return				:
*******************************************************************************/
static void	comPoll(_io *io) {
UART_HandleTypeDef *huart=io->huart;
	if(huart == huart_err) {
		HAL_UART_Receive_DMA(huart,(uint8_t*)io->rx->_buf,io->rx->size);
		io->rx->_push = io->rx->_pull = (char *)&huart->pRxBuffPtr[huart->RxXferSize - huart->hdmarx->Instance->NDTR];
		huart_err=NULL;
	} else
		io->rx->_push = (char *)&huart->pRxBuffPtr[huart->RxXferSize - huart->hdmarx->Instance->NDTR];	
	if(huart->gState == HAL_UART_STATE_READY) {
		int len;
		if(!huart->pTxBuffPtr)
			huart->pTxBuffPtr=malloc(io->tx->size);
		do {
			len=_buffer_pull(io->tx, huart->pTxBuffPtr, io->tx->size);
			if(len)
				HAL_UART_Transmit_DMA(huart, huart->pTxBuffPtr, len);
		} while(len > 0);
	}
}
/*******************************************************************************
* Function Name	: 
* Description		: 
* Output				:
* Return				:
*******************************************************************************/
_io* newCom(UART_HandleTypeDef *huart, int sizeRx, int sizeTx) {
	_io* io=_io_init(sizeRx,sizeTx);
	if(io && huart) {
		io->huart=huart;
		HAL_UART_Receive_DMA(huart,(uint8_t*)io->rx->_buf,io->rx->size);
		_proc_add(comPoll,io,"com",0);
	}
	return io;
}
