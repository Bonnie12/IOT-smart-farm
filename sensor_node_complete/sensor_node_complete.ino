#include <dht.h>
#include <BH1750.h>
#include <SPI.h>
#include <LoRa.h>
#include <i2cdetect.h> 

#include <Wire.h>

char node_id[4] = {1,1,1,1};
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
 LoRa.setTxPower(23);
//LoRa.setSpreadingFactor(12);
//LoRa.setCodingRate4(8);
//LoRa.setSignalBandwidth(4);

}

void loop() {
  // put your main code here, to run repeatedly:
moistureoutput = analogRead(A1); // read moisture level
moistureoutput = map(moistureoutput,760,250,0,100); // map the moisture level 
DHT.read22(A2); // read temperature
uint16_t lux = lightMeter.readLightLevel(true); // read light intensity

Serial.print("Sending packet: ");


Serial.print("Moisture level; ");
Serial.print(moistureoutput);
Serial.print("%   ");
Serial.print("Temperature: ");
Serial.print(DHT.temperature);
Serial.print("   ");
Serial.print("Light: ");
Serial.print(lux);
Serial.println(" lx   ");
//Serial.println(counter);

//for (int i=0;i<10;i++){
LoRa.beginPacket(); // sending data
LoRa.print("    ");

LoRa.print(moistureoutput);
LoRa.print(" ");
LoRa.print(DHT.temperature);
LoRa.print(" ");
LoRa.print(lux);
LoRa.endPacket(); // end send data

delay(10000);
}



