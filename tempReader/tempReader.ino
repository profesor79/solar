#include <EthernetUdp.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Ethernet.h>
#include <EthernetUdp.h>



const unsigned long SensorCheckInterval = 10000;
const unsigned long SensorWaitInterval = 1500;
const unsigned long FlushInterval = 60000;
const unsigned long FlushTime = 10000;
const float mivAverageTempDiffForFlush  = 5.00;
const float tempDiffToStartWaterPump = 12.00;
const float tempDiffToStoptWaterPump = 3.00;



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
byte mac[] = {0xDE, 0xAD, 0xBE, 0xAA, 0xFE, 0xAA};
IPAddress deviceIp(192, 168, 1, 177);
IPAddress destinationIP(192, 168, 1, 38);  // the remote IP address
EthernetUDP Udp;
unsigned int destinationPort = 8888;

 const long day = 86400000; // 86400000 milliseconds in a day
 const long hour = 3600000; // 3600000 milliseconds in an hour
 const long minute = 60000; // 60000 milliseconds in a minute
 const long second =  1000; // 1000 milliseconds in a second

// temp sensors
DallasTemperature sensors(&ourWire);                                                //Se declara una variable u objeto para nuestro sensor
DeviceAddress tank2PumpAddres = { 0x28, 0x2A, 0xB7, 0xFE, 0xB0, 0x22, 0x8, 0x33 };  //dirección del sensor 1
DeviceAddress fromRoofToTankAddress = { 0x28, 0xB7, 0xA4, 0xE6, 0xB0, 0x22, 0x8, 0xDE };
DeviceAddress zone1Adress = { 0x28, 0x48, 0xD9, 0xF8, 0xB0, 0x22, 0x9, 0x7A };
DeviceAddress tankAddress = {  0x28,  0x51,  0x38,  0xDD,  0xB0,  0x22,  0x7,  0x4E };
DeviceAddress zone2Adress = {  0x28,  0x36,  0x74,  0x3,  0xB1,  0x22,  0x9,  0x60 };
float tank2Pump = -128;
float roofToTankTemp = -128;
float roof1ZoneTemp = -128;
float roof2ZoneTemp = -128;
float ambientTemp = -128;
float switchBoxTemp = -128;
float tankTemp = -128;

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
  sensors.setResolution(tank2PumpAddres, tempBithDepth);
  sensors.setResolution(fromRoofToTankAddress, tempBithDepth);
  sensors.setResolution(zone1Adress, tempBithDepth);
  sensors.setResolution(zone2Adress, tempBithDepth);
  sensors.setResolution(tankAddress, tempBithDepth);
}

void loop() {
  currentMillis = millis();  // capture the latest value of millis()
  FlipFlopPumps();
  SendReadTempCommand();
  FlipFlopPumps();
  GetTemperatures();
  FlipFlopPumps();
  SendUdpReport();
  FlipFlopPumps();
}


void GetTemperatures() {
  if (currentMillis > _lastSensorCheckTime + SensorWaitInterval) {
    if (!_temperatureWaseReadInThisCycle) {
      Serial.println("reading temperatures");
      _lastSensorCheckTime = currentMillis;
      roofToTankTemp = sensors.getTempC(fromRoofToTankAddress);
      roof1ZoneTemp = sensors.getTempC(zone1Adress);
      roof2ZoneTemp = sensors.getTempC(zone2Adress);
      tank2Pump = sensors.getTempC(tank2PumpAddres);
      tankTemp = sensors.getTempC(tankAddress);
      _temperatureWaseReadInThisCycle = true;
      _udpSent = false;
    }
  }
}

void SendStateMessage(){


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
    buf += F(", Z2: ");
    buf += String(roof2ZoneTemp, 2);
    buf += F(", last read: ");
    buf += GetTimeFromStart();
    char repBuff[buf.length()];
    buf.toCharArray(repBuff, buf.length());

    Udp.beginPacket(destinationIP, 8888);
    Udp.write(repBuff);
    Udp.endPacket();
    _udpSent = true;
        Serial.println("udp send");
  }
}

int days=0;
int hours =0;
int minutes =0;
float seconds =0;
 String GetTimeFromStart()
 {
 long timeNow = millis();
  days = timeNow / day ;                                //number of days
  hours = (timeNow % day) / hour;                       //the remainder from days division (in milliseconds) divided by hours, this gives the full hours
  minutes = ((timeNow % day) % hour) / minute ;         //and so on...
  seconds = (((timeNow % day) % hour) % minute) / second;
 
 String buf;
    buf += F("D: ") ;
    buf += String(days,DEC);
    buf += F(" h: ");
    buf += String(hours,DEC);
    buf += F(" m: ");    
    buf += String(minutes,DEC);
    buf += F(" s: ");
    buf += String(seconds,DEC);

  
  return buf;
  
}

void SwitchOnWaterPump(){
  if(_waterPumpRunning)  {return;}

  digitalWrite(solarPumpMotorPin, HIGH);
  _solarPumpRunning = true;
}


void SwitchOffWaterPump(){
  digitalWrite(solarPumpMotorPin, LOW);
  _solarPumpRunning = false;
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
  _waterPumpRunning = (digitalRead(waterPumpVoltagePin) == LOW) ;
  if(_waterPumpRunning){
    digitalWrite(solarPumpMotorPin, LOW);
    _solarPumpRunning=false;
  }
  
}