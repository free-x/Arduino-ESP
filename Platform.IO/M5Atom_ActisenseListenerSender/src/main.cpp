#include "hardware.h"

#include <N2kMsg.h>
#include <NMEA2000.h>
#include <NMEA2000_CAN.h>
#include <ActisenseReader.h>

tActisenseReader ActisenseReader;

// Define READ_STREAM to port, where you write data from PC e.g. with NMEA Simulator.
#define READ_STREAM Serial       
// Define ForwardStream to port, what you listen on PC side. On Arduino Due you can use e.g. SerialUSB
#define FORWARD_STREAM Serial    

Stream *ReadStream=&READ_STREAM;
Stream *ForwardStream=&FORWARD_STREAM;

void HandleStreamN2kMsg(const tN2kMsg &N2kMsg) {
  // N2kMsg.Print(&Serial);
  NMEA2000.SendMsg(N2kMsg,-1);
}

void setup() {
  // Define buffers big enough
  NMEA2000.SetN2kCANSendFrameBufSize(150);
  NMEA2000.SetN2kCANReceiveFrameBufSize(150);
  
  if (ReadStream!=ForwardStream) READ_STREAM.begin(115200);
  FORWARD_STREAM.begin(115200);
  NMEA2000.SetForwardStream(ForwardStream); 
  NMEA2000.SetMode(tNMEA2000::N2km_ListenAndSend);
  // NMEA2000.SetForwardType(tNMEA2000::fwdt_Text); // Show bus data in clear text
  if (ReadStream==ForwardStream) NMEA2000.SetForwardOwnMessages(false); // If streams are same, do not echo own messages.
  // NMEA2000.EnableForward(false);
  NMEA2000.Open();

  // I originally had problem to use same Serial stream for reading and sending.
  // It worked for a while, but then stopped. Later it started to work.
  ActisenseReader.SetReadStream(ReadStream);
  ActisenseReader.SetDefaultSource(75);
  ActisenseReader.SetMsgHandler(HandleStreamN2kMsg); 
}

void loop() {
  NMEA2000.ParseMessages();
  ActisenseReader.ParseMessages();
}
