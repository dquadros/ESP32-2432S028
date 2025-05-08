/*
  Aplicação exemplo para teste da placa ESP32-2432S028R
  Relogio com informação de temperatura e clima
  Daniel Quadros - abril/25

  Desenvolvido a partir de https://randomnerdtutorials.com/esp32-cyd-lvgl-weather-station/
  (C) Rui Santos & Sara Santos - Random Nerd Tutorials
  Permission is hereby granted, free of charge, to any person obtaining a copy of this 
  software and associated documentation files. The above copyright notice and this 
  permission notice shall be included in all copies or substantial portions of the 
  Software.  
*/

#include "secrets.h"

#include <TFT_eSPI.h>
#include <lvgl.h>

#include "weather_images.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFiUdp.h>
#include <NTPClient.h>

// Selecione o sensor de temperatura e umidade: AHT10, SHT40 ou NENHUM
// Se for selecionado nenhum usa os valores obtidos do open-meteo
#define SENSOR_NENHUM 0
#define SENSOR_AHT10  1
#define SENSOR_SHT40  2
#define SENSOR SENSOR_SHT40

#if SENSOR != SENSOR_NENHUM
  #include <Wire.h>
  #define I2C_SDA 27
  #define I2C_SCL 22
#endif

#if SENSOR == SENSOR_AHT10
  #include "sensor_temp.h"
  #include "aht10.h"
#endif

#if SENSOR == SENSOR_SHT40
  #include "sensor_temp.h"
  #include "sht40.h"
#endif

// Acesso ao servidor NTP
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, -3*60*60);

// Objeto para acesso ao sensor de temperatura
#if SENSOR == SENSOR_AHT10
  AHT10 sensor = AHT10();
#elif SENSOR == SENSOR_SHT40
  SHT40 sensor = SHT40();
#endif

// url para consulta da previsão de tempo
String url;

// Dados atuais
char currentDate[36] = "DD-MM-AAAA";
String last_weather_update;
String temperature;
String humidity;
int is_day;
int weather_code = 0;
int last_weather_code = -1;
String weather_description;
String minmax_temp;

const char degree_symbol[] = "\u00B0C";

#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 320

#define DRAW_BUF_SIZE (SCREEN_WIDTH * SCREEN_HEIGHT / 10 * (LV_COLOR_DEPTH / 8))
uint32_t draw_buf[DRAW_BUF_SIZE / 4];

// Log para mensagens da LVGL
void log_print(lv_log_level_t level, const char * buf) {
  LV_UNUSED(level);
  Serial.println(buf);
  Serial.flush();
}

// Objetos na tela
static lv_obj_t * weather_image;
static lv_obj_t * text_label_date;
static lv_obj_t * text_label_time;
static lv_obj_t * text_label_temperature;
static lv_obj_t * text_label_minmax_temp;
static lv_obj_t * text_label_humidity;
static lv_obj_t * text_label_weather_description;
static lv_obj_t * text_label_time_location;

// Retorna hora atual no formato HH:MM
// (Adaptado de getFormattedTime() do NTPClient)
static String getCurrentTime() {
  unsigned long rawTime = timeClient.getEpochTime();
  unsigned long hours = (rawTime % 86400L) / 3600;
  String hoursStr = hours < 10 ? "0" + String(hours) : String(hours);
  unsigned long minutes = (rawTime % 3600) / 60;
  String minuteStr = minutes < 10 ? "0" + String(minutes) : String(minutes);
  return hoursStr + ":" + minuteStr;
}

// Retorna data atual no formato DD-MM-AAAA
static char *getCurrentDate() {
  time_t rawTime = (time_t) timeClient.getEpochTime();
  struct tm *ptm = gmtime(&rawTime);
  sprintf(currentDate, "%02d-%02d-%04d", ptm->tm_mday, ptm->tm_mon+1, ptm->tm_year+1900);
  return currentDate;
}

// Callback chamado periodicamente para atualizar data / hora / temperatura / umidade
static void timer2_cb(lv_timer_t * timer){
  LV_UNUSED(timer);
  #if SENSOR != SENSOR_NENHUM
  sensor.read();
  temperature = String(sensor.getTemperature(), 1);
  humidity = String(sensor.getHumidity(), 1);
  #endif
  lv_label_set_text(text_label_date, getCurrentDate());
  lv_label_set_text(text_label_time, getCurrentTime().c_str());
  lv_label_set_text(text_label_temperature, String("      " + temperature + degree_symbol).c_str());
  lv_label_set_text(text_label_humidity, String("   " + humidity + "%").c_str());
}

// Callback chamado periodicamente para atualizar toda a tela
static void timer_cb(lv_timer_t * timer){
  LV_UNUSED(timer);
  #if SENSOR != SENSOR_NENHUM
  sensor.read();
  temperature = String(sensor.getTemperature(), 1);
  humidity = String(sensor.getHumidity(), 1);
  #endif
  get_weather_data();
  update_weather(weather_code);
  lv_label_set_text(text_label_minmax_temp, minmax_temp.c_str());
  lv_label_set_text(text_label_date, getCurrentDate());
  lv_label_set_text(text_label_time, getCurrentTime().c_str());
  lv_label_set_text(text_label_temperature, String("      " + temperature + degree_symbol).c_str());
  lv_label_set_text(text_label_humidity, String("   " + humidity + "%").c_str());
  lv_label_set_text(text_label_weather_description, weather_description.c_str());
  lv_label_set_text(text_label_time_location, String("Atualizado as: " + last_weather_update + "  |  " + location).c_str());
}

// Cria os campos na tela
void lv_create_main_gui(void) {
  LV_IMAGE_DECLARE(image_weather_sun);
  LV_IMAGE_DECLARE(image_weather_cloud);
  LV_IMAGE_DECLARE(image_weather_rain);
  LV_IMAGE_DECLARE(image_weather_thunder);
  LV_IMAGE_DECLARE(image_weather_snow);
  LV_IMAGE_DECLARE(image_weather_night);
  LV_IMAGE_DECLARE(image_weather_temperature);
  LV_IMAGE_DECLARE(image_weather_humidity);

  // Obtem previsão do tempo para inicar os campos
  get_weather_data();

  // Lê temperatura e umidade
  #if SENSOR != SENSOR_NENHUM
  sensor.read();
  temperature = String(sensor.getTemperature(), 1);
  humidity = String(sensor.getHumidity(), 1);
  #endif

  // Tempo atual
  weather_image = lv_image_create(lv_screen_active());
  lv_obj_align(weather_image, LV_ALIGN_CENTER, -80, -20);

  update_weather(weather_code);

  text_label_weather_description = lv_label_create(lv_screen_active());
  lv_label_set_text(text_label_weather_description, weather_description.c_str());
  lv_obj_align(text_label_weather_description, LV_ALIGN_BOTTOM_MID, 0, -60);
  lv_obj_set_style_text_font((lv_obj_t*) text_label_weather_description, &lv_font_montserrat_18, 0);

  text_label_minmax_temp = lv_label_create(lv_screen_active());
  lv_label_set_text(text_label_minmax_temp, minmax_temp.c_str());
  lv_obj_align(text_label_minmax_temp, LV_ALIGN_BOTTOM_MID, 0, -40);
  lv_obj_set_style_text_font((lv_obj_t*) text_label_minmax_temp, &lv_font_montserrat_14, 0);


  // Data e hora atuais
  text_label_date = lv_label_create(lv_screen_active());
  lv_label_set_text(text_label_date, getCurrentDate());
  lv_obj_align(text_label_date, LV_ALIGN_CENTER, 70, -100);
  lv_obj_set_style_text_font((lv_obj_t*) text_label_date, &lv_font_montserrat_26, 0);
  lv_obj_set_style_text_color((lv_obj_t*) text_label_date, lv_palette_main(LV_PALETTE_TEAL), 0);

  text_label_time = lv_label_create(lv_screen_active());
  lv_label_set_text(text_label_time, getCurrentTime().c_str());
  lv_obj_align(text_label_time, LV_ALIGN_CENTER, 70, -70);
  lv_obj_set_style_text_font((lv_obj_t*) text_label_time, &lv_font_montserrat_26, 0);
  lv_obj_set_style_text_color((lv_obj_t*) text_label_time, lv_palette_main(LV_PALETTE_TEAL), 0);

  // Temperatura e umidade
  lv_obj_t * weather_image_temperature = lv_image_create(lv_screen_active());
  lv_image_set_src(weather_image_temperature, &image_weather_temperature);
  lv_obj_align(weather_image_temperature, LV_ALIGN_CENTER, 30, -25);
  text_label_temperature = lv_label_create(lv_screen_active());
  lv_label_set_text(text_label_temperature, String("      " + temperature + degree_symbol).c_str());
  lv_obj_align(text_label_temperature, LV_ALIGN_CENTER, 70, -25);
  lv_obj_set_style_text_font((lv_obj_t*) text_label_temperature, &lv_font_montserrat_22, 0);

  lv_obj_t * weather_image_humidity = lv_image_create(lv_screen_active());
  lv_image_set_src(weather_image_humidity, &image_weather_humidity);
  lv_obj_align(weather_image_humidity, LV_ALIGN_CENTER, 30, 10);
  text_label_humidity = lv_label_create(lv_screen_active());
  lv_label_set_text(text_label_humidity, String("   " + humidity + "%").c_str());
  lv_obj_align(text_label_humidity, LV_ALIGN_CENTER, 70, 10);
  lv_obj_set_style_text_font((lv_obj_t*) text_label_humidity, &lv_font_montserrat_22, 0);

  // Campo com hora da taulização e o local
  text_label_time_location = lv_label_create(lv_screen_active());
  lv_label_set_text(text_label_time_location, String("Last Update: " + last_weather_update + "  |  " + location).c_str());
  lv_obj_align(text_label_time_location, LV_ALIGN_BOTTOM_MID, 0, -10);
  lv_obj_set_style_text_font((lv_obj_t*) text_label_time_location, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color((lv_obj_t*) text_label_time_location, lv_palette_main(LV_PALETTE_GREY), 0);

  // Cria os timers para atualização da tela
  lv_timer_t * timer = lv_timer_create(timer_cb, 600000, NULL); // 10 minutos
  lv_timer_ready(timer);

  lv_timer_t * timer2 = lv_timer_create(timer2_cb, 30000, NULL);  // 0,5 minuto
  lv_timer_ready(timer2);
}

/*
  Atualiza a imagem e prepara a descrição do clima atual

  WMO Weather interpretation codes (WW)- Code	Description
  0	Clear sky
  1, 2, 3	Mainly clear, partly cloudy, and overcast
  45, 48	Fog and depositing rime fog
  51, 53, 55	Drizzle: Light, moderate, and dense intensity
  56, 57	Freezing Drizzle: Light and dense intensity
  61, 63, 65	Rain: Slight, moderate and heavy intensity
  66, 67	Freezing Rain: Light and heavy intensity
  71, 73, 75	Snow fall: Slight, moderate, and heavy intensity
  77	Snow grains
  80, 81, 82	Rain showers: Slight, moderate, and violent
  85, 86	Snow showers slight and heavy
  95 *	Thunderstorm: Slight or moderate
  96, 99 *	Thunderstorm with slight and heavy hail
*/
void update_weather(int code) {
  switch (code) {
    case 0:
      if(is_day==1) { lv_image_set_src(weather_image, &image_weather_sun); }
      else { lv_image_set_src(weather_image, &image_weather_night); }
      weather_description = "CEU CLARO";
      break;
    case 1: 
      if(is_day==1) { lv_image_set_src(weather_image, &image_weather_sun); }
      else { lv_image_set_src(weather_image, &image_weather_night); }
      weather_description = "POUCAS NUVENS";
      break;
    case 2: 
      lv_image_set_src(weather_image, &image_weather_cloud);
      weather_description = "PARCIALMENTE NUBLADO";
      break;
    case 3:
      lv_image_set_src(weather_image, &image_weather_cloud);
      weather_description = "NUBLADO";
      break;
    case 45:
      lv_image_set_src(weather_image, &image_weather_cloud);
      weather_description = "NEVOA";
      break;
    case 48:
      lv_image_set_src(weather_image, &image_weather_cloud);
      weather_description = "GEADA";
      break;
    case 51:
      lv_image_set_src(weather_image, &image_weather_rain);
      weather_description = "GAROA LEVE";
      break;
    case 53:
      lv_image_set_src(weather_image, &image_weather_rain);
      weather_description = "GAROA MODERADA";
      break;
    case 55:
      lv_image_set_src(weather_image, &image_weather_rain); 
      weather_description = "GAROA FORTE";
      break;
    case 56:
      lv_image_set_src(weather_image, &image_weather_rain);
      weather_description = "CHUVA GELADA LEVE";
      break;
    case 57:
      lv_image_set_src(weather_image, &image_weather_rain);
      weather_description = "CHUVA GELADA FORTE";
      break;
    case 61:
      lv_image_set_src(weather_image, &image_weather_rain);
      weather_description = "CHUVA LEVE";
      break;
    case 63:
      lv_image_set_src(weather_image, &image_weather_rain);
      weather_description = "CHUVA MODERADA";
      break;
    case 65:
      lv_image_set_src(weather_image, &image_weather_rain);
      weather_description = "CHUVA FORTE";
      break;
    case 66:
      lv_image_set_src(weather_image, &image_weather_rain);
      weather_description = "CHUVA GELADA LEVE";
      break;
    case 67:
      lv_image_set_src(weather_image, &image_weather_rain);
      weather_description = "CHUVA GELADA FORTE";
      break;
    case 71:
      lv_image_set_src(weather_image, &image_weather_snow);
      weather_description = "NEVE LEVE";
      break;
    case 73:
      lv_image_set_src(weather_image, &image_weather_snow);
      weather_description = "NEVE MODERADA";
      break;
    case 75:
      lv_image_set_src(weather_image, &image_weather_snow);
      weather_description = "NEVE PESADA";
      break;
    case 77:
      lv_image_set_src(weather_image, &image_weather_snow);
      weather_description = "GRÃOS DE NEVE";
      break;
    case 80:
      lv_image_set_src(weather_image, &image_weather_rain);
      weather_description = "CHUVA LEVE";
      break;
    case 81:
      lv_image_set_src(weather_image, &image_weather_rain);
      weather_description = "CHUVA MODERADA";
      break;
    case 82:
      lv_image_set_src(weather_image, &image_weather_rain);
      weather_description = "CHUVA VIOLENTA";
      break;
    case 85:
      lv_image_set_src(weather_image, &image_weather_snow);
      weather_description = "CHUVA C/ NEVE LEVE";
      break;
    case 86:
      lv_image_set_src(weather_image, &image_weather_snow);
      weather_description = "CHUVA C/ NEVE FORTE";
      break;
    case 95:
      lv_image_set_src(weather_image, &image_weather_thunder);
      weather_description = "TEMPESTADE";
      break;
    case 96:
      lv_image_set_src(weather_image, &image_weather_thunder);
      weather_description = "TEMPESTADE GRANIZO LEVE";
      break;
    case 99:
      lv_image_set_src(weather_image, &image_weather_thunder);
      weather_description = "TEMPESTADE COM GRANIZO";
      break;
    default: 
      weather_description = "DESCONHECIDO";
      break;
  }
}

// Obtem a informação de tempo
/* 
  Resposta do  open-meteo.com
  {
    "latitude":-23.5,"longitude":-46.5,
    "generationtime_ms":0.03921985626220703,"utc_offset_seconds":-10800,
    "timezone":"America/Sao_Paulo","timezone_abbreviation":"GMT-3",
    "elevation":737.0,
    "current_units":
    {
      "time":"iso8601","interval":"seconds","is_day":"","weather_code":"wmo code",
      "temperature_2m":"°C","relative_humidity_2m":"%"
    },
    "current":
    {
      "time":"2025-04-24T10:45",
      "interval":900,
      "is_day":1,
      "weather_code":3
      "temperature_2m":25.3,
      "relative_humidity_2m":17.2
    },
    "daily_units":
    {
      "time":"iso8601","temperature_2m_max":"°C","temperature_2m_min":"°C"
    },
    "daily":
    {
      "time":["2025-04-24"],
      "temperature_2m_max":[25.3],
      "temperature_2m_min":[17.2]
    }
  }
*/
void get_weather_data() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(url);
    int httpCode = http.GET();

    if (httpCode > 0) {
      // Verifica a resposta
      if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        // Extrai dados do JSON
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, payload);
        if (!error) {
          #if SENSOR == SENSOR_NENHUM
          temperature = String(doc["current"]["temperature_2m"]);
          humidity = String(doc["current"]["relative_humidity_2m"]);
          #endif
          is_day = String(doc["current"]["is_day"]).toInt();
          weather_code = String(doc["current"]["weather_code"]).toInt();
          String min_temp = String(doc["daily"]["temperature_2m_min"][0]);
          String max_temp = String(doc["daily"]["temperature_2m_max"][0]);
          minmax_temp = String("Previsao: Min ")+min_temp+String(" Max ")+max_temp;

          // Extrai a hora da atualização
          const char* datetime = doc["current"]["time"];
          String datetime_str = String(datetime);
          int splitIndex = datetime_str.indexOf('T');
          last_weather_update = datetime_str.substring(splitIndex + 1, splitIndex + 9);
        } else {
          Serial.print("deserializeJson() falhou: ");
          Serial.println(error.c_str());
        }
      }
      else {
        Serial.println("Falhou");
      }
    } else {
      Serial.printf("GET falhou, erro: %s\n", http.errorToString(httpCode).c_str());
    }
    http.end();
  } else {
    Serial.println("Wi-Fi nao conectado");
  }
}

// Iniciação
void setup() {
  // Informa versão da LGVL
  String LVGL_Arduino = String("Biblioteca LGVL v") + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch();
  Serial.begin(115200);
  Serial.println(LVGL_Arduino);

  // Conecta à rede WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connectando");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.print("\nConectado a rede. IP: ");
  Serial.println(WiFi.localIP());

  // Inicia o relogio
  timeClient.begin();
  while (!timeClient.update()) {
    delay(500);
  }
  Serial.println("Obteve hora");

  // Inicia sensor de temperatura
  #if SENSOR != SENSOR_NENHUM
  Wire.setPins(I2C_SDA, I2C_SCL);
  Wire.begin();
  sensor.init();
  #endif

  // Inicia LVGL
  lv_init();
  lv_log_register_print_cb(log_print);

  // Cria o objeto que representa o display
  lv_display_t * disp;
  // Inicia o display através da biblioteca TFT_eSPI
  disp = lv_tft_espi_create(SCREEN_WIDTH, SCREEN_HEIGHT, draw_buf, sizeof(draw_buf));
  lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_270);

  // Monta a URL de consulta do clima
  #if SENSOR == SENSOR_NENHUM
  url = String("http://api.open-meteo.com/v1/forecast?latitude=" + latitude + "&longitude=" + longitude + 
              "&daily=temperature_2m_max,temperature_2m_min&current=temperature_2m,relative_humidity_2m,is_day,weather_code" + 
              "&timezone=" + timezone + "&forecast_days=1");
  #else
  url = String("http://api.open-meteo.com/v1/forecast?latitude=" + latitude + "&longitude=" + longitude + 
               "&daily=temperature_2m_max,temperature_2m_min&current=is_day,weather_code" + 
               "&timezone=" + timezone + "&forecast_days=1");
  #endif

  // Cria os objetos da GUI
  lv_create_main_gui();
}

// Laço principal
void loop() {
  timeClient.update();
  lv_task_handler();  // let the GUI do its work
  lv_tick_inc(5);     // tell LVGL how much time has passed
  delay(5);           // let this time pass
}
