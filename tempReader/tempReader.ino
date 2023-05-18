#include <OneWire.h>
#include <DallasTemperature.h>
#include <SoftwareSerial.h>



const unsigned long SensorCheckInterval = 30000;
const unsigned long SensorWaitInterval = 1500;
const unsigned long FlushInterval = 60000;
const unsigned long FlushTime = 10000;
const unsigned long DebounceTime = 120000;
const float mivAverageTempDiffForFlush = 5.00;
const float tempDiffToStartWaterPump = 12.00;
const float tempDiffToStoptWaterPump = 3.00;

const float _maxTemp = 48.00;

// define pins
const byte tempSensorsPin = 2;        //49;  //purple cable on board
const byte temp2SensorsPin = 3;       //41;
const byte waterPumpMotorPin = 6;     //31;
const byte waterPumpPowerOnPin = 10;  //36;
const byte solarPumpMotorPin = 7;     //32;
const byte solarPumpPowerOnPin = 11;  //37;
const byte waterCutOffPin = 12;       //  35;  //black cable on board

const byte rxPin = 4;
const byte txPin = 5;

// Set up a new SoftwareSerial object
SoftwareSerial mySerial(rxPin, txPin);


const long day = 86400000;  // 86400000 milliseconds in a day
const long hour = 3600000;  // 3600000 milliseconds in an hour
const long minute = 60000;  // 60000 milliseconds in a minute
const long second = 1000;   // 1000 milliseconds in a second


//onewire init
OneWire ourWire(tempSensorsPin);
OneWire ourWire2(temp2SensorsPin);

// temp sensors
DallasTemperature sensors(&ourWire);
DallasTemperature sensors2(&ourWire2);

DeviceAddress tank2PumpAddres = { 0x28, 0x2A, 0xB7, 0xFE, 0xB0, 0x22, 0x8, 0x33 };  //dirección del sensor 1
DeviceAddress fromRoofToTankAddress = { 0x28, 0xB7, 0xA4, 0xE6, 0xB0, 0x22, 0x8, 0xDE };
DeviceAddress zone1Adress = { 0x28, 0x48, 0xD9, 0xF8, 0xB0, 0x22, 0x9, 0x7A };
DeviceAddress tankAddress = { 0x28, 0x51, 0x38, 0xDD, 0xB0, 0x22, 0x7, 0x4E };
DeviceAddress zone2Adress = { 0x28, 0x36, 0x74, 0x3, 0xB1, 0x22, 0x9, 0x60 };
DeviceAddress sensorWithEpoxy = { 0x28, 0x96, 0xAB, 0xDE, 0xB0, 0x22, 0x9, 0x3A };
DeviceAddress da = { 0x28, 0xAA, 0x7B, 0x7, 0xB1, 0x22, 0x8, 0x93 };


float tank2Pump = -128;
float roofToTankTemp = -128;
float roof1ZoneTemp = -128;
float roof2ZoneTemp = -128;
float ambientTemp = -128;
float switchBoxTemp = -128;
float tankTemp = -128;

float averageTankTemperature = 0;
float averageRoofTemperature = 0;
const short tempBithDepth = 9;


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
bool _waterPumpPowerOn = false;
bool _systemPowerOn = false;
bool _solarPumpPowerOn = false;
bool _temperatureWaseReadInThisCycle = true;
bool parametersSend = false;

void setup() {
  pinMode(solarPumpMotorPin, OUTPUT);
  pinMode(waterPumpMotorPin, OUTPUT);
  pinMode(waterCutOffPin, INPUT);
  pinMode(waterPumpPowerOnPin, INPUT);
  pinMode(solarPumpPowerOnPin, INPUT);

  // Define pin modes for TX and RX
  pinMode(rxPin, INPUT);
  pinMode(txPin, OUTPUT);

  // Set the baud rate for the SoftwareSerial object
  mySerial.begin(9600);

  Serial.begin(9600);
  Serial.println("version 0.37");
  mySerial.println("version 0.37");
  // start the Ethernet
  Serial.println("starting sensors 1 ");
  sensors.begin();
  Serial.println("starting sensors 2 ");
  sensors2.begin();
  Serial.println("sensors started ");

  StopRelays();
  SetSensorsResolution();
  PrintSensorAddresses();

  // read PowerOn so we can see that the system re-started
  ReadPowerOnState();
  //switch on water pump
  digitalWrite(waterPumpMotorPin, LOW);
}



void loop() {
  currentMillis = millis();  // capture the latest value of millis()
  ReadPowerOnState();
  FlipFlopPumps();
  SendReadTempCommand();
  GetTemperatures();
  SendReport();
  ManageSolarPumpStateByTemperature();

  // PlayRelaySound();
}

void ReadPowerOnState() {
  bool tmpIsPumpRunning = (digitalRead(solarPumpPowerOnPin) == LOW);
  if (tmpIsPumpRunning != _solarPumpRunning) {
    _solarPumpPowerOn = tmpIsPumpRunning;
    if (_solarPumpPowerOn == true) {
      WriteLogEntry("SolarPumpPowerOn:1");
    } else {
      WriteLogEntry("SolarPumpPowerOn:0");
    }
  }

  // water pump is enabled always and controlled by pressure switch
  bool tmpIsPowerOn = (digitalRead(waterPumpPowerOnPin) == LOW);

  if (tmpIsPowerOn != _waterPumpPowerOn) {
    _waterPumpPowerOn = tmpIsPowerOn;
    if (_waterPumpPowerOn == true) {
      WriteLogEntry("SystemPowerOn:1");
    } else {
      WriteLogEntry("SystemPowerOn:0");
    }
  }
}


void SetSensorsResolution() {
  // setting resolution to 9 bits as this helps in a star topology

  sensors.setResolution(tank2PumpAddres, tempBithDepth);
  sensors.setResolution(fromRoofToTankAddress, tempBithDepth);
  sensors.setResolution(zone1Adress, tempBithDepth);
  sensors2.setResolution(zone2Adress, tempBithDepth);
  sensors.setResolution(tankAddress, tempBithDepth);
}
void StopRelays() {
  digitalWrite(solarPumpMotorPin, HIGH);
  digitalWrite(waterPumpMotorPin, HIGH);
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
long debounceStartTime = 0;
void ManageSolarPumpStateByTemperature() {
  if (tankTemp > _maxTemp) {
    if (_solarPumpRunning) {
      WriteLogEntry("switch solap pump off overtemperature reached!!!");
      SwitchOffSolarPump();
    }

    return;
  }
  if (tankTemp < 0 | roofToTankTemp < 0 | roof1ZoneTemp < 0) {
    return;
  }

  if (roof1ZoneTemp > roof2ZoneTemp) {
    avgZ1Z2 = roof1ZoneTemp;
  } else {
    avgZ1Z2 = roof2ZoneTemp;
  }

  avgTank = tankTemp;
  diff = avgZ1Z2 - avgTank;

  if (_solarPumpRunning) {
    if (currentMillis < debounceStartTime + DebounceTime) {
      return;
      // do nothing
    }

    // now we use return to tank sensor to switch this guy off
    if (roofToTankTemp - 1 > tankTemp) {
      return;  // pump the water
    }
  }

  if (diff > (5)) {
    if (_solarPumpRunning) {
      return;
    }

    debounceStartTime = currentMillis;
    SwitchOnSolarPump();
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
      tank2Pump = readTemperaturyBySensorAndAddress(sensors, tank2PumpAddres, tank2Pump);
      tankTemp = readTemperaturyBySensorAndAddress(sensors, tankAddress, tankTemp);

      //2nd sensor group here
      //   roof2ZoneTemp = readTemperaturyBySensorAndAddress(sensors2, da, roof2ZoneTemp);
      roof2ZoneTemp = readTemperaturyBySensorAndAddress(sensors2, zone2Adress, roof2ZoneTemp);

      _temperatureWaseReadInThisCycle = true;
      parametersSend = false;
    }
  }
}

float readTemperaturyBySensorAndAddress(DallasTemperature s, DeviceAddress da, float currentTemperature) {

  float tmp = s.getTempC(da);
  if (tmp != -127.00 && tmp != 85.00) {
    return tmp;
  } else {
    // TODO: here we need to mark that we can't read
    char *b = addr2str(da);
    String msg = "Can't read temperature, sensor address is: ";
    msg += b;
    WriteLogEntry(msg);
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

short reportStep = 0;
void SendReport() {
  if (parametersSend) {
    return;
  }
  if (currentMillis > _lastSensorCheckTime + SensorWaitInterval & _lastSensorCheckTime > 0) {

    String buf;
    if (reportStep == 0) {
      reportStep++;
      buf = F("WaterTank:");
      buf += String(tankTemp, 2);
      WriteLogEntry(buf);
    }

    if (reportStep == 1) {
      reportStep++;
      buf = F("Tank2Pump:");
      buf += String(tank2Pump, 2);
      WriteLogEntry(buf);
    }

    if (reportStep == 2) {
      reportStep++;
      buf = F("ReturnToTank:");
      buf += String(roofToTankTemp, 2);
      WriteLogEntry(buf);
    }


    if (reportStep == 3) {
      reportStep++;
      buf = F("RoofZone1:");
      buf += String(roof1ZoneTemp, 2);
      WriteLogEntry(buf);
    }


    if (reportStep == 4) {
      reportStep++;
      buf = F("RoofZone2:");
      buf += String(roof2ZoneTemp, 2);
      WriteLogEntry(buf);
    }


    if (reportStep == 5) {
      reportStep++;
      buf = F("SolarPump:");
      if (_solarPumpRunning == true) {
        buf += String(1);
      } else {
        buf += String(0);
      }
      WriteLogEntry(buf);
    }

    if (reportStep == 6) {
      reportStep++;
      buf = F("WaterPump:");
      if (_waterPumpRunning == true) {
        buf += String(1);
      } else {
        buf += String(0);
      }
      WriteLogEntry(buf);
    }

if (reportStep == 7) {
      reportStep++;
      buf = F("SystemPowerOn:");
      if (_waterPumpPowerOn == true) {
        buf += String(1);
      } else {
        buf += String(0);
      }
      WriteLogEntry(buf);
    }

if (reportStep == 8) {
      reportStep++;
      buf = F("SolarPumpPowerOn:");
      if (_solarPumpPowerOn == true) {
        buf += String(1);
      } else {
        buf += String(0);
      }
      WriteLogEntry(buf);
    }

    if (reportStep == 7) {
      buf = F("TemperatureDiff:");
      buf += String(diff, 2);
      WriteLogEntry(buf);
      parametersSend = true;
      reportStep = 0;
    }
  }
}

void WriteLogEntry(String message) {
  Serial.println(message);
  mySerial.println(message);
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
    WriteLogEntry("SolarPump:1");
  }
}


void SwitchOffSolarPump() {
  digitalWrite(solarPumpMotorPin, HIGH);
  if (_solarPumpRunning) {
    _solarPumpRunning = false;
    WriteLogEntry("SolarPump:0");
  }
}

void SendReadTempCommand() {
  if (currentMillis > _lastSensorsRequestTime + SensorCheckInterval) {
    sensors.requestTemperatures();
    sensors2.requestTemperatures();
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
    WriteLogEntry("WaterPump:1");
  } else {
    WriteLogEntry("WaterPump:0");
  }
}