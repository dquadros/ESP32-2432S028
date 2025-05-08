# Relogio com informação de temperatura e clima

Aplicação exemplo para teste da placa ESP32-2432S028R.

## Montagem

Utilize o conector CN1 para ligar um sensor SHT40 ou AHT10. Os sinais de I2C estão nos pinos:

* IO22 - SCL
* IO27 - SDA

## Preparativos

Crie um arquivo secrets.h contendo:

```
// Acesso a rede
const char* ssid = "nome da rede";
const char* password = "senha da rede";

// Localização
String latitude = "-23.4601";
String longitude = "-46.6977";
String location = "Sao Paulo";

// Fuso horário
String timezone = "America/Sao_Paulo";
```

No início do programa principal, selecione em SENSOR o sensor que será usado (SHT40, AHT10 ou NENHUM).

**No menu Tools, selecione "Partition Scheme: Huge App"**

## Referências

Esta aplicação é um "mashup" de ideias e códigos de vários exemplos:

* https://www.makerhero.com/blog/relogio-com-raspberry-pi-pico-w/
* https://randomnerdtutorials.com/esp32-cyd-lvgl-weather-station/
* https://dqsoft.blogspot.com/2025/04/sensor-de-temperatura-e-umidade.html
