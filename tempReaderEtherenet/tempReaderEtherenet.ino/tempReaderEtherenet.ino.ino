#include <EthernetUdp.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <MQTT.h>

String inputString = "";      // a String to hold incoming data
bool stringComplete = false;  // whether the string is complete



//etherent
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xAA, 0xFE, 0xAA };
IPAddress deviceIp(192, 168, 1, 177);
IPAddress destinationIP(192, 168, 1, 99);  // the remote IP address
EthernetUDP Udp;
unsigned int destinationPort = 8888;

const long day = 86400000;  // 86400000 milliseconds in a day
const long hour = 3600000;  // 3600000 milliseconds in an hour
const long minute = 60000;  // 60000 milliseconds in a minute
const long second = 1000;   // 1000 milliseconds in a second



unsigned long currentMillis = 0;  // stores the value of millis() in each iteration of loop()
// time management section
// as we don't use delay function
// to be able to switch off pump as fast as possible


EthernetClient net;
MQTTClient client;

void setup() {
  Serial.begin(9600);
  // start the Ethernet
  Ethernet.begin(mac, deviceIp);
  // start UDP
  Udp.begin(8888);
  
  client.begin("192.168.1.75", net);
  //SendUDPPacket("system started.");
  connect();
  SendUDPPacket("system started.");
}



void loop() {
  currentMillis = millis();  // capture the latest value of millis()
    client.loop();

  if (!client.connected()) {
    connect();
  }


}

void connect() {
  Serial.print("connecting...");
  while (!client.connect("arduino", "", "")) {
    Serial.print(".");
    delay(1000);
  }

  Serial.println("\nconnected!");

 // client.subscribe("/hello");
  // client.unsubscribe("/hello");
}




void SendUDPPacket(String message) {
  int len = message.length() + 1;
  char repBuff[len];
  message.toCharArray(repBuff, len);

  Udp.beginPacket(destinationIP, 8888);
  Udp.write(repBuff);
  Udp.endPacket();
  
  client.publish("/hello", message);    
  
  Serial.print("udp message sent: ");
  Serial.println(message);
}

short days = 0;
short hours = 0;
short minutes = 10;
short seconds = 0;
String GetTimeFromStart() {
  long timeNow = millis();
  days = timeNow / day;                         //number of days
  hours = (timeNow % day) / hour;               //the remainder from days division (in milliseconds) divided by hours, this gives the full hours
  minutes = ((timeNow % day) % hour) / minute;  //and so on...
  seconds = (((timeNow % day) % hour) % minute) / second;

  String buf;
  buf += F("D-hh:mm:ss ");
  buf += String(days, DEC);
  buf += F("-");
  buf += String(hours, DEC);
  buf += F(":");
  buf += String(minutes, DEC);
  buf += F(":");
  buf += String(seconds, DEC);
  return buf;
}
