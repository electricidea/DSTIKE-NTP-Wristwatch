/**************************************************************************
 * NTP based Watch dislay for DSTRIKE ESP8266 Deauther Watch
 * 
 * Simple software to show Time and Date on the OLED display
 * The time can be adjusted by fetching the time from a NTP server.
 * In addition to that, the time, that is adjusted by NTP, can be 
 * compared with the actual NTP time without adjusting the time again.
 * It's interesting to see how quickly times diverge without hardware RTC.
 * Also the system Up-Time can be displayed.
 * That's all. This is the only thing the software can do!
 * 
 * For details about the NTP function, see: https://youtu.be/r2UAmBLBBRM 
 * 
 * Hague Nusseck @ electricidea
 * v2.2 01.November.2020
 * https://github.com/electricidea/DSTIKE-NTP-Wristwatch
 * 
 * Changelog:
 * v1.3 = - final version based on the NTPtimeESP.h and Timelib.h
 * v2.0 = - first version with the ESP-Library Time functions (Time.h)
 * v2.1 = - added WiFi refresh if Nav-Button is pressed during start up
 * v2.2 = - Screen Timer can be switched off / on by pressing the NAV-
 *          Button again when displaying the UP-Time.
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
// The first entry is a name that is shown on the display during connecting
/*
String WiFI_Locations[][3] = {{"Mobile", "Mobile_ssid", "Mobile_pwd"},
                              {"Home", "Home_ssid", "Home_pwd"},
                              {"Work", "Work_ssid", "Work_pwd"},
                              {"Pub", "Beer4Free", "DontDrinkAndDrive"}};
*/
String WiFI_Locations[][3] = {{"ESP-AP", "ESP32-AP2", "123456789"},
                              {"Mobile", "Xperia Z5 Dual_1280", "HagueSony"},
                              {"Home", "FingerWechNetGast", "ReinDa2020"},
                              {"Work", "BEC-Gast", "t027-jbsg-m8gk"},
                              {"Pub", "Beer4Free", "DontDrinkAndDrive"}};

// Library for basic functions of the DSTIKE Hardware
#include "Watch.h"

// library to handle times in seconds, minutes and so on...
#include <Time.h>

// network address of the Time Server
const char* NTP_SERVER = "ch.pool.ntp.org";
// time zone for Germany
// see: https://remotemonitoringsystems.ca/time-zone-abbreviations.php
// and: https://www.gnu.org/software/libc/manual/html_node/TZ-Variable.html
const char* TZ_INFO    = "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00"; 

// the tm structure contains the following data:
//
//  int	tm_sec;   --> 0 .. 59
//  int	tm_min;   --> 0 .. 59
//  int	tm_hour;  --> 0 .. 23
//  int	tm_mday;  --> 1 .. 31
//  int	tm_mon;   --> 0 .. 11 (0 = January)
//  int	tm_year;  --> years since 1900
//  int	tm_wday;  --> 0 .. 6 (0 = Sunday)
//  int	tm_yday;  --> 0 .. 365
//  int	tm_isdst; --> Daylight Saving Time flag
//
tm dateTime;

time_t actualTime;      // epoch of the current time.
time_t NTPTime;         // epoch from the NTP Server
time_t NTPdiffTime = 0; // example: 1014409342 = 22.02.2002 20:22
uint8_t last_second;
uint8_t last_minute;

// Screen flag to let the scrren stay perment on or not
bool Screen_permanent_on = false;

// variabled to establish a system time in seconds since boot up.
uint32_t prevMillis = 0;
uint32_t sysTime;

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
void print_dateTime(time_t epochTime, bool refreshAll);
time_t sysTime_now();


void setup() {
  // init DSTRIKE Watch
  Watch.begin();
  delay(1000);

  // print Welcome screen over Serial connection
  Serial.println("");
  Serial.println("-----------------------");
  Serial.println("-- DSTIKE NTP Watch  --");
  Serial.println("-- v2.2 / 07.11.2020 --");
  Serial.println("-----------------------");
  // turn the White LED on for half of a second
  Watch.WhiteLED.cycle(100);
  Serial.println("[OK] cycle white LED");
  delay(100);
  // Blink the RGB LED in RED, GREEN, BLUE and white
  // with reduced brightness
  Watch.RGBLED.cycle(10, 100);
  Serial.println("[OK] cycle RGB LED");
  // Show Welcome Screen
  Watch.setTextAlignment(TEXT_ALIGN_CENTER);
  Watch.setFont(FONT_1_NORMAL);
  Serial.println("[OK] Set Font");
  Watch.drawString(OLED_CENTER_W, OLED_Line_1,  "NTP Watch");
  Serial.println("[OK] draw Text");
  Watch.setFont(ArialMT_Plain_10);
  Serial.println("[OK] Set Font");
  Watch.drawString(OLED_CENTER_W, OLED_Line_3, "Version 2.2");
  Serial.println("[OK] draw Text");
  Watch.updateDisplay();
  Serial.println("[OK] update Display");
  Watch.setFont(FONT_1_NORMAL);
  Watch.setTextAlignment(TEXT_ALIGN_LEFT);
  delay(3000);
  // Check and establish WiFi connection 
  Watch.clearScreen();
  Watch.println("WiFi connection");
  // if the Nav-Button is pressed during boot,
  // a WiFI reconnection is forced
  bool force_WiFi_refresh = false;
  Watch.updateButtons();
  if(Watch.NavBtn_PUSH.wasPressed()) 
    force_WiFi_refresh = true;
  if(WiFi_connection(force_WiFi_refresh)){
    Watch.println("");
    Watch.println("+ WiFi Connected");
  } else {
    Watch.println("");
    Watch.println("- NO WiFi!");
  }
  // configure the NTP Server
  configTime(0, 0, NTP_SERVER);
  // define the timezone (POSIX)
  // set TZ environment variable to the correct value depending on your location
  // see: https://www.gnu.org/software/libc/manual/html_node/TZ-Variable.html
  // Note:
  // this will also change the display at reboot.
  // Comment this line to get UTC
  setenv("TZ", TZ_INFO, 1);
  // to trigger the seconds-loop every second
  last_second = 0;
  // to trigger the minutes.loop
  last_minute = 100;
  delay(3000);
  // to switch the display off after the specified time
  displayOffTimer = millis();
}


void loop() {
  Watch.updateButtons();

  // get actual time as epoch time
  actualTime = sysTime_now()+NTPdiffTime;
  // converts the epoch into the tm-structure
  localtime_r(&actualTime, &dateTime);

  // display OFF timer
  // millis() will overflow after round about 49 days
  // better calculate the absolute value of the difference
  if(!Screen_permanent_on){
    if(Watch.screenState && abs(int(displayOffTimer - millis())) > displayTimeout){
      Watch.screenOff();
    }
  }

  // trigger every second:
  if (dateTime.tm_sec != last_second) {
    last_second = dateTime.tm_sec;
    // to prevent a flicker of the display every second
    // only the area of the seconds is updated every second
    // every minute, the hole display is updated
    if(dateTime.tm_min != last_minute){
      last_minute= dateTime.tm_min;
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
          Watch.OLED.drawProgressBar(5, OLED_HEIGHT-16, OLED_WIDTH-10, 14, nTrials);
          Watch.updateDisplay();
          nTrials++;
          delay(25);
          // time() fetch the actual time from the NTP server 
          // and store it as epoch into the variable
          time(&NTPTime);
          // localtime_r() converts the epoch into the tm-structure
          localtime_r(&NTPTime, &dateTime);
          // tm_year can be used to indicate if the data is valid
        } while ((dateTime.tm_year < (2016 - 1900)) && nTrials < 100);
        Watch.clearScreen();
        Watch.println("Compare Time");
        // compare with system time if data is valid
        if(dateTime.tm_year > (2016 - 1900)){
          char TextBuffer[100];
          Watch.println("");
          Watch.println("Time difference:");
          Watch.println("");
          sprintf(TextBuffer, "-->    %lds", (sysTime_now()+NTPdiffTime)-NTPTime);
          Watch.println(String(TextBuffer));
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
  
  /***** Nav Button PUSH *****/
  if(Watch.NavBtn_PUSH.wasPressed()){
    if(!Watch.screenState){
      Watch.screenOn();
      // delay to prevent false button presses
      delay(250);
    } else{
      Watch.clearScreen();
      Watch.drawString(0, OLED_Line_1,  "UP-Time:");
      // get the system up-time in seconds
      time_t UpTime = sysTime_now();
      // the uptime is calculated directly out of the seconds since start
      // to test, here are some known values:
      // UpTime = 93784; // Friday, 2. January 1970 02:03:04 ==> UpTime 1day, 2h 3min 4sec
      // UpTime = 1264577; // Thursday, 15. January 1970 15:16:17 ==> UpTime 14day, 15h 16min 17sec
      // UpTime = 63158399; // Saturday, 1. January 1972 23:59:59 ==> UpTime 730day, 23h 59min 59sec
      char TextBuffer[100];
      Watch.setTextAlignment(TEXT_ALIGN_CENTER);
      sprintf(TextBuffer, "%i days", int(trunc(UpTime/(60*60*24)))); // s -> min -> h -> days
      Watch.drawString(OLED_CENTER_W, OLED_Line_3,  String(TextBuffer));
      sprintf(TextBuffer, "%02d:%02d:%02d", int(trunc(UpTime/(60*60))) % 24, 
                                            int(trunc(UpTime/(60))) % 60, 
                                            int(UpTime) % 60);
      Watch.drawString(OLED_CENTER_W, OLED_Line_4,  String(TextBuffer));
      Watch.updateDisplay();
      Watch.setTextAlignment(TEXT_ALIGN_LEFT);
      // to trigger the full screen update
      last_minute = 100;
      // diplay the uptime for 2.3 seconds
      // check if the button is pushed agian
      // then togle the display-timeout flag
      unsigned long display_timer = millis();
      while((display_timer + 2500) > millis()){
        Watch.updateButtons();
        if(Watch.NavBtn_PUSH.wasPressed()){
          Screen_permanent_on = !Screen_permanent_on;
          display_timer = millis();
          Watch.clearScreen();
          Watch.setTextAlignment(TEXT_ALIGN_CENTER);
          if(Screen_permanent_on){
            Watch.drawString(OLED_CENTER_W, OLED_Line_3,  "Screen Timer: OFF");
          } else {
            Watch.drawString(OLED_CENTER_W, OLED_Line_3,  "Screen Timer: ON");
          }
          Watch.updateDisplay();
          Watch.setTextAlignment(TEXT_ALIGN_LEFT);
        }
        // short delay to calm the watchdog
        delay(10);
      }
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
          Watch.OLED.drawProgressBar(5, OLED_HEIGHT-16, OLED_WIDTH-10, 14, nTrials);
          Watch.updateDisplay();
          nTrials++;
          delay(25);
          // time() fetch the actual time from the NTP server 
          // and store it as epoch into the variable
          time(&NTPTime);
          // localtime_r() converts the epoch into the tm-structure
          localtime_r(&NTPTime, &dateTime);
          // tm_year can be used to indicate if the data is valid
        } while ((dateTime.tm_year < (2016 - 1900)) && nTrials < 100);
        Watch.clearScreen();
        Watch.drawString(0, OLED_Line_1,  "Get Server Time");
        if(dateTime.tm_year > (2016 - 1900)){
          // calculate the difference to the NTP Server time
          NTPdiffTime = NTPTime-sysTime_now();
          // Show the NTP Server time          
          char TextBuffer[100];
          sprintf(TextBuffer, "%02d:%02d", dateTime.tm_hour, dateTime.tm_min);
          Watch.setFont(FONT_2_LARGE);
          Watch.setTextAlignment(TEXT_ALIGN_CENTER);
          Watch.drawString(64, OLED_Line_3,String(TextBuffer));
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
    if(force_reconnect)
      Watch.println("- Refresh WiFi");
    else
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
// Returns the system time in seconds
// THis is the (Up)-Time of the system since last reset
time_t sysTime_now() {
	// calculate number of seconds passed since last call to sysTime_now()
  while (millis() - prevMillis >= 1000) {
		// millis() and prevMillis are both unsigned ints thus the subtraction 
    // will always be the absolute value of the difference
    sysTime++;
    prevMillis += 1000;	
  }  
  return (time_t)sysTime;
}


//==============================================================
// Print the time and date on the display
// To prevent a flicker of the display every second, only the
// part of the screen that shows the seconds is refreshed
// The parameter refreshAll=true will force a complete update
void print_dateTime(time_t epochTime, bool refreshAll){
    tm _dateTime;
    // converts the epoch into the tm-structure
    localtime_r(&epochTime, &_dateTime);
    // create the String for the hours and minutes
    char timeString[100];
    sprintf(timeString, "%02d:%02d", _dateTime.tm_hour, _dateTime.tm_min);
    // create the String for the seconds
    char secondsString[100];
    sprintf(secondsString, ": %02d", _dateTime.tm_sec);
    // create the String for the date including week names and month names
    String dateString = String(dayNames[_dateTime.tm_wday])+' ';
    dateString += String(_dateTime.tm_mday)+'.';
    dateString += String(monthNames[_dateTime.tm_mon])+'.';
    dateString += String(_dateTime.tm_year+1900);
    // if the entrire screen should be updated
    if(refreshAll){
      Watch.clearScreen();
      Watch.setFont(FONT_2_XLARGE);
      Watch.setTextAlignment(TEXT_ALIGN_RIGHT);
      Watch.drawString(96, 0, String(timeString));
      Watch.setFont(FONT_2_NORMAL);
      Watch.setTextAlignment(TEXT_ALIGN_LEFT);
      Watch.drawString(98, 21, String(secondsString));
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
      Watch.drawString(98, 21, String(secondsString));
      Watch.updateDisplay();
    }
    // default font and alignment for other text outputs
    Watch.setFont(FONT_1_NORMAL);
    Watch.setTextAlignment(TEXT_ALIGN_LEFT);
}
