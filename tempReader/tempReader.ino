#include <EthernetUdp.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Ethernet.h>
#include <EthernetUdp.h>

//onewire init
OneWire ourWire(7);


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
//DeviceAddress address2 = {0x28, 0xFF, 0x89, 0x3A, 0x1, 0x16, 0x4, 0xAF};//dirección del sensor 2
//DeviceAddress address3 = {0x28, 0xFF, 0x23, 0x19, 0x1, 0x16, 0x4, 0xD9};//dirección del sensor 3
float tankTemp = -127;
float roofToTankTemp = -127;
float roof1ZoneTemp = -127;
float roof2ZoneTemp = -127;
float ambientTemp = -127;
float switchBoxTemp = -127;



unsigned long myTime;
// time management section
// as we don't use delay function
// to be able to switch off pump as fast as possible
unsigned long _lastSensorCheckTime;
unsigned long _lastSensorsRequestTime;
unsigned long _lastSolarPumpRunTime;
unsigned long _lastWaterPumpRunTime;

const unsigned long SensorCheckInterval = 5000;



// state section
bool _solarPumpRunning = false;
bool _waterPumpRunning = false;


void setup() {

  // start the Ethernet
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
}

void loop() {

  sensors.requestTemperatures();

  Serial.print("Time: ");
  myTime = millis();

  Serial.println(myTime);  // prints time since program started


  float tankTemp = sensors.getTempC(tankTempAddres);  //Se obtiene la temperatura en °C del sensor 1
  float fromRootToTank = sensors.getTempC(fromRoofToTankAddress);
  delay(1000);
  //float temp2= sensors.getTempC(address2);//Se obtiene la temperatura en °C del sensor 2
  //float temp3= sensors.getTempC(address3);//Se obtiene la temperatura en °C del sensor 3
  Serial.println(myTime);  // prints time since program started
  Serial.print("TankTemp = ");
  Serial.print(tankTemp);
  Serial.print(", RoofToTank = ");
  Serial.println(fromRootToTank);



  String buf;
  buf += F("TankTemp: ");
  buf += String(tankTemp, 2);
  buf += F(", RoofToTank: ");
  buf += String(fromRootToTank, 2);
  char repBuff[buf.length()];
  buf.toCharArray(repBuff, buf.length());

  Udp.beginPacket(destinationIP, 8888);
  Udp.write(repBuff);
  Udp.endPacket();


  delay(15000);
}


void SendReadTempCommand(){

sensors.requestTemperatures();

}
void FlipFlopPumps())
{

  
}