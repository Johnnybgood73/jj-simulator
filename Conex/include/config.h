#pragma once
#include <Arduino.h>

// =============================
// FAN PWM
// =============================

// Fan principal (exaustor)
static constexpr int pin_pwm_exhaust = 25;

// Reservado para futuro segundo fan
static constexpr int pin_pwm_spare = 26;


// =============================
// FAN PWM Config
// =============================

static constexpr uint32_t fan_pwm_freq_hz = 25000;
static constexpr uint8_t  fan_pwm_res_bits = 8;
static constexpr uint8_t  fan_pwm_ch_exhaust = 0;

// Velocidade inicial
static constexpr uint8_t fan_fixed_percent = 70;


// =============================
// I2C Sensores (AHT10)
// =============================

// Sensor INTERNO
static constexpr int i2c_inside_sda = 21;
static constexpr int i2c_inside_scl = 22;

// Sensor EXTERNO
static constexpr int i2c_outside_sda = 17;
static constexpr int i2c_outside_scl = 16;

// Frequência I2C
static constexpr uint32_t i2c_freq_hz = 100000;


// =============================
// Sensores - modo simulação
// =============================
//
// Por enquanto SEMPRE simulado.
// Depois podemos trocar para real.
//
static constexpr bool sensors_simulated = true;


// =============================
// Intervalo de leitura sensores
// =============================

static constexpr uint32_t sensors_update_ms = 1000;


// =============================
// WiFi (Web UI)
// =============================

static constexpr const char* wifi_ap_ssid = "conex-grow";
static constexpr const char* wifi_ap_pass = "12345678";

static constexpr const char* wifi_sta_ssid = "";
static constexpr const char* wifi_sta_pass = "";