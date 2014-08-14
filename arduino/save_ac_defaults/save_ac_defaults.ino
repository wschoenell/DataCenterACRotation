#include <EEPROM.h>

// This configure EEPROM values to the default

int a = 0;
int v;

// FUNCTIONS //
void write_to_eeprom(int addr, int value)
{
  Serial.print("EEPROM WRITE NEW VALUES -- ");
  v = EEPROM.read(addr);
  Serial.print("Address: ");
  Serial.print(addr);
  Serial.print(" Value: ");
  Serial.print(v);
  if (value != v){
    Serial.print(". Updating to: ");
    Serial.println(value);
    EEPROM.write(addr, value);
  } else {
    Serial.println(". Value correct!");
  }    
}
  
void setup()
{
  Serial.begin(57600);
  Serial.println("Writing defaults to the EEPROM");
}

void loop(){
  // Address: 0 - Low Temperature treshold (oC) - This temperature is only used to turn off one AC when there is two running.
  write_to_eeprom(a, 24);
  a = a+1;
  // Address: 1 - High Temperature treshold (oC)
  write_to_eeprom(a, 30);
  a = a+1;
  // Address: 2 - Temperature check Interval (in seconds)
  write_to_eeprom(a, 30);
  a = a+1;
  // Address: 3 - A/C rotation Interval (hours)
  write_to_eeprom(a, 0);
  a = a+1;
  // Address: 4 - A/C rotation Interval (minutes)
  write_to_eeprom(a, 40);
  a = a+1;

  
  while(a < 512){
    write_to_eeprom(a, 255);
    a++;
  }
  
  delay(600000);
}
