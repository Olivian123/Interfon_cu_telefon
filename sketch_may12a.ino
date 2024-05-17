#include <PinChangeInterrupt.h>
#include <LCD_I2C.h>
#include <Keypad.h>

// Define the LCD address and dimensions
#define I2C_ADDR 0x27
#define LCD_COLUMNS 16
#define LCD_ROWS 2

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

int state;
int led = 13;
LCD_I2C lcd(I2C_ADDR, LCD_COLUMNS, LCD_ROWS);

int size = 0;
int number[100];

void setup() {
  lcd.begin();
  lcd.backlight();

  pinMode(led, OUTPUT);
  digitalWrite(led, LOW);
 
  Serial.begin(9600);
}

void loop() {
  char key = keypad.getKey();

  if (key) {
    if (key == 'D') {
      lcd.clear();

      size = 0; 
    } else if (key == 'C' && size > 0) {
      lcd.clear();
      lcd.print("Suna!");

      call();
    } else if (key != '*' || key != '#') {
      lcd.print(key);

      number[size] = key - 48;
      size++;
    }
  }
}

void call() {
  unsigned long startTime = millis();
  while (true) { // Check the flag to continue or stop the loop
    char key = keypad.getKey();

    if (Serial.available())
      break;

    if (millis() - startTime >= 10000) {
      lcd.clear();
      lcd.print("Timeout occurred");
      delay(1000);
      lcd.clear();

      size = 0;
      return;
    }

    if(key == 'D') {      
      size = 0;

      lcd.clear();
      lcd.print("Apel oprit!");

      delay(1000);
      lcd.clear();

      return;
    }
  }

  size = 0;

  state = Serial.read();

  lcd.clear();
  if (state == '1') {  
    lcd.print("Intrati!");
    digitalWrite(led, HIGH);
    
    delay(10000);
    digitalWrite(led, LOW);
  } else {
    lcd.print("Respins!");
    delay(1000);
  }

  lcd.clear();
}
