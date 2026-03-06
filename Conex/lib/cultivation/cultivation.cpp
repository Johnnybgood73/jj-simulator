#include "cultivation.h"
#include <ArduinoJson.h>
#include <SPIFFS.h>
#include <Preferences.h>

namespace {
constexpr const char* kGrowsPath = "/grows.json";
constexpr const char* kSpeciesPath = "/species.json";
constexpr const char* kPlantsPath = "/plants.json";
constexpr const char* kCyclesPath = "/cycles.json";
constexpr const char* kPrefsNs = "conexgrow";
constexpr const char* kPrefsActiveGrow = "active_grow_id";
}

bool CultivationStore::begin() {
  if (!SPIFFS.begin(true)) {
    return false;
  }

  if (!load_grows_()) return false;
  if (!load_species_()) return false;
  if (!load_plants_()) return false;
  if (!load_cycles_()) return false;

  Preferences prefs;
  if (!prefs.begin(kPrefsNs, true)) {
    return false;
  }
  active_grow_id_ = prefs.getString(kPrefsActiveGrow, "");
  prefs.end();

  if (!active_grow_id_.isEmpty() && !grow_exists_(active_grow_id_)) {
    active_grow_id_ = "";
  }
  return true;
}

bool CultivationStore::set_active_grow_id(const String& grow_id) {
  String id = normalized_trim_(grow_id);

  if (!id.isEmpty() && !grow_exists_(id)) {
    return false;
  }

  Preferences prefs;
  if (!prefs.begin(kPrefsNs, false)) {
    return false;
  }
  prefs.putString(kPrefsActiveGrow, id);
  prefs.end();

  active_grow_id_ = id;
  return true;
}

bool CultivationStore::create_grow(const String& name, const String& type,
                                   uint16_t width_cm, uint16_t depth_cm, uint16_t height_cm,
                                   GrowRecord& out, String& error) {
  error = "";
  GrowRecord g;
  g.name = normalized_trim_(name);
  if (g.name.isEmpty()) {
    error = "invalid_name";
    return false;
  }

  if (!normalize_grow_type_(type, g.type)) {
    error = "invalid_type";
    return false;
  }

  if (width_cm == 0 || depth_cm == 0 || height_cm == 0) {
    error = "invalid_dimensions";
    return false;
  }

  g.width_cm = width_cm;
  g.depth_cm = depth_cm;
  g.height_cm = height_cm;
  g.id = next_id_('g');

  grows_.push_back(g);

  if (!save_grows_()) {
    grows_.pop_back();
    error = "persist_failed";
    return false;
  }

  out = g;
  return true;
}

bool CultivationStore::update_grow(const GrowRecord& record, String& error) {
  error = "";
  const String id = normalized_trim_(record.id);
  if (id.isEmpty()) {
    error = "invalid_id";
    return false;
  }

  String normalizedType;
  if (!normalize_grow_type_(record.type, normalizedType)) {
    error = "invalid_type";
    return false;
  }

  const String normalizedName = normalized_trim_(record.name);
  if (normalizedName.isEmpty()) {
    error = "invalid_name";
    return false;
  }

  if (record.width_cm == 0 || record.depth_cm == 0 || record.height_cm == 0) {
    error = "invalid_dimensions";
    return false;
  }

  for (auto& g : grows_) {
    if (g.id == id) {
      g.name = normalizedName;
      g.type = normalizedType;
      g.width_cm = record.width_cm;
      g.depth_cm = record.depth_cm;
      g.height_cm = record.height_cm;

      if (!save_grows_()) {
        error = "persist_failed";
        return false;
      }
      return true;
    }
  }

  error = "not_found";
  return false;
}

bool CultivationStore::delete_grow(const String& grow_id, String& error) {
  error = "";
  const String id = normalized_trim_(grow_id);
  if (id.isEmpty()) {
    error = "invalid_id";
    return false;
  }

  for (const auto& p : plants_) {
    if (p.grow_id == id) {
      error = "grow_has_plants";
      return false;
    }
  }
  for (const auto& c : cycles_) {
    if (c.grow_id == id) {
      error = "grow_has_cycles";
      return false;
    }
  }

  for (size_t i = 0; i < grows_.size(); ++i) {
    if (grows_[i].id == id) {
      grows_.erase(grows_.begin() + i);
      if (!save_grows_()) {
        error = "persist_failed";
        return false;
      }

      if (active_grow_id_ == id) {
        set_active_grow_id("");
      }
      return true;
    }
  }

  error = "not_found";
  return false;
}

bool CultivationStore::create_plant(const String& name, const String& species,
                                    const String& germination_date, const String& grow_id,
                                    PlantRecord& out, String& error) {
  error = "";
  PlantRecord p;
  p.name = normalized_trim_(name);
  p.species = normalized_trim_(species);
  p.germination_date = normalized_trim_(germination_date);
  p.grow_id = normalized_trim_(grow_id);

  if (p.name.isEmpty()) {
    error = "invalid_name";
    return false;
  }
  if (p.species.isEmpty()) {
    error = "invalid_species";
    return false;
  }
  if (p.germination_date.isEmpty()) {
    error = "invalid_germination_date";
    return false;
  }
  if (p.grow_id.isEmpty() || !grow_exists_(p.grow_id)) {
    error = "invalid_grow_id";
    return false;
  }

  p.id = next_id_('p');
  plants_.push_back(p);

  if (!save_plants_()) {
    plants_.pop_back();
    error = "persist_failed";
    return false;
  }

  out = p;
  return true;
}

bool CultivationStore::update_plant(const PlantRecord& record, String& error) {
  error = "";
  const String id = normalized_trim_(record.id);
  if (id.isEmpty()) {
    error = "invalid_id";
    return false;
  }

  const String name = normalized_trim_(record.name);
  const String species = normalized_trim_(record.species);
  const String germinationDate = normalized_trim_(record.germination_date);
  const String growId = normalized_trim_(record.grow_id);

  if (name.isEmpty()) {
    error = "invalid_name";
    return false;
  }
  if (species.isEmpty()) {
    error = "invalid_species";
    return false;
  }
  if (germinationDate.isEmpty()) {
    error = "invalid_germination_date";
    return false;
  }
  if (growId.isEmpty() || !grow_exists_(growId)) {
    error = "invalid_grow_id";
    return false;
  }

  for (auto& p : plants_) {
    if (p.id == id) {
      p.name = name;
      p.species = species;
      p.germination_date = germinationDate;
      p.grow_id = growId;

      if (!save_plants_()) {
        error = "persist_failed";
        return false;
      }
      return true;
    }
  }

  error = "not_found";
  return false;
}

bool CultivationStore::delete_plant(const String& plant_id, String& error) {
  error = "";
  const String id = normalized_trim_(plant_id);
  if (id.isEmpty()) {
    error = "invalid_id";
    return false;
  }

  for (size_t i = 0; i < plants_.size(); ++i) {
    if (plants_[i].id == id) {
      for (const auto& c : cycles_) {
        for (const auto& cyclePlantId : c.plant_ids) {
          if (cyclePlantId == id) {
            error = "plant_has_cycles";
            return false;
          }
        }
      }

      plants_.erase(plants_.begin() + i);
      if (!save_plants_()) {
        error = "persist_failed";
        return false;
      }
      return true;
    }
  }

  error = "not_found";
  return false;
}

bool CultivationStore::create_species(const SpeciesRecord& record, SpeciesRecord& out, String& error) {
  error = "";

  SpeciesRecord s = record;
  s.name = normalized_trim_(record.name);
  if (s.name.isEmpty()) {
    error = "invalid_name";
    return false;
  }
  if (s.duration_veg_days_suggested < 0 || s.duration_flora_days_suggested < 0) {
    error = "invalid_duration";
    return false;
  }
  if (s.stretch_medio < 0.0f) {
    error = "invalid_stretch";
    return false;
  }
  if (s.height_final_veg_cm_suggested < 0 || s.height_final_flora_cm_suggested < 0) {
    error = "invalid_height";
    return false;
  }

  s.id = next_id_('s');
  species_.push_back(s);

  if (!save_species_()) {
    species_.pop_back();
    error = "persist_failed";
    return false;
  }

  out = s;
  return true;
}

bool CultivationStore::update_species(const SpeciesRecord& record, String& error) {
  error = "";
  const String id = normalized_trim_(record.id);
  if (id.isEmpty()) {
    error = "invalid_id";
    return false;
  }

  const String name = normalized_trim_(record.name);
  if (name.isEmpty()) {
    error = "invalid_name";
    return false;
  }
  if (record.duration_veg_days_suggested < 0 || record.duration_flora_days_suggested < 0) {
    error = "invalid_duration";
    return false;
  }
  if (record.stretch_medio < 0.0f) {
    error = "invalid_stretch";
    return false;
  }
  if (record.height_final_veg_cm_suggested < 0 || record.height_final_flora_cm_suggested < 0) {
    error = "invalid_height";
    return false;
  }

  for (auto& s : species_) {
    if (s.id == id) {
      s.name = name;
      s.duration_veg_days_suggested = record.duration_veg_days_suggested;
      s.duration_flora_days_suggested = record.duration_flora_days_suggested;
      s.stretch_medio = record.stretch_medio;
      s.height_final_veg_cm_suggested = record.height_final_veg_cm_suggested;
      s.height_final_flora_cm_suggested = record.height_final_flora_cm_suggested;

      if (!save_species_()) {
        error = "persist_failed";
        return false;
      }
      return true;
    }
  }

  error = "not_found";
  return false;
}

bool CultivationStore::delete_species(const String& species_id, String& error) {
  error = "";
  const String id = normalized_trim_(species_id);
  if (id.isEmpty()) {
    error = "invalid_id";
    return false;
  }

  for (size_t i = 0; i < species_.size(); ++i) {
    if (species_[i].id == id) {
      species_.erase(species_.begin() + i);
      if (!save_species_()) {
        error = "persist_failed";
        return false;
      }
      return true;
    }
  }

  error = "not_found";
  return false;
}

bool CultivationStore::create_cycle(const CycleRecord& record, CycleRecord& out, String& error) {
  error = "";
  CycleRecord c = record;

  c.name = normalized_trim_(record.name);
  c.grow_id = normalized_trim_(record.grow_id);
  c.start_datetime = normalized_trim_(record.start_datetime);
  c.phase = normalized_trim_(record.phase);

  if (c.name.isEmpty()) {
    error = "invalid_name";
    return false;
  }
  if (c.grow_id.isEmpty() || !grow_exists_(c.grow_id)) {
    error = "invalid_grow_id";
    return false;
  }
  if (c.plant_ids.empty()) {
    error = "missing_plants";
    return false;
  }
  for (auto& plantId : c.plant_ids) {
    plantId = normalized_trim_(plantId);
    if (plantId.isEmpty() || !plant_exists_(plantId)) {
      error = "invalid_plant_id";
      return false;
    }
  }
  if (c.start_datetime.isEmpty()) {
    error = "invalid_start_datetime";
    return false;
  }
  if (c.duration_veg_days < 0 || c.duration_flora_days < 0) {
    error = "invalid_duration";
    return false;
  }
  if (c.stretch_assumed < 0.0f) {
    error = "invalid_stretch";
    return false;
  }

  c.id = next_id_('c');
  cycles_.push_back(c);

  if (!save_cycles_()) {
    cycles_.pop_back();
    error = "persist_failed";
    return false;
  }

  out = c;
  return true;
}

bool CultivationStore::update_cycle(const CycleRecord& record, String& error) {
  error = "";
  const String id = normalized_trim_(record.id);
  if (id.isEmpty()) {
    error = "invalid_id";
    return false;
  }

  const String name = normalized_trim_(record.name);
  const String growId = normalized_trim_(record.grow_id);
  const String startDatetime = normalized_trim_(record.start_datetime);
  const String phase = normalized_trim_(record.phase);

  if (name.isEmpty()) {
    error = "invalid_name";
    return false;
  }
  if (growId.isEmpty() || !grow_exists_(growId)) {
    error = "invalid_grow_id";
    return false;
  }
  if (record.plant_ids.empty()) {
    error = "missing_plants";
    return false;
  }
  std::vector<String> normalizedPlantIds;
  normalizedPlantIds.reserve(record.plant_ids.size());
  for (const auto& rawId : record.plant_ids) {
    const String plantId = normalized_trim_(rawId);
    if (plantId.isEmpty() || !plant_exists_(plantId)) {
      error = "invalid_plant_id";
      return false;
    }
    normalizedPlantIds.push_back(plantId);
  }
  if (startDatetime.isEmpty()) {
    error = "invalid_start_datetime";
    return false;
  }
  if (record.duration_veg_days < 0 || record.duration_flora_days < 0) {
    error = "invalid_duration";
    return false;
  }
  if (record.stretch_assumed < 0.0f) {
    error = "invalid_stretch";
    return false;
  }

  for (auto& c : cycles_) {
    if (c.id == id) {
      c.name = name;
      c.grow_id = growId;
      c.plant_ids = normalizedPlantIds;
      c.start_datetime = startDatetime;
      c.phase = phase;
      c.duration_veg_days = record.duration_veg_days;
      c.duration_flora_days = record.duration_flora_days;
      c.stretch_assumed = record.stretch_assumed;

      if (!save_cycles_()) {
        error = "persist_failed";
        return false;
      }
      return true;
    }
  }

  error = "not_found";
  return false;
}

bool CultivationStore::delete_cycle(const String& cycle_id, String& error) {
  error = "";
  const String id = normalized_trim_(cycle_id);
  if (id.isEmpty()) {
    error = "invalid_id";
    return false;
  }

  for (size_t i = 0; i < cycles_.size(); ++i) {
    if (cycles_[i].id == id) {
      cycles_.erase(cycles_.begin() + i);
      if (!save_cycles_()) {
        error = "persist_failed";
        return false;
      }
      return true;
    }
  }

  error = "not_found";
  return false;
}

bool CultivationStore::load_grows_() {
  grows_.clear();
  if (!SPIFFS.exists(kGrowsPath)) {
    return true;
  }

  File f = SPIFFS.open(kGrowsPath, FILE_READ);
  if (!f) return false;

  DynamicJsonDocument doc(8192);
  const DeserializationError err = deserializeJson(doc, f);
  f.close();
  if (err) return false;

  if (!doc.is<JsonArray>()) return false;
  JsonArray arr = doc.as<JsonArray>();
  for (JsonVariant v : arr) {
    GrowRecord g;
    g.id = String(v["id"] | "");
    g.name = String(v["name"] | "");
    g.type = String(v["type"] | "");
    g.width_cm = static_cast<uint16_t>(v["width_cm"] | 0);
    g.depth_cm = static_cast<uint16_t>(v["depth_cm"] | 0);
    g.height_cm = static_cast<uint16_t>(v["height_cm"] | 0);
    if (!g.id.isEmpty()) {
      grows_.push_back(g);
    }
  }

  return true;
}

bool CultivationStore::save_grows_() const {
  DynamicJsonDocument doc(8192);
  JsonArray arr = doc.to<JsonArray>();

  for (const auto& g : grows_) {
    JsonObject o = arr.createNestedObject();
    o["id"] = g.id;
    o["name"] = g.name;
    o["type"] = g.type;
    o["width_cm"] = g.width_cm;
    o["depth_cm"] = g.depth_cm;
    o["height_cm"] = g.height_cm;
  }

  File f = SPIFFS.open(kGrowsPath, FILE_WRITE);
  if (!f) return false;
  const size_t written = serializeJson(doc, f);
  f.close();
  return written > 0;
}

bool CultivationStore::load_species_() {
  species_.clear();
  if (!SPIFFS.exists(kSpeciesPath)) {
    return true;
  }

  File f = SPIFFS.open(kSpeciesPath, FILE_READ);
  if (!f) return false;

  DynamicJsonDocument doc(16384);
  const DeserializationError err = deserializeJson(doc, f);
  f.close();
  if (err) return false;

  if (!doc.is<JsonArray>()) return false;
  JsonArray arr = doc.as<JsonArray>();
  for (JsonVariant v : arr) {
    SpeciesRecord s;
    s.id = String(v["id"] | "");
    s.name = String(v["name"] | "");
    s.duration_veg_days_suggested = static_cast<int16_t>(v["duration_veg_days_suggested"] | 0);
    s.duration_flora_days_suggested = static_cast<int16_t>(v["duration_flora_days_suggested"] | 0);
    s.stretch_medio = static_cast<float>(v["stretch_medio"] | 0.0f);
    s.height_final_veg_cm_suggested = static_cast<int16_t>(v["height_final_veg_cm_suggested"] | 0);
    s.height_final_flora_cm_suggested = static_cast<int16_t>(v["height_final_flora_cm_suggested"] | 0);
    if (!s.id.isEmpty()) {
      species_.push_back(s);
    }
  }

  return true;
}

bool CultivationStore::save_species_() const {
  DynamicJsonDocument doc(16384);
  JsonArray arr = doc.to<JsonArray>();

  for (const auto& s : species_) {
    JsonObject o = arr.createNestedObject();
    o["id"] = s.id;
    o["name"] = s.name;
    o["duration_veg_days_suggested"] = s.duration_veg_days_suggested;
    o["duration_flora_days_suggested"] = s.duration_flora_days_suggested;
    o["stretch_medio"] = s.stretch_medio;
    o["height_final_veg_cm_suggested"] = s.height_final_veg_cm_suggested;
    o["height_final_flora_cm_suggested"] = s.height_final_flora_cm_suggested;
  }

  File f = SPIFFS.open(kSpeciesPath, FILE_WRITE);
  if (!f) return false;
  const size_t written = serializeJson(doc, f);
  f.close();
  return written > 0;
}

bool CultivationStore::load_plants_() {
  plants_.clear();
  if (!SPIFFS.exists(kPlantsPath)) {
    return true;
  }

  File f = SPIFFS.open(kPlantsPath, FILE_READ);
  if (!f) return false;

  DynamicJsonDocument doc(12288);
  const DeserializationError err = deserializeJson(doc, f);
  f.close();
  if (err) return false;

  if (!doc.is<JsonArray>()) return false;
  JsonArray arr = doc.as<JsonArray>();
  for (JsonVariant v : arr) {
    PlantRecord p;
    p.id = String(v["id"] | "");
    p.name = String(v["name"] | "");
    p.species = String(v["species"] | "");
    p.germination_date = String(v["germination_date"] | "");
    p.grow_id = String(v["grow_id"] | "");
    if (!p.id.isEmpty()) {
      plants_.push_back(p);
    }
  }

  return true;
}

bool CultivationStore::save_plants_() const {
  DynamicJsonDocument doc(12288);
  JsonArray arr = doc.to<JsonArray>();

  for (const auto& p : plants_) {
    JsonObject o = arr.createNestedObject();
    o["id"] = p.id;
    o["name"] = p.name;
    o["species"] = p.species;
    o["germination_date"] = p.germination_date;
    o["grow_id"] = p.grow_id;
  }

  File f = SPIFFS.open(kPlantsPath, FILE_WRITE);
  if (!f) return false;
  const size_t written = serializeJson(doc, f);
  f.close();
  return written > 0;
}

bool CultivationStore::load_cycles_() {
  cycles_.clear();
  if (!SPIFFS.exists(kCyclesPath)) {
    return true;
  }

  File f = SPIFFS.open(kCyclesPath, FILE_READ);
  if (!f) return false;

  DynamicJsonDocument doc(28672);
  const DeserializationError err = deserializeJson(doc, f);
  f.close();
  if (err) return false;

  if (!doc.is<JsonArray>()) return false;
  JsonArray arr = doc.as<JsonArray>();
  for (JsonVariant v : arr) {
    CycleRecord c;
    c.id = String(v["id"] | "");
    c.name = String(v["name"] | "");
    c.grow_id = String(v["grow_id"] | "");
    c.start_datetime = String(v["start_datetime"] | "");
    c.phase = String(v["phase"] | "");
    c.duration_veg_days = static_cast<int16_t>(v["duration_veg_days"] | 0);
    c.duration_flora_days = static_cast<int16_t>(v["duration_flora_days"] | 0);
    c.stretch_assumed = static_cast<float>(v["stretch_assumed"] | 0.0f);

    JsonArray plants = v["plant_ids"].as<JsonArray>();
    if (!plants.isNull()) {
      for (JsonVariant pid : plants) {
        c.plant_ids.push_back(String(pid.as<const char*>() ? pid.as<const char*>() : ""));
      }
    }

    if (!c.id.isEmpty()) {
      cycles_.push_back(c);
    }
  }

  return true;
}

bool CultivationStore::save_cycles_() const {
  DynamicJsonDocument doc(28672);
  JsonArray arr = doc.to<JsonArray>();

  for (const auto& c : cycles_) {
    JsonObject o = arr.createNestedObject();
    o["id"] = c.id;
    o["name"] = c.name;
    o["grow_id"] = c.grow_id;
    JsonArray plants = o.createNestedArray("plant_ids");
    for (const auto& pid : c.plant_ids) {
      plants.add(pid);
    }
    o["start_datetime"] = c.start_datetime;
    o["phase"] = c.phase;
    o["duration_veg_days"] = c.duration_veg_days;
    o["duration_flora_days"] = c.duration_flora_days;
    o["stretch_assumed"] = c.stretch_assumed;
  }

  File f = SPIFFS.open(kCyclesPath, FILE_WRITE);
  if (!f) return false;
  const size_t written = serializeJson(doc, f);
  f.close();
  return written > 0;
}

bool CultivationStore::normalize_grow_type_(const String& raw, String& normalized) {
  normalized = normalized_trim_(raw);
  normalized.toLowerCase();
  return normalized == "indoor" || normalized == "outdoor";
}

String CultivationStore::normalized_trim_(const String& raw) {
  String s = raw;
  s.trim();
  return s;
}

bool CultivationStore::split_csv_ids_(const String& csv, std::vector<String>& out) {
  out.clear();
  String token;

  for (size_t i = 0; i < csv.length(); ++i) {
    const char ch = csv[i];
    if (ch == ',') {
      token.trim();
      if (!token.isEmpty()) {
        out.push_back(token);
      }
      token = "";
      continue;
    }
    token += ch;
  }

  token.trim();
  if (!token.isEmpty()) {
    out.push_back(token);
  }
  return true;
}

String CultivationStore::next_id_(char prefix) const {
  uint32_t maxId = 0;

  if (prefix == 'g') {
    for (const auto& g : grows_) {
      if (g.id.length() < 2 || g.id[0] != 'g') continue;
      const uint32_t n = static_cast<uint32_t>(g.id.substring(1).toInt());
      if (n > maxId) maxId = n;
    }
  } else if (prefix == 'p') {
    for (const auto& p : plants_) {
      if (p.id.length() < 2 || p.id[0] != 'p') continue;
      const uint32_t n = static_cast<uint32_t>(p.id.substring(1).toInt());
      if (n > maxId) maxId = n;
    }
  } else if (prefix == 's') {
    for (const auto& s : species_) {
      if (s.id.length() < 2 || s.id[0] != 's') continue;
      const uint32_t n = static_cast<uint32_t>(s.id.substring(1).toInt());
      if (n > maxId) maxId = n;
    }
  } else if (prefix == 'c') {
    for (const auto& c : cycles_) {
      if (c.id.length() < 2 || c.id[0] != 'c') continue;
      const uint32_t n = static_cast<uint32_t>(c.id.substring(1).toInt());
      if (n > maxId) maxId = n;
    }
  }

  return String(prefix) + String(maxId + 1);
}

bool CultivationStore::grow_exists_(const String& grow_id) const {
  for (const auto& g : grows_) {
    if (g.id == grow_id) return true;
  }
  return false;
}

bool CultivationStore::species_exists_(const String& species_id) const {
  for (const auto& s : species_) {
    if (s.id == species_id) return true;
  }
  return false;
}

bool CultivationStore::plant_exists_(const String& plant_id) const {
  for (const auto& p : plants_) {
    if (p.id == plant_id) return true;
  }
  return false;
}
