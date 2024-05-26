#include <PinChangeInterrupt.h>
#include <LCD_I2C.h>
#include <Keypad.h>
#include <SPI.h>
#include <MFRC522.h>
#include <EEPROM.h>
#include <Keypad.h>
#include <SoftwareSerial.h>

// Define the LCD address and dimensions
#define I2C_ADDR 0x27
#define LCD_COLUMNS 16
#define LCD_ROWS 2

// Definition of the pins used by RFID card reader
#define SS_PIN 10
#define RST_PIN A0 // Define RST pin as A0
#define LED_PIN A1 // Define LED pin as A1

#define RELAY_LOCK A2

// Definiton of the data needed by EEPROM memory management
#define MAGIC_NUMBER 0x42
#define EEPROM_MAGIC_ADDR 0
#define EEPROM_COUNT_ADDR sizeof(int)
#define EEPROM_START_ADDR (EEPROM_COUNT_ADDR + sizeof(int))
#define MAX_APARTMENTS 100

MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance.

String allowedUIDs[] = {"33802613"}; // The id of the card that opend the lock

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

// Curent typed number
int size = 0;
int number = 0;

// Password and the apartments that are registered
const char* password = "2389"; // Password for adding / removing apartments

int apartments[MAX_APARTMENTS]; // Array to store apartment IDs
int apartmentCount = 0; // Number of apartments

void setup() {
  // Setup for LCD
  lcd.begin();
  lcd.backlight();

  // Setup for RFID module
  Serial.begin(9600);   // Initialize serial communications with the PC
  SPI.begin();          // Init SPI bus
  mfrc522.PCD_Init();   // Init MFRC522

  // Setup for Relay module and lock
  pinMode(LED_PIN, OUTPUT);    // Pin for lock
  digitalWrite(LED_PIN, LOW);  // Ensure LED is off
  pinMode(RELAY_LOCK, OUTPUT); // Pin for Relay module control

  // Setup for EEPROM memory management
  int magic; // number used to check if the EEPROM memory was initialize or not

  // If the EEPROM memory does not have magic number in first position
  // initialize this memory
  EEPROM.get(EEPROM_MAGIC_ADDR, magic);
  if (magic != MAGIC_NUMBER)
    initializeEEPROM();

  // Load apartments from EEPROM to array
  loadApartmentsFromEEPROM();
}

void loop() {
  char key = keypad.getKey();

  // If the one of the keys are pressed do / write something
  if (key) {
    // 'D' = delete
    // This command clears the curent  apartment number from memory
    // and clears the lcd display
    if (key == 'D')
      clear();

    // 'C' = call
    // Calls the curent apartment number
    // If the apartment is not register it displays 'Neinregistrat'
    // Else it calls the 'call' comand
    if (key == 'C') {
      // if there is no apartment number do nothing
      if (size == 0)
        return;

      if (!check_ap(number)) {
        lcd.clear();
        lcd.print("Neinregistrat!");
        
        delay(3000);
        
        clear();

        return;
      }

      call(number);
    } 

    // 'A' = add
    // Adds an apartment to the list stored in the EEPROM memory
    if (key == 'A') {
      // If an apartment number is displayed do nothing
      // Most probably the key was pressed by accident in thi instance
      if (size != 0) 
        return;

      // Request password and check wheter is correct or not
      if(!request_password())
        return;

      // Display request for new apartment
      lcd.clear();
      lcd.print("Introdu apartament:");
      lcd.setCursor(0, 1);
      String apartmentID = "";
      
      while (true) {
        char key = keypad.getKey();

        // Get the digits of the apartments untill 'A' key is pressed
        if(key == 'A')
          break;

        if (key && isDigit(key)) {
          apartmentID += key;
          lcd.print(key);
        }
      }

      delay(1000);
      lcd.clear();

      if (!check_ap(apartmentID.toInt())) {
        lcd.clear();
        lcd.print("Apartamentul deja");
        lcd.setCursor(0, 1);
        lcd.print("exista!");
        
        delay(3000);
        
        clear();

        return;
      }

      // Calling function for adding apartment in to EEPROM memory
      addApartment(apartmentID.toInt());
    } 

    // Function for selectivelly removing an apartment from the list
    if (key == '*') {
      if (size != 0) 
        return;

      // Request password and check wheter is correct or not
      if(!request_password())
        return;

      // Display request for apartment to be removed
      lcd.clear();
      lcd.print("Introdu apartament:");
      lcd.setCursor(0, 1);
      String apartmentID = "";
      
      while (true) {
        char key = keypad.getKey();

        if(key == 'A')
          break;

        if (key && isDigit(key)) {
          apartmentID += key;
          lcd.print(key);
        }
      }

      delay(1000);
      lcd.clear();

      removeApartment(apartmentID.toInt());
    } 

    // Function to remove all apartments from memroy
    if (key == '#') {
      if (size != 0) 
        return;

      // Request password and check wheter is correct or not
      if(!request_password())
        return;

      removeAllApartments();
    } 
    
    // Functions to show all the apartments stored
    if (key == 'B') {
      if (size != 0) 
        return;

      lcd.clear();

      // If there are no apartments show message
      if (apartmentCount == 0) {
        lcd.print("Nu exista");
        lcd.setCursor(0, 1);
        lcd.print("apartamente!");

        delay(2000);
        lcd.clear();

        return;
      }

      lcd.print("Apartamente:");
      for (int i = 0; i < apartmentCount; i++) {
        lcd.setCursor(0, 1);
        lcd.print(apartments[i]);

        // Display each apartment for 1 second
        delay(1000); 
      }
      
      delay(2000);
      lcd.clear();
    } 
    
    // Add the digit represented by the key to the number
    // and display it
    if (isdigit(key)) {
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

// Clear the curent number from RAM (?) and clears the scren
void clear() {
  size = 0;
  number = 0;

  lcd.clear();
}

// Checks wheter an apartment is in the memory or not
int check_ap(int ap_number) {
  // Going trough all the registered apartments 
  int i;
  for (i = 0; i < apartmentCount; i++)
    if (number == apartments[i])
      break;

  if (i == apartmentCount && apartmentCount != 0) 
    return 0;

  return 1;      
}

int request_password() {
  // Display request
  lcd.clear();
  lcd.print("Introdu parola:");
  lcd.setCursor(0, 1);

  // Read the given password
  String enteredPassword = "";
  while (enteredPassword.length() < 4) {
    char key = keypad.getKey();
    if (key) {
      enteredPassword += key;
      
      // Mask the password input
      lcd.print('*'); 
    }
  }
  
  // If password is not correct return
  if (strcmp(enteredPassword.c_str(), password) != 0) {
    lcd.clear();
    lcd.print("Parola incorecta!");
    delay(2000);

    lcd.clear();
    return 0;
  }

  return 1;
}

// Calls and waits for response
// Prints a message based on the response
void call(int ap_number) {
  // Displaying a message indicating to where the call is intended
  lcd.clear();
  lcd.print("Suna la ");
  lcd.print(ap_number);
  lcd.print("!");

  // Send the number to the aplication
  Serial.write(ap_number);

  unsigned long startTime = millis();
  // Check the flag to continue or stop the loop
  while (true) { 
    char key = keypad.getKey();

    if (Serial.available())
      break;

    // The call last 20 sec, it the time expiers stop
    if (millis() - startTime >= 20000) {
      lcd.clear();
      lcd.print("Timeout occurred");
      
      delay(1000);

      clear();
      
      return;
    }

    // The caller can choose to stop whenever he wants
    // By pressing 'D' he stops the call
    // Sending '0' to the app
    if(key == 'D') {      
      Serial.write(0);
      
      clear();

      lcd.print("Apel oprit!");

      delay(1000);
      lcd.clear();

      return;
    }
  }
  
  int response = Serial.read();

  clear();

  if (response == '1') {  
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

// Intialize EEPROM by puitng the magic number
void initializeEEPROM() {
  EEPROM.put(EEPROM_MAGIC_ADDR, MAGIC_NUMBER);

  apartmentCount = 0;
  
  EEPROM.put(EEPROM_COUNT_ADDR, apartmentCount);
}

void addApartment(int newApartmentID) {
  if(apartmentCount >= MAX_APARTMENTS) {
    lcd.print("Numar maxim atins.");
    lcd.clear();

    return;
  }

  EEPROM.put(EEPROM_START_ADDR + apartmentCount * sizeof(int), newApartmentID);

  // Add to array
  apartments[apartmentCount] = newApartmentID; 
  apartmentCount++;
  
  EEPROM.put(EEPROM_COUNT_ADDR, apartmentCount);

  lcd.print("Apartament aduagat!");
  delay(2000);
  lcd.clear();
}

// Removes apartment from memory
void removeApartment(int apartmentID) {
  for (int i = 0; i < apartmentCount; i++) {
    if (apartments[i] == apartmentID) {
      for (int j = i; j < apartmentCount - 1; j++) {
        apartments[j] = apartments[j + 1];
        EEPROM.put(EEPROM_START_ADDR + j * sizeof(int), apartments[j]);
      }
      
      apartmentCount--;
      EEPROM.put(EEPROM_COUNT_ADDR, apartmentCount);
      lcd.print("Apartament sters!");

      delay(2000);
      lcd.clear();
      
      return;
    }
  }
  
  lcd.print("Apartament negasit!");

  delay(2000);
  lcd.clear();
}

void removeAllApartments() {
  initializeEEPROM();

  lcd.print("Toate apartamentele");
  lcd.setCursor(0, 1);
  lcd.print("au fost sterse!");

  delay(2000);
  lcd.clear();
}

void loadApartmentsFromEEPROM() {
  EEPROM.get(EEPROM_COUNT_ADDR, apartmentCount);
  
  for (int i = 0; i < apartmentCount; i++) {
    EEPROM.get(EEPROM_START_ADDR + i * sizeof(int), apartments[i]);
  }
}