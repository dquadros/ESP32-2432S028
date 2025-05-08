// Definição de sensor genérico de temperatura e umidade
class SENSOR_TEMP {
  public:
    SENSOR_TEMP() { }
    virtual void init() = 0;
    virtual bool read() = 0;
    virtual double getTemperature() = 0;
    virtual double getHumidity() = 0;
};
