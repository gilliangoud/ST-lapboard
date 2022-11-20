#define FASTLED_ESP32_I2S true
//#define FASTLED_ALLOW_INTERRUPTS 0
#define FASTLED_SHOW_CORE 1
#include "src/FASTLED/FastLED.h"
#define LED_PIN 13
// #define N_LEDS 98
#define N_LEDS 56 
// #define N_LEDS_SEGMENT 7
#define N_LEDS_SEGMENT 4 
#define N_DISPLAYS 2
CRGB leds[N_LEDS];
static CRGB color = CRGB::White;
static CRGB colors[2] = {CRGB::White, CRGB::Red};
int globalNumber = 9;
int BRIGHTNESS = 20; //brightness  (max 254) 

#if defined(ARDUINO_ARCH_ESP8266)
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#define GET_CHIPID()  (ESP.getChipId())
#elif defined(ARDUINO_ARCH_ESP32)
#include <WiFi.h>
#include <SPIFFS.h>
#include <HTTPClient.h>
#define GET_CHIPID()  ((uint16_t)(ESP.getEfuseMac()>>32))
#endif
#include <FS.h>
#include <PubSubClient.h>
#include <AutoConnect.h>

#define PARAM_FILE        "/param.json"
#define AUX_SETTING_URI   "/mqtt_setting"
#define AUX_SAVE_URI      "/mqtt_save"
#define AUX_CLEAR_URI     "/mqtt_clear"
#define LAPBOARD_URI      "/lapboard"
#define LAPBOARD_S_URI    "/lapboard/setting"
#define LAPBOARD_URI_SAVE "/lapboard/save"

static const char AUX_mqtt_setting[] PROGMEM = R"raw(
[
  {
    "title": "MQTT Setting",
    "uri": "/mqtt_setting",
    "menu": true,
    "element": [
      {
        "name": "style",
        "type": "ACStyle",
        "value": "label+input,label+select{position:sticky;left:120px;width:230px!important;box-sizing:border-box;}"
      },
      {
        "name": "header",
        "type": "ACText",
        "value": "<h2>MQTT broker settings</h2>",
        "style": "text-align:center;color:#2f4f4f;padding:10px;"
      },
      {
        "name": "caption",
        "type": "ACText",
        "value": "Publishing the WiFi signal strength to MQTT channel. RSSI value of ESP8266 to the channel created on ThingSpeak",
        "style": "font-family:serif;color:#4682b4;"
      },
      {
        "name": "mqttserver",
        "type": "ACInput",
        "value": "",
        "label": "Server",
        "pattern": "^(([a-zA-Z0-9]|[a-zA-Z0-9][a-zA-Z0-9\\-]*[a-zA-Z0-9])\\.)*([A-Za-z0-9]|[A-Za-z0-9][A-Za-z0-9\\-]*[A-Za-z0-9])$",
        "placeholder": "MQTT broker server"
      },
      {
        "name": "channelid",
        "type": "ACInput",
        "label": "Channel ID",
        "pattern": "^[0-9]{6}$"
      },
      {
        "name": "userkey",
        "type": "ACInput",
        "label": "User Key"
      },
      {
        "name": "apikey",
        "type": "ACInput",
        "label": "API Key"
      },
      {
        "name": "newline",
        "type": "ACElement",
        "value": "<hr>"
      },
      {
        "name": "uniqueid",
        "type": "ACCheckbox",
        "value": "unique",
        "label": "Use APID unique",
        "checked": false
      },
      {
        "name": "period",
        "type": "ACRadio",
        "value": [
          "30 sec.",
          "60 sec.",
          "180 sec."
        ],
        "label": "Update period",
        "arrange": "vertical",
        "checked": 1
      },
      {
        "name": "newline",
        "type": "ACElement",
        "value": "<hr>"
      },
      {
        "name": "hostname",
        "type": "ACInput",
        "value": "",
        "label": "ESP host name",
        "pattern": "^([a-zA-Z0-9]([a-zA-Z0-9-])*[a-zA-Z0-9]){1,24}$"
      },
      {
        "name": "save",
        "type": "ACSubmit",
        "value": "Save&amp;Start",
        "uri": "/mqtt_save"
      },
      {
        "name": "discard",
        "type": "ACSubmit",
        "value": "Discard",
        "uri": "/"
      }
    ]
  },
  {
    "title": "MQTT Setting",
    "uri": "/mqtt_save",
    "menu": false,
    "element": [
      {
        "name": "caption",
        "type": "ACText",
        "value": "<h4>Parameters saved as:</h4>",
        "style": "text-align:center;color:#2f4f4f;padding:10px;"
      },
      {
        "name": "parameters",
        "type": "ACText"
      },
      {
        "name": "clear",
        "type": "ACSubmit",
        "value": "Clear channel",
        "uri": "/mqtt_clear"
      }
    ]
  },
  {
    "title": "Lapboard",
    "uri": "/lapboard",
    "menu": true,
    "element": [
      {
        "name": "saveLedss",
        "type": "ACSubmit",
        "value": "settings",
        "uri": "/lapboard/setting"
      },
      {
        "name": "newline",
        "type": "ACElement",
        "value": "<hr>"
      },
      {
        "name": "number",
        "type": "ACInput",
        "value": "99",
        "label": "Number",
        "pattern": "^([0-9]*){1,24}$"
      },
      {
        "name": "down",
        "type": "ACSubmit",
        "value": "Decrease",
        "uri": "/down"
      },
      {
        "name": "up",
        "type": "ACSubmit",
        "value": "Increase",
        "uri": "/up"
      },
      {
        "name": "set",
        "type": "ACSubmit",
        "value": "Set",
        "uri": "/set"
      }
    ]
  },
  {
    "title": "Lapboard",
    "uri": "/lapboard/save",
    "menu": false,
    "element": [
      {
        "name": "captionL",
        "type": "ACText",
        "value": "<h4>Parameters saved as:</h4>",
        "style": "text-align:center;color:#2f4f4f;padding:10px;"
      },
      {
        "name": "parametersL",
        "type": "ACText"
      },
      {
        "name": "backL",
        "type": "ACSubmit",
        "value": "back",
        "uri": "/lapboard"
      }
    ]
  },
  {
    "title": "Lapboard",
    "uri": "/lapboard/setting",
    "menu": false,
    "element": [
      {
        "name": "style",
        "type": "ACStyle",
        "value": "label+input,label+select{position:sticky;left:120px;width:230px!important;box-sizing:border-box;}"
      },
      {
        "name": "red1",
        "type": "ACInput",
        "value": "255",
        "label": "R",
        "pattern": "^([01]?[0-9]?[0-9]|2[0-4][0-9]|25[0-5])$"
      },
      {
        "name": "green1",
        "type": "ACInput",
        "value": "255",
        "label": "G",
        "pattern": "^([01]?[0-9]?[0-9]|2[0-4][0-9]|25[0-5])$"
      },
      {
        "name": "blue1",
        "type": "ACInput",
        "value": "255",
        "label": "B",
        "pattern": "^([01]?[0-9]?[0-9]|2[0-4][0-9]|25[0-5])$"
      },
      {
        "name": "newline",
        "type": "ACElement",
        "value": "<hr>"
      },
      {
        "name": "red2",
        "type": "ACInput",
        "value": "255",
        "label": "R",
        "pattern": "^([01]?[0-9]?[0-9]|2[0-4][0-9]|25[0-5])$"
      },
      {
        "name": "green2",
        "type": "ACInput",
        "value": "255",
        "label": "G",
        "pattern": "^([01]?[0-9]?[0-9]|2[0-4][0-9]|25[0-5])$"
      },
      {
        "name": "blue2",
        "type": "ACInput",
        "value": "255",
        "label": "B",
        "pattern": "^([01]?[0-9]?[0-9]|2[0-4][0-9]|25[0-5])$"
      },
      {
        "name": "newline",
        "type": "ACElement",
        "value": "<hr>"
      },
      {
        "name": "brightness",
        "type": "ACRadio",
        "value": [
          "60",
          "120",
          "200"
        ],
        "label": "Brightness  ",
        "arrange": "horizontal",
        "checked": 1
      },
      {
        "name": "saveLed",
        "type": "ACSubmit",
        "value": "save",
        "uri": "/lapboard/save"
      }
    ]
  }
]
)raw";

#if defined(ARDUINO_ARCH_ESP8266)
typedef ESP8266WebServer  WiFiWebServer;
#elif defined(ARDUINO_ARCH_ESP32)
typedef WebServer WiFiWebServer;
#endif

AutoConnect  portal;
AutoConnectConfig config;
WiFiClient   wifiClient;
PubSubClient mqttClient(wifiClient);
String  serverName;
String  channelId;
String  userKey;
String  apiKey;
String  apid;
String  hostName;
bool  uniqueid;
unsigned int  updateInterval = 0;
unsigned long lastPub = 0;

#define MQTT_USER_ID  "anyone"

bool mqttConnect() {
  static const char alphanum[] = "0123456789"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz";  // For random generation of client ID.
  char    clientId[9];

  uint8_t retry = 3;
  while (!mqttClient.connected()) {
    if (serverName.length() <= 0)
      break;

    mqttClient.setServer(serverName.c_str(), 1883);
    Serial.println(String("Attempting MQTT broker:") + serverName);

    for (uint8_t i = 0; i < 8; i++) {
      clientId[i] = alphanum[random(62)];
    }
    clientId[8] = '\0';

    if (mqttClient.connect(clientId, MQTT_USER_ID, userKey.c_str())) {
      Serial.println("Established:" + String(clientId));
      return true;
    }
    else {
      Serial.println("Connection failed:" + String(mqttClient.state()));
      if (!--retry)
        break;
      delay(3000);
    }
  }
  return false;
}

void mqttPublish(String msg) {
  String path = String("channels/") + channelId + String("/publish/") + apiKey;
  mqttClient.publish(path.c_str(), msg.c_str());
}

int getStrength(uint8_t points) {
  uint8_t sc = points;
  long    rssi = 0;

  while (sc--) {
    rssi += WiFi.RSSI();
    delay(20);
  }
  return points ? static_cast<int>(rssi / points) : 0;
}

void getParams(AutoConnectAux& aux) {
  serverName = aux["mqttserver"].value;
  serverName.trim();
  channelId = aux["channelid"].value;
  channelId.trim();
  userKey = aux["userkey"].value;
  userKey.trim();
  apiKey = aux["apikey"].value;
  apiKey.trim();
  AutoConnectRadio& period = aux["period"].as<AutoConnectRadio>();
  updateInterval = period.value().substring(0, 2).toInt() * 1000;
  uniqueid = aux["uniqueid"].as<AutoConnectCheckbox>().checked;
  hostName = aux["hostname"].value;
  hostName.trim();
}

void getLEDParams(AutoConnectAux& aux) {
//  serverName = aux["mqttserver"].value;
//  serverName.trim();
  AutoConnectRadio& period = aux["brightness"].as<AutoConnectRadio>();
  BRIGHTNESS = period.value().toInt();
  colors[0] = CRGB( aux["red1"].value.toInt(), aux["green1"].value.toInt(), aux["blue1"].value.toInt());
  colors[1] = CRGB( aux["red2"].value.toInt(), aux["green2"].value.toInt(), aux["blue2"].value.toInt());
  FastLED.setBrightness(  BRIGHTNESS );
  showNumber(globalNumber, colors, true);
}

// Load parameters saved with  saveParams from SPIFFS into the
// elements defined in /mqtt_setting JSON.
String loadParams(AutoConnectAux& aux, PageArgument& args) {
  (void)(args);
  File param = SPIFFS.open(PARAM_FILE, "r");
  if (param) {
    if (aux.loadElement(param)) {
      getParams(aux);
      Serial.println(PARAM_FILE " loaded");
    }
    else
      Serial.println(PARAM_FILE " failed to load");
    param.close();
  }
  else {
    Serial.println(PARAM_FILE " open failed");
#ifdef ARDUINO_ARCH_ESP32
    Serial.println("If you get error as 'SPIFFS: mount failed, -10025', Please modify with 'SPIFFS.begin(true)'.");
#endif
  }
  return String("");
}

String loadLEDParams(AutoConnectAux& aux, PageArgument& args) {
  (void)(args);
  File param = SPIFFS.open(PARAM_FILE, "r");
  if (param) {
    if (aux.loadElement(param)) {
      getLEDParams(aux);
      Serial.println(PARAM_FILE " loaded");
    }
    else
      Serial.println(PARAM_FILE " failed to load");
    param.close();
  }
  else {
    Serial.println(PARAM_FILE " open failed");
#ifdef ARDUINO_ARCH_ESP32
    Serial.println("If you get error as 'SPIFFS: mount failed, -10025', Please modify with 'SPIFFS.begin(true)'.");
#endif
  }
  return String("");
}

// Save the value of each element entered by '/mqtt_setting' to the
// parameter file. The saveParams as below is a callback function of
// /mqtt_save. When invoking this handler, the input value of each
// element is already stored in '/mqtt_setting'.
// In Sketch, you can output to stream its elements specified by name.
String saveParams(AutoConnectAux& aux, PageArgument& args) {
  // The 'where()' function returns the AutoConnectAux that caused
  // the transition to this page.
  AutoConnectAux&   mqtt_setting = *portal.aux(portal.where());
  getParams(mqtt_setting);
  AutoConnectInput& mqttserver = mqtt_setting["mqttserver"].as<AutoConnectInput>();

  // The entered value is owned by AutoConnectAux of /mqtt_setting.
  // To retrieve the elements of /mqtt_setting, it is necessary to get
  // the AutoConnectAux object of /mqtt_setting.
  File param = SPIFFS.open(PARAM_FILE, "w");
  mqtt_setting.saveElement(param, { "mqttserver", "channelid", "userkey", "apikey", "uniqueid", "period", "hostname" });
  param.close();

  // Echo back saved parameters to AutoConnectAux page.
  AutoConnectText&  echo = aux["parameters"].as<AutoConnectText>();
  echo.value = "Server: " + serverName;
  echo.value += mqttserver.isValid() ? String(" (OK)") : String(" (ERR)");
  echo.value += "<br>Channel ID: " + channelId + "<br>";
  echo.value += "User Key: " + userKey + "<br>";
  echo.value += "API Key: " + apiKey + "<br>";
  echo.value += "Update period: " + String(updateInterval / 1000) + " sec.<br>";
  echo.value += "Use APID unique: " + String(uniqueid == true ? "true" : "false") + "<br>";
  echo.value += "ESP host name: " + hostName + "<br>";

  return String("");
}

String saveLEDParams(AutoConnectAux& aux, PageArgument& args) {
  // The 'where()' function returns the AutoConnectAux that caused
  // the transition to this page.
  AutoConnectAux&   setting = *portal.aux(portal.where());
  getLEDParams(setting);

  File param = SPIFFS.open(PARAM_FILE, "w");
  setting.saveElement(param, { "brightness", "red1", "green1", "blue1", "red2", "green2", "blue2"});
  param.close();

  // Echo back saved parameters to AutoConnectAux page.
  AutoConnectText&  echo = aux["parametersL"].as<AutoConnectText>();
  echo.value = "Brightness: " + String(BRIGHTNESS);

  return String("");
}

String fillLEDPage(AutoConnectAux& aux, PageArgument& args) {
  aux["red1"].value = colors[0].red;
  aux["green1"].value = colors[0].green;
  aux["blue1"].value = colors[0].blue;
  aux["red2"].value = colors[1].red;
  aux["green2"].value = colors[1].green;
  aux["blue2"].value = colors[1].blue;
  aux["number"].value = globalNumber;
  return String("");
}

String fillLapPage(AutoConnectAux& aux, PageArgument& args) {
  aux["number"].value = globalNumber;
  return String("");
}

void handleRoot() {
  String  content =
    "<html>"
    "<head>"
    "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
    "</head>"
    "<body>"
    "<iframe width=\"450\" height=\"260\" style=\"transform:scale(0.79);-o-transform:scale(0.79);-webkit-transform:scale(0.79);-moz-transform:scale(0.79);-ms-transform:scale(0.79);transform-origin:0 0;-o-transform-origin:0 0;-webkit-transform-origin:0 0;-moz-transform-origin:0 0;-ms-transform-origin:0 0;border: 1px solid #cccccc;\" src=\"https://thingspeak.com/channels/454951/charts/1?bgcolor=%23ffffff&color=%23d62020&dynamic=true&type=line\"></iframe>"
    "<p style=\"padding-top:5px;text-align:center\">" AUTOCONNECT_LINK(COG_24) "</p>"
    "</body>"
    "</html>";

  WiFiWebServer&  webServer = portal.host();
//  webServer.send(200, "text/html", content);
  webServer.sendHeader("Location", String("http://") + webServer.client().localIP().toString() + String(LAPBOARD_URI));
  webServer.send(302, "text/plain", "");
  webServer.client().flush();
  webServer.client().stop();
}

// Clear channel using ThingSpeak's API.
void handleClearChannel() {
  HTTPClient  httpClient;
  WiFiClient  client;
  String  endpoint = serverName;
  endpoint.replace("mqtt", "api");
  String  delUrl = "http://" + endpoint + "/channels/" + channelId + "/feeds.json?api_key=" + userKey;

  Serial.print("DELETE " + delUrl);
  if (httpClient.begin(client, delUrl)) {
    Serial.print(":");
    int resCode = httpClient.sendRequest("DELETE");
    String  res = httpClient.getString();
    httpClient.end();
    Serial.println(String(resCode) + "," + res);
  }
  else
    Serial.println(" failed");

  // Returns the redirect response.
  WiFiWebServer&  webServer = portal.host();
  webServer.sendHeader("Location", String("http://") + webServer.client().localIP().toString() + String("/"));
  webServer.send(302, "text/plain", "");
  webServer.client().flush();
  webServer.client().stop();
}

// segments start at 1
void setSegment(int segment, CRGB color) {
  for(int i = ((segment - 1) * N_LEDS_SEGMENT); i < (segment * N_LEDS_SEGMENT); i++){
    leds[i] = color;
  }
}

void setSegments(int segments[7], CRGB color){
  for(int i = 0; i < 7; i++){
    if(segments[i] > 0){
      setSegment(segments[i], color);
    }
  }
}

// displays start at 0
// segments start at 1
void setNumber(int nDisplay, int number, CRGB color){
  int segmentOffset = nDisplay * 7;
  int number0[7] = {1 + segmentOffset,2 + segmentOffset,3 + segmentOffset,4 + segmentOffset,5 + segmentOffset,6 + segmentOffset,0};
  int number1[7] = {3 + segmentOffset,4 + segmentOffset,0,0,0,0,0};
  int number2[7] = {2 + segmentOffset,3 + segmentOffset,5 + segmentOffset,6 + segmentOffset,7 + segmentOffset,0,0};
  int number3[7] = {2 + segmentOffset,3 + segmentOffset,4 + segmentOffset,5 + segmentOffset,7 + segmentOffset,0,0};
  int number4[7] = {1 + segmentOffset,3 + segmentOffset,4 + segmentOffset,7 + segmentOffset,0,0,0};
  int number5[7] = {1 + segmentOffset,2 + segmentOffset,4 + segmentOffset,5 + segmentOffset,7 + segmentOffset,0,0};
  int number6[7] = {1 + segmentOffset,2 + segmentOffset,4 + segmentOffset,5 + segmentOffset,6 + segmentOffset,7 + segmentOffset,0};
  int number7[7] = {2 + segmentOffset,3 + segmentOffset,4 + segmentOffset,0,0,0,0};
  int number8[7] = {1 + segmentOffset,2 + segmentOffset,3 + segmentOffset,4 + segmentOffset,5 + segmentOffset,6 + segmentOffset,7 + segmentOffset};
  int number9[7] = {1 + segmentOffset,2 + segmentOffset,3 + segmentOffset,4 + segmentOffset,5 + segmentOffset,7 + segmentOffset,0};
  
  switch(number){
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

byte countDigits(int num){
  byte count=0;
  while(num){
    num=num/10;
    count++;
  }
  return count;
}

byte getDigit(unsigned int number, int digit) {
    for (int i=0; i<digit-1; i++) { 
      number /= 10; 
    }
    return number % 10;
}

void showNumber(int number, CRGB color[], bool toRight) {
  FastLED.clear();
  int nDigits = countDigits(number);
  // minus 1 because displays start at 0
  int j = 1;
  for(int i = nDigits; i >= 0; i--){
    if(toRight && number == 0){
      setNumber(1, 0, color[1]);
      Serial.println("zero");
    } else if(number > 99 || number < 0){
      setNumber(i - 1, -1, color[i-1]);
      Serial.println("above 99 or below 0");
    } else if(toRight && number < 10) {
      if(getDigit(number, j) == 0){
        setNumber(i, -1, color[i]);
        Serial.println("zero below 10");
      } else {
        setNumber(i, getDigit(number, j), color[i]);
        Serial.println("non-zero");
      }
      j++;
    } else {
      setNumber(i - 1, getDigit(number, j++), color[i-1]); 
      Serial.println("below 99");
    }
  }
  FastLED.show();
}

void handleNumberIncrease() {
  showNumber(++globalNumber, colors, true);
  WiFiWebServer&  webServer = portal.host();
  webServer.sendHeader("Location", String("http://") + webServer.client().localIP().toString() + String(LAPBOARD_URI));
  webServer.send(302, "text/plain", "");
  webServer.client().flush();
  webServer.client().stop();
}

void handleNumberDecrease() {
  showNumber(--globalNumber, colors, true);
  WiFiWebServer&  webServer = portal.host();
  webServer.sendHeader("Location", String("http://") + webServer.client().localIP().toString() + String(LAPBOARD_URI));
  webServer.send(302, "text/plain", "");
  webServer.client().flush();
  webServer.client().stop();
}

void handleNumberSet() {
  WiFiWebServer&  webServer = portal.host();
  globalNumber = webServer.arg("number").toInt();
  showNumber(globalNumber, colors, true);
  webServer.sendHeader("Location", String("http://") + webServer.client().localIP().toString() + String(LAPBOARD_URI));
  webServer.send(302, "text/plain", "");
  webServer.client().flush();
  webServer.client().stop();
}

void setup() {
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, N_LEDS);
  FastLED.setBrightness(  BRIGHTNESS );
  delay(1000);

  Serial.begin(115200);
  Serial.println();
  SPIFFS.begin(true);

  if (portal.load(FPSTR(AUX_mqtt_setting))) {
    AutoConnectAux& mqtt_setting = *portal.aux(AUX_SETTING_URI);
    PageArgument  args;
    loadParams(mqtt_setting, args);
    PageArgument  args2;
    AutoConnectAux& lappie = *portal.aux(LAPBOARD_S_URI);
    loadLEDParams(lappie, args2);
//    PageArgument  args3;
//    AutoConnectAux& lappie2 = *portal.aux(LAPBOARD_URI);
//    fillLapPage(lappie, args2);
    if (uniqueid) {
      config.apid = String("ESP") + "-" + String(GET_CHIPID(), HEX);
      Serial.println("apid set to " + config.apid);
    }
    if (hostName.length()) {
      config.hostName = hostName;
      Serial.println("hostname set to " + config.hostName);
    }
    config.bootUri = AC_ONBOOTURI_HOME;
    config.homeUri = LAPBOARD_URI;
    config.retainPortal = true;
    config.apid = "lapboard";
    config.psk  = "Gillian2";
    config.portalTimeout = 1;
    config.apip = IPAddress(10,10,10,10);
    portal.config(config);
    Serial.println("config set");

    portal.on(AUX_SETTING_URI, loadParams);
    portal.on(AUX_SAVE_URI, saveParams);
    portal.on(LAPBOARD_URI_SAVE, saveLEDParams);
    portal.on(LAPBOARD_URI, fillLapPage);
  }
  else
    Serial.println("load error");


  Serial.print("WiFi ");
  if (portal.begin()) {
    Serial.println("connected:" + WiFi.SSID());
    Serial.println("IP:" + WiFi.localIP().toString());
  }
  else {
    Serial.println("connection failed:" + String(WiFi.status()));
    Serial.println("Needs WiFi connection to start publishing messages");
  }

  WiFiWebServer&  webServer = portal.host();
  webServer.on("/", handleRoot);
  webServer.on(AUX_CLEAR_URI, handleClearChannel);
  webServer.on("/up", handleNumberIncrease);
  webServer.on("/down", handleNumberDecrease);
  webServer.on("/set", handleNumberSet);
}

void loop() { 
  if (WiFi.status() == WL_CONNECTED) {
    // MQTT publish control
    if (updateInterval > 0) {
      if (millis() - lastPub > updateInterval) {
        if (!mqttClient.connected()) {
          mqttConnect();
        }
        String item = String("field1=") + String(getStrength(7));
        mqttPublish(item);
        mqttClient.loop();
        lastPub = millis();
      }
    }
  }
  portal.handleClient();
}
