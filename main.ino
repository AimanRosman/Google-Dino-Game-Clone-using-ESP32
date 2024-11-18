#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// OLED 128x64 (Game OLED)
#define SCREEN_WIDTH_64 128
#define SCREEN_HEIGHT_64 64
#define OLED_RESET_64    -1
#define OLED_ADDRESS_64  0x3C  
Adafruit_SSD1306 displayGame(SCREEN_WIDTH_64, SCREEN_HEIGHT_64, &Wire, OLED_RESET_64);

// Push button pin
#define BUTTON_PIN 4
#define BUZZER_PIN 5  // Buzzer pin

// Game parameters
int playerX = 10;               // Player's fixed X position
int playerY;                    // Player's initial Y position (set to ground level in setup)
int obstacleX = SCREEN_WIDTH_64;   // Start obstacle off-screen
int obstacleWidth = 8;          // Obstacle width
int obstacleHeight = 10;        // Obstacle height
bool isJumping = false;         // Jump state
bool gameOver = false;          // Game over state
int jumpHeight = 15;            // Height of the jump
int jumpCounter = 0;            // Counter for jump duration
int maxJumpFrames = 15;         // Duration of jump (in frames), shorter for faster jump
int hoverFrames = 10;           // Frames to "hover" at the peak
int groundLevel = SCREEN_HEIGHT_64 - 10; // Ground level

int score = 0;                  // Score counter (time in frames)
int jumpBonus = 0;              // Bonus points when jumping over obstacles
unsigned long lastTime = 0;    // Timer to track elapsed time (milliseconds)
unsigned long currentTime = 0; // Current time to calculate elapsed time

// Dinosaur Character (using pixels)
const int dinoWidth = 12;
const int dinoHeight = 12;

void setup() {
  Serial.begin(115200);
  
  // Initialize the game OLED display (128x64)
  if (!displayGame.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS_64)) {
    Serial.println(F("SSD1306 allocation failed for game OLED"));
    for (;;);
  }
  displayGame.clearDisplay(); 

  // Set button pin as input with internal pull-up resistor
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Set buzzer pin as output
  pinMode(BUZZER_PIN, OUTPUT);
  
  // Initialize player at ground level
  playerY = groundLevel;
}

void loop() {
  // Clear the game display at the start of each loop
  displayGame.clearDisplay();

  // Check for button press to trigger jump if not already jumping or game over
  if (digitalRead(BUTTON_PIN) == LOW && !isJumping && !gameOver) {
    isJumping = true;
    jumpCounter = 0;  // Reset jump counter
  }

  // Game logic
  if (gameOver) {
    // Display Game Over message on the game OLED, centered with smaller font
    displayGame.setTextSize(1);  // Font size for "Game Over"
    displayGame.setTextColor(SSD1306_WHITE);
    
    String gameOverMessage = "TRY MEEEE!";
    int16_t x, y;
    uint16_t width, height;
    displayGame.getTextBounds(gameOverMessage, 0, 0, &x, &y, &width, &height);
    displayGame.setCursor((SCREEN_WIDTH_64 - width) / 2, SCREEN_HEIGHT_64 / 3);
    displayGame.print(gameOverMessage);
    
    // Restart Text - Centered
    displayGame.setTextSize(0);  // Smaller font size for "Try Again?"
    String tryAgainMessage = "TAK STRONG DO KAUU!!";
    displayGame.getTextBounds(tryAgainMessage, 0, 0, &x, &y, &width, &height);
    displayGame.setCursor((SCREEN_WIDTH_64 - width) / 2, SCREEN_HEIGHT_64 / 2 + 10);
    displayGame.print(tryAgainMessage);

    // Update display
    displayGame.display();
    
    // Play melody when game over (continues playing as long as button isn't pressed)
    playGameOverMelody();

    // Wait until the button is pressed to restart the game
    while (digitalRead(BUTTON_PIN) == HIGH) {
      // Wait here until the button is pressed to restart
    }
    resetGame();
  } else {
    // Move the obstacle from right to left (faster)
    obstacleX -= 4;  // Increase speed to 4 (or try 5)

    // If obstacle moves off-screen, reset to start position
    if (obstacleX < 0) {
      obstacleX = SCREEN_WIDTH_64;
      // Increase score based on time (how long the player has been running)
      score++;  
    }

    // Handle jumping logic (faster jump)
    if (isJumping) {
      if (jumpCounter < maxJumpFrames / 2) {
        // Ascend phase (faster)
        playerY = groundLevel - jumpHeight;
      } else if (jumpCounter < (maxJumpFrames / 2) + hoverFrames) {
        // Hover phase remains the same
        playerY = groundLevel - jumpHeight;
      } else if (jumpCounter < maxJumpFrames + hoverFrames) {
        // Descend phase (faster)
        playerY = groundLevel - jumpHeight + (jumpCounter - (maxJumpFrames / 2) - hoverFrames) * 3;  // Faster descent
      }

      jumpCounter++;

      // End jump when jumpCounter exceeds total frames
      if (jumpCounter >= maxJumpFrames + hoverFrames) {
        playerY = groundLevel;  // Return player to ground level
        isJumping = false;      // End jump
      }
    } else {
      playerY = groundLevel;  // Ensure player is at ground level when not jumping
    }

    // Draw the dinosaur character
    drawDino(playerX, playerY);

    // Draw the obstacle (Cactus)
    displayGame.fillRect(obstacleX, groundLevel - obstacleHeight, obstacleWidth, obstacleHeight, SSD1306_WHITE);

    // Check for collision (player hits the obstacle at ground level)
    if (playerX + dinoWidth > obstacleX && playerX < obstacleX + obstacleWidth && playerY == groundLevel) {
      jumpBonus = 0;  // Reset bonus if hit an obstacle
      gameOver = true;
    }

    // Add 10 points to score when jumping over an obstacle
    if (playerX + dinoWidth > obstacleX && playerX < obstacleX + obstacleWidth && playerY == groundLevel - jumpHeight) {
      jumpBonus = 1;  // Add bonus points
      score += jumpBonus;  // Increase score by bonus

      // Play buzzer sound when jumping over an obstacle (1 beep)
      playJumpBeep();
    }

    // Draw the ground
    displayGame.fillRect(0, groundLevel, SCREEN_WIDTH_64, 4, SSD1306_WHITE);

    // Display the score (time in frames) on the game OLED
    displayGame.setCursor(0, 0);
    displayGame.setTextSize(1); // Smaller font size for score
    displayGame.setTextColor(SSD1306_WHITE);
    displayGame.print(F("Score: "));
    displayGame.setTextSize(2); // Larger font size for score value
    displayGame.print(score);

    // Update the display (game OLED)
    displayGame.display();
    
    // Calculate elapsed time (timer)
    currentTime = millis();
    if (currentTime - lastTime >= 1000) {  // 1 second passed
      score++;  // Increment score every second
      lastTime = currentTime;  // Update the last time check
    }
  }

  delay(50); // Control game speed (adjust to make the game faster or slower)
}

// Function to reset the game state
// Function to reset the game state
// Function to reset the game state
void resetGame() {
  playerY = groundLevel;   // Reset player to ground level
  obstacleX = SCREEN_WIDTH_64;
  gameOver = false;
  isJumping = false;       // Ensure jumping state is reset
  jumpCounter = 0;         // Reset jump counter
  score = 0;               // Reset score
  jumpBonus = 0;           // Reset jump bonus
  
  // Disable button input for 0.3 seconds
  delay(300);
}



// Function to draw the dinosaur character (simplified version)
void drawDino(int x, int y) {
  // Draw dinosaur head (circle)
  displayGame.fillCircle(x + 4, y - 4, 4, SSD1306_WHITE);
  // Draw dinosaur body (rectangle)
  displayGame.fillRect(x, y - 8, 12, 8, SSD1306_WHITE);
  // Draw legs (simple rectangles)
  displayGame.fillRect(x + 2, y, 3, 4, SSD1306_WHITE);  // Left leg
  displayGame.fillRect(x + 7, y, 3, 4, SSD1306_WHITE);  // Right leg
}

// Function to play buzzer sound (1 beep when jumping over an obstacle)
void playJumpBeep() {
  tone(BUZZER_PIN, 1000, 10);  // 1000 Hz for 10ms
}

// Function to play a melody when the game is over
// Updated Function to play a melody when the game is over
void playGameOverMelody() {
  // New melody for game over
  int melody[] = {262, 294, 330, 349, 392, 440, 494, 523}; // Example melody notes
  int duration[] = {200, 200, 200, 200, 200, 200, 200, 400}; // Example durations for each noteaqq
  int numNotes = sizeof(melody) / sizeof(melody[0]);

  while (gameOver && digitalRead(BUTTON_PIN) == HIGH) {
    for (int i = 0; i < numNotes; i++) {
      if (digitalRead(BUTTON_PIN) == LOW) {
        return;  // Exit melody if button is pressed
      }
      tone(BUZZER_PIN, melody[i], duration[i]);
      delay(duration[i]);
      noTone(BUZZER_PIN);
      delay(100);  // Short pause between notes
    }
  }
}

