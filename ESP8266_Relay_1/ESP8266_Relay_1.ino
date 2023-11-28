#include "ESP8266WiFi.h"
#include "ESP8266WebServer.h"

#define RELAY0 0

const char *ssid = ""; 
const char *password = "";

const char *master_host = "";
const short master_port = ;

//Master asks for ID, HWID
#define HWID 0x0000

// Messages
#define KEEPALIVE 0x0000
#define KEEPALIVE_ACK 0x0001

#define WHO_REQUEST 0x0002
#define WHO_RESPONSE 0x0003

#define STATUS_REQUEST 0x0004
#define STATUS_RESPONSE 0x0005

#define SET_RELAYS_REQUEST 0x0006
#define SET_RELAYS_RESPONSE 0x0007

ESP8266WebServer server(80);
WiFiClient wClient;
bool relay0 = false;

void switchLed(){
  relay0 = !relay0;
  if(relay0){
    digitalWrite(RELAY0, HIGH);
  }else{
    digitalWrite(RELAY0, LOW);
  }
}

void writeRelays(){
  if(relay0){
    digitalWrite(RELAY0, LOW);
  }else{
    digitalWrite(RELAY0, HIGH);
  }
}


void reconnectToServer(){
  wClient.stop();
  wClient.connect(master_host,master_port);
}

void initConnection(){
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  wClient.connect(master_host,master_port);
}

void sendKeepAliveAck(){
    uint8_t response[] = {0x00,0x00,0x01,0x00,0x05};
    wClient.write(response,sizeof(response));
}

void sendHWID(){
  uint16_t hwid = HWID;
  uint8_t hwid_1 = (hwid >> 8);
  uint8_t hwid_2 = hwid & 0xff;
  uint8_t response[] = {0x00,0x00,0x03,0x00,0x07,hwid_1,hwid_2};
  wClient.write(response,sizeof(response));
}

void sendSTATUS(){
  uint8_t relay0_status = 0x00;
  if(relay0){
    relay0_status |= 0x01;
  }

  uint8_t response[] = {0x00,0x00,0x05,0x00,0x0B, // Message
                        0x00,0x00,0x03,0x00,0x06,relay0_status}; // Param
  wClient.write(response,sizeof(response));
}

uint16_t handleSetRelayParam(uint8_t* data){
  uint16_t param_type = (data[1] << 8) | data[2];
  uint16_t param_len = (data[3] << 8) | data[4];

  uint8_t relay_id = (data[5] >> 1);
  relay0 = ((data[5] & 0x01) == 1);

  // switchLed();

  // writeRelays();

  // if(relay0){
  //   digitalWrite(RELAY0, LOW);
  // }else{
  //   digitalWrite(RELAY0, HIGH);
  // }

  return param_len;
}

void handleSetRelaysRequest(uint8_t* data, uint16_t length){
  uint16_t remaining_length = length - 5;
  while(remaining_length > 0){
    remaining_length -= handleSetRelayParam(data+(length-remaining_length));
  }
}

void receive_data(){
  uint8_t recv_buff[5];
  if(wClient.read(recv_buff, sizeof(recv_buff)) != sizeof(recv_buff)){
    // Handle read error
    return;
  }
  uint16_t msg_type = (recv_buff[1] << 8) | recv_buff[2];
  uint16_t msg_len = (recv_buff[3] << 8) | recv_buff[4];
  if(msg_len == 5){
    switch(msg_type){
      case KEEPALIVE:
        sendKeepAliveAck();
      break;
      case WHO_REQUEST:
        sendHWID();
      break;
      case STATUS_REQUEST:
        sendSTATUS();
      break;
    }
  }else{
    uint8_t* full_recv_buff;
    full_recv_buff = new uint8_t[msg_len];
    if(full_recv_buff == nullptr){
      // Handle memory allocation failure
      return;
    }

    memcpy(full_recv_buff, recv_buff, sizeof(recv_buff));

    if(wClient.read(full_recv_buff + sizeof(recv_buff), msg_len - sizeof(recv_buff)) != (msg_len - sizeof(recv_buff))){
        // Handle read error
        delete[] full_recv_buff;
        return;
    }

    // Handle message
    switch(msg_type){
      case SET_RELAYS_REQUEST:
        handleSetRelaysRequest(full_recv_buff, msg_len);
      break;
    }

    delete full_recv_buff;
  }
  
}

void setup() {
  pinMode(RELAY0, OUTPUT);
  initConnection();
}

void loop() {
  if(wClient.connected()){
    if(wClient.available()){
      receive_data();
      // switchLed();
      writeRelays();

    }
  }else{
      reconnectToServer();
  }
}
