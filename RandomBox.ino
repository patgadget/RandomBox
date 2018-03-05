// My DMX Console Have a Duemilanove (set Arduino before Transfert)
#include <SoftwareSerial.h>

// DMX Console
// Analogue (5) Joystick X
// Analogue (4) Joystick Y
// Analogue (3) Pot #4
// Analogue (2) Pot #3
// Analogue (1) Pot #2
// Analogue (0) Pot #1


#define RxLCD 6
#define TxLCD 5
#define ledPin 13                // choose the pin for the LED
#define SclockPin 7              //clock for serial to parallel to serial
#define latchPin 9              // after serial light latch them out
#define SdataPin 8                // for LED
#define KeyOK 10
#define KeyESC 10
#define EncCHA 2                // choose the input pin (for a pushbutton)
#define EncCHB 3                // choose the input pin (for a pushbutton)
#define loadPin 4
#define sInDataPin 6
SoftwareSerial softserial(RxLCD, TxLCD);



//Define the LED value
#define TOP_LEFT_RED 0 //0x8000
#define TOP_LEFT_GREEN 1 //0x4000
#define TOP_RIGHT_RED 2 //0x2000
#define TOP_RIGHT_GREEN 3 //0x1000
#define BOT_LEFT_RED 4 //0x0800
#define BOT_LEFT_GREEN 5 //0x0400
#define BOT_RIGHT_RED 6 //0x0200
#define BOT_RIGHT_GREEN 7 //0x0100
#define ESC_RED 8 //0x0080
#define ESC_GREEN 9 //0x0040
#define OK_RED 10  //0x0020
#define OK_GREEN 11  //0x0010

//Define the input value
#define BOT_RIGHT 0
#define BOT_LEFT 1
#define TOP_RIGHT 2
#define TOP_LEFT 3
#define OK 6
#define ESC 7


int tempStateIn=0;
int lastState = 0;              //last state of led
int EncValue = 0;
int EncPulse = 3;             //0 = Nothing, 1= Forward, 2= reverse, 3= redraw
int EncDivider = 10;
int EncDividerCountUP = 0;
int EncDividerCountDW = 0;
int TopScreen =0;
int OldTopScreen=0;
int MenuIndex=1;
int DMXchannel=1;
int DMXintensity=1;
int FixtureNO=1;
int SequenceNO;
int SceneNO=1;
int ProgramNO=0;
int delayEntreSeq=500;
int contDelay;
int sequence1[15] = {5,6,7,1,7,2,10,3,7,3,10,5,6,0};
int indexSeq=0;
int full_dmx_channel=0;
int ModeNo=0; // 1 = DMX channel debug, 2= Fixture Mode, 3= Scene Mode, 4= Program Mode, 5 = ALL ON, 6 = ALL Off, 7 = Sequence mode
unsigned char randomDmx1Value=0;
unsigned char randomDmx2Value=0;
char randomDmx3Value=0;
int tempProgValue1;
int tempProgValue2;
int tempProgValue3;
long tempProgValueLong;
unsigned char dmx_data[50];
int readPot1;
int readPot2;
int readPot3;
int readPot4;
int readPot5;
int readPot6;

int Choix;
int TapAvg[10];
int ReadTimeTap;
int TapSyncTime;
int LastTapSyncTime;

int pinto=0;

int minMinutTimer=0;
int maxMinutTimer=0;
int randomMinutTimer=0;

/*************************************************************/
// Interrupt of the rotary encoder wheel
// It is call only on the rising edge of one of the 
// two quadrature channel.
// Going too fast while Serial Print to the LCD does not Work well

void Rotary()
{
  if (!digitalRead(EncCHB))
  { 
    // Up Direction High Speed  
    if (EncDividerCountUP++ > EncDivider)
    {
      EncDividerCountUP=0;
      EncPulse =1;
    }
  }
  else 
  {
    // Down direction  
    if (EncDividerCountDW++ > EncDivider)
    {
      EncDividerCountDW = 0;  
      EncPulse =2;
    }        
  }
}


// For later use.
void TapSync()
{
  ReadTimeTap = millis();
  //digitalWrite(ledPin,1-digitalRead(ledPin));
  TapSyncTime = ReadTimeTap-LastTapSyncTime;
  LastTapSyncTime = ReadTimeTap;
}



/*************************************************************/
void setup()
{
  // Config the LED output for the serial to paralel chip
  pinMode(SclockPin, OUTPUT);      
  pinMode(latchPin, OUTPUT);
  pinMode(SdataPin,OUTPUT);
  // Config the serial input switch
  pinMode(loadPin, OUTPUT);
  pinMode(sInDataPin,INPUT);

  //Config of the rotary encoder
  pinMode(EncCHA, INPUT);
  pinMode(EncCHB, INPUT);
  pinMode(1, OUTPUT);
  pinMode(ledPin, OUTPUT);      // declare LED as output
  pinMode(TxLCD, OUTPUT);
  pinMode(KeyOK, INPUT);
  pinMode(KeyESC, INPUT);
  digitalWrite(KeyOK, HIGH); // Turn ON the pull up resistor
  digitalWrite(KeyESC, HIGH);

  digitalWrite(TxLCD, HIGH);
  delay (100);
  softserial.begin(9600); // for LCD
  Serial.begin(9600); //For DMX out
  delay (100);
  //noInterrupts();
  softserial.write(22); // turn on LCD, No Cursor
  softserial.write(17); // turn on back light
  delay (50);
  softserial.write(12); // Clear Screen
  softserial.print(" RANDOM BOX V1.0");
  delay (500);
  // For debug
  //  softserial.write(0x0D);
  //  softserial.print(TCCR2A, HEX);

  //  To Change speed to 250Kb and 2 stop bit
  UBRR0L = 3;
  UCSR0C = UCSR0C | 0x08;

  interrupts();
  randomSeed(analogRead(0));
  attachInterrupt(0,Rotary,RISING);
  refreshExtendIn();
  testLed();
  ScrollMenu(MenuIndex);
  }


/*************************************************************/
// To print a fix value in lenght
void PrintFixValue(int Data, int Fix)
{
  noInterrupts();
  if (Fix > 1) {
    if (Data < 10)
      softserial.print("0");
  } 
  if (Fix > 2) {
    if (Data < 100)
      softserial.print("0");
  } 
  if (Fix > 3 ) {
    if (Data < 1000)
      softserial.print("0");
  } 
  if (Fix > 4  ) {
    if (Data < 10000)
      softserial.print("0");
  } 
  softserial.print(Data);
  interrupts();
}

/*************************************************************/
// To Enter a Value with the rotary wheel
// With a Text Prompt
int EnterValue(const char *Text, int Donnee, int Range)
{
  int ESCPressIN;
  int OKPressIN;
  
  int OldEncDivider = EncDivider;
  EncDivider = 1;
  noInterrupts();
  softserial.write(12); // Clear Screen
  softserial.print(Text);
  softserial.write(0x0D);
  //softserial.print("Current Value :");
  //PrintFixValue(Donnee,3);
  softserial.write(0x0D);
  softserial.print("Range : (0-");
  softserial.print(Range);
  softserial.print(")");
  softserial.write(0x0D);
  softserial.print("New Value :");
  softserial.write(0xC7);
  PrintFixValue(Donnee,3);

  do {
    interrupts();
    if (EncPulse ==1) {
      if (++Donnee>Range)
        Donnee=0;
    }
    if (EncPulse ==2) {
      if (Donnee)
        Donnee--;
      else 
        Donnee = Range;  
    }
    if (EncPulse) {
      noInterrupts();
      softserial.write(0xC7);
      PrintFixValue(Donnee, 3);
      EncPulse=0;
    }
  }
  while ((ESCPressIN = extendIn(ESC)) && (OKPressIN =extendIn(OK)));
  EncDivider = OldEncDivider;
  
  interrupts();
  if (!OKPressIN){
    return (Donnee);
  }
  else {
    return (-1);
  }
}


/*************************************************************/
// Menu function
// Move the arrow and return the new position the choice
void ScrollMenu(int Menu)
{
  noInterrupts();
  softserial.write(0x80); // Move Cursor to TOP LEFT
  if (Menu==1){
    softserial.print("1.Out20 Total Time  2.Out21 Light       3.Out22 Random      4.Out23 Sequence    ");
    turnOffMenuLed();
    turnOnGreenMenuLed();
  }
  if (Menu==2){
    softserial.print("1.Out24             2.Out26             3.Out27             4.Start Sequence    ");
    turnOffMenuLed();
    turnOnGreenMenuLed();
  }
  if (Menu==3){
    softserial.print("1.RGB STEP          2.STROBE            3.RGB SMOOTH        4.FLASH             ");
    turnOffMenuLed();
    turnOnGreenMenuLed();
  }
  if (Menu==4){
    softserial.print("1.XY MOVE RGB       2.POLICE            3.RANDOM            4.XY RANDOM         ");
    turnOffMenuLed();
    turnOnGreenMenuLed();
  }
  interrupts();
  
}

/*************************************************************/
void loop()
{
  int returnValue;

  if (full_dmx_channel == 0) //Make a break in serial
  {
    UCSR0B = UCSR0B & ~0x08;
    delayMicroseconds(120);
    pinMode(1, OUTPUT);
    digitalWrite(1,LOW);
    delayMicroseconds(12000);
    digitalWrite(1,HIGH);
    delayMicroseconds(12);
    UCSR0B = UCSR0B | 0x08;
    Serial.write(0x00);
    //ModeNo=0;
  }
  if (full_dmx_channel>0) //Not a break then write the DMX Channel
    {
    //dmx_data[full_dmx_channel]=255;  
    //Serial.write(dmx_data[full_dmx_channel]);

      if (ModeNo == 1){
        Serial.write(dmx_data[full_dmx_channel]);
      }
      else if (ModeNo == 2){
        Serial.write(dmx_data[full_dmx_channel]);
      }
      else if (ModeNo == 3){
        Serial.write(dmx_data[full_dmx_channel]);
      }
      else if (ModeNo == 4 || ModeNo == 7){
          Serial.write(dmx_data[full_dmx_channel]);
      }
      else if (ModeNo == 5){ // ALL ON mode
        Serial.write((char)(readPot1/4));
      }
      else if (ModeNo == 6){ // ALL OFF MODE
        Serial.write(0x00); //send 0x00
        dmx_data[full_dmx_channel]=0x00; // and clear the stream of residual
      }
      if (ModeNo == 10){
        Serial.write(dmx_data[full_dmx_channel]);
      }
    }
  full_dmx_channel++;
  extendOut(ESC_GREEN,HIGH); // Turn ON ESC LED GREEN
  // At the end of the transmit of the full DMX trame
  if (full_dmx_channel>50)
    {
    full_dmx_channel = 0;
    refreshExtendIn();
    readPot1=analogRead(0);
    readPot2=analogRead(1);
    readPot3=analogRead(2);
    readPot4=analogRead(3);
    readPot5=analogRead(4);
    readPot6=analogRead(5);
    digitalWrite(ledPin,1-digitalRead(ledPin));  //flash the LED to indicate refresh speed

    if (ModeNo == 1){
        dmx_data[DMXchannel]=(char)(readPot1/4);
        dmx_data[DMXchannel+1]=(char)(readPot2/4);
        dmx_data[DMXchannel+2]=(char)(readPot3/4);
        dmx_data[DMXchannel+3]=(char)(readPot4/4);
        dmx_data[DMXchannel+4]=(char)(readPot5/4);
        dmx_data[DMXchannel+5]=(char)(readPot6/4);
    }
    if (ModeNo == 2){
    }
    if (ModeNo == 3){
    }
    if (ModeNo == 4 || ModeNo == 7){
      if (ProgramNO == 1){
        randomDmx2Value = (char)(readPot4/4);
        if (randomDmx2Value < 3){
          randomDmx2Value=3;
        }
        randomDmx1Value++;
        if (randomDmx1Value>randomDmx2Value) randomDmx1Value=0;
        if (randomDmx1Value<(randomDmx2Value/3)){
          dmx_data[1]=(char)(readPot1/4);
          dmx_data[2]=0;
          dmx_data[3]=0;
        }
        if ((randomDmx1Value>(randomDmx2Value/3))&& (randomDmx1Value<((randomDmx2Value/3)*2))){
          dmx_data[1]=0;
          dmx_data[2]=(char)(readPot2/4);
          dmx_data[3]=0;
        }
        if (randomDmx1Value>((randomDmx2Value/3)*2)){
          dmx_data[1]=0;
          dmx_data[2]=0;
          dmx_data[3]=(char)(readPot3/4);
        }
      }
      if (ProgramNO == 2){
        tempProgValue2=(readPot1/4);
        dmx_data[27]=255;
        if (dmx_data[4]>0){
          dmx_data[4]=0;
        }
        if (tempProgValue1++>(readPot4/80)){
            dmx_data[4]=tempProgValue2;
            tempProgValue1=0;
        } 
      }
      if (ProgramNO == 3){
        if ((tempProgValue1 == 0 && tempProgValue2 == 0 && tempProgValue3 == 0)||(tempProgValue1 > 0 && tempProgValue2 > 0 && tempProgValue3 > 0)){
          tempProgValue1=1;
          tempProgValue2=255;
          tempProgValue3=0;
        }
        if (tempProgValue1>0&&tempProgValue2==0) {
          tempProgValue1=tempProgValue1+2;
          if (tempProgValue1>255){
            tempProgValue1=255;
            tempProgValue2=1;
          }
          tempProgValue3=tempProgValue3-2;
          if (tempProgValue3<0){
            tempProgValue3=0;
          }
        }
        if (tempProgValue2>0&&tempProgValue3==0) {
          tempProgValue2=tempProgValue2+2;
          if (tempProgValue2>255){
            tempProgValue2=255;
            tempProgValue3=1;
          }
          tempProgValue1=tempProgValue1-2;
          if (tempProgValue1<0){
            tempProgValue1=0;
          }
        }
        if (tempProgValue3>0&&tempProgValue1==0) {
          tempProgValue3=tempProgValue3+2;
          if (tempProgValue3>255){
            tempProgValue3=255;
            tempProgValue1=1;
          }
          tempProgValue2=tempProgValue2-2;
          if (tempProgValue2<0){
            tempProgValue2=0;
          }
        }
        dmx_data[1]=(char)(tempProgValue1*(readPot1/4)/256);
        dmx_data[2]=(char)(tempProgValue2*(readPot2/4)/256);
        dmx_data[3]=(char)(tempProgValue3*(readPot3/4)/256);
          
      }
      if (ProgramNO == 5){
        if (dmx_data[1]==255){
          dmx_data[1]=0;
        }
        if (tempProgValue1++>(readPot4/80)){
            dmx_data[1]=255;
            tempProgValue1=0;
        } 
      }
      if (ProgramNO == 6){
        if (dmx_data[2]==255){
          dmx_data[2]=0;
        }
        if (tempProgValue1++>(readPot4/80)){
            dmx_data[2]=255;
            tempProgValue1=0;
        } 
      }
      if (ProgramNO == 7){
        if (dmx_data[3]==255){
          dmx_data[3]=0;
        }
        if (tempProgValue1++>(readPot4/80)){
            dmx_data[3]=255;
            tempProgValue1=0;
        } 
      }
      if (ProgramNO == 8){
        if (dmx_data[4]==255){
          dmx_data[4]=0;
        }
        if (tempProgValue1++>(readPot4/80)){
            dmx_data[4]=255;
            tempProgValue1=0;
        } 
      }
      if (ProgramNO == 9){
        dmx_data[26]=255;
        tempProgValue1++;
        //softserial.print(tempProgValue1,DEC);
        //softserial.print(",");
        if (tempProgValue1<10){
            dmx_data[1]=255;
            dmx_data[10]=255;
            dmx_data[2]=0;
            dmx_data[3]=0;
            dmx_data[4]=0;
            dmx_data[11]=0;
            dmx_data[12]=0;
        } 
        if (tempProgValue1>10){
            dmx_data[1]=0;
            dmx_data[10]=0;
            dmx_data[2]=0;
            dmx_data[3]=255;
            dmx_data[4]=0;
            dmx_data[11]=0;
            dmx_data[12]=255;
       
        } 
        if (tempProgValue1>20){
            tempProgValue1=0;
        } 

      }
      if (ProgramNO == 10){//RANDOM
        tempProgValue2=(readPot1/4);
        if (tempProgValue1++>(readPot4/80)){
          tempProgValue1=0;
          tempProgValueLong = random(8);
            dmx_data[1]=0;
            dmx_data[2]=0;
            dmx_data[3]=0;
            dmx_data[4]=0;
            dmx_data[10]=0;
            dmx_data[11]=0;
            dmx_data[12]=0;

          if (tempProgValueLong == 0){
            dmx_data[1]=tempProgValue2;
            
          }
          if (tempProgValueLong == 1){
            dmx_data[2]=tempProgValue2;
          }
          if (tempProgValueLong == 2){
            dmx_data[3]=tempProgValue2;
          }
          if (tempProgValueLong == 3){
            dmx_data[4]=tempProgValue2;
          }
          if (tempProgValueLong == 4){
            dmx_data[10]=tempProgValue2;
          }
          if (tempProgValueLong == 5){
            dmx_data[11]=tempProgValue2;
          }
          if (tempProgValueLong == 6){
            dmx_data[12]=tempProgValue2;
          }
        }
      }
      if (ProgramNO == 11){ //FLASH
        tempProgValue1=(readPot1/4);
        dmx_data[1]=0;
        dmx_data[2]=0;
        dmx_data[3]=0;
        dmx_data[4]=0;
        dmx_data[10]=0;
        dmx_data[11]=0;
        dmx_data[12]=0;

        if (!extendInNotNow(TOP_LEFT)){
            dmx_data[1]=tempProgValue1;
            dmx_data[10]=tempProgValue1;
          }
        if (!extendInNotNow(TOP_RIGHT)){
            dmx_data[2]=tempProgValue1;
            dmx_data[11]=tempProgValue1;
          }
        if (!extendInNotNow(BOT_LEFT)){
            dmx_data[3]=tempProgValue1;
            dmx_data[12]=tempProgValue1;
          }
        if (!extendInNotNow(BOT_RIGHT)){
            dmx_data[4]=tempProgValue1;
            dmx_data[10]=tempProgValue1;
            dmx_data[11]=tempProgValue1;
            dmx_data[12]=tempProgValue1;
        }  
      }
      if (ProgramNO == 12){
        tempProgValue2=(readPot1/4);
        if (tempProgValue1++>(readPot4/80)){
          tempProgValue1=0;
          tempProgValueLong = random(3);
            dmx_data[1]=0;
            dmx_data[2]=0;
            dmx_data[3]=0;
            dmx_data[4]=0;
            dmx_data[10]=0;
            dmx_data[11]=0;
            dmx_data[12]=0;

          if (tempProgValueLong == 0){
            dmx_data[10]=tempProgValue2;
          }
          if (tempProgValueLong == 1){
            dmx_data[11]=tempProgValue2;
          }
          if (tempProgValueLong == 2){
            dmx_data[12]=tempProgValue2;
          }
        }
      }
      if (ProgramNO == 13){
        dmx_data[24]=255;
      }
    }
    if (ModeNo == 5){
    }
    if (ModeNo == 6){
    }
    if (ModeNo == 7){
      if (SequenceNO == 1){
        //ProgramNO=sequence1[indexSeq];
        contDelay++;
        if (contDelay>delayEntreSeq){
          clearDmxData();
          contDelay =0;
          indexSeq++;
          if (!sequence1[indexSeq])
            indexSeq=0;
          ProgramNO=sequence1[indexSeq];
          
        }
      }
    }
    if (ModeNo == 8){
      //softserial.print("                                                                                ");
      softserial.print(randomMinutTimer);
    }
    if (ModeNo != 1){ //ModeNo == 10 avant
      if (ModeNo==10){
        dmx_data[10]=(unsigned char)(readPot1/4);
        dmx_data[11]=(unsigned char)(readPot2/4);
        dmx_data[12]=(unsigned char)(readPot3/4);
        dmx_data[13]=(unsigned char)(readPot4/4);
      }

      if (dmx_data[14]<253){
        if ((readPot5)>600) {
          dmx_data[14]=dmx_data[14] + 1;
        }
        if ((readPot5)>750) {
          dmx_data[14]=dmx_data[14] + 1;
        }
      }
      if (dmx_data[14]>2){
        if ((readPot5)<350) {
          dmx_data[14]=dmx_data[14] - 1;
        }
        if ((readPot5)<200) {
          dmx_data[14]=dmx_data[14] - 1;
        }
      }
      if (dmx_data[15]<253){
        if ((readPot6)>600) {
          dmx_data[15]=dmx_data[15] + 1;
        }
        if ((readPot6)>750) {
          dmx_data[15]=dmx_data[15] + 1;
        }
      }
      if (dmx_data[15]>2){
        if ((readPot6)<350) {
          dmx_data[15]=dmx_data[15] - 1;
        }
        if ((readPot6)<200) {
          dmx_data[15]=dmx_data[15] - 1;
        }
      }
      //softserial.print(dmx_data[15],DEC);
      //softserial.print(",");
    }

  }
  if (!extendInNotNow(ESC))
  {
    // Each time pressing ESC Change Menu
    MenuIndex++;
    if (MenuIndex>4)
      MenuIndex=1;
    ScrollMenu(MenuIndex);
    while (!extendIn(ESC));
    refreshExtendIn();
  }
  if (MenuIndex != 100){
  if (!extendInNotNow(TOP_LEFT))
  {
    clearDmxData();
    if (MenuIndex == 1)
    {
      turnOffMenuLed();
      extendOut(OK_GREEN,HIGH); //OK GREEN LED ON
      returnValue = EnterValue("Min Minute Timer", minMinutTimer, 60);
      if (returnValue != -1){
        minMinutTimer = returnValue;
        while (!extendIn(OK));
        ModeNo =1;
      }
      returnValue = EnterValue("Max Minute Timer", maxMinutTimer, 60);
      if (returnValue != -1){
        maxMinutTimer = returnValue;
        while (!extendIn(OK));
        ModeNo =1;
      }
      turnOnGreenMenuLed();
      extendOut(OK_GREEN,LOW); //OK GREEN LED OFF
    ScrollMenu(MenuIndex);
    randomMinutTimer = random(minMinutTimer,maxMinutTimer);
    }
    if (MenuIndex == 2)
    {
      noInterrupts();
      softserial.print("P1.Intensity        P2.                 P3.                 P4.                 ");
      interrupts();

      ModeNo = 5;
    }
    if (MenuIndex == 3)
    {
      ProgramNO = 1;
      ModeNo=4;
    }
    if (MenuIndex == 4)
    {
      //temp = EnterValue("DMX Channel", DMXchannel, 512);
      //if (temp != -1){
      //  DMXchannel = temp;
      //  while (!extendIn(OK));
      //}   
      noInterrupts();
      softserial.print("P1.RED              P2.GREEN            P3.BLUE      JOY X  P4.          JOY Y  ");
      interrupts();

        
      ModeNo=10;
    }
        while (!extendIn(TOP_LEFT));
        refreshExtendIn();
    
  }

  if (!extendInNotNow(TOP_RIGHT))
  {
    clearDmxData();
    if (MenuIndex == 1)
    {
      returnValue = EnterValue("Fixture", FixtureNO, 127);
      if (returnValue != -1)
        FixtureNO = returnValue;
      
    ScrollMenu(MenuIndex);
    }
    if (MenuIndex == 2)
    {
      ModeNo = 6;
    }
    if (MenuIndex == 3)
    {
      ProgramNO = 2;
      ModeNo=4;
    }
    if (MenuIndex == 4)
    {
      ModeNo = 4;
      ProgramNO = 9;
    }
      while (!extendIn(TOP_RIGHT));
    refreshExtendIn();
    
  }

  if (!extendInNotNow(BOT_LEFT))
  {
    clearDmxData();
    if (MenuIndex == 1)
    {
    }
    if (MenuIndex == 2)
    {
      returnValue = EnterValue("Sequence", SequenceNO, 127);
      if (returnValue != -1)
        SequenceNO = returnValue;
      ModeNo=7;
      ScrollMenu(MenuIndex);
    }
    if (MenuIndex == 3)
    {
      noInterrupts();
      softserial.print("P1.Red              P2.Green            P3.Blue             P4.                 ");
      interrupts();
      ProgramNO = 3;
      ModeNo=4;
    }
    if (MenuIndex == 4)
    {
      noInterrupts();
      softserial.print("P1.Intensity        P2.                 P3.                 P4.Speed            ");
      interrupts();
      ProgramNO = 10; //Random
      ModeNo=4;
    }
    while (!extendIn(BOT_LEFT));
    refreshExtendIn();
  }

  if (!extendInNotNow(BOT_RIGHT))
  {
    clearDmxData();
    if (MenuIndex == 1)
    {
      returnValue = EnterValue("Program", ProgramNO, 127);
      if (returnValue != -1)
        ProgramNO = returnValue;
      ModeNo=4;
      ScrollMenu(MenuIndex);

    }
    if (MenuIndex == 2)
    {
      ProgramNO = 13;
      ModeNo=8;
    }
    if (MenuIndex == 3)
    {
      noInterrupts();
      softserial.print("TOP L. RED P1.INTENSTOP R. GREEN        BOT L. BLUE         BOT R. WHITE        ");
      interrupts();
      ProgramNO = 11;
      ModeNo=4;
      MenuIndex = 100;
    }
    if (MenuIndex == 4)
    {
      noInterrupts();
      softserial.print("P1.Intensity        P2.                 P3.                 P4.Speed            ");
      interrupts();
      ProgramNO = 12;
      ModeNo = 4;
    }
    while (!extendIn(BOT_RIGHT));
    refreshExtendIn();
  }
  }
}

/*************************************************************/
void extendOut(int output, int state)
{
  int tempState;
  tempState=0x01;
  for (int i=0;i<16;i++){
    if ((15-i)==output){
      digitalWrite(SdataPin, state);
      if (state == HIGH ){
        lastState |= tempState;
      }
      else {
        lastState &= ~tempState;
      }
    }
    else {
      if (lastState & tempState){
        digitalWrite(SdataPin, HIGH );
      }
      else{
        digitalWrite(SdataPin, LOW);
      }
    }

    tempState = tempState<<1;
    digitalWrite(SclockPin,HIGH);
    digitalWrite(SclockPin,LOW);
  }
  digitalWrite(latchPin,HIGH);
  digitalWrite(latchPin,LOW);
 
}

/*************************************************************/
int extendIn(int sinput)
{
  int tempSerialIn, poutput;
  noInterrupts();
  digitalWrite(loadPin,LOW);
    delayMicroseconds(100);
  digitalWrite(loadPin,HIGH);
    delayMicroseconds(100);
  for (int i=0;i<8;i++){

    tempSerialIn = digitalRead (sInDataPin);
    if (sinput == i){
      if (tempSerialIn == HIGH){
        poutput = HIGH;
      }
      else {
        poutput = LOW;
      }

    }
    digitalWrite(SclockPin,HIGH);
    delayMicroseconds(100);
    digitalWrite(SclockPin,LOW);
    delayMicroseconds(100);

  }
  interrupts();

  return poutput;
}


/*************************************************************/

int extendInNotNow(int sinput)
{
  int poutput, tempSerialIn,tempMask=0x00000001;
  for (int i=0;i<8;i++){
    tempSerialIn = tempStateIn&tempMask;
    if ((7-sinput) == i){
      if (tempSerialIn){
        poutput = HIGH;
      }
      else {
        poutput = LOW;
      }
    }
    tempMask=tempMask<<1;
  }
  return poutput;
}


/*************************************************************/
void refreshExtendIn()
{
  int tempSerialIn, poutput;
  digitalWrite(loadPin,LOW);
  delayMicroseconds(100);
  digitalWrite(loadPin,HIGH);
  delayMicroseconds(100);
  for (int i=0;i<8;i++){
    tempStateIn = tempStateIn<<1;
    tempSerialIn = digitalRead (sInDataPin);
    if (tempSerialIn == HIGH){
      tempStateIn = tempStateIn|1;
    }
    else {
      tempStateIn = tempStateIn&0xFFFFFFFE;
    }
    digitalWrite(SclockPin,HIGH);
    delayMicroseconds(100);
    digitalWrite(SclockPin,LOW);
  }
}

   
/*************************************************************/
void extendOutNotNow(int output, int state)
{
  int tempState;
  tempState=0x01;
  for (int i=0;i<16;i++){
    if ((15-i)==output){
      //digitalWrite(SdataPin, state);
      if (state == HIGH ){
        lastState |= tempState;
      }
      else {
        lastState &= ~tempState;
      }
    }
    /*else {
      if (lastState & tempState){
        digitalWrite(SdataPin, HIGH );
      }
      else{
        digitalWrite(SdataPin, LOW);
      }
    }
    */

    tempState = tempState<<1;
    //digitalWrite(SclockPin,HIGH);
    //digitalWrite(SclockPin,LOW);
  }
  //digitalWrite(latchPin,HIGH);
  //digitalWrite(latchPin,LOW);
 
}

/*************************************************************/
void refreshExtendOut()
{
  int tempState;
  tempState=0x01;
  for (int i=0;i<16;i++){
    if (lastState & tempState){
      digitalWrite(SdataPin, HIGH );
    }
    else{
      digitalWrite(SdataPin, LOW);
    }
  

    tempState = tempState<<1;
    delayMicroseconds(100);
    digitalWrite(SclockPin,HIGH);
    delayMicroseconds(100);
    digitalWrite(SclockPin,LOW);
    delayMicroseconds(100);
  }
  digitalWrite(latchPin,HIGH);
    delayMicroseconds(100);
  digitalWrite(latchPin,LOW);
 
}

/*************************************************************/
void testLed()
{
  for (int i=0;i<12;i++)
    extendOut(i,LOW);
  for (int i=0;i<12;i++)
  {
    extendOut(i,HIGH);
    delay(100);
  }
  for (int i=0;i<12;i++)
    extendOut(i,LOW);
}

/*************************************************************/
void turnOffMenuLed()
{
  for (int i=0;i<8;i++)
    extendOut(i,LOW);
}

void turnOnGreenMenuLed()
{
  for (int i=0;i<8;i++)
    if (i%2)
      extendOut(i,HIGH);
}

void clearDmxData()
{
  for (int i=0;i<48;i++){
    if (i!=14&&i!=15){
      dmx_data[i]=0;
    }
  }
}
