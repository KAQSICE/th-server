//一下为该项目需要引入的函数库
#include <ESP8266WiFi.h>
#include <Wire.h>
#include <ESP8266WebServer.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <FS.h>
#include "temphumi.h"
#include "font.h"
#include "LED.h"

//以下为一些常量
#define esp8266_ssid "esp8266_zwld"       //自身作为热点时的ssid
#define esp8266_pwd "tju1895"             //自身密码
#define ssid1 "DESKTOP-OISTSQI 4858"      
#define pwd1 "&D358f99"
#define ssid2 "dyhsk"      
#define pwd2 "dyh123456"
#define SCREEN_WIDTH 128 // 设置OLED宽度,单位:像素
#define SCREEN_HEIGHT 64 // 设置OLED高度,单位:像素
#define OLED_RESET 3  //RES引脚

HTTPClient http;
unsigned long previousMillis = 0;
unsigned long interval = 2000;
unsigned long currentMillis;
const String TimeUrl = "http://quan.suning.com/getSysTime.do";
const char* host = "api.seniverse.com";     // 将要连接的心知天气地址 
const int httpPort = 80;                    // 将要连接的服务器端口     
String reqUserKey = "SehIsBRZtDBmW7Wc2";   // 私钥
String reqLocation = "Tianjin";            // 城市
String reqUnit = "c";                      // 摄氏/华氏
String reqRes = "/v3/weather/now.json?key=" + reqUserKey +
                  + "&location=" + reqLocation + 
                  "&language=en&unit=" +reqUnit;   //天气预报请求url

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//定义服务器 端口号80
ESP8266WebServer server(80);
temphumi TH; //温湿度对象
LED led;  //LED对象

int second; //系统重置后的秒数

//连接WIFI
void connectWiFi(){
  WiFi.begin(ssid1,pwd1);
  Serial.print("Connecting to ");
  int i=0;
  while(WiFi.status()!= WL_CONNECTED){
    if(i==30){
      Serial.println("");
      Serial.print("OVER TIME!!!");
      return;
    }
    Serial.print(".");
    i++;
    delay(2000);
  }
  Serial.println("Connection Established!!!");
  Serial.println("Connected to "+WiFi.SSID());
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

//启动服务器
void setupServer(){
  server.on("/getTH",getTH);//获取当前温湿度
  server.on("/setLed",setLed); //开关LED
  server.on("/adjustLed",adjustLed);//调节LED亮度
  server.onNotFound(handleRequest); //响应用户信息
  server.begin(); 
  Serial.println("HTTP server started");
  if(SPIFFS.begin()){
    Serial.println("SPIFFS ON");
  }
}
void handleRequest(){

  //获取资源url
  String url = server.uri();
  Serial.print("Requesting: ");
  Serial.println(url);

  //处理资源文件
  bool fileExit = handleFile(url);
  if(!fileExit){
    server.send(404,"text/plain","404 NOT FOUND!!!");
  }
}

//处理资源文件
bool handleFile(String url){
  if(url.endsWith("/")){
    url="/home.html";
  }

  String contentType = getContentType(url);

  if(SPIFFS.exists(url)){
    File file = SPIFFS.open(url,"r");
    server.streamFile(file,contentType);
    file.close();
    return true;
  }
  return false;
}

String getTime(){
  WiFiClient client;
  http.begin(client,TimeUrl);
  int httpCode = http.GET();
  if(httpCode>0){
    Serial.printf("HTTPCODE: %d\n",httpCode);
    if(httpCode == HTTP_CODE_OK){
      String res = http.getString();
      String timeString = res.substring(50,60);
      return timeString;
    }
  }
  else return "getTime ERROR!!!";
}

void getTH(){
  Serial.println("getTH...");
  String temhum = TH.read_AM2321();
  int i = temhum.indexOf('-');
  float tem = temhum.substring(0,i).toFloat();
  float hum = temhum.substring(i+1,temhum.length()).toFloat();
  String t = getTime();
  DynamicJsonDocument doc(96);
  doc["temperature"] = tem;
  doc["humidity"] = hum;
  doc["time"] = t;
  String THjson;
  serializeJson(doc,THjson);
  server.send(200,"application/json",THjson);
}

void setLed(){
  led.handleLED();
}

void adjustLed(){
  int brightness = server.arg("brightness");
  led.adjustLed(brightness);
}

void WeatherRequest(){
  WiFiClient wc;
  String httpRes = String("GET ")+reqRes+" HTTP/1.1\r\n"+"HOST: "+host+"\r\n"+"Connection:close\r\n\r\n";
  Serial.println("");
  delay(2000);
  if(wc.connect(host,80)){
    Serial.println("Connected to Weather!!!");
    wc.print(httpRes);
    String status_res = wc.readStringUntil('\n');
    Serial.println("status_res: "+status_res);

    if(wc.find("\r\n\r\n")){
      Serial.println("Skip Header");
    }
    parseWeatherJson(wc);
  }else{
    Serial.println("Connection failed!!!");
  }
  wc.stop();
}

void parseWeatherJson(WiFiClient wc){
  const size_t capacity = JSON_ARRAY_SIZE(1) + JSON_OBJECT_SIZE(1) + 2*JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(6) + 230;
  DynamicJsonDocument doc(capacity);
  
  deserializeJson(doc, wc);
  
  JsonObject results_0 = doc["results"][0];
  
  JsonObject results_0_now = results_0["now"];
  const char* results_0_now_text = results_0_now["text"]; 
  const char* results_0_now_code = results_0_now["code"];
  const char* results_0_now_temperature = results_0_now["temperature"]; 
  
  const char* results_0_last_update = results_0["last_update"]; 
  String results_0_now_text_str = results_0_now["text"].as<String>(); 
  int results_0_now_code_int = results_0_now["code"].as<int>(); 
  int results_0_now_temperature_int = results_0_now["temperature"].as<int>(); 
  String results_0_last_update_str = results_0["last_update"].as<String>(); 
  drawWeather(results_0_now_temperature_int,results_0_now_text_str);
}

void drawWeather(int temperature,String weather){
  Serial.println(temperature);
  Serial.println(weather);
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1.5);
  if(weather.equals("Sunny")) display.drawBitmap(30,10,epd_bitmap_sunny,20,20,1);
  else if(weather.equals("Clear")) {display.drawBitmap(30,10,epd_bitmap_clear,20,20,1);}
  else if(weather.equals("Cloudy")) display.drawBitmap(30,10,epd_bitmap_cloudy,20,19,1);
  else if(weather.equals("Overcast")) display.drawBitmap(30,10,epd_bitmap_overcast,20,20,1);
  display.setCursor(35,35);
  display.print(weather);
  display.setTextSize(2.5);
  display.setCursor(60,15);
  display.print(temperature);
  display.drawBitmap(82,12,hans_sheshidu,20,20,1);
  display.display();
}

void setup(void){
  Serial.begin(115200);
  Serial.println("");
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // 初始化OLED并设置其IIC地址为 0x3C
  connectWiFi();
  setupServer();
  led.LED_setup();
}

void loop(){
  server.handleClient();
  if(millis()-previousMillis>=interval){
    previousMillis = millis();
    WeatherRequest();
  }
  //second = millis()/1000;
}

// 获取文件类型
String getContentType(String filename){
  if(filename.endsWith(".htm")) return "text/html";
  else if(filename.endsWith(".html")) return "text/html";
  else if(filename.endsWith(".css")) return "text/css";
  else if(filename.endsWith(".js")) return "application/javascript";
  else if(filename.endsWith(".png")) return "image/png";
  else if(filename.endsWith(".gif")) return "image/gif";
  else if(filename.endsWith(".jpg")) return "image/jpeg";
  else if(filename.endsWith(".ico")) return "image/x-icon";
  else if(filename.endsWith(".xml")) return "text/xml";
  else if(filename.endsWith(".pdf")) return "application/x-pdf";
  else if(filename.endsWith(".zip")) return "application/x-zip";
  else if(filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}
