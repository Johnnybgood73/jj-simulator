#include "web.h"
#include <WiFi.h>
#include <math.h>
#include "../../include/config.h"

static const char INDEX_HTML[] PROGMEM = R"HTML(
<!doctype html>
<html lang="pt-br">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width,initial-scale=1" />
  <title>Conex Grow</title>
  <style>
    body{font-family:system-ui,Arial;margin:0;padding:16px;padding-bottom:92px;background:#0b0f14;color:#e8eef6}
    .screen{display:none}.screen.active{display:block}
    .title{font-size:20px;font-weight:700;margin:0 0 12px}
    .card{background:#111826;border:1px solid #1d2a3a;border-radius:14px;padding:16px}
    .row{display:flex;gap:10px;align-items:center}
    .spread{justify-content:space-between}
    .label{font-size:18px;font-weight:800}
    .muted{opacity:.75;font-size:13px}
    .bottom-nav{position:fixed;left:0;right:0;bottom:0;display:grid;grid-template-columns:repeat(4,1fr);gap:8px;padding:8px;border-top:1px solid #2b3f5a;background:#0f1724;z-index:50}
    .nav-btn,.mode-btn,button{background:#1c2b40;border:1px solid #2b3f5a;color:#e8eef6;border-radius:12px;padding:10px 12px;cursor:pointer}
    .nav-btn.active,.mode-btn.active{background:#2e4768;border-color:#4f739d}
    .mode-grid{display:grid;grid-template-columns:repeat(4,1fr);gap:8px}
    .sensor-grid{display:grid;grid-template-columns:1fr 1fr;gap:12px;margin-top:12px}
    .sensor-card.temp-cold{border-color:#2b6fff}.sensor-card.temp-fresh{border-color:#2ec4b6}
    .sensor-card.temp-warm{border-color:#ff9f1c}.sensor-card.temp-hot{border-color:#ff4d4f}
    .sensor-temp{font-size:28px;font-weight:800}
    .list{display:grid;gap:8px;margin-top:10px}
    .item{border:1px solid #24344a;border-radius:10px;padding:10px}
    .item.active{border-color:#4f739d;background:#1a2a3f}
    .list button.item{width:100%;text-align:left}
    .actions{display:flex;gap:8px;flex-wrap:wrap;margin-top:8px}
    .form-grid{display:grid;grid-template-columns:repeat(2,1fr);gap:8px}
    input,select{width:100%;background:#162235;border:1px solid #2b3f5a;color:#e8eef6;border-radius:10px;padding:10px}
    input[type="range"]{width:100%;padding:0}input[type="range"]:disabled{opacity:.5}
    @media (max-width:820px){.mode-grid{grid-template-columns:repeat(2,1fr)}}
    @media (max-width:680px){.form-grid{grid-template-columns:1fr}}
  </style>
</head>
<body>
  <h1 class="title">Conex Grow</h1>

  <div id="home" class="screen active">
    <div class="card">
      <div class="label">Exaustor</div>
      <div id="homeStatus" class="muted">Carregando...</div>
      <div id="homeMode" class="muted">Modo: --</div>
      <input id="homeFanSlider" type="range" min="0" max="100" value="0" style="margin-top:10px" />
      <div id="homeFanHint" class="muted" style="margin-top:6px">Ajuste rapido (aplica ao soltar).</div>
      <div id="msg" class="muted" style="margin-top:8px"></div>
    </div>

    <div class="card" style="margin-top:12px">
      <div class="label" style="margin-bottom:10px">Modo</div>
      <div class="mode-grid">
        <button class="mode-btn" id="btnModeManual" data-mode="manual">Manual</button>
        <button class="mode-btn" id="btnModeEco" data-mode="eco">Eco-silent</button>
        <button class="mode-btn" id="btnModeBoost" data-mode="boost">Boost</button>
        <button class="mode-btn" id="btnModeAuto" data-mode="auto">Auto</button>
      </div>
    </div>

    <div class="sensor-grid">
      <div id="insideCard" class="card sensor-card">
        <div class="label">Inside</div>
        <div id="insideTemp" class="sensor-temp">--.-&deg;C</div>
        <div id="insideHum" class="muted">--%</div>
      </div>
      <div id="outsideCard" class="card sensor-card">
        <div class="label">Outside</div>
        <div id="outsideTemp" class="sensor-temp">--.-&deg;C</div>
        <div id="outsideHum" class="muted">--%</div>
      </div>
    </div>
  </div>

  <div id="grows" class="screen">
    <div class="card">
      <div class="row spread">
        <div class="label">Grows</div>
        <button id="btnRefreshGrows">Atualizar</button>
      </div>
      <div id="growsList" class="list"></div>
    </div>
    <div class="card" style="margin-top:12px">
      <div class="label" style="margin-bottom:10px">Novo Grow</div>
      <div class="form-grid">
        <input id="growName" type="text" placeholder="Nome do grow" />
        <select id="growType"><option value="indoor">indoor</option><option value="outdoor">outdoor</option></select>
        <select id="growSubtype"></select>
        <input id="growWidth" type="number" min="1" placeholder="Largura (cm)" />
        <input id="growDepth" type="number" min="1" placeholder="Profundidade (cm)" />
        <input id="growHeight" type="number" min="1" placeholder="Altura (cm)" />
      </div>
      <div class="actions"><button id="btnCreateGrow">Criar Grow</button></div>
      <div id="growsMsg" class="muted"></div>
    </div>
  </div>

  <div id="growDetail" class="screen">
    <div class="card">
      <div class="row spread">
        <div class="label">Detalhes do Grow</div>
        <button id="btnGrowDetailBack">Voltar para Grows</button>
      </div>
      <div id="growDetailInfo" class="muted" style="margin-top:8px"></div>
    </div>
    <div class="card" style="margin-top:12px">
      <div class="row spread">
        <div class="label">Apetrechos</div>
        <button id="btnGrowToolToggle">Adicionar utensilios</button>
      </div>
      <div id="growToolsList" class="list"></div>
    </div>
    <div id="growToolFormCard" class="card" style="margin-top:12px;display:none">
      <div class="label" style="margin-bottom:10px">Novo Utensilio</div>
      <div class="form-grid">
        <select id="growToolKind">
          <option value="light">Luz</option>
          <option value="exhaust">Exaustor</option>
          <option value="filter">Filtro</option>
          <option value="internal_ventilation">Ventilacao interna</option>
          <option value="passive_intake">Abertura passiva</option>
        </select>
      </div>

      <div id="toolFormLight" style="margin-top:10px">
        <div class="form-grid">
          <select id="toolLightType">
            <option value="qb">LED Quantum Board</option>
            <option value="quantum_bar">Quantum Bar</option>
            <option value="incandescente">Incandescente</option>
            <option value="mercurio">Mercurio</option>
            <option value="outro">Outro</option>
          </select>
          <input id="toolLightPowerW" type="number" min="0" placeholder="Potencia (W)" />
          <select id="toolLightHeightMode"><option value="variable">Altura variavel</option><option value="fixed">Altura fixa</option></select>
          <input id="toolLightMinTopCm" type="number" min="0" placeholder="Distancia minima LED-topo (cm)" />
          <input id="toolLightMaxDropCm" type="number" min="0" placeholder="Distancia maxima de descida (cm)" />
          <select id="toolLightFullDrop"><option value="false">Descida limitada</option><option value="true">Completo</option></select>
        </div>
      </div>

      <div id="toolFormExhaust" style="margin-top:10px;display:none">
        <div class="form-grid">
          <input id="toolExhaustModel" type="text" placeholder="Modelo" />
          <input id="toolExhaustFlow" type="number" min="0" placeholder="Vazao max (m3/h)" />
          <input id="toolExhaustPowerW" type="number" min="0" placeholder="Potencia (W)" />
        </div>
      </div>

      <div id="toolFormFilter" style="margin-top:10px;display:none">
        <div class="form-grid">
          <input id="toolFilterModel" type="text" placeholder="Modelo" />
          <input id="toolFilterDiameterMm" type="number" min="0" placeholder="Diametro (mm)" />
          <input id="toolFilterLengthCm" type="number" min="0" placeholder="Comprimento (cm)" />
        </div>
      </div>

      <div id="toolFormInternalVent" style="margin-top:10px;display:none">
        <div class="form-grid">
          <input id="toolInternalVentCount" type="number" min="0" placeholder="Qtd ventiladores" />
          <input id="toolInternalVentPowerW" type="number" min="0" placeholder="Potencia por ventilador (W)" />
        </div>
      </div>

      <div id="toolFormPassiveIntake" style="margin-top:10px;display:none">
        <div class="form-grid">
          <input id="toolPassiveAreaCm2" type="number" min="0" placeholder="Area de abertura (cm2)" />
          <input id="toolPassiveNotes" type="text" placeholder="Observacoes" />
        </div>
      </div>

      <div class="actions">
        <button id="btnGrowToolAdd">Adicionar</button>
      </div>
      <div id="growToolMsg" class="muted"></div>
    </div>
  </div>

  <div id="plants" class="screen">
    <div class="card">
      <div class="row spread">
        <div class="label">Plantas</div>
        <button id="btnRefreshPlants">Atualizar</button>
      </div>
      <div id="plantsList" class="list"></div>
    </div>
    <div class="card" style="margin-top:12px">
      <div class="label" style="margin-bottom:10px">Nova Planta</div>
      <div class="form-grid">
        <input id="plantName" type="text" placeholder="Nome da planta" />
        <input id="plantSpecies" type="text" placeholder="Especie" />
        <input id="plantDate" type="date" />
        <select id="plantGrow"></select>
      </div>
      <div class="actions"><button id="btnCreatePlant">Criar Planta</button></div>
      <div id="plantsMsg" class="muted"></div>
    </div>
  </div>

  <div id="cycles" class="screen">
    <div class="card">
      <div class="row spread">
        <div class="label">Ciclos</div>
        <button id="btnRefreshCycles">Atualizar</button>
      </div>
      <div id="cyclesList" class="list"></div>
    </div>
    <div class="card" style="margin-top:12px">
      <div class="label" style="margin-bottom:10px">Novo Ciclo</div>
      <div class="muted">Inicie o assistente guiado para criar seu ciclo por etapas.</div>
      <div class="actions"><button id="btnNewCycle">Criar novo ciclo</button></div>
      <div id="cyclesMsg" class="muted"></div>
    </div>
  </div>

  <div id="cycleStep1" class="screen">
    <div class="card">
      <div class="row spread">
        <div class="label">Novo Ciclo</div>
        <button id="btnCycleBack">Voltar para Ciclos</button>
      </div>
    </div>
    <div class="card" style="margin-top:12px">
      <div class="label" style="margin-bottom:10px">Assistente de Ciclo - Passo 1/4</div>
      <div class="muted" style="margin-bottom:10px">Selecione um ou mais ambientes existentes.</div>
      <div class="actions">
        <button id="btnCycleCreateGrowStart">Criar novo ambiente</button>
        <button id="btnCycleRefreshGrows">Atualizar ambientes</button>
      </div>
      <div id="cycleGrowList" class="list"></div>
      <div id="cycleWizardState" class="muted" style="margin-top:8px"></div>
      <div id="cycleWizardMsg" class="muted"></div>
    </div>
  </div>

  <div class="bottom-nav">
    <button class="nav-btn active" data-screen="home">Dashboard</button>
    <button class="nav-btn" data-screen="grows">Grows</button>
    <button class="nav-btn" data-screen="plants">Plantas</button>
    <button class="nav-btn" data-screen="cycles">Ciclos</button>
  </div>

<script>
  const screens={home:document.getElementById('home'),grows:document.getElementById('grows'),growDetail:document.getElementById('growDetail'),plants:document.getElementById('plants'),cycles:document.getElementById('cycles'),cycleStep1:document.getElementById('cycleStep1')};
  const navButtons=document.querySelectorAll('.nav-btn');
  const homeStatus=document.getElementById('homeStatus');
  const homeMode=document.getElementById('homeMode');
  const homeFanSlider=document.getElementById('homeFanSlider');
  const homeFanHint=document.getElementById('homeFanHint');
  const msg=document.getElementById('msg');
  const insideTemp=document.getElementById('insideTemp');
  const insideHum=document.getElementById('insideHum');
  const outsideTemp=document.getElementById('outsideTemp');
  const outsideHum=document.getElementById('outsideHum');
  const insideCard=document.getElementById('insideCard');
  const outsideCard=document.getElementById('outsideCard');
  const modeButtons={
    MANUAL:document.getElementById('btnModeManual'),
    ECO:document.getElementById('btnModeEco'),
    BOOST:document.getElementById('btnModeBoost'),
    AUTO:document.getElementById('btnModeAuto')
  };
  const growsList=document.getElementById('growsList');
  const growName=document.getElementById('growName');
  const growType=document.getElementById('growType');
  const growSubtype=document.getElementById('growSubtype');
  const growWidth=document.getElementById('growWidth');
  const growDepth=document.getElementById('growDepth');
  const growHeight=document.getElementById('growHeight');
  const btnCreateGrow=document.getElementById('btnCreateGrow');
  const btnRefreshGrows=document.getElementById('btnRefreshGrows');
  const growsMsg=document.getElementById('growsMsg');
  const btnGrowDetailBack=document.getElementById('btnGrowDetailBack');
  const growDetailInfo=document.getElementById('growDetailInfo');
  const growToolsList=document.getElementById('growToolsList');
  const btnGrowToolToggle=document.getElementById('btnGrowToolToggle');
  const growToolFormCard=document.getElementById('growToolFormCard');
  const growToolKind=document.getElementById('growToolKind');
  const btnGrowToolAdd=document.getElementById('btnGrowToolAdd');
  const growToolMsg=document.getElementById('growToolMsg');
  const toolFormLight=document.getElementById('toolFormLight');
  const toolFormExhaust=document.getElementById('toolFormExhaust');
  const toolFormFilter=document.getElementById('toolFormFilter');
  const toolFormInternalVent=document.getElementById('toolFormInternalVent');
  const toolFormPassiveIntake=document.getElementById('toolFormPassiveIntake');
  const toolLightType=document.getElementById('toolLightType');
  const toolLightPowerW=document.getElementById('toolLightPowerW');
  const toolLightHeightMode=document.getElementById('toolLightHeightMode');
  const toolLightMinTopCm=document.getElementById('toolLightMinTopCm');
  const toolLightMaxDropCm=document.getElementById('toolLightMaxDropCm');
  const toolLightFullDrop=document.getElementById('toolLightFullDrop');
  const toolExhaustModel=document.getElementById('toolExhaustModel');
  const toolExhaustFlow=document.getElementById('toolExhaustFlow');
  const toolExhaustPowerW=document.getElementById('toolExhaustPowerW');
  const toolFilterModel=document.getElementById('toolFilterModel');
  const toolFilterDiameterMm=document.getElementById('toolFilterDiameterMm');
  const toolFilterLengthCm=document.getElementById('toolFilterLengthCm');
  const toolInternalVentCount=document.getElementById('toolInternalVentCount');
  const toolInternalVentPowerW=document.getElementById('toolInternalVentPowerW');
  const toolPassiveAreaCm2=document.getElementById('toolPassiveAreaCm2');
  const toolPassiveNotes=document.getElementById('toolPassiveNotes');
  const plantsList=document.getElementById('plantsList');
  const plantName=document.getElementById('plantName');
  const plantSpecies=document.getElementById('plantSpecies');
  const plantDate=document.getElementById('plantDate');
  const plantGrow=document.getElementById('plantGrow');
  const btnCreatePlant=document.getElementById('btnCreatePlant');
  const btnRefreshPlants=document.getElementById('btnRefreshPlants');
  const plantsMsg=document.getElementById('plantsMsg');
  const cyclesList=document.getElementById('cyclesList');
  const btnRefreshCycles=document.getElementById('btnRefreshCycles');
  const btnNewCycle=document.getElementById('btnNewCycle');
  const cyclesMsg=document.getElementById('cyclesMsg');
  const btnCycleBack=document.getElementById('btnCycleBack');
  const btnCycleCreateGrowStart=document.getElementById('btnCycleCreateGrowStart');
  const btnCycleRefreshGrows=document.getElementById('btnCycleRefreshGrows');
  const cycleGrowList=document.getElementById('cycleGrowList');
  const cycleWizardState=document.getElementById('cycleWizardState');
  const cycleWizardMsg=document.getElementById('cycleWizardMsg');

  let lastApplied=0,currentMode='MANUAL',currentAutoState='DEFAULT',growsCache=[],plantsCache=[],cyclesCache=[],activeGrowId='',cycleDraft={grow_ids:[]},cycleCreateFlowActive=false,currentGrowDetailId='';

  function loadCycleDraft(){
    try{
      const raw=localStorage.getItem('conex_cycle_draft_v1');
      if(!raw) return;
      const p=JSON.parse(raw);
      if(!p||typeof p!=='object') return;
      if(Array.isArray(p.grow_ids)){cycleDraft={grow_ids:p.grow_ids.map((id)=>String(id)).filter((id)=>id)};return;}
      if(p.grow_id){cycleDraft={grow_ids:[String(p.grow_id)]};}
    }catch(_){}
  }
  function saveCycleDraft(){try{localStorage.setItem('conex_cycle_draft_v1',JSON.stringify(cycleDraft));}catch(_){}}

  function loadGrowSubtypes(){try{return JSON.parse(localStorage.getItem('conex_grow_subtypes_v1')||'{}')||{};}catch(_){return {};}}
  function saveGrowSubtypes(map){try{localStorage.setItem('conex_grow_subtypes_v1',JSON.stringify(map));}catch(_){}}
  function loadGrowTools(){try{return JSON.parse(localStorage.getItem('conex_grow_tools_v1')||'{}')||{};}catch(_){return {};}}
  function saveGrowTools(map){try{localStorage.setItem('conex_grow_tools_v1',JSON.stringify(map));}catch(_){}}

  const subtypeOptions={
    indoor:[{v:'grow',l:'Grow'},{v:'armario',l:'Armario'},{v:'pc_grow',l:'PC Grow'}],
    outdoor:[{v:'campo_aberto',l:'Campo aberto'},{v:'entre_plantas',l:'Em meio a outras plantas'},{v:'janela',l:'Janela'},{v:'varanda',l:'Varanda'}]
  };

  function renderSubtypeOptions(selected){
    const type=(growType.value==='outdoor')?'outdoor':'indoor';
    const options=subtypeOptions[type]||[];
    growSubtype.innerHTML='';
    options.forEach((o)=>{const el=document.createElement('option');el.value=o.v;el.textContent=o.l;growSubtype.appendChild(el);});
    if(selected&&options.some((o)=>o.v===selected)) growSubtype.value=selected;
  }

  function currentGrowSubtypeMap(){return loadGrowSubtypes();}
  function currentGrowToolsMap(){return loadGrowTools();}

  function setNavActive(screen){navButtons.forEach((b)=>b.classList.toggle('active',b.dataset.screen===screen));}
  function show(screen){Object.keys(screens).forEach((k)=>screens[k].classList.remove('active'));screens[screen].classList.add('active');setNavActive(screen==='cycleStep1'?'cycles':(screen==='growDetail'?'grows':screen));}
  function normalizeMode(m){m=String(m||'').toUpperCase();return['MANUAL','ECO','BOOST','AUTO'].includes(m)?m:'MANUAL';}
  function normalizeAutoState(s){s=String(s||'').toUpperCase();return['DEFAULT','REFRIGERAMENTO','SAFE'].includes(s)?s:'DEFAULT';}
  function tempClass(t){t=Number(t);if(!Number.isFinite(t))return'';if(t<=18)return'temp-cold';if(t<=26)return'temp-fresh';if(t<=30)return'temp-warm';return'temp-hot';}
  function setCardClass(el,cls){el.classList.remove('temp-cold','temp-fresh','temp-warm','temp-hot');if(cls)el.classList.add(cls);}

  async function callJson(url){
    const r=await fetch(url,{cache:'no-store'});
    let data=null;try{data=await r.json();}catch(_){}
    if(!r.ok)throw new Error((data&&data.error)||('http_'+r.status));
    return data||{};
  }

  const api={
    getState:()=>callJson('/api/state'),
    setFan:(p)=>callJson('/api/fan/exhaust/set?percent='+encodeURIComponent(p)),
    setMode:(m)=>callJson('/api/mode/set?mode='+encodeURIComponent(m)),
    getGrows:()=>callJson('/api/grows'),
    createGrow:(n,t,w,d,h)=>callJson('/api/grow/create?name='+encodeURIComponent(n)+'&type='+encodeURIComponent(t)+'&width_cm='+encodeURIComponent(w)+'&depth_cm='+encodeURIComponent(d)+'&height_cm='+encodeURIComponent(h)),
    deleteGrow:(id)=>callJson('/api/grow/delete?id='+encodeURIComponent(id)),
    setActiveGrow:(id)=>callJson('/api/grow/active/set?id='+encodeURIComponent(id)),
    getPlants:()=>callJson('/api/plants'),
    createPlant:(n,s,date,gid)=>callJson('/api/plant/create?name='+encodeURIComponent(n)+'&species='+encodeURIComponent(s)+'&germination_date='+encodeURIComponent(date)+'&grow_id='+encodeURIComponent(gid)),
    deletePlant:(id)=>callJson('/api/plant/delete?id='+encodeURIComponent(id)),
    getCycles:()=>callJson('/api/cycles')
  };

  function renderGrows(){
    const subtypeMap=currentGrowSubtypeMap();
    if(!growsCache.length){growsList.innerHTML='<div class="muted">Nenhum grow cadastrado.</div>';}else{
      growsList.innerHTML='';
      growsCache.forEach((g)=>{
        const item=document.createElement('div');
        item.className='item';
        const active=activeGrowId===g.id?' (ativo)':'';
        const subtype=subtypeMap[g.id]||'--';
        item.innerHTML='<div><b>'+g.name+active+'</b></div>'
          +'<div class="muted">ID: '+g.id+' | '+g.type+' / '+subtype+' | '+g.width_cm+'x'+g.depth_cm+'x'+g.height_cm+' cm | '+Number(g.volume_m3).toFixed(3)+' m3</div>'
          +'<div class="actions"><button data-grow-open="'+g.id+'">Abrir</button><button data-grow-active="'+g.id+'">Ativar</button><button data-grow-delete="'+g.id+'">Excluir</button></div>';
        growsList.appendChild(item);
      });
    }
    plantGrow.innerHTML='';
    growsCache.forEach((g)=>{const o=document.createElement('option');o.value=g.id;o.textContent=g.name+' ('+g.id+')';plantGrow.appendChild(o);});
    renderCycleGrowList();
  }

  function renderPlants(){
    if(!plantsCache.length){plantsList.innerHTML='<div class="muted">Nenhuma planta cadastrada.</div>';return;}
    plantsList.innerHTML='';
    plantsCache.forEach((p)=>{
      const g=growsCache.find((x)=>x.id===p.grow_id);
      const name=g?g.name:p.grow_id;
      const item=document.createElement('div');
      item.className='item';
      item.innerHTML='<div><b>'+p.name+'</b></div>'
        +'<div class="muted">'+p.species+' | Germinacao: '+p.germination_date+' | Grow: '+name+'</div>'
        +'<div class="actions"><button data-plant-delete="'+p.id+'">Excluir</button></div>';
      plantsList.appendChild(item);
    });
  }

  function renderCycles(){
    if(!cyclesCache.length){cyclesList.innerHTML='<div class="muted">Nenhum ciclo cadastrado.</div>';return;}
    cyclesList.innerHTML='';
    cyclesCache.forEach((c)=>{
      const item=document.createElement('div');
      item.className='item';
      const plants=Array.isArray(c.plant_ids)?c.plant_ids:[];
      item.innerHTML='<div><b>'+c.name+'</b></div>'
        +'<div class="muted">ID: '+c.id+' | Grow: '+(c.grow_id||'--')+' | Plantas: '+plants.length+' | Fase: '+(c.phase||'--')+'</div>'
        +'<div class="muted">Inicio: '+(c.start_datetime||'--')+'</div>';
      cyclesList.appendChild(item);
    });
  }

  function findGrowById(id){return growsCache.find((g)=>g.id===id)||null;}
  function toolKindLabel(kind){
    if(kind==='light') return 'Luz';
    if(kind==='exhaust') return 'Exaustor';
    if(kind==='filter') return 'Filtro';
    if(kind==='internal_ventilation') return 'Ventilacao interna';
    if(kind==='passive_intake') return 'Abertura passiva';
    return kind||'Utensilio';
  }

  function renderToolFormKind(){
    const k=growToolKind.value;
    toolFormLight.style.display=(k==='light')?'block':'none';
    toolFormExhaust.style.display=(k==='exhaust')?'block':'none';
    toolFormFilter.style.display=(k==='filter')?'block':'none';
    toolFormInternalVent.style.display=(k==='internal_ventilation')?'block':'none';
    toolFormPassiveIntake.style.display=(k==='passive_intake')?'block':'none';
  }

  function renderGrowTools(){
    const toolsMap=currentGrowToolsMap();
    const list=toolsMap[currentGrowDetailId]||[];
    if(!list.length){growToolsList.innerHTML='<div class="muted">Nenhum utensilio cadastrado.</div>';return;}
    growToolsList.innerHTML='';
    list.forEach((t)=>{
      const item=document.createElement('div');
      item.className='item';
      item.innerHTML='<div><b>'+toolKindLabel(t.kind)+'</b></div><div class="muted">'+JSON.stringify(t.data||{})+'</div>';
      growToolsList.appendChild(item);
    });
  }

  function openGrowDetail(id){
    const grow=findGrowById(id);
    if(!grow) return;
    currentGrowDetailId=id;
    const subtype=currentGrowSubtypeMap()[id]||'--';
    growDetailInfo.textContent='ID: '+grow.id+' | '+grow.type+' / '+subtype+' | '+grow.width_cm+'x'+grow.depth_cm+'x'+grow.height_cm+' cm';
    growToolFormCard.style.display='none';
    growToolMsg.textContent='';
    renderGrowTools();
    show('growDetail');
  }

  function readToolPayload(){
    const k=growToolKind.value;
    if(k==='light'){
      return {light_type:toolLightType.value,power_w:Number(toolLightPowerW.value||0),height_mode:toolLightHeightMode.value,min_to_top_cm:Number(toolLightMinTopCm.value||0),max_drop_cm:Number(toolLightMaxDropCm.value||0),full_drop:toolLightFullDrop.value==='true'};
    }
    if(k==='exhaust'){
      return {model:toolExhaustModel.value,flow_m3h:Number(toolExhaustFlow.value||0),power_w:Number(toolExhaustPowerW.value||0)};
    }
    if(k==='filter'){
      return {model:toolFilterModel.value,diameter_mm:Number(toolFilterDiameterMm.value||0),length_cm:Number(toolFilterLengthCm.value||0)};
    }
    if(k==='internal_ventilation'){
      return {fan_count:Number(toolInternalVentCount.value||0),power_each_w:Number(toolInternalVentPowerW.value||0)};
    }
    return {opening_area_cm2:Number(toolPassiveAreaCm2.value||0),notes:toolPassiveNotes.value};
  }

  function findGrowName(id){
    const g=growsCache.find((x)=>x.id===id);
    return g?g.name:'';
  }

  function renderCycleGrowList(){
    if(!cycleGrowList) return;
    cycleGrowList.innerHTML='';

    const available=new Set(growsCache.map((g)=>g.id));
    cycleDraft.grow_ids=cycleDraft.grow_ids.filter((id)=>available.has(id));
    saveCycleDraft();

    if(!growsCache.length){
      cycleGrowList.innerHTML='<div class="muted">Nenhum ambiente cadastrado.</div>';
      refreshCycleWizardState();
      return;
    }

    growsCache.forEach((g)=>{
      const item=document.createElement('button');
      item.type='button';
      item.className='item';
      if(cycleDraft.grow_ids.includes(g.id)) item.classList.add('active');
      item.setAttribute('data-cycle-grow-id',g.id);
      item.innerHTML='<div><b>'+g.name+'</b></div>'
        +'<div class="muted">ID: '+g.id+' | '+g.type+' | '+g.width_cm+'x'+g.depth_cm+'x'+g.height_cm+' cm</div>';
      cycleGrowList.appendChild(item);
    });

    refreshCycleWizardState();
  }

  function toggleCycleGrowSelection(id){
    if(!id) return;
    const idx=cycleDraft.grow_ids.indexOf(id);
    if(idx>=0) cycleDraft.grow_ids.splice(idx,1);
    else cycleDraft.grow_ids.push(id);
    saveCycleDraft();
    renderCycleGrowList();
  }

  function refreshCycleWizardState(){
    if(!cycleDraft.grow_ids.length){cycleWizardState.textContent='Ambientes selecionados: nenhum';return;}
    const labels=cycleDraft.grow_ids.map((id)=>{const n=findGrowName(id);return n?(n+' ('+id+')'):id;});
    cycleWizardState.textContent='Ambientes selecionados ('+cycleDraft.grow_ids.length+'): '+labels.join(', ');
  }

  function applyFanUi(p){lastApplied=p;homeFanSlider.value=p;homeStatus.textContent='Velocidade: '+p+'%';}
  function applyModeUi(mode,autoState){
    currentMode=normalizeMode(mode);currentAutoState=normalizeAutoState(autoState);
    Object.keys(modeButtons).forEach((k)=>modeButtons[k].classList.toggle('active',k===currentMode));
    const manual=currentMode==='MANUAL';
    homeFanSlider.disabled=!manual;
    homeFanHint.textContent=manual?'Ajuste rapido (aplica ao soltar).':'Slider rapido ativo apenas em modo Manual.';
    homeMode.textContent=currentMode==='AUTO'?'Modo: AUTO ('+currentAutoState+')':'Modo: '+currentMode;
  }
  function applySensorsUi(s){
    const inr=s&&s.inside;const outr=s&&s.outside;
    if(!inr||!inr.ok){insideTemp.textContent='--.-\u00B0C';insideHum.textContent='--%';setCardClass(insideCard,'');}
    else{insideTemp.textContent=Number(inr.temperature_c).toFixed(1)+'\u00B0C';insideHum.textContent=Math.round(Number(inr.humidity_rh))+'%';setCardClass(insideCard,tempClass(inr.temperature_c));}
    if(!outr||!outr.ok){outsideTemp.textContent='--.-\u00B0C';outsideHum.textContent='--%';setCardClass(outsideCard,'');}
    else{outsideTemp.textContent=Number(outr.temperature_c).toFixed(1)+'\u00B0C';outsideHum.textContent=Math.round(Number(outr.humidity_rh))+'%';setCardClass(outsideCard,tempClass(outr.temperature_c));}
  }

  async function refreshDashboard(){
    try{
      const s=await api.getState();
      const p=s&&s.fan?s.fan.exhaust_percent:0;
      applyFanUi(p);applyModeUi(s?s.mode:null,s&&s.auto?s.auto.state:null);applySensorsUi(s?s.sensors:null);msg.textContent='';
      return {online:true,sensorsReady:Boolean(s&&s.sensors&&((s.sensors.inside&&s.sensors.inside.ok)||(s.sensors.outside&&s.sensors.outside.ok)))};
    }catch(_){
      homeStatus.textContent='Offline';homeMode.textContent='Modo: --';homeFanSlider.disabled=true;applySensorsUi(null);
      return {online:false,sensorsReady:false};
    }
  }

  async function loadGrows(){const d=await api.getGrows();growsCache=d.grows||[];activeGrowId=d.active_grow_id||'';renderGrows();}
  async function loadPlants(){const d=await api.getPlants();plantsCache=d.plants||[];renderPlants();}
  async function loadCycles(){const d=await api.getCycles();cyclesCache=d.cycles||[];renderCycles();}
  async function refreshData(){await loadGrows();await loadPlants();await loadCycles();}

  navButtons.forEach((b)=>b.addEventListener('click',async()=>{show(b.dataset.screen);if(b.dataset.screen==='grows')await loadGrows();if(b.dataset.screen==='plants')await refreshData();if(b.dataset.screen==='cycles')await refreshData();}));

  Object.keys(modeButtons).forEach((k)=>{
    modeButtons[k].addEventListener('click',async()=>{
      try{msg.textContent='Aplicando modo...';const r=await api.setMode(modeButtons[k].dataset.mode);applyFanUi(r&&r.fan?r.fan.exhaust_percent:lastApplied);applyModeUi(r?r.mode:null,r&&r.auto?r.auto.state:null);msg.textContent='Modo aplicado: '+normalizeMode(r?r.mode:null);}
      catch(_){msg.textContent='Falha ao trocar modo.';}
    });
  });

  homeFanSlider.addEventListener('input',()=>{homeStatus.textContent='Velocidade: '+homeFanSlider.value+'%';});
  homeFanSlider.addEventListener('change',async()=>{
    const target=Number(homeFanSlider.value),prev=lastApplied;msg.textContent='Aplicando...';
    try{const r=await api.setFan(target);applyFanUi(r&&r.fan?r.fan.exhaust_percent:target);msg.textContent='Aplicado: '+(r&&r.fan?r.fan.exhaust_percent:target)+'%';}
    catch(e){applyFanUi(prev);msg.textContent=e&&e.message==='manual_only'?'Falha ao aplicar (somente Manual).':'Falha ao aplicar (desconectado).';}
  });

  growType.addEventListener('change',()=>{renderSubtypeOptions();});
  btnRefreshGrows.addEventListener('click',async()=>{try{await loadGrows();growsMsg.textContent='Lista atualizada.';}catch(_){growsMsg.textContent='Falha ao carregar grows.';}});
  btnCreateGrow.addEventListener('click',async()=>{
    try{
      const r=await api.createGrow(growName.value,growType.value,growWidth.value,growDepth.value,growHeight.value);
      growsCache=r.grows||[];
      activeGrowId=r.active_grow_id||'';
      const created=(growsCache.length?growsCache[growsCache.length-1].id:'');
      if(created){
        const subtypeMap=currentGrowSubtypeMap();
        subtypeMap[created]=growSubtype.value||'';
        saveGrowSubtypes(subtypeMap);
      }
      growName.value='';growWidth.value='';growDepth.value='';growHeight.value='';
      renderGrows();
      growsMsg.textContent='Grow criado.';
      if(created) openGrowDetail(created);
      if(cycleCreateFlowActive&&created){
        if(!cycleDraft.grow_ids.includes(created)) cycleDraft.grow_ids.push(created);
        saveCycleDraft();
        cycleCreateFlowActive=false;
        cycleWizardMsg.textContent='Ambiente criado e selecionado no assistente.';
        show('cycleStep1');
      }
    }
    catch(e){growsMsg.textContent='Erro: '+(e&&e.message?e.message:'create_failed');}
  });
  growsList.addEventListener('click',async(ev)=>{
    const o=ev.target.closest('[data-grow-open]');
    const a=ev.target.closest('[data-grow-active]');const d=ev.target.closest('[data-grow-delete]');
    if(o){openGrowDetail(o.getAttribute('data-grow-open')||'');}
    if(a){try{const r=await api.setActiveGrow(a.dataset.growActive);growsCache=r.grows||[];activeGrowId=r.active_grow_id||'';renderGrows();growsMsg.textContent='Grow ativo atualizado.';}catch(e){growsMsg.textContent='Erro: '+(e&&e.message?e.message:'set_active_failed');}}
    if(d){try{const id=d.dataset.growDelete;const r=await api.deleteGrow(id);growsCache=r.grows||[];activeGrowId=r.active_grow_id||'';const sm=currentGrowSubtypeMap();delete sm[id];saveGrowSubtypes(sm);const tm=currentGrowToolsMap();delete tm[id];saveGrowTools(tm);renderGrows();await loadPlants();growsMsg.textContent='Grow removido.';}catch(e){growsMsg.textContent='Erro: '+(e&&e.message?e.message:'delete_failed');}}
  });

  btnGrowDetailBack.addEventListener('click',()=>{show('grows');});
  btnGrowToolToggle.addEventListener('click',()=>{growToolFormCard.style.display=(growToolFormCard.style.display==='none'?'block':'none');});
  growToolKind.addEventListener('change',renderToolFormKind);
  btnGrowToolAdd.addEventListener('click',()=>{
    if(!currentGrowDetailId){growToolMsg.textContent='Nenhum grow selecionado.';return;}
    const toolsMap=currentGrowToolsMap();
    const list=toolsMap[currentGrowDetailId]||[];
    list.push({id:'t'+Date.now(),kind:growToolKind.value,data:readToolPayload()});
    toolsMap[currentGrowDetailId]=list;
    saveGrowTools(toolsMap);
    growToolMsg.textContent='Utensilio adicionado.';
    renderGrowTools();
  });

  btnRefreshPlants.addEventListener('click',async()=>{try{await refreshData();plantsMsg.textContent='Lista atualizada.';}catch(_){plantsMsg.textContent='Falha ao carregar plantas.';}});
  btnCreatePlant.addEventListener('click',async()=>{
    try{const r=await api.createPlant(plantName.value,plantSpecies.value,plantDate.value,plantGrow.value);plantsCache=r.plants||[];plantName.value='';plantSpecies.value='';plantDate.value='';renderPlants();plantsMsg.textContent='Planta criada.';}
    catch(e){plantsMsg.textContent='Erro: '+(e&&e.message?e.message:'create_failed');}
  });
  plantsList.addEventListener('click',async(ev)=>{
    const d=ev.target.closest('[data-plant-delete]');if(!d)return;
    try{const r=await api.deletePlant(d.dataset.plantDelete);plantsCache=r.plants||[];renderPlants();plantsMsg.textContent='Planta removida.';}
    catch(e){plantsMsg.textContent='Erro: '+(e&&e.message?e.message:'delete_failed');}
  });

  btnRefreshCycles.addEventListener('click',async()=>{try{await loadCycles();cyclesMsg.textContent='Lista atualizada.';}catch(_){cyclesMsg.textContent='Falha ao carregar ciclos.';}});
  btnNewCycle.addEventListener('click',async()=>{await loadGrows();show('cycleStep1');cyclesMsg.textContent='Assistente iniciado: Passo 1/4.';refreshCycleWizardState();});
  btnCycleBack.addEventListener('click',()=>{show('cycles');});
  cycleGrowList.addEventListener('click',(ev)=>{
    const btn=ev.target.closest('[data-cycle-grow-id]');
    if(!btn) return;
    toggleCycleGrowSelection(btn.getAttribute('data-cycle-grow-id')||'');
  });
  btnCycleRefreshGrows.addEventListener('click',async()=>{
    try{await loadGrows();cycleWizardMsg.textContent='Ambientes atualizados.';}
    catch(_){cycleWizardMsg.textContent='Falha ao atualizar ambientes.';}
  });
  btnCycleCreateGrowStart.addEventListener('click',()=>{cycleCreateFlowActive=true;cycleWizardMsg.textContent='Crie um ambiente na tela Grows.';show('grows');growsMsg.textContent='Fluxo do assistente: crie um ambiente para voltar ao ciclo.';});

  loadCycleDraft();
  renderSubtypeOptions();
  renderToolFormKind();
  (async()=>{
    const first=await refreshDashboard();
    await refreshData();
    if(first.online&&!first.sensorsReady){setTimeout(()=>{refreshDashboard();},1100);}
  })();
</script>
</body>
</html>
)HTML";

uint8_t WebUi::clamp_percent_(int v) {
  if (v < 0) return 0;
  if (v > 100) return 100;
  return static_cast<uint8_t>(v);
}

void WebUi::setup_wifi_() {
  const bool hasSta = wifi_sta_ssid && wifi_sta_ssid[0] != '\0';

  if (hasSta) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(wifi_sta_ssid, wifi_sta_pass);
    const unsigned long t0 = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t0 < 8000) {
      delay(200);
    }
    if (WiFi.status() == WL_CONNECTED) {
      Serial.print("WiFi STA connected. IP: ");
      Serial.println(WiFi.localIP());
      return;
    }
  }

  WiFi.mode(WIFI_AP);
  WiFi.softAP(wifi_ap_ssid, wifi_ap_pass);
  Serial.print("WiFi AP started. SSID: ");
  Serial.println(wifi_ap_ssid);
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());
}

void WebUi::setup_routes_() {
  auto send_error_json = [this](int status, const String& error) {
    String json = "{\"ok\":false,\"error\":\"";
    json += error;
    json += "\"}";
    server_.send(status, "application/json", json);
  };

  auto build_state_json = [this]() -> String {
    const uint8_t p = getPercent_ ? getPercent_() : 0;
    const SensorReading inside = getInside_ ? getInside_() : SensorReading{};
    const SensorReading outside = getOutside_ ? getOutside_() : SensorReading{};
    const char* mode = getMode_ ? getMode_() : "MANUAL";
    const char* autoState = getAutoState_ ? getAutoState_() : "DEFAULT";

    String json = "{";
    json += "\"fan\":{\"exhaust_percent\":";
    json += String(p);
    json += "},";
    json += "\"mode\":\"";
    json += mode;
    json += "\",";
    json += "\"auto\":{\"state\":\"";
    json += autoState;
    json += "\"},";
    json += "\"sensors\":{";
    json += "\"inside\":{";
    json += "\"temperature_c\":";
    json += String(inside.temperature_c, 1);
    json += ",\"humidity_rh\":";
    json += String(static_cast<int>(lroundf(inside.humidity_rh)));
    json += ",\"ok\":";
    json += (inside.ok ? "true" : "false");
    json += "},";
    json += "\"outside\":{";
    json += "\"temperature_c\":";
    json += String(outside.temperature_c, 1);
    json += ",\"humidity_rh\":";
    json += String(static_cast<int>(lroundf(outside.humidity_rh)));
    json += ",\"ok\":";
    json += (outside.ok ? "true" : "false");
    json += "}";
    json += "}";
    json += "}";
    return json;
  };

  server_.on("/", HTTP_GET, [this]() {
    server_.send(200, "text/html; charset=utf-8", INDEX_HTML);
  });

  server_.on("/api/state", HTTP_GET, [this, build_state_json]() {
    server_.send(200, "application/json", build_state_json());
  });

  server_.on("/api/mode/set", HTTP_GET, [this]() {
    if (!server_.hasArg("mode")) {
      server_.send(400, "application/json", "{\"ok\":false,\"error\":\"missing mode\"}");
      return;
    }
    if (!setMode_ || !setMode_(server_.arg("mode"))) {
      server_.send(400, "application/json", "{\"ok\":false,\"error\":\"invalid mode\"}");
      return;
    }

    const char* mode = getMode_ ? getMode_() : "MANUAL";
    const char* autoState = getAutoState_ ? getAutoState_() : "DEFAULT";
    const uint8_t p = getPercent_ ? getPercent_() : 0;
    String json = "{";
    json += "\"ok\":true,";
    json += "\"mode\":\"";
    json += mode;
    json += "\",";
    json += "\"auto\":{\"state\":\"";
    json += autoState;
    json += "\"},";
    json += "\"fan\":{\"exhaust_percent\":";
    json += String(p);
    json += "}}";
    server_.send(200, "application/json", json);
  });

  server_.on("/api/fan/exhaust/set", HTTP_GET, [this]() {
    if (!server_.hasArg("percent")) {
      server_.send(400, "application/json", "{\"ok\":false,\"error\":\"missing percent\"}");
      return;
    }
    const int raw = server_.arg("percent").toInt();
    const uint8_t p = clamp_percent_(raw);
    const bool applied = setPercent_ ? setPercent_(p) : false;

    if (!applied) {
      const char* mode = getMode_ ? getMode_() : "MANUAL";
      const uint8_t current = getPercent_ ? getPercent_() : 0;
      String json = "{";
      json += "\"ok\":false,";
      json += "\"error\":\"manual_only\",";
      json += "\"mode\":\"";
      json += mode;
      json += "\",";
      json += "\"fan\":{\"exhaust_percent\":";
      json += String(current);
      json += "}}";
      server_.send(409, "application/json", json);
      return;
    }

    const uint8_t current = getPercent_ ? getPercent_() : p;
    String json = "{";
    json += "\"ok\":true,";
    json += "\"fan\":{\"exhaust_percent\":";
    json += String(current);
    json += "}}";
    server_.send(200, "application/json", json);
  });

  server_.on("/api/grows", HTTP_GET, [this, send_error_json]() {
    if (!getGrowsJson_) {
      send_error_json(500, "grows_unavailable");
      return;
    }
    server_.send(200, "application/json", getGrowsJson_());
  });

  server_.on("/api/grow/create", HTTP_GET, [this, send_error_json]() {
    if (!createGrow_ || !getGrowsJson_) {
      send_error_json(500, "grows_unavailable");
      return;
    }
    if (!server_.hasArg("name") || !server_.hasArg("type") ||
        !server_.hasArg("width_cm") || !server_.hasArg("depth_cm") || !server_.hasArg("height_cm")) {
      send_error_json(400, "missing_args");
      return;
    }
    String error;
    const bool ok = createGrow_(
      server_.arg("name"),
      server_.arg("type"),
      server_.arg("width_cm").toInt(),
      server_.arg("depth_cm").toInt(),
      server_.arg("height_cm").toInt(),
      error
    );
    if (!ok) {
      send_error_json(400, error);
      return;
    }
    server_.send(200, "application/json", getGrowsJson_());
  });

  server_.on("/api/grow/update", HTTP_GET, [this, send_error_json]() {
    if (!updateGrow_ || !getGrowsJson_) {
      send_error_json(500, "grows_unavailable");
      return;
    }
    if (!server_.hasArg("id") || !server_.hasArg("name") || !server_.hasArg("type") ||
        !server_.hasArg("width_cm") || !server_.hasArg("depth_cm") || !server_.hasArg("height_cm")) {
      send_error_json(400, "missing_args");
      return;
    }
    String error;
    const bool ok = updateGrow_(
      server_.arg("id"),
      server_.arg("name"),
      server_.arg("type"),
      server_.arg("width_cm").toInt(),
      server_.arg("depth_cm").toInt(),
      server_.arg("height_cm").toInt(),
      error
    );
    if (!ok) {
      send_error_json(400, error);
      return;
    }
    server_.send(200, "application/json", getGrowsJson_());
  });

  server_.on("/api/grow/delete", HTTP_GET, [this, send_error_json]() {
    if (!deleteGrow_ || !getGrowsJson_) {
      send_error_json(500, "grows_unavailable");
      return;
    }
    if (!server_.hasArg("id")) {
      send_error_json(400, "missing_id");
      return;
    }
    String error;
    if (!deleteGrow_(server_.arg("id"), error)) {
      send_error_json(400, error);
      return;
    }
    server_.send(200, "application/json", getGrowsJson_());
  });

  server_.on("/api/grow/active/set", HTTP_GET, [this, send_error_json]() {
    if (!setActiveGrow_ || !getGrowsJson_) {
      send_error_json(500, "grows_unavailable");
      return;
    }
    if (!server_.hasArg("id")) {
      send_error_json(400, "missing_id");
      return;
    }
    String error;
    if (!setActiveGrow_(server_.arg("id"), error)) {
      send_error_json(400, error);
      return;
    }
    server_.send(200, "application/json", getGrowsJson_());
  });

  server_.on("/api/plants", HTTP_GET, [this, send_error_json]() {
    if (!getPlantsJson_) {
      send_error_json(500, "plants_unavailable");
      return;
    }
    server_.send(200, "application/json", getPlantsJson_());
  });

  server_.on("/api/plant/create", HTTP_GET, [this, send_error_json]() {
    if (!createPlant_ || !getPlantsJson_) {
      send_error_json(500, "plants_unavailable");
      return;
    }
    if (!server_.hasArg("name") || !server_.hasArg("species") ||
        !server_.hasArg("germination_date") || !server_.hasArg("grow_id")) {
      send_error_json(400, "missing_args");
      return;
    }
    String error;
    if (!createPlant_(server_.arg("name"), server_.arg("species"),
                      server_.arg("germination_date"), server_.arg("grow_id"), error)) {
      send_error_json(400, error);
      return;
    }
    server_.send(200, "application/json", getPlantsJson_());
  });

  server_.on("/api/plant/update", HTTP_GET, [this, send_error_json]() {
    if (!updatePlant_ || !getPlantsJson_) {
      send_error_json(500, "plants_unavailable");
      return;
    }
    if (!server_.hasArg("id") || !server_.hasArg("name") || !server_.hasArg("species") ||
        !server_.hasArg("germination_date") || !server_.hasArg("grow_id")) {
      send_error_json(400, "missing_args");
      return;
    }
    String error;
    if (!updatePlant_(server_.arg("id"), server_.arg("name"), server_.arg("species"),
                      server_.arg("germination_date"), server_.arg("grow_id"), error)) {
      send_error_json(400, error);
      return;
    }
    server_.send(200, "application/json", getPlantsJson_());
  });

  server_.on("/api/plant/delete", HTTP_GET, [this, send_error_json]() {
    if (!deletePlant_ || !getPlantsJson_) {
      send_error_json(500, "plants_unavailable");
      return;
    }
    if (!server_.hasArg("id")) {
      send_error_json(400, "missing_id");
      return;
    }
    String error;
    if (!deletePlant_(server_.arg("id"), error)) {
      send_error_json(400, error);
      return;
    }
    server_.send(200, "application/json", getPlantsJson_());
  });

  server_.on("/api/species", HTTP_GET, [this, send_error_json]() {
    if (!getSpeciesJson_) {
      send_error_json(500, "species_unavailable");
      return;
    }
    server_.send(200, "application/json", getSpeciesJson_());
  });

  server_.on("/api/species/create", HTTP_GET, [this, send_error_json]() {
    if (!createSpecies_ || !getSpeciesJson_) {
      send_error_json(500, "species_unavailable");
      return;
    }
    if (!server_.hasArg("name")) {
      send_error_json(400, "missing_args");
      return;
    }

    String error;
    if (!createSpecies_(
          server_.arg("name"),
          server_.hasArg("veg_days") ? server_.arg("veg_days").toInt() : 0,
          server_.hasArg("flora_days") ? server_.arg("flora_days").toInt() : 0,
          server_.hasArg("stretch_medio") ? server_.arg("stretch_medio").toFloat() : 0.0f,
          server_.hasArg("veg_height_cm") ? server_.arg("veg_height_cm").toInt() : 0,
          server_.hasArg("flora_height_cm") ? server_.arg("flora_height_cm").toInt() : 0,
          error)) {
      send_error_json(400, error);
      return;
    }
    server_.send(200, "application/json", getSpeciesJson_());
  });

  server_.on("/api/species/update", HTTP_GET, [this, send_error_json]() {
    if (!updateSpecies_ || !getSpeciesJson_) {
      send_error_json(500, "species_unavailable");
      return;
    }
    if (!server_.hasArg("id") || !server_.hasArg("name")) {
      send_error_json(400, "missing_args");
      return;
    }

    String error;
    if (!updateSpecies_(
          server_.arg("id"),
          server_.arg("name"),
          server_.hasArg("veg_days") ? server_.arg("veg_days").toInt() : 0,
          server_.hasArg("flora_days") ? server_.arg("flora_days").toInt() : 0,
          server_.hasArg("stretch_medio") ? server_.arg("stretch_medio").toFloat() : 0.0f,
          server_.hasArg("veg_height_cm") ? server_.arg("veg_height_cm").toInt() : 0,
          server_.hasArg("flora_height_cm") ? server_.arg("flora_height_cm").toInt() : 0,
          error)) {
      send_error_json(400, error);
      return;
    }
    server_.send(200, "application/json", getSpeciesJson_());
  });

  server_.on("/api/species/delete", HTTP_GET, [this, send_error_json]() {
    if (!deleteSpecies_ || !getSpeciesJson_) {
      send_error_json(500, "species_unavailable");
      return;
    }
    if (!server_.hasArg("id")) {
      send_error_json(400, "missing_id");
      return;
    }
    String error;
    if (!deleteSpecies_(server_.arg("id"), error)) {
      send_error_json(400, error);
      return;
    }
    server_.send(200, "application/json", getSpeciesJson_());
  });

  server_.on("/api/cycles", HTTP_GET, [this, send_error_json]() {
    if (!getCyclesJson_) {
      send_error_json(500, "cycles_unavailable");
      return;
    }
    server_.send(200, "application/json", getCyclesJson_());
  });

  server_.on("/api/cycle/create", HTTP_GET, [this, send_error_json]() {
    if (!createCycle_ || !getCyclesJson_) {
      send_error_json(500, "cycles_unavailable");
      return;
    }
    if (!server_.hasArg("name") || !server_.hasArg("grow_id") ||
        !server_.hasArg("plant_ids") || !server_.hasArg("start_datetime")) {
      send_error_json(400, "missing_args");
      return;
    }

    String error;
    if (!createCycle_(
          server_.arg("name"),
          server_.arg("grow_id"),
          server_.arg("plant_ids"),
          server_.arg("start_datetime"),
          server_.hasArg("phase") ? server_.arg("phase") : "PLANNED",
          server_.hasArg("veg_days") ? server_.arg("veg_days").toInt() : 0,
          server_.hasArg("flora_days") ? server_.arg("flora_days").toInt() : 0,
          server_.hasArg("stretch_assumed") ? server_.arg("stretch_assumed").toFloat() : 0.0f,
          error)) {
      send_error_json(400, error);
      return;
    }
    server_.send(200, "application/json", getCyclesJson_());
  });

  server_.on("/api/cycle/update", HTTP_GET, [this, send_error_json]() {
    if (!updateCycle_ || !getCyclesJson_) {
      send_error_json(500, "cycles_unavailable");
      return;
    }
    if (!server_.hasArg("id") || !server_.hasArg("name") || !server_.hasArg("grow_id") ||
        !server_.hasArg("plant_ids") || !server_.hasArg("start_datetime")) {
      send_error_json(400, "missing_args");
      return;
    }

    String error;
    if (!updateCycle_(
          server_.arg("id"),
          server_.arg("name"),
          server_.arg("grow_id"),
          server_.arg("plant_ids"),
          server_.arg("start_datetime"),
          server_.hasArg("phase") ? server_.arg("phase") : "PLANNED",
          server_.hasArg("veg_days") ? server_.arg("veg_days").toInt() : 0,
          server_.hasArg("flora_days") ? server_.arg("flora_days").toInt() : 0,
          server_.hasArg("stretch_assumed") ? server_.arg("stretch_assumed").toFloat() : 0.0f,
          error)) {
      send_error_json(400, error);
      return;
    }
    server_.send(200, "application/json", getCyclesJson_());
  });

  server_.on("/api/cycle/delete", HTTP_GET, [this, send_error_json]() {
    if (!deleteCycle_ || !getCyclesJson_) {
      send_error_json(500, "cycles_unavailable");
      return;
    }
    if (!server_.hasArg("id")) {
      send_error_json(400, "missing_id");
      return;
    }
    String error;
    if (!deleteCycle_(server_.arg("id"), error)) {
      send_error_json(400, error);
      return;
    }
    server_.send(200, "application/json", getCyclesJson_());
  });

  server_.onNotFound([this]() {
    server_.send(404, "text/plain", "Not found");
  });

  server_.begin();
  Serial.println("Web server started on port 80.");
}

void WebUi::begin(GetPercentFn getPercent, SetPercentFn setPercent,
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
                  CycleDeleteFn deleteCycle) {
  getPercent_ = getPercent;
  setPercent_ = setPercent;
  getInside_ = getInside;
  getOutside_ = getOutside;
  getMode_ = getMode;
  setMode_ = setMode;
  getAutoState_ = getAutoState;
  getGrowsJson_ = getGrowsJson;
  getPlantsJson_ = getPlantsJson;
  getSpeciesJson_ = getSpeciesJson;
  getCyclesJson_ = getCyclesJson;
  createGrow_ = createGrow;
  updateGrow_ = updateGrow;
  deleteGrow_ = deleteGrow;
  setActiveGrow_ = setActiveGrow;
  createPlant_ = createPlant;
  updatePlant_ = updatePlant;
  deletePlant_ = deletePlant;
  createSpecies_ = createSpecies;
  updateSpecies_ = updateSpecies;
  deleteSpecies_ = deleteSpecies;
  createCycle_ = createCycle;
  updateCycle_ = updateCycle;
  deleteCycle_ = deleteCycle;

  setup_wifi_();
  setup_routes_();
}

void WebUi::loop() {
  server_.handleClient();
}
