/*************************************************************
*   IMPORTS
*************************************************************/

#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include "Timer1.h"

/*************************************************************
*   NETWORK CONFIGURATION
*************************************************************/

// Instantiates an Ethernet client and MQTT client
EthernetClient ethClient;
PubSubClient mqttClient(ethClient);

// Ethernet MAC address
const uint8_t myMAC_LSB = 0x03;
byte mac[] = { 0x00, 0x00, 0x00, 0x00, 0x00, myMAC_LSB };

// HOME Network parameters
// IPAddress ip(192, 168, 1, 150);
// IPAddress gw(192, 168, 1, 1);

// UIB Network parameters
const uint8_t myHostAddress = 164;
IPAddress ip(192, 168, 132, myHostAddress); // This host IP
IPAddress gw(192, 168, 132, 7); // Gateway IP

IPAddress mydns(8, 8, 8, 8);

// MQTT broker configuration
const char broker_name[] = "tom.uib.es";
const unsigned int broker_port = 1883;
const char client_id[] = "mrm1";

// MQTT topics
const char *bar1Topic = "Arkanoid/bar1";
const char *bar2Topic = "Arkanoid/bar2";
const char *ball1Topic = "Arkanoid/ball1";
const char *ball2Topic = "Arkanoid/ball2";
const char *points1Topic = "Arkanoid/points1";
const char *points2Topic = "Arkanoid/points2";
const char *blocksTopic = "Arkanoid/blocks";
const char *timeTopic = "Arkanoid/time";

/*************************************************************
*   GAME CONSTANTS & VARIABLES
*************************************************************/

// Canvas settings
const int canvasWidth = 800;
const int canvasHeight = 800;

// Block settings
const int rows = 6;
const int cols = 13;
const int blockHeight = 20;
const int blockSpacing = 0;
const int topSpacing = 200;
int blockWidth;
boolean blocks[6][13];

// Paddle settings
const int paddleWidth = 150;
const int paddleHeight = 20;
const float paddleSpeed = 0.10;

// Paddle 1
float paddle1X;
float paddle1Y = canvasHeight - 50;
float targetPaddle1X;

// Paddle 2
float paddle2X;
float paddle2Y = canvasHeight - 50;
float targetPaddle2X;

// Ball settings
const float ballDiameter = 15;

// Ball 1
float ball1X, ball1Y;
float ball1SpeedX = 5, ball1SpeedY = 5;
boolean ball1Active = true;

// Ball 2
float ball2X, ball2Y;
float ball2SpeedX = -5, ball2SpeedY = 5;
boolean ball2Active = true;

// Score tracking
int score1 = 0;
int score2 = 0;
int maxScore = rows * cols;

// Wall thickness
const int wallThickness = 20;

// Game countdown timer
unsigned long countdownStart;
const unsigned long countdownDuration = 120000;  // 60 seconds (1 minute)
bool gameRunning = true;

// Timer instance
Timer1 timer;
volatile bool publishFlag = false;

// Track game over
bool gameOverSent = false;


/*************************************************************
*   INTERRUPT SERVICE ROUTINE (ISR)
*************************************************************/
ISR(TIMER1_COMPA_vect) {
  publishFlag = true;
}

/*************************************************************
*   SETUP FUNCTION
*************************************************************/
void setup() {
  Serial.begin(115200);

  Ethernet.begin(mac, ip, mydns, gw);
  mqttClient.setServer(broker_name, broker_port);
  mqttClient.setCallback(callback);

  timer.init(PS_256, 400);
  timer.start();

  blockWidth = (canvasWidth - 2 * wallThickness - (cols - 1) * blockSpacing) / cols;

  for (int row = 0; row < rows; row++) {
    for (int col = 0; col < cols; col++) {
      blocks[row][col] = true;
    }
  }

  resetBall(ball1X, ball1Y, ball1SpeedX, ball1SpeedY, ball1Active);
  resetBall(ball2X, ball2Y, ball2SpeedX, ball2SpeedY, ball2Active);

  countdownStart = millis();
}

/*************************************************************
*   MAIN LOOP FUNCTION
*************************************************************/
void loop() {
  connect();

  static bool subscribed = false;
  if (!subscribed) {
    mqttClient.subscribe(bar1Topic);
    mqttClient.subscribe(bar2Topic);
    subscribed = true;
    Serial.println("Subscribed to topics");
  }

  interpolatePaddle(paddle1X, targetPaddle1X);
  interpolatePaddle(paddle2X, targetPaddle2X);

  if (gameRunning && publishFlag) {
    updateGame();
    publishData();
    publishFlag = false;
  }

  updateCountdown();
  mqttClient.loop();
}

//*************************************************************
//   GAME LOGIC FUNCTIONS
//*************************************************************
void updateGame() {
  // Move ball1 if active
  if (ball1Active) moveBall(ball1X, ball1Y, ball1SpeedX, ball1SpeedY, ball1Active, paddle1X, paddle1Y, 1);
  else resetBall(ball1X, ball1Y, ball1SpeedX, ball1SpeedY, ball1Active);

  // Move ball2 if active
  if (ball2Active) moveBall(ball2X, ball2Y, ball2SpeedX, ball2SpeedY, ball2Active, paddle2X, paddle2Y, 2);
  else resetBall(ball2X, ball2Y, ball2SpeedX, ball2SpeedY, ball2Active);
}

void updateCountdown() {
  unsigned long elapsed = millis() - countdownStart;

  // If time over, stop game
  if (elapsed >= countdownDuration) {
    stopGame();
  } else {  // If not, update time left
    int timeLeft = (countdownDuration - elapsed) / 1000;
    publishTime(timeLeft);
  }
}

void stopGame() {
  // Stop the game and publish time
  gameRunning = false;
   if (!gameOverSent) { 
    Serial.println("Game Over: Time's up!");
    mqttClient.publish(timeTopic, "00:00");
    gameOverSent = true; 
  }
}


/*************************************************************
*   MOVE BALL
*************************************************************/
void moveBall(float &ballX, float &ballY, float &ballSpeedX, float &ballSpeedY, bool &ballActive, float &paddleX, float &paddleY, int ballNum) {
  ballX += ballSpeedX;
  ballY += ballSpeedY;

  // Check lateral wall collisions
  if (ballX <= wallThickness || ballX >= canvasWidth - wallThickness - ballDiameter) {
    ballSpeedX *= -1;
  }

  // Check top wall collisions
  if (ballY <= wallThickness + 80) {
    ballSpeedY *= -1;
  }

  // Check ball1 collision with paddle1
  if (ballNum == 1 && ballY + ballDiameter >= paddleY && ballY <= paddleY + paddleHeight) {
    if (ballX >= paddleX && ballX <= paddleX + paddleWidth) {
      float relativeIntersectX = (ballX + ballDiameter / 2) - (paddleX + paddleWidth / 2);
      float normalizedRelativeIntersectionX = relativeIntersectX / (paddleWidth / 2);

      ballSpeedX = normalizedRelativeIntersectionX * 5;
      ballSpeedY *= -1;
      ballY = paddleY - ballDiameter;
    }

    // Check ball2 collision with paddle2
  } else if (ballNum == 2 && ballY + ballDiameter >= paddleY && ballY <= paddleY + paddleHeight) {
    if (ballX >= paddleX && ballX <= paddleX + paddleWidth) {
      float relativeIntersectX = (ballX + ballDiameter / 2) - (paddleX + paddleWidth / 2);
      float normalizedRelativeIntersectionX = relativeIntersectX / (paddleWidth / 2);

      ballSpeedX = normalizedRelativeIntersectionX * 5;
      ballSpeedY *= -1;
      ballY = paddleY - ballDiameter;
    }
  }

  // Check collision with blocks
  for (int row = 0; row < rows; row++) {
    for (int col = 0; col < cols; col++) {
      if (blocks[row][col]) {
        int x = wallThickness + col * (blockWidth + blockSpacing);
        int y = topSpacing + row * (blockHeight + blockSpacing);
        if (ballX + ballDiameter > x && ballX < x + blockWidth && ballY + ballDiameter > y && ballY < y + blockHeight) {
          blocks[row][col] = false;  // If block collision, deactivate block
          ballSpeedY *= -1;
          if (ballNum == 1) {
            score1++;
            publishPoints1();
          };  // Update score for ball1
          if (ballNum == 2) {
            score2++;
            publishPoints2();

          };  // Update score for ball2
        }
      }
    }
  }

  // Check if ball is out of canvas
  if (ballY > canvasHeight) {
    ballActive = false;
  }
}

/*************************************************************
*   NETWORK COMMUNICATION FUNCTIONS
*************************************************************/
void callback(char *topic, byte *payload, unsigned int length) {

  // Read and store message
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  // Check for new bar position and modify it locally
  float newPaddleX = message.toFloat();
  if (strcmp(topic, bar1Topic) == 0) {
    targetPaddle1X = constrain(newPaddleX - 75, wallThickness, canvasWidth - wallThickness - paddleWidth);
  } else if (strcmp(topic, bar2Topic) == 0) {
    targetPaddle2X = constrain(newPaddleX - 75, wallThickness, canvasWidth - wallThickness - paddleWidth);
  }
}

void publishTime(int secondsLeft) {

  // Calculate time left and publish it in mm:ss format
  int minutes = secondsLeft / 60;
  int seconds = secondsLeft % 60;
  char timeBuffer[6];
  sprintf(timeBuffer, "%02d:%02d", minutes, seconds);
  mqttClient.publish(timeTopic, timeBuffer);
}

void publishData() {

  // Publish ball position
  String ball1Data = String(ball1X) + "," + String(ball1Y);
  String ball2Data = String(ball2X) + "," + String(ball2Y);
  mqttClient.publish(ball1Topic, ball1Data.c_str());
  mqttClient.publish(ball2Topic, ball2Data.c_str());

  // Publish block data
  String blockData = "";
  for (int i = 0; i < rows; i++) {
    for (int j = 0; j < cols; j++) {
      blockData += String(blocks[i][j]) + ",";  // Enviar estado de los bloques
    }
  }
  mqttClient.publish(blocksTopic, blockData.c_str());

  // Publish points
  char score1Payload[10], score2Payload[10];
  sprintf(score1Payload, "%d", score1);
  sprintf(score2Payload, "%d", score2);
  mqttClient.publish(points1Topic, score1Payload);
  mqttClient.publish(points2Topic, score2Payload);
}

void publishPoints1() {
  static int lastScore1 = -1;

  if (score1 != lastScore1) {
    char score1Payload[10];
    sprintf(score1Payload, "%d", score1);
    mqttClient.publish(points1Topic, score1Payload);
    lastScore1 = score1;
  }
  
}

void publishPoints2() {
  static int lastScore2 = -1;

  if (score2 != lastScore2) {
    char score2Payload[10];
    sprintf(score2Payload, "%d", score2);
    mqttClient.publish(points2Topic, score2Payload);
    lastScore2 = score2;
  }
}


void connect() {
  while (!mqttClient.connected()) {
    Serial.println("Attempting MQTT connection...");
    if (mqttClient.connect(client_id)) {
      Serial.println("Connected to MQTT broker!");
    } else {
      Serial.print("Connection failed, rc=");
      Serial.println(mqttClient.state());
      delay(2000);
    }
  }
}

/*************************************************************
*   HELPER FUNCTIONS
*************************************************************/
void resetBall(float &ballX, float &ballY, float &ballSpeedX, float &ballSpeedY, bool &ballActive) {

  // Position ball to a random X coordinate and fixed Y coordinate
  ballX = random(wallThickness + 50, canvasWidth - wallThickness - 50);
  ballY = topSpacing + rows * (blockHeight + blockSpacing) + 50;

  // Move ball in a random angle (X speed)
  ballSpeedX = random(-5, 5);
  ballSpeedY = 5;

  // Activate the ball
  ballActive = true;
}

void interpolatePaddle(float &paddleX, float &targetPaddleX) {
  paddleX = lerp(paddleX, targetPaddleX, paddleSpeed);

  if (paddleX < wallThickness) {
    paddleX = wallThickness;
  } else if (paddleX + paddleWidth > canvasWidth - wallThickness) {
    paddleX = canvasWidth - wallThickness - paddleWidth;
  }
}

float lerp(float start, float end, float amt) {
  return start + (end - start) * amt;
}
