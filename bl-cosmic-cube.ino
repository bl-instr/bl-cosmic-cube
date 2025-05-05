#define BLINKY_DIAG         0
#define COMM_LED_PIN       LED_BUILTIN
#define RST_BUTTON_PIN     15
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

int geigerPin = 20;
int LEDPin = 16;
volatile boolean ledState = false;
volatile boolean newCount = false;
volatile unsigned long countTime = -1;
unsigned long lastCountTime = -1;
float avgInterval = -1.0;
uint16_t oldNsamples = 10;

unsigned long lastPublishTime;

void setupBlinky()
{
  if (BLINKY_DIAG > 0) Serial.begin(9600);

  BlinkyPicoW.setMqttKeepAlive(15);
  BlinkyPicoW.setMqttSocketTimeout(4);
  BlinkyPicoW.setMqttPort(1883);
  BlinkyPicoW.setMqttLedFlashMs(100);
  BlinkyPicoW.setHdwrWatchdogMs(8000);

  BlinkyPicoW.begin(BLINKY_DIAG, COMM_LED_PIN, RST_BUTTON_PIN, true, sizeof(setting), sizeof(reading));
}

void setupCube()
{
  pinMode(geigerPin, INPUT);
  pinMode(LEDPin, OUTPUT);
  setting.publishInterval = 3000;
  setting.nsamples = 10;
  oldNsamples = setting.nsamples;
  reading.lastInterval = 0.1;
  reading.avgInterval = 0.1;
  reading.cpm = 0.1;

  lastPublishTime = millis(); 
  attachInterrupt(digitalPinToInterrupt(geigerPin), geiger, FALLING);
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
  }
  if (ledState)
  {
    if ((now - countTime) > 50)
    {
      ledState = false;
      digitalWrite(LEDPin, LOW); 
    }
  }
  if ((now - lastPublishTime) > setting.publishInterval)
  {
    lastPublishTime = now;
    boolean successful = BlinkyPicoW.publishCubeData((uint8_t*) &setting, (uint8_t*) &reading, false);
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
  ledState = true;
  countTime = millis();
  digitalWrite(LEDPin, HIGH);   
}
