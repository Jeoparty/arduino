#define MAX_BUZZERS 1
#define FIRST_DETECTION_PIN 22
#define FIRST_BUZZER_PIN 23
#define FIRST_COLOR_PIN 8
#define INPUT_PIN_GROUP_SIZE 2

#define OPCODE_RESET 0x01
#define OPCODE_PING 0x02
#define OPCODE_PONG 0x03
#define OPCODE_BUZZ 0x04
#define OPCODE_SET_COLOR 0x05
#define OPCODE_CONNECTED 0x06
#define OPCODE_DISCONNECTED 0x07

bool buzzerConnected[MAX_BUZZERS];
int oldBuzzerState[MAX_BUZZERS];

// Returns an array of size MAX_BUZZERS indicating which buzzers are connected
void detectBuzzers(bool *buzzers)
{
  for (int i = 0;i < MAX_BUZZERS;i++)
  {
    buzzers[i]  = digitalRead(FIRST_DETECTION_PIN + i * INPUT_PIN_GROUP_SIZE) == LOW ? true : false;
  }
}

void setup() {
  // set up pins
  for (int i = 0;i < MAX_BUZZERS * 3;i++)
  {
    pinMode(i + FIRST_COLOR_PIN, OUTPUT);
  }
  for (int i = 0;i < MAX_BUZZERS;i++)
  {
    pinMode(FIRST_DETECTION_PIN + i * INPUT_PIN_GROUP_SIZE, INPUT_PULLUP);
  }
  for (int i = 0;i < MAX_BUZZERS;i++)
  {
    pinMode(FIRST_BUZZER_PIN + i * INPUT_PIN_GROUP_SIZE, INPUT_PULLUP);
  }
  
  // Initiate serial communication
  Serial.begin(9600);
  
  // Send magic number
  Serial.write(0x00);
  Serial.write(0x42);
  
  // Send protocol version
  Serial.write(0x00);
  
  // Wait for mediator handshake
  while (Serial.available() < 2)
  {
    // Nothing to do
  }
  
  // Check the magic number (two bytes) sent by the mediator
  if (Serial.read() != 0x13 || Serial.read() != 0x37)
    exit(1);
  
  detectBuzzers(buzzerConnected);
  
  // Send list of connected buzzers
  unsigned char noConnectedBuzzers = 0;
  for (unsigned char i = 0;i < MAX_BUZZERS;i++)
  {
    if (buzzerConnected)
      noConnectedBuzzers++;
  }
  Serial.write(noConnectedBuzzers);
  for (unsigned char i = 0;i < MAX_BUZZERS;i++)
  {
    if (buzzerConnected)
      Serial.write(i);
  }
}

void loop() {
  // Check connected buzzers and notify mediator about connects/disconnects
  bool newBuzzerConnectionState[MAX_BUZZERS];
  detectBuzzers(newBuzzerConnectionState);
  for (unsigned char i = 0;i < MAX_BUZZERS;i++)
  {
    if (buzzerConnected[i] != newBuzzerConnectionState[i])
    {
      buzzerConnected[i] = newBuzzerConnectionState[i];
      if (buzzerConnected[i] == true)
      {
        Serial.write(0x06);
        Serial.write(i);
      }
      else
      {
        Serial.write(0x07);
        Serial.write(i);
      }
    }
  }
  
  // Check if a buzzer is hit and notify the mediator
  for (unsigned char i = 0;i < MAX_BUZZERS;i++)
  {
    if (!buzzerConnected[i])
      continue;
    int buzzerState = digitalRead(FIRST_BUZZER_PIN + i * INPUT_PIN_GROUP_SIZE);
    if (buzzerState != oldBuzzerState[i])
    {
      oldBuzzerState[i] = buzzerState;
      if (buzzerState == LOW)
      {
        Serial.write(0x04);
        Serial.write(i);
      }
    }
  }
  
  // Check for messages form the mediator
  bool notEnoughData = false;
  while (Serial.available() > 0 && !notEnoughData)
  {
    switch (Serial.peek())
    {
      case OPCODE_PING:
        Serial.read();
        Serial.write(OPCODE_PONG);
        break;
      case OPCODE_PONG:
        Serial.read();
        break;
      case OPCODE_SET_COLOR:
        if (Serial.available() < 5)
        {
          notEnoughData = true;
          break;
        }
        Serial.read();
        unsigned char buzzer_id;
        buzzer_id = Serial.read();
        analogWrite(FIRST_COLOR_PIN + buzzer_id * 3 + 0, Serial.read());
        analogWrite(FIRST_COLOR_PIN + buzzer_id * 3 + 1, Serial.read());
        analogWrite(FIRST_COLOR_PIN + buzzer_id * 3 + 2, Serial.read());
        break;
      default:
      case OPCODE_RESET:
        Serial.read();
        // TODO Handle reset command
        break;
    }
  }
}
