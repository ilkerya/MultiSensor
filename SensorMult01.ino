#include <Adafruit_SSD1306.h>
#include "SparkFun_Si7021_Breakout_Library.h"
#include <MutichannelGasSensor.h>
#include <MICS-VZ-89TE.h>
#include <Wire.h>
#include "utility/twi.h"

#define TCAADDR 0x71
#define CHANNEL_VOC 2
#define CHANNEL_TEMP 7
#define CHANNEL_GAS 6

void tcaselect(uint8_t i) {
  if (i > 7) return;
  Wire.beginTransmission(TCAADDR);
    //dafruit Industries
    //  https://learn.adafruit.com/adafruit-tca9548a-1-to-8-i2c-multiplexerbreakout
    Wire.write(1 << i);
    Wire.endTransmission();
    delay(20);
}


MICS_VZ_89TE VOCSensor;
Weather Si072_Sensor;

#define SensorAnalogPin A2  //this pin read the analog voltage from the HCHO sensor
#define VREF  5.0   //voltage on AREF pin


//DISPLAY CONSTANTS
#define OLED_MOSI   9
#define OLED_CLK   10
#define OLED_DC    11
#define OLED_CS    13
#define OLED_RESET 12

//Adafruit_SSD1306 display(OLED_RESET);
//DISPLAY INITIALIZER
Adafruit_SSD1306 display(OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);

#define NUMFLAKES 10
#define XPOS 0
#define YPOS 1
#define DELTAY 2

#define LOGO16_GLCD_HEIGHT 16 
#define LOGO16_GLCD_WIDTH  16 
static const unsigned char PROGMEM logo16_glcd_bmp[] =
{ B00000000, B11000000,
  B00000001, B11000000,
  B00000001, B11000000,
  B00000011, B11100000,
  B11110011, B11100000,
  B11111110, B11111000,
  B01111110, B11111111,
  B00110011, B10011111,
  B00011111, B11111100,
  B00001101, B01110000,
  B00011011, B10100000,
  B00111111, B11100000,
  B00111111, B11110000,
  B01111100, B11110000,
  B01110000, B01110000,
  B00000000, B00110000 };

#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif


char StrAmm[]         = "Ammonia(NH3): ";
char StrCO[]          = "Carbon Mon(CO): ";
char StrNO2[]         = "Nit.Dioxide(NO2): ";
char StrPropane[]     = "Propane(C3H8): ";
char StrIsoButane[]   = "IsoButane(C4H10): ";
char StrMethane[]     = "Methane(CH4): ";
char StrHydrogane[]   = "Hydrogen(H2): ";
char StrEthanol[]     = "Ethanol(C2H5OH): ";
char StrCO2[]         = "CarbonDioxide(CO2): ";

char StrFormaldehy[]  = "Formald.(HCHO): ";

/*
char SStrAmm[]         = "NH3:";
char SStrCO[]          = "CO:";
char SStrNO2[]         = "NO:";
char SStrPropane[]     = "C3H8:";
char SStrIsoButane[]   = "C4H10:";
char SStrMethane[]     = "CH4:";
char SStrHydrogane[]   = "H2:";
char SStrEthanol[]     = "C2H5OH:";
char SStrFormaldehy[]  = "HCHO:";
*/

unsigned int PreheatTimer = 0;

bool Preheat = false;

unsigned int Timeout;

struct 
{
  float Ammonia;
  float CarbonMonoxide;
  float NitrogenDioxide;
  float Propane;
  float IsoButane;
  float Methane;
  float Hyidrogen;
  float Ethanol;
  float Formaldehyd;
  int CO2;
  float eCO2;
  float VOC;
  float Temperature;
  float Humidity;
}Values;
#define MAX_AMMONIA 500
#define MIN_AMMONIA 1

#define MAX_CO 1000
#define MIN_CO 1

#define MAX_NO2 10
#define MIN_NO2 0.05

#define MAX_PROPANE 15000
#define MIN_PROPANE 3000

#define MAX_ISOBUTANE 15000
#define MIN_ISOBUTANE 3000

#define MAX_METHANE 15000
#define MIN_METHANE 3000

#define MAX_HYDROGEN 250
#define MIN_HYDROGEN 1

#define MAX_ETHANOL 500
#define MIN_ETHANOL 10

#define MAX_FORMALDEHYDE 5
#define MIN_FORMALDEHYDE 0

#define MAX_CO2 3000
#define MIN_CO2 200

byte readCO2_S8SenseAir[] = {0xFE, 0X44, 0X00, 0X08, 0X02, 0X9F, 0X25};  //Command packet to read Co2 (see app note)
byte response_S8SenseAir[] = {0,0,0,0,0,0,0};  //create an array to store the response

void sendRequest(byte packet[]) {
      Serial1.begin(9600);
      delay(10);
    while(!Serial1.available())  //keep sending request until we start to get a response
    {
      Serial1.write(readCO2_S8SenseAir,7);
      delay(50);
  }
    int timeout=0;  //set a timeoute counter
    while(Serial1.available() < 7 ) //Wait to get a 7 byte response
    {
      timeout++;
      if(timeout > 10)    //if it takes to long there was probably an error
        {
          while(Serial1.available())  //flush whatever we have
            Serial1.read();

            break;                        //exit and try again
        }
        delay(50);
    }

    for (int i=0; i < 7; i++)
    {
      response_S8SenseAir[i] = Serial1.read();
    }
}
/*
unsigned int getValue_S8SenseAir(byte packet[])
{

   unsigned int val = packet[3]*256 + packet[4];                //Combine high byte and low byte with this formula to get value
  return val;

}
*/
unsigned int readCO2SenseAir(void) {
  sendRequest(readCO2_S8SenseAir);
 // Values.CO2 = getValue_S8SenseAir(response_S8SenseAir);

  return (response_S8SenseAir[3]*256 + response_S8SenseAir[4]);                //Combine high byte and low byte with this formula to get value
  
}

void setup() {
  // put your setup code here, to run once:
    Serial.begin(115200);  // start serial for output
    delay(10);

  //-- DISPLAY INIT --//

  display.begin(SSD1306_SWITCHCAPVCC);
  display.clearDisplay();

  display.setTextSize(3);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println();
  display.println("OpenAir");
  display.display();

    Wire.begin();
     delay(20);
     
    tcaselect(CHANNEL_GAS);
      Serial.println("power on!");
    gas.begin(0x04);//the default I2C address of the slave is 0x04
    gas.powerOn();
    Serial.print("Firmware Version = ");
    Serial.println(gas.getVersion());

      tcaselect(CHANNEL_VOC);
    VOCSensor.begin();
    VOCSensor.getVersion();
  //  Serial.println(data[0]);
 //  Serial.println(data[1]);
  //  Serial.println(data[2]);

  tcaselect(CHANNEL_TEMP);
  Si072_Sensor.begin();
    
    
}
      
void loop() {
      // http://wiki.seeed.cc/Grove-Multichannel_Gas_Sensor/
      // https://www.dfrobot.com/product-1574.html
      // https://www.mouser.se/datasheet/2/18/MiCS-VZ-89TE-V1.0-1100483.pdf

    tcaselect(CHANNEL_TEMP); 


     Values.Humidity = Si072_Sensor.getRH();
      delay(10);
     Values.Temperature = Si072_Sensor.readTemp();

    Serial.print("Temp:");
    Serial.print(Values.Temperature);
    Serial.print("C  ");

    Serial.print("Humidity:");
    Serial.print(Values.Humidity);
    Serial.println("%");
     
    if (Values.Temperature < -35){
      Serial.println("Error");
      delay(100);
      Si072_Sensor.begin();
      delay(100); 
    }

 
      tcaselect(CHANNEL_GAS);
      gas.begin(0x04);//the default I2C address of the slave is 0x04

  Values.Ammonia =  gas.measure_NH3();
  if( Values.Ammonia > MAX_AMMONIA ) Values.Ammonia = MAX_AMMONIA;
  if( Values.Ammonia < MIN_AMMONIA ) Values.Ammonia = MIN_AMMONIA;
  
  Values.CarbonMonoxide = gas.measure_CO();
  if( Values.CarbonMonoxide > MAX_CO ) Values.CarbonMonoxide = MAX_CO;
  if( Values.CarbonMonoxide < MIN_CO ) Values.CarbonMonoxide = MIN_CO;
  
  Values.NitrogenDioxide = gas.measure_NO2();
    if( Values.NitrogenDioxide > MAX_NO2 ) Values.NitrogenDioxide = MAX_NO2;
    if( Values.NitrogenDioxide < MIN_NO2 ) Values.NitrogenDioxide = MIN_NO2;  
  
  Values.Propane = gas.measure_C3H8();
    if( Values.Propane > MAX_PROPANE ) Values.Propane = MAX_PROPANE;
    if( Values.Propane < MIN_PROPANE ) Values.Propane = MIN_PROPANE;
  
  Values.IsoButane = gas.measure_C4H10();
    if( Values.IsoButane > MAX_ISOBUTANE ) Values.IsoButane = MAX_ISOBUTANE;
    if( Values.IsoButane < MIN_ISOBUTANE ) Values.IsoButane = MIN_ISOBUTANE; 
  
 // delay(100);
  
  Values.Methane = gas.measure_CH4();
     if( Values.Methane > MAX_METHANE ) Values.Methane = MAX_METHANE;
    if( Values.Methane < MIN_METHANE ) Values.Methane = MIN_METHANE; 
  
  Values.Hyidrogen = gas.measure_H2();
     if( Values.Hyidrogen > MAX_HYDROGEN ) Values.Hyidrogen = MAX_HYDROGEN;
    if( Values.Hyidrogen < MIN_HYDROGEN ) Values.Hyidrogen = MIN_HYDROGEN;  

  Values.Ethanol = gas.measure_C2H5OH();
       if( Values.Ethanol > MAX_ETHANOL ) Values.Ethanol = MAX_ETHANOL;
    if( Values.Ethanol < MIN_ETHANOL ) Values.Ethanol = MIN_ETHANOL; 
  
  Values.Formaldehyd = analogReadPPM(); 
    if( Values.Formaldehyd > MAX_FORMALDEHYDE ) Values.Formaldehyd = MAX_FORMALDEHYDE;
    if( Values.Formaldehyd < MIN_FORMALDEHYDE ) Values.Formaldehyd = MIN_FORMALDEHYDE;

  Values.CO2 = readCO2SenseAir(); 
    if( Values.CO2 > MAX_CO2 ) Values.CO2 = MAX_CO2;
    if( Values.CO2 < MIN_CO2 ) Values.CO2 = MIN_CO2;

//     gas.reset();


  
    delay(1000);


    PrintVal(Values.Ammonia,&StrAmm[0], MAX_AMMONIA, MIN_AMMONIA);   
    PrintVal(Values.CarbonMonoxide,&StrCO[0], MAX_CO, MIN_CO);      
    PrintVal(Values.NitrogenDioxide,&StrNO2[0],MAX_NO2 ,MIN_NO2 );    
    PrintVal(Values.Propane,&StrPropane[0],MAX_PROPANE ,MIN_PROPANE );    
    PrintVal(Values.IsoButane,&StrIsoButane[0],MAX_ISOBUTANE ,MIN_ISOBUTANE );
    PrintVal(Values.Methane,&StrMethane[0], MAX_METHANE, MIN_METHANE); 
    PrintVal(Values.Hyidrogen,&StrHydrogane[0], MAX_HYDROGEN, MIN_HYDROGEN); 
    PrintVal(Values.Ethanol, &StrEthanol[0],MAX_ETHANOL, MIN_ETHANOL);   
    PrintVal(Values.Formaldehyd, &StrFormaldehy[0],MAX_FORMALDEHYDE, MIN_FORMALDEHYDE);
    PrintVal(Values.CO2, &StrCO2[0],MAX_CO2, MIN_CO2);


  

      tcaselect(CHANNEL_VOC);
      VOCSensor.begin();

    VOCSensor.getVersion();

    VOCSensor.readSensor();

      Values.eCO2 = VOCSensor.getCO2();   
      Values.VOC = VOCSensor.getVOC();

   

         Serial.print("VOC:"); 
         Serial.print(Values.VOC);
         Serial.print("    eCO2:"); 
         Serial.println(Values.eCO2);         

           
      Serial.print("Status:"); 

      float Status = VOCSensor.getStatus();
      Serial.println(Status);
      

    if(Preheat == false)Serial.println("Not Calibrated!");     
    if(Preheat == true)Serial.println("Calibrated!");  

    Serial.print("PreHeatTimer: "); 
     Serial.print((PreheatTimer*25)/10);   
     Serial.print("Sec      RawTimer:"); 
     Serial.println(PreheatTimer);   

     Serial.print("Timer:");Serial.print(Timeout);


   // ShowRawDAta();
       
    Serial.println(" ");
    
        displayValues(); 
        /*
            delay(1000);
            display.clearDisplay();
            display.setTextSize(1);
             display.setCursor(0, 1);
            display.print("  ");
            display.display();
            */
            
        
    delay(2500);

    Timeout++;

    
    PreheatTimer++;
    if(PreheatTimer  == 480 && (Preheat == false) ){
      displayValues(); 
      Preheat = true;  
      /*
      Serial.println("Begin to calibrate...");
      gas.doCalibrate();
      Serial.println("Calibration ok"); 
      gas.display_eeprom();  
      */ 
    }


}

void PrintVal(float Val, char *p, unsigned int Max, unsigned int Min){
  long percent;
  Serial.print(p);
   if(Val > 0){
      Serial.print(Val);
      Serial.print(" ppm");
  
      if(Val < Min ) Val = Min;
      if(Val > Max ) Val = Max;   
      percent = (long)(Val - Min) * 100;
      percent /= (Max - Min); 
    //  Serial.print(Val);
    //  Serial.print(" ppm");

      Serial.print("  // Range: ");
      Serial.print(Min);      
      Serial.print("-");
      Serial.print(Max);
      Serial.print(" ppm");
      Serial.print("   %");
      Serial.print(percent);
      Serial.println();
    }
    else Serial.println("invalid");
}

float analogReadPPM()
{
   float analogVoltage = analogRead(SensorAnalogPin) / 1024.0 * VREF;
   float ppm = 3.125 * analogVoltage - 1.25;  //linear relationship(0.4V for 0 ppm and 2V for 5ppm)
   if(ppm<0)  ppm=0;
   else if(ppm>5)  ppm = 5;
   return ppm;
}

void ShowRawDAta(){
    float R0_NH3, R0_CO, R0_NO2;
    float Rs_NH3, Rs_CO, Rs_NO2;
    float ratio_NH3, ratio_CO, ratio_NO2;
    
    R0_NH3 = gas.getR0(0);
    R0_CO  = gas.getR0(1);
    R0_NO2 = gas.getR0(2);
    
    Rs_NH3 = gas.getRs(0);
    Rs_CO  = gas.getRs(1);
    Rs_NO2 = gas.getRs(2);
    
    ratio_NH3 = Rs_NH3/R0_NH3;
    ratio_CO  = Rs_CO/R0_CO;
    ratio_NO2 = Rs_NH3/R0_NO2;
    
    Serial.println("R0:");
    Serial.print(R0_NH3);
    Serial.print('\t');
    Serial.print(R0_CO);
    Serial.print('\t');
    Serial.println(R0_NO2);
    
    Serial.println("Rs:");
    Serial.print(Rs_NH3);
    Serial.print('\t');
    Serial.print(Rs_CO);
    Serial.print('\t');
    Serial.println(Rs_NO2);
    
    Serial.println("ratio:");
    Serial.print(ratio_NH3);
    Serial.print('\t');
    Serial.print(ratio_CO);
    Serial.print('\t');
    Serial.println(ratio_NO2);
}



int buttonCounter = 0;


void displayValues(void)
{
  display.clearDisplay();

  display.setTextSize(1);  

        display.setCursor(0, 1);
    //display.print("->");
        
      display.print("Meth:");display.print((unsigned int)Values.Methane); //5/5
     //   display.setCursor(0, 12);
        display.print(" CO:");display.println((unsigned int)Values.CarbonMonoxide);

   //       display.setCursor(1, 1);
      display.print("Prop:");display.print((unsigned int)Values.Propane);  
    //  display.setCursor(1, 12);
        display.print(" NO2:");display.println(Values.NitrogenDioxide);
      
      display.print("IsBt:");display.print((unsigned int)Values.IsoButane); 
        display.print(" Ethn:");display.println((unsigned int)Values.Ethanol);  
                    
      display.print("Hyd:");display.print((unsigned int)Values.Hyidrogen);
        display.print(" Ammn:");display.println((unsigned int)Values.Ammonia);  
         
      display.print("FormAld:");display.print(Values.Formaldehyd);   
      display.print(" CO2:");display.println(Values.CO2); 

      display.print("VOC:");display.print((unsigned int)Values.VOC);
        display.print(" eCO2:");display.println((unsigned int)Values.eCO2);  

      //display.print("Temp:");
      display.print(Values.Temperature);
      display.print("C   ");
      //display.print("C  Hum:");     
      display.print(Values.Humidity);display.println("% ");  

      display.print("Timer:");display.print(Timeout);

      
                     
/*
    display.print("PM 2.5: "); display.print("DD"); display.println(" ug/m3");
    display.print("PM 10: "); display.print("DD"); display.println(" ug/m3");
    display.print("CO2: "); display.print("DD"); display.println(" ppm");
    display.print("TVOC: "); display.print("DD"); display.println(" ppb");
    display.print("HCHO: "); display.print("DD"); display.println(" ppm");
*/
  /*
  display.print("Device Id: "); display.println(deviceId);
  
  if((taskCounter == 61 || (taskCounter >= 0 && taskCounter <= 29)) && !responseRecived)
  display.println("Sending sample...");

  else if(responseRecived && dataPosted && taskCounter <= 35 && startUpFlag)
  display.println("Sample received!");

  else if(startUpFlag && ((!responseRecived && taskCounter <= 32) || (responseRecived && !dataPosted)))
  display.println("Delivery failed!");

  else
  {
    display.print("Next sample in: "); display.print(61 - taskCounter - 1); display.println(" s");
  }
  */
  display.display();
}


