// Present a "Will be back soon web page", as stand-in webserver.
// 2011-01-30 <jc@wippler.nl> http://opensource.org/licenses/mit-license.php

#include <EtherCard.h>
#include <DS1307RTC.h>
#include <EEPROM.h>



static byte ntpServer[] = {194,109,6,2}; //ntp.xs4all.nl but you can cange in any public ntp server.
uint8_t ntpMyPort = 123;
long timeZoneOffset = 3600L; //Winter (original) time Europe
boolean networkConnection = true;


static byte mymac[] = { 0x74,0x69,0x69,0x2D,0x30,0x31 };
static byte myip[] = { 192,168,1,181 };

#define BUFFER_SIZE 500
byte Ethernet::buffer[BUFFER_SIZE];
BufferFiller bfill;

#define REL 5  // define REL pin
bool RELStatus1 = false;
bool RELStatus2 = false;
float temp1, temp2;
char temp_str1[10];
char temp_str2[10];
char cas[19];

#define ONE_WIRE_BUS 2
//OneWire oneWire(ONE_WIRE_BUS);
//DallasTemperature sensors(&oneWire);

const char http_OK[] PROGMEM =
    "HTTP/1.0 200 OK\r\n"
    "Content-Type: text/html\r\n"
    "Pragma: no-cache\r\n\r\n";

const char http_Found[] PROGMEM =
    "HTTP/1.0 302 Found\r\n"
    "Location: /\r\n\r\n";

const char http_Unauthorized[] PROGMEM =
    "HTTP/1.0 401 Unauthorized\r\n"
    "Content-Type: text/html\r\n\r\n"
    "<h1>401 Unauthorized</h1>";



void homePage()
{
    bfill.emit_p(PSTR("$F"
    		"<meta charset=\"UTF-8\">"
    "<meta http-equiv='refresh' content='10'/>"
      "<title>Temp Server</title>"
      "<body bgcolor=""#99CCFF"">"
      "<center>"
      "<br />"
      "<font size=""5"">"
      "Dátum: <b>"
      "$S"
      "<br />"
      "<br />"
      "<font size=""7"">"
      "Temperature 1: <b>"
      "$S &deg;C"
      "<br />"
      "<br />"
      "Temperature 2: <b>"
      "$S &deg;C"
      "<br />"
      "<br />"
      "</b>"
      "Relay1 Status: <a href=\"?REL=$F\">$F</a><b><br /> Relay1 Status2: <a href=\"?REE=$F\">$F</button><b>"), http_OK, cas, temp_str1, temp_str2, RELStatus1?PSTR("off"):PSTR("on"), RELStatus1?PSTR("ON"):PSTR("OFF"),  RELStatus2?PSTR("off"):PSTR("on"), RELStatus2?PSTR("ON"):PSTR("OFF"));

       //"<a href=\"/?buttonmotorforward\"\"><button style='font-size:170%;background-color:darkgray; color:green; border-radius:50px; position:absolute; top:430px; left:166px;'>Forward</button></a>"
}




void setup(){

	Serial.begin(9600);
	pinMode(REL, OUTPUT);
	if (ether.begin(BUFFER_SIZE, mymac) == 0)
		Serial.println("Cannot initialise ethernet.");
	else
		Serial.println("Ethernet initialised.");
	ether.staticSetup(myip);

	temp1 = 25;
	temp2 = 13;



	pinMode(4, OUTPUT);
	pinMode(53, OUTPUT);

	printRTC_time();


}




void showServerTime()
{

	 tmElements_t tm;

	if (RTC.read(tm))
	{
		sprintf(cas, "%02d:%02d:%02d %02d-%02d-%d",tm.Hour, tm.Minute, tm.Second, tm.Day, tm.Month, tmYearToCalendar(tm.Year) );

	} else
	{
		if (RTC.chipPresent())
		{
			Serial.println("The DS1307 is stopped.  Please run the SetTime");
			Serial.println("example to initialize the time and begin running.");
			Serial.println();
		} else
		{
			Serial.println("DS1307 read error!  Please check the circuitry.");
			Serial.println();
		}
		delay(9000);
	}

}

void loop(){


	delay(1);   // necessary for my system
	word len = ether.packetReceive();   // check for ethernet packet
	word pos = ether.packetLoop(len);   // check for tcp packet



	showServerTime();

	dtostrf(temp1, 6, 2, temp_str1);
	dtostrf(temp2, 6, 2, temp_str2);

	if (pos)
	{
		bfill = ether.tcpOffset();
		char *data = (char *) Ethernet::buffer + pos;

		if (strncmp("GET /", data, 5) != 0)
		{
			// Unsupported HTTP request
			// 304 or 501 response would be more appropriate
			bfill.emit_p(http_Unauthorized);
		} else
		{
			data += 5;

			if (data[0] == ' ')
			{
				// Return home page
				homePage();
			} else if (strncmp("?REL=on ", data, 8) == 0)
			{
				// Set RELStatus true and redirect to home page
				RELStatus1 = true;
				temp1 = 11.25;
				bfill.emit_p(http_Found);

			} else if (strncmp("?REL=off ", data, 9) == 0)
			{
				// Set RELStatus false and redirect to home page
				RELStatus1 = false;
				temp1 = -11.5;
				bfill.emit_p(http_Found);
			}

			else if (strncmp("?REE=on ", data, 8) == 0)
			{
				// Set RELStatus true and redirect to home page
				RELStatus2 = true;
				temp2 = 16.25;
				bfill.emit_p(http_Found);

			} else if (strncmp("?REE=off ", data, 9) == 0)
			{
				// Set RELStatus false and redirect to home page
				RELStatus2 = false;
				temp2 = -111.5;
				bfill.emit_p(http_Found);
			}

			else
			{
				// Page not found
				bfill.emit_p(http_Unauthorized);
			}
		}
		ether.httpServerReply(bfill.position());    // send http response
	}
}


void set_ntp_time()
{
	long currenttime = getNtpTime(); //returns a serial number version of time in seconds since a specified date
	setTime(currenttime); //updates my RTC chip using this serial number.

	RTC.set(currenttime);

	delay(2000);


}

void printRTC_time()
{
	 tmElements_t tm;

	if (RTC.read(tm))
	{
		Serial.print("Ok, Time = ");
		print2digits(tm.Hour);
		Serial.write(':');
		print2digits(tm.Minute);
		Serial.write(':');
		print2digits(tm.Second);

		Serial.print(", Date (D/M/Y) = ");
		Serial.print(tm.Day);
		Serial.write('/');
		Serial.print(tm.Month);
		Serial.write('/');
		Serial.print(tmYearToCalendar(tm.Year));
		Serial.println();
	} else
	{
		if (RTC.chipPresent())
		{
			Serial.println("The DS1307 is stopped.  Please run the SetTime");
			Serial.println("example to initialize the time and begin running.");
			Serial.println();
		} else
		{
			Serial.println("DS1307 read error!  Please check the circuitry.");
			Serial.println();
		}
		delay(9000);
	}

}






void print2digits(int number) {
  if (number >= 0 && number < 10) {
    Serial.write('0');
  }
  Serial.print(number);
}


  unsigned long getNtpTime() {
    uint32_t timeFromNTP;
    const unsigned long seventy_years = 2208988800UL;
    int i = 60; //Number of attempts
    Serial.println("NTP request sent");
    while(i > 0) {
    ether.ntpRequest(ntpServer, ntpMyPort);
        Serial.print("."); //Each dot is a NTP request
        word length = ether.packetReceive();
        ether.packetLoop(length);
        if(length > 0 && ether.ntpProcessAnswer(&timeFromNTP,ntpMyPort)) {
          Serial.println();
          Serial.println("NTP reply received");
          return timeFromNTP - seventy_years + timeZoneOffset;
        }
        delay(500);
        i--;
      }
    Serial.println();
    Serial.println("NTP reply failed");
    return 0;
  }

  void digitalClockDisplay(unsigned long  epoch){
	  // print the hour, minute and second:
	      Serial.print("The UTC time is ");       // UTC is the time at Greenwich Meridian (GMT)
	      Serial.print((epoch  % 86400L) / 3600); // print the hour (86400 equals secs per day)
	      Serial.print(':');
	      if ( ((epoch % 3600) / 60) < 10 ) {
	        // In the first 10 minutes of each hour, we'll want a leading '0'
	        Serial.print('0');
	      }
	      Serial.print((epoch  % 3600) / 60); // print the minute (3600 equals secs per minute)
	      Serial.print(':');
	      if ( (epoch % 60) < 10 ) {
	        // In the first 10 seconds of each minute, we'll want a leading '0'
	        Serial.print('0');
	      }
	      Serial.println(epoch % 60); // print the second
	    }
	    // wait ten seconds before asking for the time again



