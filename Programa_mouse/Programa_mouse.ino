#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include "Mouse.h"
#include <SoftwareSerial.h>

#define CLICK_TIME_THRESHOLD      200
#define SWITCH_TIME_THRESHOLD     500
#define LED_TOGGLE_TIME           25 
#define MOUSE_THRESHOLD_INDEX_MAX 10   

const int LEFT_CLICK_BUTTON = 2,
          RIGHT_CLICK_BUTTON = 3,
          MODE_SWITCH = 4,
          OFFSET_BUTTON = 5,
          GREEN_LED = 6;

SoftwareSerial mySerial(9, 8); 
// 8: Arduino Tx - BT Rx
// 9: Arduino Rx - BT Tx

RF24 radio(A0, 10);  // select  CSN and CE  pins
const uint64_t pipeIn = 0xE8E8F0F0E1LL;  //IMPORTANT: The same as in the receiver!!!

const int MOUSE_TIME_THRESHOLD_ARRAY[MOUSE_THRESHOLD_INDEX_MAX] = {10, 20, 30, 40, 50, 60, 70, 80, 90, 100};
int mouse_time_threshold, mouse_threshold_index = 4;
int mode = 1;

struct MyData {
  float angulo_x;
  float angulo_y;
  float angulo_z;
};
MyData data;
MyData offset;

int offsetCenter;

struct Threshold {
    float x_izq = 25.0;
    float x_der = -25.0;
    float y_abj = 25.0;
    float y_arr = -25.0;
};
Threshold threshold;

enum direcciones{
  IZQUIERDA = 0,
  DERECHA,
  ABAJO, 
  ARRIBA
};
int desplazamiento = 5, desplazamiento_scroll = 1;  // dezplazamiento del cursor (de 0 a 127 segun la libreria)

bool deadzone_config_rdy = 0;

unsigned long startTimeLeftClick = 0, startTimeRightClick = 0, startTimeModeSwitch = 0;
unsigned long mouseLastReady = 0;

bool led_state = 0;
int timer1InterruptCounter = 0;

// ----- Variables viejas -----
// bluetooth
int desplazamiento_bt = 5;
char comando;

// --------------------------------------------------------------------- Main Functions

void setup() {

  pinMode(OFFSET_BUTTON, INPUT_PULLUP);  
  pinMode(MODE_SWITCH, INPUT_PULLUP);  
  pinMode(GREEN_LED, OUTPUT);

  noInterrupts();
  setup_timer_1();
  // Interrupciones de pulsador por rising edge
  attachInterrupt(digitalPinToInterrupt(LEFT_CLICK_BUTTON), left_click, RISING);
  attachInterrupt(digitalPinToInterrupt(RIGHT_CLICK_BUTTON), right_click, RISING);
  interrupts();

  Serial.begin(115200);   
  while (!Serial) ;

  mySerial.begin(9600);
  Mouse.begin();

  Serial.println("Iniciando");
  while (!radio.begin()) {
    Serial.println("ERROR - Radio not found");
    delay(2000);
  }
  Serial.println("The radio espantoso is the best");

  radio.setAutoAck(false);
  radio.setDataRate(RF24_250KBPS);
  radio.openReadingPipe(1, pipeIn);
  
  // We start the radio comunication
  radio.startListening();

  digitalWrite(GREEN_LED, LOW);

  check_mode();

  digitalWrite(GREEN_LED, LOW);
  led_state = 0;
  mouse_time_threshold = MOUSE_TIME_THRESHOLD_ARRAY[mouse_threshold_index];
}

void loop() {
    if(!deadzone_config_rdy && !digitalRead(OFFSET_BUTTON)) {
      // Secuencia de configuracion dead zone
      configuracion_dead_zone();
      deadzone_config_rdy = 1;
    }

    // Se recibe la data
    if(radio.available()) {
        radio.read(&data, sizeof(MyData));
        // print_angulos(data.angulo_x, data.angulo_y, data.angulo_z);
        apply_offset();
        // dead_zone();
        mode ? deteccion_movimiento_1() : deteccion_movimiento_2();
    }

    check_mode();
    Bluethooth();
}

// --------------------------------------------------------------------- ISR

// Interrupción Timer 1 para parpadeo de led
ISR(TIMER1_COMPA_vect) {
  timer1InterruptCounter++;
  if(timer1InterruptCounter == LED_TOGGLE_TIME){
    led_state = !led_state;
    digitalWrite(GREEN_LED, led_state);
    timer1InterruptCounter = 0;
  }
}

void left_click() {
  if(millis() - startTimeLeftClick > CLICK_TIME_THRESHOLD) {
    startTimeLeftClick = millis();
    Mouse.click(MOUSE_LEFT);
  }
}

void right_click() {
  if(millis() - startTimeRightClick > CLICK_TIME_THRESHOLD) {
    startTimeRightClick = millis();
    Mouse.click(MOUSE_RIGHT);
  }
}

// --------------------------------------------------------------------- Funciones
// Interrupcion cada 10ms
void setup_timer_1(){
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;
  OCR1A = 20000;
  TCCR1B |= (1 << WGM12);   //CTC
  TCCR1B |= (1 << CS11);    // Prescaler 256
  timer_1_ISR_disable();
}

void timer_1_ISR_enable() { TIMSK1 |= (1 << OCIE1A); }

void timer_1_ISR_disable() { TIMSK1 &= ~(1 << OCIE1A); }

void configuracion_dead_zone() {
  bool waiting;
  int incomingByte = 0;
  
  Serial.println("--- CONFIGURACIÓN DE ZONA MUERTA ---");
  
  Serial.println("Aprete el pulsador de offset");
  offsetCenter = digitalRead(OFFSET_BUTTON);
  while(offsetCenter){
    if(radio.available())
      radio.read(&data, sizeof(MyData));
    offsetCenter = digitalRead(OFFSET_BUTTON);
  }
  Serial.println("Seteando offset!");
  offset.angulo_x = data.angulo_x;
  offset.angulo_y = data.angulo_y;
  offset.angulo_z = data.angulo_z;
  
  set_offset(IZQUIERDA);
  set_offset(DERECHA);
  set_offset(ABAJO);
  set_offset(ARRIBA);

  return;
}

void set_offset(direcciones dir) {
  digitalWrite(GREEN_LED, HIGH);
  led_state = 1;

  switch(dir) {
    case 0:
      Serial.println("Mueva la cabeza hacia la IZQUIERDA y pulse el boton de offset");
      break;
    case 1:
      Serial.println("Mueva la cabeza hacia la DERECHA y pulse el boton de offset");
      break;
    case 2:
      Serial.println("Mueva la cabeza hacia la ABAJO y pulse el boton de offset");
      break;
    case 3:
      Serial.println("Mueva la cabeza hacia la ARRIBA y pulse el boton de offset");
      break;
  }
  delay(2000);
  
  timer_1_ISR_enable();
  
  bool waiting = 1;
  while(waiting){
    if(radio.available()){
      radio.read(&data, sizeof(MyData));
      apply_offset();
    }

    if(digitalRead(OFFSET_BUTTON) == 0)
      waiting = 0;
  }

  switch(dir){
    case 0:
      threshold.x_izq = data.angulo_x;
      Serial.print("THS IZQ: ");        
      Serial.println(threshold.x_izq);        
      break;
    case 1:
      threshold.x_der = data.angulo_x;        
      Serial.print("THS DER: ");        
      Serial.println(threshold.x_der);        
      break;
    case 2:
      threshold.y_abj = data.angulo_y;        
      Serial.print("THS ABJ: ");        
      Serial.println(threshold.y_abj);        
      break;
    case 3:
      threshold.y_arr = data.angulo_y;        
      Serial.print("THS ARR: ");        
      Serial.println(threshold.y_arr);        
      break;
  }
  timer_1_ISR_disable();
  return;
}

void check_mode() {
  if(millis() - startTimeModeSwitch > SWITCH_TIME_THRESHOLD) {
    startTimeModeSwitch = millis();
    if(mode != digitalRead(MODE_SWITCH)) {
      mode = !mode;
    }
  }
}

void deteccion_movimiento_1() {
  
  if(mouseReady()) {
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

bool mouseReady() {
  return (millis() - mouseLastReady) > mouse_time_threshold;
}

void apply_offset() {
    offsetCenter = digitalRead(OFFSET_BUTTON);
    if (offsetCenter == 0) {
      // Serial.println("Seteando offset!");
      offset.angulo_x = data.angulo_x;
      offset.angulo_y = data.angulo_y;
      offset.angulo_z = data.angulo_z;
    }

    data.angulo_x = data.angulo_x - offset.angulo_x;
    data.angulo_y = data.angulo_y - offset.angulo_y;
    data.angulo_z = data.angulo_z - offset.angulo_z;
    // print_angulos(data.angulo_x, data.angulo_y, data.angulo_z);
}

// --------------------------------------------------------------------- Serial prints

void print_angulos(float x, float y, float z) {
    Serial.print("X ");
    Serial.print(x);
    Serial.print(" | Y ");
    Serial.print(y);
    Serial.print(" | Z ");
    Serial.println(z);
}

void dead_zone() {
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

// --------------------------------------------------------------------- Bluetooth

// --- Cuando se termine la app ver que comando corresponde a cada letra o tal
// --- Secuencia guíada de inicialización? Al menos los pasos
// --- Se podría ver de cambiar el área de no detección

void Bluethooth() {
  if(mySerial.available() > 0) {
    char comando = mySerial.read();
    
    switch (comando) {  // a=arriba, b=below, d=derecha, i=izquierda, y=click izquierdo, z=click derecho, c = configuracion
      case 'a':
        Mouse.move(0, -desplazamiento_bt, 0);
        break;
      case 'b':
        Mouse.move(0, desplazamiento_bt, 0);
        break;
      case 'd':
        Mouse.move(desplazamiento_bt, 0, 0);
        break;
      case 'i':
        Mouse.move(-desplazamiento_bt, 0, 0);
        break;
      case 'y':
        Mouse.click(MOUSE_LEFT);
        break;
      case 'z':
        Mouse.click(MOUSE_RIGHT);
        break;
      case 'p':
        if(desplazamiento < 10) {
          desplazamiento++;
          Serial.print("Mouse desplazamiento: ");
          Serial.println(desplazamiento);
        }
        break;
      case 'w':
        if(desplazamiento > 1) {
          desplazamiento--;
          Serial.print("Mouse desplazamiento: ");
          Serial.println(desplazamiento);
        }
        break;
      case 'r':
        if(mouse_threshold_index < MOUSE_THRESHOLD_INDEX_MAX - 1){
          mouse_threshold_index++;
          mouse_time_threshold = MOUSE_TIME_THRESHOLD_ARRAY[mouse_threshold_index];
          Serial.print("Mouse THS: ");
          Serial.println(mouse_time_threshold);
        }
        break;
      case 't':
        if(mouse_threshold_index > 0){
          mouse_threshold_index--;
          mouse_time_threshold = MOUSE_TIME_THRESHOLD_ARRAY[mouse_threshold_index];
          Serial.print("Mouse THS: ");
          Serial.println(mouse_time_threshold);
        }
        break;
     // Cambiar modo (medio innecesario pero bueno)
    }
  }
}

