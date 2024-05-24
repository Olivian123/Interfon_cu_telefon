#include <PinChangeInterrupt.h>
#include <LCD_I2C.h>
#include <Keypad.h>
#include <SPI.h>
#include <MFRC522.h>

// Define the LCD address and dimensions
#define I2C_ADDR 0x27
#define LCD_COLUMNS 16
#define LCD_ROWS 2

// Definition of the pins used by RFID card reader
#define SS_PIN 10
#define RST_PIN A0 // Define RST pin as A0
#define LED_PIN A1 // Define LED pin as A1

#define RELAY_LOCK A2

MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance.

String allowedUIDs[] = {"33802613"};

// Keypad setup
const byte ROWS = 4; // four rows
const byte COLS = 4; // four columns
char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

byte rowPins[ROWS] = {5, 4, 3, 2}; // Row pinouts
byte colPins[COLS] = {9, 8, 7, 6}; // Column pinouts

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

LCD_I2C lcd(I2C_ADDR, LCD_COLUMNS, LCD_ROWS);

int size = 0;
int number = 0;

void setup() {
  Serial.begin(9600);

  lcd.begin();
  lcd.backlight();

  Serial.begin(9600);   // Initialize serial communications with the PC
  SPI.begin();          // Init SPI bus
  mfrc522.PCD_Init();   // Init MFRC522

  // Pin for lock
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW); // Ensure LED is off
  
  pinMode(RELAY_LOCK, OUTPUT);
}

void loop() {
  char key = keypad.getKey();

  if (key) {
    if (key == 'D') {
      lcd.clear();

      size = 0; 
      number = 0;
    } else if (key == 'C') {
      if (size == 0)
        return;

      lcd.clear();
      lcd.print("Suna la ");
      lcd.print(number);
      lcd.print("!");

      Serial.write(9);
      call();
    } else if (key != '*' || key != '#') {
      lcd.print(key);

      number = number * 10 + (key - '0');
      size++;
    }
  } else {
    if (!mfrc522.PICC_IsNewCardPresent()) {
      return; // Exit the loop if no new card is present
    }

    // Select one of the cards
    if (!mfrc522.PICC_ReadCardSerial()) {
      return; // Exit the loop if no card UID could be read
    }

    open_door_by_card();  
  }
}

void call() {
  unsigned long startTime = millis();
  while (true) { // Check the flag to continue or stop the loop
    char key = keypad.getKey();

    if (Serial.available())
      break;

    if (millis() - startTime >= 20000) {
      lcd.clear();
      lcd.print("Timeout occurred");
      delay(1000);
      lcd.clear();

      size = 0;
      number = 0;

      return;
    }

    if(key == 'D') {      
      size = 0;
      number = 0;

      Serial.write(8);

      lcd.clear();
      lcd.print("Apel oprit!");

      delay(1000);
      lcd.clear();

      return;
    }
  }

  size = 0;
  number = 0;

  int state = Serial.read();

  lcd.clear();
  if (state == '1') {  
    lcd.print("Intrati!");
    digitalWrite(LED_PIN, HIGH);
    digitalWrite(RELAY_LOCK, HIGH);
    
    delay(10000);

    digitalWrite(LED_PIN, LOW);
    digitalWrite(RELAY_LOCK, LOW);
  } else {
    lcd.print("Respins!");
    delay(1000);
  }

  lcd.clear();
}

void open_door_by_card() {
  // Read the UID
  String content = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? "0" : ""));
    content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  content.toUpperCase(); // Convert UID to uppercase

  // Check if UID is allowed
  if (isUIDAllowed(content)) {
    lcd.print("Intrati!");

    digitalWrite(LED_PIN, HIGH);  // Turn on LED
    digitalWrite(RELAY_LOCK, HIGH);

    delay(10000);                 // Keep LED on for 10 seconds
    
    digitalWrite(LED_PIN, LOW);   // Turn off LED
    digitalWrite(RELAY_LOCK, LOW);

    lcd.clear();
  }

  delay(1000); // Add a small delay before the next loop iteration
}

bool isUIDAllowed(String uid) {
  for (String allowedUID : allowedUIDs) {
    if (uid.equalsIgnoreCase(allowedUID)) {
      return true;
    }
  }
  return false;
}


