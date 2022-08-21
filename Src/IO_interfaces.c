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

uint8_t uart_rx_buff[4]; //4 bytes for security, only first is used.
extern UART_HandleTypeDef huart1;

void init_IO_interfaces() {
	HAL_UART_Receive_IT(&huart1, uart_rx_buff, 1); //next byte
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
#define RDM6300_PACKET_DATA_SIZE		(12)
#define RDM6300_PACKET_CHECKSUM_SIZE	(2)
#define RDM6300_PACKET_BEGIN			(0x02)
#define RDM6300_PACKET_END				(0x03)
#define RDM6300_DEFAULT_TAG_TIMEOUT_MS	(300)
#define RDM6300_READ_TIMEOUT			(20)

#define PACKET_RESET_POS ((uint32_t)-1)

uint8_t packet[RDM6300_PACKET_DATA_SIZE];
uint32_t packet_pos = -1;
uint32_t packet_rx_start_time = -1;
uint32_t char_rx_time = -1;

uint8_t _hex(uint8_t c) {
	return "0123456789ABCDEF"[c];
}

uint32_t hex(uint8_t c) {
	return ((uint32_t)_hex(c & 15) << 8) + (uint32_t)_hex(c >> 4);
}

uint32_t hexRepr(uint8_t c) {
	return (hex(c) << 8) + 'x';
}

uint8_t read_hex(uint8_t c) {
	if ('0' <= c && c <= '9')
		return c - '0';
	if ('A' <= c && c <= 'Z')
		return c + 10 - 'A';
	USB_D("error non hex char: ");
	usb_msg(&c, 1);
	USB_D("\r\n");
	return 0;
}

uint32_t packet_checksum_ok() {
	uint32_t s = RDM6300_PACKET_CHECKSUM_SIZE;
	uint8_t sum[s];
	memset(sum, 0, s);
	for (uint32_t i=0; i<RDM6300_PACKET_DATA_SIZE-s; i+=s)
		for (uint32_t j=0; j < s; j++)
			sum[j] ^= read_hex(packet[i+j]);

	for (uint32_t j=0; j < s; j++)
		sum[j] = _hex(sum[j]);

	uint32_t ok = !memcmp(sum, packet + RDM6300_PACKET_DATA_SIZE - s, s);
	USB_D("\r\nChecksum ");
	if (ok) USB_D("OK: "); else USB_D("ERROR: ");
	usb_msg(sum, s);
	USB_D("\r\n");
	return ok;
}

void RDM6300_RX_packet_complet() {
	if (packet_checksum_ok())
		USB_D("Packet received successfully\r\n");
	//usb_msg(packet, RDM6300_PACKET_SIZE);
	//USB_D("\r\n");
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	uint8_t c = *uart_rx_buff;
	HAL_UART_Receive_IT(huart, uart_rx_buff, 1); //next byte

//	usb_msg(&c, 1);
//	return;

	uint32_t t  = HAL_GetTick();
	uint32_t packet_timeout = t > packet_rx_start_time + RDM6300_DEFAULT_TAG_TIMEOUT_MS;
	uint32_t char_timeout = t > char_rx_time + RDM6300_READ_TIMEOUT;
	if (packet_timeout || char_timeout) {
		if (packet_pos != PACKET_RESET_POS) {
			if (packet_timeout)
				USB_D("\r\nPacket timeout");
			else
				USB_D("\r\nChar timeout, packet may be incomplete");
			USB_D(", packet reception aborted");
		}
		USB_D("\r\n");
		packet_pos = PACKET_RESET_POS;
		packet_rx_start_time = -1;
	}
	char_rx_time = t;

	if (packet_pos == PACKET_RESET_POS) {
	  if (c == RDM6300_PACKET_BEGIN) {
		  packet_rx_start_time = t;
		  packet_pos = 0;
		  USB_D("Packet begins:\r\n");
	  } else {
		  usb_msg(&c, 1);
	  }
	  return;
	}
	if (packet_pos >= RDM6300_PACKET_DATA_SIZE) {
	  if (c == RDM6300_PACKET_END) {
		  RDM6300_RX_packet_complet();
	  } else {
		  USB_D("corrupted packet\r\n");
	  }
	  packet_pos = PACKET_RESET_POS;
	  return;
	}
	packet[packet_pos] = c;
	packet_pos++;
	if (0x20 <= c && c < 0x7f)
		usb_msg(&c, 1);
	else {
		uint32_t h = hexRepr(c);
		usb_msg((uint8_t*)&h, 3);
	}
}



void uart_test_transmission() {
	uint8_t msg[] = "I am talking to myself\r\n>transmission: ";
	switch (HAL_UART_Transmit_IT(&huart1, msg, sizeof(msg))) {
		case HAL_OK: USB_D("OK"); break;
		case HAL_BUSY: USB_D("BUSY"); break;
		default: USB_D("ERROR"); break;
	}
	USB_D("\r\n");
}

//void _log(uint8_t *m) {
//	HAL_UART_Transmit_DMA(&huart2,m,strlen((char *)m));
//}



