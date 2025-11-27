#include "WiFi.h"
#include "LittleFS.h"
#include "ESPAsyncWebServer.h"
#include "credentials.h"
#include "display.h"

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

SemaphoreHandle_t displayUpdateSemaphore;
uint8_t *framebuffer = nullptr;
const size_t FRAMEBUFFER_SIZE = 192000;

// 0: Nenhum, 1: CLEAR_SCREEN, 2: COLOR_TEST
volatile int maintenanceCommand = 0;

void driveDisplay(void *parameter) {
  while (true) {
    if (xSemaphoreTake(displayUpdateSemaphore, portMAX_DELAY) == pdTRUE) {
      Serial.println("Display: Atualizando...");
      
      if (maintenanceCommand == 1) {
        ClearDisplayToWhite();
        maintenanceCommand = 0;
        
      } else if (maintenanceCommand == 2) {
        ShowColorBlocks();
        maintenanceCommand = 0;
        
      } else if (framebuffer != nullptr) {
        TurnOnDisplay_Sequence(framebuffer, FRAMEBUFFER_SIZE);
      }
      
      Serial.println("Display: Concluído.");
    }
  }
}

void handleMaintenanceCommand(AsyncWebSocketClient *client, const char *command) {
  if (isDisplayBusy()) {
    client->text("BUSY");
    return;
  }
  
  if (strcmp(command, "CLEAR_SCREEN") == 0) {
    maintenanceCommand = 1;
    
  } else if (strcmp(command, "COLOR_TEST") == 0) {
    maintenanceCommand = 2;
    
  } else {
    client->text("OK");
    return;
  }
  
  if (uxSemaphoreGetCount(displayUpdateSemaphore) == 0) {
    xSemaphoreGive(displayUpdateSemaphore);
    client->text("OK_UPDATED");
  } else {
    client->text("BUSY_QUEUE");
  }
}


void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
               AwsEventType type, void *arg, uint8_t *data, size_t len) {

  if (type == WS_EVT_CONNECT) {
    Serial.printf("WS: Cliente %u conectado\n", client->id());
  } else if (type == WS_EVT_DISCONNECT) {
    Serial.printf("WS: Cliente %u desconectado\n", client->id());
  } else if (type == WS_EVT_DATA) {
    AwsFrameInfo *info = (AwsFrameInfo *)arg;
    
    if (info->opcode == WS_TEXT) {
      String msg = "";
      if (len > 0) {
        msg = String((char*)data).substring(0, len);
      }
      
      if (msg.equals("Can I send data?") || msg.equals("OK")) {
        client->text(isDisplayBusy() ? "BUSY" : "OK");
      } else if (msg.equals("CLEAR_SCREEN") || msg.equals("COLOR_TEST")) {
        handleMaintenanceCommand(client, msg.c_str());
      } else {
         client->text("OK");
      }
      
    } else {
      if (framebuffer == nullptr) return;
      if ((info->index + len) <= FRAMEBUFFER_SIZE) {
        memcpy(framebuffer + info->index, data, len);
      }

      if ((info->index + len) == info->len) {
        Serial.printf("Imagem recebida: %llu bytes.\n", info->len);
        if (uxSemaphoreGetCount(displayUpdateSemaphore) == 0) {
          xSemaphoreGive(displayUpdateSemaphore);
          client->text("OK_UPDATED");
        } else {
          client->text("BUSY_QUEUE");
        }
      }
    }
  }
}

void setup() {
  Serial.begin(115200);

  // 1. Memória: Prioriza PSRAM
  if (psramFound()) {
    framebuffer = (uint8_t *)ps_malloc(FRAMEBUFFER_SIZE);
    Serial.println("Memória: PSRAM alocada.");
  } else {
    framebuffer = (uint8_t *)malloc(FRAMEBUFFER_SIZE);
    Serial.println("Memória: RAM interna alocada.");
  }

  if (!framebuffer) {
    Serial.println("ERRO FATAL: Falha de memória.");
    while (1) delay(1000);
  }

  memset(framebuffer, (EPD_WHITE << 4) | EPD_WHITE, FRAMEBUFFER_SIZE);

  displayUpdateSemaphore = xSemaphoreCreateBinary();

  // 2. Sistema de Arquivos
  if (!LittleFS.begin()) {
    Serial.println("ERRO: LittleFS não montado");
  }

  // 3. Display
  if (initDisplay() != 0) {
    Serial.println("ERRO: Display não detectado.");
  }

  // 4. Task Display (Core 0 para não travar WiFi no Core 1)
  xTaskCreatePinnedToCore(driveDisplay, "DisplayTask", 4096, NULL, 1, NULL, 0);

  // 5. WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifi_credentials.ssid, wifi_credentials.password);
  Serial.print("WiFi: Conectando");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nIP: " + WiFi.localIP().toString());

  ws.onEvent(onWsEvent);
  server.addHandler(&ws);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse(LittleFS, "/index.html.gz", "text/html");
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });
  server.on("/index.html", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse(LittleFS, "/index.html.gz", "text/html");
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });
  server.on("/settings.html", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse(LittleFS, "/settings.html.gz", "text/html");
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });

  server.begin();
}

void loop() {
	// ...
}
