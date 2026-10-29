#pragma once
#include <pgmspace.h>
// Single-file dashboard in flash; no internet, CDN, filesystem, or OLED required.
static const char WEB_UI[] PROGMEM = R"WEBPAGE(<!doctype html>
<html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1"><meta name="theme-color" content="#15191f">
<title>Quad Controller</title><style>
:root{color-scheme:dark;--bg:#15191f;--panel:#242b35;--well:#1c222b;--edge:#3b4652;--text:#e5ebf4;--muted:#a6b5c6;--accent:#39a4d7;--orange:#faaf40;--red:#ed6868;--green:#75dca0}
*{box-sizing:border-box}body{background:var(--bg);color:var(--text);font:16px system-ui,-apple-system,sans-serif;margin:0}header{background:#11151b;border-bottom:2px solid var(--accent);padding:16px 20px}header h1{margin:0;font-size:1.4rem;letter-spacing:.03em}header small{color:var(--muted)}main{max-width:900px;margin:auto;padding:15px}section{background:var(--panel);border:1px solid var(--edge);border-radius:9px;padding:17px;margin-bottom:15px;box-shadow:0 2px 5px #0004}h2{color:var(--accent);font-size:1.08rem;margin:0 0 14px}.tiles{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:9px}.tile{background:var(--well);border-radius:6px;padding:11px}.tile b{font-size:1.36rem;display:block}.tile small{color:var(--muted)}.warning{color:var(--orange)!important}.error{color:var(--red)!important}.ok{color:var(--green)}.row{display:grid;grid-template-columns:minmax(0,1fr) minmax(95px,190px);gap:10px;align-items:center;margin:11px 0}label{line-height:1.35}label small{display:block;color:var(--muted);font-size:.76rem}input,select{color:var(--text);background:var(--well);border:1px solid #586374;border-radius:5px;padding:10px;width:100%;font:inherit}button{border:none;border-radius:6px;color:white;background:var(--accent);font:600 1rem system-ui;padding:13px;cursor:pointer;touch-action:none}button:disabled{opacity:.55}#hold{background:#297f4e;width:100%;padding:22px;font-size:1.25rem;user-select:none;-webkit-user-select:none}#hold:active,#hold.active{background:#57ac74}#stop{background:#9c3939;margin:8px 0;width:100%}.notice{margin:10px 0;padding:12px;border-left:4px solid var(--orange);background:#332a22;color:#ffcf89;border-radius:3px}.minor{color:var(--muted);font-size:.84rem}.buttons{display:grid;grid-template-columns:1fr 1fr;gap:10px}.buttons button{background:#404c59}.buttons button.selected{background:var(--accent)}#save{width:100%}footer{color:var(--muted);padding:0 0 18px;font-size:.8rem}@media(min-width:680px){.tiles{grid-template-columns:repeat(4,minmax(0,1fr))}.fields{display:grid;grid-template-columns:1fr 1fr;gap:0 24px}}
</style></head><body><header><h1>QUAD <span style="color:var(--accent)">CONTROL</span></h1><small>ESP8266 &middot; local access point &middot; no internet required</small></header><main>
<section><h2>Live status</h2><div id="alerts" class="notice" hidden></div><div class="tiles">
<div class="tile"><small>State</small><b id="state">Connecting</b></div>
<div class="tile"><small>Drive</small><b id="direction">--</b></div>
<div class="tile"><small>Output</small><b id="output">--</b></div>
<div class="tile"><small>Battery</small><b id="battery">--</b></div>
<div class="tile"><small>Charger voltage lockout</small><b id="voltage-status">--</b></div>
<div class="tile"><small>Motor input current</small><b id="amps">--</b></div>
<div class="tile"><small>Motor sensor</small><b id="motor">--</b></div>
<div class="tile"><small>Gearbox sensor</small><b id="gearbox">--</b></div>
<div class="tile"><small>Throttle</small><b id="source">--</b></div>
<div class="tile"><small>Motor input power</small><b id="watts">--</b></div>
<div class="tile"><small>5-minute average</small><b id="recent">--</b></div>
<div class="tile"><small>Peak motor current</small><b id="peak">--</b></div>
<div class="tile"><small>Current-limited cap</small><b id="derate-cap">--</b></div>
<div class="tile"><small>Charge current</small><b id="charge-rate">Not wired</b></div>
<div class="tile"><small>Motor energy used</small><b id="energy">--</b></div>
<div class="tile"><small>Battery remaining</small><b id="soc">--</b></div>
<div class="tile"><small>Estimated runtime</small><b id="runtime">--</b></div>
</div><p class="minor" id="details">Loading diagnostic data...</p>
<h2 style="margin-top:17px">Live motor-branch current</h2>
<svg id="current-chart" role="img" aria-label="Recent motor current samples" viewBox="0 0 640 150" preserveAspectRatio="none" style="width:100%;height:150px;background:var(--well);border-radius:6px" xmlns="http://www.w3.org/2000/svg"><path id="current-trace" stroke="#39a4d7" stroke-width="2" fill="none" vector-effect="non-scaling-stroke"/></svg>
<p class="minor" id="chart-caption">Waiting for current readings. Samples are kept on this phone only (about 2 minutes).</p>
<p class="minor">Voltage-based charge % is an estimate; motor load lowers the voltage and charging raises it. Runtime uses that % with battery capacity and the last 5 minutes of motor current. Charging current is not measured.</p></section>
<section><h2>Wi-Fi driving</h2><p class="minor">Choose a direction and hold DRIVE. Release to stop immediately. The ESP8266 also stops remote power if the phone stops sending updates. Physical pedal has priority. Switch must not be center-OFF.</p>
<div class="buttons"><button type="button" id="fwd" class="selected">Forward</button><button type="button" id="rev">Reverse</button></div>
<div style="height:12px"></div><button id="hold" type="button">HOLD TO DRIVE</button><button id="stop" type="button">STOP</button><div id="remote-msg" class="minor" role="status">Not driving</div></section>
<section><h2>Settings</h2><form id="config-form"><div class="fields">
<div class="row"><label for="ramp">Ramp time <small>Seconds, 0.1 to 30</small></label><input type="number" id="ramp" min="0.1" max="30" step="0.1" required></div>
<div class="row"><label for="forward">Maximum forward voltage <small>Equivalent average, 0.5 to 12V</small></label><input type="number" id="forward" min="0.5" max="12" step="0.1" required></div>
<div class="row"><label for="reverse">Maximum reverse voltage <small>Equivalent average, 0.5 to 12V</small></label><input type="number" id="reverse" min="0.5" max="12" step="0.1" required></div>
<div class="row"><label for="motor-rom">Motor probe <small>Unique DS18B20 address</small></label><select id="motor-rom"></select></div>
<div class="row"><label for="motor-cut">Motor high-temp cutoff <small>&deg;C, limits output to 5V equivalent</small></label><input type="number" id="motor-cut" min="20" max="120" step="1" required></div>
<div class="row"><label for="gear-rom">Gearbox probe <small>Unique DS18B20 address</small></label><select id="gear-rom"></select></div>
<div class="row"><label for="gear-cut">Gearbox high-temp cutoff <small>&deg;C, limits output to 5V equivalent</small></label><input type="number" id="gear-cut" min="20" max="120" step="1" required></div>
<div class="row"><label for="emergency-a">Emergency current cutoff <small>Amps, 0 disables. First measured sample at or above threshold cuts immediately (25ms poll nominal; no hardware guarantee).</small></label><input type="number" id="emergency-a" min="0" max="50" step="0.1" required></div>
<div class="row"><label for="derate-a">Sustained load derating threshold <small>Amps, 0 disables. Above this for the delay, PWM cap drops 2V/s. When enabled, set below stall and emergency cutoff.</small></label><input type="number" id="derate-a" min="0" max="50" step="0.1" required></div>
<div class="row"><label for="derate-s">Derating delay <small>Seconds above threshold before limiting; 0.2 to 5.</small></label><input type="number" id="derate-s" min="0.2" max="5" step="0.1" required></div>
<div class="row"><label for="derate-min">Minimum derated voltage <small>Equivalent volts, 0.5 to 12; recovery below 90% of threshold is 1V/s.</small></label><input type="number" id="derate-min" min="0.5" max="12" step="0.1" required></div>
<div class="row"><label for="stall-a">Stall current threshold <small>Amps, 0 disables. Sustained over threshold cuts PWM. Calibrate on the bench first.</small></label><input type="number" id="stall-a" min="0" max="50" step="0.1" required></div>
<div class="row"><label for="stall-s">Stall trip delay <small>Seconds continuously over threshold, 0.2 to 5</small></label><input type="number" id="stall-s" min="0.2" max="5" step="0.1" required></div>
<div class="row"><label for="no-load-a">Possible no-load threshold <small>Amps, 0 disables. Low current at ≥20% PWM can be normal; not true traction detection.</small></label><input type="number" id="no-load-a" min="0" max="20" step="0.1" required></div>
<div class="row"><label for="no-load-s">No-load trip delay <small>Seconds continuously below threshold, 0.5 to 10</small></label><input type="number" id="no-load-s" min="0.5" max="10" step="0.1" required></div>
<div class="row"><label for="battery-full">Fully charged voltage <small>12.8V default; measure with battery at rest</small></label><input type="number" id="battery-full" min="10" max="15.5" step="0.05" required></div>
<div class="row"><label for="battery-empty">Empty voltage <small>11.8V default; measure with battery at rest</small></label><input type="number" id="battery-empty" min="9" max="13.5" step="0.05" required></div>
<div class="row"><label for="capacity-ah">Battery rated capacity <small>Ah from label, used for runtime estimate</small></label><input type="number" id="capacity-ah" min="1" max="100" step="0.1" required></div>
<div class="row"><label for="charger-cutoff">Charger voltage lockout <small>Volts, default 13.0; re-arms after 5s below cutoff minus 0.2V.</small></label><input type="number" id="charger-cutoff" min="12.5" max="15.5" step="0.1" required></div>
</div><h2>Wi-Fi</h2><div class="fields">
<div class="row"><label for="ap-ssid">ATV access point name <small>1–32 characters; reconnect after saving</small></label><input id="ap-ssid" maxlength="32" required></div>
<div class="row"><label for="ap-password">New ATV Wi-Fi password <small>8–63 characters; blank keeps current</small></label><input id="ap-password" type="password" minlength="8" maxlength="63" autocomplete="new-password" placeholder="Keep existing password"></div>
<div class="row"><label for="ota-ssid">Internet Wi-Fi name for OTA <small>2.4GHz network, used only during OTA; blank disables</small></label><input id="ota-ssid" maxlength="32"></div>
<div class="row"><label for="ota-password">Internet Wi-Fi password <small>8–63 characters; blank keeps current</small></label><input id="ota-password" type="password" minlength="8" maxlength="63" autocomplete="new-password" placeholder="Keep existing password"></div>
</div><button type="submit" id="save">Save settings</button><p class="minor" id="save-msg" role="status">Settings are stored in ESP8266 flash.</p></form></section>
<section><h2>Firmware update</h2>
<p class="minor">Stop the motor and release the throttle. GitHub OTA needs the internet Wi-Fi above. Updates reboot the ATV. You can also download the binary on your phone and upload it here without internet on the ATV.</p>
<p><a id="firmware-link" href="https://github.com/iragm/atv/releases/download/firmware-latest/quad-controller-d1-mini.bin" target="_blank" rel="noopener" style="color:var(--accent)">Latest firmware (.bin)</a></p>
<button type="button" id="ota-url">Install latest firmware from GitHub</button>
<p class="minor" id="ota-status" role="status">Idle</p>
<div class="row"><label for="ota-file">Upload downloaded .bin instead</label><input id="ota-file" type="file" accept=".bin,application/octet-stream"></div>
<button type="button" id="ota-upload">Install selected .bin</button>
</section><footer>Connect to the QUAD Wi-Fi network. If the portal doesn't open automatically, visit <b>http://192.168.4.1</b>. HTTP is local to this password-protected AP.</footer></main>
<script>
'use strict';const $=id=>document.getElementById(id);let driveDir='fwd',held=false,timer=null,loaded=false,lastStatus=0;
const api=async(path,init)=>{const r=await fetch(path,{cache:'no-store',...init});if(!r.ok)throw Error(await r.text());return r.json()};
function drive(on){fetch('/api/drive',{method:'POST',headers:{'X-Quad-Control':'1','Content-Type':'application/x-www-form-urlencoded'},body:'hold='+(on?'1':'0')+'&direction='+driveDir,keepalive:!on}).catch(()=>{});}
function stop(){if(timer!==null){clearInterval(timer);timer=null;}if(held){held=false;$('hold').classList.remove('active');drive(false);} $('remote-msg').textContent='Not driving';}
function start(e){e.preventDefault();if(held)return;held=true;drive(true);$('hold').classList.add('active');$('remote-msg').textContent='Commanding motor while held';timer=setInterval(()=>{if(held)drive(true)},180);if(e.pointerId!==undefined)$('hold').setPointerCapture(e.pointerId);}
$('hold').addEventListener('pointerdown',start);for(const event of ['pointerup','pointercancel','lostpointercapture'])$('hold').addEventListener(event,stop);
$('stop').addEventListener('click',stop);window.addEventListener('blur',stop);document.addEventListener('visibilitychange',()=>{if(document.hidden)stop()});
function choose(d){stop();driveDir=d;$('fwd').classList.toggle('selected',d==='fwd');$('rev').classList.toggle('selected',d==='rev')}
$('fwd').addEventListener('click',()=>choose('fwd'));$('rev').addEventListener('click',()=>choose('rev'));
const text=(id,value)=>{$(id).textContent=value};
const currentSamples=[];
function updateChart(amps){
 const now=Date.now();currentSamples.push([now,Number.isFinite(amps)?Math.max(0,amps):null]);
 while(currentSamples.length>180)currentSamples.shift();
 const valid=currentSamples.map(v=>v[1]).filter(Number.isFinite);
 if(!valid.length){$('current-trace').setAttribute('d','');text('chart-caption','Current monitor not available');return;}
 const max=Math.max(1,Math.ceil(Math.max(...valid)*1.1));
 const first=currentSamples[0][0],span=Math.max(1000,now-first);
 let path='',open=false;
 for(const [t,a] of currentSamples){if(a===null){open=false;continue;}
 const x=((t-first)/span*636+2).toFixed(1),y=(140-a/max*130).toFixed(1);
 path+=(open?' L':' M')+x+','+y;open=true;
 }
 $('current-trace').setAttribute('d',path);
 text('chart-caption','Last '+Math.round(span/1000)+'s · graph range 0 to '+max+'A · local phone memory');
}
function fmt(v,units,digits=1){return v===null?'Not detected':Number(v).toFixed(digits)+units}
async function status(){try{const s=await api('/api/status');lastStatus=Date.now();text('state',s.state);text('direction',s.direction);text('output',s.duty_percent.toFixed(0)+'%');text('battery',fmt(s.bus_volts,' V',2));text('voltage-status',s.voltage_inhibit?'LOCKED':'Ready');text('ota-status',s.ota_status);text('amps',fmt(s.motor_amps,' A'));text('motor',fmt(s.motor_temp,' °C'));text('gearbox',fmt(s.gearbox_temp,' °C'));text('source',s.source);text('watts',fmt(s.motor_watts,' W'));text('peak',fmt(s.peak_motor_amps,' A'));text('derate-cap',s.current_derating?fmt(s.current_limit_v,' V'):'Not limiting');updateChart(s.motor_amps);text('recent',s.recent_seconds<30?'Collecting data':fmt(s.recent_avg_amps,' A'));text('energy',fmt(s.motor_used_wh,' Wh',2));text('charge-rate',s.charging_current_supported?fmt(s.charging_amps,' A'):'Not wired');text('soc',fmt(s.estimated_charge_pct,' %'));text('runtime',s.estimated_remaining_min===null?'Calculating…':Math.floor(Math.round(s.estimated_remaining_min)/60)+'h '+(Math.round(s.estimated_remaining_min)%60)+'m');text('details','Voltage inhibit: '+(s.voltage_inhibit?'active':'clear')+' · ADC: '+s.adc+' · INA226: '+(s.ina_available?'found':'missing')+' · OLED: '+(s.oled_available?'found':'missing')+' · Probes: '+s.probes.length+' · Supply for PWM: '+s.pwm_supply_volts.toFixed(2)+'V'+(s.ina_available?'':' (12V assumed)'));let msgs=[];if(s.voltage_inhibit)msgs.push('HIGH VOLTAGE / CHARGER? Motor disabled.');if(s.ota_busy)msgs.push('OTA in progress: motor disabled');if(s.motor_hot)msgs.push('MOTOR HIGH TEMP: output capped at 5V equivalent');if(s.gearbox_hot)msgs.push('GEARBOX HIGH TEMP: output capped at 5V equivalent');if(s.fault!=='None')msgs.push('Control fault: '+s.fault);if(s.current_derating)msgs.push('SUSTAINED LOAD: output cap reduced to '+s.current_limit_v.toFixed(1)+'V equivalent');if(!s.ina_available){msgs.push('INA226 missing: charger voltage inhibit and current protection unavailable; duty uses nominal 12V');}if(s.ina_available&&s.stall_monitor_enabled===false)msgs.push('Stall cutoff disabled until calibrated');if(s.ina_available&&!s.emergency_monitor_enabled)msgs.push('Emergency current cutoff disabled until calibrated');if(s.no_load_monitor_enabled)msgs.push('No-load detection is heuristic; may trip with no traction or light load');if(!s.motor_sensor_found||!s.gearbox_sensor_found)msgs.push('One or more selected temperature probes unavailable; no derate from missing probes');$('alerts').hidden=!msgs.length;$('alerts').textContent=msgs.join(' · ');}
catch(e){if(Date.now()-lastStatus>1400){text('state','Offline');$('alerts').hidden=false;$('alerts').textContent='Connection lost. Remote throttle will time out.';stop();}}}
function probeOptions(select,probes,current){select.replaceChildren();const empty=document.createElement('option');empty.value='';empty.textContent='None';select.appendChild(empty);for(const p of probes){const opt=document.createElement('option');opt.value=p.rom;opt.textContent=p.rom;select.appendChild(opt)}if(current&&!probes.some(p=>p.rom===current)){const opt=document.createElement('option');opt.value=current;opt.textContent=current+' (missing)';select.appendChild(opt)}select.value=current||'';}
async function load(){try{const s=await api('/api/config');$('ramp').value=s.ramp_s;$('forward').value=s.forward_v;$('reverse').value=s.reverse_v;$('motor-cut').value=s.motor_cutoff_c;$('gear-cut').value=s.gearbox_cutoff_c;$('emergency-a').value=s.emergency_trip_a;$('derate-a').value=s.derate_trip_a;$('derate-s').value=s.derate_trip_ms/1000;$('derate-min').value=s.derate_minimum_v;$('stall-a').value=s.stall_trip_a;$('stall-s').value=s.stall_trip_ms/1000;$('no-load-a').value=s.no_load_below_a;$('no-load-s').value=s.no_load_trip_ms/1000;$('capacity-ah').value=s.battery_capacity_ah;$('battery-full').value=s.battery_full_v;$('battery-empty').value=s.battery_empty_v;$('charger-cutoff').value=s.charger_cutoff_v;$('ap-ssid').value=s.ap_ssid;$('ota-ssid').value=s.ota_ssid;$('firmware-link').href=s.ota_url;probeOptions($('motor-rom'),s.probes,s.motor_sensor);probeOptions($('gear-rom'),s.probes,s.gearbox_sensor);loaded=true;}catch(e){text('save-msg','Could not load settings: '+e.message)}}
$('config-form').addEventListener('submit',async e=>{e.preventDefault();if(!loaded)return;stop();const params=new URLSearchParams({ramp_s:$('ramp').value,forward_v:$('forward').value,reverse_v:$('reverse').value,motor_sensor:$('motor-rom').value,gearbox_sensor:$('gear-rom').value,motor_cutoff_c:$('motor-cut').value,gearbox_cutoff_c:$('gear-cut').value,emergency_trip_a:$('emergency-a').value,derate_trip_a:$('derate-a').value,derate_trip_s:$('derate-s').value,derate_minimum_v:$('derate-min').value,stall_trip_a:$('stall-a').value,stall_trip_s:$('stall-s').value,no_load_below_a:$('no-load-a').value,no_load_trip_s:$('no-load-s').value,battery_capacity_ah:$('capacity-ah').value,battery_full_v:$('battery-full').value,battery_empty_v:$('battery-empty').value,charger_cutoff_v:$('charger-cutoff').value,ap_ssid:$('ap-ssid').value,ap_password:$('ap-password').value,ota_ssid:$('ota-ssid').value,ota_password:$('ota-password').value});try{await api('/api/config',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded','X-Quad-Control':'1'},body:params});text('save-msg','Saved. Wi-Fi changes restart the ATV; reconnect with the new name/password.');$('ap-password').value='';$('ota-password').value='';}catch(err){text('save-msg','Save failed: '+err.message)}});
$('ota-url').addEventListener('click',async()=>{stop();if(!confirm('Stop the motor and install latest firmware from GitHub?'))return;try{const result=await api('/api/ota/url',{method:'POST',headers:{'X-Quad-Control':'1'}});text('ota-status',result.message+'; reconnect after update');}catch(e){text('ota-status','OTA not started: '+e.message)}});
$('ota-upload').addEventListener('click',async()=>{stop();const file=$('ota-file').files[0];if(!file){text('ota-status','Select a .bin firmware file');return;}if(!confirm('Install '+file.name+' and reboot?'))return;
try{const data=new FormData();data.append('firmware',file,file.name);const response=await fetch('/api/ota/upload',{method:'POST',headers:{'X-Quad-Control':'1'},body:data});const message=await response.text();text('ota-status',message);if(!response.ok)throw Error(message);}catch(e){text('ota-status','Upload failed: '+e.message)}});
load();status();setInterval(status,600);
</script></body></html>)WEBPAGE";
