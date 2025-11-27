#ifndef display_h
#define display_h

#include <Arduino.h>
#include <SPI.h>

SPIClass hspi(HSPI);

// Cores
#define EPD_BLACK 0x0
#define EPD_WHITE 0x1
#define EPD_GREEN 0x2
#define EPD_BLUE 0x3
#define EPD_RED 0x4
#define EPD_YELLOW 0x5
#define EPD_ORANGE 0x6

// Pinos
#define PIN_SPI_SCK 46
#define PIN_SPI_DIN 8
#define PIN_SPI_CS 15
#define PIN_SPI_BUSY 14
#define PIN_SPI_RST 17
#define PIN_SPI_DC 16

#define SPI_FREQUENCY 40000000

void resetDisplay() {
  digitalWrite(PIN_SPI_RST, HIGH);
  delay(20);
  digitalWrite(PIN_SPI_RST, LOW);
  delay(5);
  digitalWrite(PIN_SPI_RST, HIGH);
  delay(20);
}

void sendCommand(byte command) {
  digitalWrite(PIN_SPI_CS, LOW);
  digitalWrite(PIN_SPI_DC, LOW);
  hspi.transfer(command);
  digitalWrite(PIN_SPI_CS, HIGH);
}

void sendData(byte data) {
  digitalWrite(PIN_SPI_CS, LOW);
  digitalWrite(PIN_SPI_DC, HIGH);
  hspi.transfer(data);
  digitalWrite(PIN_SPI_CS, HIGH);
}

void sendDataBuffer(const uint8_t* data, size_t length) {
  digitalWrite(PIN_SPI_CS, LOW);
  digitalWrite(PIN_SPI_DC, HIGH);

  hspi.transferBytes(data, NULL, length);

  digitalWrite(PIN_SPI_CS, HIGH);
}

bool isDisplayBusy() {
  return (digitalRead(PIN_SPI_BUSY) == LOW);
}

void waitBusy() {
  while (!digitalRead(PIN_SPI_BUSY)) {
    delay(10);
  }
}

int IfInit(void) {
  pinMode(PIN_SPI_CS, OUTPUT);
  pinMode(PIN_SPI_RST, OUTPUT);
  pinMode(PIN_SPI_DC, OUTPUT);
  pinMode(PIN_SPI_BUSY, INPUT);
  hspi.begin(PIN_SPI_SCK, -1, PIN_SPI_DIN, PIN_SPI_CS);
  hspi.beginTransaction(SPISettings(SPI_FREQUENCY, MSBFIRST, SPI_MODE0));
  return 0;
}

int initDisplay(void) {
  if (IfInit() != 0) return -1;
  resetDisplay();
  waitBusy();

  sendCommand(0xAA);
  sendData(0x49);
  sendData(0x55);
  sendData(0x20);
  sendData(0x08);
  sendData(0x09);
  sendData(0x18);
  sendCommand(0x01);
  sendData(0x3F);
  sendData(0x00);
  sendData(0x32);
  sendData(0x2A);
  sendData(0x0E);
  sendData(0x2A);
  sendCommand(0x00);
  sendData(0x5F);
  sendData(0x69);
  sendCommand(0x03);
  sendData(0x00);
  sendData(0x54);
  sendData(0x00);
  sendData(0x44);
  sendCommand(0x05);
  sendData(0x40);
  sendData(0x1F);
  sendData(0x1F);
  sendData(0x2C);
  sendCommand(0x06);
  sendData(0x6F);
  sendData(0x1F);
  sendData(0x1F);
  sendData(0x22);
  sendCommand(0x08);
  sendData(0x6F);
  sendData(0x1F);
  sendData(0x1F);
  sendData(0x22);
  sendCommand(0x13);
  sendData(0x00);
  sendData(0x04);
  sendCommand(0x30);
  sendData(0x3C);
  sendCommand(0x41);
  sendData(0x00);
  sendCommand(0x50);
  sendData(0x3F);
  sendCommand(0x60);
  sendData(0x02);
  sendData(0x00);
  sendCommand(0x61);
  sendData(0x03);
  sendData(0x20);
  sendData(0x01);
  sendData(0xE0);
  sendCommand(0x82);
  sendData(0x1E);
  sendCommand(0x84);
  sendData(0x00);
  sendCommand(0x86);
  sendData(0x00);
  sendCommand(0xE3);
  sendData(0x2F);
  sendCommand(0xE0);
  sendData(0x00);
  sendCommand(0xE6);
  sendData(0x00);
  return 0;
}

void ClearDisplayToWhite() {
  const size_t display_size = 800 * 480 / 2; // 800x480 pixels, 4 bits por pixel (2 pixels/byte)
  const uint8_t white_byte = (EPD_WHITE << 4) | EPD_WHITE;

  waitBusy();
  // 1. Power ON
  Serial.println("Display: Ligar para Limpar...");
  sendCommand(0x04);
  waitBusy();

  // 2. Enviar Dados (tudo Branco)
  Serial.println("Display: Enviando dados Brancos...");
  sendCommand(0x10);
  digitalWrite(PIN_SPI_CS, LOW);
  digitalWrite(PIN_SPI_DC, HIGH);
  for (size_t i = 0; i < display_size; i++) {
    hspi.transfer(white_byte);
  }
  digitalWrite(PIN_SPI_CS, HIGH);

  // 3. Refresh
  Serial.println("Display: Refresh para Limpar...");
  sendCommand(0x12);
  sendData(0x00);
  waitBusy();

  // 4. Power OFF
  Serial.println("Display: Desligar...");
  sendCommand(0x02);
  sendData(0x00);
  waitBusy();
}

void ShowColorBlocks() {
  const uint8_t Color_seven[7] = 
  {EPD_BLACK, EPD_BLUE, EPD_GREEN, EPD_ORANGE,
  EPD_RED, EPD_YELLOW, EPD_WHITE};
  
  const size_t width = 800;
  const size_t height = 480;
  const size_t pixels_per_block_h = width / 4; // 200 pixels
  const size_t pixels_per_block_v = height / 2; // 240 linhas

  waitBusy();
  // 1. Power ON
  Serial.println("Display: Ligar para Teste de Cores...");
  sendCommand(0x04);
  waitBusy();

  // 2. Enviar Imagem
  Serial.println("Display: Enviando blocos de cores...");
  sendCommand(0x10);  // Start Data Transmission
  
  // Otimização: Abre a transferência SPI para enviar os dados em bloco
  digitalWrite(PIN_SPI_CS, LOW);
  digitalWrite(PIN_SPI_DC, HIGH);
  
  // Bloco Superior (Cores 0 a 3: Preto, Azul, Verde, Laranja)
  for(size_t i = 0; i < pixels_per_block_v; i++) { // 240 linhas
    for(size_t k = 0 ; k < 4; k ++) { // 4 colunas de cores
      const uint8_t color_byte = (Color_seven[k] << 4) | Color_seven[k];
      // Cada cor ocupa 200 pixels / 2 pixels/byte = 100 bytes
      for(size_t j = 0 ; j < pixels_per_block_h / 2; j ++) { // 100 bytes
        hspi.transfer(color_byte);
      }
    }
  }

  // Bloco Inferior (Cores 4 a 7: Vermelho, Amarelo, Branco, Branco)
  for(size_t i = 0; i < pixels_per_block_v; i++) { // 240 linhas
    for(size_t k = 4 ; k < 8; k ++) { // 4 colunas de cores
      const uint8_t color_byte = (Color_seven[k] << 4) | Color_seven[k];
      // Cada cor ocupa 200 pixels / 2 pixels/byte = 100 bytes
      for(size_t j = 0 ; j < pixels_per_block_h / 2; j ++) { // 100 bytes
        hspi.transfer(color_byte);
      }
    }
  }

  digitalWrite(PIN_SPI_CS, HIGH); // Fecha a transferência SPI

  // 3. Refresh
  Serial.println("Display: Refresh...");
  sendCommand(0x12);
  sendData(0x00);
  waitBusy();

  // 4. Power OFF
  Serial.println("Display: Desligar...");
  sendCommand(0x02);
  sendData(0x00);
  waitBusy();
}

// Função principal que executa todo o ciclo
void TurnOnDisplay_Sequence(const uint8_t* image_buffer, size_t size) {
  waitBusy();
  // 1. Power ON
  Serial.println("Display: Ligar...");
  sendCommand(0x04);
  waitBusy();

  // 2. Enviar Imagem
  Serial.println("Display: Enviando dados...");
  sendCommand(0x10);  // Start Data Transmission
  sendDataBuffer(image_buffer, size);

  // 3. Refresh
  Serial.println("Display: Refresh...");
  sendCommand(0x12);
  sendData(0x00);
  waitBusy();

  // 4. Power OFF
  Serial.println("Display: Desligar...");
  sendCommand(0x02);
  sendData(0x00);
  waitBusy();
}

#endif
