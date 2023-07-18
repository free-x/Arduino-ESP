#ifndef HARDWARE_H
#define HARDWARE_H

#ifdef BOARD_M5ATOM
#define ESP32_CAN_TX_PIN GPIO_NUM_22  // Set CAN TX port to 26 
#define ESP32_CAN_RX_PIN GPIO_NUM_19  // Set CAN RX port to 32
#define BMP_SDA 26
#define BMP_SCL 32

#endif
