#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

// GUItool: begin automatically generated code
AudioSynthWaveform waveform1;      //xy=200,300
AudioEffectEnvelope envelope1;     //xy=400,300
AudioFilterStateVariable filter1;  //xy=600,300
AudioOutputI2S i2s1;               //xy=800,300
AudioConnection patchCord1(waveform1, envelope1);
AudioConnection patchCord2(envelope1, 0, filter1, 0);
AudioConnection patchCord3(filter1, 0, i2s1, 0);
AudioConnection patchCord4(filter1, 0, i2s1, 1);
AudioControlSGTL5000 sgtl5000_1;  //xy=200,200
// GUItool: end automatically generated code




int loopCount = 0;
const bool debug = true;

// Pots & Multiplexer variables
#define controlPin0 32
#define controlPin1 31
#define controlPin2 30
#define controlPin3 29
#define muxOutputPin A10
const int numPots = 6;
int potValues[numPots];
int lastPotValues[numPots];
const int potChannels[numPots] = { 15, 14, 13, 12, 11, 10, 9};

// Push Button variables
#define BUTTON_PIN 34
bool lastButtonState = HIGH;
bool buttonState = HIGH;
unsigned long buttonLastDebounceTime = 0;
unsigned long buttonDebounceDelay = 10;  // milliseconds

// Toggle Switch variables
#define TOGGLE_PIN 33
bool lastToggleState = HIGH;
bool toggleState = HIGH;
unsigned long toggleLastDebounceTime = 0;
unsigned long toggleDebounceDelay = 10;  // milliseconds

// Rotary Encoder variables
#define encoderPinA 35
#define encoderPinB 36
#define encoderButtonPin 37

volatile int encoderPos = 0;
volatile int lastEncoded = 0;
volatile bool encoderButtonState = HIGH;
volatile bool encoderLastButtonState = HIGH;
const long encoderButtonDebounceDelay = 50;
long encoderLastButtonDebounceTime = 0;



void setup() {
  // Mux & Pot Setup
  pinMode(controlPin0, OUTPUT);
  pinMode(controlPin1, OUTPUT);
  pinMode(controlPin2, OUTPUT);
  pinMode(controlPin3, OUTPUT);

  // Button Setup
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  pinMode(TOGGLE_PIN, INPUT_PULLUP);


  // Rotary Encoder Setup
  pinMode(encoderPinA, INPUT_PULLUP);
  pinMode(encoderPinB, INPUT_PULLUP);
  pinMode(encoderButtonPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(encoderPinA), interruptEncoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(encoderPinB), interruptEncoder, CHANGE);
  // attachInterrupt(digitalPinToInterrupt(encoderButtonPin), interruptEncoderButton, LOW);


  // Audio Card setup
  AudioMemory(20);
  sgtl5000_1.enable();
  sgtl5000_1.volume(0.7);

  //Synth Setup
  waveform1.begin(.3, 220, WAVEFORM_SAWTOOTH);
  envelope1.attack(10);
  envelope1.hold(50);
  envelope1.decay(200);
  envelope1.sustain(0.8);
  envelope1.release(500);
  filter1.frequency(2000);
  filter1.resonance(0.7);
  filter1.octaveControl(1.0);

  if (debug) {
    Serial.begin(9600);
    while (!Serial) {
      ;  // Wait here until serial monitor is running
    }
    Serial.println("Setup complete");
  }
}

void loop() {

  readPots();
  readButton();
  readToggle();
  readEncoderButton();

  loopCount++;
  if (debug && loopCount % 1000 == 0) {
    Serial.print(loopCount);
    Serial.println(" loops");
  }
}


float inputToPitch(int n) {
  float freq = 220 * pow(2, n / 48.0);  //divide by 48 because encoder counts in 4s
  return freq;
}

//Below are functions to read all the inputs

void readPots() {
  for (int i = 0; i < numPots; i++) {
    // Select the input channel on the multiplexer
    int channel = potChannels[i];
    digitalWrite(controlPin0, channel & 1);
    digitalWrite(controlPin1, (channel >> 1) & 1);
    digitalWrite(controlPin2, (channel >> 2) & 1);
    digitalWrite(controlPin3, (channel >> 3) & 1);

    // Allow some time for the signal to settle on the mux
    delay(1);

    // Read the value from the selected potentiometer
    potValues[i] = analogRead(muxOutputPin);
    if (abs(potValues[i] - lastPotValues[i]) > 5) {
      if (debug) {
        Serial.print("Pot");
        Serial.print(i);
        Serial.print(" ");
        Serial.println(potValues[i]);
      }

      lastPotValues[i] = potValues[i];
    }
  }

  // assign pot readings
  //float amplitude = potValues[6] * .6 / 150.0;
  //waveform1.amplitude(amplitude);
  float attack = map(potValues[5], 800, 0, 0, 100);
  float decay = map(potValues[4], 800, 0, 0, 1000);
  float sustain = map(potValues[3], 800, 0, 0, 1.0);
  float release = map(potValues[2], 800, 0, 0, 2000);
  envelope1.attack(attack);
  envelope1.decay(decay);
  envelope1.sustain(sustain);
  envelope1.release(release);
  float cutoff = map(potValues[1], 1023, 0, 20, 10000);
  float resonance = map(potValues[0], 1023, 0, .7, 5);
  filter1.frequency(cutoff);
  filter1.resonance(resonance);
}

void readButton() {
  // Read the current state of the button
  int buttonReading = digitalRead(BUTTON_PIN);

  // Check if the button state has changed
  if (buttonReading != lastButtonState) {
    // Reset the debouncing timer
    buttonLastDebounceTime = millis();
  }

  // If the state has been stable for longer than the debounce delay
  if ((millis() - buttonLastDebounceTime) > buttonDebounceDelay) {
    // If the button state has changed
    if (buttonReading != buttonState) {
      buttonState = buttonReading;
      if (buttonReading == LOW) {
        if (debug) { Serial.println("Button Pressed"); }
        envelope1.noteOn();
      }
      if (buttonReading == HIGH) {
        if (debug) { Serial.println("Button Released"); }
        envelope1.noteOff();
      }
    }
  }

  // Save the reading for the next loop
  lastButtonState = buttonReading;
}

void readToggle() {
  int toggleReading = digitalRead(TOGGLE_PIN);
  // Check if the toggle state has changed
  if (toggleReading != lastToggleState) {
    // Reset the debouncing timer
    toggleLastDebounceTime = millis();
  }

  if ((millis() - toggleLastDebounceTime) > toggleDebounceDelay) {

    if (toggleReading != toggleState) {
      toggleState = toggleReading;
      if (toggleReading == LOW) {
        // do something
        if (debug) { Serial.println("Switch up"); }
      } else {
        // do something else
        if (debug) { Serial.println("Switch down"); }
      }
    }
  }
  lastToggleState = toggleReading;
}

void readEncoderButton() {

  // Handle button press with debouncing
  int buttonReading = digitalRead(encoderButtonPin);
  if (buttonReading != encoderLastButtonState) {
    encoderLastButtonDebounceTime = millis();  // Reset the debouncing timer
  }
  if ((millis() - encoderLastButtonDebounceTime) > encoderButtonDebounceDelay) {
    if (buttonReading != encoderButtonState) {
      encoderButtonState = buttonReading;
      if (encoderButtonState == LOW) {
        //if (debug) { Serial.println("Rotary Encoder Button Pressed"); }
        encoderPos = 0;
        float freq = inputToPitch(encoderPos);
        if (debug) {
          Serial.print("Frequency (hz): ");
          Serial.println(freq);
        }
        waveform1.frequency(freq);
      } else {
        if (debug) { Serial.println("Rotary Encoder Button Released"); }
      }
    }
  }
  encoderLastButtonState = buttonReading;
}

void interruptEncoder() {
  int MSB = digitalRead(encoderPinA);  // MSB = most significant bit
  int LSB = digitalRead(encoderPinB);  // LSB = least significant bit

  int encoded = (MSB << 1) | LSB;          // Converting the 2 pin values to a single number
  int sum = (lastEncoded << 2) | encoded;  // Adding it to the previous encoded value

  if (sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011) {
    encoderPos++;
  } else if (sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000) {
    encoderPos--;
  }

  lastEncoded = encoded;  // Store the value for the next time
  float freq = inputToPitch(encoderPos);
  if (debug) {
    Serial.print("Frequency (hz): ");
    Serial.println(freq);
  }
  waveform1.frequency(freq);
}