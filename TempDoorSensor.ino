// Enable debug prints to serial monitor
#define MY_DEBUG 

// Enable and select radio type attached
#define MY_RADIO_NRF24

#include <SPI.h>
#include <MySensors.h>
#include <DallasTemperature.h>
#include <OneWire.h>

#define INTERRUPT 1
#define CHILD_ID 13
#define BUTTON_PIN  3  // Arduino Digital I/O pin for button/reed switch
#define ONE_WIRE_BUS 2 // Pin where dallase sensor is connected 
#define MAX_ATTACHED_DS18B20 16
unsigned long SLEEP_TIME = 900000; // Sleep time between reads (in milliseconds)

int oldValue=-1;
OneWire oneWire(ONE_WIRE_BUS); // Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
DallasTemperature sensors(&oneWire); // Pass the oneWire reference to Dallas Temperature. 
float lastTemperature[MAX_ATTACHED_DS18B20];
int numSensors=0;

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
     present(i, S_TEMP);
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
        float temperature = static_cast<float>(static_cast<int>(sensors.getTempCByIndex(i)) * 10.) / 10.;
    
        // Only send data if temperature has changed and no error
        if (lastTemperature[i] != temperature && temperature != -127.00 && temperature != 85.00) {
            // Send in the new temperature
            send(msgTMP.setSensor(i).set(temperature,1));
            // Save new temperatures for next compare
            lastTemperature[i]=temperature;
        }
    }
  sleep(INTERRUPT, CHANGE, SLEEP_TIME);
}
