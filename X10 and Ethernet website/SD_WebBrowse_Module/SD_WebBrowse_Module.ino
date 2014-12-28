/************************************************************************/
/* This module accompanies the X10 Module to provide a Simple Website   */
/*                                                                      */
/* The Website serves pages from the SD Card. It also communicates      */
/* with the X10 Arduino using a serial connection with the EasyTransfer */
/* library. It can send commands to the X10 Arduino (generated on web   */
/* It also constantly receives the status of the lights and axtivity    */
/* over the serial connection, which it returns to the web client as a  */
/* JSON formatted string                                                */
/*                                                                      */
/************************************************************************/


#include <SD.h>

//This version includes a readout buffer that reads a specific amount of data from the SD card before it writes it to the wiznet chip.  This increases transfer
//speed considerably, up to 6x download speed.
#include <SPI.h>
#include <SdFat.h>
#include <SdFatUtil.h>
#include <Ethernet.h>
#include <EasyTransfer.h>

uint8_t bufindex;  //Buffer index, used to keep track of current buffer byte
const uint8_t maxbyte=32;  //The maximum allowable buffer length
uint8_t buf[maxbyte];  //Creates the buffer with a length equal to the max length

byte mac[] = {
  0x90,0xA2,0xDA,0x00,0x26,0xEB};
byte ip[] = {
  192,168,1,177};
char rootFileName[] = "app.htm";
EthernetServer server(80);

//create object
EasyTransfer ETin, ETout; 

struct RECEIVE_DATA_STRUCTURE{
  //put your variable definitions here for the data you want to receive
  //THIS MUST BE EXACTLY THE SAME ON THE OTHER ARDUINO
  boolean lightsOn[5];
  boolean roomOccupied[5];
  boolean linked[5];
  boolean heatingOn;
  int temperature;
};
struct SEND_DATA_STRUCTURE{
  //put your variable definitions here for the data you want to send
  //THIS MUST BE EXACTLY THE SAME ON THE OTHER ARDUINO
  char command[20];
};
//give a name to the group of data
SEND_DATA_STRUCTURE myOutput;
RECEIVE_DATA_STRUCTURE myInput;

Sd2Card card;
SdVolume volume;
SdFile root;
SdFile file;

#define error(s) error_P(PSTR(s))
void error_P(const char* str) {
  PgmPrint("error: ");
  SerialPrintln_P(str);
  if (card.errorCode()) {
    PgmPrint("SD error: ");
    Serial.print(card.errorCode(), HEX);
    Serial.print(',');
    Serial.println(card.errorData(), HEX);
  }
  while(1);
}

void setup() {
  Serial.begin(9600);
  
  //start the library, pass in the data details and the name of the serial port. Can be Serial, Serial1, Serial2, etc. 
  ETin.begin(details(myInput), &Serial);
  ETout.begin(details(myOutput), &Serial);
  
  //PgmPrint("Free RAM: ");
  //Serial.println(FreeRam());
  pinMode(10, OUTPUT);
  digitalWrite(10, HIGH);
  if (!card.init(SPI_FULL_SPEED, 4)) error("card.init failed!");
  if (!volume.init(&card)) error("vol.init failed!");
  //PgmPrint("Volume is FAT");
  //Serial.println(volume.fatType(),DEC);
  //Serial.println();
  if (!root.openRoot(&volume)) error("openRoot failed");
  //PgmPrintln("Files found in root:");
  //root.ls(LS_DATE | LS_SIZE);
  //Serial.println();
  //PgmPrintln("Files found in all dirs:");
  //root.ls(LS_R);
  //Serial.println();
  //PgmPrintln("Done");
  Ethernet.begin(mac, ip);
  server.begin();
}
#define BUFSIZ 100


void sendCommand(String inString)
{

    inString.toCharArray(myOutput.command, 20);
    ETout.sendData();
    delay(100);
}
  


int freeRam () {
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}

void loop()
{
  //check and see if a data packet has come in. 
  if(ETin.receiveData()){
    //you should make this delay shorter then your transmit delay or else messages could be lost
    delay(100);
    Serial.print('R');
    Serial.println(freeRam());
    /*String stringOne = "Lights: \n";
    for (int i=2;i<8;i++) { 
      stringOne += i;
      stringOne += ":";
      stringOne += myInput.lightsOn[i];
      stringOne += "\n";
    }
    logDebug(stringOne); 
    */
    
  }
    
  
  
  char clientline[BUFSIZ];
  char *filename;
  int index = 0;
  int image = 0;
  EthernetClient client = server.available();
  if (client) {
    Serial.print('W');
    Serial.println(freeRam());

    boolean current_line_is_blank = true;
    index = 0;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        if (c != '\n' && c != '\r') {
          clientline[index] = c;
          index++;
          if (index >= BUFSIZ)
            index = BUFSIZ -1;
          continue;
        }
        clientline[index] = 0;
        filename = 0;
        Serial.println(clientline);
        if (strstr(clientline, "GET / ") != 0) {
          filename = rootFileName;
        }
        if (strstr(clientline, "GET /") != 0) {
          if (!filename) filename = clientline + 5;
          (strstr(clientline, " HTTP"))[0] = 0;
          //Serial.println(filename);
          
          if (strstr(filename, "l.jso")!=0) {
            Serial.println("JSON");
            //bodge - spacers as some chars get removed over mobile network
            for(int i=0;i<5;i++) {
              client.println(" ");
            }
            client.println("{");
            for (int i=0;i<5;i++) { 
              client.print("\"");
              client.print(i);
              client.print("\": [");
              client.print(myInput.lightsOn[i]);
              client.print(",");
              client.print(myInput.roomOccupied[i]);
              client.print(",");
              client.print(myInput.linked[i]);
              client.print("]");
              if (i<4) {
                client.print(",");
              }
              client.println("");
            }         
            client.println("}");
            break;

            
          }
          if (strstr(filename, "cmd")!=0) {
            //Serial.println("receiving command");
            sendCommand(filename);
            //logDebug(filename);
            break; 
          }
          if (! file.open(&root, filename, O_READ)) {
            client.println("HTTP/1.1 404 Not Found");
            client.println("Content-Type: text/html");
            client.println();
            client.println("<h2>File Not Found!</h2>");
            break;
          }
          client.println("HTTP/1.1 200 OK");
          if (strstr(filename, ".htm") != 0)
            client.println("Content-Type: text/html");
          else if (strstr(filename, ".css") != 0)
            client.println("Content-Type: text/css");
          else if (strstr(filename, ".jpg") != 0)
            client.println("Content-Type: image/jpeg");
          else if (strstr(filename, ".js") != 0)
            client.println("Content-Type: application/x-javascript");
          else
            client.println("Content-Type: text");
          client.println();
          int16_t c;
          bufindex=0;  //reset buffer index
          while ((c = file.read()) >= 0) {
            buf[bufindex++]=((char)c);  //fill buffer
            if(bufindex==maxbyte)  //empty buffer when maximum length is reached.
            {
              client.write(buf, maxbyte);
              bufindex=0;
            }
          }
          file.close();  //close the file
          if(bufindex>0)  //most likely, the size of the file will not be an even multiple of the buffer length, so any remaining data is read out.
          {
            client.write(buf, bufindex);
          }
          bufindex=0; //reset buffer index (reset twice for redundancy
        } 
        else {
          client.println("HTTP/1.1 404 Not Found");
          client.println("Content-Type: text/html");
          client.println();
          client.println("<h2>File Not Found!</h2>");
        }
        break;
      }
    }
    delay(1);
    client.stop();
    //Serial.println("Finished");
  }
}
