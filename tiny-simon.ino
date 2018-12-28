/**
   Simon Game for ATTiny85
   Copyright (C) 2018, Uri Shaked
   Licensed under the MIT License.
*/

#include <avr/sleep.h>
#include <avr/interrupt.h>

#include "pitches.h"

/* Constants - define pin numbers for leds, buttons and speaker, and also the game tones */
byte buttonPins[] = {1, 2, 3, 4};
#define SPEAKER_PIN 0

#define MAX_GAME_LENGTH 100

int gameTones[] = { NOTE_G3, NOTE_C4, NOTE_E4, NOTE_G5};

/* Global variales - store the game state */
byte gameSequence[MAX_GAME_LENGTH] = {0};
byte gameIndex = 0;

/**
   Set up the GPIO pins
*/
void setup() {
  // The following line primes the random number generator. It assumes pin A0 is floating (disconnected)
  randomSeed(analogRead(1));

  // Disable ADC - saves about 324.5uA in sleep mode!
  ADCSRA = 0;
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);

  for (int i = 0; i < 4; i++) {
    pinMode(buttonPins[i], INPUT_PULLUP);
  }
  pinMode(SPEAKER_PIN, OUTPUT);
}

ISR(PCINT0_vect) {
  GIMSK &= ~0b00100000;  // Turn off pin change interrupts
  sleep_disable();
}

void sleep() {
  sleep_enable();
  noInterrupts();
  GIMSK |= 0b00100000;  // Turn on pin change interrupts
  for (byte i = 0; i < 4; i++) {
    PCMSK |= 1 << buttonPins[i];
  }
  interrupts();
  sleep_cpu();
}


// The sound-producing function
void beep (unsigned char speakerPin, int frequencyInHertz, long timeInMilliseconds)
{ // http://web.media.mit.edu/~leah/LilyPad/07_sound_code.html
  int  x;
  long delayAmount = (long)(1000000 / frequencyInHertz);
  long loopTime = (long)((timeInMilliseconds * 1000) / (delayAmount * 2));
  for (x = 0; x < loopTime; x++) {
    digitalWrite(speakerPin, HIGH);
    delayMicroseconds(delayAmount);
    digitalWrite(speakerPin, LOW);
    delayMicroseconds(delayAmount);
  }
}

/**
   Lights the given led and plays the suitable tone
*/
void lightLedAndPlaySound(byte ledIndex) {
  pinMode(buttonPins[ledIndex], OUTPUT);
  digitalWrite(buttonPins[ledIndex], LOW);
  beep(SPEAKER_PIN, gameTones[ledIndex], 300);
  pinMode(buttonPins[ledIndex], INPUT_PULLUP);
}

/**
   Plays the current sequence of notes that the user has to repeat
*/
void playSequence() {
  for (int i = 0; i < gameIndex; i++) {
    byte currentLed = gameSequence[i];
    lightLedAndPlaySound(currentLed);
    delay(50);
  }
}

/**
    Waits until the user pressed one of the buttons, and returns the index of that button
*/
byte readButton() {
  for (;;) {
    for (int i = 0; i < 4; i++) {
      byte buttonPin = buttonPins[i];
      if (digitalRead(buttonPin) == LOW) {
        return i;
      }
    }
    sleep();
  }
}

/**
   Plays the tone associated with a specific button + debouncing mechanism
*/
void playButtonTone(byte buttonIndex) {
  beep(SPEAKER_PIN, gameTones[buttonIndex], 150);

  // Wait until button is released.
  while (digitalRead(buttonPins[buttonIndex]) == LOW);
  delay(50);
}

/**
  Play the game over sequence, and report the game score
*/
void gameOver() {
  gameIndex = 0;
  delay(200);
  // Play a Wah-Wah-Wah-Wah sound
  beep(SPEAKER_PIN, NOTE_DS5, 300);
  beep(SPEAKER_PIN, NOTE_D5, 300);
  beep(SPEAKER_PIN, NOTE_CS5, 300);
  for (int i = 0; i < 200; i++) {
    beep(SPEAKER_PIN, NOTE_C5 + (i % 20 - 10), 5);
  }
  delay(500);
}

/**
   Get the user input and compare it with the expected sequence. If the user fails, play the game over sequence and restart the game.
*/
void checkUserSequence() {
  for (int i = 0; i < gameIndex; i++) {
    byte expectedButton = gameSequence[i];
    byte actualButton = readButton();
    playButtonTone(actualButton);

    if (expectedButton == actualButton) {
      /* good */
    } else {
      gameOver();
      return;
    }
  }
}

/**
   Plays an hooray sound whenever the user finishes a level
*/
void levelUp() {
  beep(SPEAKER_PIN, NOTE_E4, 150);
  beep(SPEAKER_PIN, NOTE_G4, 150);
  beep(SPEAKER_PIN, NOTE_E5, 150);
  beep(SPEAKER_PIN, NOTE_C5, 150);
  beep(SPEAKER_PIN, NOTE_D5, 150);
  beep(SPEAKER_PIN, NOTE_G5, 150);
  noTone(SPEAKER_PIN);
}

/**
   The main game loop
*/
void loop() {
  // Add a random color to the end of the sequence
  gameSequence[gameIndex] = random(0, 4);
  gameIndex++;

  playSequence();
  checkUserSequence();
  delay(300);

  if (gameIndex > 0) {
    levelUp();
    delay(300);
  }
}

