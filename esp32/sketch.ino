// ==================================================================================
// SECCIÓN 1: DECLARACIÓN DE LIBRERÍAS Y VARIABLES GLOBALES
// ==================================================================================

// --- Librerías ---

#include <SPI.h>
// Habilita el protocolo SPI (Serial Peripheral Interface) por hardware en el ESP32.
// Es el canal de comunicación de alta velocidad que usa la pantalla ILI9341
// para recibir los datos gráficos desde el microcontrolador.

#include <Adafruit_GFX.h>
// Librería gráfica base de Adafruit. Provee las funciones fundamentales de dibujo:
// rectángulos, círculos, texto, líneas. Adafruit_ILI9341 la usa internamente.

#include <Adafruit_ILI9341.h>
// Driver específico para el controlador de pantalla ILI9341.
// Traduce las funciones gráficas al protocolo SPI que entiende el hardware.

#include <WiFi.h>
// Librería nativa del ESP32 para manejar la conexión a redes inalámbricas.
// Permite conectarse a "Wokwi-GUEST" (red virtual de Wokwi) y hacer
// requests HTTP reales a servicios en internet como Supabase.

#include <HTTPClient.h>
// Cliente HTTP para el ESP32. Permite construir y enviar peticiones
// HTTP POST con headers y cuerpo JSON hacia la API REST de Supabase.

// --- Credenciales de red y base de datos ---

const char* WIFI_SSID = "Wokwi-GUEST";
// SSID de la red WiFi virtual de Wokwi. Esta red tiene salida real a internet,
// lo que permite que el ESP32 simulado mande datos a Supabase en la nube.

const char* WIFI_PASSWORD = "";
// Contraseña vacía: la red Wokwi-GUEST es abierta, no requiere autenticación.

const char* supabase_url = "https://eppoqisvtsjtzdapdtld.supabase.co";
// URL base del proyecto en Supabase (PostgreSQL en la nube).
// Todos los requests a la API REST se construyen a partir de esta URL.

const char* supabase_key = "sb_publishable_2QOar0SV-02koklIeLy0ZQ_xglZICCj";
// Clave pública (anon key) del proyecto de Supabase.
// Se envía en los headers de cada request para autenticar al cliente.

// --- Pines de la pantalla TFT (Bus SPI) ---

#define TFT_CS 15
// Chip Select: cuando está en LOW, la pantalla queda habilitada
// para recibir datos por el bus SPI.

#define TFT_DC 2
// Data/Command: distingue si los bytes recibidos son comandos de
// configuración (LOW) o datos gráficos a renderizar (HIGH).

#define TFT_RST 4
// Reset: al activarse, reinicia el controlador ILI9341 a su estado inicial.
// Se dispara automáticamente durante tft.begin().

// Objeto principal de la pantalla. Los pines MOSI (23), SCK (18) y MISO (19)
// son los pines SPI hardware del ESP32 y los usa la librería automáticamente.
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

// --- Pines de botones y buzzer ---

#define BTN_UP     12
// GPIO12: botón de dirección ARRIBA. Con INPUT_PULLUP lee HIGH en reposo
// y LOW al presionar (el botón conecta el pin a GND).

#define BTN_DOWN   13
// GPIO13: botón de dirección ABAJO.

#define BTN_LEFT   14
// GPIO14: botón de dirección IZQUIERDA.

#define BTN_RIGHT  27
// GPIO27: botón de dirección DERECHA.
// Se eligieron estos GPIOs por ser estables en el ESP32 DevKit C v4,
// sin interferir con la memoria Flash ni el proceso de arranque.

#define BUZZER_PIN 26
// GPIO26: salida PWM para el buzzer pasivo.
// tone() genera señales de frecuencia variable: aguda al comer (1000Hz),
// grave en Game Over (150Hz), confirmación de nombre (800Hz).

// --- Constantes del tablero de juego ---

#define SCREEN_W 240  // Ancho total de la pantalla en píxeles (modo Portrait)
#define SCREEN_H 320  // Alto total de la pantalla en píxeles (modo Portrait)
#define CELL_SIZE 10  // Tamaño de cada celda cuadrada del tablero (10x10 px)
#define TOP_BAR 30  // Altura reservada para la barra superior de puntaje
#define BORDER_W 2  // Grosor del borde blanco que delimita el área de juego

// Cálculos automáticos del área jugable:
#define PLAY_AREA_H (SCREEN_H - TOP_BAR - (2 * BORDER_W))
// Alto neto jugable = pantalla total - barra superior - bordes superior e inferior

#define PLAY_AREA_W (SCREEN_W - (2 * BORDER_W))
// Ancho neto jugable = pantalla total - bordes izquierdo y derecho

#define GRID_W (PLAY_AREA_W / CELL_SIZE)
// Cantidad de columnas de la grilla lógica (23 celdas)

#define GRID_H (PLAY_AREA_H / CELL_SIZE)
// Cantidad de filas de la grilla lógica (28 celdas)

#define START_X BORDER_W
// Coordenada X en píxeles donde comienza el área jugable

#define START_Y (TOP_BAR + BORDER_W)
// Coordenada Y en píxeles donde comienza el área jugable

#define BASE_SPEED 120
// Velocidad inicial del juego: ms de espera entre cada ciclo del loop().
// Se reduce dinámicamente conforme aumenta el puntaje.

// --- Paleta de colores (formato RGB565, 16 bits) ---

#define COL_BG 0x2589 // Fondo del tablero: gris azulado oscuro
#define COL_BAR ILI9341_BLACK // Fondo de la barra de puntaje: negro
#define COL_SNAKE ILI9341_BLUE  // Cuerpo de la serpiente: azul
#define COL_HEAD ILI9341_RED  // Cabeza de la serpiente: rojo
#define COL_FOOD ILI9341_MAGENTA  // Comida: magenta
#define COL_TEXT ILI9341_WHITE  // Texto de la interfaz: blanco
#define COL_BORDER ILI9341_WHITE  // Borde del tablero: blanco

// --- Estructura de coordenadas ---

struct Point {
  int x, y;
};
// Estructura personalizada para representar una posición en la grilla lógica.
// x = columna (0 a GRID_W-1), y = fila (0 a GRID_H-1).

// --- Variables globales de estado del juego ---

Point snake[400];
// Array que almacena la posición de cada segmento de la serpiente.
// snake[0] = cabeza, snake[snakeLength-1] = cola.
// Capacidad máxima: 400 segmentos.

int snakeLength = 3;
// Longitud actual de la serpiente. Inicia en 3 y crece al comer.

Point food;
// Posición actual de la comida en la grilla. Se regenera al ser consumida.

int dirX = 0;
// Componente horizontal del vector de movimiento: -1 izquierda, 0 estático, 1 derecha.

int dirY = -1;
// Componente vertical del vector de movimiento: -1 arriba, 0 estático, 1 abajo.
// Inicia en -1: la serpiente arranca moviéndose hacia arriba.

bool gameOver = false;
// Bandera de estado. false = partida en curso, true = el jugador perdió.

int score = 0;
// Puntaje acumulado. Aumenta 10 por cada fruta consumida.

int currentSpeed = BASE_SPEED;
// Velocidad actual del loop en ms. Disminuye cada 100 puntos (más rápido).

Point oldTail;
// Guarda temporalmente la posición de la cola antes de cada movimiento.
// Se usa para borrar visualmente solo esa celda en lugar de redibujar toda la pantalla.

// --- Variables para ingreso de nombre del jugador ---

const char nameChars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ .->";
// Conjunto de caracteres disponibles para el selector de letras.
// El '>' al final funciona como botón de confirmación.

int charIndex = 0;
// Índice del carácter actualmente seleccionado en nameChars[].

char playerName[11] = "";
// Nombre del jugador ingresado (máximo 10 caracteres + terminador nulo).

int nameLen = 0;
// Cantidad de letras ya ingresadas en playerName[].

// ==================================================================================
// SECCIÓN 2: SETUP
// ==================================================================================
void setup() {
  Serial.begin(115200);
  // Inicializa el canal serial a 115200 baudios para mensajes de depuración.
  // Permite ver el estado de la conexión WiFi y las respuestas de Supabase
  // en el monitor serial de Wokwi o del IDE.

  delay(500);
  // Pausa de 500ms para que todos los periféricos del ESP32 terminen
  // su inicialización interna antes de continuar.

  Serial.println("--- Snake Game Iniciado ---");

  // --- Conexión WiFi ---
  Serial.print("Conectando a WiFi de Wokwi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  // Inicia la conexión a la red virtual "Wokwi-GUEST".

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    // Espera activa: repite hasta que el ESP32 obtenga IP y confirme conexión.
    // En Wokwi esto tarda típicamente 1-3 segundos.
  }
  Serial.println("\nConectado exitosamente al WiFi virtual!");

  // --- Configuración de pines de botones ---
  // INPUT_PULLUP activa la resistencia interna del ESP32.
  // El pin lee HIGH en reposo y LOW al presionar (el botón conecta a GND).
  // Elimina la necesidad de resistencias externas en el circuito.
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_LEFT, INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);

  pinMode(BUZZER_PIN, OUTPUT);
  // Configura el pin del buzzer como salida digital para recibir señal PWM.

  // --- Inicialización de la pantalla TFT ---
  tft.begin();
  // Envía la secuencia de comandos de arranque al controlador ILI9341
  // y configura el bus SPI hardware.

  tft.setRotation(2);
  // Establece orientación Portrait invertida (180°).
  // Con este valor la pantalla queda vertical con el conector hacia arriba.

  tft.fillScreen(ILI9341_BLACK);
  // Limpia la pantalla con fondo negro antes de mostrar la pantalla de nombre.

  // --- Pantalla de ingreso de nombre ---
  ingresarNombre();
  // Muestra el selector de letras y espera que el jugador ingrese su nombre
  // usando los botones físicos antes de comenzar la partida.

  // Anti-rebote: espera a que todos los botones estén en reposo
  // antes de continuar, para evitar lecturas falsas al salir de ingresarNombre().
  while (digitalRead(BTN_UP) == LOW || digitalRead(BTN_DOWN) == LOW ||
         digitalRead(BTN_LEFT) == LOW || digitalRead(BTN_RIGHT) == LOW) {
    delay(10);
  }
  delay(200);

  // --- Inicialización del tablero ---
  tft.fillScreen(COL_BG); // Fondo gris azulado del área de juego
  drawStaticUI(); // Dibuja la barra superior y el borde del tablero
  resetGame();  // Posiciona la serpiente y genera la primera fruta
}

/*
 * ingresarNombre()
 * Muestra una interfaz de selector de letras en la pantalla TFT.
 * El jugador usa UP/DOWN para ciclar por las letras, RIGHT para confirmar
 * cada letra, LEFT para borrar, y RIGHT sobre '>' para finalizar el nombre.
 * Si no ingresa nada, el nombre queda como "Anonimo".
 */

void ingresarNombre() {
  tft.fillScreen(ILI9341_BLACK);

  // Título de la pantalla de ingreso
  tft.setTextColor(ILI9341_GREEN);
  tft.setTextSize(2);
  tft.setCursor(20, 30);
  tft.print("INGRESA TU NOMBRE");

  // Instrucciones de uso de los botones
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(1);
  tft.setCursor(20, 65);
  tft.print("UP/DOWN: letra  RIGHT: ok  LEFT: borrar");

  tft.drawFastHLine(20, 100, 200, ILI9341_WHITE);
  // Línea separadora decorativa

  tft.setTextColor(ILI9341_YELLOW);
  tft.setTextSize(2);
  tft.setCursor(20, 115);
  tft.print("Nombre: ");

  // Reinicia las variables de estado del nombre para una nueva entrada
  nameLen = 0;
  charIndex = 0;
  bool nameDone = false;
  int lastCharIndex = -1;
  int lastNameLen = -1;

  while (!nameDone) {
    // Solo redibuja la pantalla cuando cambia el estado (optimización gráfica)
    if (nameLen != lastNameLen || charIndex != lastCharIndex) {
      lastNameLen = nameLen;
      lastCharIndex = charIndex;

      // Dibuja los 10 slots del nombre: letras ingresadas o guión bajo
      tft.fillRect(20, 145, 200, 30, ILI9341_BLACK);
      tft.setTextColor(ILI9341_CYAN);
      tft.setTextSize(3);
      tft.setCursor(20, 145);
      for (int i = 0; i < 10; i++) {
        tft.print((i < nameLen) ? playerName[i] : '_');
      }

      // Resalta la celda activa con fondo navy y muestra la letra seleccionada
      tft.fillRect(20 + nameLen * 20, 145, 20, 26, ILI9341_NAVY);
      char ch[2] = {nameChars[charIndex], '\0'};
      tft.setTextColor(ILI9341_WHITE);
      tft.setTextSize(2);
      tft.setCursor(22 + nameLen * 20, 148);
      tft.print(ch);

      // Muestra la descripción del carácter actual debajo del selector
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

    // BTN_UP: avanza al siguiente carácter en el array nameChars[]
    if (digitalRead(BTN_UP) == LOW) {
      charIndex = (charIndex + 1) % 30;
      delay(200);
    }
    // BTN_DOWN: retrocede al carácter anterior (con wrap-around circular)
    if (digitalRead(BTN_DOWN) == LOW) {
      charIndex = (charIndex - 1 + 30) % 30;
      delay(200);
    }
    // BTN_RIGHT: confirma la letra actual o finaliza el nombre si es '>'
    if (digitalRead(BTN_RIGHT) == LOW) {
      if (nameChars[charIndex] == '>') {
        // El jugador eligió confirmar el nombre completo
        if (nameLen == 0) strcpy(playerName, "Anonimo");
        // Si no ingresó nada, asigna "Anonimo" por defecto
        else playerName[nameLen] = '\0';
        // Cierra el string C con el terminador nulo
        nameDone = true;
        tone(BUZZER_PIN, 800, 30);  // Tono de confirmación
        while (digitalRead(BTN_RIGHT) == LOW) delay(10);  // Espera que suelte
        delay(200);
      } else if (nameLen < 10) {
        playerName[nameLen++] = nameChars[charIndex]; // Agrega la letra
        charIndex = 0;  // Vuelve al inicio del abecedario
        delay(200);
      }
    }
    // BTN_LEFT: borra la última letra ingresada
    if (digitalRead(BTN_LEFT) == LOW) {
      if (nameLen > 0) {
        nameLen--;
        playerName[nameLen] = '\0'; // Elimina el último carácter
        charIndex = 0;
      }
      delay(200);
    }
  }
}

// ==================================================================================
// SECCIÓN 3: LOOP
// ==================================================================================
/*
 * El loop controla dos estados mutuamente excluyentes mediante el flag gameOver:
 *
 * ESTADO ACTIVO (!gameOver):
 *   1. Lee los botones y actualiza la dirección → handleInput()
 *   2. Mueve la serpiente, verifica colisiones y procesa la comida → updateGame()
 *
 * ESTADO GAME OVER (gameOver == true):
 *   Espera que el jugador presione cualquier botón para reiniciar.
 *   Al reiniciar, llama a ingresarNombre() para una nueva partida.
 *
 * delay(currentSpeed) al final de cada ciclo controla la velocidad del juego.
 * Con BASE_SPEED = 120ms la serpiente se mueve aproximadamente 8 veces por segundo.
 */

void loop() {
  if (!gameOver) {
    // Partida en curso: procesa input y actualiza el estado del juego
    handleInput();
    updateGame();
  } else {
    // Game Over: espera input del jugador para reiniciar
    if (digitalRead(BTN_UP) == LOW || digitalRead(BTN_DOWN) == LOW ||
        digitalRead(BTN_LEFT) == LOW || digitalRead(BTN_RIGHT) == LOW) {
      delay(300);
      // Anti-debounce: pausa para evitar que la misma pulsación que cierra
      // el Game Over sea leída como dirección de movimiento al reiniciar.

      ingresarNombre();
      // Vuelve a mostrar el selector para que el jugador ingrese
      // un nombre nuevo antes de cada partida.

      // Espera a que se suelten todos los botones antes de continuar
      while (digitalRead(BTN_UP) == LOW || digitalRead(BTN_DOWN) == LOW ||
             digitalRead(BTN_LEFT) == LOW || digitalRead(BTN_RIGHT) == LOW) {
        delay(10);
      }
      delay(200);
      tft.fillScreen(COL_BG);
      drawStaticUI();
      resetGame();
      // Reinicia el tablero con la serpiente en posición inicial.
    }
  }
  delay(currentSpeed);
  // Pausa al final de cada ciclo. Controla la velocidad de la serpiente.
  // currentSpeed empieza en 120ms y se reduce 10ms por cada 100 puntos,
  // hasta un mínimo de 40ms (velocidad máxima).
}

// ==================================================================================
// SECCIÓN 4: FUNCIONES DEL JUEGO
// ==================================================================================

/*
 * handleInput()
 * Lee el estado de los 4 botones y actualiza los vectores de dirección (dirX, dirY).
 * Implementa exclusión mutua para impedir giros de 180° que causarían
 * autocolisión inmediata (ej: no se puede ir de ARRIBA a ABAJO directamente).
 */

void handleInput() {
  if (digitalRead(BTN_UP) == LOW && dirY != 1) {
    dirX = 0; dirY = -1;
  }
  // ARRIBA: solo si no se está moviendo hacia abajo (dirY != 1)
  else if (digitalRead(BTN_DOWN) == LOW && dirY != -1) {
    dirX = 0; dirY = 1;
  }
  // ABAJO: solo si no se está moviendo hacia arriba (dirY != -1)
  else if (digitalRead(BTN_LEFT) == LOW && dirX != 1) {
    dirX = -1; dirY = 0;
  }
  // IZQUIERDA: solo si no se está moviendo hacia la derecha (dirX != 1)
  else if (digitalRead(BTN_RIGHT) == LOW && dirX != -1) {
    dirX = 1; dirY = 0;
  }
  // DERECHA: solo si no se está moviendo hacia la izquierda (dirX != -1)
}

/*
 * updateGame()
 * Núcleo del Game Loop. Por cada ciclo ejecuta:
 * 1. Desplaza los segmentos del cuerpo (cada [i] toma la posición de [i-1])
 * 2. Calcula la nueva posición de la cabeza sumando los vectores de dirección
 * 3. Verifica colisión con bordes y con el propio cuerpo
 * 4. Verifica si la cabeza coincide con la comida
 * 5. Actualiza el render en pantalla de forma optimizada (borra solo la cola)
 */
void updateGame() {
  oldTail = snake[snakeLength - 1];
  // Guarda la cola antes de moverla para poder borrarla de la pantalla después.

  // Desplaza el cuerpo: cada segmento ocupa la posición del anterior.
  // Se recorre de atrás hacia adelante para no sobreescribir datos.
  for (int i = snakeLength - 1; i > 0; i--) {
    snake[i] = snake[i - 1];
  }

  // Mueve la cabeza según el vector de dirección actual
  snake[0].x += dirX;
  snake[0].y += dirY;

  // --- Colisión con bordes del tablero ---
  if (snake[0].x < 0 || snake[0].x >= GRID_W || snake[0].y < 0 || snake[0].y >= GRID_H) {
    triggerGameOver();
    return;
    // return detiene updateGame() para no procesar un estado inválido
  }

  // --- Colisión con el propio cuerpo ---
  // Se compara la cabeza (snake[0]) contra cada segmento a partir del índice 1
  for (int i = 1; i < snakeLength; i++) {
    if (snake[0].x == snake[i].x && snake[0].y == snake[i].y) {
      triggerGameOver();
      return;
    }
  }

  // --- Lógica de recolección de comida ---
  bool ateFood = false;
  if (snake[0].x == food.x && snake[0].y == food.y) {
    ateFood = true;
    score += 10;
    // Suma 10 puntos al marcador por cada fruta consumida.

    snakeLength++;
    snake[snakeLength - 1] = oldTail;
    // El nuevo segmento ocupa la posición de la cola anterior,
    // haciendo crecer la serpiente visualmente de forma natural.

    tone(BUZZER_PIN, 1000, 50);
    // Tono agudo de éxito: 1000 Hz por 50ms.
    drawScore();
    // Actualiza el número de puntaje en la barra superior.

    // --- Incremento dinámico de dificultad ---
    int speedDecrease = (score / 100) * 10;
    // Cada 100 puntos, se calculan 10ms menos de delay.
    // Ej: 100pts → -10ms, 200pts → -20ms, 300pts → -30ms.

    currentSpeed = BASE_SPEED - speedDecrease;
    if (currentSpeed < 40) currentSpeed = 40;
    // Límite mínimo de 40ms: el juego no puede volverse incontrolable.

    spawnFood();
    // Genera la nueva fruta en una posición aleatoria libre.
  }

  // --- Renderizado optimizado ---
  if (!ateFood) {
    tft.fillRect(START_X + (oldTail.x * CELL_SIZE),
                 START_Y + (oldTail.y * CELL_SIZE),
                 CELL_SIZE, CELL_SIZE, COL_BG);
    // Si no comió, borra solo la celda de la cola pintándola con el fondo.
    // Evita redibujar toda la pantalla y elimina el parpadeo (flicker).
  }

  // Dibuja la cabeza en su nueva posición (rojo, bordes redondeados, margen de 1px)
  tft.fillRoundRect(START_X + (snake[0].x * CELL_SIZE),
                    START_Y + (snake[0].y * CELL_SIZE),
                    CELL_SIZE - 1, CELL_SIZE - 1, 2, COL_HEAD);

  // El segmento inmediato posterior a la cabeza pasa a color de cuerpo (azul)
  if (snakeLength > 1) {
    tft.fillRoundRect(START_X + (snake[1].x * CELL_SIZE),
                      START_Y + (snake[1].y * CELL_SIZE),
                      CELL_SIZE - 1, CELL_SIZE - 1, 2, COL_SNAKE);
  }
}

/*
 * drawStaticUI()
 * Dibuja los elementos fijos de la interfaz que no cambian durante el juego:
 * la barra negra superior y el borde blanco del área de juego.
 * Se llama al inicio y cada vez que se reinicia la partida.
 */
void drawStaticUI() {
  tft.fillRect(0, 0, SCREEN_W, TOP_BAR, COL_BAR);
  // Pinta la barra superior completamente de negro.

  tft.drawRect(0, TOP_BAR, SCREEN_W, SCREEN_H - TOP_BAR, COL_BORDER);
  // Dibuja solo el contorno blanco del área jugable (no el relleno).

  drawScore();
  // Muestra el puntaje inicial en la barra.
}

/*
 * drawScore()
 * Actualiza el texto del puntaje y el nombre del jugador en la barra superior.
 * Borra el área de texto antes de escribir para evitar superposición de caracteres.
 */
void drawScore() {
  tft.fillRect(BORDER_W, BORDER_W, SCREEN_W - (2 * BORDER_W), TOP_BAR - BORDER_W, COL_BAR);
  // Limpia toda el área de la barra superior antes de reescribir.

  tft.setTextColor(COL_TEXT, COL_BAR);
  tft.setTextSize(1);
  tft.setCursor(10, 10);
  tft.print(playerName);
  // Muestra el nombre del jugador en tamaño pequeño a la izquierda.

  tft.setTextSize(2);
  tft.setCursor(100, 8);
  tft.print("SCORE: ");
  tft.print(score);
  // Muestra el puntaje actual en tamaño mediano a la derecha del nombre.
}

/*
 * triggerGameOver()
 * Activa la secuencia de fin de partida:
 * 1. Emite el tono grave de error en el buzzer
 * 2. Envía el puntaje a Supabase via HTTP POST (persistencia de datos)
 * 3. Muestra la pantalla de Game Over con nombre y puntaje
 * 4. Activa el flag gameOver para que loop() entre en modo espera
 * El flag !gameOver asegura que esta secuencia se ejecute una sola vez.
 */
void triggerGameOver() {
  if (!gameOver) {
    tone(BUZZER_PIN, 150, 600);
    // Tono grave de fallo: 150 Hz por 600ms (sonido de derrota).

    enviarPuntaje(score);
    // Persiste el puntaje final en la base de datos de Supabase.
    // Esta llamada cumple el punto 5 de la consigna:
    // "recuperar y cargar datos del entorno en una base de datos".

    drawGameOver();
    // Muestra la pantalla final con el nombre del jugador y su puntaje.
  }
  gameOver = true;
  // Activa el flag que bloquea el Game Loop y activa la rama de espera.
}

/*
 * drawGameOver()
 * Renderiza la pantalla estática de fin de partida con:
 * - El título "GAME OVER" en rojo
 * - El nombre del jugador y su puntaje final en blanco
 * - Instrucción para reiniciar
 */
void drawGameOver() {
  tft.fillScreen(ILI9341_BLACK);
  // Borra toda la pantalla con negro.

  tft.setTextColor(ILI9341_RED);
  tft.setTextSize(4);
  tft.setCursor(15, SCREEN_H / 2 - 40);
  tft.print("GAME OVER");
  // Título principal en rojo, centrado verticalmente.

  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.setCursor(10, SCREEN_H / 2 + 10);
  tft.print(playerName);
  tft.print(": ");
  tft.print(score);
  tft.print(" pts");
  // Muestra el nombre personalizado y el puntaje final de la partida.

  tft.setTextSize(1);
  tft.setCursor(48, SCREEN_H / 2 + 50);
  tft.print("Press Button to Restart");
  // Instrucción de reinicio en texto pequeño.
}

/*
 * spawnFood()
 * Genera la comida en una posición aleatoria de la grilla que no esté
 * ocupada por ningún segmento de la serpiente.
 * Usa un bucle while que sigue intentando hasta encontrar una celda libre.
 */
void spawnFood() {
  bool valid = false;
  while (!valid) {
    valid = true;
    food.x = random(0, GRID_W); // Columna aleatoria dentro del tablero
    food.y = random(0, GRID_H); // Fila aleatoria dentro del tablero

    // Verifica que la posición no esté ocupada por ningún segmento
    for (int i = 0; i < snakeLength; i++) {
      if (snake[i].x == food.x && snake[i].y == food.y) {
        valid = false;  // Posición ocupada: el while genera nuevas coordenadas
        break;
      }
    }
  }

  // Dibuja la fruta como círculo magenta centrado en la celda calculada
  tft.fillCircle(START_X + (food.x * CELL_SIZE) + (CELL_SIZE / 2),
                 START_Y + (food.y * CELL_SIZE) + (CELL_SIZE / 2),
                 (CELL_SIZE / 2) - 1, COL_FOOD);
}

/*
 * resetGame()
 * Restaura el estado del juego a sus valores iniciales.
 * Se llama al inicio del programa y cada vez que el jugador reinicia
 * después de un Game Over.
 */
void resetGame() {
  tft.fillRect(BORDER_W, TOP_BAR + BORDER_W, PLAY_AREA_W, PLAY_AREA_H, COL_BG);
  // Limpia el área de juego pintándola con el color de fondo.

  drawStaticUI();
  // Redibuja la barra superior y el borde del tablero.

  snakeLength = 3;
  // Reinicia la longitud a 3 segmentos.

  // Posiciona los 3 segmentos iniciales centrados verticalmente en el tablero
  snake[0] = { GRID_W / 2, GRID_H / 2 };
  snake[1] = { GRID_W / 2, GRID_H / 2 + 1 };
  snake[2] = { GRID_W / 2, GRID_H / 2 + 2 };
  // Esta disposición es coherente con dirY = -1 (la serpiente sube desde el inicio).

  // Dibuja la serpiente inicial: cabeza en rojo, cuerpo en azul
  for (int i = 0; i < snakeLength; i++) {
    uint16_t c = (i == 0) ? COL_HEAD : COL_SNAKE;
    // Operador ternario: si es la cabeza (i==0) usa rojo, sino azul.
    tft.fillRoundRect(START_X + (snake[i].x * CELL_SIZE),
                      START_Y + (snake[i].y * CELL_SIZE),
                      CELL_SIZE - 1, CELL_SIZE - 1, 2, c);
  }

  // Reinicia todas las variables de estado del juego
  dirX = 0; dirY = -1; score = 0; // dirY Dirección inicial: arriba
  drawScore();  // Muestra "SCORE: 0" en la barra superior
  currentSpeed = BASE_SPEED;  // Restaura la velocidad inicial (120ms)
  gameOver = false; // Libera el flag para que loop() sea activo
  spawnFood();  // Genera y dibuja la primera fruta de la partida
}

/*
 * enviarPuntaje(int puntajeFinal)
 * Persiste el puntaje de la partida en Supabase via HTTP POST.
 * Construye un JSON con el puntaje y el nombre del jugador,
 * y lo envía a la tabla "partidas" de la base de datos en la nube.
 *
 * Parámetros:
 *   puntajeFinal → el score acumulado al terminar la partida
 *
 * Respuesta esperada: HTTP 201 (Created) indica que el registro se guardó con éxito.
 */
void enviarPuntaje(int puntajeFinal) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Error: No hay conexion WiFi para enviar los datos.");
    return;
    // Si se perdió la conexión, aborta sin crashear el programa.
  }

  HTTPClient http;
  String url = String(supabase_url) + "/rest/v1/partidas";
  // Construye la URL del endpoint REST de Supabase para la tabla "partidas".

  http.begin(url);

  // Headers requeridos por la API REST de Supabase:
  http.addHeader("Content-Type", "application/json");
  // Indica que el cuerpo del request es JSON.

  http.addHeader("apikey", supabase_key);
  // Identifica el proyecto de Supabase al que pertenece el request.

  http.addHeader("Authorization", "Bearer " + String(supabase_key));
  // Autentica al cliente usando la anon key pública.

  http.addHeader("Prefer", "return=minimal");
  // Indica a Supabase que no devuelva el registro creado en la respuesta,
  // ahorrando ancho de banda en el microcontrolador.

  // Construye el cuerpo JSON con el nombre del jugador y su puntaje
  String jsonBody = "{\"puntaje\":" + String(puntajeFinal) + ", \"jugador\":\"" + String(playerName) + "\"}";

  Serial.println("Enviando datos a Supabase...");
  int httpCode = http.POST(jsonBody);
  // Envía el request POST y almacena el código de respuesta HTTP.

  if (httpCode > 0) {
    Serial.print("Respuesta del servidor (HTTP Code): ");
    Serial.println(httpCode);
    // 201 = registro creado con éxito en la base de datos
  } else {
    Serial.print("Error en la peticion POST: ");
    Serial.println(http.errorToString(httpCode).c_str());
    // Muestra el error de red en el monitor serial para diagnóstico.
  }

  http.end();
  // Libera los recursos del cliente HTTP.
}