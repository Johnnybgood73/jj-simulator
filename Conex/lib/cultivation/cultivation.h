#pragma once
#include <Arduino.h>
#include <vector>

struct GrowRecord {
  String id;
  String name;
  String type;  // indoor | outdoor
  uint16_t width_cm = 0;
  uint16_t depth_cm = 0;
  uint16_t height_cm = 0;

  float volume_m3() const {
    const float cm3 = static_cast<float>(width_cm) * static_cast<float>(depth_cm) * static_cast<float>(height_cm);
    return cm3 / 1000000.0f;
  }
};

struct SpeciesRecord {
  String id;
  String name;
  int16_t duration_veg_days_suggested = 0;
  int16_t duration_flora_days_suggested = 0;
  float stretch_medio = 0.0f;
  int16_t height_final_veg_cm_suggested = 0;
  int16_t height_final_flora_cm_suggested = 0;
};

struct PlantRecord {
  String id;
  String name;
  String species;
  String germination_date;  // YYYY-MM-DD (texto simples por enquanto)
  String grow_id;
};

struct CycleRecord {
  String id;
  String name;
  String grow_id;
  std::vector<String> plant_ids;
  String start_datetime;  // ISO texto simples por enquanto
  String phase;
  int16_t duration_veg_days = 0;
  int16_t duration_flora_days = 0;
  float stretch_assumed = 0.0f;
};

class CultivationStore {
public:
  bool begin();

  const std::vector<GrowRecord>& grows() const { return grows_; }
  const std::vector<SpeciesRecord>& species() const { return species_; }
  const std::vector<PlantRecord>& plants() const { return plants_; }
  const std::vector<CycleRecord>& cycles() const { return cycles_; }

  String active_grow_id() const { return active_grow_id_; }
  bool set_active_grow_id(const String& grow_id);

  bool create_grow(const String& name, const String& type,
                   uint16_t width_cm, uint16_t depth_cm, uint16_t height_cm,
                   GrowRecord& out, String& error);
  bool update_grow(const GrowRecord& record, String& error);
  bool delete_grow(const String& grow_id, String& error);

  bool create_plant(const String& name, const String& species,
                    const String& germination_date, const String& grow_id,
                    PlantRecord& out, String& error);
  bool update_plant(const PlantRecord& record, String& error);
  bool delete_plant(const String& plant_id, String& error);

  bool create_species(const SpeciesRecord& record, SpeciesRecord& out, String& error);
  bool update_species(const SpeciesRecord& record, String& error);
  bool delete_species(const String& species_id, String& error);

  bool create_cycle(const CycleRecord& record, CycleRecord& out, String& error);
  bool update_cycle(const CycleRecord& record, String& error);
  bool delete_cycle(const String& cycle_id, String& error);

private:
  std::vector<GrowRecord> grows_;
  std::vector<SpeciesRecord> species_;
  std::vector<PlantRecord> plants_;
  std::vector<CycleRecord> cycles_;
  String active_grow_id_;

  bool load_grows_();
  bool save_grows_() const;
  bool load_species_();
  bool save_species_() const;
  bool load_plants_();
  bool save_plants_() const;
  bool load_cycles_();
  bool save_cycles_() const;

  static bool normalize_grow_type_(const String& raw, String& normalized);
  static String normalized_trim_(const String& raw);
  static bool split_csv_ids_(const String& csv, std::vector<String>& out);
  String next_id_(char prefix) const;
  bool grow_exists_(const String& grow_id) const;
  bool species_exists_(const String& species_id) const;
  bool plant_exists_(const String& plant_id) const;
};
