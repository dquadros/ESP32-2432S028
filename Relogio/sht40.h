
class SHT40: SENSOR_TEMP {
  private:
    const uint8_t addr = 0x44;
    uint8_t bufRx[6];
    double t_degC = 0.0;
    double rh_pRH = 0.0;

    uint8_t calc_crc(uint8_t *data) {
      uint8_t crc = 0xFF;
      for (int i = 0; i < 2; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
          crc = (crc & 0x80)? (crc << 1) ^ 0x31 : (crc << 1);
        }
      }
      return crc;
    }

    bool check_crc(uint8_t *data, int tam) {
      int i = 0;
      while (i < tam) {
        uint8_t crc = calc_crc(data+i);
        if (crc != data[i+2]) {
          return false;
        }
        i += 3;
      }
      return true;
    }

    bool exec(uint8_t cmd, int tamresp = 6, int espera = 10) {
      assert(tamresp <= sizeof(bufRx));
      for (int retry = 0; retry < 3; retry++) {
        Wire.beginTransmission(addr);
        Wire.write(cmd);
        if (Wire.endTransmission() != 0) {
          continue;
        }
        delay(espera);
        if (tamresp > 0) {
          if (Wire.requestFrom(addr, tamresp) != tamresp) {
            continue;
          }
          for (int i = 0; Wire.available() && i < tamresp; i++) {
            bufRx[i] = Wire.read();
          }
          if (check_crc(bufRx, tamresp)) {}
            return true;
        }
      }
      return false;
    }

  public:

    SHT40() {
    }

    uint32_t read_id() {
      if (exec(0x89)) {
        return ((uint32_t) bufRx[0] << 24) + ((uint32_t) bufRx[1] << 16) + ((uint32_t) bufRx[3] << 8) + bufRx[4];
      }
      return 0;
    }

    void init() {
      exec(0x94, 0);
    }

    bool read() {
      if (exec(0xFD)) {
        uint16_t t_ticks = (bufRx[0]<<8) + bufRx[1];
        uint16_t rh_ticks = (bufRx[3]<<8) + bufRx[4];
        t_degC = -45.0 + 175.0 * (t_ticks/65535.0);
        rh_pRH = min(max(-6.0 + 125.0 * (rh_ticks/65535.0), 0.0), 100.0);
        return true;
      }
      return false;
    }

    double getTemperature() {
      return t_degC;
    }

    double getHumidity() {
      return rh_pRH;
    }
};
