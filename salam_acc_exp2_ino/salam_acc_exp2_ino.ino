/**
 * Transmitting Accelerometer data by efficient call-response using ack-payloads 
 * 
 * This example continues to make use of all the normal functionality of the radios including 
 * the auto-ack and auto-retry features, but allows ack-payloads to be written optionlly as well. 
 * This allows very fast call-response communication, with the responding radio never having to 
 * switch out of Primary Receiver mode to send back a payload, but having the option to switch to 
 * primary transmitter if wanting to initiate communication instead of respond to a commmunication. 
 */
 
#include <SPI.h>
#include <Wire.h>
#include "RTClib.h"
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"

//RTC
RTC_DS1307 RTC;

// Hardware configuration: Set up nRF24L01 radio on SPI bus plus pins 7 & 8 
RF24 radio(7,8);
                                                                           // Topology
byte addresses[][6] = {"1Node","2Node"};              // Radio pipe addresses for the 2 nodes to communicate.

// Role management: Set up role.  This sketch uses the same software for all the nodes
// in this system.  Doing so greatly simplifies testing.  
typedef enum { transmit = 1, recieve } role_e;                 // The various roles supported by this sketch
const char* role_friendly_name[] = { "invalid", "Transmit", "Receive"};  // The debug-friendly names of those roles
role_e role = transmit;                                              // The role of the current running sketch

struct node_data
{
  /*
    By using a struct, everytime something new is introduced, the sizeof
    function will automatically know the size of the struct payload in bytes
    
    Adding new values means each node will need to know the same value to pass messages
    upstream
  */
  int accelerometer[3];
  uint32_t time;
}data;

void setup()
{
  Serial.begin(9600);
  printf_begin();
  printf("\n\rRF24/examples/GettingStarted/\n\r");
  printf("ROLE: %s\n\r",role_friendly_name[role]);
  printf("*** PRESS 'T' to begin recieving from the other node\n\r");

  // Setup and configure the RTC
  Wire.begin();
  RTC.begin();
 
  if (! RTC.isrunning()) 
  {
    Serial.println("RTC is NOT running!");
    // following line sets the RTC to the date & time this sketch was compiled
    // uncomment it & upload to set the time, date and start run the RTC!
    RTC.adjust(DateTime(__DATE__, __TIME__));//set the RTC
  }
  
  // Setup and configure radio
  radio.begin();
  radio.setAutoAck(1);                    // Ensure autoACK is enabled
  radio.enableAckPayload();               // Allow optional ack payloads
  radio.setRetries(0,15);                 // Smallest time between retries, max no. of retries
  radio.setPayloadSize(sizeof(data));                // Here we are sending 1-byte payloads to test the call-response speed
  radio.openWritingPipe(addresses[1]);        // Both radios listen on the same pipes by default, and switch when writing
  radio.openReadingPipe(1,addresses[0]);      // Open a reading pipe on address 0, pipe 1
  radio.startListening();                 // Start listening
  radio.powerUp();
  radio.printDetails();                   // Dump the configuration of the rf unit for debugging
}

void loop(void) 
{
  /****************** Transmit Role ***************************/
  if (role == transmit)
  {// Radio is in Transmit mode
    //get the time recorded at
    DateTime now = RTC.now();
    
    byte gotByte;// Initialize a variable for the incoming response      
    
    data.time = now.unixtime();//seconds since the 1/1/1970
    data.accelerometer[0] = analogRead(0);// X
    data.accelerometer[1] = analogRead(1);// Y
    data.accelerometer[2] = analogRead(2);// Z
    
    radio.stopListening();                                // First, stop listening so we can talk.                                           
    if ( radio.write(&data,sizeof(data) ))
    {// Send the acc variables to the other radio 
        if(!radio.available())
        { // If nothing in the buffer, we got an ack but it is blank
          Serial.print(data.accelerometer[0]);
          Serial.print(",");
          Serial.print(data.accelerometer[1]);
          Serial.print(",");
          Serial.println(data.accelerometer[2]);
          Serial.print("Sent:Time Taken @");
          Serial.println(data.time);
        }
    }
    delay(500);  // Try again later
  }


  /****************** Recieve Role ***************************/

  if ( role == recieve ) 
  {
    byte pipeNo, gotByte;                          // Declare variables for the pipe and the byte received
    while( radio.available(&pipeNo))
    {// Read all available payloads
      radio.read( &data,sizeof(data));                   
      // Since this is a call-response. Respond directly with an ack payload.
      Serial.print(data.accelerometer[0]);
      Serial.print(",");
      Serial.print(data.accelerometer[1]);
      Serial.print(",");
      Serial.println(data.accelerometer[2]);
      Serial.print("Received: Time Taken @");
      Serial.println(data.time);
      // Ack payloads are much more efficient than switching to transmit mode to respond to a call
      //radio.writeAckPayload(pipeNo,&gotByte, 1 );  // This can be commented out to send empty payloads.  
   }
 }



  /****************** Change Roles via Serial Commands ***************************/

  if ( Serial.available() )
  {
    char c = toupper(Serial.read());
    if ( c == 'T' )
    {
      if(role != transmit)
      {
        printf("*** CHANGING TO TRANSMIT ROLE -- PRESS 'R' TO SWITCH BACK\n\r");
  
        role = transmit;                      // Change roles (ping out)
        radio.openWritingPipe(addresses[0]);       // Open different pipes when writing. Write on pipe 0, address 0
        radio.openReadingPipe(1,addresses[1]);     // Read on pipe 1, as address 1
      }
    }
    else if ( c == 'R' )
    {
       if(role != recieve)
       {
         printf("*** CHANGING TO RECEIVE ROLE -- PRESS 'T' TO SWITCH BACK\n\r");
        
         role = recieve;                    // Become the primary receiver (pong back)
         radio.openWritingPipe(addresses[1]);      // Since only two radios involved, both listen on the same addresses and pipe numbers in RX mode
         radio.openReadingPipe(1,addresses[0]);    // then switch pipes & addresses to transmit. 
         radio.startListening();                   // Need to start listening after opening new reading pipes
       }
    }
  }
}
