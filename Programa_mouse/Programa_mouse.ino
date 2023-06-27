#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include "Mouse.h"
#include <SoftwareSerial.h>

const int OFFSET_BUTTON = 4;

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
int desplazamiento = 5, desplazamiento_scroll = 1;  // dezplazamiento del cursor (de 0 a 127 segun la libreria)
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
MyData offset;
MyData posicion;

struct Threshold {
    float x_izq = 25.0;
    float x_der = -25.0;
    float y_abj = 25.0;
    float y_arr = -25.0;
};
Threshold threshold;

const int MOUSE_THRESHOLD = 100;
unsigned long mouseLastReady = 0;

int timer3InterruptIndex = 1;

void setup() {

    pinMode(OFFSET_BUTTON, INPUT_PULLUP);  //offset
    pinMode(5, INPUT_PULLUP);  //offset

    Serial.begin(115200);  //Set the speed to 9600 bauds if you want.
                        //You should always have the same speed selected in the serial monitor
    while (!Serial) ;

    mySerial.begin(9600);
    // Mouse.begin();

    Serial.println("Iniciando");
    if (!radio.begin()) {
        while (1) {
            Serial.println("ERROR - Radio not found");
            delay(2000);
        }
    }
    Serial.println("The radio espantoso is the best");

    radio.setAutoAck(false);
    radio.setDataRate(RF24_250KBPS);
    radio.openReadingPipe(1, pipeIn);
    // We start the radio comunication
    radio.startListening();

    configuracion_dead_zone();
}

// void Tiempo() {
//   demora = millis();
//   if (demora - demora_anterior1 >= tiempo1) {
//     demora_anterior1 = demora;
//     mueve = 1;
//   }

//   if (demora - demora_anterior2 >= tiempo2) {
//     demora_anterior2 = demora;
//     serial = 1;
//   }
//   if (demora - demora_anterior3 >= tiempo3) {
//     demora_anterior3 = demora;
//     click = 1;
//   }
// }

// void Monitorprint() {
//   if (serial == 1 && offset == 1) {
//     Serial.print("Angulo:");
//     Serial.print("        ");
//     Serial.print(data.angulo_x);
//     Serial.print("        ");
//     Serial.print(data.angulo_y);
//     Serial.print("        ");
//     Serial.print(data.angulo_z);
//     Serial.println("     ");

//     Serial.print("Offset:");
//     Serial.print("        ");
//     Serial.print(offset_x);
//     Serial.print("        ");
//     Serial.print(offset_y);
//     Serial.print("        ");
//     Serial.print(offset_z);
//     Serial.println("     ");

//     Serial.print("Posicion:");
//     Serial.print("        ");
//     Serial.print(posicion_x);
//     Serial.print("        ");
//     Serial.print(posicion_y);
//     Serial.print("        ");
//     Serial.print(posicion_z);
//     Serial.println("     ");

//     if (-rango < posicion_x && posicion_x < rango && -rango < posicion_y && posicion_y < rango && -rango < posicion_z && posicion_z < rango) {
//       Serial.println("Cabeza en el centro");
//     }
//     // mueve a la derecha
//     if (rango < posicion_x) {
//       Serial.println("INCLINA A LA DERECHA");
//     }
//     if (posicion_x < -rango) {
//       Serial.println("INCLINA A LA IZQUIERDA");
//     }
//     if (rango < posicion_y) {
//       Serial.println("CABEZA ARRIBA");
//     }
//     if (posicion_y < -rango) {
//       Serial.println("CABEZA ABAJO");
//     }
//     if (posicion_z < -rango2 && click == 0) {

//       Serial.println("CLICK IZQUIERDA");
//     }
//     if (posicion_z > rango2 && click == 0) {

//       Serial.println("CLICK DERECHO");
//     }
//     Serial.print("DESPLAZAMIENTO:    ");
//     Serial.println(desplazamiento);
//     Serial.print("RANGO:      ");
//     Serial.println(rango);
 
//     if (mode == 1) {
//       switch (blue_mode) {
//         case 1:
//           Serial.println("MODO ESCRITURA");  // activa modo escritura
//           break;
//         case 2:
//           Serial.println("MODO LECTURA");  //activa modo lectura (scroll)
//           break;
//       }
//     }
//     if (mode == 0) {
//       Serial.println("MODO LECTURA"); 
//     }
//   }
//   serial = 0;
// }


void deteccion_movimiento_1() {

  if(mouseReady()){
    mouseLastReady = millis();
    // Mueve IZQUIEDA
    if(data.angulo_x < threshold.x_izq){
      Serial.println("IZQUIERDA");
      Mouse.move(-desplazamiento, 0, 0);
    }
    // Mueve DERECHA
    if(data.angulo_x > threshold.x_der){
      Serial.println("DERECHA");
      Mouse.move(desplazamiento, 0, 0);
    }
    // Mueve ABAJO
    if(data.angulo_y < threshold.y_abj){
      Serial.println("ABAJO");
      Mouse.move(0, desplazamiento, 0);
    }
    // Mueve ARRIBA
    if(data.angulo_y > threshold.y_arr){
      Serial.println("ARRIBA");
      Mouse.move(0, -desplazamiento, 0);
    }
    
    // FALTA AGREGAR LOS CLICKS

    // if (posicion_z > rango2 && click == 1) {
    //   demora_anterior3 = demora;
    // if (offset == 0) {
    //   Mouse.click(MOUSE_RIGHT);
    //   click = 0;
    // }
    // //click izquierdo
    // if (posicion_z < -rango2 && click == 1) {
    //   click = 0;
    //   demora_anterior3 = demora;
    //   Mouse.click(MOUSE_LEFT);
    // }
  }
}

void deteccion_movimiento_2() {

  if(mouseReady()){
    mouseLastReady = millis();

    if(data.angulo_y < threshold.y_abj){
      Serial.println("SCROLL DOWN");
      Mouse.move(0, 0, -desplazamiento_scroll);
    }
    if(data.angulo_y > threshold.y_arr){
      Serial.println("SCROLL UP");
      Mouse.move(0, 0, desplazamiento_scroll);
    }
  }
}

// void Bluethooth() {
//   if (mySerial.available() > 0) {

//     comando = mySerial.read();
//     switch (comando) {  // a=arriba, b=below, d=derecha, i=izquierda, y=click izquierdo, z=click derecho, c = configuracion
//       case 'a':
//         Mouse.move(0, (range * vel), 0);
//         break;
//       case 'b':
//         Mouse.move(0, -(range * vel), 0);
//         break;
//       case 'd':
//         Mouse.move(-(range * vel), 0, 0);
//         break;
//       case 'i':
//         Mouse.move((range * vel), 0, 0);
//         break;
//       case 'y':
//         Mouse.click(MOUSE_LEFT);
//         break;
//       case 'z':
//         Mouse.click(MOUSE_RIGHT);
//         break;
//       case 'n':
//         blue_offset = 1;
//         break;
//       case 'j':
//         blue_offset = 0;
//         break;
//       case 'c':
//         MODE_conf = 1;
//         break;
//       case 'r':
//         rango++;
//         break;
//       case 't':
//         rango--;
//         break;
//       case 'p':
//         desplazamiento++;
//         break;
//       case 'w':
//         desplazamiento--;
//         break;
//       case 'g':
//         blue_mode = 1;
//         break;
//       case 'k':
//         blue_mode = 2;
//         break;
//     }
//   }
// }

void configuracion_dead_zone() {
  bool waiting;
  int incomingByte = 0;
  
  Serial.println("--- CONFIGURACIÓN DE ZONA MUERTA ---");
  
  Serial.println("Aprete el pulsador de offset");
  offset = digitalRead(OFFSET_BUTTON);
  while(offset){
    offset = digitalRead(OFFSET_BUTTON);
  }
  Serial.println("Seteando offset!");
  offset_x = data.angulo_x;
  offset_y = data.angulo_y;
  offset_z = data.angulo_z;

  Serial.println("Mueva la cabeza hacia la IZQUIERDA y escriba O para setear threshold");
  delay(2000);
  waiting = 1;
  while(waiting){
    if (radio.available()) {
      radio.read(&data, sizeof(MyData));
      apply_offset();
    }
    if(digitalRead(OFFSET_BUTTON) == 0){    
      threshold.x_izq = data.angulo_x;
      Serial.print("THS IZQ: ");        
      Serial.println(threshold.x_izq);        
      waiting = 0;
    }
  }
  Serial.println("Mueva la cabeza hacia la DERECHA y escriba O para setear threshold");
  delay(2000);;;
  waiting = 1;
  while(waiting){
    if (radio.available()) {
      radio.read(&data, sizeof(MyData));
      apply_offset();
    }
    if(digitalRead(OFFSET_BUTTON) == 0){    
      threshold.x_der = data.angulo_x;        
      Serial.print("THS DER: ");        
      Serial.println(threshold.x_der);        
      waiting = 0;
    }
  }
  Serial.println("Mueva la cabeza hacia la ABAJO y escriba O para setear threshold");
  delay(2000);;
  waiting = 1;
  while(waiting){
    if (radio.available()) {
      radio.read(&data, sizeof(MyData));
      apply_offset();
    }
    if(digitalRead(OFFSET_BUTTON) == 0){    
      threshold.y_abj = data.angulo_y;        
      Serial.print("THS ABJ: ");        
      Serial.println(threshold.y_abj);        
      waiting = 0;
    }
  }
  Serial.println("Mueva la cabeza hacia la ARRIBA y escriba O para setear threshold");
  delay(2000);
  waiting = 1;
  while(waiting){
    if (radio.available()) {
      radio.read(&data, sizeof(MyData));
      apply_offset();
    }
    if(digitalRead(OFFSET_BUTTON) == 0){    
      threshold.y_arr = data.angulo_y;        
      Serial.print("THS ARR: ");        
      Serial.println(threshold.y_arr);        
      waiting = 0;
    }
  }
  return;
}

bool mouseReady() {
  return (millis() - mouseLastReady) > MOUSE_THRESHOLD;
}

void apply_offset() {
    offset = digitalRead(OFFSET_BUTTON);
    if (offset == 0) {
        // Serial.println("Seteando offset!");
        offset_x = data.angulo_x;
        offset_y = data.angulo_y;
        offset_z = data.angulo_z;
    }

    data.angulo_x = data.angulo_x - offset_x;
    data.angulo_y = data.angulo_y - offset_y;
    data.angulo_z = data.angulo_z - offset_z;
    print_angulos(data.angulo_x, data.angulo_y, data.angulo_z);
}


void print_angulos(float x, float y, float z) {
    //Mostrar los angulos separadas
    Serial.print("X ");
    Serial.print(x);
    Serial.print(" | Y ");
    Serial.print(y);
    Serial.print(" | Z ");
    Serial.println(z);
}

void dead_zone() {
    // Dejar movimientos perpendiculares o agregar diagonales?
    // Como está ahora permite un mov por vez unicamente, sino sacar return
    // Se podría considerar unicamente el max de los dos ejes y tomar ese

    // Hacer función de seteo porcentual considerando los máximos desplazamientos por Bt
    // En tal caso, si se quiere considerar el desplazamiento max, también tendría que ser porcentual
    if(data.angulo_x < threshold.x_izq){
        Serial.println("IZQUIERDA");
    }
    if(data.angulo_x > threshold.x_der){
        Serial.println("DERECHA");
    }
    if(data.angulo_y < threshold.y_abj){
        Serial.println("ABAJO");
    }
    if(data.angulo_y > threshold.y_arr){
        Serial.println("ARRIBA");
    }
    return;
}

void loop() {
    // Se recibe la data
    if (radio.available()) {
        radio.read(&data, sizeof(MyData));
        // print_angulos(data.angulo_x, data.angulo_y, data.angulo_z);
        apply_offset();
        // dead_zone();
        // deteccion_movimiento_1();
        deteccion_movimiento_2();
    }

    // Esto es lo bueno
    // if(mouseReady) {
    //     mouseLastReady = millis();
    //     monitor_print();
    // }

    // if (offset_comienzo == 1 && radio.available()) {
    //     offset_x = data.angulo_x;
    //     offset_y = data.angulo_y;
    //     offset_z = data.angulo_z;
    //     Serial.println("Seteando offset!");
    //     offset_comienzo = 0;
    // }

    // Bluethooth();
    // Tiempo();
    // Monitorprint();
    
    // mode = digitalRead(5);
    
    // if (mode == 1) {
    //     switch (blue_mode) {
    //     case 1:
    //         Deteccion_movimiento1();  // activa modo escritura
    //         break;
    //     case 2:
    //         Deteccion_movimiento2();  //activa modo lectura (scroll)
    //         break;
    //     }
    // }
    // if (mode == 0) {
    //     Deteccion_movimiento2();
    // }

    // rango = constrain(rango, 1, 10);
    // desplazamiento = constrain(desplazamiento, 1, 10);
    // Offset();
}
