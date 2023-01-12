#define CAYENNE_PRINT Serial  // Comment this out to disable prints and save space
#include <L298N.h>
#include <CayenneMQTTESP32.h> // Library for Cayenne
#include <analogWrite.h>
#include <Adafruit_INA219.h>




#include <OneWire.h>
#include <DallasTemperature.h>





// Data wire is plugged into digital pin 4 on the Arduino
#define ONE_WIRE_BUS 4

// Setup a oneWire instance to communicate with any OneWire device
OneWire oneWire(ONE_WIRE_BUS);

// Pass oneWire reference to DallasTemperature library
DallasTemperature sensors(&oneWire);


Adafruit_INA219 ina219;
// WiFi network info.
char ssid[] = "V2102";
char wifiPassword[] = "22222222";

// Cayenne authentication info. This should be obtained from the Cayenne Dashboard.
char username[] = "f4b80670-4d43-11ea-b73d-1be39589c6b2";
char password[] = "5cda8f381756dd285d3aa7ef8fb5a6618d8e580b";
char clientID[] = "2638fa60-35af-11ec-8da3-474359af83d7";

#define VIRTUAL_CHANNEL 4
#define VIRTUAL_CHANNEL2 8
#define EnA 5
#define In1 27
#define In2 26
#define LED_BUILTIN 2
#define SENSOR  23
#define rainAnalog 35 //define rain sensor
#define rainDigital 34




#define EnB 18



long currentMillis = 0;
long previousMillis = 0;
int interval = 1000;
boolean ledState = LOW;
float calibrationFactor = 4.5;
volatile byte pulseCount;
byte pulse1Sec = 0;
float flowRate;
unsigned int flowMilliLitres;
unsigned long totalMilliLitres;

void IRAM_ATTR pulseCounter()
{
  pulseCount++;
}

CAYENNE_IN(VIRTUAL_CHANNEL)
{
  int value = getValue.asInt(); // 0 to 255
  CAYENNE_LOG("Channel %d, pin %d, value %d", VIRTUAL_CHANNEL, EnA, value);
  // Write the value received to the PWM pin. ledcAttachPin accepts a value from 0 to 255.

  // turn on motor A
  digitalWrite(In1, HIGH);
  digitalWrite(In2, LOW);

  analogWrite(EnA, value);
  Serial.print("speedmotor:");
  Serial.print(value);
  Serial.println();
  delay(3000);

}

CAYENNE_IN(VIRTUAL_CHANNEL2)
{
  int value = getValue.asInt(); // 0 to 255
  CAYENNE_LOG("Channel %d, pin %d, value %d", VIRTUAL_CHANNEL2, EnB, value);


  // turn on fan



  analogWrite(EnB, value);
  Serial.print("speed fan:");
  Serial.print(value);
  Serial.println();
  delay(3000);

}


void setup()
{
  Serial.begin(115200); // Baud rate for serial monitor
  delay(2000);
  Cayenne.begin(username, password, clientID, ssid, wifiPassword);   // To Authenticate cayenne

  pinMode(EnA, OUTPUT);
  pinMode(In1, OUTPUT);
  pinMode(In2, OUTPUT);

  pinMode(EnB, OUTPUT); //fan pwm

  Serial.begin(115200);

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(SENSOR, INPUT_PULLUP);
  pinMode(rainDigital, INPUT);

  pulseCount = 0;
  flowRate = 0.0;
  flowMilliLitres = 0;
  totalMilliLitres = 0;
  previousMillis = 0;

  attachInterrupt(digitalPinToInterrupt(SENSOR), pulseCounter, FALLING);


  while (!Serial) {
    // will pause Zero, Leonardo, etc until serial console opens
    delay(1);
  }
  uint32_t currentFrequency;

  Serial.println("Hello!");

  // Initialize the INA219.
  // By default the initialization will use the largest range (32V, 2A).  However
  // you can call a setCalibration function to change this range (see comments).
  if (! ina219.begin()) {
    Serial.println("Failed to find INA219 chip");
    while (1) {
      delay(10);
    }
  }
  // To use a slightly lower 32V, 1A range (higher precision on amps):
  ina219.setCalibration_32V_1A();
  // Or to use a lower 16V, 400mA range (higher precision on volts and amps):
  //ina219.setCalibration_16V_400mA();

  Serial.println("Measuring voltage and current with INA219 ...");
}




void loop()
{

 

  CAYENNE_IN(VIRTUAL_CHANNEL);
  CAYENNE_IN(VIRTUAL_CHANNEL2);
  Cayenne.loop();

  delay(2000);

  currentMillis = millis();
  if (currentMillis - previousMillis > interval) {

    pulse1Sec = pulseCount;
    pulseCount = 0;

    // Because this loop may not complete in exactly 1 second intervals we calculate
    // the number of milliseconds that have passed since the last execution and use
    // that to scale the output. We also apply the calibrationFactor to scale the output
    // based on the number of pulses per second per units of measure (litres/minute in
    // this case) coming from the sensor.
    flowRate = ((1000.0 / (millis() - previousMillis)) * pulse1Sec) / calibrationFactor;
    previousMillis = millis();

    // Divide the flow rate in litres/minute by 60 to determine how many litres have
    // passed through the sensor in this 1 second interval, then multiply by 1000 to
    // convert to millilitres.
    flowMilliLitres = (flowRate / 60) * 1000;

    // Add the millilitres passed in this second to the cumulative total
    totalMilliLitres += flowMilliLitres;

    // Print the flow rate for this second in litres / minute
    Serial.print("Flow rate: ");
    Serial.print(int(flowRate));  // Print the integer part of the variable
    Serial.print("L/min");
    Serial.print("\t");       // Print tab space

    // Print the cumulative total of litres flowed since starting
    Serial.print("Output Liquid Quantity: ");
    Serial.print(totalMilliLitres);
    Serial.print("mL / ");
    Serial.print(totalMilliLitres / 1000);
    Serial.println("L");

    Cayenne.virtualWrite(6, flowRate);
    Cayenne.virtualWrite(5, totalMilliLitres);
    Cayenne.virtualWrite(7, totalMilliLitres / 1000);



    delay(2000);      // Give some delay between each Sensor reading

    Cayenne.loop();

    // Short delay
    delay(500);
    float shuntvoltage = 0;
    float busvoltage = 0;
    float current_mA = 0;
    float loadvoltage = 0;
    float power_mW = 0;
    shuntvoltage = ina219.getShuntVoltage_mV();
    busvoltage = ina219.getBusVoltage_V();
    current_mA = ina219.getCurrent_mA() + 2 ;
    power_mW = ina219.getPower_mW();
    loadvoltage = busvoltage + (shuntvoltage / 1000) + 8;

  
    int rainAnalogVal = analogRead(rainAnalog); //read rain sensor in analog
    int rainDigitalVal = digitalRead(rainDigital); //read rain sensor in digital


    if (isnan(busvoltage) || isnan(shuntvoltage) || isnan(loadvoltage) || isnan(current_mA) || isnan(power_mW) ) {       // Condition to check whether the sensor reading was successful or not
      Serial.println("Failed to read from inaf sensor!");
      return;
    }


   

    Serial.print(rainDigitalVal);

    Serial.print("Bus Voltage:   "); Serial.print(busvoltage); Serial.println(" V");
    Serial.print("Shunt Voltage: "); Serial.print(shuntvoltage); Serial.println(" mV");
    Serial.print("Load Voltage:  "); Serial.print(loadvoltage); Serial.println(" V");
    Serial.print("Current:       "); Serial.print(current_mA); Serial.println(" mA");
    Serial.print("Power:         "); Serial.print(power_mW); Serial.println(" mW");

    // Send the command to get temperatures
    sensors.requestTemperatures();

    //print the temperature in Celsius
    Serial.print("Temperature: ");
    Serial.print(sensors.getTempCByIndex(0));
    Serial.print((char)176);//shows degrees character
    Serial.print(" C  |  ");


    //Cayenne.virtualWrite(0, busvoltage);
    //Cayenne.virtualWrite(1, shuntvoltage);
    Cayenne.virtualWrite(0, loadvoltage);
    Cayenne.virtualWrite(1, current_mA);
    Cayenne.virtualWrite(2, power_mW);
    Cayenne.virtualWrite(9, rainDigitalVal);
    Cayenne.virtualWrite(10, sensors.getTempCByIndex(0));

    // This function is called when data is sent from Cayenne.

  }

}

