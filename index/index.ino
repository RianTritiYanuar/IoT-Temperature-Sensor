#include <ESP8266WiFi.h>
#include <Firebase_ESP_Client.h>
#include <addons/TokenHelper.h>
#include <time.h>
#include <ctime>
#include <DHT.h>

// === START VARIABLES DECLARATION === 

// WiFi
const String WIFI_SSID = "INTERNET";
const String WIFI_PASSWORD = "999000000";

// Firebase
const String FIREBASE_DATABASE_URL = "https://operation-ponco-default-rtdb.asia-southeast1.firebasedatabase.app/";
const String FIREBASE_API_KEY = "AIzaSyARyuJ6ZzdrRZljdk3fH8t-BJ0Yh4xsN0k";
const String FIREBASE_USER_EMAIL = "rtritiyanuar@gmail.com";
const String FIREBASE_USER_PASSWORD = "999000000";
String API_RECORDS = "/dht_22/records/";
String API_STATUS = "/dht_22/status/";
FirebaseAuth firebaseAuth;
FirebaseConfig firebaseConfig;
FirebaseData firebaseData;
FirebaseJson firebaseJson;
String uid;

// Internet Time
unsigned long timestamp;
const char* networkTimeProtocolServer = "pool.ntp.org";

// DHT Sensor
float temperature;
float humidity;
const int DHT_PIN = D4;
const int DTH_TYPE = DHT22;
DHT dhtSensor(DHT_PIN, DTH_TYPE);

// === END VARIABLES DECLARATION === 

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  Serial.println("Booted!");
  // Init DHT Begin
  dhtSensor.begin();
  // Init Config Time
  configTime(7, 0, networkTimeProtocolServer);
  // Connect to WiFi
  initWiFi();
  // Init Firebase
  initFirebase();
}

void loop() {
  sendTemperatureAndHumidityData();
}

// For WiFi connection initialization
void initWiFi() {
  // Connecting to WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  // After Connected to Wifi
  Serial.println("Connected to WiFi!");
  Serial.print("Your IP: ");
  Serial.println(WiFi.localIP());
}

// For Firebase authorization initialization
void initFirebase(){
  firebaseConfig.database_url = FIREBASE_DATABASE_URL;
  firebaseConfig.api_key = FIREBASE_API_KEY;
  firebaseAuth.user.email = FIREBASE_USER_EMAIL;
  firebaseAuth.user.password = FIREBASE_USER_PASSWORD;

  Firebase.reconnectWiFi(true);
  firebaseData.setResponseSize(4096);
  firebaseConfig.token_status_callback = tokenStatusCallback;
  firebaseConfig.max_token_generation_retry = 5;
  Firebase.begin(&firebaseConfig, &firebaseAuth);

  while (firebaseAuth.token.uid == ""){
    Serial.println("Getting User Id...");
  }

  uid = firebaseAuth.token.uid.c_str();
  Serial.print("Your Firebase UID: ");
  Serial.println(uid);
}

// Get current timestamp
unsigned long getTime() {
  time_t now;
  while (true) {
    now = time(nullptr);
    if (now > (24 * 3600)) {
      return now;
    }
    Serial.println("Waiting for time synchronization...");
    delay(1000);
  }
  return now;
}

// Get data from DHT sensor
// Has a paramater named sensorDelay (unsigned long).
void getDHTSensorData(unsigned long sensorDelay) {
  delay(sensorDelay);
  temperature = dhtSensor.readTemperature();
  humidity = dhtSensor.readHumidity();
  Serial.print("Temperature: ");
  Serial.println(temperature);
  Serial.print("Humidity: ");
  Serial.println(humidity);
}

// Send DHT sensor data to Firebase
void sendTemperatureAndHumidityData() {
  // Get data from sensor
  getDHTSensorData(60000);
  // Check if any reads failed and exit early (to try again).
  if (isnan(temperature) || isnan(humidity)) {
    // Store data to Firebase
    firebaseJson.set("is_active", false);
    if (Firebase.RTDB.setJSON(&firebaseData, API_STATUS.c_str(), &firebaseJson)) {
      Serial.println("Successfully stored data!");
    } else {
      String errorMessage = firebaseData.errorReason().c_str();
      Serial.print("Failed to store data. Error: ");
      Serial.println(errorMessage);
    }
  } else {
    // Get current time
    timestamp = getTime();
    Serial.print("Timestamp: ");
    Serial.println(timestamp);
    // Store data to Firebase
    firebaseJson.set("is_active", true);
    Serial.println(Firebase.RTDB.setJSON(&firebaseData, API_STATUS.c_str(), &firebaseJson) ? "Successfully store data!" : firebaseData.errorReason().c_str());
    firebaseJson.clear();
    // Store data to Firebase
    const String storingPath = API_RECORDS + timestamp;
    firebaseJson.set("temperature", temperature);
    firebaseJson.set("humidity", humidity);
    firebaseJson.set("created_at", timestamp);
    if (Firebase.RTDB.setJSON(&firebaseData, storingPath.c_str(), &firebaseJson)) {
      Serial.println("Successfully stored data!");
    }
  }
  firebaseJson.clear();
}