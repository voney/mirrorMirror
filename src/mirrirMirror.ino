#define FASTLED_ESP8266_NODEMCU

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiUDP.h>
#include <FastPatterns.h>
#include <EasyTransfer.h>

#define LED_PIN 1
#define CLOCK_PIN 2
#define STRIP_TYPE WS2812
#define STRIP_COLOUR RGB
#define STRIP_LENGTH 20
#define LED_UPDATE_FREQ 1000 										//How often to call the LED Update interrupt in Microseconds
#define UDP_UPDATE_FREQ 30000										//How often to send a UDP pattern update in Milliseconds
#define SLAVE_FUDGE 1												//Fudge factor to slow down the slave to match the master in Milliseconds

const char* SSID = "MirrorMirror";
const char* PSK = "OnTheWall";
unsigned int localPort = 32165;

bool master = false;

int retries = 15; 													// x500ms to wait to connect to the wifi Master
int selectedPattern = 0;

unsigned long currentMillis;
unsigned long lastMilliUpdate;
unsigned long lastUdpUpdate;

struct SyncPacket {
	uint8_t source;
	uint8_t activePattern;
	uint8_t userSpeed;
	uint16_t index;
	unsigned long lastUpdate;
	unsigned long currentMillis;
	bool backwards;
	CRGB colourA;
	CRGB colourB;
};

ESP8266WebServer httpServer(80);

FastPatterns ledStrip(STRIP_LENGTH);

EasyTransfer transfer;
SyncPacket statePacket;

WiFiUDP myUDP;
IPAddress ipBroadcast (192,168,4,255);
byte packetBuffer[14]; 												//buffer to hold incoming packet

void setup(void){
	delay(1000);
	Serial.begin(115200);
	Serial.println();
	Serial.print("MirrorMirror starting...");
	initWiFi();
	initLEDs();
	lastUdpUpdate = millis();
	currentMillis = millis();
}

void loop(void){	
	currentMillis = timeSync();
	if (master){
		if ((currentMillis - lastUdpUpdate)>UDP_UPDATE_FREQ){
			sendUdpUpdate(1);
		}
		httpServer.handleClient();
	} else {
		delay(SLAVE_FUDGE);
	}

	handleUdpInput();
}

void initLEDs(){
	noInterrupts();
	timer1_isr_init(); 												// initialise timer1
	timer1_attachInterrupt(updateLeds); 							// choose which function to call when the interrupt fires.
	timer1_enable(TIM_DIV16, TIM_EDGE, TIM_LOOP); 					// Black magic
	timer1_write(clockCyclesPerMicrosecond() * LED_UPDATE_FREQ);	// Microseconds!! Holy sheeeeet! When I first did this I read it as millis and couldn't understand why the led was just "ON" lol.
	interrupts();
	/*if (master){
		ledStrip.backwards = false;
	} else {
		ledStrip.backwards = true;
	}*/
	//FastLED.addLeds<STRIP_TYPE, LED_PIN, CLOCK_PIN, STRIP_COLOUR>(ledStrip.ledArray, STRIP_LENGTH).setCorrection( TypicalLEDStrip );			// Use this one for 4 wire strips
	FastLED.addLeds<STRIP_TYPE, LED_PIN, STRIP_COLOUR>(ledStrip.ledArray, STRIP_LENGTH).setCorrection( TypicalLEDStrip );						// Use this one for 3 wire strips
	ledStrip.SwitchPattern(0); 										// Initialise the Rainbow Cycle pattern.
}

void initWiFi(){
	WiFi.begin(SSID, PSK);
	
	Serial.print("Attempting to connect to master node.");
	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial.print(".");
		if (retries == 0){
			Serial.println("");
			Serial.println("Unable to connect to master node, taking over as master.");
			master = true;
			WiFi.mode(WIFI_STA);
			WiFi.disconnect();
			delay(100);
			break;
		} else {
			retries--;
		}
	}
	if (master){
		WiFi.softAP(SSID, PSK);
		Serial.print("master IP address: ");
		Serial.println(WiFi.softAPIP());
		httpServer.on("/", HTTP_GET, []() {
			handleRoot();
		});
		httpServer.on("/setpattern", HTTP_POST, []() {
			String value = httpServer.arg("p");
			ledStrip.SwitchPattern(value.toInt());
			sendUdpUpdate(2);
			handleRoot();
		});
		httpServer.on("/setspeed", HTTP_GET, []() {
			String value = httpServer.arg("s");
			switch (value.toInt()){
				case 0:
					if (ledStrip.userSpeed > 1){
						--ledStrip.userSpeed;
					}
					break;
				case 1:
					if (ledStrip.userSpeed < 10){
						++ledStrip.userSpeed;
					}
					break;
				default:
					break;
			} 
			sendUdpUpdate(2);
			handleRoot();
		});
		httpServer.begin();
	} else {
		Serial.println("");
		Serial.println("Connected to master node");  
	}
	Serial.println();
	myUDP.begin(localPort); 										// Begin the UDP service on localPort
	transfer.begin(details(statePacket), &myUDP); 					// Initialise the EasyTransfer object and pass it the UDP object.
}

void updateLeds(){
	currentMillis = timeSync();
	ledStrip.Update();
} 

unsigned long timeSync(){
	if (master) {
		return millis();
	} else {
		unsigned long tempMillis;
		tempMillis = currentMillis + (millis() - lastMilliUpdate);
		lastMilliUpdate = millis();
		return tempMillis;
	}
}

void handleUdpInput(){
	int numBytes = myUDP.parsePacket();
	if (transfer.receiveData()) {
		if (numBytes = 20){
			currentMillis = statePacket.currentMillis;
			Serial.print(millis() / 1000);
			Serial.print(":Packet of ");
			Serial.print(numBytes);
			Serial.print(" received from ");
			Serial.print(myUDP.remoteIP());
			Serial.print(":");
			Serial.println(myUDP.remotePort());
			if (ledStrip.activePattern != statePacket.activePattern){
				ledStrip.SwitchPattern(statePacket.activePattern);
			}
			ledStrip.userSpeed = statePacket.userSpeed;
			ledStrip.colourA = statePacket.colourA;
			ledStrip.colourB = statePacket.colourB;
			if (ledStrip.backwards == statePacket.backwards) {
				if (ledStrip.currentStep != statePacket.index){
					Serial.println("Step mismatch with master.");
					Serial.print("My step: ");
					Serial.println(ledStrip.currentStep);
					Serial.print("Masters step: ");
					Serial.println(statePacket.index);
					Serial.print("Setting my step to: ");
					Serial.println(statePacket.index);
					ledStrip.currentStep = statePacket.index;
					ledStrip.lastUpdate = (currentMillis-(statePacket.currentMillis - statePacket.lastUpdate));
				}

			} else {
				if (ledStrip.currentStep != (ledStrip.totalSteps-(statePacket.index))){
					Serial.println("Step mismatch with master.");
					Serial.print("My step: ");
					Serial.println(ledStrip.currentStep);
					Serial.print("Masters step: ");
					Serial.println(statePacket.index);
					Serial.print("Setting my step to: ");
					Serial.println(ledStrip.totalSteps-(statePacket.index));
					ledStrip.currentStep = (ledStrip.totalSteps-(statePacket.index));
					ledStrip.lastUpdate = (currentMillis-(statePacket.currentMillis - statePacket.lastUpdate));
				}
			}
		}
		
	}
}

void sendUdpUpdate(int source){
	lastUdpUpdate = millis();
	statePacket.source = source;
	statePacket.activePattern = ledStrip.activePattern;
	statePacket.userSpeed = ledStrip.userSpeed;
	statePacket.index = ledStrip.currentStep;
	statePacket.lastUpdate = ledStrip.lastUpdate;
	statePacket.currentMillis = millis();
	statePacket.backwards = ledStrip.backwards;
	statePacket.colourA = ledStrip.colourA;
	statePacket.colourB = ledStrip.colourB;
	myUDP.beginPacket(ipBroadcast,localPort);
	transfer.sendData();
	myUDP.endPacket();
}

void handleRoot(){	
	//build the list of patterns in HTML
	String tempPattern;
	Serial.println(ledStrip.activePattern);
	for (int i=0; i < ledStrip.totalPatterns; i++){
		tempPattern += "<option value='";
		tempPattern += i;
		if (i==ledStrip.activePattern){
			tempPattern += "' selected>";
		} else {
			tempPattern += "'>";
		}
		tempPattern += ledStrip.patternList[i];
		tempPattern += "</option>";
	}
	
	char tempHTML[1000];
	snprintf(tempHTML, 1000,
		"<html><head><title>MirrorMirror Control</title><style>body {font-family: Sans-serif; text-align-last:center; text-align:center;}select,input {margin: auto; width: 100%; font-size: 16px; border: 1px solid #4F518C; height: 34px;}a.button {appearance: button;}</style></head><body><form action='/setpattern' method='post' enctype='application/x-www-form-urlencoded'><h4>Select Pattern</h4><select name='p'>%s</select></br></br><input type='submit' value='Submit' /></form><h4>Speed</h4><input TYPE='button' VALUE='Down' onclick=\"window.location.href='/setspeed?s=0'\"></br></br><input TYPE='button' VALUE='Up' onclick=\"window.location.href='/setspeed?s=1'\"></body></html>",
		tempPattern.c_str());
	httpServer.send(200, "text/html", tempHTML);
}