#include "display.h"
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

void display_init(){
  Wire.begin();
  display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println("NODE 1 STARTING... ");
  display.display();
}


void display_status(float current, uint32_t timestamp, uint32_t frameCount, bool alive) {
    display.clearDisplay();

    display.setCursor(0,0);
    display.print("STATUS: ");
    display.println(alive ? "RUNNING" : "IDLE");

    display.print("Time: ");
    display.println(timestamp);

    display.print("current: ");
    display.print(current, 3);
    display.println(" A");

    display.print("Frames: ");
    display.println(frameCount);

    display.display();
}

