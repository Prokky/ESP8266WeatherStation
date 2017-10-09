#include <TFT_ILI9341_ESP.h>
#include <SPI.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <HttpClient.h>
#include <DHT.h>
#include <SoftwareSerial.h>

#define DHTTYPE DHT22
#define DHTPIN D3

TFT_ILI9341_ESP tft = TFT_ILI9341_ESP();

DHT dht(DHTPIN, DHTTYPE);
WiFiClient c;
HttpClient http(c);
SoftwareSerial bt(12, 2);

const char* ssid     = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

const char* host = "api.openweathermap.org";
const char* openWeatherKey = "YOUR_OPENWEATHER_API_KEY";
const double lon = 30.26; // weather lon
const double lat = 59.89; // weather lat
const int kNetworkTimeout = 30 * 1000;
const int kNetworkDelay = 1000;

int dInTemp = 0;
int dInHum = 0;

String readString;

void setup() {
  bt.begin(9600);
  delay(10);

  tft.init();
  tft.setRotation(2);

  connectToWifi();
  bt.println("AT+DEFAULT");
  bt.println("AT+NAMEHM10TEMP");
  bt.println("AT+IBEA1");
  bt.println("AT+RESET");
}

void loop() {
  String response = requestForecast();

  DynamicJsonBuffer jsonBuffer(200);
  JsonObject& root = jsonBuffer.parseObject(response);

  int temp1 = root["list"][0]["main"]["temp"];
  int hum1 = root["list"][0]["main"]["humidity"];
  double wind1 = root["list"][0]["wind"]["speed"];
  String time1 = root["list"][0]["dt_txt"];
  String desc1 = root["list"][0]["weather"][0]["main"];

  int temp2 = root["list"][1]["main"]["temp"];
  int hum2 = root["list"][1]["main"]["humidity"];
  double wind2 = root["list"][1]["wind"]["speed"];
  String time2 = root["list"][1]["dt_txt"];
  String desc2 = root["list"][1]["weather"][0]["main"];

  int inTemp = dht.readTemperature(false);
  int inHum = dht.readHumidity();
  if (inTemp < 100 && inHum <= 100) {
    dInTemp = inTemp;
    dInHum = inHum;
  }

  setupScreen();
  printForecast(temp1, hum1, wind1, time1, desc1);
  printForecast(temp2, hum2, wind2, time2, desc2);
  printIndoor(dInTemp, dInHum);
  setBLECharacteristics(dInTemp, dInHum);
  delay(15000);
}

void setupScreen() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_RED);
  tft.setTextSize(1);
  tft.setCursor(0, 0, 2);
  tft.print("WiFi: ");
  tft.println(WiFi.SSID());
  tft.print("IP address: ");
  tft.println(WiFi.localIP());
  tft.println("");
}

void printForecast(int temp, int hum, double wind, String tTime, String desc) {
  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE);
  tft.print(tTime[11]); tft.print(tTime[12]); tft.print(":00 "); tft.println(desc);
  tft.setTextColor(TFT_ORANGE);
  tft.print(temp); tft.print("'C "); tft.print(hum);  tft.print("% "); tft.print(wind); tft.println("m/s");
  tft.println("");
}

void printIndoor(int temp, int hum) {
  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE);
  tft.println("Indoors: ");
  tft.setTextSize(3);
  tft.setTextColor(TFT_ORANGE);
  tft.print(temp); tft.print("'C "); tft.print(hum);  tft.print("%");
}

void setBLECharacteristics(int temp, int hum) {
  bt.print("AT+MINO0x"); bt.print(temp * 1000 + hum, HEX); bt.println("");
  bt.println("AT+RESET");
}

String requestForecast() {
  String url = "/data/2.5/forecast";
  url += "?lat=";
  url += lat;
  url += "&lon=";
  url += lon;
  url += "&appid=";
  url += openWeatherKey;
  url += "&units=metric&cnt=2";
  int err = 0;

  err = http.get(host, url.c_str());
  http.skipResponseHeaders();

  unsigned long timeoutStart = millis();
  String json;
  while ( (http.connected() || http.available()) && ((millis() - timeoutStart) < kNetworkTimeout) )
  {
    if (http.available())
    {
      char ch = http.read();
      json += ch;
      timeoutStart = millis();
    }
    else
    {
      delay(kNetworkDelay);
    }
  }
  http.stop();
  return json;
}

void connectToWifi() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(1);
  tft.setCursor(0, 0, 2);
  tft.println("Connecting to:");
  tft.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    tft.print(".");
  }


  tft.println("");
  tft.println("");
  tft.println("WiFi connected");
}

