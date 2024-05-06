/*
  Optical SP02 Detection (SPK Algorithm) using the MAX30105 Breakout
  By: Nathan Seidle @ SparkFun Electronics
  Date: October 19th, 2016
  https://github.com/sparkfun/MAX30105_Breakout

  This demo shows heart rate and SPO2 levels.

  It is best to attach the sensor to your finger using a rubber band or other tightening 
  device. Humans are generally bad at applying constant pressure to a thing. When you 
  press your finger against the sensor it varies enough to cause the blood in your 
  finger to flow differently which causes the sensor readings to go wonky.

  This example is based on MAXREFDES117 and RD117_LILYPAD.ino from Maxim. Their example
  was modified to work with the SparkFun MAX30105 library and to compile under Arduino 1.6.11
  Please see license file for more info.

  Hardware Connections (Breakoutboard to Arduino):
  -5V = 5V (3.3V is allowed)
  -GND = GND
  -SDA = A4 (or SDA)
  -SCL = A5 (or SCL)
  -INT = Not connected
 
  The MAX30105 Breakout can handle 5V or 3.3V I2C logic. We recommend powering the board with 5V
  but it will also run at 3.3V.
*/

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "MAX30105.h"
#include "spo2_algorithm.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
#define OLED_RESET    -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
#define CURVE_HEIGHT 32 // height of drawing scale
#define CURVE_WEIGHT 128 // weight of drawing scale

#define BLESERVERNAME "ESP32-Wroom"
#define SERVICE_SENSOR_UUID "52bc1840-7526-401c-9e9c-5225aa48b668"
#define SERVICE_HR_UUID "c1f5180D-d7b0-4276-8dc5-2385eb4c6d22"
#define SERVICE_SPO2_UUID "0f821822-6ed9-4feb-9297-a5b083e62baa"
#define CHARACTERISTIC_GREEN_UUID "16e72700-ce09-4483-b9f7-70d1d0cbe6bb"
#define CHARACTERISTIC_IR_UUID "96ad2700-db18-41c6-af14-cfdfafd4017a"
#define CHARACTERISTIC_RED_UUID "84a22700-06e4-4e8b-aa15-11a24bc60201"
#define CHARACTERISTIC_UNIT_FREQUENCY_UUID "cbcc2722-174b-42f8-bca8-ae5b2fe85d86"
#define CHARACTERISTIC_UNIT_PERCENTAGE_UUID "02a627AD-6a22-432c-a6d5-b479d44ea3bc"

// some configuration parameters
static const byte ledBrightness = 0x1F; //Options: 0=Off to 255=51mA
static const byte sampleAverage = 1; //Options: 1, 2, 4, 8, 16, 32, it is better to use 4 as we take IR, Red and Green (3 samples here); (don't change this)
static const byte ledMode = 3; //Options: 1 = Red only, 2 = Red + IR, 3 = Red + IR + Green
static const int sampleRate = 400; //Options: 50, 100, 200, 400, 800, 1000, 1600, 3200, when sampleRate is 200, the actual frequency is 20
static const int pulseWidth = 69; //Options: 69, 118, 215, 411, you can change the pulsewidth here to improve the speed
static const int adcRange = 4096; //Options: 2048, 4096, 8192, 16384, DON'T CHANGE (relate to spo2)

// the length of bufferLength
static const int32_t bufferLength = 2048; // bufferLength must be a const, should be a postive integer, BUFFER_SIZE refer to "spo2_algorithm.h"
static const int32_t oneQuaterBuffer = bufferLength/8; // update every 512 data
// green, ir and red buffer
#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__)
//Arduino Uno doesn't have enough SRAM to store 100 samples of IR led data and red led data in 32-bit format
//To solve this problem, 16-bit MSB of the sampled data will be truncated. Samples become 16-bit data.
uint16_t irBuffer[bufferLength]; //infrared LED sensor data
uint16_t redBuffer[bufferLength];  //red LED sensor data
uint16_t greenBuffer[bufferLength]; //green LED sensor data
#else
uint32_t irBuffer[bufferLength]; //infrared LED sensor data
uint32_t redBuffer[bufferLength];  //red LED sensor data
uint32_t greenBuffer[bufferLength]; //green LED sensor data
#endif

// variables
// Instantanization peripherals
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET); // OLED
MAX30105 particleSensor; // MAX30101 (MAX30105)
BLECharacteristic green_characteristic(CHARACTERISTIC_GREEN_UUID, BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_INDICATE);
BLECharacteristic ir_characteristic(CHARACTERISTIC_IR_UUID, BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_INDICATE);
BLECharacteristic red_characteristic(CHARACTERISTIC_RED_UUID, BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_INDICATE);
BLECharacteristic frequency_characteristic(CHARACTERISTIC_UNIT_FREQUENCY_UUID, BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_INDICATE);
BLECharacteristic percentage_characteristic(CHARACTERISTIC_UNIT_PERCENTAGE_UUID, BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_INDICATE);
BLEDescriptor green_descriptor(BLEUUID((uint16_t)0x2902));
BLEDescriptor ir_descriptor(BLEUUID((uint16_t)0x2902));
BLEDescriptor red_descriptor(BLEUUID((uint16_t)0x2902));
BLEDescriptor frequency_descriptor(BLEUUID((uint16_t)0x2902));
BLEDescriptor percentage_descriptor(BLEUUID((uint16_t)0x2902));

bool deviceConnected = false; // BLE connection state check
bool oldDeviceConnected = false; // BLE connection state check
int32_t spo2; //SPO2 value, negative means invalidation
int32_t heartRate; //heart rate value, negative means invalidation
unsigned long startTime; // use to calculate the actual frequency
float frequency; // real-time frequency
uint16_t* sendingPointer; // to send data

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

void setup()
{
  Serial.begin(115200); // initialize serial communication at 115200 bits per second:

  // check OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.display();
  delay(500); // Pause for 2 seconds
  
  // Initialize sensor
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) { //Use default I2C port, 400kHz speed
    Serial.println(F("MAX30105 was not found. Please check wiring/power."));
    while (1);
  }
  // particleSensor.setup();
  particleSensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange); //Configure sensor with these settings

  //Serial.println(F("Initialing the dataBuffer ......"));
  startTime = millis();
  for (int32_t i = 0 ; i < bufferLength ; i++) {
    while (particleSensor.available() == false) //do we have new data?
      particleSensor.check(); //Check the sensor for new data

    redBuffer[i] = particleSensor.getFIFORed();
    irBuffer[i] = particleSensor.getFIFOIR();
    greenBuffer[i] = particleSensor.getFIFOGreen();
    particleSensor.nextSample(); //We're finished with this sample so move to next sample

    //Serial.println(greenBuffer[i], DEC);
  }
  frequency = (float) bufferLength / ((millis() - startTime) / 1000.0);
  Serial.printf("Average sampling rate for collecting %d data: %.2f Hz\n", bufferLength, frequency);

  // update the cruve 
  drawCruve(greenBuffer, bufferLength);

  // inialize the spo2 and heartRate
  heart_rate_and_oxygen_saturation(greenBuffer, irBuffer, redBuffer, bufferLength, (int32_t)frequency, &spo2, &heartRate);

  BLE_set_up();
}

void loop()
{
  // firstly send first oneQuaterBuffer data
  // notify changed value
  if (deviceConnected){
    // sendingPointer = (uint16_t*) greenBuffer;
    // green_characteristic.setValue((uint8_t*)(sendingPointer), oneQuaterBuffer*2); // 1 uint16_t will take 2 uint8_t, the mutiple should not exceed 512
    // green_characteristic.notify();
    // sendingPointer = (uint16_t*) irBuffer;
    // ir_characteristic.setValue((uint8_t*)(sendingPointer), oneQuaterBuffer*2); // size_of(uint16_t) = 2, buffer length = 4
    // ir_characteristic.notify();
    // sendingPointer = (uint16_t*) redBuffer;
    // red_characteristic.setValue((uint8_t*)(sendingPointer), oneQuaterBuffer*2); // size_of(uint16_t) = 2, buffer length = 4
    // red_characteristic.notify();
    sendingPointer = (uint16_t*) &heartRate;
    frequency_characteristic.setValue((uint8_t*)(sendingPointer), 2); // size_of(uint8_t) = 1
    frequency_characteristic.notify();
    // sendingPointer = (uint16_t*) &spo2;
    // frequency_characteristic.setValue((uint8_t*)(sendingPointer), 2); // size_of(uint8_t) = 1
    // frequency_characteristic.notify();
  }
  // disconnecting
  if (!deviceConnected && oldDeviceConnected) {
      delay(500); // give the bluetooth stack the chance to get things ready
      // pServer->startAdvertising(); // restart advertising
      BLEDevice::startAdvertising(); // restart advertising
      Serial.println("start advertising");
      oldDeviceConnected = deviceConnected;
  }
  // connecting
  if (deviceConnected && !oldDeviceConnected) {
      // do stuff here on connecting
      oldDeviceConnected = deviceConnected;
  }

  //Continuously taking samples from MAX30102.  Heart rate and SpO2 are calculated every 1 second
  for (int32_t i = oneQuaterBuffer; i < bufferLength; i++)
  {
    redBuffer[i - oneQuaterBuffer] = redBuffer[i];
    irBuffer[i - oneQuaterBuffer] = irBuffer[i];
    greenBuffer[i - oneQuaterBuffer] = greenBuffer[i];
  }

  startTime = millis();
  for (int32_t i = bufferLength - oneQuaterBuffer; i < bufferLength; i++)
  {
    while (particleSensor.available() == false) //do we have new data?
      particleSensor.check(); //Check the sensor for new data

    redBuffer[i] = particleSensor.getFIFORed();
    irBuffer[i] = particleSensor.getFIFOIR();
    greenBuffer[i] = particleSensor.getFIFOGreen();
    particleSensor.nextSample(); //We're finished with this sample so move to next sample

    //Serial.println(greenBuffer[i], DEC);
  }
  frequency = (float) oneQuaterBuffer / ((millis() - startTime) / 1000.0);
  Serial.printf("Average sampling rate for collecting %d data: %.2f Hz\n", oneQuaterBuffer, frequency);

  // update the cruve 
  drawCruve(greenBuffer, bufferLength);

  //After gathering the newest samples recalculate HR and SP02
  heart_rate_and_oxygen_saturation(greenBuffer, irBuffer, redBuffer, bufferLength, (int32_t)frequency, &spo2, &heartRate);

  Serial.print(F("HR="));
  Serial.print(heartRate, DEC);
  Serial.print(F(", SPO2="));
  Serial.println(spo2, DEC);
}

void drawCruve(uint32_t *dataBuffer, int32_t bufferLength){
  int16_t i;
  uint32_t maxValue = dataBuffer[0];
  uint32_t minValue = dataBuffer[0];
  for (i=0;i<bufferLength;i++){
    if (dataBuffer[i] > maxValue) maxValue = dataBuffer[i];
    if (dataBuffer[i] < minValue) minValue = dataBuffer[i];
  }
  int32_t length = min(bufferLength, CURVE_WEIGHT);
  int16_t distance = CURVE_WEIGHT/length;

  display.clearDisplay();
  for(i=1;i<length;i++){ // start from index 1
    int16_t x0 = (i-1)*distance;
    int16_t y0 = (int16_t) CURVE_HEIGHT*(dataBuffer[i-1]-minValue)/(maxValue-minValue);
    int16_t x1 = i*distance;
    int16_t y1 = (int16_t) CURVE_HEIGHT*(dataBuffer[i]-minValue)/(maxValue-minValue);
    display.drawLine(x0, y0, x1, y1, SSD1306_WHITE);
  }
  display.display();
  // refresh display every 0.5 second
  // delay(500);
}

void BLE_set_up(){
  // Serial.begin(115200);

  // 1. Create the BLE Device
  BLEDevice::init(BLESERVERNAME);

  // 2. Create the BLE Server
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // 3. Create three BLE Services
  // BLEService *pService_sensor = pServer->createService(SERVICE_SENSOR_UUID);
  BLEService *pService_hr = pServer->createService(SERVICE_HR_UUID);
  // BLEService *pService_spo2 = pServer->createService(SERVICE_SPO2_UUID);

  // 4. Create five BLE Characteristics
  // pService_sensor->addCharacteristic(&green_characteristic);
  // pService_sensor->addCharacteristic(&ir_characteristic);
  // pService_sensor->addCharacteristic(&red_characteristic);
  pService_hr->addCharacteristic(&frequency_characteristic);
  // pService_spo2->addCharacteristic(&percentage_characteristic);

  // green_descriptor.setValue("bit");
  // green_characteristic.addDescriptor(&green_descriptor);
  // ir_descriptor.setValue("bit");
  // ir_characteristic.addDescriptor(&ir_descriptor);
  // red_descriptor.setValue("bit");
  // red_characteristic.addDescriptor(&red_descriptor);
  frequency_descriptor.setValue("Hz");
  frequency_characteristic.addDescriptor(&frequency_descriptor);
  // percentage_descriptor.setValue("%");
  // percentage_characteristic.addDescriptor(&percentage_descriptor);

  // 5. Start the service
  // pService_sensor->start();
  pService_hr->start();
  // pService_spo2->start();

  // 6. Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  // pAdvertising->addServiceUUID(SERVICE_SENSOR_UUID);
  pAdvertising->addServiceUUID(SERVICE_HR_UUID);
  // pAdvertising->addServiceUUID(SERVICE_SPO2_UUID);
  BLEDevice::startAdvertising(); // only pin one device and then stop advertising after connect successfully
  // Serial.println("Waiting a client connection to notify...");
}
