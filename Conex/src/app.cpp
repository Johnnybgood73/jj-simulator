#include "app.h"
#include <math.h>
#include <ArduinoJson.h>

App app;

uint8_t App::clamp_percent_(int p) {
  if (p < 0) return 0;
  if (p > 100) return 100;
  return static_cast<uint8_t>(p);
}

void App::parse_id_csv_(const String& csv, std::vector<String>& out) {
  out.clear();
  String token;
  for (size_t i = 0; i < csv.length(); ++i) {
    const char ch = csv[i];
    if (ch == ',') {
      token.trim();
      if (!token.isEmpty()) out.push_back(token);
      token = "";
      continue;
    }
    token += ch;
  }
  token.trim();
  if (!token.isEmpty()) out.push_back(token);
}

const char* App::mode_name() const {
  switch (mode_) {
    case Mode::MANUAL: return "MANUAL";
    case Mode::ECO: return "ECO";
    case Mode::BOOST: return "BOOST";
    case Mode::AUTO: return "AUTO";
    default: return "MANUAL";
  }
}

const char* App::auto_state_name() const {
  switch (auto_state_) {
    case AutoState::AUTO_DEFAULT: return "DEFAULT";
    case AutoState::AUTO_REFRIGERAMENTO: return "REFRIGERAMENTO";
    case AutoState::SAFE: return "SAFE";
    default: return "DEFAULT";
  }
}

String App::grows_json() const {
  DynamicJsonDocument doc(12288);
  doc["active_grow_id"] = cultivation_.active_grow_id();
  JsonArray arr = doc.createNestedArray("grows");

  for (const auto& g : cultivation_.grows()) {
    JsonObject o = arr.createNestedObject();
    o["id"] = g.id;
    o["name"] = g.name;
    o["type"] = g.type;
    o["width_cm"] = g.width_cm;
    o["depth_cm"] = g.depth_cm;
    o["height_cm"] = g.height_cm;
    o["volume_m3"] = g.volume_m3();
  }

  String out;
  serializeJson(doc, out);
  return out;
}

String App::plants_json() const {
  DynamicJsonDocument doc(12288);
  JsonArray arr = doc.createNestedArray("plants");

  for (const auto& p : cultivation_.plants()) {
    JsonObject o = arr.createNestedObject();
    o["id"] = p.id;
    o["name"] = p.name;
    o["species"] = p.species;
    o["germination_date"] = p.germination_date;
    o["grow_id"] = p.grow_id;
  }

  String out;
  serializeJson(doc, out);
  return out;
}

String App::species_json() const {
  DynamicJsonDocument doc(16384);
  JsonArray arr = doc.createNestedArray("species");

  for (const auto& s : cultivation_.species()) {
    JsonObject o = arr.createNestedObject();
    o["id"] = s.id;
    o["name"] = s.name;
    o["duration_veg_days_suggested"] = s.duration_veg_days_suggested;
    o["duration_flora_days_suggested"] = s.duration_flora_days_suggested;
    o["stretch_medio"] = s.stretch_medio;
    o["height_final_veg_cm_suggested"] = s.height_final_veg_cm_suggested;
    o["height_final_flora_cm_suggested"] = s.height_final_flora_cm_suggested;
  }

  String out;
  serializeJson(doc, out);
  return out;
}

String App::cycles_json() const {
  DynamicJsonDocument doc(32768);
  JsonArray arr = doc.createNestedArray("cycles");

  for (const auto& c : cultivation_.cycles()) {
    JsonObject o = arr.createNestedObject();
    o["id"] = c.id;
    o["name"] = c.name;
    o["grow_id"] = c.grow_id;
    JsonArray plantIds = o.createNestedArray("plant_ids");
    for (const auto& pid : c.plant_ids) {
      plantIds.add(pid);
    }
    o["start_datetime"] = c.start_datetime;
    o["phase"] = c.phase;
    o["duration_veg_days"] = c.duration_veg_days;
    o["duration_flora_days"] = c.duration_flora_days;
    o["stretch_assumed"] = c.stretch_assumed;
  }

  String out;
  serializeJson(doc, out);
  return out;
}

bool App::create_grow(const String& name, const String& type,
                      int width_cm, int depth_cm, int height_cm, String& error) {
  if (width_cm <= 0 || depth_cm <= 0 || height_cm <= 0) {
    error = "invalid_dimensions";
    return false;
  }

  GrowRecord out;
  return cultivation_.create_grow(
    name, type,
    static_cast<uint16_t>(width_cm),
    static_cast<uint16_t>(depth_cm),
    static_cast<uint16_t>(height_cm),
    out, error
  );
}

bool App::update_grow(const String& id, const String& name, const String& type,
                      int width_cm, int depth_cm, int height_cm, String& error) {
  if (width_cm <= 0 || depth_cm <= 0 || height_cm <= 0) {
    error = "invalid_dimensions";
    return false;
  }

  GrowRecord g;
  g.id = id;
  g.name = name;
  g.type = type;
  g.width_cm = static_cast<uint16_t>(width_cm);
  g.depth_cm = static_cast<uint16_t>(depth_cm);
  g.height_cm = static_cast<uint16_t>(height_cm);
  return cultivation_.update_grow(g, error);
}

bool App::delete_grow(const String& id, String& error) {
  return cultivation_.delete_grow(id, error);
}

bool App::set_active_grow(const String& id, String& error) {
  if (!cultivation_.set_active_grow_id(id)) {
    error = "invalid_grow_id";
    return false;
  }
  error = "";
  return true;
}

bool App::create_plant(const String& name, const String& species,
                       const String& germination_date, const String& grow_id, String& error) {
  PlantRecord out;
  return cultivation_.create_plant(name, species, germination_date, grow_id, out, error);
}

bool App::update_plant(const String& id, const String& name, const String& species,
                       const String& germination_date, const String& grow_id, String& error) {
  PlantRecord p;
  p.id = id;
  p.name = name;
  p.species = species;
  p.germination_date = germination_date;
  p.grow_id = grow_id;
  return cultivation_.update_plant(p, error);
}

bool App::delete_plant(const String& id, String& error) {
  return cultivation_.delete_plant(id, error);
}

bool App::create_species(const String& name, int veg_days, int flora_days,
                         float stretch_medio, int veg_height_cm, int flora_height_cm, String& error) {
  if (veg_days < 0 || flora_days < 0) {
    error = "invalid_duration";
    return false;
  }
  if (stretch_medio < 0.0f) {
    error = "invalid_stretch";
    return false;
  }
  if (veg_height_cm < 0 || flora_height_cm < 0) {
    error = "invalid_height";
    return false;
  }

  SpeciesRecord s;
  s.name = name;
  s.duration_veg_days_suggested = static_cast<int16_t>(veg_days);
  s.duration_flora_days_suggested = static_cast<int16_t>(flora_days);
  s.stretch_medio = stretch_medio;
  s.height_final_veg_cm_suggested = static_cast<int16_t>(veg_height_cm);
  s.height_final_flora_cm_suggested = static_cast<int16_t>(flora_height_cm);

  SpeciesRecord out;
  return cultivation_.create_species(s, out, error);
}

bool App::update_species(const String& id, const String& name, int veg_days, int flora_days,
                         float stretch_medio, int veg_height_cm, int flora_height_cm, String& error) {
  if (veg_days < 0 || flora_days < 0) {
    error = "invalid_duration";
    return false;
  }
  if (stretch_medio < 0.0f) {
    error = "invalid_stretch";
    return false;
  }
  if (veg_height_cm < 0 || flora_height_cm < 0) {
    error = "invalid_height";
    return false;
  }

  SpeciesRecord s;
  s.id = id;
  s.name = name;
  s.duration_veg_days_suggested = static_cast<int16_t>(veg_days);
  s.duration_flora_days_suggested = static_cast<int16_t>(flora_days);
  s.stretch_medio = stretch_medio;
  s.height_final_veg_cm_suggested = static_cast<int16_t>(veg_height_cm);
  s.height_final_flora_cm_suggested = static_cast<int16_t>(flora_height_cm);
  return cultivation_.update_species(s, error);
}

bool App::delete_species(const String& id, String& error) {
  return cultivation_.delete_species(id, error);
}

bool App::create_cycle(const String& name, const String& grow_id, const String& plant_ids_csv,
                       const String& start_datetime, const String& phase,
                       int veg_days, int flora_days, float stretch_assumed, String& error) {
  if (veg_days < 0 || flora_days < 0) {
    error = "invalid_duration";
    return false;
  }
  if (stretch_assumed < 0.0f) {
    error = "invalid_stretch";
    return false;
  }

  CycleRecord c;
  c.name = name;
  c.grow_id = grow_id;
  parse_id_csv_(plant_ids_csv, c.plant_ids);
  c.start_datetime = start_datetime;
  c.phase = phase;
  c.duration_veg_days = static_cast<int16_t>(veg_days);
  c.duration_flora_days = static_cast<int16_t>(flora_days);
  c.stretch_assumed = stretch_assumed;

  CycleRecord out;
  return cultivation_.create_cycle(c, out, error);
}

bool App::update_cycle(const String& id, const String& name, const String& grow_id, const String& plant_ids_csv,
                       const String& start_datetime, const String& phase,
                       int veg_days, int flora_days, float stretch_assumed, String& error) {
  if (veg_days < 0 || flora_days < 0) {
    error = "invalid_duration";
    return false;
  }
  if (stretch_assumed < 0.0f) {
    error = "invalid_stretch";
    return false;
  }

  CycleRecord c;
  c.id = id;
  c.name = name;
  c.grow_id = grow_id;
  parse_id_csv_(plant_ids_csv, c.plant_ids);
  c.start_datetime = start_datetime;
  c.phase = phase;
  c.duration_veg_days = static_cast<int16_t>(veg_days);
  c.duration_flora_days = static_cast<int16_t>(flora_days);
  c.stretch_assumed = stretch_assumed;

  return cultivation_.update_cycle(c, error);
}

bool App::delete_cycle(const String& id, String& error) {
  return cultivation_.delete_cycle(id, error);
}

void App::apply_exhaust_percent_(uint8_t p) {
  const uint8_t clamped = clamp_percent_(p);
  exhaust_percent_ = clamped;
  fans_.set_exhaust_percent(clamped);
}

bool App::set_manual_percent(uint8_t p) {
  if (mode_ != Mode::MANUAL) return false;

  manual_percent_ = clamp_percent_(p);
  apply_exhaust_percent_(manual_percent_);
  return true;
}

bool App::set_mode_from_string(const String& mode) {
  String m = mode;
  m.toLowerCase();

  if (m == "manual") {
    mode_ = Mode::MANUAL;
  } else if (m == "eco") {
    mode_ = Mode::ECO;
  } else if (m == "boost") {
    mode_ = Mode::BOOST;
  } else if (m == "auto") {
    mode_ = Mode::AUTO;
  } else {
    return false;
  }

  apply_exhaust_percent_(compute_target_percent_());
  return true;
}

uint8_t App::compute_auto1_percent_() {
  const SensorReading in = inside();
  const SensorReading out = outside();

  if (!in.ok || !out.ok) {
    auto_state_ = AutoState::SAFE;
    return 60;
  }

  if (auto_state_ == AutoState::SAFE) {
    auto_state_ = AutoState::AUTO_DEFAULT;
  }

  const float dt = in.temperature_c - out.temperature_c;

  if (dt >= 2.0f) {
    auto_state_ = AutoState::AUTO_REFRIGERAMENTO;
  } else if (dt <= 0.5f) {
    auto_state_ = AutoState::AUTO_DEFAULT;
  }

  if (auto_state_ == AutoState::AUTO_DEFAULT) {
    return 30;
  }

  const float dt_curve = constrain(dt, 2.0f, 6.0f);
  const float pct = 40.0f + (dt_curve - 2.0f) * 15.0f;
  return clamp_percent_(static_cast<int>(lroundf(pct)));
}

uint8_t App::compute_target_percent_() {
  switch (mode_) {
    case Mode::MANUAL:
      return clamp_percent_(manual_percent_);
    case Mode::ECO:
      return 30;
    case Mode::BOOST:
      return 100;
    case Mode::AUTO:
      return compute_auto1_percent_();
    default:
      return clamp_percent_(manual_percent_);
  }
}

void App::begin() {
  Serial.begin(115200);
  delay(200);

  Serial.println();
  Serial.println("Conex - Stage 4: Base de Grows + Plantas");

  fans_.begin();
  sensors_.begin();

  const bool storeOk = cultivation_.begin();
  Serial.print("Cultivation store: ");
  Serial.println(storeOk ? "OK" : "FAIL");

  mode_ = Mode::MANUAL;
  auto_state_ = AutoState::AUTO_DEFAULT;
  manual_percent_ = clamp_percent_(fan_fixed_percent);
  apply_exhaust_percent_(manual_percent_);

  web_.begin(
    [this]() -> uint8_t { return this->exhaust_percent(); },
    [this](uint8_t p) -> bool { return this->set_manual_percent(p); },
    [this]() -> SensorReading { return this->inside(); },
    [this]() -> SensorReading { return this->outside(); },
    [this]() -> const char* { return this->mode_name(); },
    [this](const String& mode) -> bool { return this->set_mode_from_string(mode); },
    [this]() -> const char* { return this->auto_state_name(); },
    [this]() -> String { return this->grows_json(); },
    [this]() -> String { return this->plants_json(); },
    [this]() -> String { return this->species_json(); },
    [this]() -> String { return this->cycles_json(); },
    [this](const String& name, const String& type, int w, int d, int h, String& err) -> bool {
      return this->create_grow(name, type, w, d, h, err);
    },
    [this](const String& id, const String& name, const String& type, int w, int d, int h, String& err) -> bool {
      return this->update_grow(id, name, type, w, d, h, err);
    },
    [this](const String& id, String& err) -> bool {
      return this->delete_grow(id, err);
    },
    [this](const String& id, String& err) -> bool {
      return this->set_active_grow(id, err);
    },
    [this](const String& name, const String& species, const String& date, const String& growId, String& err) -> bool {
      return this->create_plant(name, species, date, growId, err);
    },
    [this](const String& id, const String& name, const String& species, const String& date, const String& growId, String& err) -> bool {
      return this->update_plant(id, name, species, date, growId, err);
    },
    [this](const String& id, String& err) -> bool {
      return this->delete_plant(id, err);
    },
    [this](const String& name, int veg, int flora, float stretch, int vegH, int floraH, String& err) -> bool {
      return this->create_species(name, veg, flora, stretch, vegH, floraH, err);
    },
    [this](const String& id, const String& name, int veg, int flora, float stretch, int vegH, int floraH, String& err) -> bool {
      return this->update_species(id, name, veg, flora, stretch, vegH, floraH, err);
    },
    [this](const String& id, String& err) -> bool {
      return this->delete_species(id, err);
    },
    [this](const String& name, const String& growId, const String& plantIdsCsv, const String& start, const String& phase,
           int veg, int flora, float stretch, String& err) -> bool {
      return this->create_cycle(name, growId, plantIdsCsv, start, phase, veg, flora, stretch, err);
    },
    [this](const String& id, const String& name, const String& growId, const String& plantIdsCsv, const String& start, const String& phase,
           int veg, int flora, float stretch, String& err) -> bool {
      return this->update_cycle(id, name, growId, plantIdsCsv, start, phase, veg, flora, stretch, err);
    },
    [this](const String& id, String& err) -> bool {
      return this->delete_cycle(id, err);
    }
  );

  last_print_ = 0;
  Serial.print("Initial mode: ");
  Serial.println(mode_name());
  Serial.print("Initial exhaust percent: ");
  Serial.println(exhaust_percent_);
}

void App::loop() {
  sensors_.update();

  const uint8_t target = compute_target_percent_();
  if (target != exhaust_percent_) {
    apply_exhaust_percent_(target);
  }

  const uint32_t now = millis();
  if (now - last_print_ >= 1000) {
    last_print_ = now;

    const auto in = inside();
    const auto out = outside();

    Serial.print("MODE: ");
    Serial.print(mode_name());
    if (mode_ == Mode::AUTO) {
      Serial.print(" (");
      Serial.print(auto_state_name());
      Serial.print(")");
    }

    Serial.print(" | FAN: ");
    Serial.print(exhaust_percent_);
    Serial.print("% | IN: ");

    if (in.ok) {
      Serial.print(in.temperature_c, 1);
      Serial.print("C ");
      Serial.print(in.humidity_rh, 0);
      Serial.print("%");
    } else {
      Serial.print("ERR");
    }

    Serial.print(" | OUT: ");
    if (out.ok) {
      Serial.print(out.temperature_c, 1);
      Serial.print("C ");
      Serial.print(out.humidity_rh, 0);
      Serial.print("%");
    } else {
      Serial.print("ERR");
    }

    if (sensors_simulated) Serial.print(" (SIM)");
    Serial.println();
  }

  web_.loop();
  delay(5);
}
