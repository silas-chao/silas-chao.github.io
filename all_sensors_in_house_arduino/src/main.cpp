#include <Wire.h>
#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <WebServer.h>

// Setup variables
int picked = 0;

struct vehicle {
    String brand;
    String model;
    float weight;
    int LPlate;
    int maxCapacity;
};

struct person {
    float weight;
    float height;
    int age;
};

struct worldCoords {
    double y; // latitude
    double x; // longitude
    double z; // altitude

    void update(double newY, double newX, double newZ) {
        y = newY;
        x = newX;
        z = newZ;
    }
};

worldCoords locus;

// All vehicles and people possible
vehicle vehicles[10];
person people[25];

// Current vehicle
vehicle currentCar = vehicles[picked];

// Function prototypes
void something();

// Define GPIOs
#define YELLOW_LED_PIN 12
#define RGB_LED_PIN 16
#define BUZZER_PIN 23
#define LEFT_BUTTON_PIN 26
#define RIGHT_BUTTON_PIN 27

// I2C Configuration
#define I2C_MASTER_SCL_IO 22
#define I2C_MASTER_SDA_IO 21
#define I2C_MASTER_FREQ_HZ 100000

// LCD Configuration
#define LCD_ADDRESS 0x27
#define LCD_COLUMNS 16
#define LCD_ROWS 2
LiquidCrystal_I2C lcd(LCD_ADDRESS, LCD_COLUMNS, LCD_ROWS);

// Wi-Fi Configuration
const char* ssid = "GitCommited";
const char* password = "GitoutGitoutGitoutofmyhead";

WebServer server(80);

// State variables
String lcdContent = "";
bool yellowLedState = false;

// Function to initialize I2C
void i2c_master_init() {
    Wire.begin(I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO);
    Wire.setClock(I2C_MASTER_FREQ_HZ);
    Serial.println("I2C initialized successfully");
}

// Function to initialize LCD
void lcd_init() {
    lcd.init();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcdContent = "Hello, World!";
    lcd.print(lcdContent);
}

// Function to handle button press events
void handle_button_press() {
    bool leftButtonPressed = digitalRead(LEFT_BUTTON_PIN) == LOW;
    bool rightButtonPressed = digitalRead(RIGHT_BUTTON_PIN) == LOW;

    if (leftButtonPressed || rightButtonPressed) {
        yellowLedState = true;
        digitalWrite(YELLOW_LED_PIN, HIGH); // Turn on the yellow LED
        lcd.clear();
        if (leftButtonPressed) {
            lcdContent = "Left Button";
            lcd.setCursor(0, 0);
            lcd.print(lcdContent);
            Serial.println("Left button pressed");
        } else if (rightButtonPressed) {
            lcdContent = "Right Button";
            lcd.setCursor(0, 0);
            lcd.print(lcdContent);
            Serial.println("Right button pressed");
        }
    } else {
        yellowLedState = false;
        digitalWrite(YELLOW_LED_PIN, LOW); // Turn off the yellow LED
        Serial.println("No button pressed");
    }
}

// I2C Scanner
void i2c_scanner() {
    Serial.println("Scanning I2C devices...");
    for (uint8_t addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {
            Serial.print("Found I2C device at address: 0x");
            Serial.println(addr, HEX);
        }
    }
    Serial.println("I2C scan complete.");
}

// HTML handler function
void handleRoot() {
    String html = "<!DOCTYPE html><html lang='en'>";
    html += "<head>";
    html += "<meta charset='UTF-8'>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
    html += "<meta http-equiv='refresh' content='1'>";
    html += "<title>Device Status</title>";
    html += "<style>";
    html += "body { background: linear-gradient(135deg, #1e3c72, #2a5298); color: white; font-family: Arial, sans-serif; text-align: center; padding: 20px; }";
    html += ".container { max-width: 400px; margin: auto; background: rgba(255, 255, 255, 0.2); padding: 20px; border-radius: 10px; box-shadow: 0px 4px 6px rgba(0,0,0,0.1); }";
    html += "h1 { font-size: 24px; margin-bottom: 20px; }";
    html += "p { font-size: 18px; margin: 10px 0; }";
    html += ".status { display: inline-block; padding: 6px 12px; border-radius: 6px; font-weight: bold; }";
    html += ".on { background: #00ff00; color: black; }";
    html += ".off { background: #ff0000; color: white; }";
    html += "</style>";
    html += "</head>";
    html += "<body>";
    html += "<div class='container'>";
    html += "<h1>Device Status</h1>";
    html += "<p><strong>LCD Display:</strong> " + lcdContent + "</p>";
    html += "<p><strong>Yellow LED State:</strong> <span class='status " + String(yellowLedState ? "on" : "off") + "'>" + String(yellowLedState ? "ON" : "OFF") + "</span></p>";
    html += "</div>";
    html += "</body></html>";
    
    server.send(200, "text/html", html);
}

void setup() {
    // Initialize Serial for debugging
    Serial.begin(115200);

    // Initialize GPIOs for LEDs, Buzzer, and Buttons
    pinMode(YELLOW_LED_PIN, OUTPUT);
    pinMode(RGB_LED_PIN, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(LEFT_BUTTON_PIN, INPUT_PULLUP);
    pinMode(RIGHT_BUTTON_PIN, INPUT_PULLUP);

    // Ensure Yellow LED is off by default
    digitalWrite(YELLOW_LED_PIN, LOW);

    // Turn on RGB LED
    digitalWrite(RGB_LED_PIN, HIGH);

    // Beep the buzzer twice
    for (int i = 0; i < 2; i++) {
        digitalWrite(BUZZER_PIN, HIGH);
        delay(200);
        digitalWrite(BUZZER_PIN, LOW);
        delay(200);
    }

    // Initialize I2C
    i2c_master_init();

    // Initialize LCD
    lcd_init();

    // Run I2C Scanner
    i2c_scanner();

    // Initialize Wi-Fi
    Serial.print("Setting up Wi-Fi...");
    WiFi.softAP(ssid, password);
    IPAddress IP = WiFi.softAPIP();
    Serial.print("Wi-Fi IP Address: ");
    Serial.println(IP);

    // Set up web servervia
    server.on("/", handleRoot);
    server.begin();
    Serial.println("Web server started");

    // Check if maxCapacity is uninitialized
    if(currentCar.maxCapacity == 0) {
        currentCar.maxCapacity = 1; // Set default value
    }

    // Dynamically allocate memory for occupants
    person* occupants = new person[currentCar.maxCapacity];

    // Set occupants
    for (int i = 0; i < currentCar.maxCapacity; i++) {
        int chosen = 0;
        occupants[i] = people[chosen];
    }
}

void loop() {
    handle_button_press();
    server.handleClient(); // Handle incoming HTTP requests
    delay(50); // Check button state every 50ms

    // Update the coordinates (example values)
    locus.update(12.34, 56.78, 100.0);  // Pass appropriate values for latitude, longitude, and altitude
}

void something() {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcdContent = "This does something";
    lcd.print(lcdContent);
}