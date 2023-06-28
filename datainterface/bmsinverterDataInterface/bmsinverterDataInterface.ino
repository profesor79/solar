#include <SoftwareSerial.h>

const byte bmsRxPin = 7;
const byte bmsTxPin = 8;

SoftwareSerial bmsSerial(bmsRxPin, bmsTxPin);

const byte inverterRxPin = 6;
const byte inverterTxPin = 5;
//SoftwareSerial inverterSerial(inverterRxPin, inverterTxPin);




String results[25] = {
  "gridVoltage_0",        
  "gridFrequency_1",      //1
  "outputVoltage_2",      //2
  "outputFrequency_3",    //3
  "outputPowerApparent_4",//4
  "outputPowerActive_5",  //5
  "outputLoadPercent_6",  //6
  "busVoltage_7",         //7
  "batteryVoltage_8",//8
  "batteryChargingCurrent_9",//9
  "batteryCapacity_10",//10
  "temperature_11",//11
  "pvBatteryCurrent_12",//12
  "pvInputVoltage_13",//13
  "batteryVoltageSCC_14",//14
  "batteryDischargeCurrent_15",  //15
    "addSBUPriorityVersion_16",
    "configChanged_17",
    "sccFirmwareUpdates_18",
    "loadOn_19",
    "batteryVoltToSteady_20",
    "charging_21",
    "chargingSCC_22",
    "chargingAC_23",  
  "pvPower_24"}
;


String _commandBuffer;
String _lastRequestedCommand = "-"; 

String _nextCommandNeeded;

//String _nextCommandNeeded = "";
bool _allMessagesUpdated = false;

 

void setup() {
  // put your setup code here, to run once:
 // Define pin modes for TX and RX
  pinMode(bmsRxPin, INPUT);
  pinMode(bmsTxPin, OUTPUT);
  pinMode(inverterRxPin, INPUT);
  pinMode(inverterTxPin, OUTPUT);
  Serial.begin(9600);
  Serial1.begin(2400);
  
  //inverterSerial.begin(2400);
  Serial.println("starting");

}

void loop() {
  // put your main code here, to run repeatedly:

qpigs();
delay(500);
}


void qpigs()
{
  
  byte c; 
    _commandBuffer = "";   
_nextCommandNeeded="QPIGS";
    unsigned short crc = cal_crc_half((byte*)_nextCommandNeeded.c_str(), _nextCommandNeeded.length());  
    Serial.print("command to execute: ");
    Serial.println(_nextCommandNeeded);
  
    Serial1.print(_nextCommandNeeded);
    Serial1.print((char)((crc >> 8) & 0xFF));
    Serial1.print((char)((crc >> 0) & 0xFF));
    Serial1.print("\r");   

  
  
delay(100);
Serial1.setTimeout(500); 
   String incomingString = Serial1.readStringUntil('\r');
Serial.println(incomingString );
    int counter = 0;
    int len = incomingString.length();
    String command ="";
    for(int i=0;i<len;i++){
      if(incomingString[i]==' '){
      Serial.print("counter: ");
        Serial.print(counter);
        Serial.print(" command: ");
        Serial.print(results[counter]);
        Serial.print(" value: ");
      Serial.println(command);
      command ="";
      counter++;
      }else{
        //Serial.println(incomingString[i]);
        command +=incomingString[i];
        }
  
    }

       Serial.print("counter: ");
        Serial.print(counter);
        Serial.print(" command: ");
        Serial.print(results[counter]);
        Serial.print(" value: ");
      Serial.println(command);
}







unsigned short cal_crc_half(byte* pin, byte len)
{
  unsigned short crc;
  byte da;
  byte *ptr;
  byte bCRCHign;
  byte bCRCLow;

  const unsigned short crc_ta[16]=
  { 
      0x0000,0x1021,0x2042,0x3063,0x4084,0x50a5,0x60c6,0x70e7,
      0x8108,0x9129,0xa14a,0xb16b,0xc18c,0xd1ad,0xe1ce,0xf1ef
  };
  
  ptr=pin;
  crc=0;
  while(len--!=0) 
  {
    da=((byte)(crc>>8))>>4; 
    crc<<=4;
    crc^=crc_ta[da^(*ptr>>4)]; 
    da=((byte)(crc>>8))>>4; 
    crc<<=4;
    crc^=crc_ta[da^(*ptr&0x0f)]; 
    ptr++;
  }
  bCRCLow = crc;
  bCRCHign= (byte)(crc>>8);
  if(bCRCLow==0x28||bCRCLow==0x0d||bCRCLow==0x0a)
  {
    bCRCLow++;
  }
  if(bCRCHign==0x28||bCRCHign==0x0d||bCRCHign==0x0a)
  {
    bCRCHign++;
  }
  crc = ((unsigned short)bCRCHign)<<8;
  crc += bCRCLow;
  return(crc);
}
