#include <Ethernet.h>
#include <ArduinoHttpClient.h>

/*
* Alarm and Discord Bot
* Ben Conomos
* 
* adapted from: 
* Dejan Nedelkovski,
* https://howtomechatronics.com/tutorials/arduino/ultrasonic-sensor-hc-sr04/
*
* "madnerd"
* https://create.arduino.cc/editor/madnerd/429197a3-8a40-4d2c-bc16-1a502cb26cd9/preview
* 
* Arduino WebClient Tutorial
* https://www.arduino.cc/en/Tutorial/WebClient
* 
*/

//eth config----------------------------------------------------------------------------------------

// Enter a MAC address for your controller below.
// Newer Ethernet shields have a MAC address printed on a sticker on the shield
byte mac[] = { 0x90, 0xA2, 0xDA, 0x0D, 0x3E, 0x89 };

// Set the static IP address to use if the DHCP fails to assign
IPAddress ip(192, 168, 1, 177);
IPAddress myDns(192, 168, 1, 1);

// Initialize the Ethernet client library
// with the IP address and port of the server
// that you want to connect to (port 80 is default for HTTP):
EthernetClient client;
const char dest[] = "discordapp.com";
const String discord_webhook = "SECRET";
HttpClient webhook = HttpClient(client, dest, 80);


//sense config----------------------------------------------------------------------------------------

// defines pin numbers
const int trigPin = 2;
const int echoPin = 3;
const int buzz = 7;
// defines variables
long duration;
int distance;
int loops;
double avgDistance;

//end config------------------------------------------------------------------------------------------

void setup() {
  
  //eth setup------------------------------------------------------------------------------------------
  
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
    if (Ethernet.linkStatus() == LinkOFF) {
      Serial.println("Ethernet cable is not connected.");
    }
    // try to congifure using IP address instead of DHCP:
    Ethernet.begin(mac, ip, myDns);
  } else {
    Serial.print("  DHCP assigned IP ");
    Serial.println(Ethernet.localIP());
  }
  
  //sens setup-----------------------------------------------------------------------------------------

  
  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT); // Sets the echoPin as an Input
  Serial.begin(9600); // Starts the serial communication
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration = pulseIn(echoPin, HIGH);
  
  // Calculating the distance
  avgDistance = duration*0.034/2;
}

//end setup-----------------------------------------------------------------------------------------------

void loop() {

  //distance calculation----------------------------------------------------------------------------------
  
  // Clears the trigPin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  
  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration = pulseIn(echoPin, HIGH);
  
  // Calculating the distance
  distance= duration*0.034/2;
  
  // Prints the distance on the Serial Monitor
  Serial.print("Distance: ");
  Serial.println(distance);
  Serial.print("Avg. Distance: ");
  Serial.println(avgDistance);

  //Detect and handle anomalies vs. avg--------------------------------------------------------------------------------------
  
  if (distance < avgDistance * .5 || distance > avgDistance * 1.5 && loops < 25) {
    if (loops >= 30) {
      avgDistance = distance;
      loops = 0;
    }
    //add if block to handle buzz or inet
    tone(buzz, 11500);
    if (!Ethernet.linkStatus() == LinkOFF) {
      if (loops==0) {
        webhook.post(discord_webhook, "application/json", "{\"content\":\"Help! The cat is eating me!\", \"tts\":false}");
        webhook.responseStatusCode(); //having these calls ensures that the request was processed before the tcp session is closed
        webhook.responseBody();
      }
    } else delay (500);
    //delay(500); //relying on http response delay, REENABLE after implementing NTP
    noTone(buzz);
    loops++;
    //Serial.println(loops);
  
  } else {
    avgDistance = avgDistance * .92 + distance * .08;
    if (loops > 0) loops--;
  }
}
