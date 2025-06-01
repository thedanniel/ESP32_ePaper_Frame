#include "WiFi.h"
#include "SPIFFS.h"
#include "ESPAsyncWebServer.h"
#include "credentials.h"
#include "display.h"

AsyncWebServer server(80);
AsyncWebSocket ws("/test");

TaskHandle_t Task1 = NULL;

void driveDisplay(void *parameter) {  //running on another core to avoid watchdog timer error
  while (true) {
    TurnOnDisplay();
    vTaskSuspend(Task1);
  }
}

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
               AwsEventType type, void *arg, uint8_t *data, size_t len) {

  if (type == WS_EVT_CONNECT) {
    Serial.println("Websocket client connection received");
  } else if (type == WS_EVT_DISCONNECT) {
    Serial.println("Websocket client disconnected");
  } else if (type == WS_EVT_DATA) {
    AwsFrameInfo *info = (AwsFrameInfo *)arg;
    if (info->opcode == WS_TEXT) {  // Got text data (the client checks if the display is busy)
      client->text(responseToNewImage());
    } else {  // Got binary data (the client sent the image)
      // Send the received data to the display buffer.
      loadImage((const char *)data, len);
      if ((info->index + len) == info->len) {  // if this data was the end of the file
        //client->text("IMAGE_LOADED");       // tell the client that the server got the image
        Serial.printf("Updating Display: %d bytes total\n", info->len);
        updateDisplay_withoutIdle();  // Display the image that received
      }
    }
  }
}

char *responseToNewImage() {
  if (isDisplayBusy()) {
    return "BUSY";
  } else {
    sendCommand(0x10);
    return "OK";
  }
}

void Clear(unsigned char color) {
  sendCommand(0x10);
  for (int i = 0; i < 800 / 2; i++) {
    for (int j = 0; j < 480; j++) {
      sendData((color << 4) | color);
    }
  }
  vTaskResume(Task1);  //call TurnOnDisplay()
}

// Shows the loaded image
void updateDisplay_withoutIdle() {
  // Refresh.
  vTaskResume(Task1);  //call TurnOnDisplay()
  Serial.println("Update successful");
}

void setup() {
  Serial.begin(115200);

  if (!SPIFFS.begin()) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  if (initDisplay() != 0) {
    Serial.println("Init failed");
    return;
  }

  Serial.println("Init successful");

  xTaskCreatePinnedToCore(
    driveDisplay,   /* Task function. */
    "DisplayDrive", /* name of task. */
    32000,          /* Stack size of task */
    NULL,           /* parameter of the task */
    1,              /* priority of the task */
    &Task1,         /* Task handle to keep track of created task */
    0);             /* Core */

  vTaskSuspend(Task1);

  WiFi.mode(WIFI_STA);
  WiFi.begin(wifi_credentials.ssid, wifi_credentials.password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());

  delay(2000);

  // setup handles for websockets connetions
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);

  // handle GET requests to route /index.html
  server.on("/index.html", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("Request recived: GET /index.html");
    request->send(SPIFFS, "/index.html", "text/html; charset=utf-8");
  });

  // handle GET requests to route /
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("Request recived: GET /");
    request->send(SPIFFS, "/index.html", "text/html");
  });

  // handle GET requests to route /hello - echo "Hello Word" (for debugging)
  server.on("/hello", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("Request recived: GET /hello");
    request->send(200, "text/plain", "Hello World");
  });

  server.begin();  //start listening to incoming HTTP requests
}

void loop() {
  //delay(100);
}
