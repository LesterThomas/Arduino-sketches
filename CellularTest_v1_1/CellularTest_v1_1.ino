/* SparkFun Cellular Shield - Pass-Through Sample Sketch SparkFun Electronics Written by Ryan Owens CC by v3.0 3/8/10 Thanks to Ryan Owens and Sparkfun for sketch */ 
#include <SoftwareSerial.h>  //Include the NewSoftSerial library to send serial commands to the cellular module. 
#include <string.h>         //Used for string manipulations 
#include <stdlib.h>

char incoming_char=0;      //Will hold the incoming character from the Serial Port. 
String inputStr="";
String textFrom="";
String textMessage="";
//TMP36 Pin Variables
int temperaturePin = 0; //the analog pin the TMP36's Vout (sense) pin is connected to
int sendTextPin = 1;
float temperature = 0;      //the resolution is 10 mV / degree centigrade 
                        //(500 mV offset) to make negative temperatures an option
int lightsPin=13;
int sendStatusPin=12;
int activePin=11;


// Variables will change:
int ledState = LOW;             // ledState used to set the LED
long previousMillis = 0;        // will store last time LED was updated
long interval = 1000;           // interval at which to blink (milliseconds)

SoftwareSerial cell(2,3);  //Create a 'fake' serial port. Pin 8 is the Rx pin, pin 3 is the Tx pin. //Lester changed 2 to 8

/*
 * getVoltage() - returns the voltage on the analog input defined by
 * pin
 */
float getVoltage(int pin){
 return (analogRead(pin) * .004882814); //converting from a 0 to 1023 digital range
                                        // to 0 to 5 volts (each 1 reading equals ~ 5 millivolts
}

boolean getSendText(int pin) {
  int value= analogRead(pin);
  if (value>500){
    return true;
  } else {
    return false;
  }

}

void blink()
{
  // here is where you'd put code that needs to be running all the time.

  // check to see if it's time to blink the LED; that is, if the 
  // difference between the current time and last time you blinked 
  // the LED is bigger than the interval at which you want to 
  // blink the LED.
  unsigned long currentMillis = millis();
 
  if(currentMillis - previousMillis > interval) {
    // save the last time you blinked the LED 
    previousMillis = currentMillis;   

    // if the LED is off turn it on and vice-versa:
    if (ledState == LOW) {
      ledState = HIGH;
      Serial.println(temperature);
    }
    else {
      ledState = LOW;
      Serial.println(temperature);
    }
    
    // set the LED with the ledState of the variable:
    digitalWrite(activePin, ledState);
    int val=analogRead(sendTextPin);
    Serial.println(val);
  }
 
  if (getSendText(sendTextPin)) {
    Serial.println("Button pressed - send a text");
    textFrom="07785246418";
    writeSMS("Hi from Lesters House");
    delay(2000);
  }
  
}

void setup() {    
  pinMode(lightsPin, OUTPUT); 
  pinMode(activePin, OUTPUT); 
  pinMode(sendStatusPin, OUTPUT);
  Serial.begin(9600); //Initialize serial ports for communication.
  cell.begin(9600); 
  Serial.println("Starting SM5100B Communication..."); 
  int i=0;
  int complete=0;
  while(complete==0)  {
    delay(1000);
    Serial.print(".");
    if(cell.available() >0) { 
      Serial.println("");
      Serial.println("Data available");
      while(cell.available()>0) {
        incoming_char=cell.read();    //Get the character from the cellular serial port. 
        inputStr+=incoming_char;
      }
      Serial.println("<"+inputStr+">");  //Print the incoming character to the terminal. 
      if (inputStr.indexOf("+SIND: 4")!=-1) {
        complete=1;
        Serial.println("Matched");
      }
      inputStr="";
    } //If a string is coming from the terminal to the Arduino... 
  }
  cell.println("AT+COPS?"); // Query the Network name
  printStatus();
  
  delay(200); 
  cell.println("AT+CMGF=1"); // set SMS mode to text 
  delay(200); 
  cell.println("AT+CNMI=3,3,0,0"); // set module to send SMS data to serial out upon receipt 
  delay(200);
  Serial.println("Ready"); 
} 

void writeSMS(String inSMS) {
  digitalWrite(sendStatusPin, HIGH);  //Set LED to show we are sending message
  //compose reply
  Serial.println("");
  Serial.println("Composing reply with text <"+inSMS+">");
  cell.println("AT+CMGF=1"); // set SMS mode to text
  cell.print("AT+CMGS=");  // now send message... 
  cell.print((char)34); // ASCII equivalent of " 
  cell.print(textFrom); 
  cell.println((char)34);  // ASCII equivalent of " 
  delay(500); // give the module some thinking time 
  cell.print(inSMS);   // our message to send 
  cell.print((char)26);  // ASCII equivalent of Ctrl-Z 
  Serial.println("Reply sent");
  Serial.println("");
  delay(1000); // the SMS module needs time to return to OK status 
  printStatus();

  Serial.println("End writeSMS");
  digitalWrite(sendStatusPin, LOW);
}

void printStatus() {
 //print any status to the monitor
 Serial.println("");  
 Serial.println("Getting status");  
 int complete=0;
 while (complete==0) {
    while(cell.available() >0) { 
      incoming_char=cell.read();    //Get the character from the cellular serial port. 
      Serial.print(incoming_char); 
      complete=1;
      } 
 }
   Serial.println("Status complete");   
}
void loop() {  
  
   //get temp from Sensor
   temperature = getVoltage(temperaturePin);  //getting the voltage reading from the temperature sensor
   temperature = (temperature - .5) * 100;          //converting from 10 mv per degree wit 500 mV offset
  
  blink(); //show we are active by blinking LED
  if(cell.available() >0) {//If a character is coming from the cellular shield...
    Serial.println("");
    while(cell.available() >0) {   //get all available characters
      incoming_char=cell.read();   //Get the character from the cellular serial port. 
      Serial.print(incoming_char); //display on terminal
      inputStr+=incoming_char;
      }  
 
                                                            //to degrees ((volatge - 500mV) times 100)
   
    //Parse inputStr to see if text has been sent
    if (inputStr.indexOf("+CMT:")!=-1) {
        Serial.println("Text Arrived");
        textFrom= inputStr.substring(7,20);
        textMessage=inputStr.substring(63);
        Serial.println("");
        Serial.println("From <"+textFrom+">");
        Serial.println("Message <"+textMessage+">");
        Serial.println("");
        
        Serial.println("Deleting all SMS");
        cell.println("AT+CMGD=1,4"); // delete all SMS
             
        //parse message
        if (inputStr.indexOf("?")!=-1) {
          writeSMS("Available commands: #Temp# Get the inside temperature  #Light#On/Off Turn LED Light on/off");
        } else if (inputStr.indexOf("#Temp#")!=-1) {
           char  tempStr[20];    
           dtostrf(temperature,3,1,tempStr); //float to string
           String tempString=tempStr;
           String smsText="The temperature is ";
           smsText+=tempString;
           smsText+=" C";
           writeSMS(smsText);          
        }  else if (inputStr.indexOf("#Light#")!=-1) {
          if (inputStr.indexOf("On")!=-1) {
            digitalWrite(lightsPin, HIGH);
            writeSMS("Light is turned On");
          }
          else if (inputStr.indexOf("Off")!=-1) {
            digitalWrite(lightsPin, LOW);
            writeSMS("Light is turned Off");
          }
          else {
            writeSMS("Do you want the light turned On or Off? Use command #Light#On or #Light#Off");
          }
        
        } else {
          Serial.println("");
          Serial.println("No commands in message");
        } 
      }
    inputStr="";
    }
    
  //Any characters from terminal send to CellularShield
  while(Serial.available() >0) { 
      incoming_char=Serial.read();  //Get the character coming from the terminal 
      cell.print(incoming_char);    //Send the character to the cellular module. 
  } 
}
