#include <EtherCard.h>

#define FIRST_LED_PIN 3
#define LED_COUNT 3
#define LAST_LED_PIN (FIRST_LED_PIN + LED_COUNT - 1)
#define STATUS_LED_PIN (FIRST_LED_PIN + LED_COUNT)
#define BUZZER_PIN (STATUS_LED_PIN + 1)
#define DHCP 1
#define DNS 1
#define ERROR -1

static byte mymac[] = {0x99,0x99,0x99,0x99,0x99,0x99};
// Used in case DHCP is disabled
static byte myip[] = {192,168,1,2};
static byte gwip[] = {192,168,1,1};

const char server_domain[] PROGMEM = "your.http.server";
// Used in case DNS is disabled
static byte server_addr[] = {0,0,0,0};
static char url[] = "a320303ad108ab78bb77c385cca8f20c/unread.php";

byte Ethernet::buffer[700];
static uint32_t timer;

int unread_count = 0, unread_count_old = ERROR;

static void callback (byte status, word off, word len)
{
	Ethernet::buffer[off + 300] = 0;
	String response = (const char*) Ethernet::buffer + off;
	
	// At this point, we should have a response in the form: "[<unread_mail_count>]"
	int i = response.indexOf('[');
	int j = response.indexOf(']');
	
	// Update status LED
	if(i >= 0 && j >= 0) {
		response = response.substring(i + 1, j);
		unread_count = response.toInt();
		digitalWrite(STATUS_LED_PIN, HIGH);
	} else {
		digitalWrite(STATUS_LED_PIN, LOW);
		return;
	}
	
	// Check whether the unread mail count is over the maximum displayable number
	if (unread_count >= (0x1 << LED_COUNT)) {
		for (int i = FIRST_LED_PIN; i <= LAST_LED_PIN; i++)
			digitalWrite(i, HIGH);
	} else {
		int pos = FIRST_LED_PIN;
		for (int j = unread_count; j > 0; j = j >> 1) {
			digitalWrite(pos, j & 0x1);
			pos++;
		}
	}
	
	// Play sound with the buzzer
	if (unread_count > unread_count_old && unread_count_old > ERROR)
	{
		for (int i = 0; i < 3; i++) {
			tone(BUZZER_PIN, 1500, 1000);
			delay(1500);
		}
	}
	unread_count_old = unread_count;
}

void setup()
{
	// Configure outputs
	for (int i = FIRST_LED_PIN; i < LAST_LED_PIN; i++) {
		pinMode(i, OUTPUT);
		digitalWrite(i, LOW);
	}
	pinMode(STATUS_LED_PIN, OUTPUT);
	digitalWrite(STATUS_LED_PIN, LOW);
	pinMode(BUZZER_PIN, OUTPUT);
	
	// Initialise serial communication
	Serial.begin(9600);
	Serial.println("[arduin00bnoob]");
	
	if (ether.begin(sizeof Ethernet::buffer, mymac) == 0) {
		Serial.println( "Failed to access Ethernet controller. Got Daaamn!");
		return;
	}
	
#if DHCP
	if (!ether.dhcpSetup()) {
		Serial.println("DHCP failed");
		ether.staticSetup(myip, gwip);
	}
#else
	ether.staticSetup(myip, gwip);
#endif
	
	ether.printIp("IP:	", ether.myip);
	ether.printIp("GW:	", ether.gwip);
	ether.printIp("DNS: ", ether.dnsip);
	
#if DNS
	// Resolve domain
	if (!ether.dnsLookup(server_domain)) {
		Serial.println("DNS failed");
	}
#else
	ether.copyIp(ether.hisip, server_addr);
#endif
	ether.printIp("SRV: ", ether.hisip);
}

void loop()
{
	ether.packetLoop(ether.packetReceive());
	
	if (millis() > timer) {
		timer = millis() + 10000;
		ether.browseUrl(PSTR("/"), url, server_domain, callback);
	}
}
