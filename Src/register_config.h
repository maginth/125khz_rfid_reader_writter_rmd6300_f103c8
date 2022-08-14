/*
 * register_config.h
 *
 *  Created on: 8 juil. 2019
 *      Author: Mathieu
 */
#include "main.h"
#include "usbd_cdc.h"
#include "usbd_cdc_if.h"

#ifndef REGISTER_CONFIG_H_
#define REGISTER_CONFIG_H_


#define USB_BUFFER_SIZE 1024
void usb_send_buffer();
uint32_t usb_msg(uint8_t *msg, uint32_t size);

#define DEBUG_MESSAGES

#ifdef DEBUG_MESSAGES
#define USB_D(m) usb_msg((uint8_t *)m, sizeof(m))
#else
#define USB_D(m) do {} while(0)
#endif

extern USBD_HandleTypeDef hUsbDeviceFS;

void init_IO_interfaces();

#endif /* REGISTER_CONFIG_H_ */
