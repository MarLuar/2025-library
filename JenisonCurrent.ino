#define BLYNK_TEMPLATE_ID "TMPL6IS9XnOZf"
#define BLYNK_TEMPLATE_NAME "Energy Monitoring System"
#define BLYNK_AUTH_TOKEN "S-1vdRwHuxtae2cMRKg2RqZDftI_I325"

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include <EmonLib.h>
#include <LiquidCrystal_I2C.h>
#include <HardwareSerial.h>
#include <ESP32Servo.h>  // Use ESP32Servo instead of Servo

char ssid[] = "koogs";
char pass[] = "unsaypass";
int readingCount;
#define LCD_I2C_ADDRESS 0x27
LiquidCrystal_I2C lcd(LCD_I2C_ADDRESS, 16, 2);

EnergyMonitor emon;
const int sensorPin = 34;
const double voltage = 220.0;
double energyWh = 0.0;
double ratePerKWh = 0.0;
unsigned long lastTime = 0;
bool smsSent = false; // Flag to prevent multiple SMS
unsigned long startupTime = 0;


// SIM800L Serial Setup
HardwareSerial sim800l(1);
#define SIM800_TX 17
#define SIM800_RX 16

// Servo Motor Setup
Servo myServo;
const int servoPin = 33;  // Pin where the servo is connected

// Variables to store previous readings
double prevIrms = 0.0;
double prevPower = 0.0;
double prevEnergyKWh = 0.0;
double prevEstimatedBill = 0.0;

// Function to send an SMS
void sendSMS(String number, String message) {
  Serial.println("Sending SMS...");
  sim800l.println("AT+CMGF=1");  // Set SMS mode to text
  delay(1000);
  sim800l.print("AT+CMGS=\"");
  sim800l.print(number);
  sim800l.println("\"");
  delay(1000);
  sim800l.print(message);
  delay(1000);
  sim800l.write(26);  // End message with Ctrl+Z (ASCII 26)
  Serial.println("SMS Sent!");
}

// Function to handle incoming data from V4 (rate per kWh)
BLYNK_WRITE(V4) { 
  String rateString = param.asStr();
  ratePerKWh = rateString.toDouble();
}


double getStableIrms() {
  double sum = 0;
  int samples = 10;  // Take 10 samples for averaging

  for (int i = 0; i < samples; i++) {
    double reading = emon.calcIrms(1480);
    
    // Ignore extreme spikes at startup (e.g., above 2.5A)
    if (millis() < 5000 && reading > 2.5) {  
      reading = 0;
    }

    sum += reading;
    delay(50); // Small delay to stabilize readings
  }

  double avgIrms = sum / samples;

  // Ignore sudden spikes above 1.0A from the previous reading
  if (abs(avgIrms - prevIrms) > 1.0) {
    return prevIrms;  // Ignore spike, keep previous stable value
  }

  return avgIrms;
}

// Function to send data to Blynk only when there are changes
void sendDataToBlynk() {
  // Ignore readings for the first 3 seconds after startup
  if (millis() - startupTime < 3000) {
    Serial.println("Ignoring startup readings...");
    return;
  }

  double Irms = emon.calcIrms(1480);
  if (Irms < 0.07) Irms = 0;

  unsigned long currentTime = millis();
  unsigned long timeElapsed = currentTime - lastTime;
  lastTime = currentTime;

  double timeElapsedHours = timeElapsed / 3600000.0;
  energyWh += (voltage * Irms) * timeElapsedHours;
  double energyKWh = energyWh / 1000.0;
  double estimatedBill = energyKWh * ratePerKWh;

  // Only update Blynk if there are significant changes
  if ((abs(Irms - prevIrms) > 0.05) ||
      (abs(energyKWh - prevEnergyKWh) > 0.002) ||
      (abs(estimatedBill - prevEstimatedBill) > 0.02)) {

    Blynk.virtualWrite(V0, Irms);
    Blynk.virtualWrite(V2, String(energyKWh, 4));
    Blynk.virtualWrite(V3, String(estimatedBill, 2));

    // Update previous readings
    prevIrms = Irms;
    prevEnergyKWh = energyKWh;
    prevEstimatedBill = estimatedBill;
  }

  // Send SMS when energy reaches 1 kWh
  if (energyKWh >= 1.0 && !smsSent) {
    sendSMS("+639923380007", "Alert: Energy consumption reached 1 kWh! Shutting down electricity in 15 mins.");
    smsSent = true;
  }

  // Control Servo Motor based on energy consumption
  if (energyKWh >= 1.0) {
    myServo.write(180);
  } else {
    myServo.write(0);
  }
}


// Function to update LCD constantly
void updateLCD() {
  double Irms = emon.calcIrms(1480);
  if (Irms < 0.07) Irms = 0;

  double power = voltage * Irms;
  double energyKWh = energyWh / 1000.0;

  // Display on LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Current: ");
  lcd.print(Irms);
  lcd.print(" A");
  
  lcd.setCursor(0, 1);
  lcd.print("kWh: ");
  lcd.print(energyKWh, 4);
}

void setup() {
  Serial.begin(115200);
  
  // Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Energy Monitor");
  delay(2000);
  
  // Initialize Current Sensor
  emon.current(sensorPin, 1.2);
  
  // Initialize SIM800L
  sim800l.begin(9600, SERIAL_8N1, SIM800_RX, SIM800_TX);
  delay(1000);
  Serial.println("Initializing SIM800L...");
  
  // Initialize Servo Motor
  myServo.attach(servoPin);
  myServo.write(0);  // Set servo to 0 degrees initially
  
  // Connect to WiFi & Blynk
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  // Set the startup timestamp
  startupTime = millis();
}

void loop() {
  Blynk.run();

  // Continuously check for changes and update Blynk
  sendDataToBlynk();

  // Update LCD constantly
  updateLCD();
  delay(500); // Small delay to avoid flickering
}