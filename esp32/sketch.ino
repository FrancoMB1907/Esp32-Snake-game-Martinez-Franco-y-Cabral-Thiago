// ==================================================================================
// SECCIÓN 1: DECLARACIÓN DE LIBRERÍAS Y VARIABLES GLOBALES
// ==================================================================================

// Inclusión de librerías del sistema y periféricos
#include <SPI.h>           // Librería nativa para el protocolo de comunicación SPI por hardware
#include <Adafruit_GFX.h>    // Librería núcleo de gráficos para primitivas (líneas, círculos, rectángulos)
#include <Adafruit_ILI9341.h>   // Controlador específico para el hardware de la pantalla TFT ILI9341

// Definición de pines de control para la pantalla TFT (Bus SPI e hilos de estado)
#define TFT_CS 15   // Pin de selección de chip (Chip Select) para activar la pantalla
#define TFT_DC 2    // Pin de selección de Datos/Comando (Data/Command)
#define TFT_RST 4   // Pin de reinicio de hardware (Reset)

// Instanciación del objeto 'tft' pasando los pines de control asignados
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

// Asignación de pines GPIO estables para la cruceta de botones físicos
#define BTN_UP     12   // Botón de dirección hacia ARRIBA
#define BTN_DOWN   13   // Botón de dirección hacia ABAJO
#define BTN_LEFT   14   // Botón de dirección hacia la IZQUIERDA
#define BTN_RIGHT  27   // Botón de dirección hacia la DERECHA
#define BUZZER_PIN 26   // Pin de salida digital para el buzzer piezoeléctrico pasivo

// Macros de configuración geométrica del entorno y mapa de juego (en píxeles)
#define SCREEN_W 240    // Ancho total de la pantalla ILI9341 en modo vertical
#define SCREEN_H 320    // Alto total de la pantalla ILI9341 en modo vertical
#define CELL_SIZE 10    // Tamaño en píxeles de cada celda de la grilla (10x10 px)
#define TOP_BAR 30      // Alto de la barra superior destinada a la interfaz de usuario (Score)
#define BORDER_W 2      // Grosor del borde delimitador del campo de juego

// Fórmulas matemáticas integradas para determinar las dimensiones lógicas de la grilla
#define PLAY_AREA_H (SCREEN_H - TOP_BAR - (2 * BORDER_W))   // Altura neta del área jugable
#define PLAY_AREA_W (SCREEN_W - (2 * BORDER_W))   // Ancho neto del área jugable
#define GRID_W (PLAY_AREA_W / CELL_SIZE)    // Cantidad de columnas lógicas disponibles (23 celdas)
#define GRID_H (PLAY_AREA_H / CELL_SIZE)    // Cantidad de filas lógicas disponibles (28 celdas)
#define START_X BORDER_W                    // Coordenada física X donde inicia la grilla gráfica
#define START_Y (TOP_BAR + BORDER_W)      // Coordenada física Y donde inicia la grilla gráfica

#define BASE_SPEED 120    // Velocidad inicial del ciclo de juego (milisegundos de delay)

// Definición de colores en formato hexadecimal RGB565 (16 bits) requeridos por la TFT
#define COL_BG 0x2589     // Color de fondo del campo de juego (Azul grisáceo)
#define COL_BAR ILI9341_BLACK    // Fondo de la barra superior de puntaje (Negro)
#define COL_SNAKE ILI9341_BLUE    // Color para los segmentos del cuerpo de la serpiente (Azul)
#define COL_HEAD ILI9341_RED    // Color diferenciado para la cabeza de la serpiente (Rojo)
#define COL_FOOD ILI9341_MAGENTA  // Color para representar la comida/fruta (Magenta)
#define COL_TEXT ILI9341_WHITE    // Color para los textos de la interfaz (Blanco)
#define COL_BORDER ILI9341_WHITE  // Color del rectángulo perimetral (Blanco)

// Estructura de datos para definir coordenadas bidimensionales discretas en la grilla
struct Point {
  int x, y;
};

// Variables globales de estado del juego
Point snake[400];   // Array de estructuras para almacenar hasta 400 segmentos del cuerpo
int snakeLength = 3;  // Longitud inicial de la serpiente al comenzar
Point food;           // Estructura que almacena la posición actual de la comida
int dirX = 0;         // Vector de dirección en el eje X (0 = estático, 1 = derecha, -1 = izquierda)
int dirY = -1;        // Vector de dirección en el eje Y (-1 = arriba). Inicia moviéndose hacia arriba
bool gameOver = false;  // Bandera booleana de control para el estado de fin de partida
int score = 0;          // Contador acumulativo del puntaje del jugador
int currentSpeed = BASE_SPEED;  // Variable controladora del delay dinámico de juego
Point oldTail;        // Almacén temporal de la última posición de la cola antes del movimiento

// ==================================================================================
// SECCIÓN 2: SETUP (CONFIGURACIONES INICIALES DE HARDWARE Y SOFTWARE)
// ==================================================================================
void setup() {
  Serial.begin(115200);   // Inicialización del canal de comunicación Serial a 115200 baudios
  delay(500);   // Pausa de estabilización para los periféricos del ESP32
  Serial.println("--- Snake Game Iniciado ---");

  // Configuración de pines de entrada con resistencia Pull-Up interna activada.
  // El pin lee HIGH en reposo y LOW cuando el botón conecta el GPIO a GND.
  // Esto elimina la necesidad de resistencias externas en el circuito.
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_LEFT, INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);  // Configuración del pin del buzzer en modo de salida digital

  tft.begin();  // Inicialización del controlador de la pantalla TFT ILI9341
  tft.setRotation(2); // Orientación de pantalla en modo Portrait vertical (rotación 180°)
  tft.fillScreen(COL_BG); // Limpieza inicial de la pantalla aplicando el color de fondo

  gameOver = false; // Inicialización forzada del estado del juego
  drawStaticUI();     // Renderiza los elementos fijos de la interfaz (barra y borde)
  resetGame();        // Establece los parámetros iniciales y arranca la primera partida
}

// ==================================================================================
// SECCIÓN 3: LOOP (LÓGICA PRINCIPAL Y CONTROL DE FLUJO SÍNCRONO)
// ==================================================================================
void loop() {
  // Estructura condicional principal basada en el estado de la bandera 'gameOver'
  if (!gameOver) {
    handleInput();  // Evalúa el estado de los botones y actualiza el vector de dirección
    updateGame(); // Ejecuta la lógica de movimiento, colisiones y renderizado del ciclo
  } else {
    // Escenario de Game Over: cualquier botón presionado (LOW) reinicia la partida
    if (digitalRead(BTN_UP) == LOW || digitalRead(BTN_DOWN) == LOW || 
        digitalRead(BTN_LEFT) == LOW || digitalRead(BTN_RIGHT) == LOW) {
      resetGame();  
      delay(300);   // Retardo Anti-Debounce: evita que la misma pulsación sea leída dos veces
    }
  }
  delay(currentSpeed);    // Control del reloj del bucle con la velocidad dinámica actual
}

// ==================================================================================
// SECCIÓN 4: FUNCIONES PERSONALIZADAS (MODULARIZACIÓN Y ENCAPSULAMIENTO)
// ==================================================================================

/*
  handleInput: Centraliza la lectura de los periféricos de entrada.
  Aplica una lógica de exclusión mutua para impedir giros inválidos de 180° que causarían 
  autocolisión inmediata (ej: no se puede ir hacia abajo si actualmente se desplaza hacia arriba).
*/
void handleInput() {
  if (digitalRead(BTN_UP) == LOW && dirY != 1) {
    dirX = 0; dirY = -1;  // Activa movimiento hacia arriba si no se va hacia abajo
  } 
  else if (digitalRead(BTN_DOWN) == LOW && dirY != -1) {
    dirX = 0; dirY = 1;   // Activa movimiento hacia abajo si no se va hacia arriba
  } 
  else if (digitalRead(BTN_LEFT) == LOW && dirX != 1) {
    dirX = -1; dirY = 0;  // Activa movimiento hacia la izquierda si no se va hacia la derecha
  } 
  else if (digitalRead(BTN_RIGHT) == LOW && dirX != -1) {
    dirX = 1; dirY = 0;   // Activa movimiento hacia la derecha si no se va hacia la izquierda
  }
}

/*
  updateGame: Ejecuta el núcleo algorítmico del juego por cada ciclo del reloj.
  Administra el desplazamiento del array, las colisiones, la recolección de comida
  y el renderizado optimizado para evitar parpadeo visual (flicker).
*/
void updateGame() {
  // Salva la coordenada del último segmento antes de mover el array
  // Es esencial para la optimización gráfica: saber qué celda borrar de la pantalla
  oldTail = snake[snakeLength - 1];

    // Algoritmo de desplazamiento: traslada las posiciones del cuerpo hacia el índice siguiente
    // Se recorre desde la cola hacia la cabeza para no sobreescribir datos
  for (int i = snakeLength - 1; i > 0; i--) {
    snake[i] = snake[i - 1];
  }

  // Modifica la cabeza sumando los vectores de dirección actuales
  snake[0].x += dirX;
  snake[0].y += dirY;

  // Condición de Frontera: verifica si la cabeza sobrepasó los límites de la grilla
  if (snake[0].x < 0 || snake[0].x >= GRID_W || snake[0].y < 0 || snake[0].y >= GRID_H) {
    triggerGameOver();
    return;   // Detiene updateGame() para no procesar un estado inválido
  }

  // Condición de Autocolisión: evalúa si la cabeza coincide con algún segmento del cuerpo
  for (int i = 1; i < snakeLength; i++) {
    if (snake[0].x == snake[i].x && snake[0].y == snake[i].y) {
      triggerGameOver();
      return;
    }
  }

  // Logica de recoleccion de comida
  bool ateFood = false;
  if (snake[0].x == food.x && snake[0].y == food.y) {
    ateFood = true;
    score += 10;    // Incremento del puntaje en 10 unidades
    snakeLength++;  // Extensión del array del cuerpo en un segmento
    snake[snakeLength - 1] = oldTail;   // El nuevo segmento ocupa la posición libre de la cola anterior

    tone(BUZZER_PIN, 1000, 50); // Tono agudo de éxito: 1000 Hz por 50 ms
    drawScore();    // Actualización inmediata del marcador en pantalla

    // Algoritmo de dificultad progresiva: reduce el delay según el puntaje acumulado
    int speedDecrease = (score / 100) * 10;
    currentSpeed = BASE_SPEED - speedDecrease;
    if (currentSpeed < 40) currentSpeed = 40;   // Clamping: límite mínimo de 40 ms para mantener el control

    spawnFood();  // Reposiciona la comida en una nueva celda aleatoria libre
  }

  // Optimización de renderizado: si no comió, borra solo la celda de la cola anterior.
  // Evita redibujar toda la pantalla por ciclo, eliminando el flicker visual.
  if (!ateFood) {
    tft.fillRect(START_X + (oldTail.x * CELL_SIZE),
                 START_Y + (oldTail.y * CELL_SIZE),
                 CELL_SIZE, CELL_SIZE, COL_BG);
  }

  // Dibuja la cabeza en su nueva posición con bordes redondeados en color rojo
  tft.fillRoundRect(START_X + (snake[0].x * CELL_SIZE),
                    START_Y + (snake[0].y * CELL_SIZE),
                    CELL_SIZE - 1, CELL_SIZE - 1, 2, COL_HEAD);

  // Convierte el segmento inmediato posterior (cuello) al color del cuerpo
  if (snakeLength > 1) {
    tft.fillRoundRect(START_X + (snake[1].x * CELL_SIZE),
                      START_Y + (snake[1].y * CELL_SIZE),
                      CELL_SIZE - 1, CELL_SIZE - 1, 2, COL_SNAKE);
  }
}

/*
  drawStaticUI: Renderiza los componentes gráficos del entorno que permanecen
  fijos durante toda la ejecución del juego.
*/
void drawStaticUI() {
  tft.fillRect(0, 0, SCREEN_W, TOP_BAR, COL_BAR);   // Barra superior negra para el Score
  tft.drawRect(0, TOP_BAR, SCREEN_W, SCREEN_H - TOP_BAR, COL_BORDER);   // Borde perimetral blanco del campo
  drawScore();
}

/*
  drawScore: Administra el borrado parcial y reescritura del texto del puntaje.
  Se llama cada vez que el score cambia para actualizar la interfaz sin redibujar todo.
*/
void drawScore() {
  tft.fillRect(BORDER_W, BORDER_W, SCREEN_W - (2 * BORDER_W), TOP_BAR - BORDER_W, COL_BAR);
  tft.setTextColor(COL_TEXT, COL_BAR);  // Texto blanco sobre fondo negro
  tft.setTextSize(2);   // Escala de fuente legible (tamaño 2)
  tft.setCursor(65, 8);   // Posición en píxeles para centrar visualmente el texto
  tft.print("SCORE: ");
  tft.print(score);   // Imprime el valor numérico actual del puntaje
}

/*
  triggerGameOver: Encapsula la transición de estados cuando el jugador pierde.
  El flag 'gameOver' asegura que la secuencia se ejecute una sola vez.
*/
void triggerGameOver() {
  if (!gameOver) {
    tone(BUZZER_PIN, 150, 600); // Tono grave de fallo: 150 Hz por 600 ms
    drawGameOver();   // Renderiza la pantalla final de Game Over
  }
  gameOver = true;    // Activa la bandera que bloquea el bucle de juego
}

/*
  drawGameOver: Renderiza la pantalla estática de fin de juego con el puntaje final
  y la instrucción para reiniciar.
*/
void drawGameOver() {
  tft.fillScreen(ILI9341_BLACK);
  
  tft.setTextColor(ILI9341_RED);
  tft.setTextSize(4);
  tft.setCursor(15, SCREEN_H / 2 - 40);
  tft.print("GAME OVER");   // Título principal en rojo

  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.setCursor(35, SCREEN_H / 2 + 10);
  tft.print("Final Score: ");
  tft.print(score);   // Puntaje final acumulado

  tft.setTextSize(1);
  tft.setCursor(48, SCREEN_H / 2 + 50);
  tft.print("Press Button to Restart");   // Instrucción de reinicio
}

/*
  spawnFood: Genera la comida en una posición pseudoaleatoria de la grilla.
  Garantiza mediante validación iterativa que la coordenada generada
  no coincida con ningún segmento del cuerpo de la serpiente.
*/
void spawnFood() {
  bool valid = false;
  while (!valid) {
    valid = true;
    food.x = random(0, GRID_W);   // Coordenada aleatoria dentro del rango de columnas
    food.y = random(0, GRID_H);   // Coordenada aleatoria dentro del rango de filas

    // Compara la posición generada contra todos los segmentos ocupados
    for (int i = 0; i < snakeLength; i++) {
      if (snake[i].x == food.x && snake[i].y == food.y) {
        valid = false;    // Posición ocupada: descarta y genera nuevas coordenadas
        break;    // Interrumpe el for para iniciar un nuevo ciclo de validación
      }
    }
  }
  // Dibuja la comida como círculo magenta centrado en la celda calculada
  tft.fillCircle(START_X + (food.x * CELL_SIZE) + (CELL_SIZE / 2),
                 START_Y + (food.y * CELL_SIZE) + (CELL_SIZE / 2),
                 (CELL_SIZE / 2) - 1, COL_FOOD);
}

/*
  resetGame: Restaura completamente el estado del juego a sus valores iniciales.
  Se llama al inicio del programa y cada vez que el jugador reinicia tras un Game Over.
*/
void resetGame() {
  tft.fillRect(BORDER_W, TOP_BAR + BORDER_W, PLAY_AREA_W, PLAY_AREA_H, COL_BG);
  drawStaticUI(); 
  snakeLength = 3;  
  // Posicionamiento inicial de los 3 segmentos centrados verticalmente en el tablero
  snake[0] = { GRID_W / 2, GRID_H / 2 };  // Cabeza
  snake[1] = { GRID_W / 2, GRID_H / 2 + 1 };  // Cuerpo
  snake[2] = { GRID_W / 2, GRID_H / 2 + 2 };  // Cola

  // Renderiza los segmentos iniciales: cabeza en rojo, cuerpo en azul
  for (int i = 0; i < snakeLength; i++) {
    uint16_t c = (i == 0) ? COL_HEAD : COL_SNAKE; // Operador ternario: cabeza=rojo, resto=azul
    tft.fillRoundRect(START_X + (snake[i].x * CELL_SIZE),
                      START_Y + (snake[i].y * CELL_SIZE),
                      CELL_SIZE - 1, CELL_SIZE - 1, 2, c);
  }

  dirX = 0; dirY = -1; score = 0; // Vector inicial: sin movimiento horizontal, sube hacia arriba
  drawScore();  // Muestra "SCORE: 0" en la barra superior
  currentSpeed = BASE_SPEED;  // Restaura la velocidad inicial (120 ms)
  gameOver = false; // Libera la bandera para que el loop() entre en rama activa
  spawnFood();  // Genera y dibuja la primera fruta de la partida
}
