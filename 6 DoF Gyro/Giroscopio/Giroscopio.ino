#include <Wire.h>
#include "MPU6050_6Axis_MotionApps20.h"

// Objeto MPU6050
MPU6050 mpu;

// Buffer FIFO para DMP
uint8_t fifoBuffer[64];

// Cuaternión
Quaternion q;

void setup() {
  Wire.begin();
  Serial.begin(115200);

  mpu.initialize();
  if (!mpu.testConnection()) {
    Serial.println("MPU6050 no conectado");
    while (1);
  }

  // Inicializar DMP
  uint8_t devStatus = mpu.dmpInitialize();
  if (devStatus == 0) {
    mpu.setDMPEnabled(true);
    Serial.println("DMP habilitado");
  } else {
    Serial.print("Error al inicializar DMP: "); Serial.println(devStatus);
  }
}

void loop() {
  // Leer paquete DMP
  if (mpu.dmpGetCurrentFIFOPacket(fifoBuffer)) {
    // Obtener cuaternión
    mpu.dmpGetQuaternion(&q, fifoBuffer);

    // Enviar por serial a Processing
    Serial.print(q.w, 3); Serial.print(",");
    Serial.print(q.x, 3); Serial.print(",");
    Serial.print(q.y, 3); Serial.print(",");
    Serial.println(q.z, 3);
  }
}
