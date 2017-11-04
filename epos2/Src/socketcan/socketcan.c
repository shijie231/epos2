#include<string.h>
#include "socketcan.h"
#include "printd.h"

#include "can.h"


extern CAN_HandleTypeDef hcan1;


static int s_fd = -1;
int socketcan_open(uint16_t filter[], uint16_t filtermask[], uint16_t num_filters) {
	++s_fd;
	if(s_fd  > 1) {
		printd(LOG_ERROR, "socketcan: Error opening socket\n");
		return -1;
	}

	// Set Filter for this conection
	int i = 0;
	for(i=0; i < num_filters/2; i++) {
		
		CAN_FilterConfTypeDef sFilterConfig;
		sFilterConfig.FilterIdLow  =filter[i*2];
		sFilterConfig.FilterIdHigh = filter[i*2 + 1];
		sFilterConfig.FilterMaskIdLow  = filtermask[i*2];
		sFilterConfig.FilterMaskIdHigh = filtermask[i*2+1];

		sFilterConfig.FilterFIFOAssignment = s_fd;		
		sFilterConfig.FilterNumber  =i;
		sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
		sFilterConfig.FilterScale  = CAN_FILTERSCALE_16BIT;
		sFilterConfig.FilterActivation = ENABLE;
		sFilterConfig.BankNumber = 14;
		
		HAL_CAN_ConfigFilter(&hcan1,&sFilterConfig);
	}

	if(num_filters/2){
		CAN_FilterConfTypeDef sFilterConfig;
		sFilterConfig.FilterIdLow  =filter[i*2];
		sFilterConfig.FilterIdHigh = 0;
		sFilterConfig.FilterMaskIdLow  = filtermask[i*2];
		sFilterConfig.FilterMaskIdHigh = 0;
		
		sFilterConfig.FilterFIFOAssignment = s_fd;
		sFilterConfig.FilterNumber  =i;
		sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
		sFilterConfig.FilterScale  = CAN_FILTERSCALE_16BIT;
		sFilterConfig.FilterActivation = ENABLE;
		sFilterConfig.BankNumber = 14;
		
		HAL_CAN_ConfigFilter(&hcan1,&sFilterConfig);
	}


	return s_fd;
}


void socketcan_close(int fd) {
	
}


int socketcan_read(int fd, my_can_frame* frame, int timeout) {
	// Wait for data or timeout

	CanRxMsgTypeDef rxMsg;
	if (fd == 0){
		hcan1.pRxMsg = &rxMsg;

	}else{
		hcan1.pRx1Msg = &rxMsg;
	}

	HAL_StatusTypeDef ret;

	ret = HAL_CAN_Receive(&hcan1, fd, timeout);

	if(ret != HAL_OK) {
		// Error, no bytes read
		frame->id = 0;
		frame->dlc = 0;
		if (ret == HAL_TIMEOUT) {
			return SOCKETCAN_TIMEOUT;
		}
		//printd(LOG_ERROR, "socketcan: Could not read data from CAN-bus\n");
		return SOCKETCAN_ERROR;
			
	}

	// Copy data
	frame->id = rxMsg.StdId;
	frame->dlc = rxMsg.DLC;
	memcpy(frame->data, rxMsg.Data, sizeof(frame->data));
	return 0;
	
}


int socketcan_write(int fd, uint16_t id, uint8_t length, Socketcan_t data[]) {
	
	uint8_t byte, n;
	CanTxMsgTypeDef txMsg;
	txMsg.StdId = id;
	txMsg.IDE = CAN_ID_STD;

	byte = 0;
	for(int i=0; i<length; i++) {
		n = 0;
		while(n < data[i].size) {
			txMsg.Data[byte] = (data[i].data >> 8*n);
			n++;
			byte++;
		}
	}

	txMsg.DLC = byte;
	hcan1.pTxMsg = &txMsg;

	HAL_StatusTypeDef ret;
	ret = HAL_CAN_Transmit_IT(&hcan1);

	if(ret != HAL_OK) {
		// Error, no data written
		printd(LOG_ERROR, "socketcan: Could not write data to CAN-bus\n");
		return SOCKETCAN_ERROR;
	}
	return 0;
}
