// Enable debug prints to serial monitor
#define MY_DEBUG 

// Enable and select radio type attached
#define MY_RADIO_NRF24

#include <SPI.h>
#include <MySensors.h>
#include <DallasTemperature.h>
#include <OneWire.h>

#define INTERRUPT 1
#define CHILD_ID 15
#define BUTTON_PIN  3  // Arduino Digital I/O pin for button/reed switch
#define ONE_WIRE_BUS 2 // Pin where dallase sensor is connected
#define TEMPERATURE_PRECISION 11 // Lower resolution 
#define MAX_ATTACHED_DS18B20 16
unsigned long SLEEP_TIME = 900000; // Sleep time between reads (in milliseconds)

int oldValue=-1;
OneWire oneWire(ONE_WIRE_BUS); // Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
DallasTemperature sensors(&oneWire); // Pass the oneWire reference to Dallas Temperature. 
float lastTemperature[MAX_ATTACHED_DS18B20];
int numSensors=0;
DeviceAddress tempDeviceAddress; // We'll use this variable to store a found device address

// Initialize temperature message
// Change to V_LIGHT if you use S_LIGHT in presentation below
MyMessage msgTMP(0,V_TEMP);
MyMessage msg(CHILD_ID,V_TRIPPED);

void before()
{
  // Startup up the OneWire library
  sensors.begin();
}

void setup()  
{  
  // Setup the button
  pinMode(BUTTON_PIN,INPUT);
  // Activate internal pull-up
  digitalWrite(BUTTON_PIN,HIGH);

  // requestTemperatures() will not block current thread
  sensors.setWaitForConversion(false);
}

void presentation() {
  sendSketchInfo("TempDoorSensor", "1.0");
  // Register binary input sensor to gw (they will be created as child devices)
  // You can use S_DOOR, S_MOTION or S_LIGHT here depending on your usage. 
  // If S_LIGHT is used, remember to update variable type you send in. See "msg" above
  
  present(CHILD_ID, S_DOOR);
  
  // Fetch the number of attached temperature sensors  
  numSensors = sensors.getDeviceCount();
  // Present all sensors to controller
  for (int i=0; i<numSensors && i<MAX_ATTACHED_DS18B20; i++) {   
     present(i+10, S_TEMP);

     if(sensors.getAddress(tempDeviceAddress, i)) {
          Serial.print("Found device ");
          Serial.print(i, DEC);
          Serial.print(" with address: ");
          printAddress(tempDeviceAddress);
          Serial.println();
          
          Serial.print("Setting resolution to ");
          Serial.println(TEMPERATURE_PRECISION, DEC);
          
          // set the resolution to TEMPERATURE_PRECISION bit (Each Dallas/Maxim device is capable of several different resolutions)
          sensors.setResolution(tempDeviceAddress, TEMPERATURE_PRECISION);
          
           Serial.print("Resolution actually set to: ");
          Serial.print(sensors.getResolution(tempDeviceAddress), DEC); 
          Serial.println();
     }
  }
}

// function to print a device address
void printAddress(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}

//  Check if digital input has changed and send in new value
void loop() 
{
  uint8_t value;
  value = digitalRead(BUTTON_PIN);
  sleep(10);

  if (value != oldValue) {
     // Send in the new value
     send(msg.set(value==HIGH ? 1 : 0));
     oldValue = value;
  }

  // Fetch temperatures from Dallas sensors
  sensors.requestTemperatures();
  // query conversion time and sleep until conversion completed
  int16_t conversionTime = sensors.millisToWaitForConversion(sensors.getResolution());
  // sleep() call can be replaced by wait() call if node need to process incoming messages (or if node is repeater)
  wait(conversionTime);

      // Read temperatures and send them to controller 
      for (int i=0; i<numSensors && i<MAX_ATTACHED_DS18B20; i++) {
        // Fetch and round temperature to one decimal
        //float temperature = static_cast<float>(static_cast<int>(sensors.getTempCByIndex(i)) * 10.) / 10.;
        //float temperature = static_cast<float>(static_cast<int>(sensors.getTempCByIndex(i)));
        float temperature = static_cast<float>(sensors.getTempCByIndex(i));
    
        // Only send data if temperature has changed and no error
        if (lastTemperature[i] != temperature && temperature != -127.00 && temperature != 85.00) {
            // Send in the new temperature
            send(msgTMP.setSensor(i+10).set(temperature,1));
            // Save new temperatures for next compare
            lastTemperature[i]=temperature;
        }
    }
  sleep(INTERRUPT, CHANGE, SLEEP_TIME);
}
