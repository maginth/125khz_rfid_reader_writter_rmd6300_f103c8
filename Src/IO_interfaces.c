/*
 * IO_interfaces.c
 *
 *  Created on: Jul 13, 2019
 *      Author: Mathieu
 */
#include "main.h"
#include "register_config.h"
#include <string.h>
#include "stm32f1xx_hal.h"


void init_IO_interfaces() {

}

/***************************************************
 ******  USB through USART & virtual COM port ******
 ***************************************************/

uint8_t usb_buffer[USB_BUFFER_SIZE];
uint32_t usb_buffer_pos = 0;

uint32_t usb_msg(uint8_t *msg, uint32_t size) {
	if (usb_buffer_pos + size < USB_BUFFER_SIZE) {
		memcpy(usb_buffer + usb_buffer_pos, msg, size);
		usb_buffer_pos += size;
		return SUCCESS;
	}
	return ERROR;
}

void usb_send_buffer(uint8_t *msg, uint32_t size) {
	  static uint32_t prev_usb_buffer_pos = 0;

	  if (prev_usb_buffer_pos != usb_buffer_pos || usb_buffer_pos == USB_BUFFER_SIZE)
		  if (CDC_Transmit_FS(usb_buffer, usb_buffer_pos) == USBD_OK)
			  usb_buffer_pos = 0;
}

extern UART_HandleTypeDef huart1;

#define RDM6300_BAUDRATE				(9600)
#define RDM6300_PACKET_SIZE				(14)
#define RDM6300_PACKET_BEGIN			(0x02)
#define RDM6300_PACKET_END				(0x03)
#define RDM6300_DEFAULT_TAG_TIMEOUT_MS	(300)

#define PACKET_RESET_POS ((uint32_t)-1)

uint8_t packet[RDM6300_PACKET_SIZE];
uint32_t packet_pos = -1;
uint32_t packet_rx_start_time = 0;

void RDM6300_RX_packet_complet() {
	USB_D("Packet received:/n");
	usb_msg(packet, RDM6300_PACKET_SIZE);
	USB_D("/n");
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	USB_D("received\n");

	uint8_t *msg = huart->pRxBuffPtr;
	uint16_t len = huart->RxXferSize;

	uint32_t t  = HAL_GetTick();
	if (t > packet_rx_start_time + RDM6300_DEFAULT_TAG_TIMEOUT_MS) {
		if (packet_pos != PACKET_RESET_POS)
			USB_D("timeout packet reception aborted\n");
		packet_pos = PACKET_RESET_POS;
	}

	for (uint32_t i=0; i < len; i++) {
	  if (packet_pos == PACKET_RESET_POS) {
		  if (msg[i] == RDM6300_PACKET_BEGIN) {
			  packet_rx_start_time = t;
			  packet_pos = 0;
		  }
		  continue;
	  }
	  if (packet_pos >= RDM6300_PACKET_SIZE) {
		  if (msg[i] == RDM6300_PACKET_END) {
			  RDM6300_RX_packet_complet();
		  } else {
			  USB_D("corrupted packet\n");
		  }
		  packet_pos = PACKET_RESET_POS;
		  continue;
	  }
	  packet[packet_pos++] = msg[i];
	}
}


//void _log(uint8_t *m) {
//	HAL_UART_Transmit_DMA(&huart2,m,strlen((char *)m));
//}



