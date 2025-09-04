#include <Wire.h> 

#define ADXL345_ADDRESS 0x53
#define ADXL345_POWER_CTL 0x2D
#define ADXL345_DATA_FORMAT 0x31
#define ADXL345_DATAX0 0x32

void setup() {
  Serial.begin(115200);
  Wire.begin(); 

  Wire.beginTransmission(ADXL345_ADDRESS); 
  Wire.write(ADXL345_POWER_CTL); 
  Wire.write(0x08); 
  Wire.endTransmission(); 

  Wire.beginTransmission(ADXL345_ADDRESS); 
  Wire.write(ADXL345_DATA_FORMAT);
  Wire.write(0x0B); 
  Wire.endTransmission(); 
}

void loop() {
  Wire.beginTransmission(ADXL345_ADDRESS);
  Wire.write(ADXL345_DATAX0);
  Wire.endTransmission(); 

  Wire.requestFrom(ADXL345_ADDRESS ,6); 
  if (Wire.available() >= 6)  {
    int16_t x = (Wire.read() | (Wire.read() << 8));
    int16_t y = (Wire.read() | (Wire.read() << 8));
    int16_t z = (Wire.read() | (Wire.read() << 8));

    float lsbX = x; 
    float lsbY = y; 
    float lsbZ = z; 

    float gX = lsbX * 0.0039;
    float gY = lsbY * 0.0039; 
    float gZ = lsbZ * 0.0039;  

    Serial.print("Aclerómetro: \nlsbX: ");
    Serial.print(lsbX);
    Serial.print("\nlsbY: ");
    Serial.print(lsbY);
    Serial.print("\nlsbZ: ");
    Serial.print(lsbZ);
    Serial.print("\nEn g:");
    Serial.print("\nGX: ");
    Serial.print(gX);
    Serial.print("\nGY: ");
    Serial.print(gY);
    Serial.print("\nGZ: "); 
    Serial.print(gZ);
  }

  delay(100);

}
