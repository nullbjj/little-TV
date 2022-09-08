
#include <TFT_eSPI.h>
#include <SPI.h>
#include <Wire.h>
#include <WebServer.h>
#include "bmp.h"
#include "text.h"
#include <WiFi.h>    //wifi库
#include <ArduinoJson.h>  //Json库
#include <HTTPClient.h>  //HTTP库
#include <NTPClient.h>    //NTP库
#include <WiFiUdp.h>
#include "Arduino.h"
#include "Ticker.h"      //定时器库
const char* ssid    = "AEA";  //wifi账号
const char* password = "AEA1989AEA";  //wifi密码
const char* ssd    = "AE";  //wifi账号

//const char* ssid    = "KC114";  //wifi账号
//const char* password = "kc114kc114";  //wifi密码
const char* host = "api.seniverse.com";  //心知天气服务器地址
String now_address="",now_time="",now_temperature="",now_weather="";//用来存储报文得到的字符串
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");
String weekDays[7]={"周日", "周一", "周二","周三", "周四", "周五", "周六"};
//Month names
String months[12]={"January", "February", "March", "April","May", "June", "July", "August", "September", "October", "November", "December"};
Ticker t1;
TFT_eSPI tft = TFT_eSPI(130,128);//设定屏幕大小
char determineqing[]="晴";
char determineduoyun[]="多云";
char determineyin[]="阴";
char determineyu[]="雨";
char determinexue[]="雪";
int i=1;
int ph;
char* now_wea;
int tm_Hour,tm_Minute,monthDay,tm_Month;
String weekDay;
char* week;
void showtext(int16_t x,int16_t y,uint8_t font,uint8_t s,uint16_t fg,uint16_t bg,const String str);
void showHanzi(int32_t x, int32_t y, const char c[3], uint32_t color);
void showHanziS(int32_t x, int32_t y, const char str[], uint32_t color);
void show_page(int16_t top,uint16_t fg,uint16_t bg,const uint16_t* page_image[],const uint16_t* wea_image, int32_t m,int32_t h,int32_t mon,int32_t days,const String temperature,const char* now_wea, const char* week);

void get_wifi()
{
  // 连接网络
  WiFi.begin(ssid, password);
  //等待wifi连接
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected"); //连接成功
  Serial.print("IP address: ");    //打印IP地址
  Serial.println(WiFi.localIP());
}
void get_weather()
{
  //创建TCP连接
    WiFiClient client;
    const int httpPort = 80;
    if (!client.connect(host, httpPort))
    {
      Serial.println("connection failed");  //网络请求无响应打印连接失败
      return;
    }
    //URL请求地址
    String url ="/v3/weather/now.json?key=S_xhO9flk_rjzOsJY&location=yangzhou&language=zh-Hans&unit=c";
    //发送网络请求
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
              "Host: " + host + "\r\n" +
              "Connection: close\r\n\r\n");
    delay(2000);
    //定义answer变量用来存放请求网络服务器后返回的数据
    String answer;
    while(client.available())
    {
      String line = client.readStringUntil('\r');
      answer += line;
    }
    //断开服务器连接
  client.stop();
  Serial.println();
  Serial.println("closing connection");
  //获得json格式的数据
  String jsonAnswer;
  int jsonIndex;
  //找到有用的返回数据位置i 返回头不要
  for (int i = 0; i < answer.length(); i++) {
    if (answer[i] == '{') {
      jsonIndex = i;
      break;
    }
  }
  jsonAnswer = answer.substring(jsonIndex);
  Serial.println();
  Serial.println("JSON answer: ");
  Serial.println(jsonAnswer);
  const size_t capacity = JSON_ARRAY_SIZE(1) + JSON_OBJECT_SIZE(1) + 2*JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(6) + 210;
  DynamicJsonDocument doc(capacity);
  deserializeJson(doc, jsonAnswer);
  JsonObject results_0 = doc["results"][0]; 
  JsonObject results_0_location = results_0["location"];
  const char* results_0_location_id = results_0_location["id"]; // "WX4FBXXFKE4F"
  const char* results_0_location_name = results_0_location["name"]; // "北京"
  const char* results_0_location_country = results_0_location["country"]; // "CN"
  const char* results_0_location_path = results_0_location["path"]; // "北京,北京,中国"
  const char* results_0_location_timezone = results_0_location["timezone"]; // "Asia/Shanghai"
  const char* results_0_location_timezone_offset = results_0_location["timezone_offset"]; // "+08:00"
  JsonObject results_0_now = results_0["now"];
  const char* results_0_now_text = results_0_now["text"]; // "多云"
  const char* results_0_now_code = results_0_now["code"]; // "4"
  const char* results_0_now_temperature = results_0_now["temperature"]; // "5"
  const char* results_0_last_update = results_0["last_update"]; // "2020-11-19T19:00:00+08:00"
  Serial.print("city:name:");
  Serial.println(results_0_location_name);
  now_temperature=results_0_now_temperature;
  Serial.println(now_temperature);
  now_time=results_0_last_update;
  Serial.println(now_time);
  now_weather=results_0_now_text;
  if(strstr(now_weather.c_str(),determineqing)!=0)
  {  now_wea = "晴";
     ph = 0;
  }
  if(strstr(now_weather.c_str(),determineduoyun)!=0)
  {  now_wea = "多云";
     ph = 1;
  }
  if(strstr(now_weather.c_str(),determineyin)!=0)
  {  now_wea = "阴";
     ph = 2;
  }
  if(strstr(now_weather.c_str(),determineyu)!=0)
  {  now_wea = "雨";
     ph = 3;
  }
  if(strstr(now_weather.c_str(),determinexue)!=0)
  {  now_wea = "雪";
     ph = 4;
  }
}

//web

WebServer server(80);
//String indexHtml;

String indexHtml = String("") + 
    "<!DOCTYPE html>" +
    "<html>" +
    "	<head>" +
    "		<meta charset=\"utf-8\">" +
    "		<title>ESP32 WebController</title>" +
    "	</head>" +
    "	<body>" +
    "		<h1>Web Controller</h1>" +
    "		<button id='button' name = 'button' onclick=\"test()\" type=\"button\" style=\"height: 100px;width: 200px;background-color: #1aa6f67d;border: 0px;\">Test</button>" +
    "	</body>" +
    "	<script type=\"text/javascript\">var counter = 0;" +
    "		function test() {" +
        "if(counter%2 == 0){document.getElementById('button').innerHTML='OFF'}else if(counter%2 == 1){document.getElementById('button').innerHTML='ON'}counter++;"+
   "			var ajax = new XMLHttpRequest();" +
   "			ajax.open(\'get\', \'/test\');" +
   "			ajax.send();" +
   "			ajax.onreadystatechange = function() {" +
   "				if (ajax.readyState == 4 && ajax.status == 200) {" +
   "					console.log(ajax.responseText);" +
   "				}" +
   "			}" +
    "		}" +
    "	</script>" +
    "</html>";

void indexHandler()
{
  if(server.arg("lcd_switch").toInt()) { Serial.println(server.arg("lcd_switch").toInt());}
  server.send(200, "text/html", indexHtml);
}

void testHandler()
{
  server.send(200, "text/html", "test seccess");
  // Serial.print("test Controller | Time:");
  // Serial.println(millis());
  //改动
  // Serial.begin(115200);
  //   Serial.println("Start");
  //   tft.init();//初始化显示寄存器
  //   tft.fillScreen(TFT_WHITE);//屏幕颜色
  //   delay(1000);
    Serial.begin(115200);
    Serial.println("Start");
    tft.init();//初始化显示寄存器
    tft.fillScreen(TFT_WHITE);//屏幕颜色
    tft.setTextSize(0);
    tft.setTextColor(TFT_BLACK);//设置字体颜色紫红色
    tft.setCursor(0, 0, 1);//设置文字开始坐标(0,0)及字体
    tft.setTextDatum(MC_DATUM);// 设置文本的引用数据
    tft.setTextSize(0);//设置文字大小
}
void handleAjax() //回调函数
{
  String message = "随机数据：";
  message += String(random(10000)); //取得随机数
  server.send(200, "text/plain", message); //将消息发送回页面
}
void setup()
{
    Serial.begin(115200);
    Serial.println("Start");
    tft.init();//初始化显示寄存器
    tft.fillScreen(TFT_BLACK);//屏幕颜色
    tft.setTextSize(2);
    tft.setTextColor(TFT_MAGENTA);//设置字体颜色紫红色
    tft.setCursor(0, 0, 1);//设置文字开始坐标(0,0)及字体
    tft.setTextDatum(MC_DATUM);// 设置文本的引用数据
    tft.setTextSize(1);//设置文字大小
    tft.setRotation(3);//屏幕内容镜像显示或者旋转屏幕0-4  ST7735_Rotation中设置
    showHanziS(40, 50, "周日晴", TFT_YELLOW);
    delay(3000);
    tft.fillScreen(TFT_BLACK);//屏幕颜色
    //0x39正常颜色 90度 0x40反色0度 0x37反色 90度 0x36反色90度
    get_wifi();
    get_weather();
    timeClient.begin();
  //设置偏移时间（以秒为单位）以调整时区，例如：
    // GMT +1 = 3600
    // GMT +8 = 28800
    timeClient.setTimeOffset(28800);
    t1.attach(3600, get_weather);

   // web
  server.on("/", indexHandler);
  server.on("/test", testHandler);
  // server.on("/getRandomData", HTTP_GET, handleAjax); //注册网页中ajax发送的get方法的请求和回调函数
  server.begin();
}

void loop()
{ 
  timeClient.update();
  unsigned long epochTime = timeClient.getEpochTime();
  String formattedTime = timeClient.getFormattedTime();
  int tm_Hour = timeClient.getHours();
  int tm_Minute = timeClient.getMinutes();
  int tm_Second = timeClient.getSeconds();
  String weekDay = weekDays[timeClient.getDay()];
  char week[weekDay.length() + 1];
  weekDay.toCharArray(week,weekDay.length() + 1);
  struct tm *ptm = gmtime ((time_t *)&epochTime);
    int monthDay = ptm->tm_mday;
    int tm_Month = ptm->tm_mon+1;
    String currentMonthName = months[tm_Month-1];
    int tm_Year = ptm->tm_year+1900;
    String currentDate = String(tm_Year) + "-" + String(tm_Month) + "-" + String(monthDay);
  /*照片数量-1*文本颜色*文本背景颜色*图片*分*时*月*日*温度*星期*/
  show_page(9,TFT_WHITE,TFT_BLACK,bmp_table_black,weather_image_black[ph], tm_Minute, tm_Hour, tm_Month, monthDay,now_temperature,now_wea,week);
  delay(100);
  //web
  server.handleClient();
}
/*图片文本页面显示*/
void show_page(int16_t top,uint16_t fg,uint16_t bg,const uint16_t* page_image[],const uint16_t* wea_image, int32_t m,int32_t h,int32_t mon,int32_t days,const String temperature,const char* now_wea, const char* week)
{
    //tft.fillScreen_1(0, 30,  64, 64,bg);
    tft.setSwapBytes(true);//开启显示
    tft.pushImage(0, 30,  64, 64, page_image[i]);
    i+=1;
    if(i>top){i=0;}
    delay(100);
    tft.drawFastHLine(4, 25, 120, tft.alphaBlend(0, TFT_RED,  fg));//绘制线  半透明颜色0-255
    tft.drawFastHLine(4, 95, 120, tft.alphaBlend(0, TFT_RED,  fg));//绘制线  半透明颜色0-255
    showtext(20,5,1,2,fg,bg,(String)mon+"/"+(String)days);
    if(h-10 < 0)
    {showtext(70,35,1,3,fg,bg,"0"+(String)h+":\n");}
    else
    {showtext(70,35,1,3,fg,bg,(String)h+":\n");}
    if((m-10)<0)
    {showtext(85,65,1,3,fg,bg,"0"+(String)m);}
    else
    {showtext(85,65,1,3,fg,bg,(String)m);}
    tft.pushImage(5, 98,  42, 32, wea_image);
    showHanziS(90, 5, week, TFT_YELLOW);
    showHanzi(52, 105, now_wea, TFT_YELLOW);
    showtext(80,105,1,2,fg,bg,temperature);
    showtext(105,100,1,1,fg,bg,".\n");
    showtext(112,108,1,1,fg,bg,"C\n");
}
/*文本显示*/
void showtext(int16_t x,int16_t y,uint8_t font,uint8_t s,uint16_t fg,uint16_t bg,const String str)
{
  //设置文本显示坐标，和文本的字体，默认以左上角为参考点，
    tft.setCursor(x, y, font);
  // 设置文本颜色为白色，文本背景黑色
  tft.setTextColor(fg,bg);
//设置文本大小，文本大小的范围是1-7的整数
  tft.setTextSize(s);
  // 设置显示的文字，注意这里有个换行符 \n 产生的效果
  tft.println(str);
}
/*单一汉字显示*/
void showHanzi(int32_t x, int32_t y, const char c[3], uint32_t color) { 
  for (int k = 0; k < 20; k++)// 根据字库的字数调节循环的次数
    if (hanzi[k].Index[0] == c[0] && hanzi[k].Index[1] == c[1] && hanzi[k].Index[2] == c[2])
    { tft.drawBitmap(x, y, hanzi[k].hz_Id, hanzi[k].hz_width, 16, color);
    }
}
/*整句汉字显示*/
void showHanziS(int32_t x, int32_t y, const char str[], uint32_t color) { //显示整句汉字，字库比较简单，上下、左右输出是在函数内实现
  int x0 = x;
  for (int i = 0; i < strlen(str); i += 3) {
    showHanzi(x0, y, str+i, color);
    x0 += 17;
  }
}