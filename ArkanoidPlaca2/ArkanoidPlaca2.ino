/*************************************************************
*   IMPORTS
*************************************************************/

#include <Ethernet2.h>
#include <PubSubClient.h>
#include "Timer1.h"
#include "LCD.h"


/*************************************************************
*   NETWORK CONFIGURATION
*************************************************************/

// Instantiates an Ethernet client and MQTT client
EthernetClient ethClient;
PubSubClient mqttClient(ethClient);

// Ethernet MAC address
const uint8_t myMAC_LSB = 0x04;
byte mac[] = { 0x00, 0x00, 0x00, 0x00, 0x00, myMAC_LSB };

// HOME Network parameters
// IPAddress ip(192, 168, 1, 151);
// IPAddress gw(192, 168, 1, 1);

// UIB Network parameters
const uint8_t myHostAddress = 163;
IPAddress ip(192, 168, 132, myHostAddress); // This host IP
IPAddress gw(192, 168, 132, 7); // Gateway IP

IPAddress mydns(8, 8, 8, 8);

// MQTT broker information
const char broker_name[] = "tom.uib.es";
const unsigned int broker_port = 1883;
const char client_id[] = "mrm2";  // Unique client ID para el segundo Arduino

// MQTT topics
const char *bar1Topic = "Arkanoid/bar1";
const char *bar2Topic = "Arkanoid/bar2";
const char *points1Topic = "Arkanoid/points1";
const char *points2Topic = "Arkanoid/points2";


/*************************************************************
*   INPUT/OUTPUT CONSTANTS & VARIABLES
*************************************************************/

// Joystick pins
const uint8_t JOYSTICK_1_X_PIN = A0;  // Joystick 1 X-axis pin
const uint8_t JOYSTICK_2_X_PIN = A1;  // Joystick 2 X-axis pin

// LCD pins
const uint8_t PIN_RS = A2;
const uint8_t PIN_RW = A3;
const uint8_t PIN_EN = A4;

const uint8_t PIN_DB0 = A7;
const uint8_t PIN_DB1 = A8;
const uint8_t PIN_DB2 = A9;
const uint8_t PIN_DB3 = A10;
const uint8_t PIN_DB4 = A11;
const uint8_t PIN_DB5 = A12;
const uint8_t PIN_DB6 = A13;
const uint8_t PIN_DB7 = A14;

// Buzzer
const uint8_t BUZZER_PIN = 7;  

// Joystick mapped values
const int minMappedValue = 20;  
const int maxMappedValue = 780; 

// Timer instance
Timer1 timer;
volatile bool publishFlag = false;

// LCD instance
LCD lcd(
  PIN_RS, PIN_RW, PIN_EN,
  PIN_DB0, PIN_DB1, PIN_DB2, PIN_DB3,
  PIN_DB4, PIN_DB5, PIN_DB6, PIN_DB7
);

/*************************************************************
*   GAME VARIABLES
*************************************************************/
// Points char []
char points1[10] = "0";
char points2[10] = "0";

// Variables para almacenar los puntos previos
int previousPoints1 = -1;
int previousPoints2 = -1;


/*************************************************************
*   INTERRUPT SERVICE ROUTINE (ISR)
*************************************************************/
ISR(TIMER1_COMPA_vect) {
  publishFlag = true;
}

// /******************************************************************************/
// // Function declarations
// void connect();

// void publishJoystickValues();

/*************************************************************
*   SETUP FUNCTION
*************************************************************/
void setup() {
  Serial.begin(115200);

  lcd.init();

  // LCD setup
  lcd.clear();
  
  lcd.moveCursor(0, 0);
  lcd.print("POINTS1: ");

  lcd.moveCursor(1, 0);
  lcd.print("POINTS2: ");
  pinMode(BUZZER_PIN, OUTPUT); 

  Ethernet.begin(mac, ip, mydns, gw);
  mqttClient.setServer(broker_name, broker_port);
  mqttClient.setCallback(callback);

  // Initialize and start Timer1 (100ms interval)
  timer.init(PS_256, 6250);  // 16MHz / 256 prescaler -> 37500 ticks = 600ms
  timer.start();
}


/*************************************************************
*   MAIN LOOP FUNCTION
*************************************************************/
void loop() {
  connect();

  // Suscribe to topics
  static bool subscribed = false;
  if (!subscribed) {
    mqttClient.subscribe(points1Topic);
    mqttClient.subscribe(points2Topic);
    subscribed = true;
    Serial.println("Subscribed to topics");
  }

  // Publish joystick values
  if (publishFlag) {
    publishJoystickValues(); 
    publishFlag = false;
  }

  mqttClient.loop(); 
}


/*************************************************************
*   NETWORK COMMUNICATION FUNCTIONS
*************************************************************/
void publishJoystickValues() {
  int joystickValue1 = analogRead(JOYSTICK_1_X_PIN);
  int joystickValue2 = analogRead(JOYSTICK_2_X_PIN);

  int mappedValue1 = map(joystickValue1, 0, 1023, minMappedValue, maxMappedValue);
  int mappedValue2 = map(joystickValue2, 0, 1023, minMappedValue, maxMappedValue);

  // Convert mapped values to strings
  char payload1[10], payload2[10];
  sprintf(payload1, "%d", mappedValue1);
  sprintf(payload2, "%d", mappedValue2);

  // Publish values to respective topics
  mqttClient.publish(bar1Topic, payload1);
  mqttClient.publish(bar2Topic, payload2);
}

void callback(char* topic, byte* payload, unsigned int length) {
  char message[length + 1];
  memcpy(message, payload, length);
  message[length] = '\0';

  // Turn message to integer
  int currentPoints = atoi(message);

  if (strcmp(topic, points1Topic) == 0) {
    if (currentPoints != previousPoints1) {  
      previousPoints1 = currentPoints;  
      sprintf(points1, "%d", currentPoints);  
      soundBuzzer();
      lcd.moveCursor(0, 8);  // Move cursor
      lcd.print(points1);  // Print points1
    }
  } else if (strcmp(topic, points2Topic) == 0) {
    if (currentPoints != previousPoints2) { 
      previousPoints2 = currentPoints;  
      sprintf(points2, "%d", currentPoints);  
      soundBuzzer();
      lcd.moveCursor(1, 8);  // Move cursor
      lcd.print(points2);  // Print points1
    }
  }
}

void connect() {
  while (!mqttClient.connected()) {
    Serial.println("Attempting MQTT connection...");
    if (mqttClient.connect(client_id)) {
      Serial.println("Connected to MQTT broker!");
    } else {
      Serial.print("Connection failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" retrying in 2 seconds...");
      delay(5000);  // Intentar nuevamente después de 2 segundos
    }
  }
}

/*************************************************************
*   HELPER FUNCTIONS
*************************************************************/
void soundBuzzer() {
  static bool buzzerState = false;

  Serial.println("buzz buzz buzzz");
  if (buzzerState) {
    digitalWrite(BUZZER_PIN, LOW);  // Turn off buzzer
  } else {
    digitalWrite(BUZZER_PIN, HIGH); // Turn on buzzer
  }
  buzzerState = !buzzerState;  
}

