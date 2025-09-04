#include "FS.h"
#include "SD.h"
#include "SPI.h"

#define SD_CS 5  // Ajusta este pin a tu conexión de CS

// Función para encontrar el siguiente nombre de archivo consecutivo
String getNextLogFileName() {
  int fileIndex = 1;
  while (true) {
    String fileName = "/log" + String(fileIndex) + ".csv";
    if (!SD.exists(fileName)) {
      return fileName;  // Retorna el primer archivo que no existe
    }
    fileIndex++;
  }
}

void setup() {
  Serial.begin(115200);
  if (!SD.begin(SD_CS)) {
    Serial.println("Error al inicializar la tarjeta SD");
    return;
  }
  Serial.println("Tarjeta SD inicializada correctamente");

  // Obtener el nombre de archivo siguiente
  String newFileName = getNextLogFileName();
  Serial.println("Nombre de archivo para escritura: " + newFileName);

  // Abrimos el archivo nuevo para escritura
  File file = SD.open(newFileName, FILE_WRITE);
  if (file) {
    // Escribimos una fila de encabezado, separada por comas
    file.println("Temperatura, Humedad, Presion");
    // Escribimos datos de ejemplo
    file.println("25.3, 60, 1013");
    file.println("26.1, 58, 1012");
    file.close();
    Serial.println("Archivo " + newFileName + " escrito exitosamente");
  } else {
    Serial.println("Error al abrir el archivo " + newFileName);
  }
}

void loop() {
  // No hay nada que hacer aquí
}