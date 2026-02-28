#define BLINKY_DIAG         0
#define CUBE_DIAG           0
#define COMM_LED_PIN       16
#define RST_BUTTON_PIN     15
#define GEIGER_PIN         20
#include <BlinkyPicoW.h>

struct CubeSetting
{
  uint16_t nsamples;
  uint16_t publishInterval;
};
CubeSetting setting;

struct CubeReading
{
  float lastInterval;
  float avgInterval;
  float cpm;
};
CubeReading reading;

volatile boolean newCount = false;
volatile unsigned long countTime;
unsigned long lastCountTime;
float avgInterval = -1.0;
uint16_t oldNsamples = 10;

unsigned long lastPublishTime;

void setupBlinky()
{
  if ((BLINKY_DIAG > 0) || (CUBE_DIAG > 0))
  {
    Serial.begin(9600);
    delay(5000);
  }
  
  boolean  useFlashStorage = true;
/**/  
  BlinkyPicoW.setSsid("blinkybox-demo");
  BlinkyPicoW.setWifiPassword("blinky-lite");
  BlinkyPicoW.setMqttServer("192.168.4.1");
  BlinkyPicoW.setMqttUsername("blinkybox-demo");
  BlinkyPicoW.setMqttPassword("areallybadpassword");
  BlinkyPicoW.setBox("blinkybox-demo");
  BlinkyPicoW.setTrayType("picoW");
  BlinkyPicoW.setTrayName("bl-cosmic-8d-cd");
  BlinkyPicoW.setCubeType("cube");
/**/ 

  BlinkyPicoW.setMqttKeepAlive(15);
  BlinkyPicoW.setMqttSocketTimeout(4);
  BlinkyPicoW.setMqttPort(1883);
  BlinkyPicoW.setMqttLedFlashMs(100);
  BlinkyPicoW.setHdwrWatchdogMs(8000);
  BlinkyPicoW.setRouterDelay(10000);

  BlinkyPicoW.begin(BLINKY_DIAG, COMM_LED_PIN, RST_BUTTON_PIN, useFlashStorage, sizeof(setting), sizeof(reading));
}

void setupCube()
{
  pinMode(GEIGER_PIN, INPUT);
  setting.publishInterval = 3000;
  setting.nsamples = 10;
  oldNsamples = setting.nsamples;
  reading.lastInterval = 0.1;
  reading.avgInterval = 0.1;
  reading.cpm = 0.1;
  newCount = false;
  avgInterval = -1.0;

  lastPublishTime = millis(); 
  lastCountTime = lastPublishTime;
  countTime = lastPublishTime;
  attachInterrupt(digitalPinToInterrupt(GEIGER_PIN), geiger, FALLING);
}
void loopCube()
{
  unsigned long now = millis();
  if (newCount)
  {
    newCount = false;
    if (lastCountTime >= 0)
    {
      reading.lastInterval = ((float) (countTime - lastCountTime)) / 1000.0;
      if (avgInterval < 0.0)
      {
        avgInterval = reading.lastInterval;
      }
      else
      {
        avgInterval = avgInterval + (reading.lastInterval - avgInterval) / ((float) setting.nsamples);
      }
      reading.avgInterval = avgInterval;
      reading.cpm = 60.0 / reading.avgInterval;
    }
    lastCountTime = countTime;
    if (setting.publishInterval == 0)
    {
      lastPublishTime = now;
      boolean successful = BlinkyPicoW.publishCubeData((uint8_t*) &setting, (uint8_t*) &reading, false);
    }
  }
  if (setting.publishInterval > 0)
  {
    if ((now - lastPublishTime) > setting.publishInterval)
    {
      lastPublishTime = now;
      boolean successful = BlinkyPicoW.publishCubeData((uint8_t*) &setting, (uint8_t*) &reading, false);
    }
  }
  if (BlinkyPicoW.retrieveCubeSetting((uint8_t*) &setting))
  {
    if (oldNsamples != setting.nsamples)
    {
      avgInterval = -1.0;
      oldNsamples = setting.nsamples;
    }
  }
}
void geiger() 
{
  newCount = true;
  countTime = millis();
}
