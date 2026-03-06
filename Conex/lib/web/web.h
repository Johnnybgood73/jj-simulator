#pragma once
#include <Arduino.h>
#include <WebServer.h>
#include "sensors.h"

class WebUi {
public:
  // Callbacks de estado/acao
  using GetPercentFn = std::function<uint8_t()>;
  using SetPercentFn = std::function<bool(uint8_t)>;
  using GetSensorFn = std::function<SensorReading()>;
  using GetModeFn = std::function<const char*()>;
  using SetModeFn = std::function<bool(const String&)>;
  using GetAutoStateFn = std::function<const char*()>;
  using GetJsonFn = std::function<String()>;
  using GrowCreateFn = std::function<bool(const String&, const String&, int, int, int, String&)>;
  using GrowUpdateFn = std::function<bool(const String&, const String&, const String&, int, int, int, String&)>;
  using GrowDeleteFn = std::function<bool(const String&, String&)>;
  using GrowSetActiveFn = std::function<bool(const String&, String&)>;
  using PlantCreateFn = std::function<bool(const String&, const String&, const String&, const String&, String&)>;
  using PlantUpdateFn = std::function<bool(const String&, const String&, const String&, const String&, const String&, String&)>;
  using PlantDeleteFn = std::function<bool(const String&, String&)>;
  using SpeciesCreateFn = std::function<bool(const String&, int, int, float, int, int, String&)>;
  using SpeciesUpdateFn = std::function<bool(const String&, const String&, int, int, float, int, int, String&)>;
  using SpeciesDeleteFn = std::function<bool(const String&, String&)>;
  using CycleCreateFn = std::function<bool(const String&, const String&, const String&, const String&, const String&, int, int, float, String&)>;
  using CycleUpdateFn = std::function<bool(const String&, const String&, const String&, const String&, const String&, const String&, int, int, float, String&)>;
  using CycleDeleteFn = std::function<bool(const String&, String&)>;

  void begin(GetPercentFn getPercent, SetPercentFn setPercent,
             GetSensorFn getInside, GetSensorFn getOutside,
             GetModeFn getMode, SetModeFn setMode,
             GetAutoStateFn getAutoState,
             GetJsonFn getGrowsJson, GetJsonFn getPlantsJson,
             GetJsonFn getSpeciesJson, GetJsonFn getCyclesJson,
             GrowCreateFn createGrow, GrowUpdateFn updateGrow,
             GrowDeleteFn deleteGrow, GrowSetActiveFn setActiveGrow,
             PlantCreateFn createPlant, PlantUpdateFn updatePlant,
             PlantDeleteFn deletePlant,
             SpeciesCreateFn createSpecies, SpeciesUpdateFn updateSpecies,
             SpeciesDeleteFn deleteSpecies,
             CycleCreateFn createCycle, CycleUpdateFn updateCycle,
             CycleDeleteFn deleteCycle);
  void loop();

private:
  WebServer server_{80};

  GetPercentFn getPercent_;
  SetPercentFn setPercent_;
  GetSensorFn getInside_;
  GetSensorFn getOutside_;
  GetModeFn getMode_;
  SetModeFn setMode_;
  GetAutoStateFn getAutoState_;
  GetJsonFn getGrowsJson_;
  GetJsonFn getPlantsJson_;
  GetJsonFn getSpeciesJson_;
  GetJsonFn getCyclesJson_;
  GrowCreateFn createGrow_;
  GrowUpdateFn updateGrow_;
  GrowDeleteFn deleteGrow_;
  GrowSetActiveFn setActiveGrow_;
  PlantCreateFn createPlant_;
  PlantUpdateFn updatePlant_;
  PlantDeleteFn deletePlant_;
  SpeciesCreateFn createSpecies_;
  SpeciesUpdateFn updateSpecies_;
  SpeciesDeleteFn deleteSpecies_;
  CycleCreateFn createCycle_;
  CycleUpdateFn updateCycle_;
  CycleDeleteFn deleteCycle_;

  void setup_wifi_();
  void setup_routes_();

  static uint8_t clamp_percent_(int v);
};
