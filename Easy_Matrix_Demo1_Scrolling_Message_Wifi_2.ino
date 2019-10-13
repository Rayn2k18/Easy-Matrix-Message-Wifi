/*
Project: Wifi controlled LED matrix display
NodeMCU pins    -> EasyMatrix pins
MOSI-D7-GPIO13  -> DIN
CLK-D5-GPIO14   -> Clk
GPIO0-D3        -> LOAD

*/

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Max72xxPanel.h>

#define SSID "your ssid"                      // insert your SSID
#define PASS "*********"                    // insert your password
// ******************* String form to sent to the client-browser ************************************
String form =
  "<p>"
  "<center>"
  "<h1>ESP8266 Web Server</h1>"
  "<form action='msg'><p>Type your message <input type='text' name='msg' size=100 autofocus> <input type='submit' value='Submit'></form>"
  "</center>";

ESP8266WebServer server(80);                             // HTTP server will listen at port 80
long period;
int offset=1,refresh=0;
int pinCS = 0; // Attach CS to this pin, DIN to MOSI and CLK to SCK (cf http://arduino.cc/en/Reference/SPI )
int numberOfHorizontalDisplays = 12;
int numberOfVerticalDisplays = 1;
String decodedMsg;
int stopanim = 0;
int intensity = 2;
Max72xxPanel matrix = Max72xxPanel(pinCS, numberOfHorizontalDisplays, numberOfVerticalDisplays);

String tape = "Arduino";
int wait = 250; // In milliseconds

int spacer = 1;
int width = 5 + spacer; // The font width is 5 pixels

/*
  handles the messages coming from the webbrowser, restores a few special characters and 
  constructs the strings that can be sent to the oled display
*/
void handle_msg() {
                        
  matrix.fillScreen(LOW);
  server.send(200, "text/html", form);    // Send same page so they can send another msg
  refresh=1;
  // Display msg on Oled
  String msg = server.arg("msg");
  String level = server.arg("level");
  String str_anim = server.arg("stopanim");
  String spd = server.arg("speed");
  intensity = 2;
  if (level != "") {
    intensity = level.toInt();
  }
  stopanim = 0;
  if (str_anim == "1" or str_anim == "on") stopanim = 1;

  wait = 250;
  if (spd != "") {
    wait = spd.toInt();
  }
  
  Serial.println(msg);
  decodedMsg = msg;
  // Restore special characters that are misformed to %char by the client browser
  decodedMsg.replace("+", " ");      
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
  //Serial.println(decodedMsg);                   // print original string to monitor
 
 
    
  //Serial.println(' ');                          // new line in monitor
}

void setup(void) {
matrix.setIntensity(intensity); // Use a value between 0 and 15 for brightness

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

//ESP.wdtDisable();                               // used to debug, disable wachdog timer, 
  Serial.begin(115200);                           // full speed to monitor
                               
  WiFi.begin(SSID, PASS);                         // Connect to WiFi network
  while (WiFi.status() != WL_CONNECTED) {         // Wait for connection
    delay(500);
    Serial.print(".");
  }
  // Set up the endpoints for HTTP server,  Endpoints can be written as inline functions:
  server.on("/", []() {
    server.send(200, "text/html", form);
  });
  server.on("/msg", handle_msg);                  // And as regular external functions:
  server.begin();                                 // Start the server 


  Serial.print("SSID : ");                        // prints SSID in monitor
  Serial.println(SSID);                           // to monitor             
 
  char result[16];
  sprintf(result, "%3d.%3d.%1d.%3d", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3]);
  Serial.println();
  Serial.println(result);
  decodedMsg = result;
  Serial.println("WebServer ready!   ");

  Serial.println(WiFi.localIP());                 // Serial monitor prints localIP
  Serial.print(analogRead(A0));
  
}


void loop(void) {

  for ( int i = 0 ; i < width * decodedMsg.length() + matrix.width() - 1 - spacer; i++ ) {
    server.handleClient();                        // checks for incoming messages
    if (refresh==1) {
      i=0;
      matrix.setIntensity(intensity);
    }
    refresh=0;
    matrix.fillScreen(LOW);

    if (stopanim == 1) {
       for(int j=0; j < decodedMsg.length(); j++ )
        {
            int nb_space = ( (matrix.width()- (width*decodedMsg.length()) )/2 );
            if (nb_space < 0) nb_space =0;
            int x= (j * width) + nb_space;  // step horizontal by width of character and spacer
            int y = (matrix.height() - 8) / 2; // center the text vertically
        
            matrix.drawChar(x, y, decodedMsg[ j ], HIGH, LOW, 1);   // draw a character from 'tape'
        }

    } else {

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
    }
    matrix.write(); // Send bitmap to display

    delay(wait);
  }
}
