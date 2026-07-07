/*
 * ====================================================
 * DINO T-REX - Juego para Arduino Nano + OLED 128x64
 * ====================================================
 *
 * CONEXIONES DEL CIRCUITO:
 * ┌─────────────────────────────────────────────┐
 * │ OLED SSD1306 (I2C)                          │
 * │   GND  → GND Arduino                        │
 * │   VCC  → 5V  Arduino                        │
 * │   SCL  → A5  Arduino Nano                   │
 * │   SDA  → A4  Arduino Nano                   │
 * │                                             │
 * │ Botón SALTAR (izquierdo)                    │
 * │   Pin 1 → D2  Arduino Nano                  │
 * │   Pin 2 → GND                               │
 * │                                             │
 * │ Botón AGACHARSE (derecho)                   │
 * │   Pin 1 → D3  Arduino Nano                  │
 * │   Pin 2 → GND                               │
 * └─────────────────────────────────────────────┘
 *
 * LIBRERÍAS NECESARIAS (instalar desde Library Manager):
 *   - Adafruit SSD1306
 *   - Adafruit GFX Library
 *
 * CONTROLES:
 *   - Botón IZQUIERDO (D2): SALTAR
 *   - Botón DERECHO  (D3): AGACHARSE (modo agachado para esquivar pterodáctilos)
 *   - Ambos botones al inicio: INICIAR juego
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ─── Configuración de pantalla ────────────────────────────
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ─── Pines de botones ─────────────────────────────────────
#define BTN_JUMP  2   // Botón izquierdo → SALTAR
#define BTN_DUCK  3   // Botón derecho  → AGACHARSE

// ─── Física del juego ─────────────────────────────────────
#define GROUND_Y      52   // Línea del suelo (y en píxeles)
#define GRAVITY       2    // Aceleración de caída
#define JUMP_FORCE   -10   // Velocidad inicial de salto (negativa = arriba)
#define GAME_SPEED_INIT 3  // Velocidad inicial del juego
#define SPEED_UP_INTERVAL 200 // Cada cuántos frames se acelera

// ─── Tamaños del Dino ─────────────────────────────────────
#define DINO_X        16
#define DINO_W        12
#define DINO_H_STAND  20
#define DINO_H_DUCK   12

// ─── Estado del juego ─────────────────────────────────────
enum GameState { WAITING, PLAYING, DEAD };

// ─── Variables del jugador ────────────────────────────────
float dinoY;
float dinoVY;
bool  isJumping;
bool  isDucking;
int   dinoFrame;  // Animación de patas (0 o 1)

// ─── Obstáculos ──────────────────────────────────────────
#define MAX_OBSTACLES 3
struct Obstacle {
  float x;
  int   w, h;
  bool  active;
  bool  flying;    // true = pterodáctilo (volador)
  int   flyY;      // Altura de vuelo si es pterodáctilo
  int   frame;     // Frame de animación pterodáctilo
};
Obstacle obstacles[MAX_OBSTACLES];

// ─── Nubes ────────────────────────────────────────────────
#define MAX_CLOUDS 3
struct Cloud {
  float x;
  int   y;
};
Cloud clouds[MAX_CLOUDS];

// ─── Suelo decorativo ────────────────────────────────────
#define MAX_GROUND_DOTS 8
struct GroundDot {
  float x;
  int   y;
};
GroundDot groundDots[MAX_GROUND_DOTS];

// ─── Puntuación y estado ─────────────────────────────────
GameState gameState;
unsigned long score;
unsigned long highScore;
int   gameSpeed;
int   frameCount;
int   obstacleTimer;
bool  btnJumpPrev, btnDuckPrev;

// ─── Función: Resetear juego ──────────────────────────────
void resetGame() {
  dinoY       = GROUND_Y - DINO_H_STAND;
  dinoVY      = 0;
  isJumping   = false;
  isDucking   = false;
  dinoFrame   = 0;

  gameSpeed   = GAME_SPEED_INIT;
  score       = 0;
  frameCount  = 0;
  obstacleTimer = 60;

  for (int i = 0; i < MAX_OBSTACLES; i++) {
    obstacles[i].active = false;
  }
  for (int i = 0; i < MAX_CLOUDS; i++) {
    clouds[i].x = random(20, 128);
    clouds[i].y = random(6, 22);
  }
  for (int i = 0; i < MAX_GROUND_DOTS; i++) {
    groundDots[i].x = random(0, 128);
    groundDots[i].y = GROUND_Y + random(1, 5);
  }
}

// ─── Función: Generar obstáculo ───────────────────────────
void spawnObstacle() {
  for (int i = 0; i < MAX_OBSTACLES; i++) {
    if (!obstacles[i].active) {
      obstacles[i].active = true;
      obstacles[i].x = 130;
      obstacles[i].frame = 0;

      // Después de cierta puntuación, aparecen pterodáctilos
      if (score > 80 && random(0, 2) == 0) {
        obstacles[i].flying = true;
        obstacles[i].w = 16;
        obstacles[i].h = 10;
        // Altura: bajo (fácil de agacharse), medio o alto (saltar)
        int rnd = random(0, 3);
        if (rnd == 0)      obstacles[i].flyY = GROUND_Y - 10;   // Bajo
        else if (rnd == 1) obstacles[i].flyY = GROUND_Y - 22;  // Medio
        else               obstacles[i].flyY = GROUND_Y - 30;  // Alto
      } else {
        obstacles[i].flying = false;
        // Cactus de diferentes tamaños
        int tipo = random(0, 3);
        if (tipo == 0) { obstacles[i].w = 8;  obstacles[i].h = 14; }  // Pequeño
        else if (tipo == 1) { obstacles[i].w = 10; obstacles[i].h = 16; }  // Mediano
        else               { obstacles[i].w = 16; obstacles[i].h = 22; }  // Grande
        obstacles[i].flyY = 0;
      }
      return;
    }
  }
}

// ─── Función: Dibujar Dino ────────────────────────────────
void drawDino(int x, int y, bool duck, int frame) {
  if (!duck) {
    // Cuerpo principal
    display.fillRect(x + 2, y, 8, 12, WHITE);
    // Cabeza
    display.fillRect(x + 4, y - 6, 8, 7, WHITE);
    // Ojo
    display.drawPixel(x + 10, y - 4, BLACK);
    // Boca
    display.drawPixel(x + 11, y - 2, WHITE);
    // Cola
    display.fillRect(x, y + 2, 3, 5, WHITE);
    // Brazito
    display.fillRect(x + 8, y + 3, 4, 2, WHITE);

    // Patas (animadas)
    if (frame == 0) {
      display.fillRect(x + 3, y + 12, 3, 6, WHITE);
      display.fillRect(x + 7, y + 12, 3, 4, WHITE);
    } else {
      display.fillRect(x + 3, y + 12, 3, 4, WHITE);
      display.fillRect(x + 7, y + 12, 3, 6, WHITE);
    }
  } else {
    // Dino agachado (más ancho, más bajo)
    display.fillRect(x, y + 4,  14, 8, WHITE);  // Cuerpo bajo
    display.fillRect(x + 6, y, 8, 6, WHITE);    // Cabeza baja
    display.drawPixel(x + 12, y + 2, BLACK);    // Ojo
    display.fillRect(x - 2, y + 5, 3, 4, WHITE); // Cola
    // Patas agachado
    if (frame == 0) {
      display.fillRect(x + 2, y + 12, 3, 4, WHITE);
      display.fillRect(x + 8, y + 12, 3, 2, WHITE);
    } else {
      display.fillRect(x + 2, y + 12, 3, 2, WHITE);
      display.fillRect(x + 8, y + 12, 3, 4, WHITE);
    }
  }
}

// ─── Función: Dibujar Cactus ──────────────────────────────
void drawCactus(int x, int y, int w, int h) {
  // Tronco principal
  int tW = max(3, w / 2);
  int tX = x + (w - tW) / 2;
  display.fillRect(tX, y, tW, h, WHITE);

  if (w >= 10) {
    // Brazo izquierdo
    display.fillRect(x, y + h / 4, tX - x, 3, WHITE);
    display.fillRect(x, y + h / 6, 3, h / 4, WHITE);
    // Brazo derecho
    int rX = tX + tW;
    display.fillRect(rX, y + h / 4, x + w - rX, 3, WHITE);
    display.fillRect(x + w - 3, y + h / 6, 3, h / 4, WHITE);
  }
}

// ─── Función: Dibujar Pterodáctilo ────────────────────────
void drawPterodactyl(int x, int y, int frame) {
  // Cuerpo
  display.fillRect(x + 4, y + 3, 8, 6, WHITE);
  // Cabeza y pico
  display.fillRect(x + 10, y + 2, 5, 4, WHITE);
  display.fillRect(x + 14, y + 1, 3, 2, WHITE);
  display.drawPixel(x + 13, y + 3, BLACK); // ojo

  // Alas (2 posiciones de animación)
  if (frame == 0) {
    // Alas arriba
    display.fillRect(x, y, 5, 3, WHITE);
    display.fillRect(x + 3, y, 10, 2, WHITE);
  } else {
    // Alas abajo
    display.fillRect(x, y + 4, 5, 3, WHITE);
    display.fillRect(x + 3, y + 5, 10, 2, WHITE);
  }
}

// ─── Función: Dibujar Nube ────────────────────────────────
void drawCloud(int x, int y) {
  display.fillRect(x, y + 2, 14, 4, WHITE);
  display.fillRect(x + 3, y, 8, 7, WHITE);
  display.fillRect(x + 2, y + 1, 10, 6, WHITE);
}

// ─── Función: Verificar Colisiones ───────────────────────
bool checkCollision() {
  int dinoH = isDucking ? DINO_H_DUCK : DINO_H_STAND;
  int dinoTop    = (int)dinoY;
  int dinoBottom = (int)dinoY + dinoH;
  int dinoLeft   = DINO_X;
  int dinoRight  = DINO_X + DINO_W;

  // Margen de tolerancia (hitbox más pequeño que el sprite)
  int margin = 3;

  for (int i = 0; i < MAX_OBSTACLES; i++) {
    if (!obstacles[i].active) continue;

    int obsX = (int)obstacles[i].x;
    int obsY = obstacles[i].flying
               ? obstacles[i].flyY
               : GROUND_Y - obstacles[i].h;
    int obsW = obstacles[i].w;
    int obsH = obstacles[i].h;

    bool overlapX = (dinoRight  - margin) > obsX         &&
                    (dinoLeft   + margin) < obsX + obsW;
    bool overlapY = (dinoBottom - margin) > obsY         &&
                    (dinoTop    + margin) < obsY + obsH;

    if (overlapX && overlapY) return true;
  }
  return false;
}

// ─── SETUP ───────────────────────────────────────────────
void setup() {
  pinMode(BTN_JUMP, INPUT_PULLUP);
  pinMode(BTN_DUCK, INPUT_PULLUP);

  randomSeed(analogRead(0));

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    while (true); // Fallo: pantalla no encontrada
  }

  display.clearDisplay();
  display.setTextColor(WHITE);

  highScore  = 0;
  gameState  = WAITING;
  resetGame();
}

// ─── Pantalla de espera ───────────────────────────────────
void drawWaitScreen() {
  display.clearDisplay();

  // Título
  display.setTextSize(2);
  display.setCursor(10, 4);
  display.print("T-REX");

  // Dino decorativo
  drawDino(90, GROUND_Y - DINO_H_STAND, false, (millis() / 200) % 2);

  // Instrucciones
  display.setTextSize(1);
  display.setCursor(8, 34);
  display.print("SALTAR: btn izq");
  display.setCursor(8, 44);
  display.print("AGACHAR: btn der");

  // Record
  if (highScore > 0) {
    display.setCursor(8, 56);
    display.print("RECORD: ");
    display.print(highScore);
  } else {
    display.setCursor(16, 56);
    display.print("Presiona SALTAR");
  }

  // Línea de suelo
  display.drawLine(0, GROUND_Y + 1, 127, GROUND_Y + 1, WHITE);

  display.display();
}

// ─── Pantalla de Game Over ────────────────────────────────
void drawGameOver() {
  display.clearDisplay();

  // Marco
  display.drawRect(20, 4, 88, 36, WHITE);

  // Texto game over
  display.setTextSize(1);
  display.setCursor(28, 10);
  display.print("GAME  OVER");

  display.setCursor(28, 22);
  display.print("Score: ");
  display.print(score);

  display.setCursor(28, 32);
  display.print("Best:  ");
  display.print(highScore);

  // Instrucción reinicio
  display.setCursor(14, 48);
  display.print("SALTAR para reiniciar");

  // Dino muerto (ojos X)
  int dx = 56;
  int dy = GROUND_Y - DINO_H_STAND;
  drawDino(dx, dy, false, 0);
  // Ojos X sobre el dino
  display.drawPixel(dx + 10, dy - 4, BLACK);
  display.drawLine(dx + 9, dy - 5, dx + 11, dy - 3, WHITE);
  display.drawLine(dx + 9, dy - 3, dx + 11, dy - 5, WHITE);

  display.drawLine(0, GROUND_Y + 1, 127, GROUND_Y + 1, WHITE);

  display.display();
}

// ─── LOOP ────────────────────────────────────────────────
void loop() {
  bool btnJump = !digitalRead(BTN_JUMP); // LOW = presionado (PULLUP)
  bool btnDuck = !digitalRead(BTN_DUCK);
  bool jumpPressed = btnJump && !btnJumpPrev; // Flanco de subida
  bool duckPressed = btnDuck;

  // ── ESTADO: ESPERANDO ────────────────────────────────
  if (gameState == WAITING) {
    drawWaitScreen();
    if (jumpPressed) {
      resetGame();
      gameState = PLAYING;
    }
    btnJumpPrev = btnJump;
    btnDuckPrev = btnDuck;
    delay(16);
    return;
  }

  // ── ESTADO: MUERTO ───────────────────────────────────
  if (gameState == DEAD) {
    drawGameOver();
    if (jumpPressed) {
      resetGame();
      gameState = PLAYING;
    }
    btnJumpPrev = btnJump;
    btnDuckPrev = btnDuck;
    delay(16);
    return;
  }

  // ── ESTADO: JUGANDO ──────────────────────────────────

  frameCount++;
  score = frameCount / 6; // Puntuación basada en frames

  // Actualizar record
  if (score > highScore) highScore = score;

  // Incrementar velocidad progresivamente
  if (frameCount % SPEED_UP_INTERVAL == 0 && gameSpeed < 8) {
    gameSpeed++;
  }

  // ── Lógica del jugador ───────────────────────────────

  // Agacharse
  isDucking = duckPressed && !isJumping;

  // Saltar
  if (jumpPressed && !isJumping) {
    isJumping = true;
    dinoVY    = JUMP_FORCE;
  }

  // Física de salto
  if (isJumping) {
    dinoVY += GRAVITY;
    dinoY  += dinoVY;

    int groundLevel = GROUND_Y - DINO_H_STAND;
    if (dinoY >= groundLevel) {
      dinoY     = groundLevel;
      dinoVY    = 0;
      isJumping = false;
    }
  } else if (!isDucking) {
    dinoY = GROUND_Y - DINO_H_STAND;
  }

  // Animación de patas (cada 8 frames)
  if (frameCount % 8 == 0) {
    dinoFrame = 1 - dinoFrame;
  }

  // ── Generar obstáculos ───────────────────────────────
  obstacleTimer--;
  if (obstacleTimer <= 0) {
    spawnObstacle();
    // Tiempo entre obstáculos (se reduce con velocidad)
    obstacleTimer = random(50, 100) - gameSpeed * 4;
    if (obstacleTimer < 25) obstacleTimer = 25;
  }

  // ── Mover obstáculos ─────────────────────────────────
  for (int i = 0; i < MAX_OBSTACLES; i++) {
    if (!obstacles[i].active) continue;
    obstacles[i].x -= gameSpeed;
    obstacles[i].frame = (frameCount / 10) % 2;
    if (obstacles[i].x + obstacles[i].w < 0) {
      obstacles[i].active = false;
    }
  }

  // ── Mover nubes ──────────────────────────────────────
  for (int i = 0; i < MAX_CLOUDS; i++) {
    clouds[i].x -= 1; // Más lentas que el suelo
    if (clouds[i].x + 14 < 0) {
      clouds[i].x = 130 + random(0, 40);
      clouds[i].y = random(6, 22);
    }
  }

  // ── Mover puntitos del suelo ─────────────────────────
  for (int i = 0; i < MAX_GROUND_DOTS; i++) {
    groundDots[i].x -= gameSpeed;
    if (groundDots[i].x < 0) {
      groundDots[i].x = 128 + random(0, 20);
      groundDots[i].y = GROUND_Y + random(1, 5);
    }
  }

  // ── Verificar colisión ───────────────────────────────
  if (checkCollision()) {
    gameState = DEAD;
    // Parpadear pantalla antes de mostrar game over
    for (int f = 0; f < 6; f++) {
      display.invertDisplay(f % 2 == 0);
      delay(80);
    }
    display.invertDisplay(false);
    btnJumpPrev = btnJump;
    btnDuckPrev = btnDuck;
    return;
  }

  // ── DIBUJAR ──────────────────────────────────────────
  display.clearDisplay();

  // Nubes
  for (int i = 0; i < MAX_CLOUDS; i++) {
    drawCloud((int)clouds[i].x, clouds[i].y);
  }

  // Línea del suelo y puntitos
  display.drawLine(0, GROUND_Y + 1, 127, GROUND_Y + 1, WHITE);
  for (int i = 0; i < MAX_GROUND_DOTS; i++) {
    display.drawPixel((int)groundDots[i].x, groundDots[i].y, WHITE);
  }

  // Obstáculos
  for (int i = 0; i < MAX_OBSTACLES; i++) {
    if (!obstacles[i].active) continue;
    int ox = (int)obstacles[i].x;
    if (obstacles[i].flying) {
      drawPterodactyl(ox, obstacles[i].flyY, obstacles[i].frame);
    } else {
      drawCactus(ox, GROUND_Y - obstacles[i].h, obstacles[i].w, obstacles[i].h);
    }
  }

  // Dino
  int dinoDrawY = isDucking
                  ? GROUND_Y - DINO_H_DUCK
                  : (int)dinoY;
  drawDino(DINO_X, dinoDrawY, isDucking, isJumping ? 0 : dinoFrame);

  // Puntuación
  display.setTextSize(1);
  display.setCursor(70, 0);
  char scoreStr[10];
  sprintf(scoreStr, "%05lu", score);
  display.print(scoreStr);

  // HI score (solo si existe)
  if (highScore > 0 && highScore > score) {
    display.setCursor(0, 0);
    display.print("HI ");
    char hiStr[10];
    sprintf(hiStr, "%05lu", highScore);
    display.print(hiStr);
  }

  display.display();

  // Guardar estado botones
  btnJumpPrev = btnJump;
  btnDuckPrev = btnDuck;

  delay(16); // ~60 FPS
}
