#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Ticker.h>

#define LED D0
#define echoPinIn D2
#define trigPinIn D3
#define echoPinOut D5
#define trigPinOut D6

// NetPie Constant
const char* ssid = "folk";
const char* password = "50830710";
const char* mqtt_server = "broker.netpie.io";
const int mqtt_port = 1883;
const char* mqtt_Client = "98dbe3df-84d6-46c9-a3b3-3abfbc9d8e5f";
const char* mqtt_username = "tVTbphFhaMzDV4Yr7qiSLzXKcgrW6WCq";
const char* mqtt_password = "WSM3oc(bFw)R6v_stQZo(WW65*Qy0SSt";
char msg[50];
WiFiClient espClient;
PubSubClient client(espClient);
Ticker t1;

long durationIn;
int distanceIn;
long durationOut;
int distanceOut;
int threashold;
int calibrate;
int People;
int time1;
int time2;
int sensor_timeout = 250;
int Emergency;

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection…");
    if (client.connect(mqtt_Client, mqtt_username, mqtt_password)) {
      Serial.println("connected");
      client.subscribe("@msg/floor");
    }
    else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println("try again in 5 seconds");
      delay(5000);
    }
  }
}

int getDistance(int trigPin, int echoPin){
  // Clears the TrigPin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  // Sets the TrigPin HIGH for 10 microseconds which will send the 8 ultrasonic wave.
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  // Reads the echoPin, returns the sound wave travel time in microseconds
  int duration = pulseIn(echoPin, HIGH);
  // Calculates the distance
  int distance = duration * 0.034/2 ; // Speed of sound wave multiply by duration/2
  return distance;
}

void showDistance(){
  Serial.print("DistanceIn: ");
  Serial.print(distanceIn);
  Serial.print(" cm");
  Serial.print(" DistanceOut: ");
  Serial.print(distanceOut);
  Serial.println(" cm");
}

void callback(char* topic, byte* payload, unsigned int length) {
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    String message;
    for (int i = 0; i < length; i++) {
        message = message + (char)payload[i];
    }
    Serial.println(message);
    if(String(topic) == "@msg/floor") {
      String data = "{\"data\": {\"floor\":" + message + "}}";
      data.toCharArray(msg, (data.length() + 1));
      client.publish("@shadow/data/update", msg);
      Serial.println(message[0]-'0');
      Serial.write(message[0]-'0');
    }
}

void send_data(){
  String data = "{\"data\": {\"People\":" + String(People) + "}}";
  Serial.println(data);
  data.toCharArray(msg, (data.length() + 1));
  client.publish("@shadow/data/update", msg);
}

void setup() {
  // put your setup code here, to run once:
  pinMode(LED, OUTPUT);
  pinMode(trigPinIn, OUTPUT);
  pinMode(echoPinIn, INPUT);
  pinMode(trigPinOut, OUTPUT);
  pinMode(echoPinOut, INPUT);
  Serial.begin(9600);

  for(int i=0; i<100; i++){
      calibrate += getDistance(trigPinIn, echoPinIn);
  }
  calibrate /= 70;
  threashold = calibrate * 0.5;

  //Connect NetPie and WIFI
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
//  t1.attach(2, send_data);
}

void loop() {
  // put your main code here, to run repeatedly:
  distanceIn = getDistance(trigPinIn, echoPinIn);
  distanceOut = getDistance(trigPinOut, echoPinOut);
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  if(Serial.available()){
    Emergency = Serial.read();
    String data = "{\"data\": {\"Emergency\":" + String(Emergency) + "}}";
    Serial.println(data);
    data.toCharArray(msg, (data.length() + 1));
    client.publish("@shadow/data/update", msg);
  }
  
//  showDistance();
//  Serial.println(calibrate);
  if(distanceIn < threashold && distanceOut > threashold){
    time1 = 0;
    while((distanceOut >= threashold || distanceOut > calibrate) && time1 <= sensor_timeout){
      distanceOut = getDistance(trigPinOut, echoPinOut);
      delayMicroseconds(250);
      time1++;
    }

    time2 = 0;
    while((distanceOut < threashold || distanceOut > calibrate) && time2 <= sensor_timeout){
      distanceOut = getDistance(trigPinOut, echoPinOut);
      delayMicroseconds(250);
      time2++;
    }

    if(time1<sensor_timeout && time2<sensor_timeout){
      People++;
      String data = "{\"data\": {\"People\":" + String(People) + "}}";
      Serial.println(data);
      data.toCharArray(msg, (data.length() + 1));
      client.publish("@shadow/data/update", msg);
      delay(700);
    }

  }
  else if(distanceOut < threashold && distanceIn > threashold){
    time1 = 0;
    while((distanceIn >= threashold || distanceIn > calibrate) && time1 <= sensor_timeout){
      distanceIn = getDistance(trigPinIn, echoPinIn);
      delayMicroseconds(250);
      time1++;
    }

    time2 = 0;
    while((distanceIn < threashold || distanceIn > calibrate) && time2 <= sensor_timeout){
      distanceIn = getDistance(trigPinIn, echoPinIn);
      delayMicroseconds(250);
      time2++;
    }

    if(time1<sensor_timeout && time2<sensor_timeout){
      People--;
      String data = "{\"data\": {\"People\":" + String(People) + "}}";
      Serial.println(data);
      data.toCharArray(msg, (data.length() + 1));
      client.publish("@shadow/data/update", msg);
      delay(700);
    }
  }

  Serial.print("People: ");
  Serial.print(People);
  Serial.print(" Calibrate: ");
  Serial.print(calibrate);
  Serial.print(" Threashold: ");
  Serial.print(threashold);
  Serial.print(" Emergency: ");
  Serial.println(Emergency);
}
