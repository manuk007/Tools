/*
   Copyright (c) 2015, Majenko Technologies
   All rights reserved.

   Redistribution and use in source and binary forms, with or without modification,
   are permitted provided that the following conditions are met:

 * * Redistributions of source code must retain the above copyright notice, this
     list of conditions and the following disclaimer.

 * * Redistributions in binary form must reproduce the above copyright notice, this
     list of conditions and the following disclaimer in the documentation and/or
     other materials provided with the distribution.

 * * Neither the name of Majenko Technologies nor the names of its
     contributors may be used to endorse or promote products derived from
     this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
   ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
   DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
   ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
   (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
   LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
   ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/* Create a WiFi access point and provide a web server on it. */

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>

#ifndef APSSID
#define APSSID "Spartan"
#define APPSK  "India123"
#endif

/* Set these to your desired credentials. */
const char *ssid = APSSID;
const char *password = APPSK;

typedef enum
{
  START = 0, 
  AQUIRE, 
}STATE_TYPE;

STATE_TYPE STATE = START;
#define DEBUG 0
#define START_MSB 0x31
#define K_MAXTIMEOUT 5000 //5 sec timout

char Matlab[50];
bool newPack = false;
char EngData[50]; //buffer to hold incoming packet,
char EngDataSize=0; //buffer to hold incoming packet,
int DataBuffer[50];int CurrLen = 0;

char CT=0;
char CTD=0;
char LEDSTAT = 0;
unsigned long AQUIRE_START_TIME;
unsigned long AQUIRE_TIMEOUT;
unsigned long LED_START_TIME;
unsigned long LED_TIMEOUT;

char BurstBuffer[500];
char BurstBufferSize=0;
char BurstIndex=0;
char BurstNoOfPack=0;
char PackIndex=0;
char BurstBufferCopy[500];
char LastBurstBufferSize=0;

char BURSTSIZE = 50;

ESP8266WebServer server(80);

unsigned int localPort = 8888;      // local port to listen on
WiFiUDP Udp;
IPAddress ip(192, 168, 4, 2);
/* Just a little test message.  Go to http://192.168.4.1 in a web browser
   connected to this access point to see it.
*/
void handleRoot() {
  server.send(200, "text/html", "<h1>You are connected</h1>");
}

void setup() {
  delay(1000);
  /****************************************************************************************************/
  Serial.begin(921600);
  //Serial.begin(115200);
  while (!Serial) {;}
  //while(Serial.available()) Serial.read();
                                                                                              if(DEBUG)Serial.println();
                                                                                              if(DEBUG)Serial.print("Configuring access point...");
  /* You can remove the password parameter if you want the AP to be open. */
  WiFi.softAP(ssid, password);

  IPAddress myIP = WiFi.softAPIP();
                                                                                              if(DEBUG)Serial.print("AP IP address: ");
                                                                                              if(DEBUG)Serial.println(myIP);
  server.on("/", handleRoot);
  server.begin();
                                                                                              if(DEBUG)Serial.println("HTTP server started");
  
                                                                                              if(DEBUG)Serial.printf("UDP port %d\n", localPort);
  Udp.begin(localPort);
  AQUIRE_START_TIME = 0;
  pinMode(16, OUTPUT);     // Initialize the LED_BUILTIN pin as an output
}

void loop() { 
     switch (STATE) 
     {
     case START:
           if (Serial.available()>=3)
           {
                                                                                              if(DEBUG)Serial.write("GOT 3\n");
                 DataBuffer[CurrLen++] = Serial.read();
                 DataBuffer[CurrLen++] = Serial.read();
                 DataBuffer[CurrLen++] = Serial.read();
                 if((DataBuffer[0] == START_MSB) &&  (DataBuffer[1] == START_MSB))
                    {          STATE = AQUIRE; AQUIRE_START_TIME = millis();                  if(DEBUG)Serial.write  ("Moving to Aquire\n");   }
                 else
                    {          CurrLen = 0;  while(Serial.available()) Serial.read();  }
           }
           break;
     case AQUIRE:
           AQUIRE_TIMEOUT = millis() - AQUIRE_START_TIME;
           if((AQUIRE_TIMEOUT > K_MAXTIMEOUT) && !DEBUG)
           {
             STATE = START; CurrLen = 0;  while(Serial.available()) Serial.read(); break;
           }
           if (Serial.available()>=DataBuffer[2])
              {
                //Serial.write("GOT Data\n");
                newPack = true;
              for(int i = CurrLen; i < (DataBuffer[2]+3); i++)
                 {DataBuffer[CurrLen++] = Serial.read();}
              
              for(int i = 0; i < DataBuffer[2]+3; i++)
                 {EngData[i] = DataBuffer[i]; EngDataSize = DataBuffer[2]+3;}
                 Serial.write(DataBuffer[2]); 
                 STATE = START; CurrLen = 0;  while(Serial.available()) Serial.read();
              }           
       break;
       }
  DoWIFI();
}




void DoWIFI() {
  server.handleClient();
  LED_TIMEOUT = millis() - LED_START_TIME;
  if(newPack)
  {newPack = false;
    /*LED DISCRETE CONTROL */
    if(LED_TIMEOUT > 500)
    {
      LED_START_TIME = millis();
            if(LEDSTAT == 0)
                {  digitalWrite(16, LOW); LEDSTAT = 1;    }
            else
                {   digitalWrite(16, HIGH); LEDSTAT = 0;    }
    }
  CheckBuffer();
  }
}

void CheckBuffer()
  {
    BurstNoOfPack++;
    EngData[EngDataSize-1] = PackIndex;
    //EngDataSize = EngDataSize + 1;
    
    for(int i=0;i<EngDataSize;i++)
        {        BurstBuffer[BurstIndex++] = EngData[i];        }
        
    if(BurstNoOfPack == BURSTSIZE)
    { 
      PackIndex++;     
      BurstNoOfPack = 0;BurstIndex=0; 
      for(int i=0;i<BurstIndex;i++)        {        BurstBufferCopy[i] = BurstBuffer[i]; LastBurstBufferSize =EngDataSize*10;        }
      Udp.beginPacket(ip, 8888);
      Udp.write(BurstBuffer,EngDataSize*BURSTSIZE);
      Udp.endPacket();
      BurstNoOfPack = 0;
      BurstIndex = 0;
      newPack = false;
    }
  }
