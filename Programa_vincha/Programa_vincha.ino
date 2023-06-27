#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include "MPU9250.h"

MPU9250 mpu;

// nrf24
const uint64_t pipeOut = 0xE8E8F0F0E1LL;  //IMPORTANT: The same as in the receiver!!!

/*Create the data struct we will send
  The sizeof this struct should not exceed 32 bytes
  This gives us up to 32 8 bits channals */
RF24 radio(7, 8);  // select  CSN and CE  pins

struct MyData {
  float x;
  float y;
  float z;
};

MyData data;

void setup() {
  Serial.begin(9600);
  
  // Inicio radiotransmisor (TODO ESTO CONFÍO EN QUE ANDA)
  if (!radio.begin()) {
    while (1) {
      Serial.println("ERROR - Radio no encontrada");
      delay(2000);
    }
  }
  Serial.println("Radio encontrada");
  
  radio.setAutoAck(false);
  radio.setDataRate(RF24_250KBPS);
  radio.openWritingPipe(pipeOut);
   
  Wire.begin();
  delay(2000);

  // Inicio sensor MPU9250
  if (!mpu.setup(0x68)) {  // Change to your own address
    while (1) {
      Serial.println("MPU connection failed. Please check your connection with `connection_check` example.");
      delay(5000);
    }
  }
  Serial.println("MPU connection succeded");

  Serial.println("En 5 seg comenzara la calibracion");
  delay(5000);

  Serial.println("Calibrando...");
  mpu.calibrateAccelGyro();
  mpu.calibrateMag();

  mpu.setMagneticDeclination(-8.5); // El valor depende de la ciudad
  Serial.println("Calibracion terminada. Comenzando programa");
}

void loop() {
  
  if(mpu.update()){
    getRollPitchYaw();
//    printValores();
  }
}

void getRollPitchYaw() {
  data.x = mpu.getRoll();
  data.y = mpu.getPitch();
  data.z = mpu.getYaw();

  radio.write(&data, sizeof(MyData));
}

void printValores() {
  //Mostrar los angulos separadas
  Serial.print("X ");
  Serial.print(data.x);
  Serial.print(" | Y ");
  Serial.print(data.y);
  Serial.print(" | Z ");
  Serial.println(data.z);
}
