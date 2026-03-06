#pragma once
#include <Arduino.h>

// =============================
// Estrutura de leitura de sensor
// =============================

struct SensorReading {

  float temperature_c = 0.0f;
  float humidity_rh   = 0.0f;

  bool ok = false;

  uint32_t last_update_ms = 0;
};


// =============================
// Classe Sensors
// =============================

class Sensors {
public:

  // Inicialização
  void begin();

  // Atualização periódica
  void update();

  // Leituras atuais
  const SensorReading& inside() const { return inside_; }
  const SensorReading& outside() const { return outside_; }


private:

  SensorReading inside_;
  SensorReading outside_;

  uint32_t last_update_ = 0;

  void update_simulated_();
};