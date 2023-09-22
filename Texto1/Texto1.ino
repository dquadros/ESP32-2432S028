// Segundo teste da placa - escrever algo no display
// Usa biblioteca TFT_eSPI, copiar o User_Setup.h para a pasta da biblioteca
// Atualização não está perfeita, tela pisca, sobra parte do número anterior


#include "SPI.h"
#include "TFT_eSPI.h"

#include "Free_Fonts.h"

// Use hardware SPI
TFT_eSPI tft = TFT_eSPI();

char hora[] = "00:00:00";

void setup(void) {
  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLUE);
  tft.setTextColor(TFT_WHITE, TFT_BLUE, true);
  tft.setFreeFont(FSB18);
  tft.setTextSize(2);
   tft.setTextDatum(BL_DATUM);
}

void loop() {
  // Função print escreve somente o foreground
  //tft.setCursor(10, 80);
  //tft.print(hora);

  // Função drawString escreve foreground e background,
  // mas dígitos tem tamanho diferente no fonte usado
  tft.drawString(hora, 10, 80, GFXFF);

  // Atualiza a cada 1 segundo
  delay(1000);

  // Incrementa "na unha" a hora
  if (hora[7] < '9') {
    hora[7]++;
    return;
  }
  hora[7] = '0';
  if (hora[6] < '5') {
    hora[6]++;
    return;
  }
  hora[6] = '0';
  if (hora[4] < '9') {
    hora[4]++;
    return;
  }
  hora[4] = '0';
  if (hora[3] < '5') {
    hora[3]++;
    return;
  }
  hora[3] = '0';
  if (hora[1] < '9') {
    hora[1]++;
    return;
  }
  hora[1] = '0';
  if (hora[0] < '9') {
    hora[0]++;
  } else {
    hora[0] = '0';
  }
}
