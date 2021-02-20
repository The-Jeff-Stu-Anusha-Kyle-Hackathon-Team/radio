#include "src/Rfm95w/RH_RF95.h"
#include "src/time/epoch.h"

#define MAX_CONNECTIONS 10
#define MAX_MESSAGES 100
#define PING_DELTA 60000

static const uint8_t ID = 0x42;

const int LED = 33;
const int RESET_PIN = 8;

RH_RF95 radio = RH_RF95 (10, 9);

long last_ping = 0;

enum state_t {
  PING, // { ID, PING }
  PING_RESPONSE, // { ID, PING_RESPONSE }
  MESSAGE, // { ID, MESSAGE, NUM, LENGTH, [MSG] }
  REQUEST, // { ID, REQUEST }
  CONTINUE, // { ID, CONTINUE, NUM }
  STOP, // { ID, STOP }
};

struct connection_t {
  uint8_t id;
  state_t state;
};

struct connection_t connections[MAX_CONNECTIONS]{};
uint8_t num_connections = 0;

struct message_t {
  uint8_t unix_timestamp;
  String message;
};

struct message_t messages[MAX_MESSAGES]{};
uint8_t num_messages = 0;

void setup() {
  Serial.begin(9600);
  pinMode(LED, OUTPUT);
  // manual reset hack
  pinMode(RESET_PIN, OUTPUT);
  digitalWrite(RESET_PIN, LOW);
  delay(10);
  digitalWrite(RESET_PIN, HIGH);
  delay(10);

  if(!radio.init()) {
    Serial.println("Radio init failed");
    while(1);
  }
  
  messages[0] = { 72, "Stored message 1" };
  num_messages++;
  messages[1] = { 100, "Stored message 2" };
  num_messages++;
}

void loop() {
  // ping every x milliseconds
  if(millis() - last_ping >= PING_DELTA) {
    Serial.println("Pinging ID");
    uint8_t buf[] = { ID, PING };
    uint8_t msg_size = sizeof(buf)/sizeof(buf[0]);
    radio.send(buf, msg_size);
  }
  
  if (radio.available()) {
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len = 0;
    if (radio.recv(buf, &len)) {
      const uint8_t recv_id = buf[0], recv_state = buf[1];
      Serial.print("Message from 0x"); Serial.print(recv_id, HEX); Serial.print(" - "); Serial.print(recv_id); Serial.print(" - ");
      bool exists = false;
      for(int i = 0; i < num_connections; i++) {
        // check if we already made contact with this node
        if(connections[i].id == recv_id) {
          exists = true; // exists so make sure we don't add it later

          // this is the return to a ping we already sent out
          if(recv_state == PING_RESPONSE) {
            Serial.print("Received ping");
            connections[i].state = MESSAGE;
            String message = "Hello World!";
            uint8_t buf[message.length() + 4] = { ID, MESSAGE, 0 };
            int length = 0;
            for(uint8_t i = 4, j = 0; i < message.length(); i++, j++) {
              buf[i] = (uint8_t)message[j];
              length++;
            }
            buf[3] = length;
            uint8_t msg_size = sizeof(buf)/sizeof(buf[0]);
            radio.send(buf, msg_size);
          }

          // asking for next message
          else if(recv_state == CONTINUE) {
            connections[i].state = MESSAGE;
            uint8_t num = buf[2];
            if(num >= num_messages) {
              Serial.print("Out of messages");
              uint8_t buf[] = { ID, STOP };
              uint8_t msg_size = sizeof(buf)/sizeof(buf[0]);
              radio.send(buf, msg_size);
            }
            else {
              Serial.print("Sending next message");
              uint8_t num = buf[2];
              String message = messages[num].message;
              uint8_t buf[message.length() + 4] = { ID, MESSAGE, num };
              int length = 0;
              for(uint8_t i = 4, j = 0; i < message.length(); i++, j++) {
                buf[i] = (uint8_t)message[j];
                length++;
              }
              buf[3] = length;
              uint8_t msg_size = sizeof(buf)/sizeof(buf[0]);
              radio.send(buf, msg_size);
            }
          }

          // now our turn to ask for messages
          else if(recv_state == STOP) {
            Serial.print("Asking for messages");
            connections[i].state = REQUEST;
            uint8_t buf[] = { ID, REQUEST };
            uint8_t msg_size = sizeof(buf)/sizeof(buf[0]);
            radio.send(buf, msg_size);
          }

          // ask for the next message
          else if(recv_state == MESSAGE) {
            Serial.print("Asking for next message");
            connections[i].state = CONTINUE;
            uint8_t next_num = buf[2]++;
            String message = "";
            for(uint8_t i = 4; i < buf[3]+4; i++) {
              message += (char)buf[i];
            }
            messages[num_messages].message = message;
            num_messages++;
            uint8_t buf[] = { ID, CONTINUE, next_num };
            uint8_t msg_size = sizeof(buf)/sizeof(buf[0]);
            radio.send(buf, msg_size);
          }

          else {
            Serial.print("Invalid request");
          }
          Serial.println();
        }
      }

      // we haven't communicated with this node before, add it and send a ping back
      if(!exists && num_connections <= MAX_CONNECTIONS) {
        connection_t new_connection = connection_t();
        new_connection.id = buf[0];
        new_connection.state = PING_RESPONSE;
        connections[++num_connections] = new_connection;
        uint8_t buf[] = { ID, PING_RESPONSE };
        uint8_t msg_size = sizeof(buf)/sizeof(buf[0]);
        radio.send(buf, msg_size);
        
      }
    }
    else {
      Serial.println("recv failed");
    }
  }
}
