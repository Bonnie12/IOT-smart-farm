#include <dht.h>

#include <LowPower.h>

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

LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
}





