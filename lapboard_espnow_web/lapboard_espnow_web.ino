#include <esp_now.h>
#include <WiFi.h>
#include "ESPAsyncWebServer.h"
#include <Arduino_JSON.h>

// Replace with your network credentials (STATION)
const char* ssid = "ssid";
const char* password = "password";
static uint8_t broadcast_mac[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

JSONVar board;

AsyncWebServer server(80);
AsyncEventSource events("/events");

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

  // Send new data to web clients
  String jsonString = JSON.stringify(globalNumber);
  events.send(jsonString.c_str(), "new_number", millis());
}

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>ESP-NOW DASHBOARD</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
  <link rel="icon" href="data:,">
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    p {  font-size: 1.2rem;}
    body {  margin: 0;}
    .topnav { overflow: hidden; background-color: #2f4468; color: white; font-size: 1.7rem; }
    .content { padding: 20px; }
    .card { background-color: white; box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5); }
    .cards { max-width: 700px; margin: 0 auto; display: grid; grid-gap: 2rem; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); }
    .reading { font-size: 2.8rem; }
    .packet { color: #bebebe; }
    .card.temperature { color: #fd7e14; }
    .card.humidity { color: #1b78e2; }
  </style>
</head>
<body>
  <div class="topnav">
    <h3>ESP-NOW DASHBOARD</h3>
  </div>
  <div class="content">
    <div class="cards">
      <div class="card temperature">
        <h4><i class="fas fa-thermometer-half"></i> BOARD #1 - TEMPERATURE</h4><p><span class="reading"><span id="t"></span> &deg;C</span></p><p class="packet">Reading ID: <span id="rt1"></span></p>
      </div>
    </div>
  </div>
<script>
if (!!window.EventSource) {
 var source = new EventSource('/events');

 source.addEventListener('open', function(e) {
  console.log("Events Connected");
 }, false);
 source.addEventListener('error', function(e) {
  if (e.target.readyState != EventSource.OPEN) {
    console.log("Events Disconnected");
  }
 }, false);

 source.addEventListener('message', function(e) {
  console.log("message", e.data);
 }, false);

 source.addEventListener('new_number', function(e) {
  console.log("new_number", e.data);
  var obj = JSON.parse(e.data);
  document.getElementById("t").innerHTML = e.data;
 }, false);
}
</script>
</body>
</html>)rawliteral";

void setup() {
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, N_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  delay(1000);

  Serial.begin(115200);
  Serial.println();
  Serial.print("ESP Board MAC Address:  ");
  Serial.println(WiFi.macAddress());

  // Set ESP32 as a Wi-Fi Station
  WiFi.mode(WIFI_AP_STA);

  // Set device as a Wi-Fi Station
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Setting as a Wi-Fi Station..");
  }
  Serial.print("Station IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Wi-Fi Channel: ");
  Serial.println(WiFi.channel());

  // Initilize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Register callback function
  esp_now_register_recv_cb(OnDataRecv);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send_P(200, "text/html", index_html);
  });

  events.onConnect([](AsyncEventSourceClient* client) {
    if (client->lastId()) {
      Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
    }
    // send event with message "hello!", id current millis
    // and set reconnect delay to 1 second
    client->send("hello!", NULL, millis(), 10000);
  });
  server.addHandler(&events);
  server.begin();

  showNumber(globalNumber, colors, true);
}

void loop() {
  static unsigned long lastEventTime = millis();
  static const unsigned long EVENT_INTERVAL_MS = 5000;
  if ((millis() - lastEventTime) > EVENT_INTERVAL_MS) {
    events.send("ping", NULL, millis());
    lastEventTime = millis();
  }
}
