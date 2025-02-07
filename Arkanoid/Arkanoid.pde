//*************************************************************
// IMPORTS
//*************************************************************

import org.eclipse.paho.client.mqttv3.*;
import java.nio.charset.StandardCharsets;

//*************************************************************
// CONSTANTS & DECLARATIONS
//*************************************************************

// MQTT constants & declarations
MqttClient mqttClient; // MQTT client
String brokerUrl = "tcp://tom.uib.es:1883";
String bar1Topic = "Arkanoid/bar1";
String bar2Topic = "Arkanoid/bar2";
String ball1Topic = "Arkanoid/ball1";
String ball2Topic = "Arkanoid/ball2";
String blocksTopic = "Arkanoid/blocks";
String points1Topic = "Arkanoid/points1";
String points2Topic = "Arkanoid/points2";
String timeTopic = "Arkanoid/time";

// Canvas constants
int canvasWidth = 800;
int canvasHeight = 800;

// Block constants
int rows = 6;
int cols = 13;
int blockWidth;
int blockHeight = 20;
int blockSpacing = 0;
int topSpacing = 200;

// Paddle constants
int paddleWidth = 150;
int paddleHeight = 20;

// Paddle 1
float paddle1X;
float paddle1Y;
float targetPaddle1X;

// Paddle 2
float paddle2X;
float paddle2Y;
float targetPaddle2X;

// Ball constants
float ballDiameter = 15;

// Score constants
int score = 0;
int maxScore = rows * cols;

// Wall constants
int wallThickness = 20;

// Image declarations
PImage topWallImage, leftWallImage, rightWallImage;
PImage paddleImage1, paddleImage2;
PImage ballImage1, ballImage2;
PImage[] blockImages = new PImage[6];
PImage logoImage;

// Font declarations
PFont customFont; // Custom font for text display

// Variable declarations
boolean[][] blocks;
float ball1X, ball2X, ball1Y, ball2Y;
int gameState = 0;
String paddlePosition = "0";  // Variable to store paddle position from MQTT

// Variables for paddle interpolation control
float paddleSpeed = 0.10;

// Time
String timeLeft = new String("0");  // Time in seconds

// Points
int points1 = 0;    // Points for player 1
int points2 = 0;    // Points for player 2


//*************************************************************
// SETTINGS
//*************************************************************

void settings() {
  size(canvasWidth, canvasHeight); // Set canvas size
}

//*************************************************************
// SETUP
//*************************************************************

void setup() {
  blockWidth = (canvasWidth - 2 * wallThickness - (cols - 1) * blockSpacing) / cols; // Calculate block width

  paddle1X = (canvasWidth - paddleWidth) / 2; // Set initial paddle X position
  paddle1Y = canvasHeight - 50; // Set initial paddle Y position
  targetPaddle1X = paddle1X; // Set target paddle position to initial position

  paddle2X = (canvasWidth - paddleWidth) / 2;
  paddle2Y = canvasHeight - 50;
  targetPaddle2X = paddle2X;

  // Load wall images
  topWallImage = loadImage("graphics/edge_top.png");
  topWallImage.resize(canvasWidth  - 40, wallThickness);
  leftWallImage = loadImage("graphics/edge_left.png");
  leftWallImage.resize(wallThickness, canvasHeight);
  rightWallImage = loadImage("graphics/edge_right.png");
  rightWallImage.resize(wallThickness, canvasHeight);

  // Load paddle image
  paddleImage1 = loadImage("graphics/paddle1.png");
  paddleImage1.resize(paddleWidth, paddleHeight);
  paddleImage2 = loadImage("graphics/paddle2.png");
  paddleImage2.resize(paddleWidth, paddleHeight);


  // Load ball image
  ballImage1 = loadImage("graphics/ball1.png");
  ballImage1.resize((int)ballDiameter, (int)ballDiameter);
  ballImage2 = loadImage("graphics/ball2.png");
  ballImage2.resize((int)ballDiameter, (int)ballDiameter);

  // Load block images
  blockImages[0] = loadImage("graphics/brick_silver.png");
  blockImages[1] = loadImage("graphics/brick_red.png");
  blockImages[2] = loadImage("graphics/brick_yellow.png");
  blockImages[3] = loadImage("graphics/brick_blue.png");
  blockImages[4] = loadImage("graphics/brick_pink.png");
  blockImages[5] = loadImage("graphics/brick_green.png");

  // Load custom font
  customFont = createFont("fonts/emulogic.ttf", 20);

  // Load logo image
  logoImage = loadImage("graphics/logo.png");

  // Initialize blocks (set all blocks to active)
  blocks = new boolean[rows][cols];
  for (int row = 0; row < rows; row++) {
    for (int col = 0; col < cols; col++) {
      blocks[row][col] = true; // All blocks are initially active
    }
  }

  try {
    // Initialize MQTT client
    mqttClient = new MqttClient(brokerUrl, MqttClient.generateClientId());
    mqttClient.setCallback(new MqttCallback() {
      @Override
        public void connectionLost(Throwable cause) {
        println("MQTT connection lost!");
        reconnect(); // Reconnect if connection is lost
      }

      @Override
        public void messageArrived(String topic, MqttMessage message) throws Exception {
        String payload = new String(message.getPayload(), StandardCharsets.UTF_8);

        // Convert received value to float and update target paddle position
        try {
          if (topic.equals(bar1Topic)) {
            float newPaddle1X = Float.parseFloat(payload);
            targetPaddle1X = constrain(newPaddle1X - 75, wallThickness, canvasWidth - wallThickness - paddleWidth);
          } else if (topic.equals(bar2Topic)) {
            float newPaddle2X = Float.parseFloat(payload);
            targetPaddle2X = constrain(newPaddle2X - 75, wallThickness, canvasWidth - wallThickness - paddleWidth);
          } else if (topic.equals(ball1Topic)) {
            String[] data = payload.split(",");
            ball1X = Float.parseFloat(data[0]);
            ball1Y = Float.parseFloat(data[1]);
          } else if (topic.equals(ball2Topic)) {
            String[] data = payload.split(",");
            ball2X = Float.parseFloat(data[0]);
            ball2Y = Float.parseFloat(data[1]);
          } else if (topic.equals(blocksTopic)) {
            updateBlocks(payload);
          } else if (topic.equals(points1Topic)) {
            points1 = Integer.parseInt(payload);  // Save points1
          } else if (topic.equals(points2Topic)) {
            points2 = Integer.parseInt(payload);  // Save points2
          } else if (topic.equals(timeTopic)) {
            timeLeft = payload;
          }
        }
        catch (NumberFormatException e) {
          println("Invalid payload received: " + payload);
        }
      }

      @Override
        public void deliveryComplete(IMqttDeliveryToken token) {
        // Not used for subscription
      }
    }
    );

    // Attempt to connect to the MQTT broker
    connectToBroker();
  }
  catch (MqttException e) {
    e.printStackTrace();
    println("Error connecting to broker: " + e.getMessage());
  }
}

//*************************************************************
// DRAW
//*************************************************************

void draw() {
 if (gameState == 0) {
    drawStartScreen(); // Dibujar pantalla de inicio
  } else if (gameState == 1) {
    drawGameScreen(); // Dibujar pantalla del juego
    
    // Si el tiempo llega a 00:00, cambiar a la pantalla de Game Over
    if (timeLeft.equals("00:00")) {
      gameState = 2;
    }
  } else if (gameState == 2) {
    drawGameOverScreen(); // Dibujar pantalla de fin del juego
  }
}

//*************************************************************
// DRAW: Start Screen
//*************************************************************

void drawStartScreen() {
  background(0); // Black background for start screen
  imageMode(CORNER); // Set image mode to CORNER to avoid misalignment
  logoImage.resize(500, 250); // Resize logo to fit header area
  image(logoImage, width / 2 - logoImage.width / 2, height / 6); // Center logo correctly

  fill(255); // White text color
  textSize(12);
  textAlign(CENTER, CENTER);
  text("Author:", width / 2, height / 2);
  text("Miquel Robles Mclean", width / 2, height / 2 + 20);

  textSize(10);
  text("IoT Device Development", width / 2, (height / 2) + 70);
  text("Universitat de les Illes Balears", width / 2, (height / 2) + 90);
  text("2025", width / 2, (height / 2) + 110);

  textFont(customFont); // Set custom font
  fill(255); // White text color
  textSize(20);
  textAlign(CENTER, CENTER);
  if (frameCount % 60 < 30) { // Blinking effect for "PRESS ANY KEY TO START"
    text("PRESS ANY KEY TO START", width / 2, height - 100);
  }
}

//*************************************************************
// DRAW: GAME SCREEN
//*************************************************************

void drawGameScreen() {
  background(0, 0, 128); // Dark blue background for game screen

  // Draw walls
  image(leftWallImage, 0, 80); // Left wall with image
  image(rightWallImage, canvasWidth - wallThickness, 80); // Right wall with image
  image(topWallImage, 20, 82); // Top wall with image

  // Draw the black background for the score area
  fill(0); // Black background for the score area
  rect(0, 0, canvasWidth, 80); // Rectangle covering the top part for points and high score

  // Draw logo to the left of "Points"
  logoImage.resize(250, 60); // Resize logo to fit header area
  image(logoImage, 10, 10); // Adjust position as needed

  // Draw header (points and high score)
  fill(255, 0, 0); // Red text color
  textFont(customFont); // Set the custom font
  textAlign(CENTER, CENTER);
  textSize(18);

  // Draw "Points1" in the center-left, accounting for logo spacing
  text("POINTS 1", canvasWidth / 4 + 170, 30);
  fill(255, 255, 255);
  text(points1, canvasWidth / 4 + 170, 60);
  
  fill(255, 0, 0); // Red text color
  // Draw "Points2" in the center-left, accounting for logo spacing
  text("POINTS 2", canvasWidth / 4 + 350, 30);
  fill(255, 255, 255);
  text(points2, canvasWidth / 4 + 350, 60);

  fill(255, 0, 0); // Red text color
  // Draw "High Score" on the right
  text("TIME", 3 * canvasWidth / 4 + 100, 30);
  fill(255, 255, 255); // White text color
  text(timeLeft, 3 * canvasWidth / 4 + 100, 60);

  // Draw blocks as images
  for (int row = 0; row < rows; row++) {
    for (int col = 0; col < cols; col++) {
      if (blocks[row][col]) {
        int x = wallThickness + col * (blockWidth + blockSpacing);
        int y = topSpacing + row * (blockHeight + blockSpacing);
        // Draw the corresponding block image
        image(blockImages[row], x, y, blockWidth, blockHeight);
      }
    }
  }

  // Draw Paddle 1 (bottom)
  image(paddleImage1, paddle1X, paddle1Y);
  paddle1X = lerp(paddle1X, targetPaddle1X, paddleSpeed);

  // Draw Paddle 2 (top)
  image(paddleImage2, paddle2X, paddle2Y);
  paddle2X = lerp(paddle2X, targetPaddle2X, paddleSpeed);


  // Verifica los límites de la pantalla sin "frenar" la barra
  if (paddle1X < wallThickness) {
    paddle1X = wallThickness; // No dejar que pase del límite izquierdo
  } else if (paddle1X + paddleWidth > canvasWidth - wallThickness) {
    paddle1X = canvasWidth - wallThickness - paddleWidth; // No dejar que pase del límite derecho
  }

  // Verifica los límites de la pantalla sin "frenar" la barra
  if (paddle2X < wallThickness) {
    paddle2X = wallThickness; // No dejar que pase del límite izquierdo
  } else if (paddle1X + paddleWidth > canvasWidth - wallThickness) {
    paddle2X = canvasWidth - wallThickness - paddleWidth; // No dejar que pase del límite derecho
  }

  // Dibuja la bola
  image(ballImage1, ball1X - ballDiameter / 2, ball1Y - ballDiameter / 2);
  image(ballImage2, ball2X - ballDiameter / 2, ball2Y - ballDiameter / 2);
}

//*************************************************************
// DRAW: Game Over Screen
//*************************************************************

void drawGameOverScreen() {
  background(0); // Fondo negro para la pantalla de Game Over
  imageMode(CENTER);
  
  // Mostrar el logo en pequeño en la parte superior
  logoImage.resize(200, 100);
  image(logoImage, width / 2, height / 6);
  
  // Mostrar texto de "Game Over" en el centro
  fill(255, 0, 0); // Texto rojo
  textFont(customFont);
  textSize(40);
  textAlign(CENTER, CENTER);
  text("GAME OVER", width / 2, height / 2);
  
  // Determinar y mostrar el ganador
  String winner;
  if (points1 > points2) {
    winner = "Player 1 Wins!";
  } else if (points2 > points1) {
    winner = "Player 2 Wins!";
  } else {
    winner = "It's a Tie!";
  }
  
  fill(255);
  textSize(30);
  text(winner, width / 2, height / 2 + 100);
  
  textSize(20);
  text("Press any key to exit", width / 2, height - 100);
}

//*************************************************************
// CONNECT TO MQTT BROKER
//*************************************************************

void connectToBroker() {
  // Attempt to connect to the MQTT broker
  try {
    mqttClient.connect(); // Connect to the broker
    mqttClient.subscribe(bar1Topic); // Subscribe to the bar1 topic
    mqttClient.subscribe(bar2Topic); // Subscribe to the bar2 topic
    mqttClient.subscribe(ball1Topic); // Subscribe to the ball1 topic
    mqttClient.subscribe(ball2Topic); // Subscribe to the ball2 topic
    mqttClient.subscribe(blocksTopic); // Subscribe to the blocks topic
    mqttClient.subscribe(points1Topic); // Subscribe to the points topic
    mqttClient.subscribe(points2Topic); // Subscribe to the points topic
    mqttClient.subscribe(timeTopic);   // Subscribe to the time topic

    println("Successful connection and subscription.");
  }
  catch (MqttException e) {
    e.printStackTrace();
    println("Error connecting to MQTT broker: " + e.getMessage());
  }
}

//*************************************************************
// RECONNECT TO MQTT BROKER
//*************************************************************

void reconnect() {
  // Attempt to reconnect to the broker if connection is lost
  boolean reconnected = false;
  while (!reconnected) {
    try {
      println("Attempting to reconnect...");
      mqttClient.connect(); // Try to connect again
      mqttClient.subscribe(bar1Topic); // Subscribe to the bar1 topic
      mqttClient.subscribe(bar2Topic); // Subscribe to the bar2 topic
      mqttClient.subscribe(ball1Topic); // Subscribe to the ball topic
      mqttClient.subscribe(ball2Topic); // Subscribe to the ball topic
      mqttClient.subscribe(blocksTopic); // Subscribe to the blocks topic
      mqttClient.subscribe(points1Topic); // Subscribe to the points topic
      mqttClient.subscribe(points2Topic); // Subscribe to the points topic
      mqttClient.subscribe(timeTopic);   // Subscribe to the time topic

      println("Reconnected successfully.");
      reconnected = true; // Connection re-established
    }
    catch (MqttException e) {
      e.printStackTrace();
      println("Error reconnecting: " + e.getMessage());
      delay(5000); // Wait 5 seconds before retrying
    }
  }
}

/*************************************************************
*   HELPER FUNCTIONS
*************************************************************/

void updateBlocks(String payload) {
  String[] blockStates = payload.split(",");
  if (blockStates.length == rows * cols) {
    int index = 0;
    for (int row = 0; row < rows; row++) {
      for (int col = 0; col < cols; col++) {
        blocks[row][col] = blockStates[index].equals("1");
        index++;
      }
    }
  } else {
    println("Error: Unexpected block data length");
  }
}

void keyPressed() {
  if (gameState == 0) {
    gameState = 1; // Cambiar al juego
  } else if (gameState == 2) {
    exit(); // Cerrar el programa
  }
}
