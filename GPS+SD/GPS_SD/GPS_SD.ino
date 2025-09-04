#include <TinyGPS++.h>
#include <HardwareSerial.h>
#include <SPI.h>
#include <SD.h>

// Crear instancia TinyGPSPlus
TinyGPSPlus gps;
// Puerto serial del GPS UART1: RX=16, TX=17
HardwareSerial GPSserial(1);

const int chipSelect = 5;  // Pin CS de la tarjeta SD (ajusta según tu conexión)

File gpsFile;

// Función para encontrar el siguiente nombre de archivo consecutivo
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

//File name getter
String getLogFileName() {
  int fileIndex = 1;
  while (true) {
    String gpsFile = "/gps_log" + String(fileIndex) + ".csv";
    if (!SD.exists(gpsFile)) {
      String gpsFile = "/gps_log" + String(fileIndex-1) + ".csv";
      return gpsFile;  
    }
    fileIndex++;
  }
}

void setup() {
  Serial.begin(115200);
  GPSserial.begin(115200, SERIAL_8N1, 16, 17);  

  Serial.println("Iniciando tarjeta SD...");

  if (!SD.begin(chipSelect)) {
    Serial.println("Error al inicializar SD");
    while (1);
  }
  Serial.println("SD inicializada");

  // Obtener el nombre de archivo siguiente
  String newFileName = getNextLogFileName();

  // Abrimos el archivo nuevo para escritura
  gpsFile = SD.open(newFileName, FILE_WRITE);
  if (gpsFile) {
    // Escribimos una fila de encabezado, separada por comas
    gpsFile.println("Fecha,Hora,Latitud,Longitud,Satelites,HDOP");
  } else {
    Serial.println("Error al abrir el archivo " + newFileName);
  }
}

void loop() {
  while (GPSserial.available()) {
    gps.encode(GPSserial.read());
  }

  if (gps.location.isUpdated()) {
    // Obtener fecha y hora del GPS
    String fecha = "";
    String hora = "";
    if (gps.date.isValid()) {
      fecha = String(gps.date.day()-1) + "/" + String(gps.date.month()) + "/" + String(gps.date.year());
    } else {
      fecha = "NA";
    }

    if (gps.time.isValid()) {
      // Formato hh:mm:ss
      hora = String(gps.time.hour()-6) + ":" + String(gps.time.minute()) + ":" + String(gps.time.second());
    } else {
      hora = "NA";
    }

    // Construir línea CSV
    String line = fecha + "," + hora + ",";
    line += String(gps.location.lat(), 6) + ",";
    line += String(gps.location.lng(), 6) + ",";
    line += String(gps.satellites.value()) + ",";
    line += String(gps.hdop.hdop());

    Serial.println(line);

    // Guardar en archivo
    gpsFile.println(line);
    gpsFile.flush();
  }
}