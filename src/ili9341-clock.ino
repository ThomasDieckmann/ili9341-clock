
// #define DEBUG           // general debugging
// #define DISPLAY_ALIGN   // debug display alignment
// #define SET_TIME        // set time for first run
// #define DEBUG_STATS     // debugging for statistics
// #define DEBUG_DATE      // debugging date display

#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"

#include <Wire.h>
#include "RTClib.h"
#include "SparkFunHTU21D.h"

/*  define the 7 segments: https://en.wikipedia.org/wiki/Seven-segment_display
         _____
      _ <__A__> _ 
     | |       | |
     | |       | |
     |F|       |B|
     | |       | |
     |_| _____ |_|
      _ <__G__> _ 
     | |       | |
     | |       | |
     |E|       |C|
     | |       | |
     |_| _____ |_|
        <__D__>         */

/* Color definitions
#define ILI9341_BLACK 0x0000       ///<   0,   0,   0
#define ILI9341_NAVY 0x000F        ///<   0,   0, 123
#define ILI9341_DARKGREEN 0x03E0   ///<   0, 125,   0
#define ILI9341_DARKCYAN 0x03EF    ///<   0, 125, 123
#define ILI9341_MAROON 0x7800      ///< 123,   0,   0
#define ILI9341_PURPLE 0x780F      ///< 123,   0, 123
#define ILI9341_OLIVE 0x7BE0       ///< 123, 125,   0
#define ILI9341_LIGHTGREY 0xC618   ///< 198, 195, 198
#define ILI9341_DARKGREY 0x7BEF    ///< 123, 125, 123
#define ILI9341_BLUE 0x001F        ///<   0,   0, 255
#define ILI9341_GREEN 0x07E0       ///<   0, 255,   0
#define ILI9341_CYAN 0x07FF        ///<   0, 255, 255
#define ILI9341_RED 0xF800         ///< 255,   0,   0
#define ILI9341_MAGENTA 0xF81F     ///< 255,   0, 255
#define ILI9341_YELLOW 0xFFE0      ///< 255, 255,   0
#define ILI9341_WHITE 0xFFFF       ///< 255, 255, 255
#define ILI9341_ORANGE 0xFD20      ///< 255, 165,   0
#define ILI9341_GREENYELLOW 0xAFE5 ///< 173, 255,  41
#define ILI9341_PINK 0xFC18        ///< 255, 130, 198   */

// For the Adafruit shield, these are the default.
#define TFT_DC     9
#define TFT_CS    10
#define TFT_MISO  12
#define TFT_MOSI  11
#define TFT_CLK   13
#define TFT_RST   A7

#define BAUDRATE 115200
#define LEDPIN 3

#define CLK_PIN           6    // Rotary Encoder Pins  // Definition der Pins. CLK an D6, DT an D5. 
#define DT_PIN            5    // Rotary Encoder Pins
#define SW_PIN            2    // Switch is connected to D2 Verbunden. CAVE : use an interrupt pin!
#define MAXMENU           8    // Anzahl der Setup-Menü-Einträge
#define MAXVALUES        40    // array sizes for temp and humid stats
//#define MAXVALUES        3    // array sizes for temp and humid stats
#define DEFAULT_BRIGHT   50    // default brightness

int   date_text_length      = 0,
      encoderVal            = 0,
      change                = 0,
      MenuSelection         = MAXMENU,  // default Menü-Eintrag
      date_year             = 2021,
      segment_height        = 20, // 7-Segment-Display values
      segment_width         = 5,
      segment_color         = ILI9341_BLUE,
      humd_array[MAXVALUES],
      temp_array[MAXVALUES],
      array_pointer         = -1,
      maximum_temp          = -32000,
      minimum_temp          = 32000,
      maximum_humd          = 0,
      minimum_humd          = 2000;

bool  SelectCelsius         = true,
      AlarmOnOff            = true,
      HourChime             = true,
      SW_pressed            = false,
      adjust_alarm_hour     = false,
      adjust_alarm_minute   = false,
      adjust_time_hour     = false,
      adjust_time_minute   = false,
      adjust_date_day       = false,
      adjust_date_month     = false,
      adjust_date_year      = false,
      time_has_changed      = false,
      date_has_changed      = false,
      alarm_has_changed     = true,
      chime_has_changed     = true,
      stats_have_changed    = true,
      year_can_be_added     = false,   // append 4-digit year to output string
      locking_insert_stats  = false;   // can lock this, to prevent inserting multiple same values in one second

byte  stunde_digit1_alt     = 11,  // init-Werte egal, Hauptsache Unterscheidlich alt ./. neu
      stunde_digit2_alt     = 11,
      stunde_digit1_neu     = 1,
      stunde_digit2_neu     = 1,
      minute_digit1_alt     = 11,
      minute_digit2_alt     = 11,
      minute_digit1_neu     = 1,
      minute_digit2_neu     = 1,
      sekunde_digit1_alt    = 11,
      sekunde_digit2_alt    = 11,
      sekunde_digit1_neu    = 1,
      sekunde_digit2_neu    = 1,
      alarm_hour            = 12,
      alarm_minute          = 0,
      time_hour             = 12,
      time_minute           = 0,
      date_day              = 1,
      date_day_old          = 32,  // first run, we'll draw a date
      date_month            = 1,
      display_brightness    = DEFAULT_BRIGHT;

float humd,
      temp,
      humd_alt              = 0,
      temp_alt              = 9999;

// Use hardware SPI (on Uno, #13, #12, #11) and the above for CS/DC
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

// If using the breakout, change pins as desired
//Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST, TFT_MISO);

// Date and time functions using a DS1307 RTC connected via I2C and Wire lib
RTC_DS1307 rtc;
// char daysOfTheWeek[7][12] = {"Sunday",  "Monday", "Tuesday",  "Wednesday", "Thursday",   "Friday",  "Saturday"};
   char daysOfTheWeek[7][12] = {"Sonntag", "Montag", "Dienstag", "Mittwoch",  "Donnerstag", "Freitag", "Samstag"};
// char daysOfTheWeek[7][12] = {"So", "Mo", "Di", "Mi", "Do", "Fr", "Sa"};
// char monthsOfTheYear[12][11] = {"January", "February", "March", "April",  "May", "June", "July", "August", "September", "October", "November", "December"};
// char monthsOfTheYear[12][11] = {"Januar",  "Februar",  "März",  "April",  "Mai", "Juni", "Juli", "August", "September", "Oktober", "November", "Dezember"};
// char monthsOfTheYear[12][11] = {"Januar",  "Februar",  77 0x84 114 122,  "April",  "Mai", "Juni", "Juli", "August", "September", "Oktober", "November", "Dezember"};
//   char monthsOfTheYear[12][11] = {"Januar",  "Februar", 0x4D0x840x720x7A ,  "April",  "Mai", "Juni", "Juli", "August", "September", "Oktober", "November", "Dezember"};
   char monthsOfTheYear[12][11] = {"Januar",  "Februar", "\x4d\x84\x72\x7a" ,  "April",  "Mai", "Juni", "Juli", "August", "September", "Oktober", "November", "Dezember"};


//Create an instance of the object
HTU21D myHumidity;



void setup() {
              
   Serial.begin(BAUDRATE);
   Serial.println(F("ILI9341 Clock")); 

   //
   //
   // redo the map function, not enough precision!
   //
   //
   /*for (int i = 0; i < 145; i++ ) {
      Serial.print(i);  // we're having a higher display pixel window space than values in array!
      Serial.print(' ');
      Serial.print(remap(i, 0, 145, 0, 3));  // we're having a higher display pixel window space than values in array!
      Serial.println(' ');
   }*/
//   while (1);
   pinMode(A2,OUTPUT);
   digitalWrite(A2,HIGH);

   // init array for stats
   #ifdef DEBUG_STATS
      Serial.print("init array: ");
   #endif
   for (int i = 0; i < MAXVALUES; i++)
   {
      humd_array[i] = 0;
      temp_array[i] = 0;
      #ifdef DEBUG_STATS
         Serial.print(i);
         Serial.print(' ');
      #endif
   }
   #ifdef DEBUG_STATS
      Serial.println(' ');
   #endif

   pinMode(LEDPIN, OUTPUT);
   analogWrite(LEDPIN, display_brightness);
  
   tft.begin();
   if (! rtc.begin()) {
      Serial.println(F("Couldn't find 1307-RTC"));
      while (1);
   } else {
      Serial.println(F("RTC found"));
   }

   tone(8,680,80);
   delay(130);
   tone(8,440,80);
   delay(130);
   tone(8,560,120);
   delay(130);
   noTone(8);
   yield();
   analogWrite(LEDPIN,display_brightness);
    
   myHumidity.begin();

   // init rotary encoder
   pinMode(SW_PIN, INPUT_PULLUP);   // Hier wird der Interrupt installiert.
   //attachInterrupt(digitalPinToInterrupt(SW_PIN), SW_Interrupt, LOW);
   attachInterrupt(digitalPinToInterrupt(SW_PIN), SW_Interrupt, LOW);

   pinMode(CLK_PIN, INPUT_PULLUP);
   pinMode(DT_PIN, INPUT_PULLUP);

   if (! rtc.isrunning()) {
      Serial.println("RTC is NOT running!");
      // following line sets the RTC to the date & time this sketch was compiled
      // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
      // This line sets the RTC with an explicit date & time, for example to set
      // August 8, 2021 at 0:35 you would call:
      #ifdef SET_TIME
          rtc.adjust(DateTime(2021, 8, 25, 17, 11, 0));
      #endif
      MenuSelection = 5;
      SW_pressed = true;
      CallSetupMenu();
   }

  
   DateTime now = rtc.now();
   #ifdef DEBUG
      Serial.print(now.day(), DEC);
      Serial.print(". ");
      Serial.print(monthsOfTheYear[now.month()]);
      Serial.print(' ');
      Serial.print(now.year(), DEC);
      Serial.print(" (");
      Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);
      Serial.print(") ");
      Serial.print(now.hour(), DEC);
      Serial.print(':');
      Serial.print(now.minute(), DEC);
      Serial.print(':');
      Serial.print(now.second(), DEC);
      Serial.println();
      // read diagnostics (optional but can help debug problems)
      uint8_t x = tft.readcommand8(ILI9341_RDMODE);
      Serial.print("Display Power Mode: 0x"); Serial.println(x, HEX);
      x = tft.readcommand8(ILI9341_RDMADCTL);
      Serial.print("MADCTL Mode: 0x"); Serial.println(x, HEX);
      x = tft.readcommand8(ILI9341_RDPIXFMT);
      Serial.print("Pixel Format: 0x"); Serial.println(x, HEX);
      x = tft.readcommand8(ILI9341_RDIMGFMT);
      Serial.print("Image Format: 0x"); Serial.println(x, HEX);
      x = tft.readcommand8(ILI9341_RDSELFDIAG);
      Serial.print("Self Diagnostic: 0x"); Serial.println(x, HEX); 
   #endif

   tft.setRotation(3);
   tft.fillScreen(ILI9341_BLACK);

   size_7segment (15);
   color_7segment(0xFFE0);
   display_7segment ('P',  20, 65);
   display_7segment ('U',  80, 65);
   display_7segment ('L', 140, 65);
   display_7segment ('S', 200, 65);
   display_7segment ('E', 260, 65);
   delay(2000);
  
   size_7segment (6);

   tft.setRotation(3);

   // CallSetupMenu();

   tft.fillScreen(ILI9341_BLACK);
   pulse_logo();
   delay(6000);
   tft.fillScreen(ILI9341_BLACK);

   tft.setTextSize(5);
   SW_pressed = false;
 
   tft.fillScreen(ILI9341_BLACK);


   /* Test für Schaltjahresberechnung
   for (int i = 2020; i < 2800; i++) {
       Serial.print (i); Serial.print (": ");
       Serial.println (28 + int((4-(i%4))/4) - int((100-(i%100))/100) + int((400-(i%400))/400) );
   }*/ 
}


void loop() {

   showClock();
   delay (30);
 
   float humd = myHumidity.readHumidity();
   float temp = myHumidity.readTemperature();

   #ifdef DEBUG
     Serial.print(F(" Temperature: "));
     Serial.print(temp, 3);
     Serial.print("C");
     Serial.print(F(" Humidity: "));
     Serial.print(humd, 3);
     Serial.print('%');
     Serial.println();
   #endif
  
   if (SW_pressed == true) {
      SW_pressed = false;
      CallSetupMenu();
      tft.fillScreen(ILI9341_BLACK);
   }
}


void showClock() {
  
   tft.setRotation(3);

   #ifdef DISPLAY_ALIGN
      tft.fillRect(0, 0, 1, 240, ILI9341_WHITE);
      tft.fillRect(319, 0, 1, 240, ILI9341_WHITE);
      tft.drawLine(319,0,319,239,ILI9341_WHITE);
   #endif

   segment_color = ILI9341_YELLOW;

   // Uhrzeit übertragen
   DateTime now = rtc.now();
   stunde_digit1_neu  = now.hour() / 10;
   stunde_digit2_neu  = now.hour() % 10;
   minute_digit1_neu  = now.minute() / 10;
   minute_digit2_neu  = now.minute() % 10;
   sekunde_digit1_neu = now.second() / 10;
   sekunde_digit2_neu = now.second() % 10;

   date_day = now.day();
   date_month = now.month();
   date_year = now.year();
  
   size_7segment(14); 
   if (stunde_digit1_neu != stunde_digit1_alt) {
       stunde_digit1_alt = stunde_digit1_neu;   display_7segment (char(48 + stunde_digit1_neu), 0, 6);
   }
   if (stunde_digit2_neu != stunde_digit2_alt) {
       stunde_digit2_alt = stunde_digit2_neu;   display_7segment (char(48 + stunde_digit2_neu), 55, 6);
   }

   if (sekunde_digit2_neu != sekunde_digit2_alt) {
      if (sekunde_digit2_neu%2 == 1) {
         display_dots (false, false, 106, 6);
      } else {
         display_dots (true, true, 106, 6);        
      }
   }
   
   if (minute_digit1_neu != minute_digit1_alt) {
       minute_digit1_alt = minute_digit1_neu;   display_7segment (char(48 + minute_digit1_neu), 123, 6);
   }
   if (minute_digit2_neu != minute_digit2_alt) {
       minute_digit2_alt = minute_digit2_neu;   display_7segment (char(48 + minute_digit2_neu), 178, 6);
   }

   if (sekunde_digit2_neu != sekunde_digit2_alt) {
      if (sekunde_digit2_neu%2 == 1) {
         display_dots (false, false, 230, 6);
      } else {
         display_dots (true, true, 230, 6);        
      }
   }
   // seconds digits may be smaller
   size_7segment(10); 
   if (sekunde_digit1_neu != sekunde_digit1_alt) {
       sekunde_digit1_alt = sekunde_digit1_neu; display_7segment (char(48 + sekunde_digit1_neu), 246, 25);
   }
   if (sekunde_digit2_neu != sekunde_digit2_alt) {
       sekunde_digit2_alt = sekunde_digit2_neu; display_7segment (char(48 + sekunde_digit2_neu), 286, 25);
   }

   // Output new date
   if (date_day_old != now.day())
   {
      year_can_be_added = false;
      date_day_old = now.day();
      tft.setTextColor(ILI9341_BLACK);
      tft.setTextSize(2);
      // 5 Chars (,. ) + date text + month text + day text
      date_text_length = 4 + 2 + strlen(daysOfTheWeek[now.dayOfTheWeek()]) + strlen(monthsOfTheYear[now.month()-1]);
      if (now.day() < 10) date_text_length -= 1;
      if (date_text_length < 22) {
         date_text_length+=5;  // if string is not too long, we can add the year
         year_can_be_added = true;
      }
      tft.fillRect (0, 91, 320, 22, ILI9341_GREEN);
      tft.setCursor((320 - date_text_length * 12) / 2, 95);
      tft.print(daysOfTheWeek[now.dayOfTheWeek()]);
      tft.print(", ");
      tft.print(now.day());
      tft.print(". ");
      tft.print(monthsOfTheYear[now.month()-1]);
      // add year, if length short enough
      if (year_can_be_added) {
         tft.print(" ");
         tft.print(now.year());
      }
      #ifdef DEBUG_DATE
         Serial.print("length of complete date string: ");
         Serial.print(date_text_length);
         Serial.print("\nlength of day of week ");
         Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);
         Serial.print(": ");
         Serial.print(strlen(daysOfTheWeek[now.dayOfTheWeek()]));
         Serial.print("\nlength of month string ");
         Serial.print(monthsOfTheYear[now.month()-1]);
         Serial.print(": ");
         Serial.println(strlen(monthsOfTheYear[now.month()-1]));
      #endif
   }
   //just for debugging alignment => center
   //for (int i=12;i<315;i+=12) tft.drawLine(i, 100, i, 170, ILI9341_WHITE);
   //tft.fillRect (0, 115, 320, 20, ILI9341_BLACK);
   //tft.setCursor(1, 115);
   //tft.print(date_text_length);


   float humd = myHumidity.readHumidity();
   float temp = myHumidity.readTemperature();
 
   if (calculate_divergence(temp, temp_alt) > 0) {
      temp_alt = temp;
      tft.setTextColor(ILI9341_ORANGE);
      tft.setTextSize(4);
      tft.fillRect (15, 120, 145, 30, ILI9341_BLACK);
      tft.setCursor(15, 120);
      if (SelectCelsius) {
         tft.print(temp, 1);
         tft.print(char(247));
         tft.print('C');
      } else {
         tft.print(convert_c2f(temp));
         tft.print(char(247));
         tft.print('F');
      }
   }
   if (calculate_divergence(humd, humd_alt) > 0) {
      humd_alt = humd;
      tft.setTextColor(ILI9341_ORANGE);
      tft.setTextSize(4);
      tft.fillRect (185, 120, 145, 30, ILI9341_BLACK);
      tft.setCursor(185, 120);
      tft.print(humd, 1);
      tft.print('%');
   }


   // draw chime and alarm indicators
   tft.setTextSize(1);
   tft.setTextColor(ILI9341_WHITE);
      
   if (alarm_has_changed) {
      alarm_has_changed = false;
      if (AlarmOnOff) {
         tft.fillRect(282,0,40,11,ILI9341_BLUE);
         tft.setCursor(286, 2);
         if (alarm_hour<10) tft.print('0');
         tft.print(alarm_hour);
         tft.print(':');
         if (alarm_minute<10) tft.print('0');
         tft.print(alarm_minute);
      } else {
         tft.fillRect(282,0,40,11,ILI9341_BLACK);     
      }
   }
   
   if (chime_has_changed) {
      chime_has_changed = false;
      if (HourChime) {
         tft.fillRect(240,0,40,11,ILI9341_BLUE);
         tft.setCursor(245, 2);
         tft.print("Chime");    
      } else {
         tft.fillRect(240,0,40,11,ILI9341_BLACK);        
      }
   }

   // draw statistic boxes
   if (stats_have_changed) {
      stats_have_changed = false;
      tft.setTextColor(ILI9341_CYAN);
      tft.setRotation(2);      // paint maximum and minimum values
      if (array_pointer >= 0) {
         tft.fillRect (0, 0, 85, 8, ILI9341_BLACK);
         tft.setCursor(0, 0);
         tft.print(float(minimum_temp/10.0),1);
         tft.setCursor(58, 0);
         tft.print(float(maximum_temp/10.0),1);
         tft.fillRect (0, 162, 85, 8, ILI9341_BLACK);
         tft.setCursor(0, 162);
         tft.print(float(minimum_humd/10.0),1);
         tft.setCursor(58, 162);
         tft.print(float(maximum_humd/10.0),1);              
      }
      tft.setRotation(3);
      tft.drawRect (9,   154, 149, 86, ILI9341_LIGHTGREY); // draw boxes for stats
      tft.drawRect (171, 154, 149, 86, ILI9341_LIGHTGREY);
   }

   //   if (now.minute()%2 == 0 && now.second() == 0) stats_have_changed = true;
   if (now.second() == 0) stats_have_changed = true;  // filling every minute
   if (stats_have_changed && !locking_insert_stats) {
      append_new_array_values(int(temp * 10.0), int(humd * 10.0)); // so we don't need to store in float array, int will be enough
      locking_insert_stats = true;
      #ifdef DEBUG
         tft.fillRect(250,200,40,15,ILI9341_BLACK);
         tft.setCursor(250,200);
         tft.setTextSize(2);
         tft.print(array_pointer);
         for (int i = 0; i < MAXVALUES; i++)
         {
            Serial.print(humd_array[i]);
            Serial.print(' ');
         }
         Serial.println(' ');
         for (int i = 0; i < MAXVALUES; i++)
         {
            Serial.print(temp_array[i]);
            Serial.print(' ');
         }
         Serial.println(' ');
      #endif

      // tft.drawRect( 10, 155, 147, 84, ILI9341_BLUE);  // area for stats
      // tft.drawRect(172, 155, 147, 84, ILI9341_BLUE);  // area for stats
     
      for (int i = 0; i < 146; i++) {
          //
          //
          // redo the map function, not enough precision!
          //
          //
          int array_element = remap(i, 0, 145, 0, min(array_pointer,MAXVALUES -1));  // we're having a higher display pixel window space than values in array!

          int y_temp=map(temp_array[array_element],minimum_temp,maximum_temp,238,154);
          int y_humd=map(humd_array[array_element],minimum_humd,maximum_humd,238,154);
          tft.drawLine(11+i , 155,    11+i,  y_temp, ILI9341_BLACK);
          tft.drawLine(11+i , y_temp, 11+i,  155+84, ILI9341_CYAN);
          tft.drawLine(172+i, 155,    172+i, y_humd, ILI9341_BLACK);
          tft.drawLine(172+i, y_humd, 172+i, 155+84, ILI9341_CYAN);
             
          #ifdef DEBUG_STATS
             if (y_humd < 154 || y_temp < 154) { // finde den Fehler
                Serial.print("Array Pointer: "); Serial.println(array_pointer);
                Serial.print("min_temp: "); Serial.print(minimum_temp); Serial.print("   max_temp: "); Serial.println(maximum_temp);
                Serial.print("min_humd: "); Serial.print(minimum_humd); Serial.print("   max_humd: "); Serial.println(maximum_humd);
                Serial.print("y_temp: ");Serial.print(y_temp); Serial.print("  y_humd: "); Serial.println(y_humd);
                for (int i = 0; i < 146; i++) {
                    int array_element = remap(i, 0, 145, 0, min(array_pointer,MAXVALUES -1));  // we're having a higher display pixel window space than values in array!
                    Serial.print(array_element); Serial.print(' ');
                }
                Serial.println(' '); Serial.print("humd: ");
                for (int i = 0; i < 146; i++) {
                    int array_element = remap(i, 0, 145, 0, min(array_pointer,MAXVALUES -1));  // we're having a higher display pixel window space than values in array!
                    //int y_humd = map(humd_array[array_element],minimum_humd,maximum_humd,0,83) + 155;
                    Serial.print(humd_array[array_element]); Serial.print(' ');
                }
                Serial.println(' '); Serial.print("temp: ");
                for (int i = 0; i < 146; i++) {
                    int array_element = remap(i, 0, 145, 0, min(array_pointer,MAXVALUES -1));  // we're having a higher display pixel window space than values in array!
                    //int y_humd = map(humd_array[array_element],minimum_humd,maximum_humd,0,83) + 155;
                    Serial.print(temp_array[array_element]); Serial.print(' ');
                }
                Serial.println(' ');
                while (1) { analogWrite(LEDPIN,0); delay(50); analogWrite(LEDPIN,60); delay(150); }
             }
          #endif
      }
   }
   if (now.second() > 0) locking_insert_stats = false;  // unlock   
}


void display_SetupMenu()
{
   tft.setRotation(3);
   tft.setCursor(40, 1);
   tft.setTextColor(ILI9341_RED);
   tft.setTextSize(4);
   tft.print(F("Setup Menu"));

   tft.fillRect(20, 36,  280, 4, ILI9341_LIGHTGREY);
   tft.fillRect(20, 125, 280, 4, ILI9341_LIGHTGREY);
   // check the margins
   #ifdef DISPLAY_ALIGN
      tft.fillRect(300, 36, 3, 180, ILI9341_WHITE);
      tft.fillRect(288, 36, 1, 180, ILI9341_WHITE);
   #endif

   tft.setTextSize(2);

   tft.fillRect(260, 45, 40, 16, ILI9341_BLACK);
   tft.setCursor(20, 45);
   if (MenuSelection == 1) {
      tft.setTextColor(ILI9341_CYAN);
   } else {
      tft.setTextColor(ILI9341_YELLOW);
   }
   tft.print(F("Alarm:"));
   tft.setCursor(265, 45);
   if (AlarmOnOff) {
      digitalWrite(A2,LOW);
      tft.print(" ON");
   } else {
      digitalWrite(A2,HIGH);
      tft.print("OFF");  
   }
   tft.fillRect(260, 65, 40, 16, ILI9341_BLACK);
   tft.setCursor(20, 65);
   if (MenuSelection == 2) {
      tft.setTextColor(ILI9341_CYAN);
   } else {
      tft.setTextColor(ILI9341_YELLOW);
   }
   tft.print(F("Hour Chime: "));
   tft.setCursor(265, 65);
   if (HourChime) {
      tft.print(" ON");
   } else {
      tft.print("OFF");  
   }
   tft.fillRect(284,  85, 16, 16, ILI9341_BLACK);
   tft.setCursor(20, 85);
   if (MenuSelection == 3) {
      tft.setTextColor(ILI9341_CYAN);
   } else {
      tft.setTextColor(ILI9341_YELLOW);
   }  
   tft.print(F("Set C/F:"));
   tft.setCursor(289, 85);
   if (SelectCelsius) {
      tft.print('C');
   } else {
      tft.print('F');  
   }
   tft.setCursor(20, 105);
   if (MenuSelection == 4) {
      tft.setTextColor(ILI9341_CYAN);
   } else {
      tft.setTextColor(ILI9341_YELLOW);
   }  
   tft.print(F("Brightness: "));
   display_setup_brightness();
   
   tft.setCursor(20, 135);
   if (MenuSelection == 5) {
      tft.setTextColor(ILI9341_CYAN);
   } else {
      tft.setTextColor(ILI9341_YELLOW);
   }
   tft.print(F("Set Time"));
   display_setup_time();

   tft.setCursor(20, 155);
   if (MenuSelection == 6) {
      tft.setTextColor(ILI9341_CYAN);
   } else {
      tft.setTextColor(ILI9341_YELLOW);
   }  
   tft.print(F("Set Date"));
   display_setup_date();

   tft.setCursor(20, 175);
   if (MenuSelection == 7) {
      tft.setTextColor(ILI9341_CYAN);
   } else {
      tft.setTextColor(ILI9341_YELLOW);
   }  
   tft.print(F("Set Alarm"));
   display_setup_alarm();
   
   tft.setCursor(20, 215);
   if (MenuSelection == 8) {
      tft.setTextColor(ILI9341_CYAN);
   } else {
      tft.setTextColor(ILI9341_YELLOW);
   }  
   tft.print(F("EXIT"));
}



// show brightness value in setup menu
void display_setup_brightness() {
   tft.fillRect(260, 105, 40, 16, ILI9341_BLACK); // delete old values
   if (display_brightness < 10) {
      tft.setCursor(291, 105);
   } else if (display_brightness < 100) {
      tft.setCursor(277, 105);
   } else {
      tft.setCursor(265, 105);
   }
   tft.print(display_brightness);  
}


// show date value in setup menu
void display_setup_date() {
   tft.fillRect(181, 155, 119, 16, ILI9341_BLACK);
   tft.setCursor(181, 155);
   if (adjust_date_day) {
      tft.setTextColor(ILI9341_WHITE);
   } else {
      if (MenuSelection == 6 ) {
          tft.setTextColor(ILI9341_CYAN);
      } else {
          tft.setTextColor(ILI9341_YELLOW);
      } 
   }
   if (date_day < 10) tft.print('0');
   tft.print(date_day, DEC);
   if (MenuSelection == 6 ) {
       tft.setTextColor(ILI9341_CYAN);
   } else {
       tft.setTextColor(ILI9341_YELLOW);    
   }   
   tft.print('.');
   if (adjust_date_month) {
      tft.setTextColor(ILI9341_WHITE);
   } else {
      if (MenuSelection == 6 ) {
          tft.setTextColor(ILI9341_CYAN);
      } else {
          tft.setTextColor(ILI9341_YELLOW);
      }
   }
   if (date_month < 10) tft.print('0'); 
   tft.print(date_month, DEC);
   if (MenuSelection == 6 ) {
       tft.setTextColor(ILI9341_CYAN);
   } else {
       tft.setTextColor(ILI9341_YELLOW);    
   }
   tft.print('.');
   if (adjust_date_year) tft.setTextColor(ILI9341_WHITE);
   tft.print(date_year, DEC);
}



void display_setup_time() {

   // DateTime now = rtc.now();

   tft.fillRect(241, 135, 95, 16, ILI9341_BLACK);
   tft.setCursor(241, 135);
   if (adjust_time_hour) {
       tft.setTextColor(ILI9341_WHITE);
   } else {
      if (MenuSelection == 5 ) {
          tft.setTextColor(ILI9341_CYAN);
      } else {
          tft.setTextColor(ILI9341_YELLOW);    
      }
   }
   if (time_hour < 10) tft.print('0');
   tft.print(time_hour, DEC);
   tft.print(':');
   if (adjust_time_minute) {
       tft.setTextColor(ILI9341_WHITE);
   } else {
      if (MenuSelection == 5 ) {
          tft.setTextColor(ILI9341_CYAN);
      } else {
          tft.setTextColor(ILI9341_YELLOW);    
      }
   }  
   if (time_minute < 10) tft.print('0');
   tft.print(time_minute, DEC);
}   


// show alarm value in setup menu
void display_setup_alarm() {
   tft.fillRect(241, 175, 59, 16, ILI9341_BLACK);
   tft.setCursor(241, 175);
   if (adjust_alarm_hour) {
       tft.setTextColor(ILI9341_WHITE);
   } else {
      if (MenuSelection == 7 ) {
          tft.setTextColor(ILI9341_CYAN);
      } else {
          tft.setTextColor(ILI9341_YELLOW);    
      }
   }
   if (alarm_hour < 10) tft.print('0');
   tft.print(alarm_hour, DEC);
   if (MenuSelection == 7 ) {
       tft.setTextColor(ILI9341_CYAN);
   } else {
       tft.setTextColor(ILI9341_YELLOW);    
   } 
   tft.print(':');
   if (adjust_alarm_minute) {
      tft.setTextColor(ILI9341_WHITE);
   }
   if (alarm_minute < 10) tft.print('0'); 
   tft.print(alarm_minute, DEC);
}



// we'll display a dot or a colon, maybe other stuff l8r
void display_specialchar (char digit, int x_pos, int y_pos, unsigned int d_color)
{
   while (1); 
}


void size_7segment (int scaling)
{
    switch (scaling) {
        case  0: segment_height =  4; segment_width = 2;  break;
        case  1: segment_height =  6; segment_width = 2;  break;
        case  2: segment_height =  8; segment_width = 2;  break;
        case  3: segment_height = 10; segment_width = 3;  break;
        case  4: segment_height = 12; segment_width = 3;  break;
        case  5: segment_height = 14; segment_width = 3;  break;
        case  6: segment_height = 16; segment_width = 4;  break;
        case  7: segment_height = 18; segment_width = 4;  break;
        case  9: segment_height = 20; segment_width = 5;  break;
        case 10: segment_height = 22; segment_width = 5;  break;
        case 11: segment_height = 24; segment_width = 5;  break;
        case 12: segment_height = 26; segment_width = 6;  break;
        case 13: segment_height = 28; segment_width = 6;  break;
        case 14: segment_height = 30; segment_width = 6;  break;
        case 15: segment_height = 32; segment_width = 7;  break;
        default: segment_height = 20; segment_width = 5;  break;
    } 
}

// display colon or upper/lower dot
void display_dots (bool dot1, bool dot2, int x_pos, int y_pos)
{
   if (dot1) {
      tft.fillRect(x_pos, y_pos + segment_height, segment_width, segment_width, segment_color);
   } else {
      tft.fillRect(x_pos, y_pos + segment_height, segment_width, segment_width, ILI9341_BLACK);
   }
   if (dot2) {
      tft.fillRect(x_pos, y_pos + segment_height*2, segment_width, segment_width, segment_color);
   } else {
      tft.fillRect(x_pos, y_pos + segment_height*2, segment_width, segment_width, ILI9341_BLACK);    
   }
}

void color_7segment (unsigned int d_color)
{
  segment_color = d_color;
}



// display digits in 7 segment style
void display_7segment (char digit, int x_pos, int y_pos)
{
   int lit_segments = 0;
   //unsigned int seg_colors[7] = {0,0,0,0,0,0,0};  // first all segments are dark
   unsigned int seg_colors[7] = {8416,8416,8416,8416,8416,8416,8416};  // first all segments are dark/dimmed grey
       
   switch (digit) {
      case '0':  lit_segments = 126;  break;
      case '1':  lit_segments = 48;   break;
      case '2':  lit_segments = 109;  break;
      case '3':  lit_segments = 121;  break;
      case '4':  lit_segments = 51;   break;
      case '5':  lit_segments = 91;   break;
      case '6':  lit_segments = 95;   break;
      case '7':  lit_segments = 112;  break;
      case '8':  lit_segments = 0x7F; break;
      case '9':  lit_segments = 0x7B; break;
      case 'A':  lit_segments = 0x77; break;
      case 'B':  lit_segments = 0x1F; break;
      case 'C':  lit_segments = 0x4E; break;
      case 'c':  lit_segments = 13;   break;
      case 'D':  lit_segments = 0x3D; break;
      case 'E':  lit_segments = 0x4F; break;
      case 'F':  lit_segments = 0x47; break;
      case 'H':  lit_segments = 55;   break;
      case 'h':  lit_segments = 23;   break;
      case 'L':  lit_segments = 14;   break;
      case 'N':  lit_segments = 21;   break;
      case 'O':  lit_segments = 29;   break;
      case 'P':  lit_segments = 103;  break;
      case 'R':  lit_segments = 5;    break;
      case 'S':  lit_segments = 91;   break;
      case 'u':  lit_segments = 28;   break;
      case 'U':  lit_segments = 62;   break;
      case 'Y':  lit_segments = 59;   break;
      case '-':  lit_segments = 1;    break;
      case '_':  lit_segments = 8;    break;
      case '=':  lit_segments = 9;    break;
      case 'X':  lit_segments = 99;   break;
      default:   lit_segments = 0;    break;
   }

   if (lit_segments & 1)   seg_colors[6] = segment_color; if (lit_segments & 2)   seg_colors[5] = segment_color;
   if (lit_segments & 4)   seg_colors[4] = segment_color; if (lit_segments & 8)   seg_colors[3] = segment_color;
   if (lit_segments & 16)  seg_colors[2] = segment_color; if (lit_segments & 32)  seg_colors[1] = segment_color;
   if (lit_segments & 64)  seg_colors[0] = segment_color;
   
   tft.fillRect(x_pos + segment_width , y_pos , segment_height, segment_width, seg_colors[0]);                                                        // Segment-A
   tft.fillRect(x_pos + segment_height  + segment_width, y_pos  + segment_width, segment_width, segment_height, seg_colors[1]);                       // Segment-B
   tft.fillRect(x_pos + segment_height  + segment_width, y_pos  + 2 * segment_width + segment_height, segment_width, segment_height, seg_colors[2]);  // Segment-C
   tft.fillRect(x_pos + segment_width , y_pos  + segment_width * 2 + segment_height * 2, segment_height, segment_width, seg_colors[3]);               // Segment-D
   tft.fillRect(x_pos , y_pos  + 2 * segment_width + segment_height, segment_width, segment_height, seg_colors[4]);                                   // Segment-E
   tft.fillRect(x_pos , y_pos  + segment_width, segment_width, segment_height, seg_colors[5]);                                                        // Segment-F
   tft.fillRect(x_pos + segment_width , y_pos  + segment_width + segment_height, segment_height, segment_width, seg_colors[6]);                       // Segment-G
}

// Convert Celsius degrees to Fahrenheit
int convert_c2f (float c_degrees)
{
   return (c_degrees * 9/5 + 32);
}

int getEncoderTurn() {
  static int oldA = HIGH; //set the oldA as HIGH
  static int oldB = HIGH; //set the oldB as HIGH
  int result = 0;
  int newA = digitalRead(CLK_PIN); //read the value of clkPin to newA
  int newB = digitalRead(DT_PIN); //read the value of dtPin to newB
  if (newA != oldA || newB != oldB) //if the value of clkPin or the dtPin has changed
  {
    // something has changed
    if (oldA == HIGH && newA == LOW)
    {
      result = (oldB * 2 - 1);
    }
  }
  oldA = newA;
  oldB = newB;
  return result;
}


void CallSetupMenu() {
   tft.fillScreen(ILI9341_BLACK);
   display_SetupMenu();
   do {
      while (SW_pressed == false) {
         change = getEncoderTurn();
         if (change != 0) {
            MenuSelection = MenuSelection + change;
            if (MenuSelection > MAXMENU) MenuSelection = 1;
            if (MenuSelection < 1) MenuSelection = MAXMENU;
            display_SetupMenu();
         }
      }
      if (MenuSelection == 1 && SW_pressed) { AlarmOnOff = !AlarmOnOff; display_SetupMenu(); alarm_has_changed = true; }
      if (MenuSelection == 2 && SW_pressed) { HourChime = !HourChime; display_SetupMenu(); chime_has_changed = true;}
      if (MenuSelection == 3 && SW_pressed) { SelectCelsius = !SelectCelsius; display_SetupMenu(); }
      if (MenuSelection == 4 && SW_pressed) // adjust brighness
      {
         delay(300);
         SW_pressed = false;
         // tft.fillRect(260, 105, 40, 16, ILI9341_BLACK);
         tft.setTextColor(ILI9341_WHITE);
         display_setup_brightness();

         #ifdef DEBUG
            Serial.println("Enter brightness");
         #endif
         do
         {
            change = getEncoderTurn();
            if (change != 0) {
               display_brightness = constrain(display_brightness + (change * 4), 0, 120);
               analogWrite(LEDPIN, display_brightness + 15);
               #ifdef DEBUG
                  Serial.print(change); Serial.print(' '); Serial.print(display_brightness); Serial.println(' ');
               #endif
               // tft.fillRect(260, 105, 40, 16, ILI9341_BLACK);
               tft.setTextColor(ILI9341_WHITE);
               display_setup_brightness();
            }
         } while (SW_pressed == false);
         // tft.fillRect(260, 105, 40, 16, ILI9341_BLACK);
         tft.setTextColor(ILI9341_CYAN);
         display_setup_brightness();
         delay (350);  
      }
      if (MenuSelection == 5 && SW_pressed) // adjust time
      {
         delay(400);
         SW_pressed = false;
         adjust_time_hour = true;
         display_setup_time();
         do
         {
            change = getEncoderTurn();
            if (change != 0) {
               time_hour = constrain(time_hour + change, 0, 23);
               display_setup_time();
               time_has_changed = true;
            }
         } while (SW_pressed == false);
         adjust_time_hour = false;
         adjust_time_minute = true;
         // tft.setTextColor(ILI9341_CYAN);
         display_setup_time();
         delay (400);
         SW_pressed = false;
         do
         {
            change = getEncoderTurn();
            if (change != 0) {
               time_minute = constrain(time_minute + change, 0, 59);
               display_setup_time();
               time_has_changed = true;
            }
         } while (SW_pressed == false);
         adjust_time_minute = false;
         tft.setTextColor(ILI9341_CYAN);
         display_setup_time();
         delay (500);
      }
      
      if (MenuSelection == 6 && SW_pressed) // adjust date
      {
         delay(400);
         SW_pressed = false;
         adjust_date_day = true;
         display_setup_date();
         do
         {
            change = getEncoderTurn();
            if (change != 0) {
               date_day = constrain(date_day + change, 1, berechne_tage_pro_monat(date_month,date_year));
               display_setup_date();
               date_has_changed = true;
            }
         } while (SW_pressed == false);
         adjust_date_day = false;
         adjust_date_month = true;
         display_setup_date();
         delay (400);
         SW_pressed = false;
         do
         {
            change = getEncoderTurn();
            if (change != 0) {
               date_month = constrain(date_month + change, 1, 12);
               date_day = constrain(date_day, 1, berechne_tage_pro_monat(date_month,date_year));
               display_setup_date();
               date_has_changed = true;
            }
         } while (SW_pressed == false);
         adjust_date_month = false;
         adjust_date_year = true;
         display_setup_date();
         delay (400);
         SW_pressed = false;
         do
         {
            change = getEncoderTurn();
            if (change != 0) {
               date_year = constrain(date_year + change, 2010, 2400);
               date_day = constrain(date_day, 1, berechne_tage_pro_monat(date_month,date_year));
               display_setup_date();
               date_has_changed = true;
            }
         } while (SW_pressed == false);
         adjust_date_year = false;
         display_setup_date();
         delay (400);      
      }
      // SW_pressed = false;

      if (MenuSelection == 7 && SW_pressed) // adjust alarm time
      {
         delay(400);
         SW_pressed = false;
         adjust_alarm_hour = true;
         display_setup_alarm();
         do
         {
            change = getEncoderTurn();
            if (change != 0) {
               alarm_hour = constrain(alarm_hour + change, 0, 23);
               display_setup_alarm();
            }
         } while (SW_pressed == false);
         adjust_alarm_hour = false;
         adjust_alarm_minute = true;
         // tft.setTextColor(ILI9341_CYAN);
         display_setup_alarm();
         delay (400);
         SW_pressed = false;
         do
         {
            change = getEncoderTurn();
            if (change != 0) {
               alarm_minute = constrain(alarm_minute + change, 0, 59);
               display_setup_alarm();
            }
         } while (SW_pressed == false);
         adjust_alarm_minute = false;
         tft.setTextColor(ILI9341_CYAN);
         display_setup_alarm();
         delay (500);
      }
      SW_pressed = false;
   }
   while (MenuSelection < MAXMENU);

   if (time_has_changed)
   {
      rtc.adjust(DateTime(date_year, date_month, date_day, time_hour, time_minute, 0));
      time_has_changed = false;    
   }
   
   // nach dem Menü sollen ja alle Teile erstmalig neu gezeichnet werden
   date_day_old       = 32;
   stunde_digit1_alt  = 11;
   stunde_digit2_alt  = 11;
   minute_digit1_alt  = 11;
   minute_digit2_alt  = 11;
   sekunde_digit1_alt = 11;
   sekunde_digit2_alt = 11;
   temp_alt           = 9999;
   humd_alt           = 0;
   chime_has_changed  = true;
   alarm_has_changed  = true;
   stats_have_changed = true;
}



byte berechne_tage_pro_monat(byte b_monat, int b_jahr)
{
    return ( 30 + ((b_monat == 1) || (b_monat == 3) || (b_monat == 5) || (b_monat == 7) || (b_monat == 8) || (b_monat == 10) || (b_monat == 12)) - (b_monat == 2) * ( 2 - int((4-(b_jahr%4))/4) + int((100-(b_jahr%100))/100) - int((400-(b_jahr%400))/400)) );
}


// weichen 2 Werte stark voneinander ab?
int calculate_divergence(float wert1, float wert2)
{
    float hilfe;
    if (wert1 < wert2) { hilfe = wert1; wert1 = wert2; wert2 = hilfe; }    // immer den großen Wert zuerst
    if (wert2 == 0) { wert1 = wert1 + 0.1 ; wert2 = wert2 + 0.1; }         // verhindere division by zero
    hilfe = ((wert1 / wert2) * 100) - 100;
    if (hilfe > 100) {   // overflow verhindern
       return (100);
    } else {
       return (int(hilfe));
    }
}


// new values vor humidity and temperature are added, if we're having all values, we need to delete the oldest
void append_new_array_values(int insert_temperature,int insert_humidity)
{
   
   // if pointer < MAXVALUES, we just fill in
   if (array_pointer < MAXVALUES - 1) {
      array_pointer++;
      humd_array[array_pointer] = insert_humidity;
      temp_array[array_pointer] = insert_temperature;
      #ifdef DEBUG_STATS
         Serial.print("first filling array["); Serial.print(array_pointer); Serial.print("] humd: "); Serial.print(insert_humidity); Serial.print(" temp: "); Serial.println(insert_temperature);
      #endif
   } else {
      for (int i=0; i < MAXVALUES - 1; i++) {
          humd_array[i] = humd_array[i+1];
          temp_array[i] = temp_array[i+1];
          #ifdef DEBUG_STATS
             Serial.print("pushing array: "); Serial.print(i); Serial.print(' ');
          #endif
      }
      humd_array[MAXVALUES-1] = insert_humidity;
      temp_array[MAXVALUES-1] = insert_temperature;
      #ifdef DEBUG_STATS
         Serial.print("after push: "); Serial.print(temp_array[MAXVALUES-1]); Serial.print(' '); Serial.println(humd_array[MAXVALUES-1]);
      #endif
   }

   // calculate min and max values
   minimum_humd = 9999;
   maximum_humd = -9999;
   minimum_temp = 9999;
   maximum_temp = -9999;
   for (int i = 0; i <= array_pointer; i++) {
       #ifdef DEBUG_STATS
          Serial.print("minmax-calc: "); Serial.println(i);
       #endif
       if (humd_array[i] < minimum_humd) minimum_humd = humd_array[i];
       if (humd_array[i] > maximum_humd) maximum_humd = humd_array[i];
       if (temp_array[i] < minimum_temp) minimum_temp = temp_array[i];
       if (temp_array[i] > maximum_temp) maximum_temp = temp_array[i];
   }
}


int remap(int input, int range1_a, int range1_b, int range2_a, int range2_b)
{
   int diff1, diff2;
   diff1 = range1_b - range1_a;
   diff2 = range2_b - range2_a;
   float factor;

   if (diff2 == 0)
   {
      return(range2_a);
   } else {
      if (diff1 == 0) {
         return(round((range2_a+range2_b)/2.0));
      } else {
         factor = float(diff2) / float(diff1);
         return(round(float(input) * factor));
      }
   }
}


void SW_Interrupt() // Beginn des Interrupts. Wenn der Rotary Knopf betätigt wird, springt das Programm automatisch an diese Stelle. Nachdem...
{
  #ifdef DEBUG
    Serial.println("Switch betaetigt"); //... das Signal ausgegeben wurde, wird das Programm fortgeführt.
  #endif
  delay (250);
  SW_pressed = true;
}


// draw PULSE logo center=160/120
void pulse_logo()
{
   tft.fillCircle(160, 120, 99, 0x03E0);
   tft.drawCircle(160, 120, 100, 0xC618);
   tft.drawCircle(160, 120, 99, 0xC618);
   for (unsigned int i = 12; i < 189; i+=22) {
       tft.drawLine(i+60,120-sqrt(10000.0-pow((100.0-float(i)),2.0)),i+60,120+sqrt(10000.0-pow((100.0-float(i)),2.0)),0xC618);
       tft.drawLine(160-sqrt(10000.0-pow((100.0-float(i)),2.0)),i+20,160+sqrt(10000.0-pow((100.0-float(i)),2.0)),i+20,0xC618);
   }
   for (unsigned int i = 1; i < 20; i++) {
      tft.drawLine(60,120,94,120,0xFFE0);
      tft.drawLine(94,120,127,43,0xFFE0);
      tft.drawLine(127,43,193,197,0xFFE0);
      tft.drawLine(193,197,226,120,0xFFE0);
      tft.drawLine(226,120,260,120,0xFFE0);
      delay(200);
      tft.drawLine(60,120,94,120,0x001F);
      tft.drawLine(94,120,127,43,0x001F);
      tft.drawLine(127,43,193,197,0x001F);
      tft.drawLine(193,197,226,120,0x001F);
      tft.drawLine(226,120,260,120,0x001F);
      delay(200);

   }
}
