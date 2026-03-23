#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>

// Konfigurasi Pin
#define LED1_PIN    PC13
#define LED2_PIN    PC14
#define LED3_PIN    PC15
#define BUTTON_PIN  PB1
#define DHTPIN      PA0      
#define DHTTYPE     DHT22

DHT dht(DHTPIN, DHTTYPE);
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Variabel Logika
enum State { ACTIVE, IDLE };
State currentState = IDLE;
int displayPage = 0; 

int tempHistory[128]; 
float temperature = 0;
const float threshold = 30.0;

unsigned long animationMillis = 0;
unsigned long dhtMillis = 0;
bool lastButtonState = HIGH;
int idleStep = 0; 

void allLedsOff() {
    digitalWrite(LED1_PIN, HIGH); digitalWrite(LED2_PIN, HIGH); digitalWrite(LED3_PIN, HIGH);
}

void setLedsActive() {
    digitalWrite(LED1_PIN, LOW); digitalWrite(LED2_PIN, LOW); digitalWrite(LED3_PIN, LOW);
}

// --- FUNGSI HEADER (Suhu, Threshold, Mode) ---
// Fungsi ini akan dipanggil di semua halaman agar informasi utama selalu tampil
void drawUnifiedInfo() {
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    
    // Baris 1: Suhu dan Threshold
    display.setCursor(0, 0);
    display.print("T:"); display.print(temperature, 1); display.print("C");
    display.setCursor(65, 0);
    display.print("TH:"); display.print(threshold, 1); display.print("C");
    
    // Baris 2: Mode (Active/Idle)
    display.setCursor(0, 10);
    display.print("MODE: ");
    if (currentState == ACTIVE) {
        display.print("ACTIVE (HOT)");
    } else {
        display.print("IDLE (COOL)");
    }
    
    display.drawFastHLine(0, 20, 128, SSD1306_WHITE);
}

// --- HALAMAN 1: STATUS LED DETAIL ---
void showStatusPage() {
    display.clearDisplay();
    drawUnifiedInfo(); // Tampilkan Suhu, Mode, Threshold

    display.setCursor(0, 28);
    display.print("STATUS LED:");
    display.setCursor(0, 40);
    display.print("L1: "); display.println((digitalRead(LED1_PIN) == LOW) ? "ON" : "OFF");
    display.print("L2: "); display.println((digitalRead(LED2_PIN) == LOW) ? "ON" : "OFF");
    display.print("L3: "); display.println((digitalRead(LED3_PIN) == LOW) ? "ON " : "OFF ");

    display.setCursor(35, 56); display.print("[ STATUS ]");
    display.display();
}

// --- HALAMAN 2: GRAFIK MONITORING ---
void showGraphPage() {
    display.clearDisplay();
    drawUnifiedInfo(); // Tampilkan Suhu, Mode, Threshold
    
    // Area Grafik (Diturunkan sedikit agar tidak menabrak header)
    display.drawRect(0, 24, 128, 30, SSD1306_WHITE);
    
    // Garis Threshold pada Grafik
    int threshY = map(30, 20, 40, 53, 25);
    for(int i=0; i<128; i+=4) display.drawFastHLine(i, threshY, 2, SSD1306_WHITE);

    // Plot Data Suhu
    for (int i = 0; i < 127; i++) {
        int y1 = map(tempHistory[i], 20, 40, 53, 25);
        int y2 = map(tempHistory[i+1], 20, 40, 53, 25);
        display.drawLine(i, y1, i+1, y2, SSD1306_WHITE);
    }
    
    display.setCursor(38, 56); display.print("[ GRAPH ]");
    display.display();
}

void setup() {
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(LED1_PIN, OUTPUT); pinMode(LED2_PIN, OUTPUT); pinMode(LED3_PIN, OUTPUT);
    
    allLedsOff();
    dht.begin();
    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { for(;;); }
    
    for(int i=0; i<128; i++) tempHistory[i] = 25;
}

void loop() {
    unsigned long currentMillis = millis();
    bool currentButtonReading = digitalRead(BUTTON_PIN);

    // 1. PEMBACAAN SENSOR & LOGIKA MODE
    if (currentMillis - dhtMillis >= 1000) {
        dhtMillis = currentMillis;
        float t = dht.readTemperature();
        if (!isnan(t)) {
            temperature = t;
            // Update history untuk grafik
            for (int i = 0; i < 127; i++) tempHistory[i] = tempHistory[i+1];
            tempHistory[127] = (int)temperature;
        }

        // Cek Threshold
        if (temperature >= threshold) {
            currentState = ACTIVE;
            setLedsActive();
        } else {
            currentState = IDLE;
        }
    }

    // 2. ANIMASI LED (Hanya saat IDLE)
    if (currentState == IDLE && (currentMillis - animationMillis >= 80)) {
        animationMillis = currentMillis;
        allLedsOff();
        if (idleStep == 0) digitalWrite(LED1_PIN, LOW);
        else if (idleStep == 1) digitalWrite(LED2_PIN, LOW);
        else if (idleStep == 2) digitalWrite(LED3_PIN, LOW);
        idleStep = (idleStep + 1) % 3;
    }

    // 3. TOMBOL NAVIGASI HALAMAN (PB1)
    if (currentButtonReading == LOW && lastButtonState == HIGH) {
        displayPage = !displayPage; 
        delay(150); 
    }
    lastButtonState = currentButtonReading;

    // 4. UPDATE TAMPILAN
    if (displayPage == 0) showStatusPage();
    else showGraphPage();
}