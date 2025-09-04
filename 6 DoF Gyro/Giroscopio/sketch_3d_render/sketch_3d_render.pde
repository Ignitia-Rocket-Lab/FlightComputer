import processing.serial.*;

Serial myPort;
float qw=1,qx=0,qy=0,qz=0;

void setup() {
  size(600,600,P3D);
  println(Serial.list());
  myPort = new Serial(this,"COM12",115200);
  myPort.bufferUntil('\n');
}

void draw() {
  background(220);
  lights();
  translate(width/2,height/2,0);

  // Aplicar cuaternión para rotación
  PMatrix3D mat = quaternionToMatrix(qw, -qx, qz, qy);
  applyMatrix(mat);

  drawRocket();

  fill(0);
  textSize(16);
  text("qw: "+nf(qw,1,3),10,20);
  text("qx: "+nf(qx,1,3),10,40);
  text("qy: "+nf(qy,1,3),10,60);
  text("qz: "+nf(qz,1,3),10,80);
}

void serialEvent(Serial p) {
  String inString = p.readStringUntil('\n');
  if (inString != null) {
    inString = trim(inString);
    String[] values = split(inString, ',');
    if (values.length == 4) {
      try {
        qw = float(values[0]);
        qx = float(values[1]);
        qy = float(values[2]);
        qz = float(values[3]);
      } catch(Exception e) {
        println("Error al parsear: "+inString);
      }
    }
  }
}

// Convierte cuaternión a matriz 3D
PMatrix3D quaternionToMatrix(float w, float x, float y, float z) {
  PMatrix3D m = new PMatrix3D();

  m.m00 = 1 - 2*y*y - 2*z*z;
  m.m01 = 2*x*y - 2*z*w;
  m.m02 = 2*x*z + 2*y*w;
  m.m03 = 0;

  m.m10 = 2*x*y + 2*z*w;
  m.m11 = 1 - 2*x*x - 2*z*z;
  m.m12 = 2*y*z - 2*x*w;
  m.m13 = 0;

  m.m20 = 2*x*z - 2*y*w;
  m.m21 = 2*y*z + 2*x*w;
  m.m22 = 1 - 2*x*x - 2*y*y;
  m.m23 = 0;

  m.m30 = 0;
  m.m31 = 0;
  m.m32 = 0;
  m.m33 = 1;

  return m;
}

// Función para dibujar un cohete simple alineado
void drawRocket() {
  float r = 20;  // radio para cilindro y base del cono
  float hBody = 100; // altura del cuerpo
  float hNose = 40;  // altura de la nariz

  // Cuerpo
  fill(180,0,0);
  pushMatrix();
  translate(0,0,0);
  cylinder(r, hBody);
  popMatrix();

  // Nariz
  fill(255,200,0);
  pushMatrix();
  translate(0,-hBody/2 - hNose/2,0); // poner en la parte superior del cuerpo
  cone(r, hNose);
  popMatrix();

  // Aletas
  fill(0,0,180);
  pushMatrix();
  translate(-15,hBody/2,0); rotateZ(PI/4);
  box(10,30,2);
  popMatrix();

  pushMatrix();
  translate(15,hBody/2,0); rotateZ(-PI/4);
  box(10,30,2);
  popMatrix();

  pushMatrix();
  translate(0,hBody/2,-15); rotateX(-PI/4);
  box(2,30,10);
  popMatrix();

  pushMatrix();
  translate(0,hBody/2,15); rotateX(PI/4);
  box(2,30,10);
  popMatrix();
}

// Funciones auxiliares: cilindro y cono
void cylinder(float r, float h) {
  int sides = 30;
  beginShape(QUAD_STRIP);
  for (int i=0; i<=sides; i++) {
    float angle = TWO_PI / sides * i;
    float x = cos(angle)*r;
    float z = sin(angle)*r;
    vertex(x, -h/2, z);
    vertex(x, h/2, z);
  }
  endShape();
}

void cone(float r, float h) {
  int sides = 30;
  beginShape(TRIANGLE_FAN);
  vertex(0, -h/2, 0);
  for (int i=0; i<=sides; i++) {
    float angle = TWO_PI / sides * i;
    float x = cos(angle)*r;
    float z = sin(angle)*r;
    vertex(x, h/2, z);
  }
  endShape();
}
