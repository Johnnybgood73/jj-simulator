#pragma once
#include <Arduino.h>
#include <config.h>
#include <vector>

#include "fans.h"
#include "web.h"
#include "sensors.h"
#include "cultivation.h"


class App {
public:
  enum class Mode : uint8_t {
    MANUAL = 0,
    ECO,
    BOOST,
    AUTO
  };

  enum class AutoState : uint8_t {
    AUTO_DEFAULT = 0,
    AUTO_REFRIGERAMENTO,
    SAFE
  };

  void begin();
  void loop();

  // Fan state (efetivo)
  uint8_t exhaust_percent() const { return exhaust_percent_; }

  // Manual state
  uint8_t manual_percent() const { return manual_percent_; }
  bool set_manual_percent(uint8_t p);

  // Mode state
  Mode mode() const { return mode_; }
  AutoState auto_state() const { return auto_state_; }
  bool set_mode_from_string(const String& mode);
  const char* mode_name() const;
  const char* auto_state_name() const;

  // Sensor state
  const SensorReading& inside() const { return sensors_.inside(); }
  const SensorReading& outside() const { return sensors_.outside(); }

  // Grow CRUD + active selection
  String grows_json() const;
  bool create_grow(const String& name, const String& type,
                   int width_cm, int depth_cm, int height_cm, String& error);
  bool update_grow(const String& id, const String& name, const String& type,
                   int width_cm, int depth_cm, int height_cm, String& error);
  bool delete_grow(const String& id, String& error);
  bool set_active_grow(const String& id, String& error);

  // Plant CRUD
  String plants_json() const;
  bool create_plant(const String& name, const String& species,
                    const String& germination_date, const String& grow_id, String& error);
  bool update_plant(const String& id, const String& name, const String& species,
                    const String& germination_date, const String& grow_id, String& error);
  bool delete_plant(const String& id, String& error);

  // Species CRUD
  String species_json() const;
  bool create_species(const String& name, int veg_days, int flora_days,
                      float stretch_medio, int veg_height_cm, int flora_height_cm, String& error);
  bool update_species(const String& id, const String& name, int veg_days, int flora_days,
                      float stretch_medio, int veg_height_cm, int flora_height_cm, String& error);
  bool delete_species(const String& id, String& error);

  // Cycles CRUD
  String cycles_json() const;
  bool create_cycle(const String& name, const String& grow_id, const String& plant_ids_csv,
                    const String& start_datetime, const String& phase,
                    int veg_days, int flora_days, float stretch_assumed, String& error);
  bool update_cycle(const String& id, const String& name, const String& grow_id, const String& plant_ids_csv,
                    const String& start_datetime, const String& phase,
                    int veg_days, int flora_days, float stretch_assumed, String& error);
  bool delete_cycle(const String& id, String& error);

private:

  Fans fans_;
  WebUi web_;
  Sensors sensors_;
  CultivationStore cultivation_;

  Mode mode_ = Mode::MANUAL;
  AutoState auto_state_ = AutoState::AUTO_DEFAULT;

  uint8_t manual_percent_ = fan_fixed_percent;
  uint8_t exhaust_percent_ = 0;

  uint32_t last_print_ = 0;

  uint8_t compute_target_percent_();
  uint8_t compute_auto1_percent_();
  void apply_exhaust_percent_(uint8_t p);
  static uint8_t clamp_percent_(int p);
  static void parse_id_csv_(const String& csv, std::vector<String>& out);
};

extern App app;
