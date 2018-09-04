#include <dht.h>
#include <BH1750.h>
#include <SPI.h>
#include <LoRa.h>
#include <i2cdetect.h> 

#include <Wire.h>


int counter = 0;
float moistureoutput;
dht DHT;
BH1750 lightMeter;

void setup() {
  // put your setup code here, to run once:
 //SETUP WATCHDOG TIMER
WDTCSR = (24);//change enable and WDE - also resets
WDTCSR = (33);//prescalers only - get rid of the WDE and WDCE bit
WDTCSR |= (1<<6);//enable interrupt mode

  //Disable ADC - don't forget to flip back after waking up if using ADC in your application ADCSRA |= (1 << 7);
  ADCSRA &= ~(1 << 7);
  
  //ENABLE SLEEP - this enables the sleep mode
  SMCR |= (1 << 2); //power down mode
  SMCR |= 1;//enable sleep
 Wire.begin();
 Serial.begin(9600);


  Serial.println("LoRa Sender");

  if (!LoRa.begin(915E6)) { //lora frequency
    Serial.println("Starting LoRa failed!");
    while (1);
  }
 Serial.println("i2cdetect example\n"); // i2c address detect
 Serial.print("Scanning address range 0x03-0x77\n\n");
 i2cdetect();
 lightMeter.begin();
 

}

void loop() {
  // put your main code here, to run repeatedly:
moistureoutput = analogRead(A0); // read moisture level
moistureoutput = map(moistureoutput,1023,460,0,100); // map the moisture level 
DHT.read11(A1); // read temperature
uint16_t lux = lightMeter.readLightLevel(true); // read light intensity

Serial.print("Sending packet: ");


Serial.print("Moisture level; ");
Serial.print(moistureoutput);
Serial.print("%   ");
Serial.print("Light: ");
Serial.print(lux);
Serial.print(" lx   ");
Serial.print("Temperature: ");
Serial.print(DHT.temperature);
Serial.print("   ");
Serial.println(counter);

for (int i=0;i<10;i++){
LoRa.beginPacket(); // sending data
LoRa.print("     Moisture; ");
LoRa.print(moistureoutput);
LoRa.print("% ");
LoRa.print("Light: ");
LoRa.print(lux);
LoRa.print("lx ");
LoRa.print("Temp: ");
LoRa.print(DHT.temperature);
LoRa.print("C ");
LoRa.print("2");
LoRa.endPacket(); // end send data
}


// one minute
for(int i=0;i<5;i++){
//BOD DISABLE 
  MCUCR |= (3 << 5); //set both BODS and BODSE at the same time
  MCUCR = (MCUCR & ~(1 << 5)) | (1 << 6); //then set the BODS bit and clear the BODSE bit at the same time
  __asm__  __volatile__("sleep");//in line assembler to go to sleep
  }

}

ISR(WDT_vect){
  
}// watchdog interrupt
