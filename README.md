# Sensor para mediar o nível da caixa D'Agua e O fluxo de água de um "cano"

Agradecimento ao <b>Leonardo Pereira</b> e a <a href="https://github.com/izacarias"><b>Iulisloi Zacarias</b></a>.<br><br>
  <b>Bibliotecas</b><br>
  MQTT - como instalar -> https://www.youtube.com/watch?v=GMMH6qT8_f4<br>
  ESP - como instalar -> https://www.youtube.com/watch?v=4d8joORYTIA&t=1s<br>
  Wi-Fi Manager  como instalar -> https://www.youtube.com/watch?v=wWO9n5DnuLA<br>
  OTA -> já está incluida no IDE do Arudino<br>
  
  NodeMcu Lua ESP8266 ESP-12F<br>
  https://www.banggood.com/Geekcreit-Doit-NodeMcu-Lua-ESP8266-ESP-12F-WIFI-Development-Board-p-985891.html
  
  Sensor de Fluxo:<br>
  https://www.banggood.com/G34-Inch-Liquid-Water-Flow-Sensor-Switch-Copper-Hall-Effect-Flowmeter-Meter-p-1188160.html
  
  Sensor Ultrassônico<br>
  https://www.banggood.com/Wholesale-Geekcreit-Ultrasonic-Module-HC-SR04-Distance-Measuring-Ranging-Transducer-Sensor-DC-5V-2-450cm-p-40313.html


  <b>Ligação Sensor de Fluxo:</b>
  <pre>
  Fio Vermelho --> Pino VIN 5V
  Fio Preto    --> Pino GND (Qualquer um)
  Fio Amarelo  --> Pino D7 (GPIO13)</pre>

  <b>Ligação Sensor HC-SR04:</b>
  <pre>Vcc  -->  Pino VIN 5V
  Trig -->  Pino D6 (GPIO12)
  Echo -->  Pino D5 (GPIO14)
  Gnd  -->  Pino Gnd (Qualquer um)</pre>

  OBS: Alimentar ambos sensores com 5V //VIN
<p><img width="250" src="https://raw.githubusercontent.com/remontti/SensorAgua/master/esquema.png">

<b>É importante que você altere as principais configurações no código!</b>
<pre>
#define MQTT_USERNAME "usuario"
#define MQTT_PASSWORD "senha"

const String HOSTNAME = "SensorAgua";       // Nome do Host e tambem do AP para fazer a configuração inicial.
const char *OTA_PASSWORD = "";              // Senha para conectar no AP (Atualizar o Firmware Over the Air)
const char *MQTT_SERVER = "192.168.87.5";   // IP ou DNS do Broker MQTT (IP DO HA)
</pre>
Ajuste a contagem dos pusolsos para saber a sua vazão corretamente.  <b>vazao = contaPulso / 6.6; </b> para sua realidade, no <a href="https://www.openhacks.com/page/productos/id/3080/title/Flow-Sensor-3-4-Inch-Brass-YF-B5#.XYuXuOZKicm">meu caso o sensor é 3/4</a> onde frequencia:  F=6.6*Q(Q=L/MIN) 


<b>CONECTANDO WIFI</b><br>
Ao enviar pela primeira vez seu ESP vai ficar em modo AP, com seu celular conecte no nome de sinal abra o 192.168.4.1 e defina o nome e senha de sua rede wifi.
<p><img width="100" src="https://raw.githubusercontent.com/remontti/SensorAgua/master/wifi1.jpg">
<img width="100" src="https://raw.githubusercontent.com/remontti/SensorAgua/master/wifi2.jpg">
<img width="100" src="https://raw.githubusercontent.com/remontti/SensorAgua/master/wifi3.jpg">
<img width="100" src="https://raw.githubusercontent.com/remontti/SensorAgua/master/wifi4.jpg"></p>

Para resetar as configurações do wifi localize a linha <b>wifiManager.resetSettings();</b><br>
  1 - descomentar a linha wifiManager.resetSettings()<br>
  2 - fazer Upload do código para o ESP<br>
  3 - voltar a comentar a linha e enviar novamente o código para o ESP<br>

Configuração no Home Assistant:
<pre>
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
    </pre>
<img src="https://raw.githubusercontent.com/remontti/SensorAgua/master/agua.png">
<b>CARDs:</b><br>

<pre>
  entities:
  - entity: sensor.vazao_de_agua
  - type: section
  - entity: sensor.consumo_de_agua
    name: Consumo de água no último minuto
  - type: section
  - entity: sensor.consumo_de_agua_dia
    name: Hoje
  - entity: sensor.consumo_de_agua_semana
    name: Semana
  - entity: sensor.consumo_de_agua_mes
    name: Mes
  - entity: sensor.consumo_de_agua_ano
    name: Ano
type: entities
title: Consumo de Água
show_header_toggle: false
</pre>
<pre>
cards:
  - entities:
      - label: Sensor Ultrasonico
        type: section
      - entity: sensor.distancia_d_agua
      - label: Descona a distância da tampa até a água
        type: section
      - entity: sensor.nivel_real
    type: entities
  - cards:
      - entity: sensor.caixa_dagua_litros
        max: 2000
        min: 0
        severity:
          green: 1500
          red: 500
          yellow: 1000
        type: gauge
      - entity: sensor.caixa_dagua
        max: 100
        min: 0
        severity:
          green: 90
          red: 20
          yellow: 50
        type: gauge
    type: horizontal-stack
  - cards:
      - entities:
          - color: '#18FFFF'
            entity: sensor.consumo_de_agua
            show_state: true
          - color: '#CDDC39'
            entity: sensor.vazao_de_agua
            show_state: true
        font_size: 90
        hours_to_show: 1
        line_width: 2
        name: Consumo de água na última hora
        points_per_hour: 30
        show:
          extrema: true
          fill: true
        type: 'custom:mini-graph-card'
    type: horizontal-stack
type: vertical-stack
</pre>
