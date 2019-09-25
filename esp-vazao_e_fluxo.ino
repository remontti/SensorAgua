/*
  Bibliotecas:

  MQTT - como instalar -> https://www.youtube.com/watch?v=GMMH6qT8_f4
  ESP - como instalar -> https://www.youtube.com/watch?v=4d8joORYTIA&t=1s
  Wi-Fi Manager  como instalar -> https://www.youtube.com/watch?v=wWO9n5DnuLA
  OTA -> já está incluida no IDE do Arudino

  OBS:
  - Alimentar ambos sensores com 5V //VIN
  
  NodeMcu Lua ESP8266 ESP-12F
  https://www.banggood.com/Geekcreit-Doit-NodeMcu-Lua-ESP8266-ESP-12F-WIFI-Development-Board-p-985891.html
  Sensor de Fluxo:
  https://www.banggood.com/G34-Inch-Liquid-Water-Flow-Sensor-Switch-Copper-Hall-Effect-Flowmeter-Meter-p-1188160.html
  Sensor Ultrassônico
  https://www.banggood.com/Wholesale-Geekcreit-Ultrasonic-Module-HC-SR04-Distance-Measuring-Ranging-Transducer-Sensor-DC-5V-2-450cm-p-40313.html


  Ligação Sensor de Fluxo:
  Fio Vermelho --> Pino VIN 5V
  Fio Preto    --> Pino GND (Qualquer um)
  Fio Amarelo  --> Pino D3 (GPIO0)

  Ligação Sensor HC-SR04:
  Vcc  -->  Pino VIN 5V
  Trig -->  Pino D6 (GPIO12)
  Echo -->  Pino D5 (GPIO14)
  Gnd  -->  Pino Gnd (Qualquer um)

  Ao enviar pela primeira vez seu ESP vai ficar em modo AP, com seu celular conecte no nome de sinal abra o 192.168.4.1 e defina o nome e senha de sua rede wifi

Configuração no Home Assistant
sensor:
  - platform: mqtt
    state_topic: 'SensorAgua/contagem'
    name: 'Vazão de Água'
    icon: mdi:water
    unit_of_measurement: 'litros/s'
    value_template: '{{ value_json.vazao }}'
  - platform: mqtt
    state_topic: 'SensorAgua/contagem'
    name: 'Consumo de Água'
    icon: mdi:water-percent
    unit_of_measurement: 'litros'
    value_template: '{{ value_json.consumo_agua }}'
  - platform: mqtt
    state_topic: 'SensorAgua/contagem'
    name: "Distancia D'agua"
    icon: mdi:ruler
    unit_of_measurement: 'cm'
    value_template: '{{ value_json.distancia }}'

utility_meter:
  consumo_de_agua_dia:
    source: sensor.consumo_de_agua
    cycle: daily
  consumo_de_agua_semana:
    source: sensor.consumo_de_agua
    cycle: weekly
  consumo_de_agua_mes:
    source: sensor.consumo_de_agua
    cycle: monthly
  consumo_de_agua_ano:
    source: sensor.consumo_de_agua
    cycle: yearly

*/

/* Including files */
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>


/* Configuração do acesso ao Broker MQTT */
#define MQTT_AUTH true
#define MQTT_USERNAME "USUARIO"   //USUARIO 
#define MQTT_PASSWORD "SENHA"     //SENHA 

/* Constantes */
const String HOSTNAME = "SensorAgua";                 // Nome do Host. Este nome tambem será o nome do AP para fazer a configuração inicial.
const char *OTA_PASSWORD = "";                        // Senha para conectar no AP (Atualizar o Firmware Over the Air)
const char *MQTT_LOG = "system/log";                  // Topico onde o Device Publica informações relacionadas com o sistema
const char *MQTT_SYSTEM_CONTROL_TOPIC = "system/set"; // Topico onde o Device subscreve para aceitar instruções de sistema
const char *MQTT_TEST_TOPIC = "superTopic";           // Topico de exemplo onde o Device subscreve (por exemplo controlar uma lâmpada)
const char *MQTT_SERVER = "192.168.87.5";             // IP ou DNS do Broker MQTT - IP DO HA

/* Configurando GPIO */
const int ULTRASSOM_TRIGGER = 12; // D6  --> GPIO12 (HC-SR04)
const int ULTRASSOM_ECHO    = 14; // D5  --> GPIO14 (HC-SR04)
const int pulsoPin = 2;           // D3  --> GPIO0  Sensor de Fluxo d'agua

/* Cliente MQTT */
WiFiClient wclient;
PubSubClient client(MQTT_SERVER, 1883, wclient);

/* FLAGS de Controle
  /*     O Serviço OTA é muito pesado para estar sempre ativo por isso é ligado via MQTT e
       fica disponivel até ser desligado ou até o device ser reiniciado */
bool OTA = false;
bool OTABegin = false;

/* Variareis da Agua */
float vazao;      // Variável para armazenar o valor em L/min
float media = 0;  // Variável para tirar a média a cada 1 minuto
int contaPulso;   // Variável para a quantidade de pulsos
int i = 0;        // Variável para contagem

void setup() {
  Serial.begin(9600);
  WiFiManager wifiManager;

  /* Resetando as configurações de WifiAnterior
      1 - descomentar a linha wifiManager.resetSettings()
      2 - fazer Upload do código para o ESP
      3 - voltar a comentar a linha e enviar novamente o código para o ESP
  */
  // wifiManager.resetSettings();

  /*define o tempo limite até o portal de configuração ficar novamente inátivo, útil para quando alteramos a password do AP*/
  wifiManager.setTimeout(180);
  wifiManager.autoConnect(HOSTNAME.c_str());
  client.setCallback(callback); //Registo da função que vai responder ás mensagens vindos do MQTT

  //Setup Ultrasonico
  pinMode(ULTRASSOM_TRIGGER, OUTPUT); // Define o trigPin como uma saída
  pinMode(ULTRASSOM_ECHO, INPUT);  // Define o echoPin como uma entrada

  //Setup Fluxo d'agua
  pinMode(pulsoPin, INPUT);
  attachInterrupt(0, incpulso, RISING); //Configura o pino 2(Interrupção 0) para trabalhar como interrupção
  Serial.println("\n\nInicio\n\n");     //Imprime Inicio na serial

  // Piscar LED do ESP
  pinMode(LED_BUILTIN, OUTPUT);
}

/* Chamada de recepção de mensagem */
void callback(char *topic, byte *payload, unsigned int length)
{
  String payloadStr = "";
  for (int i = 0; i < length; i++)
  {
    payloadStr += (char)payload[i];
  }
  String topicStr = String(topic);
  if (topicStr.equals(MQTT_SYSTEM_CONTROL_TOPIC))
  {
    if (payloadStr.equals("OTA_ON_" + String(HOSTNAME)))
    {
      Serial.println("OTA ON");
      OTA = true;
      OTABegin = true;
    }
    else if (payloadStr.equals("OTA_OFF_" + String(HOSTNAME)))
    {
      Serial.println("OTA OFF");
      OTA = false;
      OTABegin = false;
    }
    else if (payloadStr.equals("REBOOT_" + String(HOSTNAME)))
    {
      Serial.println("REBOOT");
      ESP.restart();
    }
  }
  else if (topicStr.equals(MQTT_TEST_TOPIC))
  {
    //TOPICO DE TESTE
    Serial.println(payloadStr);
  }
}

bool checkMqttConnection()
{
  if (!client.connected())
  {
    if (MQTT_AUTH ? client.connect(HOSTNAME.c_str(), MQTT_USERNAME, MQTT_PASSWORD) : client.connect(HOSTNAME.c_str()))
    {
      Serial.println("CONNECTED TO MQTT BROKER " + String(MQTT_SERVER));
      client.publish(MQTT_LOG, String("CONNECTED_" + HOSTNAME).c_str());
      //SUBSCRIÇÃO DE TOPICOS
      client.subscribe(MQTT_SYSTEM_CONTROL_TOPIC);
      client.subscribe(MQTT_TEST_TOPIC);
    }
  }
  return client.connected();
}

void incpulso()
{
  contaPulso++; //Incrementa a variável de pulsos
}

void setupOTA()
{
  if (WiFi.status() == WL_CONNECTED && checkMqttConnection())
  {
    client.publish(MQTT_LOG, "OTA SETUP");
    ArduinoOTA.setHostname(HOSTNAME.c_str());
    ArduinoOTA.setPassword(OTA_PASSWORD);

    ArduinoOTA.onStart([]() {
      client.publish(MQTT_LOG, "START");
    });
    ArduinoOTA.onEnd([]() {
      client.publish(MQTT_LOG, "END");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
      String p = "Progress: " + String((progress / (total / 100)));
      client.publish(MQTT_LOG, p.c_str());
    });
    ArduinoOTA.onError([](ota_error_t error) {
      if (error == OTA_AUTH_ERROR)
        client.publish(MQTT_LOG, "Auth Failed");
      else if (error == OTA_BEGIN_ERROR)
        client.publish(MQTT_LOG, "Begin Failed");
      else if (error == OTA_CONNECT_ERROR)
        client.publish(MQTT_LOG, "Connect Failed");
      else if (error == OTA_RECEIVE_ERROR)
        client.publish(MQTT_LOG, "Receive Failed");
      else if (error == OTA_END_ERROR)
        client.publish(MQTT_LOG, "End Failed");
    });
    ArduinoOTA.begin();
  }
}

float le_distancia(int triger_pin, int echo_pin) {
  long duration;
  int distance;
  // Clears the trigPin
  digitalWrite(triger_pin, LOW);
  delayMicroseconds(2);
  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(triger_pin, HIGH);
  delayMicroseconds(10);
  digitalWrite(triger_pin, LOW);
  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration = pulseIn(echo_pin, HIGH);
  // Calculating the distance
  distance = duration * 0.034 / 2;
  return distance;
}

void loop() {

  struct dados_t
  {
    int distancia;
    float vazao;
    float media;
  };
  typedef struct dados_t DADOS_T;

  /* Armazena os dados para depois mandar por MQTT */
  DADOS_T dados;

  if (WiFi.status() == WL_CONNECTED)
  {
    if (checkMqttConnection())
    {
      client.loop();
      if (OTA)
      {
        if (OTABegin)
        {
          setupOTA();
          OTABegin = false;
        }
        ArduinoOTA.handle();
      }

      /* Lê distancia */
      dados.distancia = le_distancia(ULTRASSOM_TRIGGER, ULTRASSOM_ECHO);
      // Prints the distance on the Serial Monitor
      Serial.print("Distance: ");
      Serial.println(dados.distancia);

      // **************************************
      //    Inicio Agua
      // **************************************
      contaPulso = 0; //Zera a variável para contar os giros por segundos
      sei();          //Habilita interrupção
      digitalWrite(LED_BUILTIN, HIGH);   // Liga o LED
      delay(900);     //Aguarda 1 segundo
      digitalWrite(LED_BUILTIN, LOW);    // Desliga o LED
      delay(100);     //Aguarda 1 segundo
      cli();          //Desabilita interrupção

      vazao = contaPulso / 6.6; //Converte para L/min
      media = media + vazao;    //Soma a vazão para o calculo da media
      i++;

      dados.vazao = vazao;
      Serial.print(vazao);       //Imprime na serial o valor da vazão
      Serial.print(" L/min - "); //Imprime L/min
      Serial.print(i);           //Imprime a contagem i (segundos)
      Serial.println("s");       //Imprime s indicando que está em segundos

      if (i == 60)
      {
        media = media / 60; //Tira a media dividindo por 60
        dados.media = media;
        Serial.print("\nMedia por minuto = "); //Imprime a frase Media por minuto =
        Serial.print(media);                   //Imprime o valor da media
        Serial.println(" L/min - ");           //Imprime L/min
        media = 0;                             //Zera a variável media para uma nova contagem
        i = 0;                                 //Zera a variável i para uma nova contagem
        Serial.println("\n\nInicio\n\n");      //Imprime Inicio indicando que a contagem iniciou
      }

      // **************************************
      //    Fim agua
      // **************************************

      // **************************************
      //    Publica no Pub / Sub
      // **************************************
      String payload = "{";
      payload += "\"distancia\":" + String(dados.distancia) + ",";
      payload += "\"vazao\":" + String(dados.vazao) + ",";
      payload += "\"consumo_agua\":" + String(dados.media);
      payload += "}";
    
      char attributes[100];
      payload.toCharArray(attributes, 100);
      client.publish("SensorAgua/contagem", attributes); // Defina um nome para seu "state topic"
      Serial.println(attributes);

    }
  }
}
