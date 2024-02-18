#include <ETH.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <ESPping.h>
#include <WiFi.h>
#include <PubSubClient.h>  //默认，加载MQTT库文件
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "time.h"
#define ETH_ADDR 1
#define ETH_POWER_PIN -1
#define ETH_MDC_PIN 23
#define ETH_MDIO_PIN 18
#define ETH_TYPE ETH_PHY_LAN8720
#define ETH_CLK_MODE ETH_CLOCK_GPIO17_OUT

const char* mqtt_server = "bemfa.com";              //默认，MQTT服务器
const int mqtt_server_port = 9501;                  //默认，MQTT服务器
#define ID_MQTT "***************"  //修改，你的Client ID
const char* xigate006 = ""***************"";                //主题名字，可在巴法云控制台自行创建，名称随意
const char* donggate006 = ""***************"";            //主题名字，可在巴法云控制台自行创建，名称随意

// 初始化 MQTT PubSubClient 对象
WiFiClient espClient;
PubSubClient client(espClient);
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600 * 8;   // 北京时区为GMT +8
const int   daylightOffset_sec = 0;
void printLocalTime()
{
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}

// 设置WiFi的SSID和密码
const char* ssid = "WiFi";
const char* password = "wifimima";
void setup_wifi() {
  // 连接WiFi
  WiFi.begin(ssid, password);
  // ETH.begin(ETH_ADDR, ETH_POWER_PIN, ETH_MDC_PIN, ETH_MDIO_PIN, ETH_TYPE, ETH_CLK_MODE);  //启用ETH

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Waiting for WiFi...");
  }

  // while (!((uint32_t)ETH.localIP()))  //等待获取到IP
  // {
  //   delay(1000);
  //   Serial.println("Waiting for ETH...");
  // }
  Serial.println("Connected");
  // 初始化NTP
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  // 打印当前时间
  printLocalTime();
  Serial.print("WiFi—IP-Address:");
  Serial.println(WiFi.localIP());
  // Serial.print("ETH-IP-Address:");
  // Serial.println(ETH.localIP());

}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect(ID_MQTT)) {
      Serial.println("mqtt connected");
      client.subscribe(xigate006);    // 订阅第一个主题
      client.subscribe(donggate006);  // 订阅第二个主题
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
 
}
double random_double() {
    return (double) rand() / RAND_MAX;
}
void append_random_to_url(char* url, double random_num) {
    // 将小数转换为字符串
    char random_str[20]; // 假设小数点后最多有16位数字
    sprintf(random_str, "%.16lf", random_num);

    // 追加随机数到URL的末尾
    strcat(url, "?random=");
    strcat(url, random_str);
}
uint64_t get_current_timestamp() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    // uint64_t timestamp = tv.tv_sec * 1000 + tv.tv_usec / 1000; // 毫秒级时间戳
    uint64_t timestamp = tv.tv_sec ;
    return timestamp;
}
// 定义一个处理消息的函数，减少代码重复
void HandleMessage(String message) {
  IPAddress staticIP(192,168,1,244); //设定你要设定的静态IP
  IPAddress gateway(192,168,0,1);
  IPAddress subnet(255,255,248,0);
  IPAddress primaryDNS(8,8,8,8);    //optional
  IPAddress secondaryDNS(114,114,114,114);  //optio
  Serial.println(message);
  // ETH.begin(ETH_ADDR, ETH_POWER_PIN, ETH_MDC_PIN, ETH_MDIO_PIN, ETH_TYPE, ETH_CLK_MODE);  //启用ETH
  ETH.begin(ETH_ADDR, ETH_POWER_PIN, ETH_MDC_PIN, ETH_MDIO_PIN, ETH_TYPE, ETH_CLK_MODE);
  delay(1000); //等待连接以太网
  // Serial.print("ETH-IP-Address:");
  // Serial.println(ETH.localIP());
  if(!ETH.config(staticIP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("ETH Config failed");
  }
  else {
    Serial.println("ETH Config Success. IP Address: ");
    Serial.println(ETH.localIP());
  }
  IPAddress ip(192,168,1,79);
  if(Ping.ping(ip)) {
    Serial.println("Ping success!!");
  }
  else {
    Serial.println("Ping failed!!");
  }
  HTTPClient http;                                     //定义http对象
  char url[100] = "http://192.168.1.76:8000/cgi-bin/set.cgi";
  // char url[100] = "http://192.168.1.79:8000/cgi-bin/get.cgi?random=0.08465518581956433&key=user";

  double random_num = random_double();
  append_random_to_url(url, random_num);
  // 追加参数到URL
  // strcat(url, "?key=opengate");
  http.begin(url);              //设置请求url
  http.setTimeout(2000);
  Serial.println(url);
  
  http.addHeader("Content-Type", "application/x-www-form-urlencoded; charset=UTF-8");  //设置请求头
  http.addHeader("Referer", "http://192.168.1.79:8000/preview_ws.html");
  http.addHeader("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36"); 
  http.addHeader("X-Requested-With", "XMLHttpRequest"); 
  uint64_t timestamp = get_current_timestamp();
  String epochTime = String(timestamp)+"000";                // 获取当前时间的13位时间戳
  Serial.println(epochTime);
  http.addHeader("time", epochTime);                          // 设置请求头参数time
  // 将string类型的字符串转换为字符数组
  char sourceArray[100];
  strcpy(sourceArray, epochTime.c_str());
  char cookie[200] = "version=EMF85-V3021C; uid=09H2103100119198; session=admin|0; target_iframe=main; target_index=0; target_url=preview_ws.html; times=";
  http.addHeader("Cookie", strcat(cookie, sourceArray));
  int httpCode = http.POST("key=opengate");                      // 发送POST请求
  if (httpCode > 0) {                                         // 如果状态码>0，说明请求成功
    Serial.printf("[HTTP] POST code: %d\n", httpCode);
    if (httpCode == HTTP_CODE_OK) {                           // 如果状态码为200，说明服务器处理成功
      String responsePayload = http.getString();              // 获取服务器返回的响应内容
      Serial.println(responsePayload);
    }
  } else {                                                    // 状态码<=0，请求失败
    Serial.printf("[HTTP] POST failed, error: %s\n", http.errorToString(httpCode).c_str());
  }
  http.end();                                                 // 关闭http
}
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);

  // 根据不同主题处理消息
  if (String(topic) == xigate006) {
    String message;

    Serial.println("西门");
    for (int i = 0; i < length; i++) {
      message += (char)payload[i];
    }
    if (message == "on") {
      HandleMessage(message);    
      // 处理来自主题1的消息
    }     
  } else if (String(topic) == donggate006) {
    String message;

    Serial.println("东门");
    for (int i = 0; i < length; i++) {
      message += (char)payload[i];
    }
    if (message == "on") {
      HandleMessage(message);    
      // 处理来自主题2的消息
    } 
  }
}
void setup() {
  Serial.begin(115200);
  setup_wifi();

  client.setServer(mqtt_server, mqtt_server_port);
  client.setCallback(callback);
}
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}
