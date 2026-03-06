#include "sensors.h"
#include "../../include/config.h"
#include <math.h>

// =============================
// Inicialização
// =============================

void Sensors::begin() {

  last_update_ = 0;

  // Inicializa estado como inválido
  inside_.ok = false;
  outside_.ok = false;
}


// =============================
// Update periódico
// =============================

void Sensors::update() {

  const uint32_t now = millis();

  if (now - last_update_ < sensors_update_ms)
    return;

  last_update_ = now;

  if (sensors_simulated) {
    update_simulated_();
  }
}


// =============================
// Simulação dos sensores
// =============================

void Sensors::update_simulated_() {

  const float t = millis() / 1000.0f;

  // Temperatura externa (variação lenta)
  outside_.temperature_c =
      26.0f + 2.0f * sinf(t * 0.05f);

  // Umidade externa
  outside_.humidity_rh =
      50.0f + 8.0f * sinf(t * 0.03f);


  // Temperatura interna (ligeiramente maior)
  inside_.temperature_c =
      outside_.temperature_c + 0.8f
      + 0.5f * sinf(t * 0.08f);

  // Umidade interna (ligeiramente diferente)
  inside_.humidity_rh =
      outside_.humidity_rh + 3.0f
      + 2.0f * sinf(t * 0.06f);


  // Marca como válidos
  inside_.ok = true;
  outside_.ok = true;

  inside_.last_update_ms = millis();
  outside_.last_update_ms = millis();
}