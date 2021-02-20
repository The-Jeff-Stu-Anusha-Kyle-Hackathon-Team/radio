#include "src/Rfm95w/RH_RF95.h"

#define MAX_CONNECTIONS 10
#define MAX_MESSAGES 100
#define PING_DELTA 60000

static const uint8_t ID = 0x42;

const int LED = 33;
const int RESET_PIN = 8;

RH_RF95 radio = RH_RF95 (10, 9);

long last_ping = 0;

enum state_t {
  PING,
  HANDSHAKE,
  REQUEST,
  RESPONSE,
  END
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

struct message_t messages[MAX_MESSAGES];
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
      Serial.println("Received request");
      const uint8_t recv_id = buf[0];
      bool exists = false;
      for(int i = 0; i < num_connections; i++) {
        // check if we already made contact with this node
        if(connections[i].id == recv_id) {
          exists = true; // exists so make sure we don't add it later

          // this is the return to a ping we already sent out
          if(connections[i].state == PING) {
            // go into handshake mode where we send the number of messages we have ever received for comparison
            connections[i].state == HANDSHAKE;
            uint8_t buf[] = { ID, HANDSHAKE, num_messages };
            uint8_t msg_size = sizeof(buf)/sizeof(buf[0]);
            radio.send(buf, msg_size);
          }

          else if(connections[i].state == HANDSHAKE) {
            connections[i].state == REQUEST;
            if(buf[2] == num_connections) {
              // up to date!
              connections[i].state == END;
              uint8_t buf[] = { ID, END };
            }
            else {
              uint8_t request_timestamp = 0; // need to cut current timestamp in half
              uint8_t buf[] = { ID, REQUEST, request_timestamp };
              uint8_t msg_size = sizeof(buf)/sizeof(buf[0]);
              radio.send(buf, msg_size);
            }
          }
          else if(connections[i].state == REQUEST) {
            connections[i].state == RESPONSE;
            for(int i = num_messages-1; i >= 0; i--) {
              
            }
          }
        }
      }

      // we haven't communicated with this node before, add it and send a ping back
      if(!exists && num_connections <= MAX_CONNECTIONS) {
        connection_t new_connection = connection_t();
        new_connection.id = buf[0];
        new_connection.state = PING;
        connections[++num_connections] = new_connection;
        uint8_t buf[] = { ID, PING };
        uint8_t msg_size = sizeof(buf)/sizeof(buf[0]);
        radio.send(buf, msg_size);
        
      }
    }
    else {
      Serial.println("recv failed");
    }
  }
}
