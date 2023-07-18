#ifndef HARDWARE_H
#define HARDWARE_H

#ifdef BOARD_M5ATOM
#define ESP32_CAN_TX_PIN GPIO_NUM_22  // Set CAN TX port to 22
#define ESP32_CAN_RX_PIN GPIO_NUM_19  // Set CAN RX port to 19
#endif

#ifdef BOARD_M5ATOM_CANUNIT
#define ESP32_CAN_TX_PIN GPIO_NUM_26
#define ESP32_CAN_RX_PIN GPIO_NUM_32
#endif

#endif
