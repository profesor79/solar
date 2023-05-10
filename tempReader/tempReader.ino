#include <EthernetUdp.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Ethernet.h>
#include <EthernetUdp.h>


// define pins
short tempSensorsPin = 7;

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
DeviceAddress tankTempAddres = { 0x28, 0x2A, 0xB7, 0xFE, 0xB0, 0x22, 0x8, 0x33 };  //dirección del sensor 1
DeviceAddress fromRoofToTankAddress = { 0x28, 0xB7, 0xA4, 0xE6, 0xB0, 0x22, 0x8, 0xDE };
DeviceAddress zone1Adress = {
  0x28,
  0x48,
  0xD9,
  0xF8,
  0xB0,
  0x22,
  0x9,
  0x7A,
};
//DeviceAddress address2 = {0x28, 0xFF, 0x89, 0x3A, 0x1, 0x16, 0x4, 0xAF};//dirección del sensor 2
//DeviceAddress address3 = {0x28, 0xFF, 0x23, 0x19, 0x1, 0x16, 0x4, 0xD9};//dirección del sensor 3
float tankTemp = -127;
float roofToTankTemp = -127;
float roof1ZoneTemp = -127;
float roof2ZoneTemp = -127;
float ambientTemp = -127;
float switchBoxTemp = -127;




unsigned long currentMillis = 0;  // stores the value of millis() in each iteration of loop()
// time management section
// as we don't use delay function
// to be able to switch off pump as fast as possible
unsigned long _lastSensorCheckTime;
unsigned long _lastSensorsRequestTime;
unsigned long _lastSolarPumpRunTime;
unsigned long _lastWaterPumpRunTime;

const unsigned long SensorCheckInterval = 5000;
const unsigned long SensorWaitInterval = 1500;



// state section
bool _solarPumpRunning = false;
bool _waterPumpRunning = false;
bool _temperatureWaseReadInThisCycle = true;


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
  sensors.setResolution(tankTempAddres, 9);
  sensors.setResolution(fromRoofToTankAddress, 9);
  sensors.setResolution(zone1Adress, 9);
}

void loop() {
  currentMillis = millis();  // capture the latest value of millis()
  SendReadTempCommand();
  GetTemperatures();
  SendUdpReport();
  delay(15000);
}
void GetTemperatures() {
  if (currentMillis < _lastSensorCheckTime + SensorWaitInterval) {
    if (!_temperatureWaseReadInThisCycle) {
      tankTemp = sensors.getTempC(tankTempAddres);
      roofToTankTemp = sensors.getTempC(fromRoofToTankAddress);
      roof1ZoneTemp = sensors.getTempC(zone1Adress);
      _temperatureWaseReadInThisCycle = true;
    }
  }
}
void SendUdpReport() {
  String buf;
  buf += F("TankTemp: ");
  buf += String(tankTemp, 2);
  buf += F(", RoofToTank: ");
  buf += String(roofToTankTemp, 2);
  char repBuff[buf.length()];
  buf.toCharArray(repBuff, buf.length());

  Udp.beginPacket(destinationIP, 8888);
  Udp.write(repBuff);
  Udp.endPacket();
}

void SendReadTempCommand() {
  if (currentMillis < _lastSensorCheckTime + SensorCheckInterval) {
    sensors.requestTemperatures();
    _temperatureWaseReadInThisCycle = false;
    _lastSensorsRequestTime = currentMillis;
  }
}

void FlipFlopPumps() {
}