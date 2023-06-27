#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include "Mouse.h"
#include <SoftwareSerial.h>

//SoftwareSerial (9, 6);
SoftwareSerial mySerial(9, 6);
float anguloz;
float ultimo_anguloz;


// bluetooth
int range = 20;
int vel = 10;
char comando;
int MODE_conf = 0;
int blue_config;

int blue_offset = 0;
int blue_sensibilidad;
int blue_mode = 1;

int mode;

int serial;
int offset_comienzo = 1;

// mouse
int mueve;
// variables de calibracion mouse
int desplazamiento = 5;  // dezplazamiento del cursor (de 0 a 127 segun la libreria)
int rango = 20;          // rango desde donde se comienza a mover
int rango2 = 35;         // rango desde donde se comienza a mover
//int rango3 = 25;           // rango desde donde se comienza a mover
int tiempo1 = 30;    // cada cuantos mili segundos se mueve
int tiempo2 = 1000;  //monitor serie
int tiempo3 = 2000;  // tiempo de click

int demora;
int demora_anterior1;
int demora_anterior2;
int demora_anterior3;
int demora_anterior4;


int click;

int offset;
float offset_x;
float offset_y;
float offset_z;

float posicion_x;
float posicion_y;
float posicion_z;

const uint64_t pipeIn = 0xE8E8F0F0E1LL;  //IMPORTANT: The same as in the receiver!!!

RF24 radio(8, 10);  // select  CSN and CE  pins
struct MyData {
  float angulo_x;
  float angulo_y;
  float angulo_z;
};
MyData data;
////////////////////////////////////////////////////////

/**/

void setup() {

  pinMode(4, INPUT_PULLUP);  //offset

  pinMode(5, INPUT_PULLUP);  //offset

  Serial.begin(9600);  //Set the speed to 9600 bauds if you want.
                       //You should always have the same speed selected in the serial monitor


  mySerial.begin(9600);
  Mouse.begin();


  if (!radio.begin()) {
    Serial.println("Radio not found amigo");
    while (1) {
      delay(10);
    }
  }
  Serial.println("The radio espantoso is the best amigo");

  radio.setAutoAck(false);
  radio.setDataRate(RF24_250KBPS);
  radio.openReadingPipe(1, pipeIn);
  //we start the radio comunication
  radio.startListening();
}


void recvData() {
  while (radio.available()) {
    radio.read(&data, sizeof(MyData));

    //here we receive the data
  }
}

void Tiempo() {
  demora = millis();
  if (demora - demora_anterior1 >= tiempo1) {
    demora_anterior1 = demora;
    mueve = 1;
  }

  if (demora - demora_anterior2 >= tiempo2) {
    demora_anterior2 = demora;
    serial = 1;
  }
  if (demora - demora_anterior3 >= tiempo3) {
    demora_anterior3 = demora;
    click = 1;
  }
}

void Monitorprint() {
  if (serial == 1 && offset == 1) {
    Serial.print("Angulo:");
    Serial.print("        ");
    Serial.print(data.angulo_x);
    Serial.print("        ");
    Serial.print(data.angulo_y);
    Serial.print("        ");
    Serial.print(data.angulo_z);
    Serial.println("     ");

    Serial.print("Offset:");
    Serial.print("        ");
    Serial.print(offset_x);
    Serial.print("        ");
    Serial.print(offset_y);
    Serial.print("        ");
    Serial.print(offset_z);
    Serial.println("     ");

    Serial.print("Posicion:");
    Serial.print("        ");
    Serial.print(posicion_x);
    Serial.print("        ");
    Serial.print(posicion_y);
    Serial.print("        ");
    Serial.print(posicion_z);
    Serial.println("     ");

    if (-rango < posicion_x && posicion_x < rango && -rango < posicion_y && posicion_y < rango && -rango < posicion_z && posicion_z < rango) {
      Serial.println("Cabeza en el centro");
    }
    // mueve a la derecha
    if (rango < posicion_x) {
      Serial.println("INCLINA A LA DERECHA");
    }
    if (posicion_x < -rango) {
      Serial.println("INCLINA A LA IZQUIERDA");
    }
    if (rango < posicion_y) {
      Serial.println("CABEZA ARRIBA");
    }
    if (posicion_y < -rango) {
      Serial.println("CABEZA ABAJO");
    }
    if (posicion_z < -rango2 && click == 0) {

      Serial.println("CLICK IZQUIERDA");
    }
    if (posicion_z > rango2 && click == 0) {

      Serial.println("CLICK DERECHO");
    }
    Serial.print("DESPLAZAMIENTO:    ");
    Serial.println(desplazamiento);
    Serial.print("RANGO:      ");
    Serial.println(rango);
 
    if (mode == 1) {
      switch (blue_mode) {
        case 1:
          Serial.println("MODO ESCRITURA");  // activa modo escritura
          break;
        case 2:
          Serial.println("MODO LECTURA");  //activa modo lectura (scroll)
          break;
      }
    }
    if (mode == 0) {
      Serial.println("MODO LECTURA"); 
    }
  }
  serial = 0;
}

void Offset() {

  offset = digitalRead(4);
  if (offset == 0) {
    offset_x = data.angulo_x;
    offset_y = data.angulo_y;
    offset_z = data.angulo_z;
    Serial.println("Seteando offset!");
  }
  if (blue_offset == 1) {
    offset_x = data.angulo_x;
    offset_y = data.angulo_y;
    offset_z = data.angulo_z;
    Serial.println("Seteando offset!");
  }

  posicion_x = data.angulo_x - offset_x;
  posicion_y = data.angulo_y - offset_y;
  posicion_z = data.angulo_z - offset_z;
}

void Deteccion_movimiento1() {

  if (mueve == 1 && offset == 1) {


    // mueve a la derecha
    if (rango < posicion_x && click == 1) {

      Mouse.move(-desplazamiento, 0, 0);
    }
    //mueve a la izquierda
    if (posicion_x < -rango && click == 1) {
      Mouse.move(desplazamiento, 0, 0);
    }
    //mueve arriba
    if (rango < posicion_y && click == 1) {

      Mouse.move(0, desplazamiento, 0);
    }
    // mueve abajo
    if (posicion_y < -rango && click == 1) {

      Mouse.move(0, -desplazamiento, 0);
    }
    //click derecho
    if (posicion_z > rango2 && click == 1) {
      demora_anterior3 = demora;
      Mouse.click(MOUSE_RIGHT);
      click = 0;
    }
    //click izquierdo
    if (posicion_z < -rango2 && click == 1) {
      click = 0;
      demora_anterior3 = demora;
      Mouse.click(MOUSE_LEFT);
    }
  }

  mueve = 0;
}
void Deteccion_movimiento2() {

  if (mueve == 1 && offset == 1) {

    if (-rango < posicion_x && posicion_x < rango && -rango < posicion_y && posicion_y < rango && -rango < posicion_z && posicion_z < rango) {
    }
    /*
    // mueve a la derecha
    if (rango < posicion_x) {

      Mouse.move(0, 0, desplazamiento);
    }

    if (posicion_x < -rango) {

      Mouse.move(0, 0, -desplazamiento);
    }
    */
    if (rango < posicion_y) {

      Mouse.move(0, 0, desplazamiento);
    }
    if (posicion_y < -rango) {

      Mouse.move(0, 0, -desplazamiento);
    }
  }
  mueve = 0;
  /*
  anguloz = data.angulo_z;
  if ()
    if ()

      if (20 < data.angulo_z) {
        Serial.println("Gira a la derecha");
      }
  if (data.angulo_y < -20) {
    Serial.println("Gira a la izquierda");
  }
    */
}

void Bluethooth() {
  if (mySerial.available() > 0) {

    comando = mySerial.read();
    switch (comando) {  // a=arriba, b=below, d=derecha, i=izquierda, y=click izquierdo, z=click derecho, c = configuracion
      case 'a':
        Mouse.move(0, (range * vel), 0);
        break;
      case 'b':
        Mouse.move(0, -(range * vel), 0);
        break;
      case 'd':
        Mouse.move(-(range * vel), 0, 0);
        break;
      case 'i':
        Mouse.move((range * vel), 0, 0);
        break;
      case 'y':
        Mouse.click(MOUSE_LEFT);
        break;
      case 'z':
        Mouse.click(MOUSE_RIGHT);
        break;
      case 'n':
        blue_offset = 1;
        break;
      case 'j':
        blue_offset = 0;
        break;
      case 'c':
        MODE_conf = 1;
        break;
      case 'r':
        rango++;
        break;
      case 't':
        rango--;
        break;
      case 'p':
        desplazamiento++;
        break;
      case 'w':
        desplazamiento--;
        break;
      case 'g':
        blue_mode = 1;
        break;
      case 'k':
        blue_mode = 2;
        break;
    }
  }
}

void loop() {
  recvData();

  if (offset_comienzo == 1 && radio.available()) {
    offset_x = data.angulo_x;
    offset_y = data.angulo_y;
    offset_z = data.angulo_z;
    Serial.println("Seteando offset!");
    offset_comienzo = 0;
  }

  Bluethooth();
  Tiempo();
  Monitorprint();
  mode = digitalRead(5);
  if (mode == 1) {
    switch (blue_mode) {
      case 1:
        Deteccion_movimiento1();  // activa modo escritura
        break;
      case 2:
        Deteccion_movimiento2();  //activa modo lectura (scroll)
        break;
    }
  }
  if (mode == 0) {
    Deteccion_movimiento2();
  }

  rango = constrain(rango, 1, 10);
  desplazamiento = constrain(desplazamiento, 1, 10);
  Offset();
}
