/*
 * Author: Andrew Berntson
 * Date Created: August 11 2023
 * Date Updated: September 9 2023
 */

// Define output pins for 74HC595
#define RCLK D0  // Output register latch pin
#define SER D1   // Serial data input pin
#define SRCLK D2 // Serial register clock pin

// Define pins for enabling digits
#define enable1 D4  // First digit
#define enable2 D5  // Second digit
#define enable3 D6  // Third digit
#define enable4 D7  // Fourth digit

// Define pin for reading input for time changing
#define toggle D3 // Toggle between adjusting hours or minutes
#define button D8 // Increment hours/minutes by 1

// Parameters for pressing the time-adjust button
int last_pressed = 0;         // last time (in ms since starting) button was pressed
const int button_delay = 200; // in milliseconds

// Time offsets for adjusting clock's time
int offset_h = 16; // Offset for hours
int offset_m = 30; // Offset for minutes
int offset_s = 0; // Offset for seconds

// Hour, minute, second outputs for display
int hours = 0;
int minutes = 0;
int seconds = 0; // not displayed

// Shift out a byte from the shift register
void sendByte(byte data) {
  // Shift lowest bit out
  shiftOut(SER, SRCLK, LSBFIRST, data);
  
  // Set the output register latch to low, as it
  // will update when it set to high
  digitalWrite(RCLK, LOW);
  digitalWrite(RCLK, HIGH);
}

// Enable a specific digit of the common anode
// display.
void enableDigit(int n) {
  // Disable all digits
  digitalWrite(enable1, HIGH);
  digitalWrite(enable2, HIGH);
  digitalWrite(enable3, HIGH);
  digitalWrite(enable4, HIGH);

  // Now enable only one digit
  if (n == 1) {
    digitalWrite(enable1, LOW);
  } else if (n == 2) {
    digitalWrite(enable2, LOW);
  } else if (n == 3) {
    digitalWrite(enable3, LOW);
  } else if (n == 4) {
    digitalWrite(enable4, LOW);
  } else {
    // Do nothing if n is not between 1 and 4
  }
}

// Take a digit 0-9 and return the encoding
// for the 8-seg display. Note this encoding is
// unique to the wiring setup.
byte encode(int n) {
  // If n is not between 0 and 9, return
  // the encoding for a decimal place as the
  // error character.
  if (n < 0 || n > 9) {
    return 0b00000100;
  }
  
  byte matrix[10] = {
      0b11111010, // 0
      0b00100010, // 1
      0b10111001, // 2
      0b10101011, // 3
      0b01100011, // 4
      0b11001011, // 5
      0b11011011, // 6
      0b10100010, // 7
      0b11111011, // 8
      0b11100011  // 9
    };
  return matrix[n];
}
// Overload encode method so we can also encode
// some letters.
byte encode(char c) {
  // The given character MUST be lowercase.

  // Handle special cases for c
  if (c == ' ') {
    return 0b00000000;
  }

  // Convert the character to an index in the matrix
  // representing that characters encoding. Do so by
  // subtracting the ascii value for a.
  int i = c - 97;

  // If i is not between 0 and 25, it is not a lowercase
  // letter so return the error character (a decimal).
  if (i < 0 || i > 25) {
    return 0b00000100;
  }

  byte matrix[26] = {
    0b11110011,  // a
    0b01011011,  // b
    0b00011001,  // c
    0b00111011,  // d
    0b11011001,  // e
    0b11010001,  // f
    0b11011010,  // g
    0b01110011,  // h
    0b00000010,  // i
    0b00111010,  // j
    0b00000100,  // k, error .
    0b01011000,  // l
    0b00000100,  // m, error .
    0b00010011,  // n
    0b00011011,  // o
    0b11110001,  // p
    0b00000100,  // q, error .
    0b00000100,  // r, error .
    0b11001011,  // s
    0b00000100,  // t, error .
    0b00011010,  // u
    0b00000100,  // v, error .
    0b00000100,  // w, error .
    0b00000100,  // x, error .
    0b00000100,  // y, error .
    0b00000100,  // z, error .
  };
  return matrix[i];
}

// Output 4 ints to the display
void displayNum(int dig1, int dig2, int dig3, int dig4) {
  // Set the first digit
  sendByte(encode(dig1));
  enableDigit(1); // Turn off all digits but the 1st
  delay(1); // Delay makes multiplexing possible

  // Set the second digit
  sendByte(encode(dig2));
  enableDigit(2); // Turn off all digits but the 2nd
  delay(1); // Delay makes multiplexing possible

  // Set the third digit
  sendByte(encode(dig3));
  enableDigit(3); // Turn off all digits but the 3rd
  delay(1); // Delay makes multiplexing possible

  // Set the fourth digit
  sendByte(encode(dig4));
  enableDigit(4); // Turn off all digits but the 4th
  delay(1); // Delay makes multiplexing possible
}

// Output 4 chars to the display, message must be lowercase
void displayStr(char message[4]) {
  // Display the first character in the first digit
  sendByte(encode(message[0]));
  enableDigit(1); // Turn off all digits but the 1st
  delay(1); // Delay makes multiplexing possible

  // Display the second character in the second digit
  sendByte(encode(message[1]));
  enableDigit(2); // Turn off all digits but the 2nd
  delay(1); // Delay makes multiplexing possible

  // Display the third character in the third digit
  sendByte(encode(message[2]));
  enableDigit(3); // Turn off all digits but the 3rd
  delay(1); // Delay makes multiplexing possible

  // Display the fourth character in the fourth digit
  sendByte(encode(message[3]));
  enableDigit(4); // Turn off all digits but the 4th
  delay(1); // Delay makes multiplexing possible
}

// Return the seconds on the clock.
int getSeconds() {
  return (offset_s + millis() / 1000) % 60;
}

// Return the minutes on the clock.
int getMinutes() {
  return (offset_m + (offset_s + millis() / 1000) / 60) % 60;
}

// Return the hours on the clock.
int getHours() {
  return (offset_h + (offset_m + (offset_s + millis() / 1000) / 60) / 60) % 24;
}

// Adjust offsets if the button was pressed.
void adjustOffsets() {
  // When the button is pressed, check which part of the
  // time to increment: Hours or minutes. Provided they didn't
  // press the button less than 200ms since they last pressed it
  //
  // toggle == HIGH for hours
  // toggle == LOW for minutes
  if (digitalRead(button) == HIGH && last_pressed + button_delay <= millis()) {
    if (digitalRead(toggle) == HIGH) {  // Increment hours by 1
      offset_h = (offset_h + 1) % 24;
    } else {                            // Increment minutes by 1
      offset_m = (offset_m + 1) % 60;
    }
    last_pressed = millis();
  }
}

void setup() {
  // Set pins to output
  pinMode(RCLK, OUTPUT);
  pinMode(SER, OUTPUT);
  pinMode(SRCLK, OUTPUT);
  pinMode(enable1, OUTPUT);
  pinMode(enable2, OUTPUT);
  pinMode(enable3, OUTPUT);
  pinMode(enable4, OUTPUT);

  // Set pins to input
  pinMode(toggle, INPUT_PULLUP);
  pinMode(button, INPUT);
}

void loop() {
  // Adjust offsets to change the time
  adjustOffsets();

  // Update the time on the clock
  seconds = getSeconds();
  minutes = getMinutes();
  hours = getHours();

  // Update the clock's display with a super cool secret
  // message, or just the boring time.
  if (hours == 0 && minutes < 1) {         // Display message for 1 min at 12am
    displayStr("poo ");
  } else if (hours == 12 && minutes < 1) { // Display message for 1 min at 12pm
    displayStr("pee ");
  } else {
    // Update the clock's display with the time
    displayNum(hours / 10, hours % 10, minutes / 10, minutes % 10);
  }

  
}
