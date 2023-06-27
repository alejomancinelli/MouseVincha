#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include "MPU9250.h"

MPU9250 mpu;
int yaw,pitch,roll;
String valores;




// nrf24
const uint64_t pipeOut = 0xE8E8F0F0E1LL;  //IMPORTANT: The same as in the receiver!!!
////////////////////////////////////////////////////////

/*Create the data struct we will send
  The sizeof this struct should not exceed 32 bytes
  This gives us up to 32 8 bits channals */
RF24 radio(7, 8);  // select  CSN and CE  pins

struct MyData {
  float angulo_x;
  float angulo_y;
  float angulo_z;
};
MyData data;


void setup() {
  Serial.begin(9600);
  if (!radio.begin()) {
    Serial.println("Radio not found amigo");
    while (1) {
      delay(10);
    }
  } else {
    Serial.println("The radio espantoso is the best amigo");
  }
  radio.setAutoAck(false);
  radio.setDataRate(RF24_250KBPS);
  radio.openWritingPipe(pipeOut);
   
    Wire.begin();
    delay(2000);

    if (!mpu.setup(0x68)) {  // change to your own address
        while (1) {
            Serial.println("MPU connection failed. Please check your connection with `connection_check` example.");
            delay(5000);
        }
    }
    mpu.setMagneticDeclination(-8.5);


}

void loop() {
    if (mpu.update()) {
        static uint32_t prev_ms = millis();
        if (millis() > prev_ms + 25) {
            print_roll_pitch_yaw();
            prev_ms = millis();
        }
    }
    print_roll_pitch_yaw();
  senddata();
}

void print_roll_pitch_yaw() {
   yaw = mpu.getYaw() - 68;
   pitch = mpu.getPitch();
   roll = mpu.getRoll() ;

   valores = "90.     " + String(yaw) + "           " + String(pitch) + "              " + String(roll) + "          -90";

   Serial.println(valores);

}


void senddata() {
  data.angulo_x = roll;
  data.angulo_y = pitch;
  data.angulo_z = yaw;

  radio.write(&data, sizeof(MyData));
}
