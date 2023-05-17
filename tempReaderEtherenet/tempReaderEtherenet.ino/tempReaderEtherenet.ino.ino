#include <EthernetUdp.h>
#include <MQTT.h>
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

unsigned long currentMillis = 0;  // stores the value of millis() in each iteration of loop()
unsigned long lastConfigSendTime = 0;
unsigned long configSendInterval = 60000;

EthernetClient net;
MQTTClient client;

void setup() {
  Serial.begin(9600);
  Serial1.begin(9600);
  // start the Ethernet
  Ethernet.begin(mac, deviceIp);
  // start UDP
  Udp.begin(8888);
  client.begin("192.168.1.75", net);
  connect();
  SendUDPPacket("system started.");

  // reserve 200 bytes for the inputString:
  inputString.reserve(200);

  publishConfig("WaterTank");
  publishConfig("Tank2Pump");
  publishConfig("ReturnToTank");
  publishConfig("RoofZone1");
}

const String configBase = "{\"name\": \"__\",\"unit_of_measurement\": \"C\",\"state_topic\": \"solar/sensor/__\",\"icon\": \"mdi:temperature-celsius\" }";

void SendConfiguration() {

  /*
RoofZone2
SolarPump
WaterPump

*/

  if (currentMillis > configSendInterval + lastConfigSendTime) {

    publishConfig("WaterTank");
    publishConfig("Tank2Pump");
    publishConfig("ReturnToTank");
    publishConfig("RoofZone1");
    lastConfigSendTime = currentMillis;
  }

  //publishConfig("");
  //publishConfig("");
}

void publishConfig(String name) {
  String c = configBase;
  c.replace("__","greg_" + name);
  String topic = "solar/sensor/greg_" + name + "/config";
  client.publish(topic, c);
  Serial.println(topic);
  Serial.println(c);
}

void loop() {
  currentMillis = millis();  // capture the latest value of millis()
  if (stringComplete) {
    //Serial.println(inputString);
    SendUDPPacket(inputString);
    // clear the string:
    inputString = "";
    stringComplete = false;
  }
  client.loop();
  SendConfiguration();
}

void connect() {
  Serial.print("connecting...");
  while (!client.connect("arduino", "", "")) {
    Serial.print(".");
    delay(1000);
  }

  Serial.println("\nconnected!");
}

/*
  SerialEvent occurs whenever a new data comes in the hardware serial RX. This
  routine is run between each time loop() runs, so using delay inside loop can
  delay response. Multiple bytes of data may be available.
*/
void serialEvent1() {
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

void PublishMessageoNTopic()
void SendUDPPacket(String message) {
  int len = message.length() + 1;
  char repBuff[len];
  message.toCharArray(repBuff, len);

  Udp.beginPacket(destinationIP, 8888);
  Udp.write(repBuff);
  Udp.endPacket();
  //Serial.print(message);

  short splitAt = message.indexOf(":");

  if (splitAt > 0) {
    String name = message.substring(0, splitAt);
    String value = message.substring(splitAt + 1, message.length());
    //  Serial.print("Name: " + name);
    //  Serial.println(", V: " + value);
    client.publish("solar/sensor/greg_" + name, value);
  
  

  }
}
