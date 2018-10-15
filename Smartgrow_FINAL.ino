#include "Arduino.h"
#include "Wire.h"
#include "uRTCLib.h"      // biblioteca RTC DS3231
#include "DHTesp.h"       // biblioteca sensor DHT22
#include <ESP8266WiFi.h>  // biblioteca WiFi ESP8266
#include <LiquidCrystal_I2C.h>
#include <time.h>

const char* ssid = "ROCHETI";
const char* password = "200861sr";
const char* server = "api.thingspeak.com";
String writeAPIKey = "62UPHINJ4LYJM4NW";
int timezone = -3;
int dst = 0; // horário de verão

WiFiClient client;
DHTesp dht;
uRTCLib rtc(0x68, 0x57);
LiquidCrystal_I2C lcd(0x27, 20, 4);

String rtcToString(int sec, int min, int hour, int day, int month, int year) {
  String sec_str, min_str, hour_str, day_str, month_str;

  if (sec < 10) sec_str = String("0" + String(sec));
  else sec_str = String(sec);
  if (min < 10) min_str = String("0" + String(min));
  else min_str = String(min);
  if (hour < 10) hour_str = String("0" + String(hour));
  else hour_str = String(hour);
  if (day < 10) day_str = String("0" + String(day));
  else day_str = String(day);
  if (month < 10) month_str = String("0" + String(month));
  else month_str = String(month);

  return String(hour_str + ":" + min_str + " " + day_str + "/" + month_str + "/" + String(year));
}

void print_serial() {
  Serial.print(rtc.day(), 1);
  Serial.print('/');
  Serial.print(rtc.month(), 1);
  Serial.print('/');
  Serial.print(rtc.year(), 1);
  Serial.print(' ');
  Serial.print(rtc.hour(), 1);
  Serial.print(':');
  Serial.print(rtc.minute(), 1);
  Serial.print(':');
  if (rtc.second() >= 0 and rtc.second() < 10) {
    Serial.print("0");
    Serial.print(rtc.second(), 1);
  } else {
    Serial.print(rtc.second(), 1);
  }
  Serial.print("\t\t");
  Serial.print(dht.getStatusString());
  Serial.print("\t\t");
  Serial.print(dht.getHumidity(), 1);
  Serial.print("\t\t");
  Serial.print(dht.getTemperature(), 1);
  Serial.print("\t\t");
  Serial.println(rtc.temp(), 1);
}

void setup() {
  Serial.begin(9600);
  delay(500);
  Wire.begin(4, 5);
  delay(500);
  dht.setup(16); // data pin 2

  lcd.begin(4, 5);
  lcd.backlight();
  lcd.setCursor(0, 2);
  lcd.print("Temp:       C");
  lcd.setCursor(0, 3);
  lcd.print("Umid:       %");

  Serial.print("Iniciando conexão WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("");

  configTime(timezone * 3600, dst, "a.st1.ntp.br", "b.st1.ntp.br");
  Serial.println("Atualizando data e hora");
  while (!time(nullptr)) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("");
  time_t now;
  struct tm * timeinfo;
  time(&now);
  timeinfo = localtime(&now);
  rtc.set(timeinfo->tm_sec, timeinfo->tm_min, timeinfo->tm_hour, 6, timeinfo->tm_mday , (timeinfo->tm_mon + 1), (timeinfo->tm_year - 100));

  Serial.println("Data e hora \t\tStatus DHT22\tHumidity (%)\tTemp DHT22 (°C)\tTemp RTC (°C)\t");
}

uint8_t prev_min = 0;
String date_time;

void loop() {

  rtc.refresh();
  if (rtc.minute() != prev_min) {

    float humidity = dht.getHumidity();
    float temperature = dht.getTemperature();

    prev_min = rtc.minute();
    date_time = rtcToString(rtc.second(), rtc.minute(), rtc.hour(), rtc.day(), rtc.month(), rtc.year());

    lcd.home();
    lcd.print(String("   " + date_time));
    lcd.setCursor(6, 2);
    lcd.print(String(temperature));
    lcd.setCursor(6, 3);
    lcd.print(String(humidity));

    print_serial();

    // envio dos dados para o ThingSpeak
    if (client.connect(server, 80)) {
      String body = "field1=";
      body += String(humidity);
      body +=  "&field2=";
      body += String(temperature);

      client.println("POST /update HTTP/1.1");
      client.println("Host: api.thingspeak.com");
      client.println("User-Agent: ESP8266 (nothans)/1.0");
      client.println("Connection: close");
      client.println("X-THINGSPEAKAPIKEY: " + writeAPIKey);
      client.println("Content-Type: application/x-www-form-urlencoded");
      client.println("Content-Length: " + String(body.length()));
      client.println("");
      client.print(body);
    } else { }
    client.stop();
  }
  delay(500);

}


