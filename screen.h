#ifndef screen_h
#define screen_h
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

//OLED pins
#define OLED_SDA 4
#define OLED_SCL 15 
#define OLED_RST 16
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

String message_main; //message to be displayed
int index; //keep track of what message is currently being displayed

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);

//put this function in setup
void initScreen() {
  
  //reset OLED display via software
  pinMode(OLED_RST, OUTPUT);
  digitalWrite(OLED_RST, LOW);
  delay(20);
  digitalWrite(OLED_RST, HIGH);

  //initialize OLED
  Wire.begin();
  //Wire.begin(OLED_SDA, OLED_SCL); was the old code, didn't work :(
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3c, false, false)) { // Address 0x3C for 128x32
    //Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0,0);
  display.print("WELCOME :D");
  display.display();

}

//write message to screen
void writeMessage() {
   display.clearDisplay();
   display.print(message_main);
   display.display();
}

//for switching between messages, will need to increment/decrement the counter in radio code 
//and retrieve the message saved in that index, then set message_main and call writeMessage()
//...I think :O

#endif
