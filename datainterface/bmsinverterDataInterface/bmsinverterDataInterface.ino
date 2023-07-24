
String results[25] = {
  "gridVoltage",        
  "gridFrequency",      //1
  "outputVoltage",      //2
  "outputFrequency",    //3
  "outputPowerApparent",//4
  "outputPowerActive",  //5
  "outputLoadPercent",  //6
  "busVoltage",         //7
  "batteryVoltage",//8
  "batteryChargingCurrent",//9
  "batteryCapacity",//10
  "temperature",//11
  "pvBatteryCurrent",//12
  "pvInputVoltage",//13
  "batteryVoltageSCC",//14
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

 

String inputString = "";      // a String to hold incoming data
bool stringComplete = false;  // whether the string is complete

unsigned long currentMillis = 0;  // stores the value of millis() in each iteration of loop()
unsigned short crc;
 

const unsigned long CheckInterval = 1000;
unsigned long _lastCheckTime;
void setup() {
  Serial.begin(9600);
  Serial1.begin(2400);
  Serial.println("starting");
    // reserve 200 bytes for the inputString:
  inputString.reserve(200);  


}


void serialEvent1() {
  while (Serial1.available()) {
    // get the new byte:
    char inChar = (char)Serial1.read();
    // add it to the inputString:
    inputString += inChar;
   
    // if the incoming character is a newline, set a flag so the main loop can
    // do something about it:
    if (inChar == '\r') {
      stringComplete = true;
    }
  }
}
 

void loop() {
  // put your main code here, to run repeatedly:
  currentMillis = millis();  // capture the latest value of millis()
  if(currentMillis-_lastCheckTime>CheckInterval){
     _lastCheckTime=currentMillis; 
    qpigs();
      
    }
  if (stringComplete) {
   ProcessSerial1Data();
    // clear the string:
    inputString = "";
    stringComplete = false;
  } 
}

void ProcessSerial1Data(){
  float u_13=0;
  float i_12=0;
   int counter = 0;
    int len = inputString.length();
    String command ="";
    for(int i=0;i<len;i++){
      if(counter>13) continue;
      if(inputString[i]==' '){
        
       String buffer=results[counter];
       buffer+=":";
       buffer+=command;
       buffer+="\r\n";    
       Serial.print(results[counter]);
       Serial.print(":");        
       Serial.println(command);        
       if(counter==12)
       {
        i_12 = command.toFloat();
        }

      if(counter==13)
      {
        u_13 = command.toFloat();
        long pvW = (long)u_13*i_12 + 0.5;
        Serial.print("PvPower:");
        Serial.println(pvW);                
      }
      
      command ="";
      counter++;
      }else{
        //Serial.println(incomingString[i]);
        command +=inputString[i];
        }
      }    
  }


void qpigs()
{
    _nextCommandNeeded="QPIGS";
  crc = cal_crc_half((byte*)_nextCommandNeeded.c_str(), _nextCommandNeeded.length());  
  
    Serial1.print(_nextCommandNeeded);
    Serial1.print((char)((crc >> 8) & 0xFF));
    Serial1.print((char)((crc >> 0) & 0xFF));
    Serial1.print("\r");   
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
