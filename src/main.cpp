#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

// 设置基本的PWM属性
const int freq = 25000;
const int resolution = 8;

class Fan
{
private:
  uint8_t pwmPin;
  uint8_t tachPin;
  uint8_t pwmChannel;
  uint8_t level;

  volatile unsigned long tach;
  volatile unsigned long tachAt;
  volatile unsigned long rpm;

  static void IRAM_ATTR isrHandler(void *arg)
  {
    Fan *fan = static_cast<Fan *>(arg);
    fan->handleInterrupt();
  }

  void handleInterrupt()
  {
    tach++;
    long currentTime = millis();
    long deltaTime = currentTime - tachAt;
    if (deltaTime >= 1000)
    {
      rpm = tach * 60 * 1000 / 2 / deltaTime;
      tach = 0;
      tachAt = millis();
    }
  }

public:
  Fan(int pwmPin, int tachPin, int pwmChannel)
      : pwmPin(pwmPin), tachPin(tachPin), pwmChannel(pwmChannel),
        level(0), tach(0), rpm(0), tachAt(0) {}

  void setup()
  {
    pinMode(pwmPin, OUTPUT);
    ledcSetup(pwmChannel, freq, resolution);
    ledcAttachPin(pwmPin, pwmChannel);
    ledcWrite(pwmChannel, level);

    pinMode(tachPin, INPUT_PULLUP);
    attachInterruptArg(digitalPinToInterrupt(tachPin), isrHandler, this, RISING);
  }

  void setSpeed(uint8_t speed)
  {
    level = speed;
    ledcWrite(pwmChannel, level);
  }

  long getSpeed()
  {
    return rpm;
  }

  String status()
  {
    char strBuf[64];
    int written = sprintf(strBuf, "{\"index\":%d,\"level\":%d,\"rpm\":%d}", pwmChannel, level, rpm);
    strBuf[written] = '\0';
    return strBuf;
  }
};

// 配置你想要的风扇数量和对应的PWM、Tach引脚和通道
Fan fans[] = {
    {12, 18, 1}, // PWM pin 12, tach pin 18, channel 1
    {3, 10, 2},  // PWM pin 13, tach pin 19, channel 2
                 // 这里可以进一步添加更多风扇
};

void initWiFi()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin("Network", "Password");
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print('.');
    delay(300);
  }
  Serial.println(WiFi.localIP());
}

AsyncWebServer server(80);

void notFound(AsyncWebServerRequest *request)
{
  request->send(404, "text/plain", "Not found");
}

void setup()
{
  Serial.begin(115200);
  initWiFi();
  for (auto &fan : fans)
  {
    fan.setup();
  }
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "text/plain", "Hello, world"); });

  // Send a POST request to <IP>/post with a form field message set to <message>
  server.on("/fan", HTTP_POST, [](AsyncWebServerRequest *request)
            {
          int fanIndex = request->getParam("fan")->value().toInt();
          int level = request->getParam("level")->value().toInt();
          fans[fanIndex].setSpeed(level);
          Serial.printf("set %d to %d\n",fanIndex,level);
          request->send(200, "text/plain"); });

  server.on("/fan", HTTP_GET, [](AsyncWebServerRequest *request)
            {
          int fanIndex = request->getParam("fan")->value().toInt();
          Serial.printf("Fan %d: %d\n", fanIndex, fans[fanIndex].getSpeed());
          request->send(200, "application/json", fans[fanIndex].status()); });

  server.onNotFound(notFound);

  server.begin();
}

void loop()
{
}
