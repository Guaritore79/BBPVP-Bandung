#include <Arduino.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <ESP32Servo.h>

#define SERVO_PIN 16
#define I2c_SDA 17
#define I2c_SCL 18
#define LED 15

Servo myservo;
Adafruit_MPU6050 mpu;

unsigned long previousMillis = 0;
unsigned long currentMillis = 0;
float elapsedTime = 0;
float previousAngle = 0;

void setup(void) {
  Wire.begin(I2c_SDA, I2c_SCL);
  Serial.begin(115200);
  pinMode(LED, OUTPUT);

  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  myservo.setPeriodHertz(50); 
  myservo.attach(SERVO_PIN, 500, 2400); 

  while (!Serial)
    delay(10); // will pause Zero, Leonardo, etc until serial console opens

  Serial.println("Adafruit MPU6050 test!");

  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) {
      delay(10);
    }
  }
  Serial.println("MPU6050 Found!");

  mpu.setAccelerometerRange(MPU6050_RANGE_2_G); // Sangat sensitif untuk kemiringan halus
  mpu.setGyroRange(MPU6050_RANGE_250_DEG);      // Akurat untuk putaran lambat/sedang
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);   // Menyaring getaran agar servo tidak gemetar

  Serial.println("");
  delay(100);
}

void loop() {
  /* Get new sensor events with the readings */
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  previousMillis = currentMillis;
  currentMillis = millis();
  elapsedTime = (currentMillis - previousMillis) / 1000.0;

  if (elapsedTime > 0.1 || elapsedTime < 0) {
    elapsedTime = 0.1; 
  }

  float sudutAkselerometerY = atan2(a.acceleration.x, a.acceleration.z) * 180.0 / PI;
  float kecepatanGyroY = -g.gyro.y * 180.0 / PI;
  previousAngle = 0.98 * (previousAngle + (kecepatanGyroY * elapsedTime)) + 0.02 * sudutAkselerometerY;
  int posisiServo = map((int)previousAngle, -45, 45, 0, 180);
  posisiServo = constrain(posisiServo, 0  , 180);
  myservo.write(posisiServo);

  if(previousAngle > 45 || previousAngle < -45){
    digitalWrite(LED, HIGH);
  } else {
    digitalWrite(LED, LOW);
  }

  Serial.print("Sudut MPU: ");
  Serial.print(previousAngle);
  Serial.print(" deg | Posisi Servo: ");
  Serial.println(posisiServo);

  delay(15);
}
