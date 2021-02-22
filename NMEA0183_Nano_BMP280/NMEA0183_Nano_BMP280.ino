
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_BMP280.h>

Adafruit_BMP280 bmp; // use I2C interface
Adafruit_Sensor *bmp_temp = bmp.getTemperatureSensor();
Adafruit_Sensor *bmp_pressure = bmp.getPressureSensor();

bool sendXDR = true;
bool sendMDA = false;

void setup() {
  Serial.begin(9600);
  /* Serial.println(F("BMP280 Sensor event test")); */

  if (!bmp.begin()) {  
    if (!bmp.begin(0x76)) { 
      Serial.println(F("Could not find a valid BMP280 sensor, check wiring!"));
      while (1) delay(10);
    }
  }

  /* Default settings from datasheet. */
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                  Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                  Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                  Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                  Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */

  /* bmp_temp->printSensorDetails(); */
}

const byte buff_len = 90;
char CRCbuffer[buff_len];

byte nmea_crc(String msg) {
  // NMEA CRC: XOR each byte with previous for all chars between '$' and '*'
  char c;
  int i;
  byte crc = 0;
  for (i = 0; i < buff_len; i++){
    crc ^= msg.charAt(i);  // XOR
  }
  return crc;
  //String hexcrc = String(crc,HEX);
  //return hexcrc.toUpperCase();

}

float bar2inches(float bar) {
  return bar*29.53;
}


void loop() {
  sensors_event_t temp_event, pressure_event;
  bmp_temp->getEvent(&temp_event);
  bmp_pressure->getEvent(&pressure_event);
  String NMEA;

  if (sendXDR) {
    NMEA = "IIXDR,P,"+String(pressure_event.pressure)+",H,Barometer,C,"+String(temp_event.temperature)+",C,TempAir";
    String CRC = String(nmea_crc(NMEA),HEX);
    CRC.toUpperCase();
    Serial.println("$"+NMEA+"*"+CRC);
  }

  if (sendMDA) {
    float inches = bar2inches(pressure_event.pressure/1000.000);
    NMEA = "IIMDA,"+String(inches,3)+",I,"+String(pressure_event.pressure/1000.000,4)+",B,"+String(temp_event.temperature,2)+",C,,C,,,,C,,T,,M,,N,,M";
    String CRC = String(nmea_crc(NMEA),HEX);
    CRC.toUpperCase();
    Serial.println("$"+NMEA+"*"+CRC);  
  }
  
  delay(2000);
}
