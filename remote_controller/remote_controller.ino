#include <TFT_eSPI.h>
#include <SPI.h>
#include "Button2.h"
#include <esp_now.h>
#include <WiFi.h>

// 24:6F:28:25:2B:40 mac address of remote
uint8_t broadcastAddress[] = { 0x30, 0xAE, 0xA4, 0x07, 0x0D, 0x64 };
esp_now_peer_info_t peerInfo;

#define BUTTON_1 35
#define BUTTON_2 0
Button2 btn1(BUTTON_1);
Button2 btn2(BUTTON_2);

TFT_eSPI tft = TFT_eSPI();

byte arr[10][5][3] = {
  { { 1, 1, 1 },  //0
    { 1, 0, 1 },
    { 1, 0, 1 },
    { 1, 0, 1 },
    { 1, 1, 1 }

  },
  { { 0, 1, 0 },  //1
    { 0, 1, 0 },
    { 0, 1, 0 },
    { 0, 1, 0 },
    { 0, 1, 0 }

  },
  { { 1, 1, 1 },  //2
    { 0, 0, 1 },
    { 1, 1, 1 },
    { 1, 0, 0 },
    { 1, 1, 1 }

  },
  { { 1, 1, 1 },  //3
    { 0, 0, 1 },
    { 1, 1, 1 },
    { 0, 0, 1 },
    { 1, 1, 1 }

  },
  { { 1, 0, 1 },  //4
    { 1, 0, 1 },
    { 1, 1, 1 },
    { 0, 0, 1 },
    { 0, 0, 1 }

  },
  { { 1, 1, 1 },  //5
    { 1, 0, 0 },
    { 1, 1, 1 },
    { 0, 0, 1 },
    { 1, 1, 1 }

  },
  { { 1, 1, 1 },  //6
    { 1, 0, 0 },
    { 1, 1, 1 },
    { 1, 0, 1 },
    { 1, 1, 1 }

  },
  { { 1, 1, 1 },  //7
    { 0, 0, 1 },
    { 0, 0, 1 },
    { 0, 0, 1 },
    { 0, 0, 1 }

  },
  { { 1, 1, 1 },  //8
    { 1, 0, 1 },
    { 1, 1, 1 },
    { 1, 0, 1 },
    { 1, 1, 1 }

  },
  { { 1, 1, 1 },  //9
    { 1, 0, 1 },
    { 1, 1, 1 },
    { 0, 0, 1 },
    { 1, 1, 1 }

  }
};

const int pwmFreq = 5000;
const int pwmResolution = 8;
const int pwmLedChannelTFT = 0;

void OnDataSent(const uint8_t* mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
  if (status == ESP_NOW_SEND_SUCCESS) {
    tft.fillCircle(10, 10, 10, TFT_GREEN);
  } else {
    tft.fillCircle(10, 10, 10, TFT_RED);
  }
}

short number = 0;

void setup(void) {
  Serial.begin(115200);
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);

  ledcSetup(pwmLedChannelTFT, pwmFreq, pwmResolution);
  ledcAttachPin(TFT_BL, pwmLedChannelTFT);
  ledcWrite(pwmLedChannelTFT, 53);

  // btn1.setDebounceTime(10);
  btn1.setTapHandler([](Button2& b) {
    number += 1;
  });

  // btn1.setDebounceTime(10);
  btn2.setTapHandler([](Button2& b) {
    number += -1;
  });

  // Set ESP32 as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Initilize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  } else {
  }

  // Register the send callback
  esp_now_register_send_cb(OnDataSent);

  // Register peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  // Add peer
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
  } else {
  }
}

short colors[2] = { TFT_BLACK, TFT_YELLOW };  //first colour is color of background , second is color of digit


int sizee = 25;     //size of each box
byte space = 3;     // space between boxes
int fromTop = 0;    //positon x
int fromLeft = 40;  //position y
int Round = 0;
short onScreenNumber = 1;

void loop() {
  if (number == 100 || number < 0) {
    number = 0;
  }

  if (number != onScreenNumber) {
    String n = String(number);

    if (number < 10) {
      n = "0" + n;
    }

    for (int z = 0; z < 2; z++) {
      for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 3; j++) {
          String c = n.substring(z, z + 1);
          int b = c.toInt();
          tft.fillRoundRect((z * (sizee * 4)) + fromLeft + (j * sizee) + (j * space), fromTop + (i * sizee) + (i * space), sizee, sizee, Round, colors[arr[b][i][j]]);
        }
      }
    }
    onScreenNumber = number;

    // Send message via ESP-NOW
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t*)&number, sizeof(number));

    if (result == ESP_OK) {
      Serial.println("Sending confirmed");
    } else {
      Serial.println("Sending error");
      tft.fillCircle(10, 30, 10, TFT_RED);
    }
  }

  btn1.loop();
  btn2.loop();
}