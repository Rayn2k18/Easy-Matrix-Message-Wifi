/*
Project: Wifi controlled LED matrix display
NodeMCU pins    -> EasyMatrix pins
     VU -(+5V)  -> Vcc
          GND   -> GND
MOSI-D7-GPIO13  -> DIN
CLK-D5-GPIO14   -> Clk
CS-D8-GPIO15    -> LOAD / CS

Reset Button (if needed) : 
      Gnd
      Rst
*/
#include <ESP8266WiFi.h>
#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal and control of led matrix
#include <WiFiManager.h>  
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Max72xxPanel.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <EEPROM.h>
//#include "Fonts\Org_01.h"
#include "font.h"

// ------------- NTP DATE / TIME -----------------------

int gmt_h = 2;  // default value for ntp_offset
int ntp_offset = gmt_h*3600; // In seconds

#define NTP_INTERVAL 1800000    // In miliseconds
#define DEBUG_NTPClient ON
const char* ntpServer = "fr.pool.ntp.org";

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
char daysOfTheWeekFR[7][12] = {"Dimanche", "Lundi", "Mardi", "Mercredi", "Jeudi", "Vendredi", "Samedi"};

WiFiManager wifiManager;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, ntpServer, ntp_offset, NTP_INTERVAL);

ESP8266WebServer server(80);                             // HTTP server will listen at port 80
long period;
int offset=1,refresh=0;
int *pRefresh = &refresh;

int pinCS = 15; // Attach CS to this pin, DIN to MOSI and CLK to SCK (cf http://arduino.cc/en/Reference/SPI )
int numberOfHorizontalDisplays = 12;
int numberOfVerticalDisplays = 1;
int max_fill = 15;
String decodedMsg;
String cmd;
int anim = 1;
int *pAnim = &anim;
int intensity = 2;
Max72xxPanel matrix = Max72xxPanel(pinCS, numberOfHorizontalDisplays, numberOfVerticalDisplays);
int wait = 80; // In milliseconds

int showTime = 1;
String THour;
String THourSec;
String TDate;
int displayMode = 2;
int IPRoll = 0;

int spacer = 1;
int width = 5 + spacer; // The font width is 5 pixels

// ******************* String form to sent to the client-browser ************************************
String form =
  "<p>"
  "<center>"
  "<h1>ESP8266 Web Server</h1>"
  "<form action='msg'>"
  " <p>Type your message <input type='text' name='msg' size=100 autofocus> <input type='submit' value='Submit'>"
  "</form>"
  "</center><br><br>"
  "Available GET parameters : <br>"
  " <ul><li>msg : (string) message to display</li>"
  "     <li>anim : (0-2) 0: still, 1: rolling message, 2: back and forth (to do)</li>"
  "     <li>level : (0-15) intensity of leds</li>"
  "     <li>displaymode : (1-3) 1= 'HH:mm' still mode, 2= 'HH:mm:ss' still mode, 3= 'day, HH:mm' rolling mode</li>"
  "     <li>cmd : (string) execute command</li>"
  "         <ul><li><a href='/msg?cmd=restart'>restart</a> : restarts the ESP (equiv to reset button, usefull when display is not complete or blank after boot up)</li>"
  "         <li><a href='/msg?cmd=showtime'>showtime</a> : displays automatically NTP updated time on screen</li>"
  "         <li><a href='/msg?cmd=forcentp'>forcentp</a> : forces NTP client update</li></ul>"
  "         <li><a href='/msg?cmd=resetwifi'>resetwifi</a> : reset wifi connexion info and restarts</li>"
  "         <li><a href='/msg?ntpoffset=2'>ntpoffset</a> : sets ntpoffset (in hour) and store it to eeprom (ex: GMT+2 -> ntpoffset=2 / GMT-10 -> ntpoffset=-10)"
  "         <li><a href='/msg?cmd=off'>off</a> : turn off display</li>"
  "         <li><a href='/msg?cmd=flash'>flash</a> : make the screen blink 2x</li></ul>"
  "  </ul>";
  


void eeWriteInt(int pos, int val) {   // write to eeprom
    EEPROM.begin(512);
    byte* p = (byte*) &val;
    EEPROM.write(pos, *p);
    EEPROM.write(pos + 1, *(p + 1));
    EEPROM.write(pos + 2, *(p + 2));
    EEPROM.write(pos + 3, *(p + 3));
    EEPROM.commit();
    EEPROM.end();
}

int eeGetInt(int pos) {   // read from eeprom
  int val;
  byte* p = (byte*) &val;
  EEPROM.begin(512);
  *p        = EEPROM.read(pos);
  *(p + 1)  = EEPROM.read(pos + 1);
  *(p + 2)  = EEPROM.read(pos + 2);
  *(p + 3)  = EEPROM.read(pos + 3);
  EEPROM.end();
  return val;
}

void ShowTime(int displayMode)  // displays time
{
  String Smin;
  showTime = 1; 
  //timeClient.update();  // update from ntp server OR locally (ntp_offset)
  int minutes = (int)timeClient.getMinutes();
  if (minutes < 10) {
    Smin = (String) "0"+minutes;
  } else {
    Smin = (String) minutes;
  }
  int hours = (int)timeClient.getHours();

  THour = (String) hours + ":" + Smin;
  THourSec = timeClient.getFormattedTime();
  TDate = (String)daysOfTheWeekFR[timeClient.getDay()] + ", " + THour + " ";

  //Serial.print("---- Time : ");
  //Serial.println(TDate);

  //Serial.print("---- Formatted : ");
  //Serial.println(timeClient.getFormattedTime());

  switch (displayMode) {
    case 1:   // show "hh:mm", center, still
      decodedMsg = THour;
      anim = 0;
      break;
    case 2:   // show "hh:mm:ss", center, still
      decodedMsg = THourSec;
      anim = 0;
      break;
    case 3:   // show "day, hh:mm", scrolling
      decodedMsg = TDate;
      anim = 1;
      wait = 40;
      break;
  } 
  //Serial.print("---- MESSAGE : ");
  //Serial.println(decodedMsg);
  

}

void process(String cmd){
  String formCmd =
          "<html>"
          "  <head>"
          "    <meta http-equiv='refresh' content='5;url=/msg?redir=1' />"
          "  </head>"
          "  <body>"
          "     <center>"
          "     <h1>ESP8266 Web Server</h1>"
          "     <span> Processing command "+cmd+"  ...</span><br>"
          "     <h1>Redirecting in 5 seconds...</h1>"
          "     </center>"
          "  </body>"
          "</html>";
  server.send(200, "text/html", formCmd);
  Serial.println((String)"COMMAND : "+cmd);
  
  if (cmd == "restart") {                         // restart the ESP (will erase stored variables such as gmt offset)
    server.send(205, "text/html", formCmd);
    delay(2000);
    cmd = "";
    ESP.restart();
    exit(0);
  }
  if (cmd == "reset") {                         // reset the ESP
    server.send(205, "text/html", formCmd);
    delay(2000);
    cmd = "";
    ESP.reset();
    exit(0);
  }
  if (cmd == "checkboot") {                       // check last boot (for debug)
    cmd = "";
    checkRebootReason();
  }
  if (cmd == "checkstart") {                      // check last boot (for debug)
    cmd = "";
    checkStart();
  }
  if (cmd == "showtime") {                        // displays time (sync by ntp) automatically
    cmd = "";
    ShowTime(displayMode);
  }
  if (cmd == "forcentp") {                        // forces ntp client update then displays time automatically
    cmd = "";
    timeClient.forceUpdate();
    timeClient.setTimeOffset(ntp_offset);
    ShowTime(displayMode);
  }
  if (cmd == "resetwifi") {                         // reset saved wifi settings and restarts
    server.send(205, "text/html", formCmd);
    delay(2000);
    cmd = "";
    wifiManager.resetSettings();
    delay(2000);
    ESP.reset();
    exit(0);
  }
  if (cmd == "off") {                         // reset saved wifi settings and restarts
    decodedMsg = "";
    intensity = 0;
    showTime = 0; 
    anim = 0;
    cmd = "";
  }
  if (cmd == "flash") {                         // flash screen 2x
    int flick_delay = 100;    // can't go above (probably due to tension drop) else display corrupted or blank
    matrix.setIntensity(4); // can't go above (probably due to tension drop) else display corrupted or blank

    //matrix.fillScreen(HIGH); // doesn't work, so, we'll fill screen with a full ON char
    int y = (matrix.height() - 8) / 2;
    char filler = '#';  // full char
    
    for (int i=0; i<max_fill; i++) {
      int x= (i * width);  
      matrix.drawChar(x, y, filler, LOW, HIGH, 1);
    }
    matrix.write();
    delay(flick_delay);
    for (int i=0; i<max_fill; i++) {
      int x= (i * width);  
      matrix.drawChar(x, y, filler, HIGH, LOW, 1);
    }
    matrix.write();
    delay(flick_delay);  
    for (int i=0; i<max_fill; i++) {
      int x= (i * width);  
      matrix.drawChar(x, y, filler, LOW, HIGH, 1);
    }
    matrix.write();
    delay(flick_delay);
    for (int i=0; i<max_fill; i++) {
      int x= (i * width);  
      matrix.drawChar(x, y, filler, HIGH, LOW, 1);
    }  
    matrix.write(); 
    delay(flick_delay);
    for (int i=0; i<max_fill; i++) {
      int x= (i * width);  
      matrix.drawChar(x, y, filler, LOW, HIGH, 1);
    }
    matrix.write();
    delay(flick_delay);
    for (int i=0; i<max_fill; i++) {
      int x= (i * width);  
      matrix.drawChar(x, y, filler, HIGH, LOW, 1);
    }
    matrix.write();
    delay(flick_delay);  
    for (int i=0; i<max_fill; i++) {
      int x= (i * width);  
      matrix.drawChar(x, y, filler, LOW, HIGH, 1);
    }
    matrix.write();
    delay(flick_delay);
    for (int i=0; i<max_fill; i++) {
      int x= (i * width);  
      matrix.drawChar(x, y, filler, HIGH, LOW, 1);
    }  
    matrix.write(); 
    delay(flick_delay);
  
    matrix.setIntensity(intensity);
    wait=80;
    showTime = 1;
    cmd = "";
  }
  
  
   
  //delay(3000);  // display http message for 3s
  
}

void CheckNtpOffset(int valNtp){
  int temp=0;
  ntp_offset = gmt_h*3600;
  if (valNtp != gmt_h) {
    Serial.println((String)" Used GMT OFFSET (before) : "+gmt_h);
    temp = eeGetInt(0);  // read from eeprom (check)
    Serial.println((String)" Stored GMT OFFSET (before) : "+temp);
    if (valNtp >= -23 and valNtp <= 23){    // not incoherent value, write
      gmt_h = valNtp;
      eeWriteInt(0, valNtp);   // store default value
      
    }
    ntp_offset = gmt_h*3600; 
    Serial.println((String)"Changing GMT OFFSET : "+valNtp);
    timeClient.setTimeOffset(ntp_offset);
    temp = eeGetInt(0);  // read from eeprom (check)
    Serial.println((String)"Stored GMT OFFSET Value : "+temp);
  } else {
    Serial.println((String)"Not Changing GMT OFFSET : "+gmt_h+" : SAME AS Stored");
    timeClient.setTimeOffset(ntp_offset);
  }
}

void handle_msg() {
  IPRoll = 5;    // http server reached, so @ip is known, no need to display it anymore                      
  matrix.fillScreen(LOW);
  // Display msg on Oled
  String msg = server.arg("msg");
  String cmd = server.arg("cmd");
  String level = server.arg("level");
  String s_max = server.arg("max");
  String str_anim = server.arg("anim");
  String dispMode = server.arg("displaymode");
  String redir = server.arg("redir");
  String spd = server.arg("speed");
  String ntpoff = server.arg("ntpoffset");

  if(redir == "1") {  // do nothing else than display server page.
    server.send(200, "text/html", form);    // Send same page so they can send another msg
    *pRefresh=0;
    return;
  }

  if (dispMode != "") {
    displayMode = dispMode.toInt();
    Serial.println((String)"Changing displaymode ... "+displayMode);
  }
  intensity = 2;
  if (level != "") {
    intensity = level.toInt();
    Serial.println((String)' Changing intensity ... '+intensity);
  }
  if (s_max != "") {
    max_fill = s_max.toInt();
    Serial.println((String)' Changing max fill ... '+max_fill);
  }
  if (ntpoff != "") {
    CheckNtpOffset( ntpoff.toInt() );
  }
  if (cmd != "") {
    Serial.println((String)"new command ... "+cmd);
    if (cmd == "flash") {
      process(cmd);
      *pRefresh=1;
      return;
    }
    showTime=0; // disable display of time / date
    decodedMsg = cmd;
    process(cmd);
    return; // if cmd is made, do not process msg as usual
  }
  
  if (str_anim != "") {
    *pAnim = str_anim.toInt();
    Serial.println((String)"Changing animation ... "+ *pAnim);
  } else {
    *pAnim = 1;
  }

  wait = 80;
  if (spd != "") {
    wait = spd.toInt();
    if(wait > 1000) wait=1000;
    Serial.println((String)"Changing speed ... "+wait);
  }
  
  if (msg != "") {
    showTime=0; // disable display of time / date
    Serial.println((String)"new message ... "+msg);
  }
  Serial.println((String) "BEFORE : "+msg);
  decodedMsg = msg;
  // Restore special characters that are misformed to %char by the client browser     
  decodedMsg.replace("%21", "!");  
  decodedMsg.replace("%22", "");  
  decodedMsg.replace("%23", "#");
  decodedMsg.replace("%24", "$");
  decodedMsg.replace("%25", "%");  
  decodedMsg.replace("%26", "&");
  decodedMsg.replace("%27", "'");  
  decodedMsg.replace("%28", "(");
  decodedMsg.replace("%29", ")");
  decodedMsg.replace("%2A", "*");
  decodedMsg.replace("%2B", "+");  
  decodedMsg.replace("%2C", ",");  
  decodedMsg.replace("%2F", "/");   
  decodedMsg.replace("%3A", ":");    
  decodedMsg.replace("%3B", ";");  
  decodedMsg.replace("%3C", "<");  
  decodedMsg.replace("%3D", "=");  
  decodedMsg.replace("%3E", ">");
  decodedMsg.replace("%3F", "?");  
  decodedMsg.replace("%40", "@"); 
// conversion from unicode (html side) to CP247 (matrix side)
  decodedMsg.replace((char)176, (char) 247);  // °
  decodedMsg.replace((char)231, (char) 135);  // ç
  decodedMsg.replace((char)233, (char) 130);  // é
  decodedMsg.replace((char)232, (char) 138);  // è
  decodedMsg.replace((char)235, (char) 138);  // ê
  decodedMsg.replace((char)234, (char) 137);  // ë
  decodedMsg.replace((char)224, (char) 133);  // à
  
  decodedMsg.replace((char)239, (char) 139);  // ï
  decodedMsg.replace((char)238, (char) 140);  // î
  decodedMsg.replace((char)246, (char) 148);  // ö
  decodedMsg.replace((char)244, (char) 147);  // ô
  decodedMsg.replace((char)252, (char) 129);  // ü
  decodedMsg.replace((char)251, (char) 150);  // û
  decodedMsg.replace((char)249, (char) 151);  // ù
  
  decodedMsg.replace((char)178, (char) 253);  // ²
  decodedMsg.replace((char)189, (char) 171);  // ½
  decodedMsg.replace((char)188, (char) 172);  // ¼
  decodedMsg.replace((char)183, (char) 249);  // ·
  
  Serial.println((String) "AFTER : "+decodedMsg);                   // print original string to monitor
  Serial.print("ASCII : "); 
  for( int c=0; c<decodedMsg.length(); c++) {
    Serial.print((int)decodedMsg[c]);
    Serial.print(' ');
  }
  Serial.println("[end]"); 
 
   server.send(200, "text/html", (String)form+"<br><br>LAST MESSAGE :<br><pre>"+msg+"</pre><br><pre>"+decodedMsg+"</pre>");    // Send same page so they can send another msg
  *pRefresh=1;
    
  //Serial.println(' ');                          // new line in monitor
}

int checkRebootReason() {
  Serial.println("reboot :");
  rst_info *myResetInfo;
  myResetInfo = ESP.getResetInfoPtr();
  Serial.println("Reason_fn " + String(ESP.getResetReason()));
  Serial.println("Reason_ptr " + String((*myResetInfo).reason));
  String reason = String((*myResetInfo).reason);
  return reason.toInt();
}

void checkStart() {
  int boot = checkRebootReason();
  if (boot != 4) {
    Serial.println("Rebooting in 5s (bugfix)");
    delay(5000);
    ESP.restart();
  }
  Serial.println('==> ');
  Serial.println(boot);
}

void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
  decodedMsg = (String) "AP Mode, SSID : "+ myWiFiManager->getConfigPortalSSID()+" @IP : 192.168.4.1 ";
  Scrolling(decodedMsg);
}

void Scrolling(String decodedMsg) {
  for ( int i = 0 ; i < width * decodedMsg.length() + matrix.width() - 1 - spacer; i++ ) {    // boucle scrolling
    server.handleClient();
    if (*pRefresh==1) {
      //i=0;
      //matrix.setIntensity(intensity);
      *pRefresh=0;
      break;
    }
    
    if (*pAnim == 0) break;
    int letter = i / width;
    int x = (matrix.width() - 1) - i % width;
    int y = (matrix.height() - 8) / 2; // center the text vertically
    
    while ( x + width - spacer >= 0 && letter >= 0 ) {
      if ( letter < decodedMsg.length() ) {
        matrix.drawChar(x, y, decodedMsg[letter], HIGH, LOW, 1);
      }
    
      letter--;
      x -= width;
    }
    matrix.write(); // Send bitmap to display
    delay(wait);
  }
}

void BackForth(String decodedMsg) {
  decodedMsg = decodedMsg+"  ";
  Scrolling(decodedMsg);
  // todo : send text in the opposite direction
}

void Still(String decodedMsg) {
   server.handleClient();                        // checks for incoming messages
   for(int j=0; j < decodedMsg.length(); j++ )
    {
        int nb_space = ( (matrix.width()- (width*decodedMsg.length()) )/2 );
        if (nb_space < 0) nb_space =0;
        //Serial.println(nb_space);
        int x= (j * width) + nb_space;  // step horizontal by width of character and spacer
        int y = (matrix.height() - 8) / 2; // center the text vertically
    
        matrix.drawChar(x, y, decodedMsg[ j ], HIGH, LOW, 1);   // draw a character from 'tape'
    }
    matrix.write(); // Send bitmap to display
    delay(wait);
}

void setup(void) {
  Serial.begin(115200);                           // full speed to monitor
  gmt_h = eeGetInt(0);  // read eeprom
  //delay(10000); // debug
  Serial.println((String)" GMT OFFSET : "+gmt_h);
  if (gmt_h < -23 or gmt_h > 23){ // no value or incoherent value stored, rewrite
    Serial.println("gmt value error, rewiriting default ...");
    eeWriteInt(0, 2);   // store default value
    gmt_h = 2;
  }
  
  ntp_offset = gmt_h*3600; // In seconds
  Serial.println((String)"NTP OFFSET : "+ntp_offset+" s");
  //matrix.setFont(&Org_01); // doesn't work
  matrix.setIntensity(intensity); // Use a value between 0 and 15 for brightness

  Serial.println((String)"width : "+width+", matrix width : "+matrix.width()+", spacer : "+spacer+" total :"+(width + matrix.width() - 1 - spacer));
  Serial.println((String)"matrix height : "+matrix.height());
  
  // Adjust to your own needs
  //  matrix.setPosition(0, 1, 0); // The first display is at <0, 0>
  //  matrix.setPosition(1, 0, 0); // The second display is at <1, 0>
  
  // Adjust to your own needs
  /*  
    matrix.setPosition(0, 7, 0); // The first display is at <0, 7>
    matrix.setPosition(1, 6, 0); // The second display is at <1, 0>
    matrix.setPosition(2, 5, 0); // The third display is at <2, 0>
    matrix.setPosition(3, 4, 0); // And the last display is at <3, 0>
    matrix.setPosition(4, 3, 0); // The first display is at <0, 0>
    matrix.setPosition(5, 2, 0); // The second display is at <1, 0>
    matrix.setPosition(6, 1, 0); // The third display is at <2, 0>
    matrix.setPosition(7, 0, 0); // And the last display is at <3, 0>
   */
  
  matrix.setRotation(0, 1);
  matrix.setRotation(1, 1);
  matrix.setRotation(2, 1);
  matrix.setRotation(3, 1);
  matrix.setRotation(4, 1);
  matrix.setRotation(5, 1);
  matrix.setRotation(6, 1);
  matrix.setRotation(7, 1);
  matrix.setRotation(8, 1);
  matrix.setRotation(9, 1);
  matrix.setRotation(10, 1);
  matrix.setRotation(11, 1);
  matrix.setRotation(12, 1);
                                 
  // ----- WIFI CONNEXION --------------------
  wifiManager.setAPCallback(configModeCallback);
  wifiManager.setConfigPortalTimeout(120); // shows wifi config page for 2min then starts normally on wifi connect fail (or not defined wifi connexion)
  wifiManager.autoConnect();
  
  Serial.println("wifi connexion : OK !");
  // ----------- END WIFI Section ------------
  // Set up the endpoints for HTTP server,  Endpoints can be written as inline functions:
  server.on("/", []() {
    server.send(200, "text/html", form);
  });
  server.on("/msg", handle_msg);                  // And as regular external functions:
  server.begin();                                 // Start the server 
          
 
  char result[16];
  sprintf(result, "%3d.%3d.%1d.%3d", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3]);
  Serial.println();
  Serial.println(result);
  if (WiFi.status() == WL_CONNECTED) decodedMsg = (String)result+" ";
  Serial.println("WebServer ready!   ");

  Serial.println(WiFi.localIP());                 // Serial monitor prints localIP
  //Serial.print(analogRead(A0));

  //init and get the time
  timeClient.begin();
  timeClient.setUpdateInterval(NTP_INTERVAL);
  timeClient.setTimeOffset(ntp_offset);
  timeClient.update();
}


void loop(void) {
  if (IPRoll < 5) Serial.println((String)"displaying @IP "+decodedMsg+", nb Roll : "+IPRoll);
  if (showTime == 1 and IPRoll > 4) {          // displays time automatically ONLY if uptime > 10s (display @IP before)
      timeClient.update();                      // update from ntp server OR locally (using NTP_INTERVAL)
      ShowTime(displayMode);
  }
    
    matrix.setIntensity(intensity);
    matrix.fillScreen(LOW);

    switch (*pAnim) {
      case 0:
        Still(decodedMsg);
        break;
      case 1:
        Scrolling(decodedMsg);
        break;
      default:
        BackForth(decodedMsg);
        break;
    }
 
    
  if (showTime == 1) {
     delay(1000-wait);
  }
  if(IPRoll < 5)IPRoll++;
}
