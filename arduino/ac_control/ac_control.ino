/*
               AC datacenter rotator
*/

#define PROG_VERSION 0.1

/*
		LOAD LIBRARIES
*/

//the following line tells the compiler that
//timer0_millis is declared somewhere else
// extern unsigned long timer0_millis;

// - Ethernet library
#include <SPI.h>
#include <Ethernet.h>

// - Temp and humidity sensor
#include <dht11.h> 
dht11 DHT11;

#include <EEPROM.h>


/*
		CONFIGURATIONS
*/

// PIN DEFINITIONS
#define BUTTON1PIN 0
#define DHT11PIN 8
#define AC1PIN 4
#define AC2PIN 5

// SYSTEM PROTECTIONS
#define TEMPERATURE_T0 240000 // Will wait 4 minutes to start check the temperature. In case of reset, we should give few minutes to the compressors.
#define ROTATION_T0 300000 // Will wait 5 minutes to start the ACs. Same as above.
#define MINIMUM_OVERHEAT_TIME 1800000 // In case of a temperature overheating, temperature check will be disabled for 30min.

//EEPROM ADDRESSES
#define TEMP_LOWADDR 0  // degrees Celsius
#define TEMP_UPP_ADDR 1  // degrees Celsius
#define TEMP_INTERVAL_ADDR 2  // seconds
#define ROTATION_INTERVAL_H_ADDR 3 // hours
#define ROTATION_INTERVAL_M_ADDR 4 // minutes

/*
		MISC VARIABLES
*/

// Variables to the EthernetServer
EthernetServer server(80);
String cmd = String(10);
String arg = String(10);
byte cmd_status; // Status of the requested command. 0 = No requested command. 1 = Success. 2 = Fail. 3 = Unknown command.
///////////////////////



boolean ac_all = false; // If true, will START with both engines.
long temperatureTime; // next millis() which should check the temperature
long rotationTime; // next millis() which should rotate the ACs
byte ac_now = 1; // Will start with engine 0
byte chk; // To DHT11

byte temp_low;
byte temp_upp;
byte temperatureInterval;
byte rotationInterval_h;
byte rotationInterval_m;

/*
		FUNCTIONS
*/

// Reads configuration from the EEPROM
void read_from_eeprom()
{
  Serial.println("-- Reading default configuration values from EEPROM --");
  temp_low = EEPROM.read(TEMP_LOWADDR);
  temp_upp = EEPROM.read(TEMP_UPP_ADDR);
  temperatureInterval = EEPROM.read(TEMP_INTERVAL_ADDR);
  rotationInterval_h = EEPROM.read(ROTATION_INTERVAL_H_ADDR);
  rotationInterval_m = EEPROM.read(ROTATION_INTERVAL_M_ADDR);
}

// Writes configuration on the EEPROM
void write_to_eeprom(int addr, int value)
{
  byte v = EEPROM.read(addr); // Actual value on EEPROM. Avoids saving it again.
  
  Serial.println("-- EEPROM WRITE NEW VALUES --");
  Serial.print("   Address: ");
  Serial.println(addr);
  Serial.print("   Value: ");
  Serial.println(v);
  
  if (value != v){
    Serial.print("   Updating to: ");
    Serial.println(value);
    EEPROM.write(addr, value);
  } else {
    Serial.println("   Value correct! No need to update.");
  }    
}

// Maximum temperature check function
// Checks periodically the local temperature. If goes over a treshold, turn the two AC units.
void temperature_check()
{
  Serial.println("-- Temperature check function called --");
  
  chk = DHT11.read(DHT11PIN);

  if (chk == DHTLIB_OK) {
      Serial.print("   Temperature: ");
      Serial.println(DHT11.temperature);
      Serial.print("   Humidity: ");
      Serial.println(DHT11.humidity);
      Serial.print("   Check interval: ");
      Serial.print(temperatureInterval);
      Serial.println(" s.");
      if (DHT11.temperature >= temp_upp){
         if(ac_all == false) {
           Serial.println("   AC problem detected. Turning ON the two ACs.");
            // Set ALL ACs to run
            ac_all = true;
           check_ac();
           temperatureTime = (long)(millis() + MINIMUM_OVERHEAT_TIME);
           Serial.print("   Temperature check will be disabled for the next ");
           Serial.print(MINIMUM_OVERHEAT_TIME);
           Serial.println(" ms.");
         }

      }
      else if (ac_all == true && DHT11.temperature <= temp_low){
           ac_all = false;
           check_ac(); 
           // This assures that the AC we just shut down will remain off for at least 4 minutes.
           temperatureTime = millis() + TEMPERATURE_T0;   // Set the 1st temperature check to 4 minutes from now
           rotationTime = rotationTime + ROTATION_T0;      // Set the 1st rotation check to 5 minutes later than the programmed

      }
    } else {
       Serial.println("   Error reading temperature/humidity sensor!");
    }
}


// Turns ON/OFF the ACs
void check_ac()
{
	if (ac_all == true)    // Turn both ACs
	{
		digitalWrite(AC1PIN, HIGH); 
		digitalWrite(AC2PIN, HIGH);
	}
	else if (ac_now == 0)		// Turn AC1 only
	{
	    digitalWrite(AC1PIN, HIGH);
		digitalWrite(AC2PIN, LOW);
	}
	else if (ac_now == 1)		// Turn AC2 only
	{
	    digitalWrite(AC2PIN, HIGH);
	    digitalWrite(AC1PIN, LOW);
	}
}


// Rotate the ACs
void ac_rotate()
{
      	Serial.println("-- AC rotating function called --");
	Serial.print("   AC before: ");
 	Serial.println(ac_now);
	if (ac_now == 0)
	{
		ac_now = 1;
	}
	else if (ac_now == 1)
	{
	    ac_now = 0;
	}
  	Serial.print("   AC after: ");
	Serial.println(ac_now);
        rotationTime = (long)(millis() + rotationInterval_h * 3600000 + rotationInterval_m * 60000); // Program the time to the next rotation.
}		

// Run commands via HTTP requests
void run_command()
{
  	/// COMMANDS section
        if (cmd == "temp_low"){
//          Serial.print("   Configuring temp_low to ");
//          Serial.println(arg.toInt());
          write_to_eeprom(TEMP_LOWADDR, arg.toInt());
          read_from_eeprom();
//          temperature_check();
          cmd_status = 1;
        }
        else if (cmd == "temp_upp"){
//          Serial.print("   Configuring temp_upp to ");
//          Serial.println(arg.toInt());
          write_to_eeprom(TEMP_UPP_ADDR, arg.toInt());
          read_from_eeprom();
//          temperature_check();
          cmd_status = 1;
        }
        else if (cmd == "temp_int"){
//          Serial.print("   Configuring temp_interval to ");
//          Serial.println(arg.toInt());
          write_to_eeprom(TEMP_INTERVAL_ADDR, arg.toInt());
          read_from_eeprom();
//          temperatureTime = (long)(millis()) + temperatureInterval * 1000; // Set next temperature check for now + temperatureInterval
          cmd_status = 1;
        }
        else if (cmd == "rot_int"){
          byte ind1;
          ind1 = arg.indexOf(":");
//          Serial.print("   Configuring rot_interval to ");
//          Serial.println(arg);
          write_to_eeprom(ROTATION_INTERVAL_H_ADDR, arg.substring(0, ind1).toInt()); // Write hours
          write_to_eeprom(ROTATION_INTERVAL_M_ADDR, arg.substring(ind1+1, arg.length()).toInt()); // Write minutes
          read_from_eeprom();
//          rotationTime = (long)(millis() + rotationInterval_h * 3600000 + rotationInterval_m * 60000); // Set next AC rotation for now + rotationInterval
          cmd_status = 1;
        }
          else if (cmd == "rot_now"){
//          Serial.println("   FORCED to rotate the AC units.");
          ac_rotate();
          check_ac();
          cmd_status = 1;
        }
          else if (cmd == "temp_now"){
//          Serial.println("   FORCED to check the temperature.");
          temperature_check();
          cmd_status = 1; 
        }
        else {
          cmd_status = 3;
        }
} 

/*
		SETUP AND INIT
*/

void setup()
{
  /* This is to test  millis() overflow.  */ /*
  delay(2000);
  cli(); //halt the interrupts
  timer0_millis = 4294950000UL; //change the value of the register
  sei(); //re-enable the interrupts 
//  */  
  
  // SETUP PINS AND TIME TO START THE UNITS //
  pinMode(AC1PIN, OUTPUT);
  pinMode(AC2PIN, OUTPUT);
  digitalWrite(AC1PIN, LOW);
  digitalWrite(AC2PIN, LOW);
  
  // CONFIGURE SERIAL PORT FOR DEBUGGING //
  Serial.begin(57600);
  Serial.println("AC ROTATOR - William Schoenell - <william@iaa.es>");
  Serial.println("http://github.com/wschoenell/DataCenterACRotation");
  Serial.print("DHT11 LIBRARY VERSION: ");
  Serial.println(DHT11LIB_VERSION);
  Serial.print("PROGRAM VERSION: ");
  Serial.println(PROG_VERSION);
  Serial.println();
  
  // Read EEPROM configuration
  read_from_eeprom();
  temperatureTime = millis() + TEMPERATURE_T0;   // Set the 1st temperature check to 4 minutes from now
  rotationTime = millis() + ROTATION_T0;      // Set the 1st rotation check to 5 minutes from now

  // Start the Ethernet connection and the server //
  byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
  Ethernet.begin(mac); // Automatic IP via DHCP , ip);
  server.begin();
  Serial.println("-- Starting WebServer --");
  Serial.print("   server is at ");
  Serial.println(Ethernet.localIP());
}

/*
		MAIN LOOP
*/

void loop()
{
  EthernetClient client = server.available();
  if (client) {
    Serial.println("-- New Ethernet Client --");
    // an http request ends with a blank line
    cmd_status = 0;
    boolean currentLineIsBlank = true;
    String read_string = String(50); //string for fetching data from address
    byte ind1 = 0;
    byte ind2 = 0;

      while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        read_string += c;
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
		  	if(read_string.indexOf("cmd") >=0) {
		  	  //100GET /?cmd=pump&arg=13 HTTP/1.1
			  ind1 = read_string.indexOf('=');
			  ind2 = read_string.indexOf('&');
			  cmd = read_string.substring(ind1+1, ind2);
			  Serial.print("   Command is: ");
			  Serial.println(cmd);
			  ind1 = read_string.indexOf("arg");
			  if(ind1 >=0){
			    ind1 = ind1+4;
				ind2 = read_string.indexOf(" ", ind1);
				arg = read_string.substring(ind1, ind2);
				Serial.print("   Argument is: ");
				Serial.println(arg);
				if (arg.length() != 0)
					run_command();
           }
 }
          read_string = "";
		  cmd = "";
		  arg = "";

          chk = DHT11.read(DHT11PIN);
        
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");  // the connection will be closed after completion of the response
          client.println();
          client.println("temp,hum,act_unit,slaves_on,cfg_temp_low,cfg_temp_upp,cfg_temp_int,cfg_rot_int,cmd_status");
          client.print((float)DHT11.temperature, 2);
          client.print(",");
          client.print((float)DHT11.humidity, 2);
          client.print(",");
          client.print(ac_now);
          client.print(",");
          client.print(ac_all);
          client.print(",");
          client.print((float)temp_low, 1);
          client.print(",");
          client.print((float)temp_upp, 1);
          client.print(",");
          client.print(temperatureInterval);
          client.print(",");
          client.print(rotationInterval_h);
          client.print(":");
          client.print(rotationInterval_m);
          client.print(",");
          client.print(cmd_status);
          client.println();
//          client.println("<br />");
//          client.println("</html>");
          break;
        }
        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
        } 
        else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();
    Serial.println("   Client disonnected.");
  }
  
  // Check ambient temperature
  if ( (long)(millis() - temperatureTime) >= 0 )
  {
        temperatureTime = (long)(millis() + temperatureInterval * 1000);
	temperature_check();

  }
  
  // Check AC rotation
  if (  (long)(millis() - rotationTime) >= 0 )
  {
    ac_rotate();
    check_ac();
  }

}
