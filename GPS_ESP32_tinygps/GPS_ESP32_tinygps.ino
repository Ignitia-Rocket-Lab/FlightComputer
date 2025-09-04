#include <TinyGPS++.h>

// Crear una instancia de TinyGPSPlus
TinyGPSPlus gps;

// Usa HardwareSerial (UART 1 o 2 en ESP32)
HardwareSerial GPSserial(1);  // UART1

void setup()
{
  Serial.begin(115200);      // Monitor serial
  GPSserial.begin(115200, SERIAL_8N1, 16, 17); // RX, TX del GPS

  Serial.println("ESP32 + TinyGPSPlus funcionando...");
}

void loop()
{
  while (GPSserial.available()) {
    gps.encode(GPSserial.read());
  }

  if (gps.location.isUpdated()) {
    Serial.print("Latitud: ");
    Serial.println(gps.location.lat(), 6);
    Serial.print("Longitud: ");
    Serial.println(gps.location.lng(), 6);
    Serial.print("Satélites: ");
    Serial.println(gps.satellites.value());
    Serial.print("HDOP: ");
    Serial.println(gps.hdop.hdop());
    Serial.println("---");
  }
}
