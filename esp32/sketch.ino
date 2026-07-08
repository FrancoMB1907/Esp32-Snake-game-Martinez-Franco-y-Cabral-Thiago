// ==================================================================================
// SECCIÓN 1: DECLARACIÓN DE LIBRERÍAS Y VARIABLES GLOBALES
// ==================================================================================

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <WiFi.h>
#include <HTTPClient.h>

const char* WIFI_SSID = "Wokwi-GUEST";
const char* WIFI_PASSWORD = "";
const char* supabase_url = "https://eppoqisvtsjtzdapdtld.supabase.co";
const char* supabase_key = "sb_publishable_2QOar0SV-02koklIeLy0ZQ_xglZICCj";

#define TFT_CS 15
#define TFT_DC 2
#define TFT_RST 4

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

#define BTN_UP     12
#define BTN_DOWN   13
#define BTN_LEFT   14
#define BTN_RIGHT  27
#define BUZZER_PIN 26

#define SCREEN_W 240
#define SCREEN_H 320
#define CELL_SIZE 10
#define TOP_BAR 30
#define BORDER_W 2
#define PLAY_AREA_H (SCREEN_H - TOP_BAR - (2 * BORDER_W))
#define PLAY_AREA_W (SCREEN_W - (2 * BORDER_W))
#define GRID_W (PLAY_AREA_W / CELL_SIZE)
#define GRID_H (PLAY_AREA_H / CELL_SIZE)
#define START_X BORDER_W
#define START_Y (TOP_BAR + BORDER_W)
#define BASE_SPEED 120

#define COL_BG 0x2589
#define COL_BAR ILI9341_BLACK
#define COL_SNAKE ILI9341_BLUE
#define COL_HEAD ILI9341_RED
#define COL_FOOD ILI9341_MAGENTA
#define COL_TEXT ILI9341_WHITE
#define COL_BORDER ILI9341_WHITE

struct Point {
  int x, y;
};

Point snake[400];
int snakeLength = 3;
Point food;
int dirX = 0;
int dirY = -1;
bool gameOver = false;
int score = 0;
int currentSpeed = BASE_SPEED;
Point oldTail;

const char nameChars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ .->";
int charIndex = 0;
char playerName[11] = "";
int nameLen = 0;

// ==================================================================================
// SECCIÓN 2: SETUP
// ==================================================================================
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("--- Snake Game Iniciado ---");

  Serial.print("Conectando a WiFi de Wokwi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConectado exitosamente al WiFi virtual!");

  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_LEFT, INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);

  tft.begin();
  tft.setRotation(2);
  tft.fillScreen(ILI9341_BLACK);

  ingresarNombre();

  while (digitalRead(BTN_UP) == LOW || digitalRead(BTN_DOWN) == LOW ||
         digitalRead(BTN_LEFT) == LOW || digitalRead(BTN_RIGHT) == LOW) {
    delay(10);
  }
  delay(200);

  tft.fillScreen(COL_BG);
  drawStaticUI();
  resetGame();
}

void ingresarNombre() {
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_GREEN);
  tft.setTextSize(2);
  tft.setCursor(20, 30);
  tft.print("INGRESA TU NOMBRE");

  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(1);
  tft.setCursor(20, 65);
  tft.print("UP/DOWN: letra  RIGHT: ok  LEFT: borrar");

  tft.drawFastHLine(20, 100, 200, ILI9341_WHITE);

  tft.setTextColor(ILI9341_YELLOW);
  tft.setTextSize(2);
  tft.setCursor(20, 115);
  tft.print("Nombre: ");

  nameLen = 0;
  charIndex = 0;
  bool nameDone = false;
  int lastCharIndex = -1;
  int lastNameLen = -1;

  while (!nameDone) {
    if (nameLen != lastNameLen || charIndex != lastCharIndex) {
      lastNameLen = nameLen;
      lastCharIndex = charIndex;

      tft.fillRect(20, 145, 200, 30, ILI9341_BLACK);
      tft.setTextColor(ILI9341_CYAN);
      tft.setTextSize(3);
      tft.setCursor(20, 145);
      for (int i = 0; i < 10; i++) {
        tft.print((i < nameLen) ? playerName[i] : '_');
      }

      tft.fillRect(20 + nameLen * 20, 145, 20, 26, ILI9341_NAVY);
      char ch[2] = {nameChars[charIndex], '\0'};
      tft.setTextColor(ILI9341_WHITE);
      tft.setTextSize(2);
      tft.setCursor(22 + nameLen * 20, 148);
      tft.print(ch);

      tft.fillRect(20, 190, 200, 15, ILI9341_BLACK);
      tft.setTextColor(ILI9341_WHITE);
      tft.setTextSize(1);
      tft.setCursor(20, 190);
      if (nameChars[charIndex] == '>') {
        tft.print("OK para finalizar");
      } else if (nameChars[charIndex] == ' ') {
        tft.print("Espacio");
      } else {
        tft.print("Letra: ");
        tft.print(ch);
      }
    }

    if (digitalRead(BTN_UP) == LOW) {
      charIndex = (charIndex + 1) % 30;
      delay(200);
    }
    if (digitalRead(BTN_DOWN) == LOW) {
      charIndex = (charIndex - 1 + 30) % 30;
      delay(200);
    }
    if (digitalRead(BTN_RIGHT) == LOW) {
      if (nameChars[charIndex] == '>') {
        if (nameLen == 0) strcpy(playerName, "Anonimo");
        else playerName[nameLen] = '\0';
        nameDone = true;
        tone(BUZZER_PIN, 800, 30);
        while (digitalRead(BTN_RIGHT) == LOW) delay(10);
        delay(200);
      } else if (nameLen < 10) {
        playerName[nameLen++] = nameChars[charIndex];
        charIndex = 0;
        delay(200);
      }
    }
    if (digitalRead(BTN_LEFT) == LOW) {
      if (nameLen > 0) {
        nameLen--;
        playerName[nameLen] = '\0';
        charIndex = 0;
      }
      delay(200);
    }
  }
}

// ==================================================================================
// SECCIÓN 3: LOOP
// ==================================================================================
void loop() {
  if (!gameOver) {
    handleInput();
    updateGame();
  } else {
    if (digitalRead(BTN_UP) == LOW || digitalRead(BTN_DOWN) == LOW ||
        digitalRead(BTN_LEFT) == LOW || digitalRead(BTN_RIGHT) == LOW) {
      delay(300);
      ingresarNombre();
      while (digitalRead(BTN_UP) == LOW || digitalRead(BTN_DOWN) == LOW ||
             digitalRead(BTN_LEFT) == LOW || digitalRead(BTN_RIGHT) == LOW) {
        delay(10);
      }
      delay(200);
      tft.fillScreen(COL_BG);
      drawStaticUI();
      resetGame();
    }
  }
  delay(currentSpeed);
}

// ==================================================================================
// SECCIÓN 4: FUNCIONES DEL JUEGO
// ==================================================================================

void handleInput() {
  if (digitalRead(BTN_UP) == LOW && dirY != 1) {
    dirX = 0; dirY = -1;
  }
  else if (digitalRead(BTN_DOWN) == LOW && dirY != -1) {
    dirX = 0; dirY = 1;
  }
  else if (digitalRead(BTN_LEFT) == LOW && dirX != 1) {
    dirX = -1; dirY = 0;
  }
  else if (digitalRead(BTN_RIGHT) == LOW && dirX != -1) {
    dirX = 1; dirY = 0;
  }
}

void updateGame() {
  oldTail = snake[snakeLength - 1];

  for (int i = snakeLength - 1; i > 0; i--) {
    snake[i] = snake[i - 1];
  }

  snake[0].x += dirX;
  snake[0].y += dirY;

  if (snake[0].x < 0 || snake[0].x >= GRID_W || snake[0].y < 0 || snake[0].y >= GRID_H) {
    triggerGameOver();
    return;
  }

  for (int i = 1; i < snakeLength; i++) {
    if (snake[0].x == snake[i].x && snake[0].y == snake[i].y) {
      triggerGameOver();
      return;
    }
  }

  bool ateFood = false;
  if (snake[0].x == food.x && snake[0].y == food.y) {
    ateFood = true;
    score += 10;
    snakeLength++;
    snake[snakeLength - 1] = oldTail;

    tone(BUZZER_PIN, 1000, 50);
    drawScore();

    int speedDecrease = (score / 100) * 10;
    currentSpeed = BASE_SPEED - speedDecrease;
    if (currentSpeed < 40) currentSpeed = 40;

    spawnFood();
  }

  if (!ateFood) {
    tft.fillRect(START_X + (oldTail.x * CELL_SIZE),
                 START_Y + (oldTail.y * CELL_SIZE),
                 CELL_SIZE, CELL_SIZE, COL_BG);
  }

  tft.fillRoundRect(START_X + (snake[0].x * CELL_SIZE),
                    START_Y + (snake[0].y * CELL_SIZE),
                    CELL_SIZE - 1, CELL_SIZE - 1, 2, COL_HEAD);

  if (snakeLength > 1) {
    tft.fillRoundRect(START_X + (snake[1].x * CELL_SIZE),
                      START_Y + (snake[1].y * CELL_SIZE),
                      CELL_SIZE - 1, CELL_SIZE - 1, 2, COL_SNAKE);
  }
}

void drawStaticUI() {
  tft.fillRect(0, 0, SCREEN_W, TOP_BAR, COL_BAR);
  tft.drawRect(0, TOP_BAR, SCREEN_W, SCREEN_H - TOP_BAR, COL_BORDER);
  drawScore();
}

void drawScore() {
  tft.fillRect(BORDER_W, BORDER_W, SCREEN_W - (2 * BORDER_W), TOP_BAR - BORDER_W, COL_BAR);
  tft.setTextColor(COL_TEXT, COL_BAR);
  tft.setTextSize(1);
  tft.setCursor(10, 10);
  tft.print(playerName);
  tft.setTextSize(2);
  tft.setCursor(100, 8);
  tft.print("SCORE: ");
  tft.print(score);
}

void triggerGameOver() {
  if (!gameOver) {
    tone(BUZZER_PIN, 150, 600);
    enviarPuntaje(score);
    drawGameOver();
  }
  gameOver = true;
}

void drawGameOver() {
  tft.fillScreen(ILI9341_BLACK);

  tft.setTextColor(ILI9341_RED);
  tft.setTextSize(4);
  tft.setCursor(15, SCREEN_H / 2 - 40);
  tft.print("GAME OVER");

  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.setCursor(10, SCREEN_H / 2 + 10);
  tft.print(playerName);
  tft.print(": ");
  tft.print(score);
  tft.print(" pts");

  tft.setTextSize(1);
  tft.setCursor(48, SCREEN_H / 2 + 50);
  tft.print("Press Button to Restart");
}

void spawnFood() {
  bool valid = false;
  while (!valid) {
    valid = true;
    food.x = random(0, GRID_W);
    food.y = random(0, GRID_H);

    for (int i = 0; i < snakeLength; i++) {
      if (snake[i].x == food.x && snake[i].y == food.y) {
        valid = false;
        break;
      }
    }
  }
  tft.fillCircle(START_X + (food.x * CELL_SIZE) + (CELL_SIZE / 2),
                 START_Y + (food.y * CELL_SIZE) + (CELL_SIZE / 2),
                 (CELL_SIZE / 2) - 1, COL_FOOD);
}

void resetGame() {
  tft.fillRect(BORDER_W, TOP_BAR + BORDER_W, PLAY_AREA_W, PLAY_AREA_H, COL_BG);
  drawStaticUI();
  snakeLength = 3;
  snake[0] = { GRID_W / 2, GRID_H / 2 };
  snake[1] = { GRID_W / 2, GRID_H / 2 + 1 };
  snake[2] = { GRID_W / 2, GRID_H / 2 + 2 };

  for (int i = 0; i < snakeLength; i++) {
    uint16_t c = (i == 0) ? COL_HEAD : COL_SNAKE;
    tft.fillRoundRect(START_X + (snake[i].x * CELL_SIZE),
                      START_Y + (snake[i].y * CELL_SIZE),
                      CELL_SIZE - 1, CELL_SIZE - 1, 2, c);
  }

  dirX = 0; dirY = -1; score = 0;
  drawScore();
  currentSpeed = BASE_SPEED;
  gameOver = false;
  spawnFood();
}

void enviarPuntaje(int puntajeFinal) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Error: No hay conexion WiFi para enviar los datos.");
    return;
  }

  HTTPClient http;
  String url = String(supabase_url) + "/rest/v1/partidas";

  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("apikey", supabase_key);
  http.addHeader("Authorization", "Bearer " + String(supabase_key));
  http.addHeader("Prefer", "return=minimal");

  String jsonBody = "{\"puntaje\":" + String(puntajeFinal) + ", \"jugador\":\"" + String(playerName) + "\"}";

  Serial.println("Enviando datos a Supabase...");
  int httpCode = http.POST(jsonBody);

  if (httpCode > 0) {
    Serial.print("Respuesta del servidor (HTTP Code): ");
    Serial.println(httpCode);
  } else {
    Serial.print("Error en la peticion POST: ");
    Serial.println(http.errorToString(httpCode).c_str());
  }

  http.end();
}