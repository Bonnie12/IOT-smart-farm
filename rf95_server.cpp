// rf95_server.cpp
//
// Example program showing how to use RH_RF95 on Raspberry Pi
// Uses the bcm2835 library to access the GPIO pins to drive the RFM95 module
// Requires bcm2835 library to be already installed
// http://www.airspayce.com/mikem/bcm2835/
// Use the Makefile in this directory:
// cd example/raspi/rf95
// make
// sudo ./rf95_server
//
// Contributed by Charles-Henri Hallard based on sample RH_NRF24 by Mike Poublon
// Edited by: Ramin Sangesari
// https://www.hackster.io/idreams/

#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

#include <bcm2835.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>

#include <RH_RF69.h>
#include <RH_RF95.h>

#define THINGSPEAK_OFFLINE_ERROR     0
#define THINGSPEAK_CONNECTION_ERROR  1
#define OPEN_SOCKET_ERROR            2
#define PARAMS_ERROR                 3
#define SEND_OK                      4
#define URL_THINGSPEAK             "api.thingspeak.com"
#define PORT_THINGSPEAK            80
#define BEGIN_OF_HTTP_REQ          "GET /update?key="
#define END_OF_HTTP_REQ            "\r\n\r\n"
#define MAX_SIZE                   9999

// define hardware used change to fit your need
// Uncomment the board you have, if not listed 
// uncommment custom board and set wiring tin custom section


// LoRasPi board 
// see https://github.com/hallard/LoRasPI
//#define BOARD_LORASPI

// Adafruit RFM95W LoRa Radio Transceiver Breakout
// see https://www.adafruit.com/product/3072
#define BOARD_ADAFRUIT_RFM95W

// iC880A and LinkLab Lora Gateway Shield (if RF module plugged into)
// see https://github.com/ch2i/iC880A-Raspberry-PI
//#define BOARD_IC880A_PLATE

// Raspberri PI Lora Gateway for multiple modules 
// see https://github.com/hallard/RPI-Lora-Gateway
//#define BOARD_PI_LORA_GATEWAY

// Dragino Raspberry PI hat
// see https://github.com/dragino/Lora
//#define BOARD_DRAGINO_PIHAT

// Now we include RasPi_Boards.h so this will expose defined 
// constants with CS/IRQ/RESET/on board LED pins definition
#include "../RasPiBoards.h"

// Our RFM95 Configuration 
#define RF_FREQUENCY  915.00
#define RF_NODE_ID    0


// Create an instance of a driver
RH_RF95 rf95(RF_CS_PIN, RF_IRQ_PIN);
//RH_RF95 rf95(RF_CS_PIN);

//Flag for Ctrl-C
volatile sig_atomic_t force_exit = false;

char SendDataToThingSpeak(int FieldNo, float* FieldArray, char * Key, int SizeOfKey)
{
	int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *ServerTCP;
	int ReqStringSize;
	int i;
	char ReqString[MAX_SIZE];
	char BeginOfHTTPReq[]=BEGIN_OF_HTTP_REQ;
	char EndOfHTTPReq[]=END_OF_HTTP_REQ;
	char *ptReqString;
	
	if (FieldNo <=0)
		return PARAMS_ERROR;
	
	//Setting up HTTP Req. string:
	bzero(&ReqString,sizeof(ReqString));
	sprintf(ReqString,"%s%s",BeginOfHTTPReq,Key);
	
	ptReqString = &ReqString[0]+(int)strlen(ReqString);
	for(i=1; i<= FieldNo; i++)
	{
		sprintf(ptReqString,"&field%d=%.2f",i,FieldArray[i-1]);
		ptReqString = &ReqString[0]+(int)strlen(ReqString);
	}
	
	sprintf(ptReqString,"%s",EndOfHTTPReq);
	printf("%s",EndOfHTTPReq);
	//Connecting to ThingSpeak and sending data:
	portno = PORT_THINGSPEAK;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    
	//Step 1: opening a socket
	if (sockfd < 0)
		return OPEN_SOCKET_ERROR;
	
	//Step 2: check if ThingSpeak is online
	ServerTCP = gethostbyname(URL_THINGSPEAK);
	if (ServerTCP == NULL) 
	    return THINGSPEAK_OFFLINE_ERROR;
    
	//Step 3: setting up TCP/IP socket structure
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)ServerTCP->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         ServerTCP->h_length);
    serv_addr.sin_port = htons(portno);
    
	//Step 4: connecting to ThingSpeak server (via HTTP port / port no. 80)
	if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
		return THINGSPEAK_CONNECTION_ERROR;
	
	//Step 5: sending data to ThingSpeak's channel
    write(sockfd,ReqString,strlen(ReqString));
		
	//Step 6: close TCP connection
    close(sockfd);    
	
	//All done!
	return SEND_OK;
}

void sig_handler(int sig)
{
  printf("\n%s Break received, exiting!\n", __BASEFILE__);
  force_exit=true;
}

//Main Function
int main (int argc, const char* argv[] )
{
  char data [255];
  int no_field = 3;
  char key[] = "OT6QKYJM8U4M0B3L";
  int sizeofkey = 16;
  //float uploaddata[] ;{fsoilmoist,flight,ftemp}
  char soilmoist[10];
  char light[10];
  char temp[10];
  float fsoilmoist;
  float flight;
  float ftemp;
  
  unsigned long led_blink = 0;
  
  signal(SIGINT, sig_handler);
  //printf( "%s\n", __BASEFILE__);

  if (!bcm2835_init()) {
    fprintf( stderr, "%s bcm2835_init() Failed\n\n", __BASEFILE__ );
    return 1;
  }
  
  //printf( "RF95 CS=GPIO%d", RF_CS_PIN);

#ifdef RF_LED_PIN
  pinMode(RF_LED_PIN, OUTPUT);
  digitalWrite(RF_LED_PIN, HIGH );
#endif

#ifdef RF_IRQ_PIN
  //printf( ", IRQ=GPIO%d", RF_IRQ_PIN );
  // IRQ Pin input/pull down
  pinMode(RF_IRQ_PIN, INPUT);
  bcm2835_gpio_set_pud(RF_IRQ_PIN, BCM2835_GPIO_PUD_DOWN);
  // Now we can enable Rising edge detection
  bcm2835_gpio_ren(RF_IRQ_PIN);
#endif
  
#ifdef RF_RST_PIN
  //printf( ", RST=GPIO%d", RF_RST_PIN );
  // Pulse a reset on module
  pinMode(RF_RST_PIN, OUTPUT);
  digitalWrite(RF_RST_PIN, LOW );
  bcm2835_delay(150);
  digitalWrite(RF_RST_PIN, HIGH );
  bcm2835_delay(100);
#endif

#ifdef RF_LED_PIN
  //printf( ", LED=GPIO%d", RF_LED_PIN );
  digitalWrite(RF_LED_PIN, LOW );
#endif

  if (!rf95.init()) {
    fprintf( stderr, "\nRF95 module init failed, Please verify wiring/module\n" );
  } else {
    // Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on

    // The default transmitter power is 13dBm, using PA_BOOST.
    // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then 
    // you can set transmitter powers from 5 to 23 dBm:
    //  driver.setTxPower(23, false);
    // If you are using Modtronix inAir4 or inAir9,or any other module which uses the
    // transmitter RFO pins and not the PA_BOOST pins
    // then you can configure the power transmitter power for -1 to 14 dBm and with useRFO true. 
    // Failure to do that will result in extremely low transmit powers.
    // rf95.setTxPower(14, true);


    // RF95 Modules don't have RFO pin connected, so just use PA_BOOST
    // check your country max power useable, in EU it's +14dB
    rf95.setTxPower(14, false);

    // You can optionally require this module to wait until Channel Activity
    // Detection shows no activity on the channel before transmitting by setting
    // the CAD timeout to non-zero:
    //rf95.setCADTimeout(10000);

    // Adjust Frequency
    rf95.setFrequency(RF_FREQUENCY);
    
    // If we need to send something
    rf95.setThisAddress(RF_NODE_ID);
    rf95.setHeaderFrom(RF_NODE_ID);
    
    // Be sure to grab all node packet 
    // we're sniffing to display, it's a demo
    rf95.setPromiscuous(true);

    // We're ready to listen for incoming message
    rf95.setModeRx();

    //printf( " OK NodeID=%d @ %3.2fMHz\n", RF_NODE_ID, RF_FREQUENCY );
    //printf( "Listening packet...\n" );

    //Begin the main body of code
    while (!force_exit) {
      
#ifdef RF_IRQ_PIN
      // We have a IRQ pin ,pool it instead reading
      // Modules IRQ registers from SPI in each loop
      
      // Rising edge fired ?
      if (bcm2835_gpio_eds(RF_IRQ_PIN)) {
        // Now clear the eds flag by setting it to 1
        bcm2835_gpio_set_eds(RF_IRQ_PIN);
        //printf("Packet Received, Rising event detect for pin GPIO%d\n", RF_IRQ_PIN);
#endif

        if (rf95.available()) { 
#ifdef RF_LED_PIN
          led_blink = millis();
          digitalWrite(RF_LED_PIN, HIGH);
#endif
          // Should be a message for us now
          uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
          uint8_t len  = sizeof(buf);
          uint8_t from = rf95.headerFrom();
          uint8_t to   = rf95.headerTo();
          uint8_t id   = rf95.headerId();
          uint8_t flags= rf95.headerFlags();;
          int8_t rssi  = rf95.lastRssi();
          
          if (rf95.recv(buf, &len)) {
            printf("Packet[%02d] #%d => #%d %ddB: ", len, from, to, rssi);
            printbuffer(buf, len);
            
            //strcpy(data,buf);
            for (int i=0;i<len;i++){
            data[i] = buf[i];
            printf("%c",data[i]);    
            }
            sscanf(data,"%f %f %f", &fsoilmoist, &ftemp, &flight);
            printf("\n%f and %f or %f", flight, ftemp, fsoilmoist);
            
            float uploaddata[] = {fsoilmoist,ftemp,flight} ;
            
            SendDataToThingSpeak(no_field, uploaddata,key,sizeofkey);
            //delay(10000);
            printf("\ndone");
            


          } else {
            Serial.print("receive failed");
          }
          printf("\n");
        }
        
#ifdef RF_IRQ_PIN
      }
#endif
      
#ifdef RF_LED_PIN
      // Led blink timer expiration ?
      if (led_blink && millis()-led_blink>200) {
        led_blink = 0;
        digitalWrite(RF_LED_PIN, LOW);
      }
#endif
      // Let OS doing other tasks
      // For timed critical appliation you can reduce or delete
      // this delay, but this will charge CPU usage, take care and monitor
      bcm2835_delay(5);
    }
  }

#ifdef RF_LED_PIN
  digitalWrite(RF_LED_PIN, LOW );
#endif
  printf( "\n%s Ending\n", __BASEFILE__ );
  bcm2835_close();
  return 0;
}

