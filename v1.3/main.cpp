/**************************************************************************
 * NTP based Watch dislay for DSTRIKE ESP8266 Deauther Watch
 * 
 * Simple software to show Time and Date on the OLED display
 * The system time can be adjusted by fetching the time from a NTP server
 * In addition to that, the system, that is adjusted by NTP, can be 
 * compared with the actual NTP time without adjusting the system time
 * again. It's interesting to see how quickly times diverge without hardware RTC.
 * Also the system Up-Time can be displayed.
 * That's all. This is the only thing the software can do!
 * 
 * Hague Nusseck @ electricidea
 * v1.3 24.April.2020
 * https://github.com/electricidea/xxxx
 * 
 * Changelog:
 * v1.3 = - first published version
 * 
 * Distributed as-is; no warranty is given.
**************************************************************************/

#include <Arduino.h>

// the DSTIKE Hardware is based on a ESP8266 chip
// for ESP8266: include ESP8266WiFi.h
#include <ESP8266WiFi.h>

// WiFi network configuration for multiple locations:
// A simple method to configure multiple WiFi Access Configurations:
// Add the SSID and the password to the lists.
// IMPORTANT: keep both arrays with the same length!
//String WIFI_ssid[]     = {"Home_ssid", "Work_ssid", "Mobile_ssid", "Best-Friend_ssid"};
//String WIFI_password[] = {"Home_pwd",  "Work_pwd",  "Mobile_pwd",  "Best-Friend_pwd"};

String WiFI_Locations[][3] = {{"Mobile", "Xperia Z5 Dual_1280", "HagueSony"},
                              {"Home", "FingerWechNetGast", "ReinDa2020"},
                              {"Work", "BEC-Gast", "t027-jbsg-m8gk"},
                              {"Pub", "Beer4Free", "DontDrinkAndDrive"}};

// Library for basic functions of the DSTIKE Hardware
#include "Watch.h"

// Small NTP Time Server library for ESP8266
#include "NTPtimeESP.h"
// see: https://platformio.org/lib/show/2057/NTPtimeESP
// Install:
// pio lib install "NTPtimeESP"
// Note:
// The NTP time stamps encode the seconds since 1 January 1900 00:00:00
// The NTPtimeESP library provide a epochTime as UNIX time
//
// The structure strDateTime contains following fields:
//    byte hour;
//    byte minute;
//    byte second;
//    int year;
//    byte month;
//    byte day;
//    byte dayofWeek;
//    unsigned long epochTime;
//    boolean valid;

NTPtime NTPch("ch.pool.ntp.org");

// Time zone in floating point (for India); second parameter: 1 for European summer time; 2 for US daylight saving time
// Time zone is the "Time shift" in hours to UTC (Coordinated Universal Time)
// 0.0  = UTC or formally GMT (Greenwch MEan Time)
// +1.0 = Central European Time, CET 
// remember: CET - Daylight saving time or Central European Summer Time (CEST/CEDT) add another hour!
// +2.0 = Eastern European Time (EET)
// +10.0 = Australian Eastern Standard Time (AEST)
// -5.0 = Eastern Standard Time (EST)
float timezone=+1.0;



// library to handle times in seconds, minutes and so on...
#include <TimeLib.h>
// Install:
// pio lib install "Time"
// lib_deps = Time
// Note:
// now() is returning UNIX time (epochTime).  
// That is, the number of seconds that have passed since 12:00 AM Jan 1, 1970
time_t actualTime;
time_t NTPdiffTime = 0; // example: 1014409342 = 22.02.2002 20:22
uint8_t last_second;
uint8_t last_minute;
strDateTime dateTime;

// German
//const char dayNames[7][10]={"So","Mo","Di","Mi","Do","Fr","Sa"};
//const char monthNames[12][6]={"Jan","Feb","Mar","Apr","Mai","Jun","Jul","Aug","Sep","Okt","Nov","Dez"};

// English
const char dayNames[7][10]={"Sun","Mo","Tue","Wed","Thu","Fri","Sat"};
const char monthNames[12][6]={"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};

// variable to establish a display OFF-Timer
const unsigned long displayTimeout = 10*1000; // 10 seconds
unsigned long displayOffTimer = 0;

/****** function forward declaration ******/
bool WiFi_connection(bool force_reconnect = false);
bool connect_Wifi(const char * _name, const char * _ssid, const char * _password);
void print_dateTime(time_t _dateTime, bool refreshAll);


void setup() {
  // init DSTRIKE Watch
  Watch.begin();
  delay(1000);

  // print Welcome screen over Serial connection
  Serial.println("");
  Serial.println("-----------------------");
  Serial.println("-- DSTIKE NTP Watch  --");
  Serial.println("-- v1.3 / 24.04.2020 --");
  Serial.println("-----------------------");
  // turn the White LED on for half of a second
  Watch.WhiteLED.cycle(100);
  delay(1000);
  // Blink the RGB LED in RED, GREEN, BLUE and white
  // in reduced brightness
  Watch.RGBLED.cycle(10, 250);
  // SHow Welcome Screen
  Watch.setTextAlignment(TEXT_ALIGN_CENTER);
  Watch.setFont(FONT_1_NORMAL);
  Watch.drawString(OLED_CENTER_W, OLED_Line_1,  "NTP Watch");
  Watch.setFont(ArialMT_Plain_10);
  Watch.drawString(OLED_CENTER_W, OLED_Line_3, "Version 1.3");
  Watch.updateDisplay();
  Watch.setFont(FONT_1_NORMAL);
  Watch.setTextAlignment(TEXT_ALIGN_LEFT);
  delay(3000);
  // Check and establish WiFi connection 
  Watch.clearScreen();
  Watch.println("WiFi connection");
  if(WiFi_connection()){
    Watch.println("");
    Watch.println("+ WiFi Connected");
  } else {
    Watch.println("");
    Watch.println("- NO WiFi!");
  }
  // to trigger the seconds-loop every second
  last_second = second(now());
  // to trigger the minutes.loop right now
  last_minute = 100;
  delay(3000);
  // to switch the display off after the specified time
  displayOffTimer = millis();
}


void loop() {
  Watch.updateButtons();

  // get actual time
  actualTime = now()+NTPdiffTime;

  // display OFF timer
  // millis() will overflow after round about 49 days
  // better calculate the absolute value o fthe difference
  if(Watch.screenState && abs(displayOffTimer - millis()) > displayTimeout){
    Watch.screenOff();
  }

  // trigger every second:
  if (second(actualTime) != last_second) {
    last_second = second(actualTime);
    // to prevent a flicker of the display every second
    // only the area of the seconds is updated every second
    // every minute, the hole display is updated
    if(minute(actualTime) != last_minute){
      last_minute= minute(actualTime);
      print_dateTime(actualTime, true);
    } else
      print_dateTime(actualTime, false);
  }

  /***** Nav Button UP *****/
  if(Watch.NavBtn_UP.wasPressed()){
    if(!Watch.screenState){
      Watch.screenOn();
      // delay to prevent false button presses
      delay(250);
    } else{
      // check if connected to the Internet
      if(!WiFi_connection()){
        Watch.clearScreen();
        Watch.println("");
        Watch.println("- NO WiFi");
      } else {
        Watch.clearScreen();
        Watch.println("Compare Time");
        // get the actual time from NTP Server
        int nTrials = 0;
        do {
          Watch.OLED.drawProgressBar(5, OLED_HEIGHT-16, OLED_WIDTH-5, 14, nTrials);
          Watch.updateDisplay();
          nTrials++;
          delay(25);
          dateTime=NTPch.getNTPtime(timezone, 1);
        } while (!dateTime.valid && nTrials < 100);
        Watch.clearScreen();
        Watch.println("Compare Time");
        // compare with system time
        if(dateTime.valid){
          char TextBuffer[100];
          Watch.println("");
          Watch.println("Time difference:");
          Watch.println("");
          sprintf(TextBuffer, "-->    %lds", (now()+NTPdiffTime)-dateTime.epochTime);
          Watch.println(String(TextBuffer));
        } else {
          Watch.println("");
          Watch.println("- invalid data");
        }
      }
      // to trigger the full screen update right now
      last_minute = 100;
      delay(2500);
    }
    displayOffTimer = millis();
  }
  
  /***** Nav Button PUSH *****/
  if(Watch.NavBtn_PUSH.wasPressed()){
    if(!Watch.screenState){
      Watch.screenOn();
      // delay to prevent false button presses
      delay(250);
    } else{
      Watch.clearScreen();
      Watch.drawString(0, OLED_Line_1,  "UP-Time:");
      // get the system up-time in milliseconds
      time_t UpTime = now();
      char TextBuffer[100];
      Watch.setTextAlignment(TEXT_ALIGN_CENTER);
      sprintf(TextBuffer, "%i days", trunc(UpTime/(1000*60*60*24))); // ms -> s -> min -> h -> days
      Watch.drawString(OLED_CENTER_W, OLED_Line_3,  String(TextBuffer));
      sprintf(TextBuffer, "%02d:%02d:%02d", hour(UpTime), minute(UpTime), second(UpTime));
      Watch.drawString(OLED_CENTER_W, OLED_Line_4,  String(TextBuffer));
      Watch.updateDisplay();
      Watch.setTextAlignment(TEXT_ALIGN_LEFT);
      // to trigger the full screen update
      last_minute = 100;
      delay(2500);
    }
    displayOffTimer = millis();
  }

  /***** Nav Button DOWN *****/
  if(Watch.NavBtn_DOWN.wasPressed()){
    if(!Watch.screenState){
      Watch.screenOn();
      // delay to prevent false button presses
      delay(250);
    } else {
      // check if connected to the Internet
      if(!WiFi_connection()){
        Watch.clearScreen();
        Watch.println("");
        Watch.println("- NO WiFi");
      } else {
        Watch.clearScreen();
        Watch.drawString(0, OLED_Line_1,  "Get Server Time");
        Watch.updateDisplay();
        // get the actual time from NTP Server
        int nTrials = 0;
        do {
          Watch.OLED.drawProgressBar(5, OLED_HEIGHT-16, OLED_WIDTH-5, 14, nTrials);
          Watch.updateDisplay();
          nTrials++;
          delay(25);
          dateTime=NTPch.getNTPtime(timezone, 1);
        } while (!dateTime.valid && nTrials < 100);
        Watch.clearScreen();
        Watch.drawString(0, OLED_Line_1,  "Get Server Time");
        if(dateTime.valid){
          // calculate the difference to the NTP Server time
          NTPdiffTime = dateTime.epochTime-now();
          // Shoe the NTP Server time
          String timeString = "";
          if(dateTime.hour<10)
            timeString += ' ';
          timeString += String(dateTime.hour)+':';
          if(dateTime.minute<10)
            timeString += '0';
          timeString += String(dateTime.minute);
          Watch.setFont(FONT_2_LARGE);
          Watch.setTextAlignment(TEXT_ALIGN_CENTER);
          Watch.drawString(64, OLED_Line_3,timeString);
          Watch.setFont(FONT_1_NORMAL);
          Watch.drawString(64, OLED_Line_5,"Time was updated");
          Watch.updateDisplay();
          Watch.setTextAlignment(TEXT_ALIGN_LEFT);
        } else {
          Watch.println("");
          Watch.println("- invalid data");
        }
      }
      // to trigger the full screen update
      last_minute = 100;
      delay(2500);
    }
    displayOffTimer = millis();
  }
  // short delay to calm the watchdog
  delay(10);
}


//==============================================================
// check the actual WiFi connection 
// if not connected, a connection attempt is started otherwise not!
// With "force_reconnect", a new connection will always be established
// The routine tries 5 times to establish a connection to an AP out of 
// the list of WiFi locations. 
// return value: true if connected. otherwise: false
bool WiFi_connection(bool force_reconnect){
  if(WiFi.status() != WL_CONNECTED || force_reconnect){
    Watch.clearScreen();
    Watch.println("- No WiFi");
    delay(1000);
    // fist always disconnect
    WiFi.disconnect();
    // Indicate the WiFi locations out of the list
    int WIFI_location = 0;
    // It should only attempt to establish a connection 3 times.
    // so, the function will go throw the list of WiFi loactions 3 times
    // and try to establish a connection...
    int attempt_counter = 0;
    while(WiFi.status() != WL_CONNECTED && attempt_counter <3){
      Watch.clearScreen();
      // call the connection function with the specific WiFi data
      connect_Wifi(WiFI_Locations[WIFI_location][0].c_str(), 
                   WiFI_Locations[WIFI_location][1].c_str(), 
                   WiFI_Locations[WIFI_location][2].c_str());
      if(WiFi.status() != WL_CONNECTED){
        delay(1000);
        WIFI_location++;
        if(WIFI_location >= (sizeof(WiFI_Locations)/sizeof(WiFI_Locations[0]))){
          WIFI_location = 0;
          attempt_counter++;
        }
      }
    }
  }
  // return true, if connected
  return WiFi.status() == WL_CONNECTED;
}

//==============================================================
// establish the connection to an Wifi Access point
// the function waits 30 seconds (15*2000ms) before giving up.
bool connect_Wifi(const char * _name, const char * _ssid, const char * _password){
  // Establish connection to the specified network until success.
  // Important to disconnect in case that there is a valid connection
  WiFi.disconnect();
  Watch.println("Connecting to ");
  Watch.println(_name);
  delay(1500);
  //Start connecting (done by the ESP in the background)
  WiFi.begin(_ssid, _password);
  // read wifi Status
  wl_status_t wifi_Status = WiFi.status();  
  int n_trials = 0;
  // loop while waiting for Wifi connection
  // run only for 15 trials.
  while (wifi_Status != WL_CONNECTED && n_trials < 15 && wifi_Status != WL_NO_SSID_AVAIL) {
    // Check periodicaly the connection status using WiFi.status()
    // Keep checking until ESP has successfuly connected
    // or maximum number of trials is reached
    wifi_Status = WiFi.status();
    n_trials++;
    switch(wifi_Status){
      case WL_NO_SSID_AVAIL:
          Watch.println("-SSID unavailable");
          break;
      case WL_CONNECT_FAILED:
          Watch.println("-Connection failed");
          break;
      case WL_CONNECTION_LOST:
          Watch.println("-Connection lost");
          break;
      case WL_DISCONNECTED:
          Watch.println("Connection attempt");
          break;
      case WL_IDLE_STATUS:
          Watch.println("-WiFi idle status");
          break;
      case WL_SCAN_COMPLETED:
          Watch.println("+scan completed");
          break;
      case WL_CONNECTED:
          Watch.println("+ WiFi connected");
          break;
      default:
          Watch.println("-unknown Status");
          break;
    }
    // wait a long time between attemts
    if(wifi_Status == WL_DISCONNECTED)
      delay(2000);
    else
      // but only a short time in case of other faulure reasons
      delay(250);
  }
  if(wifi_Status == WL_CONNECTED){
    // connected
    Watch.clearScreen();
    Watch.println("IP address: ");
    Watch.println(WiFi.localIP().toString());
    return true;
  } else {
    // not connected
    Watch.println("");
    Watch.println("-unable to connect");
    return false;
  }
}


//==============================================================
// Print the time and date on the display
// To prevent a flicker of the display every second, only the
// part of the screen that shows the seconds is refreshed
// The parameter refreshAll=true will force a complete update
void print_dateTime(time_t _dateTime, bool refreshAll){
    // create the String for the hours and minutes
    String timeString = "";
    // check for leading zero
    if(hour(_dateTime)<10)
      timeString += '0';
    timeString += String(hour(_dateTime))+':';
    if(minute(_dateTime)<10)
      timeString += '0';
    timeString += String(minute(_dateTime));
    // create the String for the seconds
    String secondsString = ": ";
    if(second(_dateTime)<10)
      secondsString += '0';
    secondsString += String(second(_dateTime));
    // create the String for the date including week names and month names
    String dateString = String(dayNames[weekday(_dateTime)-1])+' ';
    dateString += String(day(_dateTime))+'.';
    dateString += String(monthNames[month(_dateTime)-1])+'.';
    dateString += String(year(_dateTime));
    // if the entrire screen should be updated
    if(refreshAll){
      Watch.clearScreen();
      Watch.setFont(FONT_2_XLARGE);
      Watch.setTextAlignment(TEXT_ALIGN_RIGHT);
      Watch.drawString(96, 0, timeString);
      Watch.setFont(FONT_2_NORMAL);
      Watch.setTextAlignment(TEXT_ALIGN_LEFT);
      Watch.drawString(98, 21, secondsString);
      Watch.setFont(FONT_2_SMALL);
      Watch.setTextAlignment(TEXT_ALIGN_CENTER);
      Watch.drawString(OLED_CENTER_W, OLED_HEIGHT-18, dateString);
      Watch.updateDisplay();
    } else {
      // or only the part with the seconds
      Watch.OLED.setColor(BLACK);
      Watch.OLED.fillRect(98, 21, 128-98, 17);
      Watch.OLED.setColor(WHITE);
      Watch.setFont(FONT_2_NORMAL);
      Watch.setTextAlignment(TEXT_ALIGN_LEFT);
      Watch.drawString(98, 21, secondsString);
      Watch.updateDisplay();
    }
    // default font and alignment for other text outputs
    Watch.setFont(FONT_1_NORMAL);
    Watch.setTextAlignment(TEXT_ALIGN_LEFT);
}
