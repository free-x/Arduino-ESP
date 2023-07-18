#ifndef HARDWARE_H
#define HARDWARE_H

#ifdef BOARD_M5ATOM
#define ESP32_CAN_TX_PIN GPIO_NUM_22  // Set CAN TX port to 26 
#define ESP32_CAN_RX_PIN GPIO_NUM_19  // Set CAN RX port to 32
#define N2K_LOAD_LEVEL 3
#endif

#ifdef BOARD_M5ATOM_CANUNIT
#define ESP32_CAN_TX_PIN GPIO_NUM_26
#define ESP32_CAN_RX_PIN GPIO_NUM_32
#endif

#ifdef BMP280
#define BMP_SDA 25
#define BMP_SCL 21
#endif

#ifdef SENSOR_BPS
#define BMP_SDA 26
#define BMP_SCL 32
#endif

#define NMEA0183_Out_Stream_Speed 115200
#define NMEA0183_Out_Stream Serial

#endif
