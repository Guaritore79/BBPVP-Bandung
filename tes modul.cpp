#include <Wire.h>
#include <Arduino.h>

#define I2c_SDA 17
#define I2c_SCL 18

void setup() {
  Serial.begin(115200);
  Wire.begin(I2c_SDA, I2c_SCL);
  while (!Serial); 
  Serial.println("\nMemulai I2C Scanner...");
}

void loop() {
  byte error, address;
  int nDevices = 0;

  Serial.println("Mencari perangkat I2C...");

  for (address = 1; address < 127; address++ ) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("Perangkat I2C ditemukan di alamat 0x");
      if (address < 16)
        Serial.print("0");
      Serial.print(address, HEX);
      Serial.println("  !");
      nDevices++;
    }
    else if (error == 4) {
      Serial.print("Terjadi error yang tidak diketahui di alamat 0x");
      if (address < 16)
        Serial.print("0");
      Serial.println(address, HEX);
    }
  }
  
  if (nDevices == 0)
    Serial.println("Tidak ada perangkat I2C yang ditemukan\n");
  else
    Serial.println("Pencarian selesai\n");

  delay(5000); // Tunggu 5 detik sebelum mencari lagi
}