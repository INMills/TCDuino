#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <Adafruit_ADS1015.h>
#include <MCUFRIEND_kbv.h>
MCUFRIEND_kbv tft;
#include <TouchScreen.h>

//constructs an instance of the 1115 16-bit ADC
  Adafruit_ADS1115 ads;

//SD Card
  //** UNO:  MOSI - pin 11, MISO - pin 12, CLK - pin 13, CS - pin 4 (CS pin can be changed) and pin #10 (SS) must be an output
  const int chipSelect = 4;
  File dataFile;
  //The pellistor takes 10-15 minutes to warm up, so we'll start with datalogging off.
  //Users will need to press start once the TCD output has stabilized on the screen.
  //0=press start to begin writing data, 1=start writing all data immediately
  int WRITE = 0; 

//Display
//  MCUFRIEND_kbv tft;
  #define MINPRESSURE 10
  #define MAXPRESSURE 1000
  // ALL Touch panels and wiring are DIFFERENT
  // Run TouchScreen_Calibr_native.ino from mcufriend, then copy and paste results below:
  const int XP=8,XM=A2,YP=A3,YM=9; //240x320 ID=0x9341
  const int TS_LEFT=113,TS_RT=896,TS_TOP=77,TS_BOT=900;
  TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);
  //Name the buttons
  Adafruit_GFX_Button start_btn, pause_btn;
  //Set up touch location sensing for touchscreen
  int pixel_x, pixel_y;     //Touch_getXY() updates global vars
  bool Touch_getXY(void)
    {
    TSPoint p = ts.getPoint();
    pinMode(YP, OUTPUT);      //restore shared pins
    pinMode(XM, OUTPUT);
    digitalWrite(YP, HIGH);   //because TFT control pins
    digitalWrite(XM, HIGH);
    bool pressed = (p.z > MINPRESSURE && p.z < MAXPRESSURE);
    if (pressed) {
        pixel_x = map(p.x, TS_LEFT, TS_RT, 0, tft.width()); //.kbv makes sense to me
        pixel_y = map(p.y, TS_TOP, TS_BOT, 0, tft.height());
        }
    return pressed;
    }
  // Assign human-readable names to some common 16-bit color values:
  #define	BLACK   0x0000
  #define	BLUE    0x001F
  #define	RED     0xF800
  #define	GREEN   0x07E0
  #define CYAN    0x07FF
  #define MAGENTA 0xF81F
  #define YELLOW  0xFFE0
  #define WHITE   0xFFFF

void setup(void) {
  uint16_t ID = tft.readID();
  
  Serial.begin(9600);
  Serial.println("Starting");
  Serial.print("Initializing SD card...");

//Initialize Display
tft.begin();

//SD Card
  //Specify SS pin as output
  pinMode(SS, OUTPUT);
//Initialize SD card and set up data file
  if (!SD.begin(chipSelect)) {
    //if it fails, TFT displays an error and program waits forever
    tft.fillScreen(BLACK);
    tft.setCursor(0, 50);
    tft.setTextColor(WHITE);    
    tft.setTextSize(3);
    tft.print("SD UNREADABLE");
    while (1) ;
  }
  Serial.println("card initialized.");
  //Creates or Opens the data log
  dataFile = SD.open("datalog.txt", FILE_WRITE);
      if (! dataFile) {
      Serial.println("error opening datalog.txt");
    //if the file and/or SD isn't available, TFT displays an error and program waits forever
      tft.fillScreen(BLACK);
      tft.setCursor(0, 50);
      tft.setTextColor(WHITE);    
      tft.setTextSize(3);
      tft.print("ERROR OPENING DATAFILE");
      while (1) ;
  }
  //Here are the column headers; the file will be CSV delimited
    dataFile.print("Time (ms)");
    dataFile.print(",");
    dataFile.println("TCD Sensor (mV)");
    dataFile.close();

//Initialize ADC; default address is 0x48 if ADDR->GND
  ads.begin();

//Draw Statics on Display
  tft.fillScreen(BLACK);
  tft.fillRect(0, 0, 240, 50, WHITE);
    tft.fillRect(4, 4, 232, 42, BLACK);
    tft.setCursor(7, 15);
    tft.setTextColor(WHITE);    
    tft.setTextSize(3);
    tft.print("Time=");
    tft.setCursor(105, 15);
    tft.print("0");
    tft.setCursor(220,15);
    tft.println("s");
  tft.fillRect(0, 80, 240, 50, BLUE);
    tft.fillRect(4, 84, 232, 42, BLACK);
    tft.setCursor(7, 95);
    tft.setTextColor(WHITE);    
    tft.setTextSize(3);
    tft.println("TCD=");
    tft.setCursor(200, 95);
    tft.println("mV");
  tft.fillRect(0, 160, 240, 160, CYAN);
    tft.fillRect(4, 164, 232, 152, BLACK);
    tft.setCursor(7, 175);
    tft.setTextColor(WHITE);    
    tft.setTextSize(3);
    tft.println("Status:");
    tft.setTextSize(2);
    tft.setCursor(40, 220);
    //Check SD default write status and return appropriate status
    if (WRITE>0) {
        tft.print("Writing to SD");
        }
    if (WRITE<1) {
        tft.print("Writing Paused");
        }
     delay(500);
  //Set up buttons
    start_btn.initButton(&tft,  60, 280, 100, 40, WHITE, CYAN, BLACK, "Write", 2);
    pause_btn.initButton(&tft, 180, 280, 100, 40, WHITE, CYAN, BLACK, "Pause", 2);
  //Make sure your buttons start unpressed
    start_btn.drawButton(false);
    pause_btn.drawButton(false);
}

void loop(void) {

long millisec = millis();
long sec = (millisec / 1000); 
int16_t OUTres;

//Ensure SD WRITE state stays in reasonable bounds:
if (WRITE<0) {WRITE++;}
if (WRITE>1) {WRITE--;}

//Check for button presses:
  bool down = Touch_getXY();
    start_btn.press(down && start_btn.contains(pixel_x, pixel_y));
    pause_btn.press(down && pause_btn.contains(pixel_x, pixel_y));
    if (start_btn.justReleased())
        start_btn.drawButton();
    if (pause_btn.justReleased())
        pause_btn.drawButton();
    if (start_btn.justPressed()) {
        start_btn.drawButton(true);
        tft.fillRect(10, 200, 220, 40, BLACK);
        tft.setTextColor(WHITE);    
        tft.setTextSize(2);
        tft.setCursor(40, 220);
        tft.print("Writing to SD");
        WRITE++;
        }
    if (pause_btn.justPressed()) {
        pause_btn.drawButton(true);
        tft.fillRect(10, 200, 220, 40, BLACK);
        tft.setTextColor(WHITE);    
        tft.setTextSize(2);
        tft.setCursor(40, 220);
        tft.print("Writing Paused");
        WRITE--;
        }
 
//16-bit ADC
  //Measures output from pellistor as a differential of ADC A2-A3; pellistor max non-scaled value ~0.3 V
  //Adjust gain to 4x, Vout max +/- 1.024V  with resolution of 1 bit=0.03125mV
  ads.setGain(GAIN_FOUR);  
  float multiplier = 0.03125F; 
  OUTres = ads.readADC_Differential_2_3();
  float OUTmV = (OUTres * multiplier);
  Serial.print("OUT Differential: "); Serial.print(OUTres); Serial.print("("); Serial.print(OUTmV); Serial.println("mV)");

//Display
  //Time
     tft.fillRect(105, 15, 115, 22, BLACK);
     tft.setTextColor(WHITE);    
     tft.setTextSize(3);
     tft.setCursor(105, 15);
     tft.print(sec);
   //TCD Sensor readings
     tft.fillRect(80, 90, 120, 30, BLACK);
     tft.setTextColor(WHITE);    
     tft.setTextSize(3);
     tft.setCursor(80, 95);
     tft.print(OUTmV);

//Write Data to SD
    //Check if write mode is ON:
  if (WRITE>0) {
    // if the file is available, write to it:
    File dataFile = SD.open("datalog.txt", FILE_WRITE);
      dataFile.print(millisec);
      dataFile.print(",");
      dataFile.println(OUTmV);
      dataFile.close();
    }
      //Don't kill the poor SD card, give it time between measurements!
      delay(1000);   
  }
