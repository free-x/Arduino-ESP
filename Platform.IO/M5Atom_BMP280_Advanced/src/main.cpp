#include "hardware.h"

#include <Preferences.h>
#include <NMEA2000_CAN.h>  // This will automatically choose right CAN library and create suitable NMEA2000 object
#include <N2kMessages.h>
#include <NMEA0183.h>
#include <NMEA0183Messages.h>

#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>

// Set time offsets
#define SlowDataUpdatePeriod 1000  // Time between CAN Messages sent
#define TempSendOffset 0           // + 0 ms
#define PressureSendOffset 100     // + 100 ms


Adafruit_BMP280 bmp; // use I2C interface

tNMEA0183 NMEA0183_Out;

int NodeAddress;            // To store last Node Address
Preferences preferences;    // Nonvolatile storage on ESP32 - To store LastDeviceAddress

// Set the information for other bus devices, which messages we support
const unsigned long TransmitMessages[] PROGMEM = {130311L, // Enviroinment
                                                  130312L, // Temperature
                                                  130314L, // Pressure
                                                  0
                                                 };

void setup() {
  uint8_t chipid[6];
  uint32_t id = 0;
  int i = 0;
  
  Serial.begin(115200);
  delay(10);

  Wire.begin(BMP_SDA,BMP_SCL);
  if (!bmp.begin()) {
    if (!bmp.begin(0x76)) {
      Serial.println(F("Could not find a valid BMP280 sensor, check wiring!"));
      while (1) delay(10);
    }
  }
// Reserve enough buffer for sending all messages.
  NMEA2000.SetN2kCANMsgBufSize(8);
  NMEA2000.SetN2kCANReceiveFrameBufSize(150);
  NMEA2000.SetN2kCANSendFrameBufSize(150);

  // Generate unique number from chip id
  // esp_efuse_read_mac(chipid);
  esp_efuse_mac_get_default(chipid);
  for (i = 0; i < 6; i++) id += (chipid[i] << (7 * i)); 

 // Set product information
  NMEA2000.SetProductInformation("1", // Manufacturer's Model serial code
                                 100, // Manufacturer's product code
                                 "M5Atom BMP280 Sensor Module",  // Manufacturer's Model ID
                                 "1.0.2.25 (2019-07-07)",  // Manufacturer's Software version code
                                 "1.0.2.0 (2019-07-07)" // Manufacturer's Model version
                                );
  // Set device information
  NMEA2000.SetDeviceInformation(id, // Unique number. Use e.g. Serial number.
                                132, // Device function=Analog to NMEA 2000 Gateway. See codes on http://www.nmea.org/Assets/20120726%20nmea%202000%20class%20&%20function%20codes%20v%202.00.pdf
                                25, // Device class=Inter/Intranetwork Device. See codes on  http://www.nmea.org/Assets/20120726%20nmea%202000%20class%20&%20function%20codes%20v%202.00.pdf
                                2046 // Just choosen free from code list on http://www.nmea.org/Assets/20121020%20nmea%202000%20registration%20list.pdf
                               );

  preferences.begin("nvs", false);                          // Open nonvolatile storage (nvs)
  NodeAddress = preferences.getInt("LastNodeAddress", 34);  // Read stored last NodeAddress, default 34
  preferences.end();
  Serial.printf("NodeAddress=%d\n", NodeAddress);

  // If you also want to see all traffic on the bus use N2km_ListenAndNode instead of N2km_NodeOnly below
  NMEA2000.SetMode(tNMEA2000::N2km_NodeOnly, NodeAddress);
  NMEA2000.ExtendTransmitMessages(TransmitMessages);

  NMEA2000.Open();

  NMEA0183_Out_Stream.begin(NMEA0183_Out_Stream_Speed);
  // Setup NMEA0183 ports and handlers
  NMEA0183_Out.SetMessageStream(&NMEA0183_Out_Stream);
  NMEA0183_Out.Open();

#ifdef RAYMARINE
  Serial.println(F("Raymarine"));
#endif
  
  delay(200);

}

// Functions to control send interval and time offets

//*****************************************************************************
bool IsTimeToUpdate(unsigned long NextUpdate) {
  return (NextUpdate < millis());
}


//*****************************************************************************
unsigned long InitNextUpdate(unsigned long Period, unsigned long Offset = 0) {
  return millis() + Period + Offset;
}


//*****************************************************************************
void SetNextUpdate(unsigned long & NextUpdate, unsigned long Period) {
  while ( NextUpdate < millis() ) NextUpdate += Period;
}

//*****************************************************************************
void SendN2kTemperature(void) {
  static unsigned long SlowDataUpdated = InitNextUpdate(SlowDataUpdatePeriod, TempSendOffset);
  tN2kMsg N2kMsg;
  tNMEA0183Msg NMEA0183Msg;
  double Temperature;

  if ( IsTimeToUpdate(SlowDataUpdated) ) {
    SetNextUpdate(SlowDataUpdated, SlowDataUpdatePeriod);

    Temperature = bmp.readTemperature();    
    // Serial.printf("Temperature: %3.1f °C \n", Temperature);

    // Definition from N2kMessages.h
    // void SetN2kPGN130312(tN2kMsg &N2kMsg, unsigned char SID, unsigned char TempInstance, tN2kTempSource TempSource,
    //        double ActualTemperature, double SetTemperature=N2kDoubleNA);

    // tN2kTempSource is defined in N2kTypes.h

    // Set N2K message
    SetN2kPGN130312(N2kMsg, 0, 0, N2kts_MainCabinTemperature, CToKelvin(Temperature), N2kDoubleNA);
    
    // Send message
    NMEA2000.SendMsg(N2kMsg);
    if (!NMEA0183Msg.Init("XDR", "II"))
        return;
    if (!NMEA0183Msg.AddStrField("C"))
        return;    
    if (!NMEA0183Msg.AddDoubleField(Temperature))
        return;
    if (!NMEA0183Msg.AddStrField("C"))
        return;
    if (!NMEA0183Msg.AddStrField("TempAir"))
        return;

    NMEA0183_Out.SendMessage(NMEA0183Msg);
  }
}

void Send2kEnv(void) {
static unsigned long SlowDataUpdated = InitNextUpdate(SlowDataUpdatePeriod, PressureSendOffset);
  tN2kMsg N2kMsg;
  tNMEA0183Msg NMEA0183Msg;
  double Pressure;
  double Temperature;
  double Humidity = N2kDoubleNA;

  if ( IsTimeToUpdate(SlowDataUpdated) ) {
    SetNextUpdate(SlowDataUpdated, SlowDataUpdatePeriod);

    Pressure = bmp.readPressure() /100;  // Read and convert to mBar
    Temperature = bmp.readTemperature();

    // Definition from N2kMessages.h
    // void SetN2kPGN130311(tN2kMsg &N2kMsg, unsigned char SID, tN2kTempSource TempSource, double Temperature,
    //        tN2kHumiditySource HumiditySource, double Humidity, double AtmosphericPressure)
    
    // Set N2K message
    SetN2kPGN130311(N2kMsg, 0, N2kts_MainCabinTemperature, CToKelvin(Temperature), N2khs_InsideHumidity, Humidity, mBarToPascal(Pressure));
    // Send message
    NMEA2000.SendMsg(N2kMsg);
    if (!NMEA0183Msg.Init("XDR", "II"))
        return;
    if (!NMEA0183Msg.AddStrField("C"))
        return;
    if (!NMEA0183Msg.AddDoubleField(Temperature))
        return;
    if (!NMEA0183Msg.AddStrField("C"))
        return;
    if (!NMEA0183Msg.AddStrField("TempAir"))
        return;

    NMEA0183_Out.SendMessage(NMEA0183Msg);

    if (!NMEA0183Msg.Init("XDR", "II"))
        return;
    if (!NMEA0183Msg.AddStrField("P"))
        return;
    if (!NMEA0183Msg.AddDoubleField(Pressure/1000,1,"%2.4f"))
        return;
    if (!NMEA0183Msg.AddStrField("B"))
        return;
    if (!NMEA0183Msg.AddStrField("Barometer"))
        return;

    NMEA0183_Out.SendMessage(NMEA0183Msg);
  }
}

//*****************************************************************************
void SendN2kPressure(void) {
  static unsigned long SlowDataUpdated = InitNextUpdate(SlowDataUpdatePeriod, PressureSendOffset);
  tN2kMsg N2kMsg;
  tNMEA0183Msg NMEA0183Msg;
  double Pressure;        

  if ( IsTimeToUpdate(SlowDataUpdated) ) {
    SetNextUpdate(SlowDataUpdated, SlowDataUpdatePeriod);
        
    Pressure = bmp.readPressure() /100;  // Read and convert to mBar 
    //Serial.printf("Pressure: %3.1f mBar \n", Pressure);

    // Definition from N2kMessages.h
    // SetN2kPGN130314(tN2kMsg &N2kMsg, unsigned char SID, unsigned char PressureInstance,
    //                 tN2kPressureSource PressureSource, double Pressure);
    
    // PressureSource is defined in N2kTypes.h

    // Set N2K message
    SetN2kPGN130314(N2kMsg, 0, 0, N2kps_Atmospheric, mBarToPascal(Pressure));
    

    // Send message
    NMEA2000.SendMsg(N2kMsg);
    if (!NMEA0183Msg.Init("XDR", "II"))
        return;
    if (!NMEA0183Msg.AddStrField("P"))
        return;    
    if (!NMEA0183Msg.AddDoubleField(Pressure/1000,1,"%2.4f"))
        return;
    if (!NMEA0183Msg.AddStrField("B"))
        return;
    if (!NMEA0183Msg.AddStrField("Barometer"))
        return;

    NMEA0183_Out.SendMessage(NMEA0183Msg);
  }
  
}

//*****************************************************************************
// Function to check if SourceAddress has changed (due to address conflict on bus)

void CheckSourceAddressChange() {
  int SourceAddress = NMEA2000.GetN2kSource();

  if (SourceAddress != NodeAddress) { // Save potentially changed Source Address to NVS memory
    NodeAddress = SourceAddress;
    preferences.begin("nvs", false);
    preferences.putInt("LastNodeAddress", SourceAddress);
    preferences.end();
    Serial.printf("Address Change: New Address=%d\n", SourceAddress);
  }
}

void loop() {

#ifdef RAYMARINE
  Send2kEnv();
#else
  SendN2kTemperature();
  SendN2kPressure();
#endif

  NMEA2000.ParseMessages();

  CheckSourceAddressChange();
  
  // Dummy to empty input buffer to avoid board to stuck with e.g. NMEA Reader
  if ( Serial.available() ) {
    Serial.read();
  }
}
