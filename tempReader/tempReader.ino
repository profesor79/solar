#include <EthernetUdp.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Ethernet.h>
#include <EthernetUdp.h>



const unsigned long SensorCheckInterval = 10000;
const unsigned long SensorWaitInterval = 1500;
const unsigned long FlushInterval = 60000;
const unsigned long FlushTime = 10000;
const float mivAverageTempDiffForFlush = 5.00;
const float tempDiffToStartWaterPump = 12.00;
const float tempDiffToStoptWaterPump = 3.00;

const float _maxTemp = 45.00;

// define pins
short tempSensorsPin = 49;  //purple cable on board
short temp2SensorsPin = 51;
short waterPumpMotorPin = 31;
short waterPumpVoltagePin = 36;
short solarPumpMotorPin = 32;
short solarPumpVoltagePin = 37;
short waterCutOffPin = 35;  //black cable on board


//onewire init
OneWire ourWire(tempSensorsPin);
OneWire ourWire2(temp2SensorsPin);


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

// temp sensors
DallasTemperature sensors(&ourWire);
DallasTemperature sensors2(&ourWire2);

DeviceAddress tank2PumpAddres = { 0x28, 0x2A, 0xB7, 0xFE, 0xB0, 0x22, 0x8, 0x33 };  //dirección del sensor 1
DeviceAddress fromRoofToTankAddress = { 0x28, 0xB7, 0xA4, 0xE6, 0xB0, 0x22, 0x8, 0xDE };
DeviceAddress zone1Adress = { 0x28, 0x48, 0xD9, 0xF8, 0xB0, 0x22, 0x9, 0x7A };
DeviceAddress tankAddress = { 0x28, 0x51, 0x38, 0xDD, 0xB0, 0x22, 0x7, 0x4E };
DeviceAddress zone2Adress = { 0x28, 0x36, 0x74, 0x3, 0xB1, 0x22, 0x9, 0x60 };
float tank2Pump = -128;
float roofToTankTemp = -128;
float roof1ZoneTemp = -128;
float roof2ZoneTemp = -128;
float ambientTemp = -128;
float switchBoxTemp = -128;
float tankTemp = -128;

float averageTankTemperature = 0;
float averageRoofTemperature = 0;
const short tempBithDepth = 12;


unsigned long currentMillis = 0;  // stores the value of millis() in each iteration of loop()
// time management section
// as we don't use delay function
// to be able to switch off pump as fast as possible
unsigned long _lastSensorCheckTime;
unsigned long _lastSensorsRequestTime;
unsigned long _lastSolarPumpRunTime;
unsigned long _lastWaterPumpRunTime;




// state section
bool _solarPumpRunning = false;
bool _waterPumpRunning = false;
bool _temperatureWaseReadInThisCycle = true;
bool _udpSent = false;

void StopRelays() {

  digitalWrite(solarPumpMotorPin, HIGH);
  digitalWrite(waterPumpMotorPin, HIGH);
}
void setup() {

  pinMode(solarPumpMotorPin, OUTPUT);
  pinMode(waterPumpMotorPin, OUTPUT);
  pinMode(waterCutOffPin, INPUT);

  StopRelays();
  Serial.begin(9600);

  // start the Ethernet
  Ethernet.begin(mac, deviceIp);

  // start UDP
  Udp.begin(8888);
  sensors.begin();
//  sensors2.begin();
  SetSensorsResolution();
  PrintSensorAddresses();
  //switch on water pump
  digitalWrite(waterPumpMotorPin, LOW);
}

void SetSensorsResolution() {
  // setting resolution to 9 bits as this helps in a star topology
  sensors.setResolution(tank2PumpAddres, tempBithDepth);
  sensors.setResolution(fromRoofToTankAddress, tempBithDepth);
  sensors.setResolution(zone1Adress, tempBithDepth);
  sensors.setResolution(zone2Adress, tempBithDepth);
  sensors.setResolution(tankAddress, tempBithDepth);
}

void readOneWire(OneWire wire) {
  byte addr[8];
  Serial.println("Getting addresses:");
  while (wire.search(addr)) {
    Serial.print("DeviceAddress da = {");
    for (int i = 0; i < 8; i++) {
      Serial.print("0x");
      Serial.print(addr[i], HEX);
      Serial.print(", ");
    }
    Serial.println("};");
  }

  Serial.println();
  wire.reset_search();
}
void PrintSensorAddresses() {
  // this is to display temperature sensors addresses
  readOneWire(ourWire);
  readOneWire(ourWire2);
}

void loop() {
  currentMillis = millis();  // capture the latest value of millis()
  FlipFlopPumps();
  SendReadTempCommand();
  GetTemperatures();
  SendUdpReport();
  ManageSolarPumpStateByTemperature();

  // PlayRelaySound();
}

void PlayRelaySound() {
  while (true) {



    delay(500);
    digitalWrite(solarPumpMotorPin, LOW);
    delay(50000);
    digitalWrite(solarPumpMotorPin, HIGH);
    delay(500);

    delay(500);
    digitalWrite(waterPumpMotorPin, LOW);
    delay(50000);
    digitalWrite(waterPumpMotorPin, HIGH);
    delay(500);
    delay(10000);
  }
}

float avgZ1Z2 = 0;
float avgTank = 0;
float diff = 0;

void ManageSolarPumpStateByTemperature() {
  if (tankTemp > _maxTemp) {
    if (_solarPumpRunning) {
      SendUDPPacket("switch solap pupm off overtemperature reached!!!");
      SwitchOffSolarPump();
    }

    return;
  }
  if (tankTemp < 0 | roofToTankTemp < 0 | roof1ZoneTemp < 0) {
    return;
  }
  avgZ1Z2 = roof1ZoneTemp;
  avgTank = tankTemp;
  diff = avgZ1Z2 - avgTank;

  if (_solarPumpRunning) {
    // now we use return to tank sensor to switch this guy off
    if (roofToTankTemp - 3 > tankTemp) {
      return;  // pump the water
    } else {
      SwitchOffSolarPump();
    }
  }

  if (diff > (5)) {
    if (_solarPumpRunning) {
      return;
    }

    SwitchOnSolarPump();
    Serial.print("diff: ");
    return;
  }
  SwitchOffSolarPump();
}



void GetTemperatures() {
  if (currentMillis > _lastSensorCheckTime + SensorWaitInterval) {
    if (!_temperatureWaseReadInThisCycle) {
      Serial.println("reading temperatures");
      _lastSensorCheckTime = currentMillis;
      roofToTankTemp = readTemperaturyBySensorAndAddress(sensors, fromRoofToTankAddress, roofToTankTemp);

      roof1ZoneTemp = readTemperaturyBySensorAndAddress(sensors, zone1Adress, roof1ZoneTemp);
 //     roof2ZoneTemp = readTemperaturyBySensorAndAddress(sensors2, zone2Adress, roof2ZoneTemp);
      tank2Pump = readTemperaturyBySensorAndAddress(sensors, tank2PumpAddres, tank2Pump);
      tankTemp = readTemperaturyBySensorAndAddress(sensors, tankAddress, tankTemp);

      //2nd sensor group here


      _temperatureWaseReadInThisCycle = true;
      _udpSent = false;
    }
  }
}

float readTemperaturyBySensorAndAddress(DallasTemperature s, DeviceAddress da, float currentTemperature) {

  float tmp = s.getTempC(da);
  if (tmp != -127.00 && tmp != 85.00) {
    return tmp;
  } else {
    // TODO: here we need to mark that we can't read
    Serial.println("cant read temperature");
    char *b = addr2str(da);
    Serial.println(b);
    String msg = "Can't read temperature, sensor address is: ";
    msg += b;
    SendUDPPacket(msg);
    return currentTemperature;
  }
}

char *addr2str(DeviceAddress deviceAddress) {
  static char return_me[18];
  static char *hex = "0123456789ABCDEF";
  uint8_t i, j;

  for (i = 0, j = 0; i < 8; i++) {
    return_me[j++] = hex[deviceAddress[i] / 16];
    return_me[j++] = hex[deviceAddress[i] & 15];
  }
  return_me[j] = '\0';

  return (return_me);
}
void SendStateMessage() {
}

void SendUdpReport() {
  if (_udpSent) {
    return;
  }
  if (currentMillis > _lastSensorCheckTime + SensorWaitInterval & _lastSensorCheckTime > 0) {
    String buf;
    buf += F("T: ");
    buf += String(tankTemp, 2);
    buf += F(", TP: ");
    buf += String(tank2Pump, 2);
    buf += F(", RTT: ");
    buf += String(roofToTankTemp, 2);
    buf += F(", Z1: ");
    buf += String(roof1ZoneTemp, 2);
    //buf += F(", Z2: ");
    //buf += String(roof2ZoneTemp, 2);
    buf += F(", SP: ");
    buf += String(_solarPumpRunning);
    buf += F(", WP: ");
    buf += String(_waterPumpRunning);
    buf += F(", diff: ");
    buf += String(diff, 2);
    buf += F(", last read: ");
    buf += GetTimeFromStart();
    SendUDPPacket(buf);
    _udpSent = true;
  }
}

void SendUDPPacket(String message) {
  int len = message.length() + 1;
  char repBuff[len];
  message.toCharArray(repBuff, len);

  Udp.beginPacket(destinationIP, 8888);
  Udp.write(repBuff);
  Udp.endPacket();

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

void SwitchOnSolarPump() {
  if (_waterPumpRunning) { return; }

  digitalWrite(solarPumpMotorPin, LOW);
  if (!_solarPumpRunning) {
    _solarPumpRunning = true;
    SendUDPPacket("Solar Pump on");
  }
}


void SwitchOffSolarPump() {

  digitalWrite(solarPumpMotorPin, HIGH);
  if (_solarPumpRunning) {
    _solarPumpRunning = false;
    SendUDPPacket("Solar Pump OFF");
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
  bool tmpIsPumpRunning = (digitalRead(waterCutOffPin) == LOW);
  if (tmpIsPumpRunning == _waterPumpRunning) {
    // do nothing here
    return;
  }

  _waterPumpRunning = tmpIsPumpRunning;

  if (_waterPumpRunning) {
    SwitchOffSolarPump();
    _solarPumpRunning = false;
    SendUDPPacket("Water Pump On");
  } else {
    SendUDPPacket("Water Pump OFF");
  }
}