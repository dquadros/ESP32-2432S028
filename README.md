# Testes e Projetos com a placa ESP32-2432S028

A placa ESP32-2432S028 possui um microcontrolador ESP32 e um display TFT com resolução 240x320.

Os programas aqui foram desenvolvidos na IDE Arduino.

Vídeos no meu canal no YouTube:

* https://www.youtube.com/watch?v=RQJemuFVu-M
* https://www.youtube.com/watch?v=tlyVUodfkIE
* https://www.youtube.com/watch?v=IzO_j2Pwcts

## User_Setup.h

Arquivo de configuração para a biblioteca TFT_eSPI. Precisa ser colocado no diretório onde a biblioteca está instalada.

## lv_conf.h

Arquivo de configuração da biblioteca LVGL. Precisa ser colocado no diretório de bibliotecas do Arduino.

Fonte: https://randomnerdtutorials.com/lvgl-cheap-yellow-display-esp32-2432s028r/

## Blink

Primeiro teste, para confirmar que conseguimos compilar e carregar um programa na placa. Pisca o LED RGB.

## Texto1

Primeira experiência de apresentar algo no display. Usa a biblioteca TFT_eSPI.

## Relogio

Um relógio que apresenta informações de temperatura e clima. Usa a biblioteca LVGL. 

