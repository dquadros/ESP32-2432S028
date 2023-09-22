// Primeiro teste com a placa - Pisca o LED RGB

// Conexões dos LED RGB
// LOW acende, HIGH apaga
#define LED_R 4
#define LED_G 16
#define LED_B 17

void setup() {
  // Inicia as saídas
  pinMode(LED_R,OUTPUT);
  pinMode(LED_G,OUTPUT);
  pinMode(LED_B,OUTPUT);
  digitalWrite(LED_R, HIGH);
  digitalWrite(LED_G, HIGH);
  digitalWrite(LED_B, HIGH);
}

void loop() {
  digitalWrite(LED_R, LOW);
  delay(200);
  digitalWrite(LED_R, HIGH);
  delay(1000);
}
