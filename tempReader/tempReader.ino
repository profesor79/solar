#include <EthernetUdp.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Ethernet.h>
#include <EthernetUdp.h>


// define pins
short tempSensorsPin = 7;
short waterPumpMotorPin = 10;
short waterPumpVoltagePin = 10;
short solarPumpMotorPin = 10;
short solarPumpVoltagePin = 10;
short waterCutOffPin = 10;


//onewire init
OneWire ourWire(tempSensorsPin);


//etherent
byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xAA, 0xFE, 0xAA
};
IPAddress deviceIp(192, 168, 1, 177);
IPAddress destinationIP(192, 168, 1, 38);  // the remote IP address
EthernetUDP Udp;
unsigned int destinationPort = 8888;



// temp sensors
DallasTemperature sensors(&ourWire);                                               //Se declara una variable u objeto para nuestro sensor
DeviceAddress tank2PumpAddres = { 0x28, 0x2A, 0xB7, 0xFE, 0xB0, 0x22, 0x8, 0x33 };  //dirección del sensor 1
DeviceAddress fromRoofToTankAddress = { 0x28, 0xB7, 0xA4, 0xE6, 0xB0, 0x22, 0x8, 0xDE };
DeviceAddress zone1Adress = { 0x28, 0x48, 0xD9, 0xF8, 0xB0, 0x22, 0x9, 0x7A };
DeviceAddress waterProofTestAddress = {0x28, 0x51, 0x38, 0xDD, 0xB0, 0x22, 0x7, 0x4E, };
DeviceAddress zone2Adress = {0x28, 0x36, 0x74, 0x3, 0xB1, 0x22, 0x9, 0x60, };
//DeviceAddress address2 = {0x28, 0xFF, 0x89, 0x3A, 0x1, 0x16, 0x4, 0xAF};//dirección del sensor 2
//DeviceAddress address3 = {0x28, 0xFF, 0x23, 0x19, 0x1, 0x16, 0x4, 0xD9};//dirección del sensor 3
float tank2Pump = -128;
float roofToTankTemp = -128;
float roof1ZoneTemp = -128;
float roof2ZoneTemp = -128;
float ambientTemp = -128;
float switchBoxTemp = -128;
float waterProofTest = -128;

const short tempBithDepth = 12;


unsigned long currentMillis = 0;  // stores the value of millis() in each iteration of loop()
// time management section
// as we don't use delay function
// to be able to switch off pump as fast as possible
unsigned long _lastSensorCheckTime;
unsigned long _lastSensorsRequestTime;
unsigned long _lastSolarPumpRunTime;
unsigned long _lastWaterPumpRunTime;

const unsigned long SensorCheckInterval = 60000;
const unsigned long SensorWaitInterval = 1500;



// state section
bool _solarPumpRunning = false;
bool _waterPumpRunning = false;
bool _temperatureWaseReadInThisCycle = true;
bool _udpSent = false;


void setup() {
  Serial.begin(9600);

  // start the Ethernet
  Ethernet.begin(mac, deviceIp);

  // start UDP
  Udp.begin(8888);
  sensors.begin();  //Se inicia el sensor

  // this is to display temperature sensors addresses
  byte addr[8];
  Serial.println("Getting addresses:");
  while (ourWire.search(addr)) {
    Serial.print("DeviceAddress da = {");
    for (int i = 0; i < 8; i++) {
      Serial.print("0x");
      Serial.print(addr[i], HEX);
      Serial.print(", ");
    }
    Serial.println("};");
  }

  Serial.println();
  ourWire.reset_search();

  // setting resolution to 9 bits as this helps in a star topology
  sensors.setResolution(tankTempAddres, tempBithDepth);
  sensors.setResolution(fromRoofToTankAddress, tempBithDepth);
  sensors.setResolution(zone1Adress, tempBithDepth);
   sensors.setResolution(zone2Adress, tempBithDepth);
   sensors.setResolution(waterProofTestAddress, tempBithDepth);

}

void loop() {
  currentMillis = millis();  // capture the latest value of millis()
  SendReadTempCommand();
  GetTemperatures();
  SendUdpReport();
  
}
void GetTemperatures() {

  if (currentMillis > _lastSensorCheckTime + SensorWaitInterval) {
    if (!_temperatureWaseReadInThisCycle) {
      Serial.println("reading temperatures");
      _lastSensorCheckTime = currentMillis;            
      roofToTankTemp = sensors.getTempC(fromRoofToTankAddress);
      roof1ZoneTemp = sensors.getTempC(zone1Adress);
      roof2ZoneTemp = sensors.getTempC(zone2Adress);
      tankTemp = sensors.getTempC(tankTempAddres);
      waterProofTest = sensors.getTempC(waterProofTestAddress);
      _temperatureWaseReadInThisCycle = true;
      _udpSent = false;
    }
  }
}
void SendUdpReport() {
  if(_udpSent){
    return;
  }

  if (currentMillis > _lastSensorCheckTime + SensorWaitInterval) {
     Serial.println("udp send");
      String buf;
      buf += F("Tank: ");
      buf += String(tankTemp, 2);
      buf += F(", RoofTotank: ");      
      buf += String(roofToTankTemp, 2);
      buf += F(", Z1: ");
      buf += String(roof1ZoneTemp, 2);
      buf += F(", Z2: ");
      buf += String(roof2ZoneTemp, 2);
      buf += F(", waterTest: ");
      buf += String(waterProofTest, 2);      
      buf += F(", last read: ");
      buf += String(_lastSensorCheckTime);
      char repBuff[buf.length()];
      buf.toCharArray(repBuff, buf.length());

      Udp.beginPacket(destinationIP, 8888);
      Udp.write(repBuff);
      Udp.endPacket();
      _udpSent = true;
    
  }
}

void SendReadTempCommand() {
 
  if (currentMillis > _lastSensorsRequestTime + SensorCheckInterval) {
    Serial.println("sending get temp request");
    sensors.requestTemperatures();
    _temperatureWaseReadInThisCycle = false;

    _lastSensorsRequestTime = currentMillis;
  }
}

void FlipFlopPumps() {
}