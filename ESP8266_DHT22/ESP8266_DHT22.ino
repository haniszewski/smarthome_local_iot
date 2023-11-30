#include "ESP8266WiFi.h"
#include "ESP8266WebServer.h"
#include <DHT.h>

const char *ssid = ""; 
const char *password = "";

const char *master_host = "";
const short master_port = 12023;

//Master asks for ID, HWID
#define HWID 0x0002

// Messages
#define KEEPALIVE 0x0000
#define KEEPALIVE_ACK 0x0001

#define WHO_REQUEST 0x0002
#define WHO_RESPONSE 0x0003

#define STATUS_REQUEST 0x0004
#define STATUS_RESPONSE 0x0005

// Most likely to have length 10
// Temperature param
// bytes 0 - header (0x80)
// bytes 1 - 2 type
// bytes 3 - 4 length
// bytes 5 - id of sensor
// bytes 6 - 9 - sensor data (humi/temp)

#define DHTPIN 5

// Uncomment the type of sensor in use:
//#define DHTTYPE    DHT11     // DHT 11
#define DHTTYPE    DHT22     // DHT 22 (AM2302)
//#define DHTTYPE    DHT21     // DHT 21 (AM2301)

unsigned long previousMillis = 0;    // will store last time DHT was updated

// Updates DHT readings every 30 seconds
const long interval = 10000; 

float temp = 0.0;
float humi = 0.0;

ESP8266WebServer server(80);
WiFiClient wClient;
DHT dht(DHTPIN, DHTTYPE);

void updateTemp(){
  float newtemp = dht.readTemperature();
  if (!isnan(newtemp)){
    temp = newtemp;
  }
}

void updateHumi(){
  float newhumi = dht.readHumidity();
  if (!isnan(newhumi)){
    humi = newhumi;
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

void sendKeepAliveAck() {
  uint8_t response[] = {0x00,0x00,0x01,0x00,0x05};
  wClient.write(response,sizeof(response));
}

void sendHWID() {
  uint16_t hwid = HWID;
  uint8_t hwid_1 = (hwid >> 8);
  uint8_t hwid_2 = hwid & 0xff;

  uint8_t response[] = {0x00,0x00,0x03,0x00,0x07,hwid_1,hwid_2};
  wClient.write(response,sizeof(response));
}

void sendStatus(){
  uint8_t* humi_bytes;
  humi_bytes = new uint8_t[4];
  uint8_t* temp_bytes;
  temp_bytes = new uint8_t[4];

  *humi_bytes = humi;
  *temp_bytes = temp;

  uint8_t response[] = {
    0x00,0x00,0x03,0x00,0x19, // TODO verify length
    0x80,0x00,0x01,0x00,0x0a,0x01, temp_bytes[0],temp_bytes[1],temp_bytes[2],temp_bytes[3],
    0x80,0x00,0x02,0x00,0x0a,0x01, humi_bytes[0],humi_bytes[1],humi_bytes[2],humi_bytes[3]
  };
  delete[] humi_bytes;
  delete[] temp_bytes;

  wClient.write(response,sizeof(response));  
}



void receive_data(){
  uint8_t recv_buff[5];

  if(wClient.read(recv_buff, sizeof(recv_buff)) != sizeof(recv_buff)){
    // handle error?
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
        sendStatus();
      break;
    }
  }else{

  }
}


void setup() {
  // put your setup code here, to run once:
  initConnection();
}

void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval){
    previousMillis = currentMillis;
    updateTemp();
    updateHumi();
  }
  if(wClient.connected()){
    if(wClient.available()){
      receive_data();
    }
  }else{
      reconnectToServer();
  }
}
