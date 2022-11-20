#include <esp_now.h>
#include <WiFi.h>

#define FASTLED_ESP32_I2S true
//#define FASTLED_ALLOW_INTERRUPTS 0
#define FASTLED_SHOW_CORE 1
#include "src/FASTLED/FastLED.h"
#define LED_PIN 13
#define N_LEDS 98         // 56 for newer models
#define N_LEDS_SEGMENT 7  // 4 leds for newer models
#define N_DISPLAYS 2
CRGB leds[N_LEDS];
static CRGB color = CRGB::White;
// static CRGB colors[2] = {CRGB::White, CRGB::Red};
static CRGB colors[2] = { CRGB::Red, CRGB::Red };
int globalNumber = 9;
int BRIGHTNESS = 40;  //brightness  (max 254)

// segments start at 1
void setSegment(int segment, CRGB color) {
  for (int i = ((segment - 1) * N_LEDS_SEGMENT); i < (segment * N_LEDS_SEGMENT); i++) {
    leds[i] = color;
  }
}

void setSegments(int segments[7], CRGB color) {
  for (int i = 0; i < 7; i++) {
    if (segments[i] > 0) {
      setSegment(segments[i], color);
    }
  }
}

// displays start at 0
// segments start at 1
void setNumber(int nDisplay, int number, CRGB color) {
  int segmentOffset = nDisplay * 7;
  int number0[7] = { 1 + segmentOffset, 2 + segmentOffset, 3 + segmentOffset, 4 + segmentOffset, 5 + segmentOffset, 6 + segmentOffset, 0 };
  int number1[7] = { 3 + segmentOffset, 4 + segmentOffset, 0, 0, 0, 0, 0 };
  int number2[7] = { 2 + segmentOffset, 3 + segmentOffset, 5 + segmentOffset, 6 + segmentOffset, 7 + segmentOffset, 0, 0 };
  int number3[7] = { 2 + segmentOffset, 3 + segmentOffset, 4 + segmentOffset, 5 + segmentOffset, 7 + segmentOffset, 0, 0 };
  int number4[7] = { 1 + segmentOffset, 3 + segmentOffset, 4 + segmentOffset, 7 + segmentOffset, 0, 0, 0 };
  int number5[7] = { 1 + segmentOffset, 2 + segmentOffset, 4 + segmentOffset, 5 + segmentOffset, 7 + segmentOffset, 0, 0 };
  int number6[7] = { 1 + segmentOffset, 2 + segmentOffset, 4 + segmentOffset, 5 + segmentOffset, 6 + segmentOffset, 7 + segmentOffset, 0 };
  int number7[7] = { 2 + segmentOffset, 3 + segmentOffset, 4 + segmentOffset, 0, 0, 0, 0 };
  int number8[7] = { 1 + segmentOffset, 2 + segmentOffset, 3 + segmentOffset, 4 + segmentOffset, 5 + segmentOffset, 6 + segmentOffset, 7 + segmentOffset };
  int number9[7] = { 1 + segmentOffset, 2 + segmentOffset, 3 + segmentOffset, 4 + segmentOffset, 5 + segmentOffset, 7 + segmentOffset, 0 };

  switch (number) {
    case 0:
      setSegments(number0, color);
      break;
    case 1:
      setSegments(number1, color);
      break;
    case 2:
      setSegments(number2, color);
      break;
    case 3:
      setSegments(number3, color);
      break;
    case 4:
      setSegments(number4, color);
      break;
    case 5:
      setSegments(number5, color);
      break;
    case 6:
      setSegments(number6, color);
      break;
    case 7:
      setSegments(number7, color);
      break;
    case 8:
      setSegments(number8, color);
      break;
    case 9:
      setSegments(number9, color);
      break;
    default:
      setSegments(number8, CRGB::Black);
      break;
  }
}

byte countDigits(int num) {
  byte count = 0;
  while (num) {
    num = num / 10;
    count++;
  }
  return count;
}

byte getDigit(unsigned int number, int digit) {
  for (int i = 0; i < digit - 1; i++) {
    number /= 10;
  }
  return number % 10;
}

void showNumber(int number, CRGB color[], bool toRight) {
  FastLED.clear();
  int nDigits = countDigits(number);
  // minus 1 because displays start at 0
  int j = 1;
  for (int i = nDigits; i >= 0; i--) {
    if (toRight && number == 0) {
      setNumber(1, 0, color[1]);
      // Serial.println("zero");
    } else if (number > 99 || number < 0) {
      setNumber(i - 1, -1, color[i - 1]);
      // Serial.println("above 99 or below 0");
    } else if (toRight && number < 10) {
      if (getDigit(number, j) == 0) {
        setNumber(i, -1, color[i]);
        // Serial.println("zero below 10");
      } else {
        setNumber(i, getDigit(number, j), color[i]);
        // Serial.println("non-zero");
      }
      j++;
    } else {
      setNumber(i - 1, getDigit(number, j++), color[i - 1]);
      // Serial.println("below 99");
    }
  }
  FastLED.show();
}

// Handle receiving instructions from controller
void OnDataRecv(const uint8_t* mac, const uint8_t* incomingData, int len) {
  memcpy(&globalNumber, incomingData, sizeof(globalNumber));
  Serial.print("Received new number from remote control: ");
  Serial.println(globalNumber);
  showNumber(globalNumber, colors, true);
}

void setup() {
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, N_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  delay(1000);

  Serial.begin(115200);
  Serial.println();
  Serial.print("ESP Board MAC Address:  ");
  Serial.println(WiFi.macAddress());
  Serial.println(WiFi.channel());

  // Set ESP32 as a Wi-Fi Station
  WiFi.mode(WIFI_AP_STA);

  // Initilize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Register callback function
  esp_now_register_recv_cb(OnDataRecv);

  showNumber(globalNumber, colors, true);
}

void loop() {
}
