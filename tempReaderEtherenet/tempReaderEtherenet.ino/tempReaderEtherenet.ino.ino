#include <EthernetUdp.h>
#include <Ethernet.h>
#include <EthernetUdp.h>

//etherent
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xAA, 0xFE, 0xAA };
IPAddress deviceIp(192, 168, 1, 177);
IPAddress destinationIP(192, 168, 1, 99);  // the remote IP address
EthernetUDP Udp;
unsigned int destinationPort = 8888;

String inputString = "";      // a String to hold incoming data
bool stringComplete = false;  // whether the string is complete

EthernetClient net;

void setup() {
  Serial.begin(9600);
  Serial1.begin(9600);
  // start the Ethernet
  Ethernet.begin(mac, deviceIp);
  // start UDP
  Udp.begin(8888);
    
  SendUDPPacket("system started.");
  
  // reserve 200 bytes for the inputString:
  inputString.reserve(200);
}



void loop() {
  if (stringComplete) {
    Serial.println(inputString);
    SendUDPPacket(inputString);
    // clear the string:
    inputString = "";
    stringComplete = false;
  }  
}


/*
  SerialEvent occurs whenever a new data comes in the hardware serial RX. This
  routine is run between each time loop() runs, so using delay inside loop can
  delay response. Multiple bytes of data may be available.
*/
void serialEvent() {
  while (Serial1.available()) {
    // get the new byte:
    char inChar = (char)Serial1.read();
    // add it to the inputString:
    inputString += inChar;
    // if the incoming character is a newline, set a flag so the main loop can
    // do something about it:
    if (inChar == '\n') {
      stringComplete = true;
    }
  }
}


void SendUDPPacket(String message) {
  int len = message.length() + 1;
  char repBuff[len];
  message.toCharArray(repBuff, len);

  Udp.beginPacket(destinationIP, 8888);
  Udp.write(repBuff);
  Udp.endPacket();
  Serial.println(message);
}

