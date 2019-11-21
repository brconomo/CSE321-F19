# Plant Protector!
The motion sensing cat alarm!

Have you had houseplants die because your cat decided they were tasty treats? Is this causing a strained relationship between you and your pet? Then **Plant Protector** is for you!

# [Discord](https://discordapp.com/)
1. Make a new server, or go to the one you want the Plant Protector to message.
2. Invite your housemates to the server.
3. Go to your server settings, and create a new **webhook** for the Plant Protector to use.
4. Install Discord on your phone, or whatever device is best for Plant Protector to reach you on.

# Software Setup
1. Assuming you have the hardware, download the [Arduino IDE.](https://www.arduino.cc/en/main/software) 
2. Clone this repository and use the IDE to tweak the values in the **USER CONFIGURABLES** section at the top of the main file.  
    * **timezone** - your time zone, used to accurately compute local time from UTC.
    * **wl** - When you leave for work in the morning in 24 hour time.
    * **wr** - When you return from work in the evening in 24 hour time.
    * **discord_webhook** - A webhook URL the Plant Protector can use to message a discord server.
    * **timeServer[]** - An NTP server the Plant Protector can contact to get the current time.
    * **tol** - The tolerance before the sensor responds to a change. Low tol is good for short resting distances, while longer distances will require a bigger tol as the sensor loses accuracy over distance.
    * **mac[]** - Your Plant Protector's MAC address.
    * **ip** - A static IP if you aren't using DHCP (most home networks use DHCP)
    * **myDns** - A DNS override. (Most home networks will provide a DNS over DHCP)
    * **conn** - Set to false to enable a no-internet fallback mode.
3. Use the IDE to upload the configured sketch to the hardware via the USB cable.

# Hardware Setup
1. Plug it into an ethernet cable that is connected to the internet and can perform http and ntp queries. Most home networks in their default configuration will be fine.
2. Position the box looking at the plant so it's looking **away** from any spaces humans might be. It needs to detect the cat being on the table, not you walking past it to go to the bathroom!
3. Plug the USB cable and power adapter into the wall.
4. It should initialize after a few seconds. Test it by waving in front of it.
5. If it gets bumped during setup and starts alerting continuously, just leave it. It will recalibrate after a short while of constant alerts. If the behavior continues, you may need a larger tol.

# Build your own!
1. You need:
    * Arduino/Genuino
    * Arduino Ethernet Shield
    * HC-SR04 Ultrasonic Sensor or equivalent
    * Piezoelectric buzzer or other tone playing circuit E.G. Speaker connected to a simple transistor amp off of the 5v rail
2. Connect the Arduino and Ethernet shield. Digital Pins 10-13 are reserved for this part, don't use them if you add to the design.
3. Connect SR04 VCC to the 5v rail, GND to GND, Trig to Digital Pin 2, and Echo to Digital Pin 3.
4. Connect your tone playing circuit to Digital Pin 7 and GND (and VCC too if needed E.G. transistor amp)
