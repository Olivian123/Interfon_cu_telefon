#include <LCD_I2C.h>
#include <Keypad.h>

const byte ROWS = 4; //four rows
const byte COLS = 4; //three columns
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};

// connect the pins from right to left to pin 2, 3, 4, 5,6,7,8,9
byte rowPins[ROWS] = {5,4,3,2}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {9,8,7,6}; //connect to the column pinouts of the keypad

Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

int reset = 0;

int len;

int state[100];
int led = 13;

LCD_I2C lcd(0x27);

void setup() {
  lcd.begin();
  lcd.backlight();
  
  // Configurarea Bluetooth
  pinMode(led, OUTPUT);
  digitalWrite(led, HIGH);
  Serial.begin(9600);
}

void loop() {

  char key = keypad.getKey();
  if (key){
    if (!reset) {
      lcd.clear(); // Șterge afișajul LCD
      lcd.print("Apartamen: ");
    } 
    
    reset = 1;

    lcd.print(key);
  } else if (!reset) {
    lcd.clear();
    lcd.print("Apelati!");
  }
  
  // this checks if 4 is pressed, then do something. Here  we print the text but you can control something.
  if (key == 'D'){
    reset = 0;
  }

  len = 0;

  // Verifică dacă există date disponibile în portul serial
  while (Serial.available() > 0) { 
    state[len] = Serial.read();
    len ++;
  }

  if (len > 0 ) {
    lcd.clear(); // Șterge afișajul LCD
    for (int i = 0; i < len; i++)
      Serial.print((char)state[i]);
  }

    if (state[0] == '0') {
      digitalWrite(led, LOW); // Stinge LED-ul

    } 
    else if (state[0] == '1') {
      digitalWrite(led, HIGH); // Aprinde LED-ul
    }

    delay(500);
}