#define ENABLE_USER_AUTH
#define ENABLE_DATABASE

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <FirebaseClient.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADS1X15.h>

SemaphoreHandle_t xVibrationMutex;
SemaphoreHandle_t xCurrentMutex;
SemaphoreHandle_t xI2CMutex;

// Network and Firebase credentials
#define WIFI_SSID "GTR"
#define WIFI_PASSWORD "syifasipasip"

#define Web_API_KEY "AIzaSyDE4jEkz7tPQaqCbqy0ZmeojHYJCLAdPDI"
#define DATABASE_URL "https://esp-project1-28c0f-default-rtdb.asia-southeast1.firebasedatabase.app"
#define USER_EMAIL "tester1@gmai.com"
#define USER_PASS "tester111"

#define I2c_SDA 17
#define I2c_SCL 18

Adafruit_ADS1115 ads;
Adafruit_MPU6050 mpu;

unsigned long currentMillis = 0;

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

// --- Variabel Waktu & Sampling RMS ---
unsigned long lastSampleTime = 0;
const unsigned long sampleInterval = 10; // Ambil data sensor tiap 10 ms (100 Hz)

unsigned long lastCalcTime = 0;
const unsigned long calcInterval = 1000; // Hitung & kirim RMS tiap 1 detik

float sumSqX = 0.0;
float sumSqY = 0.0;
float sumSqZ = 0.0;
int sampleCount = 0;

float arusRMS_Global = 0.0;

// Timer variables for sending data every 10 seconds
unsigned long lastSendTime = 0;
float lastSentAngle = 0; // Menyimpan nilai sudut yang terakhir sukses dikirim
const unsigned long sendInterval = 500; // 10 seconds in milliseconds

TaskHandle_t TaskMPU;
TaskHandle_t TaskFirebase;

void TaskCurrentCode(void *pvParameters){
  for(;;){
    float maxTegangan = 0.0;
    float minTegangan = 5.0; // Nilai referensi maksimal 5V
    
    unsigned long startMillis = millis();
    
    // Sampling gelombang selama 20ms (1 siklus AC 50Hz)
    while (millis() - startMillis < 20) {
      int16_t adc0 = 0;
      if(xSemaphoreTake(xI2CMutex, pdMS_TO_TICKS(5)) == pdTRUE){
      adc0 = ads.readADC_SingleEnded(0);
      xSemaphoreGive(xI2CMutex);
  }
      float tegangan = (adc0 * 0.125) / 1000.0; // Konversi ke Volt
      
      if (tegangan > maxTegangan) maxTegangan = tegangan;
      if (tegangan < minTegangan) minTegangan = tegangan;
    }
    
    // Kalkulasi RMS
    float teganganPeakToPeak = maxTegangan - minTegangan;
    float teganganRMS = (teganganPeakToPeak / 2.0) * 0.707;
    
    // 185.0 adalah sensitivitas untuk ACS712 versi 5A. 
    // (Ubah ke 100.0 untuk versi 20A, atau 66.0 untuk versi 30A)
    float arusRMS = (teganganRMS * 1000.0) / 185.0; 
    
    // Simpan ke variabel global dengan aman menggunakan Mutex
    if (xSemaphoreTake(xCurrentMutex, portMAX_DELAY) == pdTRUE) {
      arusRMS_Global = arusRMS;
      xSemaphoreGive(xCurrentMutex);
    }
    
    // Beri jeda 1 detik sebelum membaca arus lagi
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void TaskMPUCode(void *pvParameters){
  for(;;){
    unsigned long currentMillis = millis();
    if (currentMillis - lastSampleTime >= sampleInterval){
      lastSampleTime = currentMillis;

      sensors_event_t a, g, temp;
      bool ok = false;

      // Satu-satunya pemanggilan I2C, dilindungi mutex
      if(xSemaphoreTake(xI2CMutex, pdMS_TO_TICKS(10)) == pdTRUE){
        ok = mpu.getEvent(&a, &g, &temp);
        xSemaphoreGive(xI2CMutex);
      }

      if(ok){
        float ax = a.acceleration.x;
        float ay = a.acceleration.y;
        float az = a.acceleration.z - 9.81; // Mengurangi gaya gravitasi

        if(xSemaphoreTake(xVibrationMutex, pdMS_TO_TICKS(10)) == pdTRUE){
          sumSqX += (ax * ax);
          sumSqY += (ay * ay);
          sumSqZ += (az * az);
          sampleCount++;
          xSemaphoreGive(xVibrationMutex);
        }
      }
    }
    vTaskDelay(2/portTICK_PERIOD_MS);
  }
}

void TaskFirebaseCode(void *pvParameters){
  for(;;){
    app.loop();
    unsigned long currentMillis = millis();

    if (currentMillis - lastCalcTime >= calcInterval){
      lastCalcTime = currentMillis;

      float localSumX = 0, localSumZ = 0;
      int localCount = 0;

      if (xSemaphoreTake(xVibrationMutex, portMAX_DELAY) == pdTRUE) {
        localSumX = sumSqX;
        localSumZ = sumSqZ;
        localCount = sampleCount;

        sumSqX = 0;
        sumSqY = 0;
        sumSqZ = 0;
        sampleCount = 0;

        // 6. LEPASKAN MUTEX
        xSemaphoreGive(xVibrationMutex); 
      }

      if(localCount > 0){
        float localArus = 0;
        if (xSemaphoreTake(xCurrentMutex, portMAX_DELAY) == pdTRUE) {
          localArus = arusRMS_Global;
          xSemaphoreGive(xCurrentMutex);
        }
        float rmsX = sqrt(localSumX / localCount);
        float rmsZ = sqrt(localSumZ / localCount);

        Serial.print("RMS Getaran -> X: "); Serial.print(rmsX, 4);
        Serial.print(" | Z: "); Serial.println(rmsZ, 4);
        Serial.print(" || Arus RMS: "); Serial.println(localArus, 3);

        if (app.ready()){
          Database.set<float>(aClient, "/motor/vibrasi/rms_x", rmsX, processData, "Send_RMS_X");
          Database.set<float>(aClient, "/motor/vibrasi/rms_z", rmsZ, processData, "Send_RMS_Z");
          Database.set<float>(aClient, "/motor/arus/rms", localArus, processData, "Send_Arus");
        }
      }
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}


void setup(){
  Wire.begin(I2c_SDA, I2c_SCL);
  Serial.begin(115200);


  Serial.println("Adafruit MPU6050 test!");

  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) {
      delay(10);
    }
  }
  Serial.println("MPU6050 Found!");

  mpu.setAccelerometerRange(MPU6050_RANGE_4_G); // Sangat sensitif untuk kemiringan halus
  mpu.setFilterBandwidth(MPU6050_BAND_44_HZ);   // Menyaring getaran agar servo tidak gemetar


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

  xVibrationMutex = xSemaphoreCreateMutex();
  if(xVibrationMutex == NULL){
    Serial.println("Failed to create mutex");
    ESP.restart(); // Restart the ESP if mutex creation fails
  }

  xCurrentMutex = xSemaphoreCreateMutex();
  if(xCurrentMutex == NULL){
    Serial.println("Failed to create current mutex");
    ESP.restart(); 
  }

  xI2CMutex = xSemaphoreCreateMutex();
  if(xI2CMutex == NULL){
    Serial.println("Failed to create I2C mutex");
    ESP.restart();
  }

  if (!ads.begin(0x48)) {
    Serial.println("Gagal menemukan ADS1115!");
  } else {
    ads.setGain(GAIN_ONE); // Set rentang bacaan ke +/- 4.096V
    Serial.println("ADS1115 Siap!");
  }

  xTaskCreatePinnedToCore(
    TaskCurrentCode, 
    "TaskCurrent",   
    4096,            
    NULL,            
    1, // Prioritas sama dengan Firebase, di bawah MPU6050            
    NULL,      
    0
  );

  xTaskCreatePinnedToCore(
    TaskMPUCode,   // Function to implement the task
    "TaskMPU",     // Name of the task
    4096,          // Stack size in words
    NULL,          // Task input parameter
    1,             // Priority of the task
    &TaskMPU,      // Task handle.
    1              // Core where the task should run
  );

  xTaskCreatePinnedToCore(
    TaskFirebaseCode, // Function to implement the task
    "TaskFirebase",   // Name of the task
    16384,             // Stack size in words
    NULL,             // Task input parameter
    1,                // Priority of the task
    &TaskFirebase,    // Task handle.
    0                 // Core where the task should run
  );

  Serial.println("");
  delay(100);
}

void loop(){
  vTaskDelete(NULL);
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