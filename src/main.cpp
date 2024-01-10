#include <Adafruit_SSD1327.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <bitmaps.h>
#include <Stepper.h>

#include <ArduinoJson.h>

#include <neotimer.h>

// Used for software SPI
#define OLED_CLK 5
#define OLED_MOSI 16

// Used for software or hardware SPI
#define OLED_CS 4
#define OLED_DC 0

// Used for I2C or SPI
#define OLED_RESET 2

// software SPI
Adafruit_SSD1327 display(128, 128, OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);
// hardware SPI
//Adafruit_SSD1327 display(128, 128, &SPI, OLED_DC, OLED_RESET, OLED_CS);

// I2C
//Adafruit_SSD1327 display(128, 128, &Wire, OLED_RESET, 1000000);

ESP8266WiFiMulti WiFiMulti;

float frequency = 0;
float demand = 0;
DynamicJsonDocument data(4096);

Neotimer fetchTimer = Neotimer();
Neotimer screenTimer = Neotimer();

//Test type showing
uint8 typeShowing = 0;
uint8 apiTypes = 0;

const int stepsPerRevolution = 200;
Stepper gauge(stepsPerRevolution, 15, 13, 12, 14);



void displaySpashScreen() {
  display.clearDisplay();
  display.drawBitmap(0, 0, bmp_splash, 128, 128, SSD1327_WHITE);
  display.display();
}

void renderIcon(uint index) {
  display.drawBitmap(32, 20, logo_allArray[index], 64, 64, SSD1327_WHITE);
}

void renderWifiStatus() {

  display.drawBitmap(32, 20, logo_wifi, 64, 64, SSD1327_WHITE);
  display.setTextSize(2);
  display.setTextWrap(false);
  display.setTextColor(SSD1327_WHITE);

  display.setCursor(0, 0);
  display.print("Connecting...");
  display.setCursor(0, 90);
  display.setTextSize(3);
  //display.printf("%.2f", idx == 1 ? demand : frequency);

  display.display();
}

int getIconIdx() {
  switch(typeShowing) {
    case 1: return 3; //Demand
    case 0: return 4; // Freq
  };

  if (data["types"][typeShowing - 2]["type"].as<String>() == "CCGT") {
    return 1;
  }
  if (data["types"][typeShowing - 2]["type"].as<String>() == "Wind") {
    return 12;
  }
  if (data["types"][typeShowing - 2]["type"].as<String>() == "Nuclear") {
    return 7;
  }
  if ((data["types"][typeShowing - 2]["type"].as<String>().indexOf("ICT") > 0)) {
    return 6; //Interconnectors
  }
  if (data["types"][typeShowing - 2]["type"].as<String>() == "Pumped") {
    return 9;
  }
  if (data["types"][typeShowing - 2]["type"].as<String>() == "Hydro") {
    return 5;
  }
  if (data["types"][typeShowing - 2]["type"].as<String>() == "Biomass") {
    return 0;
  }
  if (data["types"][typeShowing - 2]["type"].as<String>() == "Solar") {
    return 11;
  }
  if (data["types"][typeShowing - 2]["type"].as<String>() == "Coal") {
    return 2;
  }
  if (data["types"][typeShowing - 2]["type"].as<String>() == "OCGT") {
    return 8;
  }
  return 3;
}

void renderScreen(uint8_t idx) {
  display.clearDisplay();
  if (idx == 0 || idx == 1) {
    //Freq and demand have different screens.
    display.setTextSize(2);
    display.setTextWrap(false);
    display.setTextColor(SSD1327_WHITE);

    display.setCursor(0, 0);
    display.print(idx == 1 ? "  Demand" : "  Frequ.");
    display.setCursor(0, 90);
    display.setTextSize(3);
    display.printf("%.2f", idx == 1 ? demand : frequency);
    display.print(idx == 1 ? "GW" : "Hz");
    renderIcon(idx == 1 ? 3 : 4);
  } else {
    display.setTextSize(2);
    display.setTextWrap(false);
    display.setTextColor(SSD1327_WHITE);
    display.setCursor(0, 0);
    display.print(data["types"][idx - 2]["type"].as<String>());
    renderIcon(getIconIdx());

    display.setCursor(0, 90);
    display.print(data["types"][idx - 2]["output"].as<String>());
    display.print("GW");

    display.setCursor(0, 108);
    display.print(data["types"][idx - 2]["percent"].as<String>());
    display.print("%");
  }

  display.display();
  return;
}

void delayYield(int millis) {
  for(int j = 0; j < millis; j++) {
    yield();
    delay(1);
  }
}

void setup()   {                
  Serial.begin(115200);
  //while (! Serial) delay(100);
  Serial.println("SSD1327 OLED test");
  
  if ( ! display.begin(0x3D) ) {
     Serial.println("Unable to initialize OLED");
     while (1) yield();
  }

  displaySpashScreen();
  delayYield(1000);

  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP("DISCOVERY", "QEETDRPA");
  WiFiMulti.addAP("MiSMK", "3M3n1nSh3ds12");

  for(int i = 0; i < 13; i++) {
    Serial.println("displaying icon...");
    display.clearDisplay();
    renderIcon(i);
    display.display();
    delayYield(250);
  }

  fetchTimer.set(1000);
  screenTimer.set(10000);

  gauge.step(stepsPerRevolution);
  gauge.step(stepsPerRevolution * 2);
}

void fetchData() {
    if ((WiFiMulti.run() == WL_CONNECTED)) {
    std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);
    client->setInsecure();
    HTTPClient https;
    Serial.print("[HTTPS] begin...\n");
    if (https.begin(*client, "https://4fgfqq5us9.execute-api.eu-west-2.amazonaws.com/")) {  // HTTPS

      Serial.print("[HTTPS] GET...\n");
      // start connection and send HTTP header
      int httpCode = https.GET();

      // httpCode will be negative on error
      if (httpCode > 0) {
        // HTTP header has been send and Server response header has been handled
        Serial.printf("[HTTPS] GET... code: %d\n", httpCode);

        // file found at server
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
          deserializeJson(data, https.getString());

          demand = data["demand"];
          frequency = data["frequency"];  

          apiTypes = data["types"].size();        

          // for(uint k = 0; k < data["types"].size(); k++) {
          //   Serial.printf("Get Type Percent %s\n", data["types"][k]["percent"].as<const char*>());
          // }

          renderScreen(typeShowing);

          //serializeJson(data, Serial);
        }
      } else {
        Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
      }

      https.end();
    } else {
      Serial.printf("[HTTPS] Unable to connect\n");
    }
  }
}

void loop() {
  if (WiFiMulti.run() != WL_CONNECTED) {
      renderWifiStatus();
  } else {
    renderScreen(typeShowing);
    if (fetchTimer.repeat()) {
      fetchData();
      fetchTimer.set(60000);
      if (typeShowing < 2) {
        typeShowing = 2;
      }
    }
    if (screenTimer.repeat()) {
      if ((typeShowing + 1) >= (apiTypes + 2)) {
        typeShowing = 0;
      } else {
        typeShowing++;
      }
      renderScreen(typeShowing);
    }
  }
}