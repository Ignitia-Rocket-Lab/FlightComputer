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

// Barómetro 
float p_inicial;
float presion;
const int p_altura=2390; 

// Para Velocidad Vertical 
float altura_prev = 0;          // altura anterior
unsigned long lastTime = 0;     // tiempo anterior

// Para altura filtrada 
float altura_filtrada = 0;
bool primeraLectura = true;

//Para detección de apogeo 
bool apogeeDetected = false;
int consecutiveConfirm = 0;
const int confirmSamplesNeeded = 3;   // Número de lecturas consecutivas
const float velUmbral = -0.2;         // Velocidad vertical mínima para confirmar apogeo
float alturaMaxima = 0;             // registra la altura máxima alcanzada
const float alturaSeguraMinima = 50; // ejemplo: despliegue mínimo, metros
unsigned long tiempoLanzamiento = 0;
const unsigned long tiempoMaximo = 10000; // ms desde despegue para fallback
bool paracaidasDesplegado = false;

//Apogeo con IMU
float accZ_filtrada;
int apogeoMensajeContador = 0;  // contador de veces que se imprime el mensaje
bool apogeoMensajeImpreso = false; // para resetear si quieres repetir pruebas

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

//Función Barómetro 
float pressureToAltitude(float p, float p0) {
  // Fórmula barométrica estándar
  float T0 = 288.15;     // Temperatura estándar (K) ---> Podemos medirla con sensor tambien para mayor exactitud
  float lapse = 0.0065;  // Gradiente térmico (K/m)
  float exponent = 1.0 / 5.257;
  float ratio = p0 / p;
  float h = (pow(ratio, exponent) - 1.0) * (T0 / lapse);
  return h;
}

void setup() {
  Serial.begin(115200);
  Wire.begin();
  GPSserial.begin(115200, SERIAL_8N1, 16, 17);

  // Inicializar tiempo --> velocidad
  lastTime = millis();

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

  // Actualizar presión inicial
  p_inicial = bmp.readPressure();      // Pa
  Serial.print("Presión inicial: ");
  Serial.println(p_inicial);

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
    presion = bmp.readPressure();      // Pa
    
    // Datos de altura y presion 
    float altura_rel = pressureToAltitude(presion, p_inicial);
    Serial.print("Presion actual: ");
    Serial.print(presion);
    Serial.print(" Pa, Altura relativa: ");
    Serial.print(altura_rel);
    Serial.println(" m");

    // calculo de dt para velocidad vertical 
    unsigned long now = millis();
    float dt = (now - lastTime) / 1000.0;  // en segundos
    lastTime = now;
    if (dt <= 0) dt = 0.02;  // evitar división por cero

    //INICIO Filtro IIR (promedio exponencial)
    const float alpha = 0.5;  // 0.05-0.2 típico, ajusta según pruebas
    if (primeraLectura) {
      altura_filtrada = altura_rel;  // iniciar filtro
      primeraLectura = false;
    } else {
      altura_filtrada = alpha * altura_rel + (1 - alpha) * altura_filtrada;
    }

    // Datos para velocidad vertical -------------------------------------------------
    float vel_vertical = (altura_filtrada - altura_prev) / dt;
    altura_prev = altura_filtrada;

    // Registrar la altura máxima alcanzada
    if (altura_filtrada > alturaMaxima) {
      alturaMaxima = altura_filtrada;
    }
    // Inicializar tiempo de lanzamiento en la primera lectura significativa
    if (tiempoLanzamiento == 0 && altura_filtrada > 5) { // altura > 5 m indica despegue
      tiempoLanzamiento = millis();
    }


    //Para detectr apogeo con IMU
    VectorInt16 aa;
    mpu.getAcceleration(&aa.x, &aa.y, &aa.z);
    float accZ = aa.z / 16384.0; // g o m/s²

    // Filtro IIR
    const float alphaAcc = 0.2;
    accZ_filtrada = alphaAcc * accZ + (1 - alphaAcc) * accZ_filtrada;

    //Para detectar apogeo
    if (!paracaidasDesplegado) {
      // Apogeo normal
      if (altura_filtrada >= 200.0 && vel_vertical <= velUmbral && accZ_filtrada < 0) {
          consecutiveConfirm++;
          if (consecutiveConfirm >= confirmSamplesNeeded) {
              paracaidasDesplegado = true;
              Serial.println("Apogeo detectado (con aceleración): desplegar paracaídas");
          }
      } else {
          consecutiveConfirm = 0;
      }

      // Fallback: altura mínima o tiempo máximo
      if (!paracaidasDesplegado &&
          (alturaMaxima >= alturaSeguraMinima ||
          (tiempoLanzamiento > 0 && millis() - tiempoLanzamiento >= tiempoMaximo))) {
          paracaidasDesplegado = true;
          Serial.println("Fallback: desplegar paracaídas");
      }
    }

    Serial.print("Altura (filt): ");
    Serial.print(altura_filtrada, 2);
    Serial.print(" m, Velocidad vertical (filt): ");
    Serial.print(vel_vertical, 2);
    Serial.println(" m/s");

    //Fin velocidad vertical -------------------------------------------------------------
    
    //Mnsaje de apogeo
    if (paracaidasDesplegado && accZ_filtrada < 0 && apogeoMensajeContador < 5) {
      Serial.println("Apogeo alcanzado");
      apogeoMensajeContador++;
    }

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

    //Serial.println(line);

    // Guardar en archivo
    gpsFile.println(line);
    gpsFile.flush();
  }

  // --- Verificar si ya alcanzamos ~200 m ---
  if ((p_inicial - presion) >= 2390) {   // diferencia en Pa
    Serial.println("Altura alcanzada");
  }
}
