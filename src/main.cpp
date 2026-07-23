#define ENABLE_USER_AUTH
#define ENABLE_DATABASE

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <FirebaseClient.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <ESP32Servo.h>

// Network and Firebase credentials
#define WIFI_SSID "GTR"
#define WIFI_PASSWORD "syifasipasip"

#define Web_API_KEY "AIzaSyDE4jEkz7tPQaqCbqy0ZmeojHYJCLAdPDI"
#define DATABASE_URL "https://esp-project1-28c0f-default-rtdb.asia-southeast1.firebasedatabase.app"
#define USER_EMAIL "tester1@gmai.com"
#define USER_PASS "tester111"

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

// User function
void processData(AsyncResult &aResult);

// Authentication
UserAuth user_auth(Web_API_KEY, USER_EMAIL, USER_PASS);

// Firebase components
FirebaseApp app;
WiFiClientSecure ssl_client;
using AsyncClient = AsyncClientClass;
AsyncClient aClient(ssl_client);
RealtimeDatabase Database;

// Timer variables for sending data every 10 seconds
unsigned long lastSendTime = 0;
float lastSentAngle = 0; // Menyimpan nilai sudut yang terakhir sukses dikirim
const unsigned long sendInterval = 500; // 10 seconds in milliseconds

// Variables to send to the database
int intValue = 0;
float floatValue = 0.01;
String stringValue = "";

void setup(){
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


  // Connect to Wi-Fi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  
  // Configure SSL client
  ssl_client.setInsecure();
  // ssl_client.setConnectionTimeout(1000);
  ssl_client.setHandshakeTimeout(5);
  
  // Initialize Firebase
  initializeApp(aClient, app, getAuth(user_auth), processData, "🔐 authTask");
  app.getApp<RealtimeDatabase>(Database);
  Database.url(DATABASE_URL);

  Serial.println("");
  delay(100);
}

void loop(){
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

  // Maintain authentication and async tasks
  app.loop();
  // Check if authentication is ready
  if (app.ready()){ 
    // Periodic data sending every 10 seconds
    unsigned long currentTime = millis();
    if (currentTime - lastSendTime >= sendInterval){
      float selisihSudut = abs(previousAngle - lastSentAngle);
      if(selisihSudut > 2.0){
        lastSentAngle = previousAngle;
        // Update the last send time
        lastSendTime = currentTime;
        
        Database.set<float>(aClient, "/sensor/mpu6050", previousAngle, processData, "Update_MPU6050_Angle");
        Database.set<int>(aClient, "/sensor/servo", posisiServo, processData, "Update_Servo_Position");
      }
    }
  }
}

void processData(AsyncResult &aResult) {
  if (!aResult.isResult())
    return;

  if (aResult.isEvent())
    Firebase.printf("Event task: %s, msg: %s, code: %d\n", aResult.uid().c_str(), aResult.eventLog().message().c_str(), aResult.eventLog().code());

  if (aResult.isDebug())
    Firebase.printf("Debug task: %s, msg: %s\n", aResult.uid().c_str(), aResult.debug().c_str());

  if (aResult.isError())
    Firebase.printf("Error task: %s, msg: %s, code: %d\n", aResult.uid().c_str(), aResult.error().message().c_str(), aResult.error().code());

  if (aResult.available())
    Firebase.printf("task: %s, payload: %s\n", aResult.uid().c_str(), aResult.c_str());
}