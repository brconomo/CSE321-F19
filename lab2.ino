#include <SPI.h>
#include <Ethernet.h>
#include <ArduinoHttpClient.h>
#include <EthernetUdp.h>

/****************************************************************************************************/
/* Project name: Plant Protector                                                                    */
/* Author: Ben Conomos                                                                              */
/* Hardware Needed: Arduino, Ethernet Shield, HC-SR04 Ultrasonic Sensor,                            */
/*                  Piezo speaker+100Ohm Resistor or other tone output circuitry                    */
/*                                                                                                  */
/* adapted from:                                                                                    */
/* Dejan Nedelkovski,                                                                               */
/* https://howtomechatronics.com/tutorials/arduino/ultrasonic-sensor-hc-sr04/                       */
/*                                                                                                  */
/* "madnerd"                                                                                        */
/* https://create.arduino.cc/editor/madnerd/429197a3-8a40-4d2c-bc16-1a502cb26cd9/preview            */
/*                                                                                                  */
/* Arduino WebClient Tutorial                                                                       */
/* https://www.arduino.cc/en/Tutorial/WebClient                                                     */
/*                                                                                                  */
/* Arduino UdpNTPClient Tutorial                                                                    */
/* https://www.arduino.cc/en/Tutorial/UdpNtpClient                                                  */
/****************************************************************************************************/

//USER CONFIGURABLES------------------------------------------------------------------------------
// --== Edit these to match your needs. ==--

//APP SETTINGS------------------------------------------------------------------------------------
// Your UTC timezone
  const int timezone = -5;
// When you leave for work in military time. MAKE SURE TO FOLLOW THE FORMAT PRESENTED BELOW.
  const int wl = 730;
// When you get home from work
  const int wr = 1645;
// Your discord bot's webhook URL.
  const String discord_webhook = "https://discordapp.com/api/webhooks/XXXXXXXXXXXXXXXXXX/YYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYY";
// A local NTP server. You can find these easiy by using a search engine. The default is a good choice for the USA.
  const char timeServer[] = "time.nist.gov";
// The tolerance for the sensor detecting motion. Should be between 0 and 1.
// A good range is 0.3-0.6.
//Closer distances should have lower tolerances.
//Increase the tolerance if you have too many false positives.
  const float tol = 0.5;

//ETHERNET CONFIG----------------------------------------------------------------------------------
// Enter a MAC address for your controller below.
// Newer Ethernet shields have a MAC address printed on a sticker on the shield
  byte mac[] = { 0x90, 0xA2, 0xDA, 0x0D, 0x3E, 0x89 };

// Set the static IP address and DNS to use as a fallback if the DHCP fails to assign.
// These values should be fine in most home networks. If you have to change them,
// you probably know what they mean.
  IPAddress ip(192, 168, 1, 177);
  IPAddress myDns(192, 168, 1, 1);

//set to false if not connected to the internet. I'd test for this but the library function
//to do so is currently broken. I decided to have a manual toggle here as a workaround.
//It will fall back to always using the buzzer.
  bool conn = true;

//USER CONFIGURABLES END---------------------------------------------------------------------------



//Discord webhook init------------------------------------------------------------------------------
// Initialize the Ethernet client library
// with the IP address and port of the server
// that you want to connect to (port 80 is default for HTTP):
EthernetClient client;
const char dest[] = "discordapp.com";
HttpClient webhook = HttpClient(client, dest, 80);
int lastAlert = 0;

//NTP UDP init--------------------------------------------------------------------------------------

unsigned int localPort = 8888;      // local port to listen for UDP packets
//const char timeServer[] = "tock.cse.buffalo.edu"; // ub cse NTP server
const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets
int mt = 0;
int ntpPoll = 0;
EthernetUDP Udp;

//sensor init----------------------------------------------------------------------------------------

// defines pin numbers
const int trigPin = 2;
const int echoPin = 3;
const int buzz = 7;
// defines variables
int distance;
int loops;
double avgDistance;

//end config------------------------------------------------------------------------------------------

void setup() {
  
  //eth setup------------------------------------------------------------------------------------------
  if (conn) {
    Ethernet.init(10);  // Most Arduino shields
    Serial.println("Initialize Ethernet with DHCP:");
    if (Ethernet.begin(mac) == 0) {
      Serial.println("Failed to configure Ethernet using DHCP");
      // Check for Ethernet hardware present
      if (Ethernet.hardwareStatus() == EthernetNoHardware) {
        Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
        while (true) {
          delay(1); // do nothing, no point running without Ethernet hardware
        }
      }
      //LINK STATUS is broken as of the current ethernet library release. Reenable if it's fixed,
      //and deprecate the option to set bool conn in the configurables.
      /* 
      if (Ethernet.linkStatus() == LinkOFF) {
        Serial.println("Ethernet cable is not connected.");
        conn = false;
      } 
      */
      // try to congifure using IP address instead of DHCP:
      Ethernet.begin(mac, ip, myDns);
    } else {
      //Serial.print("  DHCP assigned IP ");
      //Serial.println(Ethernet.localIP());
    }
    Udp.begin(localPort);
  }
 
  //sens setup-----------------------------------------------------------------------------------------

  
  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT); // Sets the echoPin as an Input
  Serial.begin(9600); // Starts the serial communication
  avgDistance = getDistance(); // Calculating the starting distance
}

//end setup-----------------------------------------------------------------------------------------------

void loop() {
  //NTP handler------------------------------------------------------------------------------------------
  // this is expensive, only do it every minute or so. Accuracy isn't super important.
  if (conn && ntpPoll <= 0) {
    mt = militaryTimeNTP();
    // Call NTP every ntpPoll loops
    // Bear in mind NTP is UDP and can potentially fail when picking this value. 
    ntpPoll = 1000; 
    Serial.print("--------------------------------------");
    Serial.println(mt); 
  }
  
  distance = getDistance(); //get the distance per run
  
  //Detect and handle anomalies vs. avg--------------------------------------------------------------------------------------
  if (distance < avgDistance * (1.0-tol) || distance > avgDistance * (1.0+tol)) {
    // alarm trips if 30 event loops in a row, reset avg distance as something is wrong.
    // usually results from an environment change (device bumped, object moved, etc.)
    if (loops >= 300) {
      loops = 0;
      avgDistance = distance;
    }
    if (conn && !(wl < mt && mt < wr)) {     // If connected and not at work, check to send text. Else buzz
      if (lastAlert <= 0) {                 // If we haven't texted recently, send another text. This reduces spam.
        lastAlert = 500;       // Wait X loops before sending another text
        
        // post message to discord
        webhook.post(discord_webhook, "application/json", "{\"content\":\"Help! The cat is eating me!\", \"tts\":false}");
        
        // having these calls will force the tcp session to end gracefully, even if their responses are unused.
        webhook.responseStatusCode();
        webhook.responseBody();
      } else delay(500);         // burn when no msg sent but still sensed
    } else  {
      tone(buzz, 11500);
      delay(500);
      noTone(buzz);
    }
    // Loop alarm weighted in favor of event loops, as they take far longer.
    // Requires 10 non-event loops to counteract
    // Prevents scenarios where environment changed to just on border of tol continuing to alert forever with short breaks.
    loops = loops + 10;
  } 
  // No anomaly detected--------------------------------------------------------------------------------------------------
  else {
    // No anomaly detected based on tol, include distance in avg
    // and decrease loop alarm count by 1.
    avgDistance = avgDistance * .92 + distance * .08;
    if (loops > 0) loops--;
  }
  // Count down lastAlert and ntpPoll alarms once per loop.
  if (lastAlert > 0) {
    lastAlert--;
  }
  if (ntpPoll > 0) {
    ntpPoll--;
  }
  // Renew DHCP lease.
  if (conn) Ethernet.maintain();
}

// Adapted from Dejan Nedelkovski
// Returns distance from the sensor in centimeters
int getDistance() {
  int retVal = 0;
  long duration;
  // Clears the trigPin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(5);
  
  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration = pulseIn(echoPin, HIGH);
  
  // Calculating the distance
  retVal = duration*0.034/2;
  
  // Prints the distance on the Serial Monitor.
  //<!CAUTION!>
  //REMOVING THESE PRINTS SOMEHOW MAKES THE SENSOR SIGNIFICANTLY LESS RELIABLE. DON'T DO IT.
  //ADDING DELAYS IN THEIR PLACE DOES NOT SOLVE THE ISSUE. I'VE TRIED.
  Serial.print("Distance: ");
  Serial.println(retVal);
  Serial.print("Avg. Distance: ");
  Serial.println(avgDistance);
  
  return retVal;
}

//Adapted from Arduino example UdpNTPClient
//Returns the time to the minute in military time.
int militaryTimeNTP() {
  sendNTPpacket(timeServer); // send an NTP packet to a time server

  // wait to see if a reply is available
  delay(1000);
  if (Udp.parsePacket()) {
    int hr;
    int minute;
    // We've received a packet, read the data from it
    Udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer

    // the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, extract the two words:

    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    unsigned long secsSince1900 = highWord << 16 | lowWord;
    //Serial.print("Seconds since Jan 1 1900 = ");
    //Serial.println(secsSince1900);

    // now convert NTP time into everyday time:
    //Serial.print("Unix time = ");
    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    const unsigned long seventyYears = 2208988800UL;
    // subtract seventy years:
    unsigned long epoch = secsSince1900 - seventyYears + (timezone * 3600);
    // print Unix time:
    //Serial.println(epoch);

    /*NTP DEBUG
    // print the hour, minute and second:
    Serial.print("The local time is ");       // Local time is computed with timezone user config
    Serial.print((epoch  % 86400L) / 3600); // print the hour (86400 equals secs per day)
    Serial.print(':');
    if (((epoch % 3600) / 60) < 10) {
      // In the first 10 minutes of each hour, we'll want a leading '0'
      Serial.print('0');
    }
    Serial.print((epoch  % 3600) / 60); // print the minute (3600 equals secs per minute)
    Serial.print(':');
    if ((epoch % 60) < 10) {
      // In the first 10 seconds of each minute, we'll want a leading '0'
      Serial.print('0');
    }
    Serial.println(epoch % 60); // print the second
    */
    hr = (epoch  % 86400L) / 3600;
    minute = (epoch  % 3600) / 60;
    return hr*100 + minute;
  }
}

// Used from Arduino example UdpNTPClient
// send an NTP request to the time server at the given address
void sendNTPpacket(const char * address) {
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); // NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}
