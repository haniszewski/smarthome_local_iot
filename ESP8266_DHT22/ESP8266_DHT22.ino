#include "ESP8266WiFi.h"
#include "ESP8266WebServer.h"
#include <DHT.h>

const char *ssid = ""; 
const char *password = "";

const char *master_host = "";
const short master_port = 12023;

//Master asks for ID, HWID
#define REQUEST_WHO 0b10110011

//Master commands
#define REQUEST_STATUS 0b11010011
#define REQUEST_PING 0b11111111

//Response codes
//HWID, Hardware ID response
#define RESPONSE_WHO 0b00000001
//Last byte transmited from device
#define RESPONSE_ACKNOWLEDGED 0b00000001

//Response about temp, 8 bits, 8 bits


#define DHTPIN 5

// Uncomment the type of sensor in use:
//#define DHTTYPE    DHT11     // DHT 11
#define DHTTYPE    DHT22     // DHT 22 (AM2302)
//#define DHTTYPE    DHT21     // DHT 21 (AM2301)

unsigned long previousMillis = 0;    // will store last time DHT was updated

// Updates DHT readings every 30 seconds
const long interval = 30000; 

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

void loadtemp(uint8_t* resp_buff){
  uint8_t* bytestemp;
  bytestemp = new uint8_t[4];
  *bytestemp = temp;
  resp_buff[0] = bytestemp[0];
  resp_buff[1] = bytestemp[1];
  resp_buff[2] = bytestemp[2];
  resp_buff[3] = bytestemp[3];
  delete bytestemp;
}

void loadhumi(uint8_t* resp_buff){
  uint8_t* byteshumi;
  byteshumi = new uint8_t[4];
  *byteshumi = temp;
  resp_buff[4] = byteshumi[0];
  resp_buff[5] = byteshumi[1];
  resp_buff[6] = byteshumi[2];
  resp_buff[7] = byteshumi[3];
  delete byteshumi;
}

void receive_data(){
  uint8_t recv_buff[2];
  uint8_t* resp_buff;
  wClient.read(recv_buff,sizeof(recv_buff));
  switch(recv_buff[0]){
    case REQUEST_WHO:
      resp_buff = new uint8_t[2];
      resp_buff[0] = RESPONSE_WHO;
      resp_buff[1] = RESPONSE_ACKNOWLEDGED;
      wClient.write((uint8_t*)&resp_buff,sizeof(resp_buff));
      break;
    case REQUEST_STATUS:
      resp_buff = new uint8_t[9];
      loadtemp(resp_buff);
      loadhumi(resp_buff);
      resp_buff[8] = RESPONSE_ACKNOWLEDGED;
      wClient.write((uint8_t*)&resp_buff,sizeof(resp_buff));
      break;
  }
  delete resp_buff;
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
