#define BLYNK_TEMPLATE_ID "-----------------"
#define BLYNK_TEMPLATE_NAME "-------------"
#define BLYNK_AUTH_TOKEN "--------------"

// --------------------- Libraries ---------------------
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <DFRobot_ID809.h>

// --------------------- WiFi Config ---------------------
char ssid[] = "------------";
char pass[] = "------------";

// --------------------- Blynk Config ---------------------
#define BLYNK_RELAY_VPIN V1
#define BLYNK_ADD_VPIN   V2
#define BLYNK_DEL_VPIN   V3

// --------------------- Hardware Pins ---------------------
#define ADD_BUTTON     8
#define DEL_BUTTON     6
#define RELAY_PIN      18
#define INSIDE_BUTTON  7

// --------------------- Objects ---------------------
DFRobot_ID809 fingerprint;
HardwareSerial FingerSerial(1); // RX=3, TX=2
bool sensorConnected = false;   // 🔹 Track sensor status

// ===================================================
// ===================== SETUP =======================
// ===================================================
void setup() {
  Serial.begin(115200);
  pinMode(ADD_BUTTON, INPUT_PULLUP);
  pinMode(DEL_BUTTON, INPUT_PULLUP);
  pinMode(INSIDE_BUTTON, INPUT_PULLUP);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  // --- Initialize Fingerprint Sensor ---
  FingerSerial.begin(115200, SERIAL_8N1, 3, 2);
  fingerprint.begin(FingerSerial);

  Serial.println("🔹 Fingerprint System Initializing...");
  if (fingerprint.isConnected()) {
    sensorConnected = true;
    Serial.println("✅ Fingerprint sensor connected!");
  } else {
    sensorConnected = false;
    Serial.println("⚠️ Fingerprint sensor NOT detected. Running without it.");
  }

  // --- Connect to WiFi ---
  WiFi.begin(ssid, pass);
  Serial.print("📶 Connecting to WiFi");
  int wifiTimeout = 0;
  while (WiFi.status() != WL_CONNECTED && wifiTimeout < 20) {
    delay(500);
    Serial.print(".");
    wifiTimeout++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi Connected!");
    Blynk.config(BLYNK_AUTH_TOKEN);
    Blynk.connect(10000);
    if (Blynk.connected()) {
      Serial.println("✅ Connected to Blynk!");
    } else {
      Serial.println("⚠️ Blynk server not reachable — offline mode.");
    }
  } else {
    Serial.println("\n⚠️ WiFi not connected — running offline.");
  }
}

// ===================================================
// ===================== MAIN LOOP ===================
// ===================================================
void loop() {
  if (WiFi.status() == WL_CONNECTED)
    Blynk.run();

  // 🔁 Auto recheck sensor connection every 5 seconds
  static unsigned long lastCheck = 0;
  if (millis() - lastCheck > 5000) {
    lastCheck = millis();

    if (!sensorConnected && fingerprint.isConnected()) {
      sensorConnected = true;
      Serial.println("✅ Fingerprint sensor reconnected!");
    } 
    else if (sensorConnected && !fingerprint.isConnected()) {
      sensorConnected = false;
      Serial.println("⚠️ Fingerprint sensor disconnected!");
    }
  }

  // --- Add Fingerprint Button ---
  if (digitalRead(ADD_BUTTON) == LOW) {
    delay(200);
    addFingerprint();
  }

  // --- Delete Fingerprint Button ---
  if (digitalRead(DEL_BUTTON) == LOW) {
    delay(200);
    deleteSpecificFingerprint();
  }

  // --- Physical Door Open Button ---
  if (digitalRead(INSIDE_BUTTON) == LOW) {
    delay(200);
    unlockDoor();
  }

  // --- Check Finger for Access ---
  if (sensorConnected) {
    if (fingerprint.detectFinger()) {
      if (fingerprint.collectionFingerprint(10) == 0) {
        uint16_t fingerID = fingerprint.search();
        if (fingerID != 0 && fingerID != 0xFFFF) {
          Serial.print("✅ Access Granted! ID: ");
          Serial.println(fingerID);

          fingerprint.ctrlLED(fingerprint.eKeepsOn, fingerprint.eLEDGreen, 0);
          unlockDoor();
          delay(1000);
          fingerprint.ctrlLED(fingerprint.eNormalClose, fingerprint.eLEDGreen, 0);
        } else {
          Serial.println("❌ Access Denied! No match found.");
          fingerprint.ctrlLED(fingerprint.eFastBlink, fingerprint.eLEDRed, 3);
          delay(1000);
          fingerprint.ctrlLED(fingerprint.eNormalClose, fingerprint.eLEDRed, 0);
        }
      } else {
        Serial.println("⚠️ Failed to capture fingerprint.");
        fingerprint.ctrlLED(fingerprint.eFastBlink, fingerprint.eLEDWhite, 2);
      }
      delay(1000);
    }
  }
}

// ===================================================
// ============== FUNCTION DEFINITIONS ===============
// ===================================================

BLYNK_WRITE(BLYNK_RELAY_VPIN) {
  int relayState = param.asInt();
  digitalWrite(RELAY_PIN, relayState ? HIGH : LOW);
  Serial.print("📱 Relay from Blynk: ");
  Serial.println(relayState ? "ON" : "OFF");
}

BLYNK_WRITE(BLYNK_ADD_VPIN) {
  if (param.asInt() == 1) {
    Serial.println("📲 Add Fingerprint Command from Blynk");
    addFingerprint();
    Blynk.virtualWrite(BLYNK_ADD_VPIN, 0);
  }
}

BLYNK_WRITE(BLYNK_DEL_VPIN) {
  if (param.asInt() == 1) {
    Serial.println("📲 Delete Fingerprint Command from Blynk");
    deleteSpecificFingerprint();
    Blynk.virtualWrite(BLYNK_DEL_VPIN, 0);
  }
}

void addFingerprint() {
  if (!sensorConnected) {
    Serial.println("⚠️ Fingerprint sensor not connected — cannot add!");
    return;
  }

  Serial.println("🟢 Add Fingerprint Mode Started");
  uint8_t emptyID = fingerprint.getEmptyID();
  if (emptyID == ERR_ID809) {
    Serial.println("❌ No empty ID available!");
    return;
  }

  Serial.print("➡️ New ID to store: ");
  Serial.println(emptyID);
  uint8_t i = 0;
  const uint8_t COLLECT_COUNT = 3;

  while (i < COLLECT_COUNT) {
    fingerprint.ctrlLED(fingerprint.eBreathing, fingerprint.eLEDBlue, 0);
    Serial.printf("Place finger %d\n", i + 1);
    if (fingerprint.collectionFingerprint(10) == 0) {
      Serial.println("Captured successfully. Remove finger...");
      fingerprint.ctrlLED(fingerprint.eFastBlink, fingerprint.eLEDYellow, 3);
      i++;
      while (fingerprint.detectFinger());
      delay(800);
    } else {
      Serial.println("❌ Capture failed, try again.");
    }
  }

  if (fingerprint.storeFingerprint(emptyID) == 0) {
    Serial.printf("✅ Finger stored with ID %d\n", emptyID);
    fingerprint.ctrlLED(fingerprint.eKeepsOn, fingerprint.eLEDGreen, 0);
    delay(1000);
  } else {
    Serial.println("❌ Failed to store fingerprint!");
    fingerprint.ctrlLED(fingerprint.eKeepsOn, fingerprint.eLEDRed, 0);
    delay(1000);
  }
  fingerprint.ctrlLED(fingerprint.eNormalClose, fingerprint.eLEDGreen, 0);
}

void deleteSpecificFingerprint() {
  if (!sensorConnected) {
    Serial.println("⚠️ Fingerprint sensor not connected — cannot delete!");
    return;
  }

  Serial.println("🟥 Delete Specific Fingerprint Mode");
  fingerprint.ctrlLED(fingerprint.eBreathing, fingerprint.eLEDYellow, 0);
  Serial.println("➡️ Place the finger to remove...");

  unsigned long startTime = millis();
  while (!fingerprint.detectFinger()) {
    if (millis() - startTime > 10000) {
      Serial.println("⏱️ Timeout — no finger detected.");
      fingerprint.ctrlLED(fingerprint.eNormalClose, fingerprint.eLEDGreen, 0);
      return;
    }
  }

  if (fingerprint.collectionFingerprint(10) == 0) {
    uint16_t fingerID = fingerprint.search();
    if (fingerID != 0 && fingerID != 0xFFFF) {
      Serial.print("🧭 Found Finger ID: ");
      Serial.println(fingerID);
      if (fingerprint.delFingerprint(fingerID) == 0) {
        Serial.println("✅ Fingerprint deleted successfully!");
        fingerprint.ctrlLED(fingerprint.eFastBlink, fingerprint.eLEDRed, 3);
      } else {
        Serial.println("❌ Failed to delete fingerprint!");
      }
    } else {
      Serial.println("❌ No matching fingerprint found to delete.");
    }
  } else {
    Serial.println("⚠️ Failed to capture fingerprint.");
  }

  fingerprint.ctrlLED(fingerprint.eNormalClose, fingerprint.eLEDGreen, 0);
}

void unlockDoor() {
  digitalWrite(RELAY_PIN, HIGH);
  if (WiFi.status() == WL_CONNECTED)
    Blynk.virtualWrite(BLYNK_RELAY_VPIN, 1);
  Serial.println("🔓 Door Unlocked!");
  delay(3000);
  digitalWrite(RELAY_PIN, LOW);
  if (WiFi.status() == WL_CONNECTED)
    Blynk.virtualWrite(BLYNK_RELAY_VPIN, 0);
  Serial.println("🔒 Door Locked!");
}