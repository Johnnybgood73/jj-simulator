// ================================
// Conex Grow - Simulador (backend virtual)
// ================================

const simState = {
  fan: { exhaust_percent: 70 },
  mode: "MANUAL",
  auto: { state: "DEFAULT" },
  manual_percent: 70,
  sensors: {
    inside: { temperature_c: 0, humidity_rh: 0, ok: false },
    outside: { temperature_c: 0, humidity_rh: 0, ok: false }
  },
  grows: [
    {
      id: "g1",
      name: "Grow Flora",
      type: "indoor",
      width_cm: 80,
      depth_cm: 80,
      height_cm: 180
    }
  ],
  plants: [],
  species: [
    {
      id: "s1",
      name: "White Widow",
      avg_veg_days: 35,
      avg_flora_days: 60,
      expected_height_indoor_cm: 110,
      expected_height_outdoor_cm: 190,
      expected_yield_indoor_g_m2: 500,
      expected_yield_outdoor_g_m2: 650,
      sativa_pct: 40,
      indica_pct: 60,
      thc_pct: 19,
      cbd_pct: 0.2
    },
    {
      id: "s2",
      name: "Critical 2.0",
      avg_veg_days: 32,
      avg_flora_days: 56,
      expected_height_indoor_cm: 130,
      expected_height_outdoor_cm: 220,
      expected_yield_indoor_g_m2: 700,
      expected_yield_outdoor_g_m2: 900,
      sativa_pct: 30,
      indica_pct: 70,
      thc_pct: 22,
      cbd_pct: 0.2
    }
  ],
  cycles: [],
  active_grow_id: "g1",
  online: true,
  latency_ms: 120,
  fail_mode: false,
  sensors_started: false
};

function clampPercent(v) {
  if (v < 0) return 0;
  if (v > 100) return 100;
  return v;
}

function normalizeMode(mode) {
  if (!mode) return null;
  const m = String(mode).toUpperCase();
  if (m === "MANUAL" || m === "ECO" || m === "BOOST" || m === "AUTO") return m;
  return null;
}

function sleep(ms) {
  return new Promise((r) => setTimeout(r, ms));
}

function round1(v) {
  return Math.round(v * 10) / 10;
}

function normalizeTrim(raw) {
  return String(raw || "").trim();
}

function growVolumeM3(g) {
  return (Number(g.width_cm) * Number(g.depth_cm) * Number(g.height_cm)) / 1000000;
}

function nextId(prefix, items) {
  let max = 0;
  for (const item of items) {
    const id = String(item.id || "");
    if (!id.startsWith(prefix)) continue;
    const n = Number(id.slice(1));
    if (Number.isFinite(n) && n > max) max = n;
  }
  return prefix + String(max + 1);
}

function parseCsvIds(csv) {
  return String(csv || "")
    .split(",")
    .map((s) => s.trim())
    .filter((s) => s.length > 0);
}

function plantContainerSupportsSoil(containerType) {
  if (!containerType) return false;
  return containerType === "jiffy" || String(containerType).startsWith("vaso_");
}

function nowForDatetimeLocal() {
  const d = new Date();
  const pad = (n) => String(n).padStart(2, "0");
  const y = d.getFullYear();
  const m = pad(d.getMonth() + 1);
  const day = pad(d.getDate());
  const h = pad(d.getHours());
  const min = pad(d.getMinutes());
  return `${y}-${m}-${day}T${h}:${min}`;
}

function updateSensorsSim() {
  const t = Date.now() / 1000;

  const outsideTemp = 26 + 2 * Math.sin(t * 0.05);
  const outsideHum = 50 + 8 * Math.sin(t * 0.03);

  const insideTemp = outsideTemp + 0.8 + 0.5 * Math.sin(t * 0.08);
  const insideHum = outsideHum + 3 + 2 * Math.sin(t * 0.06);

  simState.sensors.outside = {
    temperature_c: round1(outsideTemp),
    humidity_rh: Math.round(outsideHum),
    ok: true
  };

  simState.sensors.inside = {
    temperature_c: round1(insideTemp),
    humidity_rh: Math.round(insideHum),
    ok: true
  };
}

function computeAuto1Percent() {
  const inside = simState.sensors.inside;
  const outside = simState.sensors.outside;

  if (!inside.ok || !outside.ok) {
    simState.auto.state = "SAFE";
    return 60;
  }

  if (simState.auto.state === "SAFE") {
    simState.auto.state = "DEFAULT";
  }

  const dt = inside.temperature_c - outside.temperature_c;

  if (dt >= 2.0) {
    simState.auto.state = "REFRIGERAMENTO";
  } else if (dt <= 0.5) {
    simState.auto.state = "DEFAULT";
  }

  if (simState.auto.state === "DEFAULT") {
    return 30;
  }

  const dtCurve = Math.min(6.0, Math.max(2.0, dt));
  const pct = 40 + (dtCurve - 2.0) * 15.0;
  return clampPercent(Math.round(pct));
}

function computeTargetPercent() {
  if (simState.mode === "MANUAL") return clampPercent(simState.manual_percent);
  if (simState.mode === "ECO") return 30;
  if (simState.mode === "BOOST") return 100;
  return computeAuto1Percent();
}

function applyControl() {
  simState.fan.exhaust_percent = computeTargetPercent();
}

function buildStatePayload() {
  return {
    fan: { exhaust_percent: simState.fan.exhaust_percent },
    mode: simState.mode,
    auto: { state: simState.auto.state },
    sensors: {
      inside: {
        temperature_c: simState.sensors.inside.temperature_c,
        humidity_rh: simState.sensors.inside.humidity_rh,
        ok: simState.sensors.inside.ok
      },
      outside: {
        temperature_c: simState.sensors.outside.temperature_c,
        humidity_rh: simState.sensors.outside.humidity_rh,
        ok: simState.sensors.outside.ok
      }
    }
  };
}

function buildGrowsPayload() {
  return {
    active_grow_id: simState.active_grow_id,
    grows: simState.grows.map((g) => ({
      id: g.id,
      name: g.name,
      type: g.type,
      width_cm: g.width_cm,
      depth_cm: g.depth_cm,
      height_cm: g.height_cm,
      volume_m3: growVolumeM3(g)
    }))
  };
}

function buildPlantsPayload() {
  return {
    plants: simState.plants.map((p) => ({
      id: p.id,
      name: p.name,
      species_id: p.species_id,
      species_name: p.species_name,
      grow_id: p.grow_id,
      source_type: p.source_type,
      purpose: p.purpose,
      sex: p.sex,
      life_state: p.life_state,
      germination_start: p.germination_start,
      container_type: p.container_type,
      soil_type: p.soil_type,
      photoperiod_type: p.photoperiod_type,
      training_types: [...p.training_types],
      notes: p.notes
    }))
  };
}

function buildSpeciesPayload() {
  return {
    species: simState.species.map((s) => ({
      id: s.id,
      name: s.name,
      avg_veg_days: s.avg_veg_days,
      avg_flora_days: s.avg_flora_days,
      expected_height_indoor_cm: s.expected_height_indoor_cm,
      expected_height_outdoor_cm: s.expected_height_outdoor_cm,
      expected_yield_indoor_g_m2: s.expected_yield_indoor_g_m2,
      expected_yield_outdoor_g_m2: s.expected_yield_outdoor_g_m2,
      sativa_pct: s.sativa_pct,
      indica_pct: s.indica_pct,
      thc_pct: s.thc_pct,
      cbd_pct: s.cbd_pct
    }))
  };
}

function buildCyclesPayload() {
  return {
    cycles: simState.cycles.map((c) => ({
      id: c.id,
      name: c.name,
      grow_id: c.grow_id,
      plant_ids: [...c.plant_ids],
      start_datetime: c.start_datetime,
      phase: c.phase,
      duration_veg_days: c.duration_veg_days,
      duration_flora_days: c.duration_flora_days,
      stretch_assumed: c.stretch_assumed
    }))
  };
}

// ---------- Endpoints virtuais ----------
async function apiGetState() {
  await sleep(simState.latency_ms);
  if (!simState.online) throw new Error("offline");

  if (!simState.sensors_started) {
    simState.sensors_started = true;
  } else {
    updateSensorsSim();
  }

  applyControl();
  return buildStatePayload();
}

async function apiSetMode(mode) {
  await sleep(simState.latency_ms);
  if (simState.fail_mode) throw new Error("simulated failure");
  if (!simState.online) throw new Error("offline");

  const m = normalizeMode(mode);
  if (!m) return { ok: false, error: "invalid_mode" };

  simState.mode = m;
  applyControl();

  return {
    ok: true,
    mode: simState.mode,
    auto: { state: simState.auto.state },
    fan: { exhaust_percent: simState.fan.exhaust_percent }
  };
}

async function apiSetFan(percent) {
  await sleep(simState.latency_ms);
  if (simState.fail_mode) throw new Error("simulated failure");
  if (!simState.online) throw new Error("offline");

  if (simState.mode !== "MANUAL") {
    return {
      ok: false,
      error: "manual_only",
      mode: simState.mode,
      fan: { exhaust_percent: simState.fan.exhaust_percent }
    };
  }

  const p = clampPercent(Number(percent));
  simState.manual_percent = p;
  applyControl();

  return { ok: true, fan: { exhaust_percent: simState.fan.exhaust_percent } };
}

async function apiGetGrows() {
  await sleep(simState.latency_ms);
  if (simState.fail_mode) throw new Error("simulated failure");
  if (!simState.online) throw new Error("offline");
  return buildGrowsPayload();
}

async function apiCreateGrow(name, type, widthCm, depthCm, heightCm) {
  await sleep(simState.latency_ms);
  if (simState.fail_mode) throw new Error("simulated failure");
  if (!simState.online) throw new Error("offline");

  const growName = normalizeTrim(name);
  const growType = normalizeTrim(type).toLowerCase();
  const width = Number(widthCm);
  const depth = Number(depthCm);
  const height = Number(heightCm);

  if (!growName) return { ok: false, error: "invalid_name" };
  if (growType !== "indoor" && growType !== "outdoor") return { ok: false, error: "invalid_type" };
  if (!Number.isFinite(width) || !Number.isFinite(depth) || !Number.isFinite(height) || width <= 0 || depth <= 0 || height <= 0) {
    return { ok: false, error: "invalid_dimensions" };
  }

  simState.grows.push({
    id: nextId("g", simState.grows),
    name: growName,
    type: growType,
    width_cm: Math.round(width),
    depth_cm: Math.round(depth),
    height_cm: Math.round(height)
  });
  return { ok: true, ...buildGrowsPayload() };
}

async function apiDeleteGrow(id) {
  await sleep(simState.latency_ms);
  if (simState.fail_mode) throw new Error("simulated failure");
  if (!simState.online) throw new Error("offline");

  const growId = normalizeTrim(id);
  if (!growId) return { ok: false, error: "invalid_id" };
  if (simState.plants.some((p) => p.grow_id === growId)) return { ok: false, error: "grow_has_plants" };

  const idx = simState.grows.findIndex((g) => g.id === growId);
  if (idx < 0) return { ok: false, error: "not_found" };
  simState.grows.splice(idx, 1);

  if (simState.active_grow_id === growId) simState.active_grow_id = "";
  return { ok: true, ...buildGrowsPayload() };
}

async function apiSetActiveGrow(id) {
  await sleep(simState.latency_ms);
  if (simState.fail_mode) throw new Error("simulated failure");
  if (!simState.online) throw new Error("offline");

  const growId = normalizeTrim(id);
  if (!growId) {
    simState.active_grow_id = "";
    return { ok: true, ...buildGrowsPayload() };
  }

  if (!simState.grows.some((g) => g.id === growId)) return { ok: false, error: "invalid_grow_id" };
  simState.active_grow_id = growId;
  return { ok: true, ...buildGrowsPayload() };
}

async function apiGetPlants() {
  await sleep(simState.latency_ms);
  if (simState.fail_mode) throw new Error("simulated failure");
  if (!simState.online) throw new Error("offline");
  return buildPlantsPayload();
}

async function apiCreatePlant(
  name,
  speciesId,
  growId,
  sourceType,
  purpose,
  sex,
  lifeState,
  germinationStart,
  containerType,
  soilType,
  photoperiodType,
  trainingCsv,
  notes
) {
  await sleep(simState.latency_ms);
  if (simState.fail_mode) throw new Error("simulated failure");
  if (!simState.online) throw new Error("offline");

  const plantName = normalizeTrim(name);
  const spId = normalizeTrim(speciesId);
  const gId = normalizeTrim(growId);
  const source = normalizeTrim(sourceType);
  const target = normalizeTrim(purpose);
  const plantSex = normalizeTrim(sex);
  const life = normalizeTrim(lifeState);
  const germStart = normalizeTrim(germinationStart);
  const container = normalizeTrim(containerType);
  const soil = normalizeTrim(soilType);
  const photoperiod = normalizeTrim(photoperiodType);
  const plantNotes = normalizeTrim(notes);
  let trainings = parseCsvIds(trainingCsv);

  if (!plantName) return { ok: false, error: "invalid_name" };
  if (!simState.species.some((s) => s.id === spId)) return { ok: false, error: "invalid_species" };
  if (!simState.grows.some((g) => g.id === gId)) return { ok: false, error: "invalid_grow_id" };
  if (!["semente", "clone", "planta_estabelecida", "planta_pronta"].includes(source)) return { ok: false, error: "invalid_source_type" };
  if (!["colheita", "reproducao", "mae"].includes(target)) return { ok: false, error: "invalid_purpose" };
  if (!["femea", "macho", "indefinido", "hermafrodita"].includes(plantSex)) return { ok: false, error: "invalid_sex" };
  if (!["nao_germinada", "germinacao_iniciada"].includes(life)) return { ok: false, error: "invalid_life_state" };
  if (life === "germinacao_iniciada" && !germStart) return { ok: false, error: "missing_germination_start" };
  const allowedContainers = [
    "agua", "papel_toalha", "jiffy", "vaso_200ml", "vaso_500ml", "vaso_1l", "vaso_3l",
    "vaso_5l", "vaso_7l", "vaso_11l", "vaso_15l", "hidroponico"
  ];
  let containerFinal = "";
  let soilFinal = "";
  if (life === "germinacao_iniciada") {
    if (!allowedContainers.includes(container)) return { ok: false, error: "invalid_container_type" };
    containerFinal = container;
    if (plantContainerSupportsSoil(containerFinal)) {
      if (!["organico", "mineral"].includes(soil)) return { ok: false, error: "invalid_soil_type" };
      soilFinal = soil;
    }
  }
  if (!["autoflower", "fotoperiodo"].includes(photoperiod)) return { ok: false, error: "invalid_photoperiod" };

  const validTraining = new Set(["none", "indefinido", "lst", "topping", "fim", "scrog", "sog", "supercrop", "mainline"]);
  trainings = trainings.filter((t) => validTraining.has(t));
  if (trainings.includes("none")) trainings = ["indefinido"];
  if (trainings.includes("indefinido") && trainings.length > 1) trainings = trainings.filter((t) => t !== "indefinido");
  if (!trainings.length) trainings = ["indefinido"];
  const speciesItem = simState.species.find((s) => s.id === spId);

  simState.plants.push({
    id: nextId("p", simState.plants),
    name: plantName,
    species_id: spId,
    species_name: speciesItem ? speciesItem.name : "",
    grow_id: gId,
    source_type: source,
    purpose: target,
    sex: plantSex,
    life_state: life,
    germination_start: germStart,
    container_type: containerFinal,
    soil_type: soilFinal,
    photoperiod_type: photoperiod,
    training_types: trainings,
    notes: plantNotes
  });
  return { ok: true, ...buildPlantsPayload() };
}

async function apiDeletePlant(id) {
  await sleep(simState.latency_ms);
  if (simState.fail_mode) throw new Error("simulated failure");
  if (!simState.online) throw new Error("offline");

  const plantId = normalizeTrim(id);
  const idx = simState.plants.findIndex((p) => p.id === plantId);
  if (idx < 0) return { ok: false, error: "not_found" };
  simState.plants.splice(idx, 1);
  return { ok: true, ...buildPlantsPayload() };
}

async function apiGetSpecies() {
  await sleep(simState.latency_ms);
  if (simState.fail_mode) throw new Error("simulated failure");
  if (!simState.online) throw new Error("offline");
  return buildSpeciesPayload();
}

async function apiCreateSpecies(
  name,
  vegDays,
  floraDays,
  indoorHeightCm,
  outdoorHeightCm,
  indoorYield,
  outdoorYield,
  sativaPct,
  indicaPct,
  thcPct,
  cbdPct
) {
  await sleep(simState.latency_ms);
  if (simState.fail_mode) throw new Error("simulated failure");
  if (!simState.online) throw new Error("offline");

  const speciesName = normalizeTrim(name);
  const veg = Number(vegDays || 0);
  const flora = Number(floraDays || 0);
  const indoorHeight = Number(indoorHeightCm || 0);
  const outdoorHeight = Number(outdoorHeightCm || 0);
  const indoorYieldVal = Number(indoorYield || 0);
  const outdoorYieldVal = Number(outdoorYield || 0);
  const sativa = Number(sativaPct || 0);
  const indica = Number(indicaPct || 0);
  const thc = Number(thcPct || 0);
  const cbd = Number(cbdPct || 0);

  if (!speciesName) return { ok: false, error: "invalid_name" };
  if (!Number.isFinite(veg) || !Number.isFinite(flora) || veg < 0 || flora < 0) return { ok: false, error: "invalid_duration" };
  if (!Number.isFinite(indoorHeight) || !Number.isFinite(outdoorHeight) || indoorHeight < 0 || outdoorHeight < 0) return { ok: false, error: "invalid_height" };
  if (!Number.isFinite(indoorYieldVal) || !Number.isFinite(outdoorYieldVal) || indoorYieldVal < 0 || outdoorYieldVal < 0) return { ok: false, error: "invalid_yield" };
  if (!Number.isFinite(sativa) || !Number.isFinite(indica) || sativa < 0 || indica < 0) return { ok: false, error: "invalid_genetics" };
  if (!Number.isFinite(thc) || !Number.isFinite(cbd) || thc < 0 || cbd < 0) return { ok: false, error: "invalid_cannabinoids" };

  simState.species.push({
    id: nextId("s", simState.species),
    name: speciesName,
    avg_veg_days: Math.round(veg),
    avg_flora_days: Math.round(flora),
    expected_height_indoor_cm: Math.round(indoorHeight),
    expected_height_outdoor_cm: Math.round(outdoorHeight),
    expected_yield_indoor_g_m2: Math.round(indoorYieldVal),
    expected_yield_outdoor_g_m2: Math.round(outdoorYieldVal),
    sativa_pct: Number(sativa.toFixed(1)),
    indica_pct: Number(indica.toFixed(1)),
    thc_pct: Number(thc.toFixed(1)),
    cbd_pct: Number(cbd.toFixed(1))
  });
  return { ok: true, ...buildSpeciesPayload() };
}

async function apiUpdateSpecies(
  id,
  name,
  vegDays,
  floraDays,
  indoorHeightCm,
  outdoorHeightCm,
  indoorYield,
  outdoorYield,
  sativaPct,
  indicaPct,
  thcPct,
  cbdPct
) {
  await sleep(simState.latency_ms);
  if (simState.fail_mode) throw new Error("simulated failure");
  if (!simState.online) throw new Error("offline");

  const speciesId = normalizeTrim(id);
  const speciesName = normalizeTrim(name);
  const veg = Number(vegDays || 0);
  const flora = Number(floraDays || 0);
  const indoorHeight = Number(indoorHeightCm || 0);
  const outdoorHeight = Number(outdoorHeightCm || 0);
  const indoorYieldVal = Number(indoorYield || 0);
  const outdoorYieldVal = Number(outdoorYield || 0);
  const sativa = Number(sativaPct || 0);
  const indica = Number(indicaPct || 0);
  const thc = Number(thcPct || 0);
  const cbd = Number(cbdPct || 0);

  if (!speciesId) return { ok: false, error: "invalid_id" };
  if (!speciesName) return { ok: false, error: "invalid_name" };
  if (!Number.isFinite(veg) || !Number.isFinite(flora) || veg < 0 || flora < 0) return { ok: false, error: "invalid_duration" };
  if (!Number.isFinite(indoorHeight) || !Number.isFinite(outdoorHeight) || indoorHeight < 0 || outdoorHeight < 0) return { ok: false, error: "invalid_height" };
  if (!Number.isFinite(indoorYieldVal) || !Number.isFinite(outdoorYieldVal) || indoorYieldVal < 0 || outdoorYieldVal < 0) return { ok: false, error: "invalid_yield" };
  if (!Number.isFinite(sativa) || !Number.isFinite(indica) || sativa < 0 || indica < 0) return { ok: false, error: "invalid_genetics" };
  if (!Number.isFinite(thc) || !Number.isFinite(cbd) || thc < 0 || cbd < 0) return { ok: false, error: "invalid_cannabinoids" };

  const item = simState.species.find((s) => s.id === speciesId);
  if (!item) return { ok: false, error: "not_found" };

  item.name = speciesName;
  item.avg_veg_days = Math.round(veg);
  item.avg_flora_days = Math.round(flora);
  item.expected_height_indoor_cm = Math.round(indoorHeight);
  item.expected_height_outdoor_cm = Math.round(outdoorHeight);
  item.expected_yield_indoor_g_m2 = Math.round(indoorYieldVal);
  item.expected_yield_outdoor_g_m2 = Math.round(outdoorYieldVal);
  item.sativa_pct = Number(sativa.toFixed(1));
  item.indica_pct = Number(indica.toFixed(1));
  item.thc_pct = Number(thc.toFixed(1));
  item.cbd_pct = Number(cbd.toFixed(1));
  return { ok: true, ...buildSpeciesPayload() };
}

async function apiDeleteSpecies(id) {
  await sleep(simState.latency_ms);
  if (simState.fail_mode) throw new Error("simulated failure");
  if (!simState.online) throw new Error("offline");

  const speciesId = normalizeTrim(id);
  if (simState.plants.some((p) => p.species_id === speciesId)) return { ok: false, error: "species_has_plants" };
  const idx = simState.species.findIndex((s) => s.id === speciesId);
  if (idx < 0) return { ok: false, error: "not_found" };
  simState.species.splice(idx, 1);
  return { ok: true, ...buildSpeciesPayload() };
}

async function apiGetCycles() {
  await sleep(simState.latency_ms);
  if (simState.fail_mode) throw new Error("simulated failure");
  if (!simState.online) throw new Error("offline");
  return buildCyclesPayload();
}

async function apiCreateCycle(name, growId, plantIdsCsv, startDatetime, phase, vegDays, floraDays, stretchAssumed) {
  await sleep(simState.latency_ms);
  if (simState.fail_mode) throw new Error("simulated failure");
  if (!simState.online) throw new Error("offline");

  const cycleName = normalizeTrim(name);
  const gId = normalizeTrim(growId);
  const start = normalizeTrim(startDatetime);
  const cyclePhase = normalizeTrim(phase) || "PLANNED";
  const plantIds = parseCsvIds(plantIdsCsv);
  const veg = Number(vegDays || 0);
  const flora = Number(floraDays || 0);
  const stretch = Number(stretchAssumed || 0);

  if (!cycleName) return { ok: false, error: "invalid_name" };
  if (!simState.grows.some((g) => g.id === gId)) return { ok: false, error: "invalid_grow_id" };
  if (!plantIds.length) return { ok: false, error: "missing_plants" };
  if (!plantIds.every((id) => simState.plants.some((p) => p.id === id))) return { ok: false, error: "invalid_plant_id" };
  if (!start) return { ok: false, error: "invalid_start_datetime" };
  if (!Number.isFinite(veg) || !Number.isFinite(flora) || veg < 0 || flora < 0) return { ok: false, error: "invalid_duration" };
  if (!Number.isFinite(stretch) || stretch < 0) return { ok: false, error: "invalid_stretch" };

  simState.cycles.push({
    id: nextId("c", simState.cycles),
    name: cycleName,
    grow_id: gId,
    plant_ids: plantIds,
    start_datetime: start,
    phase: cyclePhase,
    duration_veg_days: Math.round(veg),
    duration_flora_days: Math.round(flora),
    stretch_assumed: Number(stretch.toFixed(2))
  });
  return { ok: true, ...buildCyclesPayload() };
}

async function apiUpdateCycle(id, name, growId, plantIdsCsv, startDatetime, phase, vegDays, floraDays, stretchAssumed) {
  await sleep(simState.latency_ms);
  if (simState.fail_mode) throw new Error("simulated failure");
  if (!simState.online) throw new Error("offline");

  const cycleId = normalizeTrim(id);
  const cycleName = normalizeTrim(name);
  const gId = normalizeTrim(growId);
  const start = normalizeTrim(startDatetime);
  const cyclePhase = normalizeTrim(phase) || "PLANNED";
  const plantIds = parseCsvIds(plantIdsCsv);
  const veg = Number(vegDays || 0);
  const flora = Number(floraDays || 0);
  const stretch = Number(stretchAssumed || 0);

  if (!cycleId) return { ok: false, error: "invalid_id" };
  if (!cycleName) return { ok: false, error: "invalid_name" };
  if (!simState.grows.some((g) => g.id === gId)) return { ok: false, error: "invalid_grow_id" };
  if (!plantIds.length) return { ok: false, error: "missing_plants" };
  if (!plantIds.every((pid) => simState.plants.some((p) => p.id === pid))) return { ok: false, error: "invalid_plant_id" };
  if (!start) return { ok: false, error: "invalid_start_datetime" };
  if (!Number.isFinite(veg) || !Number.isFinite(flora) || veg < 0 || flora < 0) return { ok: false, error: "invalid_duration" };
  if (!Number.isFinite(stretch) || stretch < 0) return { ok: false, error: "invalid_stretch" };

  const item = simState.cycles.find((c) => c.id === cycleId);
  if (!item) return { ok: false, error: "not_found" };

  item.name = cycleName;
  item.grow_id = gId;
  item.plant_ids = plantIds;
  item.start_datetime = start;
  item.phase = cyclePhase;
  item.duration_veg_days = Math.round(veg);
  item.duration_flora_days = Math.round(flora);
  item.stretch_assumed = Number(stretch.toFixed(2));

  return { ok: true, ...buildCyclesPayload() };
}

async function apiDeleteCycle(id) {
  await sleep(simState.latency_ms);
  if (simState.fail_mode) throw new Error("simulated failure");
  if (!simState.online) throw new Error("offline");

  const cycleId = normalizeTrim(id);
  const idx = simState.cycles.findIndex((c) => c.id === cycleId);
  if (idx < 0) return { ok: false, error: "not_found" };
  simState.cycles.splice(idx, 1);
  return { ok: true, ...buildCyclesPayload() };
}

// ================================
// UI
// ================================
const screens = {
  home: document.getElementById("home"),
  fan: document.getElementById("fan"),
  grows: document.getElementById("grows"),
  growDetail: document.getElementById("growDetail"),
  plants: document.getElementById("plants"),
  cycles: document.getElementById("cycles"),
  cycleStep1: document.getElementById("cycleStep1"),
  cycleStep2: document.getElementById("cycleStep2")
};

const navButtons = document.querySelectorAll(".nav-btn");
const fanIcon = document.getElementById("fanIcon");
const btnBack = document.getElementById("btnBack");
const homeStatus = document.getElementById("homeStatus");
const homeMode = document.getElementById("homeMode");
const connStatus = document.getElementById("connStatus");
const fanValue = document.getElementById("fanValue");
const fanSlider = document.getElementById("fanSlider");
const homeFanSlider = document.getElementById("homeFanSlider");
const fanHint = document.getElementById("fanHint");
const homeFanHint = document.getElementById("homeFanHint");
const msg = document.getElementById("msg");
const toggleFail = document.getElementById("toggleFail");

const insideTempEl = document.getElementById("insideTemp");
const insideHumEl = document.getElementById("insideHum");
const outsideTempEl = document.getElementById("outsideTemp");
const outsideHumEl = document.getElementById("outsideHum");
const insideCardEl = document.getElementById("insideCard");
const outsideCardEl = document.getElementById("outsideCard");

const modeButtons = {
  MANUAL: document.getElementById("btnModeManual"),
  ECO: document.getElementById("btnModeEco"),
  BOOST: document.getElementById("btnModeBoost"),
  AUTO: document.getElementById("btnModeAuto")
};

const growsList = document.getElementById("growsList");
const growName = document.getElementById("growName");
const growTypeSegment = document.getElementById("growTypeSegment");
const growType = document.getElementById("growType");
const growSubtype = document.getElementById("growSubtype");
const growWidth = document.getElementById("growWidth");
const growDepth = document.getElementById("growDepth");
const growHeight = document.getElementById("growHeight");
const btnCreateGrow = document.getElementById("btnCreateGrow");
const btnRefreshGrows = document.getElementById("btnRefreshGrows");
const growsMsg = document.getElementById("growsMsg");
const btnGrowDetailBack = document.getElementById("btnGrowDetailBack");
const growDetailInfo = document.getElementById("growDetailInfo");
const growPlantsList = document.getElementById("growPlantsList");
const growToolsList = document.getElementById("growToolsList");
const btnGrowToolToggle = document.getElementById("btnGrowToolToggle");
const growToolFormCard = document.getElementById("growToolFormCard");
const growToolKind = document.getElementById("growToolKind");
const btnGrowToolAdd = document.getElementById("btnGrowToolAdd");
const growToolMsg = document.getElementById("growToolMsg");
const toolFormLight = document.getElementById("toolFormLight");
const toolFormExhaust = document.getElementById("toolFormExhaust");
const toolFormFilter = document.getElementById("toolFormFilter");
const toolFormInternalVent = document.getElementById("toolFormInternalVent");
const toolFormPassiveIntake = document.getElementById("toolFormPassiveIntake");
const toolLightType = document.getElementById("toolLightType");
const toolLightPowerW = document.getElementById("toolLightPowerW");
const toolLightHeightMode = document.getElementById("toolLightHeightMode");
const toolLightMinTopCm = document.getElementById("toolLightMinTopCm");
const toolLightMaxDropCm = document.getElementById("toolLightMaxDropCm");
const toolLightFullDrop = document.getElementById("toolLightFullDrop");
const toolExhaustModel = document.getElementById("toolExhaustModel");
const toolExhaustType = document.getElementById("toolExhaustType");
const toolExhaustFlow = document.getElementById("toolExhaustFlow");
const toolExhaustPowerW = document.getElementById("toolExhaustPowerW");
const toolExhaustDuctDiameterMm = document.getElementById("toolExhaustDuctDiameterMm");
const toolExhaustMaxDuctLengthCm = document.getElementById("toolExhaustMaxDuctLengthCm");
const toolExhaustFullLength = document.getElementById("toolExhaustFullLength");
const toolFilterType = document.getElementById("toolFilterType");
const toolFilterModel = document.getElementById("toolFilterModel");
const toolFilterDiameterMm = document.getElementById("toolFilterDiameterMm");
const toolFilterLengthCm = document.getElementById("toolFilterLengthCm");
const toolFilterFlowM3h = document.getElementById("toolFilterFlowM3h");
const toolInternalVentCount = document.getElementById("toolInternalVentCount");
const toolInternalVentSizeCm = document.getElementById("toolInternalVentSizeCm");
const toolInternalVentPowerW = document.getElementById("toolInternalVentPowerW");
const toolInternalVentMaxHeightCm = document.getElementById("toolInternalVentMaxHeightCm");
const toolInternalVentFullHeight = document.getElementById("toolInternalVentFullHeight");
const toolPassiveMode = document.getElementById("toolPassiveMode");
const toolPassiveWidthCm = document.getElementById("toolPassiveWidthCm");
const toolPassiveHeightCm = document.getElementById("toolPassiveHeightCm");
const toolPassiveAreaCm2 = document.getElementById("toolPassiveAreaCm2");
const toolPassiveFullHeight = document.getElementById("toolPassiveFullHeight");
const toolPassiveNotes = document.getElementById("toolPassiveNotes");

const plantsList = document.getElementById("plantsList");
const speciesList = document.getElementById("speciesList");
const speciesPanelBody = document.getElementById("speciesPanelBody");
const btnSpeciesPanelToggle = document.getElementById("btnSpeciesPanelToggle");
const btnSpeciesToggle = document.getElementById("btnSpeciesToggle");
const speciesFormCard = document.getElementById("speciesFormCard");
const speciesNameInput = document.getElementById("speciesNameInput");
const speciesVegDays = document.getElementById("speciesVegDays");
const speciesFloraDays = document.getElementById("speciesFloraDays");
const speciesIndoorHeight = document.getElementById("speciesIndoorHeight");
const speciesOutdoorHeight = document.getElementById("speciesOutdoorHeight");
const speciesIndoorYield = document.getElementById("speciesIndoorYield");
const speciesOutdoorYield = document.getElementById("speciesOutdoorYield");
const speciesSativaPct = document.getElementById("speciesSativaPct");
const speciesIndicaPct = document.getElementById("speciesIndicaPct");
const speciesThcPct = document.getElementById("speciesThcPct");
const speciesCbdPct = document.getElementById("speciesCbdPct");
const btnCreateSpecies = document.getElementById("btnCreateSpecies");
const speciesMsg = document.getElementById("speciesMsg");
const plantName = document.getElementById("plantName");
const plantSpeciesId = document.getElementById("plantSpeciesId");
const plantGrow = document.getElementById("plantGrow");
const plantSource = document.getElementById("plantSource");
const plantPurpose = document.getElementById("plantPurpose");
const plantSex = document.getElementById("plantSex");
const plantLifeState = document.getElementById("plantLifeState");
const plantContainer = document.getElementById("plantContainer");
const plantSoilType = document.getElementById("plantSoilType");
const plantGermStart = document.getElementById("plantGermStart");
const plantPhotoperiod = document.getElementById("plantPhotoperiod");
const plantTrainingDetails = document.getElementById("plantTrainingDetails");
const plantTrainingSummary = document.getElementById("plantTrainingSummary");
const plantTrainingChecks = document.querySelectorAll("#plantTrainingDetails input[data-training]");
const plantNotes = document.getElementById("plantNotes");
const btnCreatePlant = document.getElementById("btnCreatePlant");
const btnRefreshPlants = document.getElementById("btnRefreshPlants");
const plantsMsg = document.getElementById("plantsMsg");
const cyclesList = document.getElementById("cyclesList");
const btnRefreshCycles = document.getElementById("btnRefreshCycles");
const btnNewCycle = document.getElementById("btnNewCycle");
const cyclesMsg = document.getElementById("cyclesMsg");
const btnCycleBack = document.getElementById("btnCycleBack");
const btnCycleCreateGrowStart = document.getElementById("btnCycleCreateGrowStart");
const btnCycleRefreshGrows = document.getElementById("btnCycleRefreshGrows");
const btnCycleNextToPlants = document.getElementById("btnCycleNextToPlants");
const cycleGrowList = document.getElementById("cycleGrowList");
const cycleWizardState = document.getElementById("cycleWizardState");
const cycleWizardMsg = document.getElementById("cycleWizardMsg");
const btnCycleStep2Back = document.getElementById("btnCycleStep2Back");
const cyclePlantList = document.getElementById("cyclePlantList");
const cyclePlantState = document.getElementById("cyclePlantState");
const cyclePlantMsg = document.getElementById("cyclePlantMsg");

let lastApplied = null;
let currentMode = "MANUAL";
let currentAutoState = "DEFAULT";
let growsCache = [];
let plantsCache = [];
let speciesCache = [];
let cyclesCache = [];
let activeGrowId = "";
let cycleDraft = { grow_ids: [], plant_ids: [] };
let cycleCreateFlowActive = false;
let currentGrowDetailId = "";
let editingGrowToolId = "";
let expandedSpeciesId = "";
let speciesPanelExpanded = false;

function loadCycleDraft() {
  try {
    const raw = localStorage.getItem("conex_cycle_draft_v1");
    if (!raw) return;
    const parsed = JSON.parse(raw);
    if (parsed && typeof parsed === "object") {
      if (Array.isArray(parsed.grow_ids)) {
        const growIds = parsed.grow_ids.map((id) => String(id)).filter((id) => id);
        const plantIds = Array.isArray(parsed.plant_ids) ? parsed.plant_ids.map((id) => String(id)).filter((id) => id) : [];
        cycleDraft = { grow_ids: growIds, plant_ids: plantIds };
        return;
      }
      if (parsed.grow_id) {
        cycleDraft = { grow_ids: [String(parsed.grow_id)], plant_ids: [] };
      }
    }
  } catch (_) {}
}

function saveCycleDraft() {
  try {
    localStorage.setItem("conex_cycle_draft_v1", JSON.stringify(cycleDraft));
  } catch (_) {}
}

function loadGrowSubtypes() {
  try { return JSON.parse(localStorage.getItem("conex_grow_subtypes_v1") || "{}") || {}; } catch (_) { return {}; }
}

function saveGrowSubtypes(map) {
  try { localStorage.setItem("conex_grow_subtypes_v1", JSON.stringify(map)); } catch (_) {}
}

function loadGrowTools() {
  try { return JSON.parse(localStorage.getItem("conex_grow_tools_v1") || "{}") || {}; } catch (_) { return {}; }
}

function saveGrowTools(map) {
  try { localStorage.setItem("conex_grow_tools_v1", JSON.stringify(map)); } catch (_) {}
}

const subtypeOptions = {
  indoor: [{ v: "grow", l: "Tenda de cultivo" }, { v: "armario", l: "Armario" }, { v: "pc_grow", l: "PC grow" }],
  outdoor: [{ v: "campo_aberto", l: "Campo aberto" }, { v: "entre_plantas", l: "Em meio a outras plantas" }, { v: "janela", l: "Janela" }, { v: "varanda", l: "Varanda" }]
};

function growTypeLabel(type) {
  return type === "outdoor" ? "Outdoor" : "Indoor";
}

function renderSubtypeOptions(selected) {
  const type = (growType.value === "outdoor") ? "outdoor" : "indoor";
  const options = subtypeOptions[type] || [];
  growSubtype.innerHTML = "";
  for (const o of options) {
    const el = document.createElement("option");
    el.value = o.v;
    el.textContent = o.l;
    growSubtype.appendChild(el);
  }
  if (selected && options.some((o) => o.v === selected)) growSubtype.value = selected;
}

function setGrowType(type) {
  const normalized = (type === "outdoor") ? "outdoor" : "indoor";
  growType.value = normalized;
  const buttons = growTypeSegment ? growTypeSegment.querySelectorAll("[data-grow-type]") : [];
  buttons.forEach((btn) => {
    btn.classList.toggle("active", btn.getAttribute("data-grow-type") === normalized);
  });
  renderSubtypeOptions();
}

function currentGrowSubtypeMap() {
  return loadGrowSubtypes();
}

function currentGrowToolsMap() {
  return loadGrowTools();
}

function setNavActive(screenName) {
  navButtons.forEach((b) => b.classList.toggle("active", b.dataset.screen === screenName));
}

function show(screenName) {
  for (const k in screens) screens[k].classList.remove("active");
  screens[screenName].classList.add("active");
  setNavActive(
    screenName === "fan" ? "home"
      : ((screenName === "cycleStep1" || screenName === "cycleStep2")
        ? "cycles"
        : (screenName === "growDetail" ? "grows" : screenName))
  );
}

function normalizeAutoState(state) {
  if (!state) return "DEFAULT";
  const s = String(state).toUpperCase();
  if (s === "DEFAULT" || s === "REFRIGERAMENTO" || s === "SAFE") return s;
  return "DEFAULT";
}

function getTempClass(tempC) {
  const t = Number(tempC);
  if (!Number.isFinite(t)) return "";
  if (t <= 18) return "temp-cold";
  if (t <= 26) return "temp-fresh";
  if (t <= 30) return "temp-warm";
  return "temp-hot";
}

function setSensorCardTempClass(cardEl, tempClass) {
  cardEl.classList.remove("temp-cold", "temp-fresh", "temp-warm", "temp-hot");
  if (tempClass) cardEl.classList.add(tempClass);
}

function uiSetFan(percent) {
  fanValue.textContent = percent + "%";
  fanSlider.value = percent;
  homeFanSlider.value = percent;
  homeStatus.textContent = "Velocidade: " + percent + "%";
}

function uiSetSensor(tempEl, humEl, cardEl, reading) {
  if (!reading || !reading.ok) {
    tempEl.textContent = "--.-\u00B0C";
    humEl.textContent = "--%";
    setSensorCardTempClass(cardEl, "");
    return;
  }

  const temp = Number(reading.temperature_c);
  tempEl.textContent = temp.toFixed(1) + "\u00B0C";
  humEl.textContent = Math.round(Number(reading.humidity_rh)) + "%";
  setSensorCardTempClass(cardEl, getTempClass(temp));
}

function uiSetSensors(sensors) {
  uiSetSensor(insideTempEl, insideHumEl, insideCardEl, sensors?.inside);
  uiSetSensor(outsideTempEl, outsideHumEl, outsideCardEl, sensors?.outside);
}

function uiSetMode(mode, autoState) {
  currentMode = normalizeMode(mode) || "MANUAL";
  currentAutoState = normalizeAutoState(autoState);

  for (const key in modeButtons) {
    modeButtons[key].classList.toggle("active", key === currentMode);
  }

  const isManual = currentMode === "MANUAL";
  fanSlider.disabled = !isManual;
  homeFanSlider.disabled = !isManual;

  fanHint.textContent = isManual
    ? "Ajuste a velocidade (aplica ao soltar)."
    : "Slider ativo apenas em modo Manual.";

  homeFanHint.textContent = isManual
    ? "Ajuste rapido (aplica ao soltar)."
    : "Slider rapido ativo apenas em modo Manual.";

  if (currentMode === "AUTO") {
    homeMode.textContent = "Modo: AUTO (" + currentAutoState + ")";
  } else {
    homeMode.textContent = "Modo: " + currentMode;
  }
}

function renderGrows() {
  const subtypeMap = loadGrowSubtypes();
  if (!growsCache.length) {
    growsList.innerHTML = '<div class="muted">Nenhum ambiente cadastrado.</div>';
  } else {
    growsList.innerHTML = "";
    for (const g of growsCache) {
      const isActive = activeGrowId === g.id;
      const subtype = subtypeMap[g.id] || "--";
      const item = document.createElement("div");
      item.className = "list-item";
      item.innerHTML = `
        <div class="list-title">${g.name} ${isActive ? "(ativo)" : ""}</div>
        <div class="muted">ID: ${g.id} | ${growTypeLabel(g.type)} / ${subtype} | ${g.width_cm}x${g.depth_cm}x${g.height_cm} cm | ${Number(g.volume_m3).toFixed(3)} m3</div>
        <div class="list-actions">
          <button data-grow-open="${g.id}">Abrir</button>
          ${isActive ? "" : `<button data-grow-active="${g.id}">Ativar</button>`}
          <button data-grow-delete="${g.id}">Excluir</button>
        </div>
      `;
      growsList.appendChild(item);
    }
  }

  plantGrow.innerHTML = "";
  for (const g of growsCache) {
    const opt = document.createElement("option");
    opt.value = g.id;
    opt.textContent = `${g.name} (${g.id})`;
    plantGrow.appendChild(opt);
  }
  renderCycleGrowList();
  renderCyclePlantList();
}

function renderPlants() {
  if (!plantsCache.length) {
    plantsList.innerHTML = '<div class="muted">Nenhuma planta cadastrada.</div>';
    return;
  }

  plantsList.innerHTML = "";
  for (const p of plantsCache) {
    const growName = growsCache.find((g) => g.id === p.grow_id)?.name || p.grow_id;
    const sourceLabel = ({ semente: "Semente", clone: "Clone", planta_estabelecida: "Planta estabelecida", planta_pronta: "Planta estabelecida" })[p.source_type] || p.source_type || "--";
    const purposeLabel = ({ colheita: "Colheita", reproducao: "Reproducao", mae: "Planta mae" })[p.purpose] || p.purpose || "--";
    const sexLabel = ({ femea: "Femea", macho: "Macho", indefinido: "Indefinido", hermafrodita: "Hermafrodita" })[p.sex] || p.sex || "--";
    const lifeLabel = ({ nao_germinada: "Nao germinada", germinacao_iniciada: "Germinacao iniciada" })[p.life_state] || p.life_state || "--";
    const containerLabel = ({
      agua: "Recipiente com agua",
      papel_toalha: "Papel toalha",
      jiffy: "Celula jiffy",
      vaso_200ml: "Vaso 200 ml",
      vaso_500ml: "Vaso 500 ml",
      vaso_1l: "Vaso 1 L",
      vaso_3l: "Vaso 3 L",
      vaso_5l: "Vaso 5 L",
      vaso_7l: "Vaso 7 L",
      vaso_11l: "Vaso 11 L",
      vaso_15l: "Vaso 15 L",
      hidroponico: "Hidroponico"
    })[p.container_type] || "--";
    const soilLabel = ({ organico: "Organico", mineral: "Mineral" })[p.soil_type] || "--";
    const photoperiodLabel = ({ autoflower: "Autoflorecente", fotoperiodo: "Fotoperiodo" })[p.photoperiod_type] || p.photoperiod_type || "--";
    const trainingNames = {
      indefinido: "Indefinido",
      lst: "LST",
      topping: "Topping",
      fim: "FIM",
      scrog: "SCROG",
      sog: "SOG",
      supercrop: "Supercrop",
      mainline: "Mainline"
    };
    const trainings = (Array.isArray(p.training_types) && p.training_types.length)
      ? p.training_types.map((t) => trainingNames[t] || t).join(", ")
      : "Sem treinamento";
    const item = document.createElement("div");
    item.className = "list-item";
    item.innerHTML = `
      <div class="list-title">${p.name}</div>
      <div class="muted">Especie: ${p.species_name || "--"} | Ambiente: ${growName}</div>
      <div class="muted">${sourceLabel} | ${purposeLabel} | ${sexLabel}</div>
      <div class="muted">${lifeLabel} ${p.germination_start ? `(${p.germination_start})` : ""} | ${photoperiodLabel}</div>
      <div class="muted">Recipiente: ${containerLabel}${p.soil_type ? ` | Solo: ${soilLabel}` : ""}</div>
      <div class="muted">Treinamento: ${trainings}</div>
      ${p.notes ? `<div class="muted">Obs: ${p.notes}</div>` : ""}
      <div class="list-actions">
        <button data-plant-delete="${p.id}">Excluir</button>
      </div>
    `;
    plantsList.appendChild(item);
  }
}

function renderSpecies() {
  if (!speciesCache.length) {
    speciesList.innerHTML = '<div class="muted">Nenhuma especie cadastrada.</div>';
    return;
  }

  speciesList.innerHTML = "";
  for (const s of speciesCache) {
    const isExpanded = expandedSpeciesId === s.id;
    const item = document.createElement("div");
    item.className = "list-item species-item";
    item.setAttribute("data-species-item", s.id);
    item.innerHTML = `
      <div class="list-title">${s.name} ${isExpanded ? "(aberto)" : ""}</div>
      ${isExpanded ? `
        <div class="kpi-grid">
          <div class="kpi">Veg: ${s.avg_veg_days} dias</div>
          <div class="kpi">Flora: ${s.avg_flora_days} dias</div>
          <div class="kpi">Altura IN: ${s.expected_height_indoor_cm} cm</div>
          <div class="kpi">Altura OUT: ${s.expected_height_outdoor_cm} cm</div>
          <div class="kpi">Rend. IN: ${s.expected_yield_indoor_g_m2} g/m2</div>
          <div class="kpi">Rend. OUT: ${s.expected_yield_outdoor_g_m2} g/m2</div>
          <div class="kpi">Sativa/Indica: ${s.sativa_pct}% / ${s.indica_pct}%</div>
          <div class="kpi">THC/CBD: ${s.thc_pct}% / ${s.cbd_pct}%</div>
        </div>
      ` : ""}
    `;
    speciesList.appendChild(item);
  }
}

function setSpeciesPanelExpanded(expanded) {
  speciesPanelExpanded = !!expanded;
  speciesPanelBody.style.display = speciesPanelExpanded ? "block" : "none";
  btnSpeciesToggle.style.display = speciesPanelExpanded ? "inline-block" : "none";
  btnSpeciesPanelToggle.textContent = speciesPanelExpanded ? "Ocultar" : "Mostrar";
  if (!speciesPanelExpanded) {
    speciesFormCard.style.display = "none";
    speciesMsg.textContent = "";
  }
}

function renderSpeciesOptions() {
  plantSpeciesId.innerHTML = "";
  for (const s of speciesCache) {
    const opt = document.createElement("option");
    opt.value = s.id;
    opt.textContent = `${s.name} (${s.id})`;
    plantSpeciesId.appendChild(opt);
  }
}

function getSelectedTrainings() {
  return Array.from(plantTrainingChecks)
    .filter((cb) => cb.checked)
    .map((cb) => cb.getAttribute("data-training") || "")
    .filter((v) => v);
}

function setSelectedTrainings(values) {
  const set = new Set(values || []);
  plantTrainingChecks.forEach((cb) => {
    const key = cb.getAttribute("data-training") || "";
    cb.checked = set.has(key);
  });
}

function syncTrainingSelection(changedKey) {
  const indef = Array.from(plantTrainingChecks).find((cb) => cb.getAttribute("data-training") === "indefinido");
  if (!indef) return;

  if (changedKey === "indefinido" && indef.checked) {
    plantTrainingChecks.forEach((cb) => {
      if (cb !== indef) cb.checked = false;
    });
  } else if (changedKey && changedKey !== "indefinido") {
    if (Array.from(plantTrainingChecks).some((cb) => cb !== indef && cb.checked)) {
      indef.checked = false;
    }
  }

  const selected = getSelectedTrainings();
  if (!selected.length) {
    indef.checked = true;
  }

  updateTrainingSummary();
}

function updateTrainingSummary() {
  const labels = {
    indefinido: "indefinido",
    lst: "LST",
    topping: "Topping",
    fim: "FIM",
    scrog: "SCROG",
    sog: "SOG",
    supercrop: "Supercrop",
    mainline: "Mainline"
  };
  const selected = getSelectedTrainings();
  const text = selected.map((k) => labels[k] || k).join(", ");
  plantTrainingSummary.textContent = `Treinamentos: ${text || "indefinido"}`;
}

function syncPlantSourceConstraints() {
  const nonGermOption = plantLifeState.querySelector('option[value="nao_germinada"]');
  const mustBeGerminated = plantSource.value !== "semente";

  if (nonGermOption) {
    nonGermOption.hidden = mustBeGerminated;
    nonGermOption.disabled = mustBeGerminated;
  }

  if (mustBeGerminated) {
    plantLifeState.value = "germinacao_iniciada";
    plantLifeState.disabled = true;
  } else {
    plantLifeState.disabled = false;
    if (!plantLifeState.value) plantLifeState.value = "nao_germinada";
  }
}

function syncPlantLifeState() {
  const needsDate = plantLifeState.value === "germinacao_iniciada";
  plantGermStart.disabled = !needsDate;
  if (needsDate && !plantGermStart.value) {
    plantGermStart.value = nowForDatetimeLocal();
  }
  if (!needsDate) plantGermStart.value = "";
  plantContainer.disabled = !needsDate;
  if (!needsDate) {
    plantContainer.value = "";
    plantSoilType.value = "";
    plantSoilType.disabled = true;
  } else {
    syncPlantContainerSoil();
  }

  updateCreatePlantButtonState();
}

function syncPlantContainerSoil() {
  const supportsSoil = plantContainerSupportsSoil(plantContainer.value);
  plantSoilType.disabled = !supportsSoil;
  if (!supportsSoil) plantSoilType.value = "";
  updateCreatePlantButtonState();
}

function updateCreatePlantButtonState() {
  const hasName = Boolean(normalizeTrim(plantName.value));
  const hasSpecies = Boolean(normalizeTrim(plantSpeciesId.value));
  const hasGrow = Boolean(normalizeTrim(plantGrow.value));
  const hasSource = Boolean(normalizeTrim(plantSource.value));
  const hasPurpose = Boolean(normalizeTrim(plantPurpose.value));
  const hasSex = Boolean(normalizeTrim(plantSex.value));
  const hasLife = Boolean(normalizeTrim(plantLifeState.value));
  const hasPhotoperiod = Boolean(normalizeTrim(plantPhotoperiod.value));

  const needsGermData = plantLifeState.value === "germinacao_iniciada";
  const hasGermStart = !needsGermData || Boolean(normalizeTrim(plantGermStart.value));
  const hasContainer = !needsGermData || Boolean(normalizeTrim(plantContainer.value));
  const needsSoil = needsGermData && plantContainerSupportsSoil(plantContainer.value);
  const hasSoil = !needsSoil || Boolean(normalizeTrim(plantSoilType.value));
  const hasTraining = getSelectedTrainings().length > 0;

  const ready = hasName && hasSpecies && hasGrow && hasSource && hasPurpose && hasSex &&
    hasLife && hasPhotoperiod && hasGermStart && hasContainer && hasSoil && hasTraining;
  btnCreatePlant.disabled = !ready;
}

function renderCycles() {
  if (!cyclesCache.length) {
    cyclesList.innerHTML = '<div class="muted">Nenhum ciclo cadastrado.</div>';
    return;
  }

  cyclesList.innerHTML = "";
  for (const c of cyclesCache) {
    const plants = Array.isArray(c.plant_ids) ? c.plant_ids : [];
    const item = document.createElement("div");
    item.className = "list-item";
    item.innerHTML = `
      <div class="list-title">${c.name}</div>
      <div class="muted">ID: ${c.id} | Ambiente: ${c.grow_id || "--"} | Plantas: ${plants.length} | Fase: ${c.phase || "--"}</div>
      <div class="muted">Inicio: ${c.start_datetime || "--"}</div>
    `;
    cyclesList.appendChild(item);
  }
}

function findGrowById(id) {
  return growsCache.find((g) => g.id === id) || null;
}

function toolKindLabel(kind) {
  if (kind === "light") return "Iluminacao";
  if (kind === "exhaust") return "Exaustao";
  if (kind === "filter") return "Filtragem";
  if (kind === "internal_ventilation") return "Ventilacao interna";
  if (kind === "passive_intake") return "Entrada de ar";
  return kind || "Equipamento";
}

function renderToolFormKind() {
  const kind = growToolKind.value;
  toolFormLight.style.display = (kind === "light") ? "block" : "none";
  toolFormExhaust.style.display = (kind === "exhaust") ? "block" : "none";
  toolFormFilter.style.display = (kind === "filter") ? "block" : "none";
  toolFormInternalVent.style.display = (kind === "internal_ventilation") ? "block" : "none";
  toolFormPassiveIntake.style.display = (kind === "passive_intake") ? "block" : "none";
  btnGrowToolAdd.disabled = !kind;
}

function syncCompleteToggle(checkEl, inputEl) {
  if (!checkEl || !inputEl) return;
  inputEl.disabled = !!checkEl.checked;
  if (checkEl.checked) inputEl.value = "";
}

function syncLightHeightMode() {
  const isFixed = toolLightHeightMode.value === "fixed";
  if (isFixed) {
    toolLightFullDrop.checked = false;
    toolLightFullDrop.disabled = true;
    toolLightMaxDropCm.disabled = true;
    toolLightMaxDropCm.value = "";
    return;
  }
  toolLightFullDrop.disabled = false;
  syncCompleteToggle(toolLightFullDrop, toolLightMaxDropCm);
}

function syncAllCompleteToggles() {
  syncLightHeightMode();
  syncCompleteToggle(toolExhaustFullLength, toolExhaustMaxDuctLengthCm);
  syncCompleteToggle(toolInternalVentFullHeight, toolInternalVentMaxHeightCm);
  syncCompleteToggle(toolPassiveFullHeight, toolPassiveHeightCm);
}

function resetToolForm() {
  growToolKind.value = "";
  toolLightType.value = "quantum_board";
  toolLightPowerW.value = "";
  toolLightHeightMode.value = "variable";
  toolLightMinTopCm.value = "";
  toolLightMaxDropCm.value = "";
  toolLightFullDrop.checked = false;

  toolExhaustModel.value = "";
  toolExhaustType.value = "inline";
  toolExhaustFlow.value = "";
  toolExhaustPowerW.value = "";
  toolExhaustDuctDiameterMm.value = "";
  toolExhaustMaxDuctLengthCm.value = "";
  toolExhaustFullLength.checked = false;

  toolFilterType.value = "carvao_ativado";
  toolFilterModel.value = "";
  toolFilterDiameterMm.value = "";
  toolFilterLengthCm.value = "";
  toolFilterFlowM3h.value = "";

  toolInternalVentCount.value = "";
  toolInternalVentSizeCm.value = "";
  toolInternalVentPowerW.value = "";
  toolInternalVentMaxHeightCm.value = "";
  toolInternalVentFullHeight.checked = false;

  toolPassiveMode.value = "passiva";
  toolPassiveWidthCm.value = "";
  toolPassiveHeightCm.value = "";
  toolPassiveAreaCm2.value = "";
  toolPassiveFullHeight.checked = false;
  toolPassiveNotes.value = "";

  editingGrowToolId = "";
  btnGrowToolAdd.textContent = "Adicionar";
  renderToolFormKind();
  syncAllCompleteToggles();
}

function fillToolForm(tool) {
  const kind = tool?.kind || "";
  const data = tool?.data || {};
  growToolKind.value = kind;
  renderToolFormKind();
  growToolFormCard.style.display = "block";

  if (kind === "light") {
    toolLightType.value = data.light_type || "quantum_board";
    toolLightPowerW.value = data.power_w ?? "";
    toolLightHeightMode.value = data.height_mode || "variable";
    toolLightMinTopCm.value = data.min_to_top_cm ?? "";
    toolLightMaxDropCm.value = data.max_drop_cm ?? "";
    toolLightFullDrop.checked = Boolean(data.full_drop);
  } else if (kind === "exhaust") {
    toolExhaustModel.value = data.model || "";
    toolExhaustType.value = data.exhaust_type || "inline";
    toolExhaustFlow.value = data.flow_m3h ?? "";
    toolExhaustPowerW.value = data.power_w ?? "";
    toolExhaustDuctDiameterMm.value = data.duct_diameter_mm ?? "";
    toolExhaustMaxDuctLengthCm.value = data.max_duct_length_cm ?? "";
    toolExhaustFullLength.checked = Boolean(data.full_length);
  } else if (kind === "filter") {
    toolFilterType.value = data.filter_type || "carvao_ativado";
    toolFilterModel.value = data.model || "";
    toolFilterDiameterMm.value = data.diameter_mm ?? "";
    toolFilterLengthCm.value = data.length_cm ?? "";
    toolFilterFlowM3h.value = data.flow_m3h ?? "";
  } else if (kind === "internal_ventilation") {
    toolInternalVentCount.value = data.fan_count ?? "";
    toolInternalVentSizeCm.value = data.fan_size_cm ?? "";
    toolInternalVentPowerW.value = data.power_each_w ?? "";
    toolInternalVentMaxHeightCm.value = data.max_height_cm ?? "";
    toolInternalVentFullHeight.checked = Boolean(data.full_height);
  } else if (kind === "passive_intake") {
    toolPassiveMode.value = data.intake_mode || "passiva";
    toolPassiveWidthCm.value = data.width_cm ?? "";
    toolPassiveHeightCm.value = data.height_cm ?? "";
    toolPassiveAreaCm2.value = data.opening_area_cm2 ?? "";
    toolPassiveFullHeight.checked = Boolean(data.full_height);
    toolPassiveNotes.value = data.notes || "";
  }

  syncAllCompleteToggles();
}

function summarizeTool(tool) {
  const data = tool?.data || {};
  if (tool?.kind === "light") {
    return `Tipo: ${data.light_type || "--"} | Potencia: ${data.power_w || 0}W | Distancia minima topo: ${data.min_to_top_cm || 0}cm`;
  }
  if (tool?.kind === "exhaust") {
    return `Tipo: ${data.exhaust_type || "--"} | Vazao: ${data.flow_m3h || 0}m3/h | Duto: ${data.duct_diameter_mm || 0}mm`;
  }
  if (tool?.kind === "filter") {
    return `Tipo: ${data.filter_type || "--"} | Diametro: ${data.diameter_mm || 0}mm | Comprimento: ${data.length_cm || 0}cm`;
  }
  if (tool?.kind === "internal_ventilation") {
    return `Ventiladores: ${data.fan_count || 0} | Helice: ${data.fan_size_cm || 0}cm | Potencia: ${data.power_each_w || 0}W`;
  }
  if (tool?.kind === "passive_intake") {
    return `Modo: ${data.intake_mode || "--"} | Abertura: ${data.opening_area_cm2 || 0}cm2`;
  }
  return "Sem detalhes";
}

function renderGrowTools() {
  const toolsMap = currentGrowToolsMap();
  const list = toolsMap[currentGrowDetailId] || [];
  if (!list.length) {
    growToolsList.innerHTML = '<div class="muted">Nenhum equipamento cadastrado.</div>';
    return;
  }

  growToolsList.innerHTML = "";
  for (const tool of list) {
    const item = document.createElement("div");
    item.className = "list-item";
    item.innerHTML = `
      <div class="list-title">${toolKindLabel(tool.kind)}</div>
      <div class="muted">${summarizeTool(tool)}</div>
      <div class="tool-actions">
        <button data-grow-tool-edit="${tool.id}">Editar</button>
        <button class="btn-danger" data-grow-tool-delete="${tool.id}">Excluir</button>
      </div>
    `;
    growToolsList.appendChild(item);
  }
}

function renderGrowLinkedPlants() {
  const linked = plantsCache.filter((p) => p.grow_id === currentGrowDetailId);
  if (!linked.length) {
    growPlantsList.innerHTML = '<div class="muted">Nenhuma planta vinculada a este ambiente.</div>';
    return;
  }

  growPlantsList.innerHTML = "";
  for (const p of linked) {
    const item = document.createElement("div");
    item.className = "list-item";
    item.innerHTML = `
      <div class="list-title">${p.name}</div>
      <div class="muted">Especie: ${p.species_name || "--"} | Sexo: ${p.sex || "--"} | Finalidade: ${p.purpose || "--"}</div>
    `;
    growPlantsList.appendChild(item);
  }
}

function openGrowDetail(id) {
  const grow = findGrowById(id);
  if (!grow) return;

  currentGrowDetailId = id;
  const subtype = currentGrowSubtypeMap()[id] || "--";
  growDetailInfo.textContent = `ID: ${grow.id} | ${growTypeLabel(grow.type)} / ${subtype} | ${grow.width_cm}x${grow.depth_cm}x${grow.height_cm} cm`;
  growToolFormCard.style.display = "none";
  resetToolForm();
  growToolMsg.textContent = "";
  renderGrowLinkedPlants();
  renderGrowTools();
  show("growDetail");
}

function readToolPayload() {
  const kind = growToolKind.value;
  if (kind === "light") {
    return {
      light_type: toolLightType.value,
      power_w: Number(toolLightPowerW.value || 0),
      height_mode: toolLightHeightMode.value,
      min_to_top_cm: Number(toolLightMinTopCm.value || 0),
      max_drop_cm: Number(toolLightMaxDropCm.value || 0),
      full_drop: toolLightFullDrop.checked
    };
  }
  if (kind === "exhaust") {
    return {
      model: toolExhaustModel.value,
      exhaust_type: toolExhaustType.value,
      flow_m3h: Number(toolExhaustFlow.value || 0),
      power_w: Number(toolExhaustPowerW.value || 0),
      duct_diameter_mm: Number(toolExhaustDuctDiameterMm.value || 0),
      max_duct_length_cm: Number(toolExhaustMaxDuctLengthCm.value || 0),
      full_length: toolExhaustFullLength.checked
    };
  }
  if (kind === "filter") {
    return {
      filter_type: toolFilterType.value,
      model: toolFilterModel.value,
      diameter_mm: Number(toolFilterDiameterMm.value || 0),
      length_cm: Number(toolFilterLengthCm.value || 0),
      flow_m3h: Number(toolFilterFlowM3h.value || 0)
    };
  }
  if (kind === "internal_ventilation") {
    return {
      fan_count: Number(toolInternalVentCount.value || 0),
      fan_size_cm: Number(toolInternalVentSizeCm.value || 0),
      power_each_w: Number(toolInternalVentPowerW.value || 0),
      max_height_cm: Number(toolInternalVentMaxHeightCm.value || 0),
      full_height: toolInternalVentFullHeight.checked
    };
  }
  return {
    intake_mode: toolPassiveMode.value,
    width_cm: Number(toolPassiveWidthCm.value || 0),
    height_cm: Number(toolPassiveHeightCm.value || 0),
    opening_area_cm2: Number(toolPassiveAreaCm2.value || 0),
    full_height: toolPassiveFullHeight.checked,
    notes: toolPassiveNotes.value
  };
}

function findGrowName(id) {
  const g = growsCache.find((x) => x.id === id);
  return g ? g.name : "";
}

function findPlantName(id) {
  const p = plantsCache.find((x) => x.id === id);
  return p ? p.name : "";
}

function refreshCycleWizardState() {
  if (!cycleDraft.grow_ids.length) {
    cycleWizardState.textContent = "Ambientes selecionados: nenhum";
    btnCycleNextToPlants.disabled = true;
    return;
  }
  const labels = cycleDraft.grow_ids.map((id) => {
    const name = findGrowName(id);
    return name ? `${name} (${id})` : id;
  });
  cycleWizardState.textContent = `Ambientes selecionados (${cycleDraft.grow_ids.length}): ${labels.join(", ")}`;
  btnCycleNextToPlants.disabled = false;
}

function refreshCyclePlantState() {
  if (!cycleDraft.plant_ids.length) {
    cyclePlantState.textContent = "Plantas selecionadas: nenhuma";
    return;
  }
  const labels = cycleDraft.plant_ids.map((id) => {
    const name = findPlantName(id);
    return name ? `${name} (${id})` : id;
  });
  cyclePlantState.textContent = `Plantas selecionadas (${cycleDraft.plant_ids.length}): ${labels.join(", ")}`;
}

function renderCycleGrowList() {
  if (!cycleGrowList) return;
  cycleGrowList.innerHTML = "";

  const available = new Set(growsCache.map((g) => g.id));
  cycleDraft.grow_ids = cycleDraft.grow_ids.filter((id) => available.has(id));
  const allowedPlants = new Set(
    plantsCache
      .filter((p) => cycleDraft.grow_ids.includes(p.grow_id))
      .map((p) => p.id)
  );
  cycleDraft.plant_ids = cycleDraft.plant_ids.filter((id) => allowedPlants.has(id));
  saveCycleDraft();

  if (!growsCache.length) {
    cycleGrowList.innerHTML = '<div class="muted">Nenhum ambiente cadastrado.</div>';
    refreshCycleWizardState();
    return;
  }

  for (const g of growsCache) {
    const item = document.createElement("button");
    item.type = "button";
    item.className = "list-item";
    if (cycleDraft.grow_ids.includes(g.id)) item.classList.add("active");
    item.setAttribute("data-cycle-grow-id", g.id);
    item.innerHTML = `
      <div class="list-title">${g.name}</div>
      <div class="muted">ID: ${g.id} | ${growTypeLabel(g.type)} | ${g.width_cm}x${g.depth_cm}x${g.height_cm} cm</div>
    `;
    cycleGrowList.appendChild(item);
  }

  refreshCycleWizardState();
}

function renderCyclePlantList() {
  if (!cyclePlantList) return;
  cyclePlantList.innerHTML = "";

  if (!cycleDraft.grow_ids.length) {
    cyclePlantList.innerHTML = '<div class="muted">Selecione ambientes no passo anterior.</div>';
    refreshCyclePlantState();
    return;
  }

  const available = plantsCache.filter((p) => cycleDraft.grow_ids.includes(p.grow_id));
  const availableSet = new Set(available.map((p) => p.id));
  cycleDraft.plant_ids = cycleDraft.plant_ids.filter((id) => availableSet.has(id));
  saveCycleDraft();

  if (!available.length) {
    cyclePlantList.innerHTML = '<div class="muted">Nenhuma planta encontrada para os ambientes selecionados.</div>';
    refreshCyclePlantState();
    return;
  }

  for (const p of available) {
    const growName = findGrowName(p.grow_id) || p.grow_id;
    const item = document.createElement("button");
    item.type = "button";
    item.className = "list-item";
    if (cycleDraft.plant_ids.includes(p.id)) item.classList.add("active");
    item.setAttribute("data-cycle-plant-id", p.id);
    item.innerHTML = `
      <div class="list-title">${p.name}</div>
      <div class="muted">Especie: ${p.species_name || "--"} | Ambiente: ${growName}</div>
    `;
    cyclePlantList.appendChild(item);
  }

  refreshCyclePlantState();
}

function toggleCycleGrowSelection(id) {
  if (!id) return;
  const idx = cycleDraft.grow_ids.indexOf(id);
  if (idx >= 0) cycleDraft.grow_ids.splice(idx, 1);
  else cycleDraft.grow_ids.push(id);
  saveCycleDraft();
  renderCycleGrowList();
  renderCyclePlantList();
}

function toggleCyclePlantSelection(id) {
  if (!id) return;
  const idx = cycleDraft.plant_ids.indexOf(id);
  if (idx >= 0) cycleDraft.plant_ids.splice(idx, 1);
  else cycleDraft.plant_ids.push(id);
  saveCycleDraft();
  renderCyclePlantList();
}

async function loadGrows() {
  const data = await apiGetGrows();
  growsCache = data?.grows || [];
  activeGrowId = data?.active_grow_id || "";
  renderGrows();
}

async function loadPlants() {
  const data = await apiGetPlants();
  plantsCache = data?.plants || [];
  renderPlants();
  if (currentGrowDetailId) renderGrowLinkedPlants();
  renderCyclePlantList();
}

async function loadSpecies() {
  const data = await apiGetSpecies();
  speciesCache = data?.species || [];
  if (expandedSpeciesId && !speciesCache.some((s) => s.id === expandedSpeciesId)) {
    expandedSpeciesId = "";
  }
  renderSpecies();
  renderSpeciesOptions();
}

async function loadCycles() {
  const data = await apiGetCycles();
  cyclesCache = data?.cycles || [];
  renderCycles();
}

async function refreshDataScreens() {
  await loadGrows();
  await loadSpecies();
  await loadPlants();
  await loadCycles();
}

function applyManualSetpoint(target, prev) {
  return apiSetFan(target).then((res) => {
    if (!res?.ok && res?.error === "manual_only") {
      if (prev !== null) uiSetFan(prev);
      msg.textContent = "Falha ao aplicar (somente Manual).";
      return;
    }

    const p = res?.fan?.exhaust_percent ?? target;
    lastApplied = p;
    uiSetFan(p);
    msg.textContent = "Aplicado: " + p + "%";
  }).catch(() => {
    if (prev !== null) uiSetFan(prev);
    msg.textContent = "Falha ao aplicar (simulada/desconectado).";
  });
}

async function refreshDashboard() {
  try {
    const s = await apiGetState();
    const p = s?.fan?.exhaust_percent ?? 0;
    const mode = normalizeMode(s?.mode) || "MANUAL";
    const autoState = normalizeAutoState(s?.auto?.state);

    lastApplied = p;
    uiSetFan(p);
    uiSetMode(mode, autoState);
    uiSetSensors(s?.sensors);

    connStatus.textContent = "Online";
    msg.textContent = "";
    return {
      online: true,
      sensorsReady: Boolean(s?.sensors?.inside?.ok) || Boolean(s?.sensors?.outside?.ok)
    };
  } catch (e) {
    connStatus.textContent = "Offline";
    homeStatus.textContent = "Offline";
    homeMode.textContent = "Modo: --";
    fanSlider.disabled = true;
    homeFanSlider.disabled = true;
    uiSetSensors(null);
    return { online: false, sensorsReady: false };
  }
}

async function refreshAll() {
  await refreshDashboard();
  await refreshDataScreens();
}

async function bootRefresh() {
  const first = await refreshDashboard();
  await refreshDataScreens();
  if (first.online && !first.sensorsReady) {
    setTimeout(() => {
      refreshDashboard();
    }, 1100);
  }
}

navButtons.forEach((btn) => {
  btn.addEventListener("click", async () => {
    const screen = btn.dataset.screen;
    show(screen);
    if (screen === "grows") {
      await loadGrows();
    } else if (screen === "plants") {
      await refreshDataScreens();
    } else if (screen === "cycles") {
      await refreshDataScreens();
    }
  });
});

fanIcon.addEventListener("click", async () => {
  show("fan");
  await refreshDashboard();
});

btnBack.addEventListener("click", () => {
  show("home");
});

for (const key in modeButtons) {
  const btn = modeButtons[key];
  btn.addEventListener("click", async () => {
    const modeArg = btn.dataset.mode;
    try {
      msg.textContent = "Aplicando modo...";
      const res = await apiSetMode(modeArg);
      if (!res?.ok) {
        msg.textContent = "Falha ao trocar modo.";
        return;
      }

      const mode = normalizeMode(res?.mode) || "MANUAL";
      const autoState = normalizeAutoState(res?.auto?.state);
      const p = res?.fan?.exhaust_percent ?? (lastApplied ?? 0);

      lastApplied = p;
      uiSetFan(p);
      uiSetMode(mode, autoState);
      msg.textContent = "Modo aplicado: " + mode;
    } catch (e) {
      msg.textContent = "Falha ao trocar modo.";
    }
  });
}

toggleFail.addEventListener("change", () => {
  simState.fail_mode = toggleFail.checked;
});

fanSlider.addEventListener("input", () => {
  fanValue.textContent = fanSlider.value + "%";
});

homeFanSlider.addEventListener("input", () => {
  homeStatus.textContent = "Velocidade: " + homeFanSlider.value + "%";
});

fanSlider.addEventListener("change", async () => {
  const target = Number(fanSlider.value);
  const prev = lastApplied;
  msg.textContent = "Aplicando...";
  await applyManualSetpoint(target, prev);
});

homeFanSlider.addEventListener("change", async () => {
  const target = Number(homeFanSlider.value);
  const prev = lastApplied;
  msg.textContent = "Aplicando...";
  await applyManualSetpoint(target, prev);
});

growTypeSegment.addEventListener("click", (ev) => {
  const btn = ev.target.closest("[data-grow-type]");
  if (!btn) return;
  setGrowType(btn.getAttribute("data-grow-type") || "indoor");
});

btnRefreshGrows.addEventListener("click", async () => {
  try {
    await loadGrows();
    growsMsg.textContent = "Lista atualizada.";
  } catch {
    growsMsg.textContent = "Falha ao carregar ambientes.";
  }
});

btnCreateGrow.addEventListener("click", async () => {
  try {
    const res = await apiCreateGrow(growName.value, growType.value, growWidth.value, growDepth.value, growHeight.value);
    if (!res?.ok) {
      growsMsg.textContent = "Erro: " + (res.error || "create_failed");
      return;
    }
    growName.value = "";
    growWidth.value = "";
    growDepth.value = "";
    growHeight.value = "";
    growsCache = res.grows || [];
    activeGrowId = res.active_grow_id || "";
    const created = growsCache.length ? growsCache[growsCache.length - 1].id : "";
    if (created) {
      const subtypeMap = currentGrowSubtypeMap();
      subtypeMap[created] = growSubtype.value || "";
      saveGrowSubtypes(subtypeMap);
    }
    renderGrows();
    growsMsg.textContent = "Ambiente criado.";
    if (created) openGrowDetail(created);
    if (cycleCreateFlowActive && created) {
      if (!cycleDraft.grow_ids.includes(created)) cycleDraft.grow_ids.push(created);
      saveCycleDraft();
      cycleCreateFlowActive = false;
      cycleWizardMsg.textContent = "Ambiente criado e selecionado no assistente.";
      show("cycleStep1");
    }
  } catch {
    growsMsg.textContent = "Falha ao criar ambiente.";
  }
});

growsList.addEventListener("click", async (ev) => {
  const openBtn = ev.target.closest("[data-grow-open]");
  const activeBtn = ev.target.closest("[data-grow-active]");
  const deleteBtn = ev.target.closest("[data-grow-delete]");

  if (openBtn) {
    openGrowDetail(openBtn.getAttribute("data-grow-open") || "");
  }

  if (activeBtn) {
    try {
      const res = await apiSetActiveGrow(activeBtn.dataset.growActive);
      if (!res?.ok) {
        growsMsg.textContent = "Erro: " + (res.error || "set_active_failed");
        return;
      }
      growsCache = res.grows || [];
      activeGrowId = res.active_grow_id || "";
      renderGrows();
      growsMsg.textContent = "Ambiente ativo atualizado.";
    } catch {
      growsMsg.textContent = "Falha ao definir ambiente ativo.";
    }
  }

  if (deleteBtn) {
    try {
      const res = await apiDeleteGrow(deleteBtn.dataset.growDelete);
      if (!res?.ok) {
        growsMsg.textContent = "Erro: " + (res.error || "delete_failed");
        return;
      }
      growsCache = res.grows || [];
      activeGrowId = res.active_grow_id || "";
      const growId = deleteBtn.dataset.growDelete;
      const subtypeMap = currentGrowSubtypeMap();
      delete subtypeMap[growId];
      saveGrowSubtypes(subtypeMap);
      const toolsMap = currentGrowToolsMap();
      delete toolsMap[growId];
      saveGrowTools(toolsMap);
      renderGrows();
      await loadPlants();
      growsMsg.textContent = "Ambiente removido.";
    } catch {
      growsMsg.textContent = "Falha ao remover ambiente.";
    }
  }
});

btnGrowDetailBack.addEventListener("click", () => {
  show("grows");
});

btnGrowToolToggle.addEventListener("click", () => {
  const willShow = growToolFormCard.style.display === "none";
  growToolFormCard.style.display = willShow ? "block" : "none";
  if (willShow) {
    resetToolForm();
    growToolMsg.textContent = "Escolha o tipo de equipamento para continuar.";
  } else {
    growToolMsg.textContent = "";
  }
});

growToolKind.addEventListener("change", renderToolFormKind);

btnGrowToolAdd.addEventListener("click", () => {
  if (!currentGrowDetailId) {
    growToolMsg.textContent = "Nenhum ambiente selecionado.";
    return;
  }

  if (!growToolKind.value) {
    growToolMsg.textContent = "Selecione um equipamento.";
    return;
  }

  const toolsMap = currentGrowToolsMap();
  const list = toolsMap[currentGrowDetailId] || [];

  if (editingGrowToolId) {
    const idx = list.findIndex((t) => t.id === editingGrowToolId);
    if (idx >= 0) {
      list[idx] = {
        ...list[idx],
        kind: growToolKind.value,
        data: readToolPayload()
      };
      growToolMsg.textContent = "Equipamento atualizado.";
    } else {
      list.push({
        id: "t" + Date.now(),
        kind: growToolKind.value,
        data: readToolPayload()
      });
      growToolMsg.textContent = "Equipamento adicionado.";
    }
  } else {
    list.push({
      id: "t" + Date.now(),
      kind: growToolKind.value,
      data: readToolPayload()
    });
    growToolMsg.textContent = "Equipamento adicionado.";
  }

  toolsMap[currentGrowDetailId] = list;
  saveGrowTools(toolsMap);
  resetToolForm();
  renderGrowTools();
});

growToolsList.addEventListener("click", (ev) => {
  const editBtn = ev.target.closest("[data-grow-tool-edit]");
  const deleteBtn = ev.target.closest("[data-grow-tool-delete]");
  if (!currentGrowDetailId) return;

  const toolsMap = currentGrowToolsMap();
  const list = toolsMap[currentGrowDetailId] || [];

  if (editBtn) {
    const id = editBtn.getAttribute("data-grow-tool-edit") || "";
    const tool = list.find((t) => t.id === id);
    if (!tool) return;
    editingGrowToolId = id;
    btnGrowToolAdd.textContent = "Salvar alteracoes";
    fillToolForm(tool);
    growToolMsg.textContent = "Editando equipamento.";
    return;
  }

  if (deleteBtn) {
    const id = deleteBtn.getAttribute("data-grow-tool-delete") || "";
    const filtered = list.filter((t) => t.id !== id);
    toolsMap[currentGrowDetailId] = filtered;
    saveGrowTools(toolsMap);
    if (editingGrowToolId === id) resetToolForm();
    renderGrowTools();
    growToolMsg.textContent = "Equipamento removido.";
  }
});

[toolLightFullDrop, toolExhaustFullLength, toolInternalVentFullHeight, toolPassiveFullHeight].forEach((el) => {
  el.addEventListener("change", syncAllCompleteToggles);
});
toolLightHeightMode.addEventListener("change", syncAllCompleteToggles);

btnSpeciesToggle.addEventListener("click", () => {
  const willShow = speciesFormCard.style.display === "none";
  speciesFormCard.style.display = willShow ? "block" : "none";
  speciesMsg.textContent = "";
});

btnSpeciesPanelToggle.addEventListener("click", () => {
  setSpeciesPanelExpanded(!speciesPanelExpanded);
});

btnCreateSpecies.addEventListener("click", async () => {
  try {
    const res = await apiCreateSpecies(
      speciesNameInput.value,
      speciesVegDays.value,
      speciesFloraDays.value,
      speciesIndoorHeight.value,
      speciesOutdoorHeight.value,
      speciesIndoorYield.value,
      speciesOutdoorYield.value,
      speciesSativaPct.value,
      speciesIndicaPct.value,
      speciesThcPct.value,
      speciesCbdPct.value
    );
    if (!res?.ok) {
      speciesMsg.textContent = "Erro: " + (res.error || "create_species_failed");
      return;
    }
    speciesCache = res.species || [];
    renderSpecies();
    renderSpeciesOptions();
    speciesNameInput.value = "";
    speciesVegDays.value = "";
    speciesFloraDays.value = "";
    speciesIndoorHeight.value = "";
    speciesOutdoorHeight.value = "";
    speciesIndoorYield.value = "";
    speciesOutdoorYield.value = "";
    speciesSativaPct.value = "";
    speciesIndicaPct.value = "";
    speciesThcPct.value = "";
    speciesCbdPct.value = "";
    speciesMsg.textContent = "Especie cadastrada.";
  } catch {
    speciesMsg.textContent = "Falha ao criar especie.";
  }
});

speciesList.addEventListener("click", (ev) => {
  const row = ev.target.closest("[data-species-item]");
  if (!row) return;
  const id = row.getAttribute("data-species-item") || "";
  expandedSpeciesId = (expandedSpeciesId === id) ? "" : id;
  renderSpecies();
});

plantLifeState.addEventListener("change", syncPlantLifeState);
plantSource.addEventListener("change", () => {
  syncPlantSourceConstraints();
  syncPlantLifeState();
});
plantContainer.addEventListener("change", syncPlantContainerSoil);
[
  plantName,
  plantSpeciesId,
  plantGrow,
  plantPurpose,
  plantSex,
  plantPhotoperiod,
  plantGermStart,
  plantSoilType
].forEach((el) => {
  el.addEventListener("input", updateCreatePlantButtonState);
  el.addEventListener("change", updateCreatePlantButtonState);
});

plantTrainingChecks.forEach((cb) => {
  cb.addEventListener("change", () => {
    syncTrainingSelection(cb.getAttribute("data-training") || "");
    updateCreatePlantButtonState();
  });
});

btnRefreshPlants.addEventListener("click", async () => {
  try {
    await refreshDataScreens();
    plantsMsg.textContent = "Lista atualizada.";
  } catch {
    plantsMsg.textContent = "Falha ao carregar plantas.";
  }
});

btnCreatePlant.addEventListener("click", async () => {
  try {
    const selectedTrainings = getSelectedTrainings();
    const res = await apiCreatePlant(
      plantName.value,
      plantSpeciesId.value,
      plantGrow.value,
      plantSource.value,
      plantPurpose.value,
      plantSex.value,
      plantLifeState.value,
      plantGermStart.value,
      plantContainer.value,
      plantSoilType.value,
      plantPhotoperiod.value,
      selectedTrainings.join(","),
      plantNotes.value
    );
    if (!res?.ok) {
      plantsMsg.textContent = "Erro: " + (res.error || "create_failed");
      return;
    }

    plantName.value = "";
    plantSource.value = "semente";
    plantPurpose.value = "colheita";
    plantSex.value = "indefinido";
    plantLifeState.value = "nao_germinada";
    plantGermStart.value = "";
    plantContainer.value = "";
    plantSoilType.value = "";
    plantPhotoperiod.value = "fotoperiodo";
    setSelectedTrainings(["indefinido"]);
    updateTrainingSummary();
    plantTrainingDetails.open = false;
    plantNotes.value = "";
    syncPlantSourceConstraints();
    syncPlantLifeState();
    updateCreatePlantButtonState();
    plantsCache = res.plants || [];
    renderPlants();
    plantsMsg.textContent = "Planta criada.";
  } catch {
    plantsMsg.textContent = "Falha ao criar planta.";
  }
});

plantsList.addEventListener("click", async (ev) => {
  const deleteBtn = ev.target.closest("[data-plant-delete]");
  if (!deleteBtn) return;

  try {
    const res = await apiDeletePlant(deleteBtn.dataset.plantDelete);
    if (!res?.ok) {
      plantsMsg.textContent = "Erro: " + (res.error || "delete_failed");
      return;
    }
    plantsCache = res.plants || [];
    renderPlants();
    plantsMsg.textContent = "Planta removida.";
  } catch {
    plantsMsg.textContent = "Falha ao remover planta.";
  }
});

btnRefreshCycles.addEventListener("click", async () => {
  try {
    await loadCycles();
    cyclesMsg.textContent = "Lista atualizada.";
  } catch {
    cyclesMsg.textContent = "Falha ao carregar ciclos.";
  }
});

btnNewCycle.addEventListener("click", () => {
  refreshDataScreens().then(() => {
    show("cycleStep1");
    cyclesMsg.textContent = "Assistente iniciado: Passo 1/4.";
    refreshCycleWizardState();
    renderCyclePlantList();
  }).catch(() => {
    cycleWizardMsg.textContent = "Falha ao carregar ambientes.";
  });
});

btnCycleBack.addEventListener("click", () => {
  show("cycles");
});

cycleGrowList.addEventListener("click", (ev) => {
  const btn = ev.target.closest("[data-cycle-grow-id]");
  if (!btn) return;
  toggleCycleGrowSelection(btn.getAttribute("data-cycle-grow-id") || "");
});

btnCycleNextToPlants.addEventListener("click", () => {
  if (!cycleDraft.grow_ids.length) {
    cycleWizardMsg.textContent = "Selecione pelo menos 1 ambiente para avancar.";
    return;
  }
  cycleWizardMsg.textContent = "";
  cyclePlantMsg.textContent = "Passo 2: selecione as plantas do ciclo.";
  renderCyclePlantList();
  show("cycleStep2");
});

btnCycleRefreshGrows.addEventListener("click", async () => {
  try {
    await loadGrows();
    cycleWizardMsg.textContent = "Ambientes atualizados.";
  } catch {
    cycleWizardMsg.textContent = "Falha ao atualizar ambientes.";
  }
});

btnCycleCreateGrowStart.addEventListener("click", () => {
  cycleCreateFlowActive = true;
  cycleWizardMsg.textContent = "Crie um ambiente na tela Ambientes.";
  show("grows");
  growsMsg.textContent = "Fluxo do assistente: crie um ambiente para voltar ao ciclo.";
});

btnCycleStep2Back.addEventListener("click", () => {
  show("cycleStep1");
});

cyclePlantList.addEventListener("click", (ev) => {
  const btn = ev.target.closest("[data-cycle-plant-id]");
  if (!btn) return;
  toggleCyclePlantSelection(btn.getAttribute("data-cycle-plant-id") || "");
});

loadCycleDraft();
setGrowType(growType.value);
renderToolFormKind();
syncPlantSourceConstraints();
setSelectedTrainings(["indefinido"]);
updateTrainingSummary();
syncPlantLifeState();
updateCreatePlantButtonState();
setSpeciesPanelExpanded(false);
bootRefresh();
