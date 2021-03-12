

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

const byte interruptPin = D7;

void ICACHE_RAM_ATTR handleInterrupt();

char ssid[] = "Sgt. Sam";        //  your network SSID (name)
char pass[] = "koopjeeigenwifi";  // your network password

int feedturns = 7; //amount of motor turns to feed cats
int totalcounts = feedturns * 3400;

int feedPin = D6;
int feedDir = D0;
int counts = 0;

int feedhour1 = 6;
int feedminute1 = 15;
int feedhour2 = 9;
int feedminute2 = 15;
int feedhour3 = 12;
int feedminute3 = 15;
int feedhour4 = 15;
int feedminute4 = 15;
int feedhour5 = 18;
int feedminute5 = 15;
int timesfed = 0;
int feedhour6 = 21;
int feedminute6 = 15;

unsigned int localPort = 2390;      // local port to listen for UDP packets

IPAddress timeServerIP; // time.nist.gov NTP server address
const char* ntpServerName = "time.nist.gov";

const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message

byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets

WiFiUDP udp;

int Hour = 00;
int Minute = 00;

void setup() {
  pinMode(interruptPin, INPUT_PULLUP);
  pinMode(feedPin, OUTPUT);
  pinMode(feedDir, OUTPUT);
  digitalWrite(feedPin, LOW);
  digitalWrite(feedDir, HIGH);

  // put your setup code here, to run once:
  Serial.begin(115200); //Start serial link to debug
  Serial.println();
  Serial.println();

  Serial.print("Connecting to "); //show connecting to WiFi
  Serial.println(ssid);
  WiFi.begin(ssid, pass);

  // wait untill connection is established:
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");

  Serial.println("WiFi connected"); //Shiow connected network
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  Serial.println("Starting UDP");
  udp.begin(localPort);
  Serial.print("Local port: ");
  Serial.println(udp.localPort());

  attachInterrupt(digitalPinToInterrupt(interruptPin), handleInterrupt, RISING);
}

void ICACHE_RAM_ATTR handleInterrupt() {
  counts++;
}

void loop() {
  // put your main code here, to run repeatedly:
  // Search for time server:
  WiFi.hostByName(ntpServerName, timeServerIP);

  sendNTPpacket(timeServerIP); // send an NTP packet to a time server
  // wait to see if a reply is available
  delay(1000);

  int cb = udp.parsePacket();
  if (!cb) {
    Serial.println("no packet yet");
  }
  else {
    Serial.print("packet received, length=");
    Serial.println(cb);
    // We've received a packet, read the data from it
    udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer

    //the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, esxtract the two words:

    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    unsigned long secsSince1900 = highWord << 16 | lowWord;
    // Serial.print("Seconds since Jan 1 1900 = " );
    // Serial.println(secsSince1900);

    // now convert NTP time into everyday time:
    // Serial.print("Unix time = ");
    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    const unsigned long seventyYears = 2208988800UL;
    // subtract seventy years:
    unsigned long epoch = secsSince1900 - seventyYears;
    // print Unix time:
    // Serial.println(epoch);


    // print the hour, minute and second:
    Serial.print("The UTC time is ");       // UTC is the time at Greenwich Meridian (GMT)
    Serial.print(((epoch  % 86400L) / 3600) + 1); // print the hour (86400 equals secs per day)
    Hour = (((epoch  % 86400L) / 3600) + 1);
    Serial.print(':');
    Serial.print((epoch  % 3600) / 60); // print the minute (3600 equals secs per minute)
    Minute = ((epoch  % 3600) / 60);
    Serial.print(':');
    Serial.println(epoch % 60); // print the second
  }
  // wait 40 seconds before asking for the time again
  delay(40000);

  if ((Hour == feedhour1 && Minute == feedminute1) || (Hour == feedhour2 && Minute == feedminute2) || (Hour == feedhour3 && Minute == feedminute3) || (Hour == feedhour4 && Minute == feedminute4) || (Hour == feedhour5 && Minute == feedminute5) || (Hour == feedhour6 && Minute == feedminute6)) {

    counts = 0;
    delay(10);
    Serial.println("Feeding!");

    while (counts < totalcounts) {
      digitalWrite(feedPin, HIGH);
      delay(10);
    }

    Serial.println("Done!");
    digitalWrite(feedPin, LOW);
    delay(100000);
    counts = 0;


  }


}

unsigned long sendNTPpacket(IPAddress& address)
{
  Serial.println("sending NTP packet...");
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
  udp.beginPacket(address, 123); //NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}
