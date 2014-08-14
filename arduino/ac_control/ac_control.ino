//the following line tells the compiler that
//timer0_millis is declared somewhere else
// extern unsigned long timer0_millis;

#include <EEPROM.h>

/*
 AC datacenter rotator
*/

#define PROG_VERSION 0.1

/*
		CONFIGURATIONS
*/
#define DHT11PIN 2
#define BUTTON1PIN 0

#define AC1PIN 9
#define AC2PIN 8

int temp_low = EEPROM.read(0); // oC
int temp_upp = EEPROM.read(1); // oC

long temperatureInterval = EEPROM.read(2) * 1000; // in seconds
long rotationInterval = EEPROM.read(3) * 3600000 + EEPROM.read(4) * 60000; // hours + minutes

/*
		LOAD LIBRARIES
*/

// - Ethernet library
#include <SPI.h>
#include <Ethernet.h>
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
//IPAddress ip(192,168,1,50);

// - SD Card library
// #include <SD.h>

// - Temp and humidity sensor
#include <dht11.h> 
dht11 DHT11;

// Initialize the Ethernet server library
// with the IP address and port you want to use
// 80 is the default HTTP port. 
EthernetServer server(80);

///////////////////////
String read_string = String(50); //string for fetching data from address
String cmd = String(10);
String arg = String(10);
String arg1 = String(5);
String arg2 = String(5);
byte ind1 = 0;
byte ind2 = 0;
///////////////////////

// ------------------ FUNCTIONS -------------------------
long time_now;
long temperatureTime; // next millis() which should check the temperature
long rotationTime; // next millis() which should rotate the ACs
boolean ac_all = false; // If true, will START with both engines.
int ac_now = 1; // Will start with engine 0
byte chk; // To DHT11


// Maximum temperature check function
// Checks periodically the local temperature. If goes over a treshold, turn the two AC units.
void temperature_check()
{
  Serial.println("-- Temperature check function called --");
  
  chk = DHT11.read(DHT11PIN);

  if (chk == DHTLIB_OK) {
      Serial.print("   Humidity: ");
      Serial.println(DHT11.humidity);
      Serial.print("   Temperature: ");
      Serial.println(DHT11.temperature);
      Serial.print("   Check interval: ");
      Serial.print(temperatureInterval);
      Serial.println(" ms.");
      if (DHT11.temperature >= temp_upp){
         if(ac_all == false) {
           Serial.println("   AC problem detected. Turning ON the two ACs");
            // Set ALL ACs to run
            ac_all = true; 
           check_ac();
         }

      }
      else if (ac_all == true && DHT11.temperature <= temp_low){
           ac_all = false;
           check_ac(); 
      }
    } else {
       Serial.println("   Error reading temperature/humidity sensor!");
    }
}

void check_ac()
{
	if (ac_all == true)    // Turn both ACs
	{
		digitalWrite(AC1PIN, LOW); 
		digitalWrite(AC2PIN, LOW);
	}
	else if (ac_now == 0)		// Turn AC1 only
	{
	    digitalWrite(AC1PIN, LOW);
		digitalWrite(AC2PIN, HIGH);
	}
	else if (ac_now == 1)		// Turn AC2 only
	{
	    digitalWrite(AC2PIN, LOW);
	    digitalWrite(AC1PIN, HIGH);
	}
}

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
	Serial.print("   Rotation interval: ");
        Serial.print(rotationInterval);
        Serial.println(" ms.");
        
}		

void run_command()
{
	Serial.print("Running command ");
	Serial.print(cmd);
	Serial.print(" with ");
	Serial.print(arg);
	Serial.println(" as argument.");
	
	/// COMMANDS section
}  

// ------------------ END FUNCTIONS ------------------

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
  digitalWrite(AC1PIN, HIGH);
  digitalWrite(AC2PIN, HIGH);
  temperatureTime = millis() + 24000; //0;   // Set the 1st temperature check to 4 minutes from now
  rotationTime = millis() + 30000; //0;      // Set the 1st rotation check to 5 minutes from now
  
  // CONFIGURE SERIAL PORT FOR DEBUGGING //
  Serial.begin(57600);
  Serial.println("AC ROTATOR - William Schoenell - <william@iaa.es>");
  Serial.print("DHT11 LIBRARY VERSION: ");
  Serial.println(DHT11LIB_VERSION);
  Serial.print("PROGRAM VERSION: ");
  Serial.println(PROG_VERSION);
  Serial.println();

  // Start the Ethernet connection and the server:
  Ethernet.begin(mac); // Automatic IP via DHCP , ip);
  server.begin();
  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());
}

/*
		MAIN LOOP
*/

void loop()
{
  EthernetClient client = server.available();
  if (client) {
    Serial.println("new client");
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;

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
			  Serial.print("Command is: ");
			  Serial.println(cmd);
			  ind1 = read_string.indexOf("arg");
			  if(ind1 >=0){
			    ind1 = ind1+4;
				ind2 = read_string.indexOf(" ", ind1);
				arg = read_string.substring(ind1, ind2);
				Serial.print("Argument is: ");
				Serial.println(arg);
				Serial.println(read_string);
				if (arg.length() != 0)
					run_command();
			  }
           }
          read_string = "";
		  cmd = "";
		  arg = "";
        
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");  // the connection will be closed after completion of the response
//	      client.println("Refresh: 5");  // refresh the page automatically every 5 sec
          client.println();
          client.println("<!DOCTYPE HTML>");
          client.println("<html>");
          chk = DHT11.read(DHT11PIN);
          client.print("Temperature: ");
          client.print((float)DHT11.temperature, 2);
          client.println("<br />");
          client.print("Humidity: ");
          client.print((float)DHT11.humidity, 2);
          client.println("<br />");
          client.print("Configuration temp_upp, temperatureInterval, rotationInterval: ");
          client.print((float)temp_upp, 1);
          client.print((float)temperatureInterval, 1);
          client.print((float)rotationInterval, 1);
          client.println("<br />");
          client.println("</html>");
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
    Serial.println("client disonnected");
  }
  
  time_now = millis(); // millisec
  
  // Check ambient temperature
  if ( (long)(time_now - temperatureTime) >= 0 )
  {
	temperature_check();
//	check_ac();
        temperatureTime = time_now + temperatureInterval;

  }
  
  // Check AC rotation
  if (  time_now - rotationTime >= 0 )
  {
//    temperature_check();
    ac_rotate();
    check_ac();
    rotationTime = time_now + rotationInterval;
  }

}
