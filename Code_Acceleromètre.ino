#include <MPU6050Raw.h>

#define SDA GPIO_NUM_21
#define SCL GPIO_NUM_22
MPU6050 mpu;



void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);  
  mpu.init(SDA,SCL);

}

void loop() {
  // put your main code here, to run repeatedly:
  mpu.readAccel();
  Serial.println(String("accel_x: ")+String(mpu.getAccelX())+String(", accel_y: ")+String(mpu.getAccelY())+String(", accel_z: ")+String(mpu.getAccelZ()));
  //mpu.readGyro();
  //Serial.println(String("gyro_x: ")+String(mpu.getGyroX())+String(", gyro_y: ")+String(mpu.getGyroY())+String(", gryo_z: ")+String(mpu.getGyroZ()));
  delay(1000);
}
