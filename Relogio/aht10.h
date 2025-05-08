class AHT10: SENSOR_TEMP {
  private:
    const uint8_t addr = 0x38;
    const uint8_t cmdInit[3] = { 0xE1, 0x08, 0x00 };
    const uint8_t cmdConv[3] = { 0xAC, 0x33, 0x00 };
    double t_degC = 0.0;
    double rh_pRH = 0.0;

    uint8_t getStatus() {
      Wire.requestFrom(addr, 1);
      return Wire.read();
    }

  public:

    AHT10() {
    }

    void init() {
      uint8_t status = getStatus();
      if ((status& 0x08) == 0) {
        Wire.beginTransmission(addr);  
        Wire.write(cmdInit, sizeof(cmdInit));  
        Wire.endTransmission();
        delay(10);
      }
    }

    bool read() {
      Wire.beginTransmission(addr);  
      Wire.write(cmdConv, sizeof(cmdConv));  
      Wire.endTransmission();

      delay(80);

      uint16_t r[6];
      Wire.requestFrom(addr, 6);
      for (int i = 0; i < 6; i++) {
        r[i] = Wire.read();
      }
      rh_pRH = (r[1] << 12) + (r[2] << 4) + (r[3] >> 4);
      rh_pRH = (rh_pRH / 0x100000) * 100.0;
      t_degC = ((r[3] & 0x0F) << 16) + (r[4] << 8) + r[5];
      t_degC = (t_degC / 0x100000) * 200.0 - 50.0;
      return true;
    }

    double getTemperature() {
      return t_degC;
    }

    double getHumidity() {
      return rh_pRH;
    }
};
