#include <WiFi.h>
#include <HTTPClient.h>
#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN 5      // SDA (ESP32)
#define RST_PIN 21    // RST
#define LED_PIN 25    // LED connected to pin 25

const char* ssid = "koogs";
const char* password = "unsaypass";
const char* scriptURL = "https://script.google.com/macros/s/AKfycbylzVyiZIAseYWs-BXZvtI0sWFHoDhYgyMdfHgFkCHXj4JnKuhtQpLM1d2SsP1S29dE/exec"; 

MFRC522 mfrc522(SS_PIN, RST_PIN);
String lastScannedID = "";
String lastScannedStudent = "";
bool waitingForAsset = false;

void setup() {
    Serial.begin(115200);
    SPI.begin();
    mfrc522.PCD_Init();
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);  // Ensure LED is OFF at start

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConnected to WiFi!");
    Serial.println("Scan an RFID card...");
}

String urlencode(String str) {
    String encoded = "";
    for (int i = 0; i < str.length(); i++) {
        char c = str.charAt(i);
        if (c == ' ') encoded += "%20";
        else if (c == '&') encoded += "%26";
        else if (c == '=') encoded += "%3D";
        else if (c == '?') encoded += "%3F";
        else encoded += c;
    }
    return encoded;
}

void sendToGoogleSheets(String assetName, String assetID, String studentName, String studentID, String action) {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        String url = String(scriptURL) + "?asset_name=" + urlencode(assetName) 
            + "&asset_id=" + urlencode(assetID)
            + "&student_name=" + urlencode(studentName) 
            + "&student_id=" + urlencode(studentID)
            + "&action=" + urlencode(action);

        Serial.println("Sending data to Google Sheets...");
        http.begin(url);
        int httpCode = http.GET();

        if (httpCode > 0) {
            String response = http.getString();
            Serial.println("Server Response: " + response);
        } else {
            Serial.println("Error sending data");
        }

        http.end();
    } else {
        Serial.println("WiFi not connected!");
    }
}

void loop() {
    if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) return;

    String uidString = "";
    for (byte i = 0; i < mfrc522.uid.size; i++) {
        uidString += String(mfrc522.uid.uidByte[i], HEX);
    }

    Serial.print("Card UID: ");
    Serial.println(uidString);
    digitalWrite(LED_PIN, HIGH);  // Turn on LED when scanning
    delay(1000);                  // Keep it on for 1 second
    digitalWrite(LED_PIN, LOW);   // Turn off LED

    if (uidString.equalsIgnoreCase("c2cf4979")) {
        Serial.println("Student: Mar Luar Igot");
        Serial.println("Department: College of Computer Studies");
        Serial.println("Student ID: 23233299");
        lastScannedStudent = "Mar Luar Igot";
        lastScannedID = "23233299";
        waitingForAsset = true;
    } 
    else if (uidString.equalsIgnoreCase("32e64779")) {
        Serial.println("Student: Keith Vincent Dacoma");
        Serial.println("Department: College of Computer Studies");
        Serial.println("Student ID: 23250871");
        lastScannedStudent = "Keith Vincent Dacoma";
        lastScannedID = "23250871";
        waitingForAsset = true;
    } 
    else if (uidString.equalsIgnoreCase("5a87de80") && waitingForAsset) {  
        Serial.println("Asset: Projector");
        processAssetScan("22", "Projector", lastScannedStudent, lastScannedID);
    } 
    else if (uidString.equalsIgnoreCase("89c3bc16") && waitingForAsset) {  
        Serial.println("Asset: Laptop");
        processAssetScan("23", "Laptop", lastScannedStudent, lastScannedID);
    } 
    else {
        Serial.println("Unknown card.");
    }

    mfrc522.PICC_HaltA();
}

void processAssetScan(String assetNumber, String assetName, String studentName, String studentID) {
    Serial.print("Checking asset status: ");
    Serial.println(assetName);

    String action = "Borrowed";

    digitalWrite(LED_PIN, HIGH);  // Turn on LED
    delay(1000);                  // Keep it on for 1 sec
    digitalWrite(LED_PIN, LOW);   // Turn off LED

    sendToGoogleSheets(assetName, assetNumber, studentName, studentID, action);

    lastScannedID = "";
    waitingForAsset = false;
}
