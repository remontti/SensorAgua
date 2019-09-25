# SensorAgua
Sensor de vazão e ultrassônico.

Agradecimento ao Leonardo Pereira e a Iulisloi Zacarias

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
