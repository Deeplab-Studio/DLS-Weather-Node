#pragma once
// Cihaz web arayuzu (PROGMEM). Harici font/CDN yok: internet olmadan da calisir.

static const char WEBUI_LOGIN[] PROGMEM = R"HTMLLOGIN(<!DOCTYPE html>
<html lang="tr"><head>
<meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<meta name="color-scheme" content="light dark">
<title>DLS Weather - Giriş</title>
<style>
:root{--bg:#f6f7f9;--card:#fff;--fg:#111827;--muted:#5b6675;--line:#dfe4ea;--field:#7d8a9c;--acc:#0f766e;--acc2:#14b8a6;--err:#b91c1c}
@media (prefers-color-scheme:dark){:root{--bg:#0a1120;--card:#141d2c;--fg:#e9f0f9;--muted:#a3b3c8;--line:#2a3a55;--field:#5f7396;--acc:#2dd4bf;--acc2:#5eead4;--err:#fca5a5}}
*{box-sizing:border-box}
body{margin:0;min-height:100vh;display:flex;align-items:center;justify-content:center;background:var(--bg);color:var(--fg);
font:15px/1.55 -apple-system,BlinkMacSystemFont,"Segoe UI",Roboto,Helvetica,Arial,sans-serif;padding:24px}
.card{width:100%;max-width:360px;background:var(--card);border:1px solid var(--line);border-radius:16px;padding:26px 22px}
.brand{display:flex;align-items:center;gap:11px;margin-bottom:20px}
.mark{width:36px;height:36px;border-radius:11px;background:linear-gradient(135deg,var(--acc),var(--acc2));
display:flex;align-items:center;justify-content:center;color:#fff;font-weight:700;font-size:13px;letter-spacing:.5px}
h1{font-size:15px;margin:0}
.sub{color:var(--muted);font-size:12px;margin:2px 0 0}
label{display:block;font-size:12px;font-weight:600;color:var(--muted);margin:0 0 6px}
input{width:100%;padding:14px;border:1px solid var(--field);border-radius:11px;background:var(--card);color:var(--fg);
font-size:20px;letter-spacing:6px;text-align:center;font-variant-numeric:tabular-nums}
input::placeholder{letter-spacing:6px;color:var(--muted);opacity:1}
input:focus{outline:none;border-color:var(--acc);box-shadow:0 0 0 3px rgba(13,148,136,.18)}
button{width:100%;margin-top:16px;padding:14px;border:0;border-radius:11px;background:var(--acc);color:#08201d;font-size:15px;
font-weight:700;cursor:pointer}
@media (prefers-color-scheme:light){button{color:#fff}}
button:active{transform:translateY(1px)}
.err{margin:12px 0 0;color:var(--err);font-size:13px;min-height:18px}
.hint{margin:14px 0 0;color:var(--muted);font-size:12px;line-height:1.7}
.hint code{background:rgba(127,127,127,.16);padding:1px 5px;border-radius:5px}
</style></head><body>
<form class="card" method="POST" action="/login">
  <div class="brand"><div class="mark">DLS</div>
    <div><h1>DLS Weather Node</h1><p class="sub">%HOST%</p></div></div>
  <label for="pin">Cihaz PIN kodu (6 hane)</label>
  <input id="pin" name="pin" inputmode="numeric" autocomplete="off" maxlength="6" pattern="[0-9]{6}"
         placeholder="••••••" required autofocus>
  <button type="submit">Giriş yap</button>
  <p class="err">%ERROR%</p>
  <p class="hint">PIN kodu cihazın OLED ekranında ve seri portta görünür
    (<code>pin</code> komutu). 5 hatalı denemeden sonra 1 dakika kilit uygulanır.</p>
</form></body></html>
)HTMLLOGIN";

static const char WEBUI_PAGE[] PROGMEM = R"HTMLPAGE(<!DOCTYPE html>
<html lang="tr"><head>
<meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<meta name="color-scheme" content="light dark">
<title>DLS Weather Node</title>
<style>
:root{--bg:#f6f7f9;--card:#fff;--fg:#0f172a;--muted:#5b6675;--line:#e2e7ee;--field:#7d8a9c;--acc:#0f766e;--acc2:#14b8a6;
--soft:#eef7f6;--warn:#a16207;--err:#b91c1c;--ok:#0f9d58}
@media (prefers-color-scheme:dark){:root{--bg:#0a1120;--card:#141d2c;--fg:#e9f0f9;--muted:#a3b3c8;--line:#2a3a55;--field:#5f7396;
--acc:#2dd4bf;--acc2:#5eead4;--soft:#16283a;--warn:#fbbf24;--err:#fca5a5;--ok:#4ade80}}
*{box-sizing:border-box}
body{margin:0;background:var(--bg);color:var(--fg);font:15px/1.55 -apple-system,BlinkMacSystemFont,"Segoe UI",Roboto,Helvetica,Arial,sans-serif}
.wrap{max-width:600px;margin:0 auto;padding:18px 16px 40px}
@media (min-width:900px){.wrap{max-width:760px}}
header{display:flex;align-items:center;gap:12px;margin:6px 0 18px}
.mark{width:38px;height:38px;border-radius:11px;background:linear-gradient(135deg,var(--acc),var(--acc2));color:#fff;
display:flex;align-items:center;justify-content:center;font-weight:700;font-size:12px;letter-spacing:.5px;flex:0 0 auto}
.htxt{min-width:0}
.htxt h1{font-size:16px;margin:0;white-space:nowrap;overflow:hidden;text-overflow:ellipsis}
.htxt p{margin:1px 0 0;font-size:12px;color:var(--muted)}
.pill{margin-left:auto;font-size:11.5px;font-weight:700;padding:6px 11px;border-radius:999px;background:var(--soft);color:var(--acc);
white-space:nowrap;border:1px solid transparent}
.pill.off{background:#fdecec;color:#a41c1c;border-color:#f7d4d4}
@media (prefers-color-scheme:dark){.pill.off{background:#3a1418;color:#ffc9c9;border-color:#5d1f26}}
nav{display:flex;gap:6px;background:var(--soft);padding:4px;border-radius:12px;margin-bottom:16px}
nav button{flex:1;border:0;background:transparent;color:var(--muted);font-size:14px;font-weight:600;padding:12px;border-radius:9px;cursor:pointer}
nav button.on{background:var(--card);color:var(--fg);box-shadow:0 1px 3px rgba(2,10,25,.14)}
.grid{display:grid;grid-template-columns:repeat(2,1fr);gap:10px}
@media (min-width:520px){.grid{grid-template-columns:repeat(4,1fr)}}
.tile{background:var(--card);border:1px solid var(--line);border-radius:14px;padding:12px 13px}
.tile span{display:block;font-size:11px;color:var(--muted);text-transform:uppercase;letter-spacing:.05em;font-weight:600}
.tile strong{display:block;font-size:20px;font-weight:650;margin-top:6px;font-variant-numeric:tabular-nums;line-height:1.2}
.tile strong small{font-size:12px;font-weight:500;color:var(--muted);margin-left:3px}
.card{background:var(--card);border:1px solid var(--line);border-radius:14px;padding:14px 15px;margin-top:12px}
.rows{display:grid;gap:9px}
.row{display:flex;justify-content:space-between;gap:14px;font-size:13px;align-items:baseline}
.row b{font-weight:600;text-align:right}
.row span{color:var(--muted);flex:0 0 auto}
.row b.wrapv{white-space:normal;word-break:break-word}
fieldset{border:0;padding:0;margin:0 0 20px;min-width:0}
@media (min-width:900px){form#f{display:grid;grid-template-columns:1fr 1fr;gap:0 22px}form#f .actions{grid-column:1/-1}}
legend{font-size:11.5px;font-weight:700;color:var(--muted);text-transform:uppercase;letter-spacing:.07em;padding:0;margin-bottom:10px}
label{display:block;font-size:12px;font-weight:600;color:var(--muted);margin:13px 0 5px}
input,select{width:100%;padding:12px;border:1px solid var(--field);border-radius:10px;background:var(--card);color:var(--fg);font-size:15px}
input:focus,select:focus{outline:none;border-color:var(--acc);box-shadow:0 0 0 3px rgba(13,148,136,.18)}
input::placeholder{color:var(--muted);opacity:1}
.two{display:flex;gap:10px;flex-wrap:wrap}
.two>div{flex:1 1 140px;min-width:0}
.help{font-size:11.5px;color:var(--muted);margin:5px 0 0}
.actions{display:flex;gap:8px;margin-top:18px}
.card .actions{margin-top:0}
.btn{flex:1;padding:13px;border:0;border-radius:11px;background:var(--acc);color:#fff;font-size:15px;font-weight:650;cursor:pointer;
text-align:center;text-decoration:none;line-height:1.35}
@media (prefers-color-scheme:dark){.btn{color:#08201d}}
.btn.ghost{background:transparent;border:1px solid var(--field);color:var(--fg)}
@media (prefers-color-scheme:dark){.btn.ghost{color:var(--fg)}}
.btn.danger{background:transparent;border:1px solid var(--field);color:var(--fg)}
.btn:disabled{opacity:.55}
#toast{position:fixed;left:50%;bottom:24px;transform:translate(-50%,20px);opacity:0;transition:.25s;
background:#111827;color:#fff;padding:11px 16px;border-radius:10px;font-size:13px;max-width:88%;pointer-events:none;z-index:9}
#toast.show{opacity:1;transform:translate(-50%,0)}
#toast.err{background:#b91c1c}
.switch{display:flex;align-items:center;justify-content:space-between;gap:12px;margin-top:16px;
min-height:44px;padding:6px 0;cursor:pointer;user-select:none}
label.switch{margin:16px 0 0}
.switch input{width:22px;height:22px;flex:0 0 auto;accent-color:var(--acc);cursor:pointer}
</style></head><body>
<div class="wrap">
  <header>
    <div class="mark">DLS</div>
    <div class="htxt"><h1>%DEVNAME%</h1><p>%STATION% &middot; %IP%</p></div>
    <div class="pill" id="pill">Bağlanıyor</div>
  </header>

  <nav>
    <button id="tab-dash" class="on" type="button">Durum</button>
    <button id="tab-cfg" type="button">Ayarlar</button>
  </nav>

  <section id="dash">
    <div class="grid">
      <div class="tile"><span>Sıcaklık</span><strong id="v-temp">--</strong></div>
      <div class="tile"><span>Nem</span><strong id="v-hum">--</strong></div>
      <div class="tile"><span>Basınç</span><strong id="v-pres">--</strong></div>
      <div class="tile"><span>Gaz</span><strong id="v-gas">--</strong></div>
      <div class="tile"><span>Rüzgar</span><strong id="v-wind">--</strong></div>
      <div class="tile"><span>Yön</span><strong id="v-dir">--</strong></div>
      <div class="tile"><span>Yağış</span><strong id="v-rain">--</strong></div>
      <div class="tile"><span>Pil</span><strong id="v-bat">--</strong></div>
    </div>
    <div class="card"><div class="rows">
      <div class="row"><span>Sensörler</span><b class="wrapv" id="v-sensors">%SENSORS%</b></div>
      <div class="row"><span>Son gönderim</span><b id="v-send">--</b></div>
      <div class="row"><span>Sinyal</span><b id="v-rssi">--</b></div>
      <div class="row"><span>Çalışma süresi</span><b id="v-up">--</b></div>
      <div class="row"><span>Yazılım</span><b>%VERSION%</b></div>
    </div></div>
    <div class="card"><div class="actions">
      <a class="btn ghost" href="/api/weather" target="_blank" rel="noopener">JSON verisi</a>
      <button class="btn danger" id="btn-restart" type="button">Yeniden başlat</button>
    </div></div>
  </section>

  <section id="cfg" hidden>
    <form id="f" autocomplete="off">
      <fieldset><legend>Ağ</legend>
        <label for="ssid">WiFi adı (SSID)</label>
        <input id="ssid" name="ssid" maxlength="32" required value="%SSID%" autocomplete="off">
        <label for="pass">WiFi şifresi</label>
        <input id="pass" name="pass" type="password" maxlength="64" autocomplete="new-password" placeholder="%PASSPH%">
        <p class="help">Boş bırakırsanız kayıtlı şifre korunur.</p>
      </fieldset>

      <fieldset><legend>Cihaz</legend>
        <label for="api">API anahtarı</label>
        <input id="api" name="api" type="password" maxlength="128" placeholder="%APIPH%" value="">
        <p class="help">Boş bırakırsanız kayıtlı anahtar korunur.</p>
        <label for="station">İstasyon ID</label>
        <input id="station" name="station" maxlength="32" required value="%STATIONV%" pattern="[A-Za-z0-9_-]+">
        <p class="help">Harf, rakam, "-" ve "_" kullanılabilir.</p>
        <label for="alias">Cihaz adı (opsiyonel)</label>
        <input id="alias" name="alias" maxlength="32" value="%ALIAS%">
      </fieldset>

      <fieldset><legend>Konum</legend>
        <div class="two">
          <div><label for="lat">Enlem</label>
            <input id="lat" name="lat" inputmode="decimal" required value="%LAT%" placeholder="41.0082"></div>
          <div><label for="lon">Boylam</label>
            <input id="lon" name="lon" inputmode="decimal" required value="%LON%" placeholder="28.9784"></div>
        </div>
        <p class="help">Ondalık derece. Enlem -90…90, boylam -180…180.</p>
      </fieldset>

      <fieldset><legend>Ölçüm ve güç</legend>
        <label for="interval">Gönderim aralığı (dakika)</label>
        <select id="interval" name="interval">%INTERVAL_OPTS%</select>
        <label for="txPower">WiFi çıkış gücü</label>
        <select id="txPower" name="txPower">%TX_OPTS%</select>
        <label class="switch" for="deepSleep">
          <span><b style="font-size:14px">Derin uyku</b>
            <span class="help" style="display:block;margin:2px 0 0">Pil ile çalışırken açın</span></span>
          <input id="deepSleep" name="deepSleep" type="checkbox" value="1" %DS_CHECKED%>
        </label>
        <label for="webpin">Web PIN (değiştirmek için)</label>
        <input id="webpin" name="webpin" inputmode="numeric" maxlength="6" pattern="[0-9]{6}" placeholder="6 hane">
        <p class="help">Boş bırakırsanız PIN değişmez.</p>
      </fieldset>

      <div class="actions">
        <button class="btn" id="save" type="submit">Kaydet</button>
      </div>
    </form>
  </section>
</div>
<div id="toast"></div>
<script>
var $=function(s){return document.querySelector(s)};
function toast(m,err){var t=$('#toast');t.textContent=m;t.className=err?'show err':'show';
clearTimeout(t._h);t._h=setTimeout(function(){t.className=''},3200)}
function tab(which){var dash=which==='dash';$('#dash').hidden=!dash;$('#cfg').hidden=dash;
$('#tab-dash').className=dash?'on':'';$('#tab-cfg').className=dash?'':'on'}
$('#tab-dash').onclick=function(){tab('dash')};
$('#tab-cfg').onclick=function(){tab('cfg')};
function num(v,d){return (v===null||v===undefined)?'--':(Math.round(v*Math.pow(10,d))/Math.pow(10,d))}
function up(s){
  if(!s)return'--';
  var d=Math.floor(s/86400),h=Math.floor(s%86400/3600),m=Math.floor(s%3600/60);
  if(d>0)return d+' g '+h+' sa';
  if(h>0)return h+' sa '+m+' dk';
  return m+' dk';
}
function load(){
 fetch('/api/status',{cache:'no-store'}).then(function(r){return r.json()}).then(function(s){
   $('#pill').textContent=s.wifi?'Çevrimiçi':'Çevrimdışı';
   $('#pill').className=s.wifi?'pill':'pill off';
   $('#v-send').textContent=s.lastSend||'--';
   $('#v-rssi').textContent=s.wifi?(s.rssi+' dBm'):'--';
   $('#v-up').textContent=up(s.uptime);
   $('#v-bat').innerHTML=(s.bat===null||s.bat<0)?'--':(s.bat+'<small>%</small>');
   if(s.sensors)$('#v-sensors').textContent=s.sensors;
 }).catch(function(){});
 fetch('/api/weather',{cache:'no-store'}).then(function(r){return r.json()}).then(function(w){
   $('#v-temp').innerHTML=w.temperature===null?'--':num(w.temperature,1)+'<small>°C</small>';
   $('#v-hum').innerHTML=w.humidity===null?'--':num(w.humidity,0)+'<small>%</small>';
   $('#v-pres').innerHTML=w.pressure===null?'--':num(w.pressure,0)+'<small>hPa</small>';
   $('#v-gas').innerHTML=w.air_quality===null?'--':num(w.air_quality,1)+'<small>kΩ</small>';
   $('#v-wind').innerHTML=w.wind_speed===null?'--':num(w.wind_speed,1)+'<small>m/s</small>';
   $('#v-dir').innerHTML=w.wind_dir===null?'--':num(w.wind_dir,0)+'<small>°</small>';
   $('#v-rain').innerHTML=w.rain_daily===null?'--':num(w.rain_daily,1)+'<small>mm</small>';
 }).catch(function(){});
}
load();setInterval(load,10000);
$('#f').onsubmit=function(e){
 e.preventDefault();
 var b=$('#save');b.disabled=true;b.textContent='Kaydediliyor...';
 var body=new URLSearchParams(new FormData(e.target)).toString();
 fetch('/save',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:body})
  .then(function(r){return r.json()}).then(function(j){
    if(j.ok){toast('Kaydedildi, cihaz yeniden başlıyor...')}
    else{toast(j.error||'Hata',true);b.disabled=false;b.textContent='Kaydet'}
  }).catch(function(){toast('Bağlantı hatası',true);b.disabled=false;b.textContent='Kaydet'});
};
$('#btn-restart').onclick=function(){
 if(!confirm('Cihaz yeniden başlatılsın mı?'))return;
 fetch('/restart',{method:'POST'}).then(function(){toast('Yeniden başlatılıyor...')}).catch(function(){});
};
</script>
</body></html>
)HTMLPAGE";
