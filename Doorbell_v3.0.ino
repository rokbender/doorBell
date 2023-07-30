#include <MQTT.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>   // Universal Telegram Bot Library written by Brian Lough: https://github.com/witnessmenow/Universal-Arduino-Telegram-Bot
#include <ArduinoJson.h>

const char *ssid =  "";  // Імя вайфай точки доступа'
const char *pass =  ""; // Пароль від точки доступа

const char *mqtt_server = ""; // Імя сервера MQTT
const int mqtt_port = 1883; // Порт для підключення до серверу MQTT
const char *mqtt_user = ""; // Логін від сервера
const char *mqtt_pass = ""; // Пароль від сервера

#define BUFFER_SIZE 100


int attempsWifi=0;
int attempsMQTT=0;
int attemps=2;
bool State1=true;
bool alarm=true;
int numNewMessages;

WiFiClient wclient;      
PubSubClient client(wclient, mqtt_server, mqtt_port);


#define BOTtoken ""  // your Bot Token (Get from Botfather)

// Use @myidbot to find out the chat ID of an individual or a group
// Also note that you need to click "start" on a bot before it can
// message you
#define CHAT_ID "" 


#ifdef ESP8266
  X509List cert(TELEGRAM_CERTIFICATE_ROOT);
#endif

WiFiClientSecure client2;
UniversalTelegramBot bot(BOTtoken, client2);

// Checks for new messages every 1 second.
int botRequestDelay = 1500;

unsigned long lastTimeBotRan;


int detectRequestDelay = 2000;
unsigned long lastTimedetect=0;

float vout = 0.0;           //
float vin = 0.0;            //
float R1 = 330000.0;          // сопротивление R1 (10K)
float R2 = 99200.0;         // сопротивление R2 (1,5K)
int val=0;
#define analogvolt A0
bool flagVolt=false;

void handleNewMessages(int numNewMessages) {
  Serial.println("handleNewMessages");
  Serial.println(String(numNewMessages));

  for (int i=0; i<numNewMessages; i++) {
    // Chat id of the requester
    String chat_id = String(bot.messages[i].chat_id);
    if (chat_id != CHAT_ID){
      bot.sendMessage(chat_id, "Unauthorized user", "");
      continue;
    }
    
    // Print the received message
    String text = bot.messages[i].text;
    Serial.println(text);

    String from_name = bot.messages[i].from_name;

    if (text == "/startDoorBell") {
      String welcome = "Привіт, " + from_name + ".\n";
      welcome += "Чим я можу допомогти ? \n";
       welcome += "Це набір команд для керування: \n";
      welcome += "/DBell_Volt  Напруга батарейок  \n";
      bot.sendMessage(chat_id, welcome, "");
    }
   
    if (text == "/DBell_Volt") {
      bot.sendMessage(chat_id, "Надсилаю напругу батарейок", "");
      flagVolt=true;
      Volt();
         }
     
    
    
  }
}
// Функция получения данных от сервера

void callback( const MQTT ::Publish& pub)
{
  Serial.print(pub.topic());   // выводим в сериал порт название топика
  Serial.print(" => ");
  Serial.print(pub.payload_string()); // выводим в сериал порт значение полученных данных
  
  String payload = pub.payload_string();
  
   
}


void setup() {
  
  delay(10);
  Serial.begin(115200);
  configTime(0, 0, "pool.ntp.org");      // get UTC time via NTP
   client2.setTrustAnchors(&cert); // Add root certificate for api.telegram.org
   WiFi.mode(WIFI_STA);
   delay(500);
  // Connect to Wi-Fi
 
  client2.setInsecure(); 
}

void loop() {
  // подключаемся к wi-fi
  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("Connecting to ");
    Serial.print(ssid);
    Serial.println("...");
    WiFi.begin(ssid, pass);
    if (attempsWifi>attemps){
    Sleep1();
    }
    if (WiFi.waitForConnectResult() != WL_CONNECTED){
       delay(1000);
       attempsWifi+=1;
       return;
      }
      
    Serial.println("WiFi connected");
  }
  if (millis() > lastTimeBotRan + botRequestDelay)  {
    
   delay(100);
   int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    while(numNewMessages) {
      Serial.println("got response");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
      
      
    }
      
    lastTimeBotRan = millis();
   }
  if(alarm){
      bot.sendMessage(CHAT_ID, "Тук-тук. Дзвінок у вхідні двері!", "");
      Serial.println("here////");
      alarm=false;
    }
  // подключаемся к MQTT серверу
  if (WiFi.status() == WL_CONNECTED) {
    if (!client.connected()) {
      Serial.println("Connecting to MQTT server");
      if (client.connect(MQTT::Connect("Doorbell") // назва пристрою що підкл тобто ESp8266
                         .set_auth(mqtt_user, mqtt_pass))) {
        Serial.println("Connected to MQTT server");
        client.set_callback(callback);
      } else {
        Serial.println("Could not connect to MQTT server");
        attempsMQTT+=1;
        delay(1000);
        if (attempsMQTT>attemps){
           Sleep1();
          }   
      }
    }

    if (client.connected()){
      client.loop();
      SendData();
      delay(200);
      Sleep1();
  }
  
}
} // конец основного цикла


// Функция отправки показаний с термодатчика
void SendData(){
    
    client.publish("DoorBell/state",String(State1)); // отправляем в топик для термодатчика значение температуры
    Serial.println("SEND...");
   
}
void Sleep1(){
  Serial.println("SleepNow");
  ESP.deepSleep(0);
  }
  
void Volt(){
  delay(100);
  val = analogRead(analogvolt);
  Serial.println(val);
  if (val>0){
     vout = (val * 1.0) / 1024.0;
     vin = (vout / (R2 / (R1 + R2)))*0.984;
    }
   else{
    vin=0.0;
    } 
    
  if (vin<2.4){
     bot.sendMessage(CHAT_ID, "Батарейки розряджені", "");
    }
  if (flagVolt){
    String text="Напруга U=" + String(vin, 2);;
    bot.sendMessage(CHAT_ID, text, "");
    Serial.println("SENDVolt...");
    flagVolt=false;
  }
  
}
