#include <TinyGPS++.h>
#include <HardwareSerial.h>
#include <SPI.h>
#include <SD.h>
#include <Adafruit_BMP085.h>

#include <Wire.h>
#include "MPU6050_6Axis_MotionApps20.h"

// Crear instancia TinyGPSPlus y barómetro
TinyGPSPlus gps;
HardwareSerial GPSserial(1);

Adafruit_BMP085 bmp;

const int chipSelect = 5;  // Pin CS de la tarjeta SD
File gpsFile;

// MPU6050
MPU6050 mpu;
uint8_t fifoBuffer[64];
Quaternion q;

String getNextLogFileName() {
  int fileIndex = 1;
  while (true) {
    String gpsFile = "/gps_log" + String(fileIndex) + ".csv";
    if (!SD.exists(gpsFile)) {
      return gpsFile;  // Retorna el primer archivo que no existe
    }
    fileIndex++;
  }
}

void setup() {
  Serial.begin(115200);
  Wire.begin();
  GPSserial.begin(115200, SERIAL_8N1, 16, 17);

  Serial.println("Iniciando tarjeta SD...");
  if (!SD.begin(chipSelect)) {
    Serial.println("Error al inicializar SD");
    while (1);
  }
  Serial.println("SD inicializada");

  if (!bmp.begin()) {
    Serial.println("No se encontró un sensor BMP085 válido, revisar conexiones!");
    while (1);
  }

  // Inicializar MPU6050
  mpu.initialize();
  if (!mpu.testConnection()) {
    Serial.println("MPU6050 no conectado");
    while (1);
  }

  uint8_t devStatus = mpu.dmpInitialize();
  if (devStatus == 0) {
    mpu.setDMPEnabled(true);
    Serial.println("DMP habilitado");
  } else {
    Serial.print("Error al inicializar DMP: ");
    Serial.println(devStatus);
    while (1); // Parar si fallo el DMP
  }

  // Obtener nombre de archivo nuevo
  String newFileName = getNextLogFileName();

  // Abrir archivo para escritura
  gpsFile = SD.open(newFileName, FILE_WRITE);
  if (gpsFile) {
    // Encabezado CSV extendido con columnas del giroscopio
    gpsFile.println("Fecha,Hora,Latitud,Longitud,Satelites,HDOP,Temp_BMP_C,Presion_Pa,Q_w,Q_x,Q_y,Q_z");
  } else {
    Serial.println("Error al abrir archivo " + newFileName);
  }
}

void loop() {
  // Leer datos GPS
  while (GPSserial.available()) {
    gps.encode(GPSserial.read());
  }

  // Variables para cuaterniones
  bool hasQuaternion = false;

  // Leer paquete DMP si disponible
  if (mpu.dmpGetCurrentFIFOPacket(fifoBuffer)) {
    mpu.dmpGetQuaternion(&q, fifoBuffer);
    hasQuaternion = true;
  }

  // Solo actúa si la ubicación se actualiza y tenemos datos del giroscopio
  if (gps.location.isUpdated() && hasQuaternion) {
    // Fecha
    String fecha = "NA";
    if (gps.date.isValid()) {
      fecha = String(gps.date.day()) + "/" + String(gps.date.month()) + "/" + String(gps.date.year());
    }

    // Hora
    String hora = "NA";
    if (gps.time.isValid()) {
      int horaLocal = gps.time.hour() - 6;  // Ajuste horario, ajustar si necesario
      if (horaLocal < 0) horaLocal += 24;
      char tiempo[9];
      sprintf(tiempo, "%02d:%02d:%02d", horaLocal, gps.time.minute(), gps.time.second());
      hora = String(tiempo);
    }

    // Leer barómetro (temperatura y presión)
    float temp_bmp = bmp.readTemperature();  // °C
    float presion = bmp.readPressure();      // Pa

    // Construir línea CSV con datos del giroscopio
    String line = fecha + "," + hora + ",";
    line += String(gps.location.lat(), 6) + ",";
    line += String(gps.location.lng(), 6) + ",";
    line += String(gps.satellites.value()) + ",";
    line += String(gps.hdop.hdop()) + ",";
    line += String(temp_bmp, 2) + ",";
    line += String(presion, 0) + ",";
    line += String(q.w, 3) + ",";
    line += String(q.x, 3) + ",";
    line += String(q.y, 3) + ",";
    line += String(q.z, 3);

    Serial.println(line);

    // Guardar en archivo
    gpsFile.println(line);
    gpsFile.flush();
  }
}
