/*
  Plan Lutowania - ESP32 + karta SD + strona WWW
  ------------------------------------------------------------
  Rysujesz na stronie schemat polaczen (piny ESP -> TFT / moduly),
  drukujesz go z lista przewodow do lutowania. Schematy zapisuja sie
  na karcie SD w folderze /plan (pliki .json).

  Jeden plik .ino, tylko biblioteki z pakietu ESP32 (bez instalowania
  czegokolwiek): WiFi, WebServer, SD, SPI, ESPmDNS.

  Uzycie:
   1. Wpisz nazwe i haslo swojej sieci WiFi (nizej).
   2. Wgraj na plytke, otworz Monitor portu (115200) - zobaczysz adres IP.
   3. Wpisz ten adres w przegladarce (albo http://plan.local).
   Jesli WiFi sie nie polaczy, plytka uruchomi wlasna siec
   "PlanLutowania" (haslo: lutowanie) i strona bedzie pod 192.168.4.1
*/

#include <WiFi.h>
#include <WebServer.h>
#include <SPI.h>
#include <SD.h>
#include <ESPmDNS.h>

// ---------- USTAWIENIA ----------
const char* WIFI_SSID = "TWOJA_SIEC";
const char* WIFI_PASS = "TWOJE_HASLO";

// Piny karty SD (takie same jak w projektach "Magazyn czesci" / "Moje programy")
#define SD_CS   21
#define SD_SCK  36
#define SD_MOSI 35
#define SD_MISO 37
// ---------------------------------

SPIClass sdSPI(HSPI);
WebServer server(80);
bool sdOk = false;

// ===================== STRONA WWW (w pamieci programu) =====================
const char PAGE_1[] PROGMEM = R"PLAN(<!doctype html>
<html lang="pl"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>Plan Lutowania</title>
<style>
html,body{margin:0}
:root{
  color-scheme:light;
  --bg:#e3eaee; --panel:#f6f9fb; --panel2:#eaf0f4; --ink:#10202b; --ink2:#43586a; --ink3:#71828e;
  --line:#c9d4db; --accent:#0a7f90; --accent-ink:#ffffff; --accent-soft:rgba(10,127,144,.13);
  --warn:#a8480a; --warn-bg:#fbeadb; --good:#1f7a3a; --danger:#b3261e;
  --shadow:0 1px 2px rgba(16,32,43,.10),0 8px 24px rgba(16,32,43,.12);
  --sans:'IBM Plex Sans',system-ui,-apple-system,'Segoe UI',Arial,sans-serif;
  --mono:'IBM Plex Mono',ui-monospace,Menlo,Consolas,monospace;
  --head:'Barlow Condensed','Arial Narrow',Arial,sans-serif;
}
@media (prefers-color-scheme: dark){
  :root:not([data-theme="light"]){
    color-scheme:dark;
    --bg:#0b1218; --panel:#131d25; --panel2:#1a2731; --ink:#e4eef3; --ink2:#a4b7c3; --ink3:#71889a;
    --line:#26363f; --accent:#3cc4d6; --accent-ink:#04222a; --accent-soft:rgba(60,196,214,.16);
    --warn:#f0a15c; --warn-bg:#33231a; --good:#5cc27a; --danger:#ff8a80;
    --shadow:0 1px 2px rgba(0,0,0,.4),0 8px 24px rgba(0,0,0,.45);
  }
}
:root[data-theme="dark"]{
  color-scheme:dark;
  --bg:#0b1218; --panel:#131d25; --panel2:#1a2731; --ink:#e4eef3; --ink2:#a4b7c3; --ink3:#71889a;
  --line:#26363f; --accent:#3cc4d6; --accent-ink:#04222a; --accent-soft:rgba(60,196,214,.16);
  --warn:#f0a15c; --warn-bg:#33231a; --good:#5cc27a; --danger:#ff8a80;
  --shadow:0 1px 2px rgba(0,0,0,.4),0 8px 24px rgba(0,0,0,.45);
}
*,*::before,*::after{box-sizing:border-box}
[hidden]{display:none!important}
html,body{height:100%}
body{background:var(--bg);color:var(--ink);font:13px/1.45 var(--sans);overflow:hidden}
button,input,select,textarea{font:inherit;color:inherit}
button{cursor:pointer}
h1,h2,h3,p{margin:0}
:focus-visible{outline:2px solid var(--accent);outline-offset:2px}

#app{height:100%;display:flex;flex-direction:column;min-width:0}

/* ---------- top bar ---------- */
.bar{display:flex;flex-wrap:wrap;align-items:center;gap:8px 14px;padding:8px 16px;background:var(--panel);border-bottom:1px solid var(--line)}
.brand{font:700 21px/1 var(--head);letter-spacing:.04em;text-transform:uppercase;display:flex;align-items:center;gap:8px;white-space:nowrap}
.brand i{width:12px;height:12px;border-radius:50%;background:#d4a23a;box-shadow:0 0 0 2px var(--panel),0 0 0 3.5px var(--ink);display:inline-block}
.grow{flex:1 1 40px}
.sep{width:1px;align-self:stretch;background:var(--line);margin:2px 0}
.name{width:clamp(150px,22vw,260px);height:30px;border:1px solid transparent;background:var(--panel2);border-radius:6px;padding:0 10px;font-weight:500}
.name:hover{border-color:var(--line)}
.btn{height:30px;padding:0 11px;border:1px solid var(--line);background:var(--panel);border-radius:6px;display:inline-flex;align-items:center;gap:6px;white-space:nowrap;font-weight:500}
.btn:hover{background:var(--panel2)}
.btn:disabled{opacity:.4;cursor:default}
.btn.primary{background:var(--accent);border-color:var(--accent);color:var(--accent-ink)}
.btn.primary:hover{filter:brightness(1.08)}
.btn.danger{color:var(--danger)}
.btn.icon{width:30px;padding:0;justify-content:center}
.btn.sm{height:26px;padding:0 9px;font-size:12px}
.seg{display:inline-flex;border:1px solid var(--line);border-radius:6px;overflow:hidden}
.seg button{height:28px;padding:0 10px;border:0;background:var(--panel);border-right:1px solid var(--line)}
.seg button:last-child{border-right:0}
.seg button.on{background:var(--accent);color:var(--accent-ink)}
.chk{display:inline-flex;align-items:center;gap:6px;cursor:pointer;user-select:none}
.chk input{accent-color:var(--accent);width:15px;height:15px;margin:0}
.only-narrow{display:none}

/* ---------- workspace ---------- */
.work{position:relative;flex:1 1 auto;min-height:0;display:flex}
.panel{width:252px;flex:0 0 252px;background:var(--panel);display:flex;flex-direction:column;min-height:0}
#lib{border-right:1px solid var(--line)}
#side{border-left:1px solid var(--line);width:296px;flex-basis:296px}
.ph{padding:14px 16px 10px;display:flex;flex-direction:column;gap:10px}
.ph h2,.sec h3{font:700 15px/1 var(--head);letter-spacing:.09em;text-transform:uppercase;color:var(--ink2)}
input[type=search],.fld{width:100%;height:32px;border:1px solid var(--line);background:var(--bg);border-radius:6px;padding:0 10px}
.libscroll{flex:1 1 auto;overflow:auto;padding:0 16px 12px}
.cat{margin-top:12px}
.cat h3{font:700 13px/1 var(--head);letter-spacing:.1em;text-transform:uppercase;color:var(--ink3);margin-bottom:6px;display:flex;align-items:center;gap:8px}
.cat h3 span{width:10px;height:10px;border-radius:2px;border:1px solid var(--ink3)}
.item{display:flex;width:100%;text-align:left;align-items:center;gap:8px;padding:6px 8px;border:1px solid transparent;border-radius:6px;background:transparent}
.item:hover{background:var(--panel2);border-color:var(--line)}
.item b{font-weight:500;flex:1 1 auto;min-width:0}
.item small{color:var(--ink3);font-family:var(--mono);font-size:11px;white-space:nowrap}
.pf{padding:12px 16px;border-top:1px solid var(--line);display:flex;gap:8px;flex-wrap:wrap}

#stage{position:relative;flex:1 1 auto;min-width:0;background-color:#f3f6f8;overflow:hidden;
  background-image:linear-gradient(#c7d3db 1px,transparent 1px),linear-gradient(90deg,#c7d3db 1px,transparent 1px),linear-gradient(#dfe7ec 1px,transparent 1px),linear-gradient(90deg,#dfe7ec 1px,transparent 1px);
  background-size:100px 100px,100px 100px,20px 20px,20px 20px}
#cv{position:absolute;inset:0;width:100%;height:100%;display:block;touch-action:none;user-select:none;-webkit-user-select:none}
.zoom{position:absolute;right:14px;bottom:14px;display:flex;align-items:center;gap:2px;background:var(--panel);border:1px solid var(--line);border-radius:8px;padding:3px;box-shadow:var(--shadow)}
.zoom span{min-width:46px;text-align:center;font:500 12px var(--mono);color:var(--ink2)}
.stagebtns{position:absolute;left:12px;top:12px;display:none;gap:8px}
.empty{position:absolute;left:50%;top:50%;transform:translate(-50%,-50%);width:min(420px,calc(100% - 32px));background:var(--panel);border:1px solid var(--line);border-radius:10px;padding:18px 20px;box-shadow:var(--shadow);display:flex;flex-direction:column;gap:10px}
.empty h2{font:700 20px/1.1 var(--head);letter-spacing:.05em;text-transform:uppercase}
.empty p{color:var(--ink2)}

#stage.back{background-color:#f6f0dc;background-image:linear-gradient(#d6c9a4 1px,transparent 1px),linear-gradient(90deg,#d6c9a4 1px,transparent 1px),linear-gradient(#e8dfc4 1px,transparent 1px),linear-gradient(90deg,#e8dfc4 1px,transparent 1px)}
.backbadge{position:absolute;left:50%;top:12px;transform:translateX(-50%);background:var(--warn-bg);color:var(--warn);border:1px solid var(--warn);border-radius:6px;padding:4px 12px;font:700 14px/1.2 var(--head);letter-spacing:.1em;text-transform:uppercase;pointer-events:none;white-space:nowrap;max-width:calc(100% - 24px);overflow:hidden;text-overflow:ellipsis}
.btn.on{background:var(--accent);border-color:var(--accent);color:var(--accent-ink)}
.partbar{position:absolute;transform:translateX(-100%);display:flex;gap:6px;z-index:4;padding:3px;background:var(--panel);border:1px solid var(--line);border-radius:8px;box-shadow:var(--shadow)}

/* ---------- side panel ---------- */
.tabs{display:flex;border-bottom:1px solid var(--line)}
.tabs button{flex:1;height:38px;border:0;background:transparent;font:700 14px var(--head);letter-spacing:.09em;text-transform:uppercase;color:var(--ink3);border-bottom:2px solid transparent}
.tabs button.on{color:var(--ink);border-bottom-color:var(--accent)}
.tabs em{font:500 11px var(--mono);font-style:normal;background:var(--panel2);border-radius:9px;padding:1px 7px;margin-left:4px;letter-spacing:0}
#sideBody{flex:1 1 auto;overflow:auto;padding:4px 16px 16px}
.sec{padding:14px 0;border-bottom:1px solid var(--line);display:flex;flex-direction:column;gap:10px}
.sec:last-child{border-bottom:0}
.lbl{font-size:11px;font-weight:600;letter-spacing:.06em;text-transform:uppercase;color:var(--ink3)}
.field{display:flex;flex-direction:column;gap:4px}
.ends{display:grid;grid-template-columns:1fr auto 1fr;gap:8px;align-items:center}
.end{background:var(--panel2);border-radius:6px;padding:7px 9px;min-width:0}
.end b{display:block;font-family:var(--mono);font-weight:600;overflow-wrap:anywhere}
.end small{color:var(--ink3);display:block;overflow-wrap:anywhere}
.sw{display:grid;grid-template-columns:repeat(6,1fr);gap:7px}
.sw button{aspect-ratio:1;border-radius:50%;border:2px solid var(--line);padding:0;position:relative}
.sw button.on{outline:2px solid var(--accent);outline-offset:2px}
.sw button[data-c=white]{border-color:var(--ink3)}
.row2{display:flex;gap:8px;flex-wrap:wrap}
.meter{height:6px;background:var(--panel2);border-radius:3px;overflow:hidden}
.meter i{display:block;height:100%;background:var(--good)}
.stat{display:flex;justify-content:space-between;gap:8px;font-family:var(--mono);font-size:12px}
.warn{background:var(--warn-bg);color:var(--warn);border-radius:6px;padding:8px 10px;font-size:12px}
.help{color:var(--ink2);font-size:12.5px;display:flex;flex-direction:column;gap:6px}
.help kbd{font:500 11px var(--mono);background:var(--panel2);border:1px solid var(--line);border-radius:4px;padding:0 5px}
.wl{list-style:none;margin:0;padding:0;display:flex;flex-direction:column;gap:2px}
.wl li{display:flex;align-items:center;gap:8px;padding:5px 6px;border-radius:6px;cursor:pointer;border:1px solid transparent}
.wl li:hover{background:var(--panel2)}
.wl li.on{background:var(--accent-soft);border-color:var(--accent)}
.wl input{accent-color:var(--good);width:15px;height:15px;margin:0;flex:none}
.wl .n{font:600 11px var(--mono);width:22px;text-align:right;color:var(--ink2);flex:none}
.wl .c{width:12px;height:12px;border-radius:50%;border:1.5px solid var(--ink);flex:none}
.wl .t{font:12px var(--mono);min-width:0;overflow-wrap:anywhere}
.wl li.done .t{color:var(--ink3);text-decoration:line-through}

.status{display:flex;flex-wrap:wrap;gap:4px 18px;padding:5px 16px;background:var(--panel);border-top:1px solid var(--line);font-size:12px;color:var(--ink2)}
.status .r{margin-left:auto;font-family:var(--mono)}
#stPin{font-family:var(--mono);color:var(--ink)}

/* ---------- dialogs / toast ---------- */
dialog{border:1px solid var(--line);border-radius:12px;background:var(--panel);color:var(--ink);padding:0;box-shadow:var(--shadow);max-width:min(980px,94vw);width:min(980px,94vw);max-height:92vh}
dialog.narrow{width:min(620px,94vw)}
dialog::backdrop{background:rgba(5,12,18,.6)}
.dlg{padding:18px 20px;display:flex;flex-direction:column;gap:12px}
.dlg h2{font:700 20px/1.1 var(--head);letter-spacing:.06em;text-transform:uppercase}
.dlg label{display:flex;flex-direction:column;gap:4px;font-size:12px;font-weight:600;color:var(--ink2)}
.dlg input,.dlg textarea{border:1px solid var(--line);background:var(--bg);border-radius:6px;padding:7px 10px;font-weight:400;color:var(--ink)}
.dlg textarea{font-family:var(--mono);font-size:12.5px;resize:vertical;min-height:120px}
.two{display:grid;grid-template-columns:1fr 1fr;gap:12px}
.dlgbtns{display:flex;gap:8px;justify-content:flex-end;flex-wrap:wrap}
.note{color:var(--ink3);font-size:12px}
.pvbar{display:flex;flex-wrap:wrap;gap:8px 10px;align-items:center}
.pvbody{overflow:auto;max-height:calc(92vh - 150px);background:var(--bg);border:1px solid var(--line);border-radius:8px;padding:10px}
.pvbody svg{display:block;width:100%;height:auto;background:#fff;box-shadow:var(--shadow)}
.menu{position:fixed;z-index:30;background:var(--panel);border:1px solid var(--line);border-radius:10px;box-shadow:var(--shadow);padding:6px;width:min(340px,calc(100vw - 24px));max-height:70vh;overflow:auto}
.menu .mi{display:flex;width:100%;align-items:center;gap:8px;padding:7px 10px;border:0;background:transparent;border-radius:6px;text-align:left}
.menu .mi:hover{background:var(--panel2)}
.menu .mi.cur{background:var(--accent-soft)}
.menu .mi small{margin-left:auto;color:var(--ink3);font-family:var(--mono);font-size:11px}
.menu hr{border:0;border-top:1px solid var(--line);margin:6px 0}
.menu .h{padding:4px 10px;font-size:11px;font-weight:600;letter-spacing:.06em;text-transform:uppercase;color:var(--ink3)}
#toast{position:fixed;left:50%;bottom:44px;transform:translateX(-50%);background:var(--ink);color:var(--bg);padding:8px 14px;border-radius:8px;font-size:13px;box-shadow:var(--shadow);z-index:60;max-width:calc(100vw - 32px)}
#printRoot{display:none}

/* ---------- narrow ---------- */
@media (max-width:980px){
  .only-narrow{display:inline-flex}
  .stagebtns{display:flex}
  .panel{position:absolute;top:0;bottom:0;z-index:20;width:min(320px,88%);flex:none;box-shadow:var(--shadow);transition:transform .18s ease}
  #lib{left:0;transform:translateX(-105%)}
  #side{right:0;transform:translateX(105%);width:min(340px,92%)}
  #app.showLib #lib,#app.showSide #side{transform:none}
  .two{grid-template-columns:1fr}
  .hide-narrow{display:none}
}
@media (prefers-reduced-motion:reduce){.panel{transition:none}}

/* ---------- print ---------- */
@media print{
  :root{padding:0!important}
  html,body{height:auto;overflow:visible;background:#fff!important}
  #app,dialog,#toast,.menu{display:none!important}
  #printRoot{display:block}
  #printRoot svg{display:block;width:100%;height:auto;max-height:100vh}
}
</style></head><body>

<div id="app">
  <header class="bar">
    <div class="brand"><i></i>Plan Lutowania</div>
    <input id="pname" class="name" type="text" maxlength="60" aria-label="Nazwa schematu" spellcheck="false">
    <button class="btn" id="btnProj" aria-haspopup="true"><span data-ico="folder"></span>Projekty</button>
    <span class="sep hide-narrow"></span>
    <button class="btn icon" id="btnUndo" data-ico="undo" aria-label="Cofnij" title="Cofnij (Ctrl+Z)"></button>
    <button class="btn icon" id="btnRedo" data-ico="redo" aria-label="Ponów" title="Ponów (Ctrl+Y)"></button>
    <span class="sep hide-narrow"></span>
    <div class="seg" role="group" aria-label="Styl tras przewodów">
      <button data-ws="elbow">Kątowa</button><button data-ws="curve">Krzywa</button><button data-ws="straight">Prosta</button>
    </div>
    <label class="chk"><input type="checkbox" id="optNums"> Numery</label>
    <button class="btn" id="btnBack" aria-pressed="false" title="Pokaż schemat tak, jak wygląda po odwróceniu płytek na drugą stronę (odbicie lustrzane)"><span data-ico="flip"></span>Widok od spodu</button>
    <span class="grow"></span>
    <button class="btn primary" id="btnPrint"><span data-ico="print"></span>Wydruk</button>
  </header>

  <div class="work">
    <aside class="panel" id="lib" aria-label="Biblioteka elementów">
      <div class="ph"><h2>Elementy</h2><input id="q" type="search" placeholder="Szukaj: ESP32, TFT, SD…" aria-label="Szukaj elementu"></div>
      <div class="libscroll" id="libList"></div>
      <div class="pf"><button class="btn" id="btnCustom"><span data-ico="plus"></span>Własny element</button></div>
    </aside>

    <main id="stage">
      <svg id="cv" role="application" aria-label="Arkusz ze schematem połączeń"></svg>
      <div class="stagebtns">
        <button class="btn" id="tgLib"><span data-ico="plus"></span>Elementy</button>
        <button class="btn" id="tgSide"><span data-ico="list"></span>Panel</button>
      </div>
      <div class="partbar" id="partBar" hidden><button class="btn sm" id="pbRot" title="Obróć wybrany element o 90° (klawisz R, Shift+R w drugą stronę)"><span data-ico="rotate"></span>Obróć</button></div>
      <div class="backbadge" id="backBadge" hidden>Widok od spodu · odbicie lustrzane</div>
      <div class="empty" id="empty" hidden>
        <h2>Pusty arkusz</h2>
        <p>Dodaj płytkę i wyświetlacz z listy elementów. Potem przeciągnij od kropki jednego pinu do kropki drugiego, żeby poprowadzić przewód.</p>
        <div class="row2"><button class="btn primary" id="btnSample">Wczytaj przykład</button></div>
      </div>
      <div class="zoom">
        <button class="btn icon" id="zOut" data-ico="minus" aria-label="Oddal"></button>
        <span id="zLbl">100%</span>
        <button class="btn icon" id="zIn" data-ico="plus" aria-label="Przybliż"></button>
        <button class="btn icon" id="zFit" data-ico="fit" aria-label="Dopasuj widok" title="Dopasuj widok"></button>
      </div>
    </main>

    <aside class="panel" id="side" aria-label="Panel właściwości">
      <div class="tabs"><button data-tab="props" class="on">Właściwości</button><button data-tab="list">Lista <em id="cnt">0</em></button></div>
      <div id="sideBody"></div>
    </aside>
  </div>

  <footer class="status">
    <span id="stHint"></span>
    <span id="stPin"></span>
    <span class="r" id="stSave"></span>
  </footer>
</div>

<div id="menu" class="menu" hidden></div>

<dialog id="dlgPart"><form class="dlg" id="fPart">
  <h2 id="dpHead">Edytuj piny</h2>
  <div class="two">
    <label>Nazwa<input id="dpName" maxlength="40"></label>
    <label>Podtytuł<input id="dpSub" maxlength="60"></label>
  </div>
  <label style="max-width:240px">Skrót na liście połączeń<input id="dpShort" maxlength="12"></label>
  <div class="two">
    <label>Piny po lewej (od góry do dołu)<textarea id="dpL" rows="12" spellcheck="false"></textarea></label>
    <label>Piny po prawej (od góry do dołu)<textarea id="dpR" rows="12" spellcheck="false"></textarea></label>
  </div>
  <p class="note">Jeden pin w linii. Opis dodasz po kresce pionowej, np. <code>GPIO23|MOSI</code>. Usunięcie pinu usuwa też jego przewody.</p>
  <div class="dlgbtns"><button type="button" class="btn" id="dpCancel">Anuluj</button><button type="submit" class="btn primary">Zapisz element</button></div>
</form></dialog>

<dialog id="dlgPrint"><div class="dlg">
  <div class="pvbar">
    <h2>Wydruk</h2><span class="grow"></span>
    <div class="seg" role="group" aria-label="Orientacja"><button data-or="landscape">A4 poziomo</button><button data-or="portrait">A4 pionowo</button></div>
    <button class="btn primary" id="pvPrint"><span data-ico="print"></span>Drukuj</button>
    <button class="btn" id="pvPng"><span data-ico="download"></span>PNG</button>
    <button class="btn" id="pvSvg"><span data-ico="download"></span>SVG</button>
    <button class="btn" id="pvCsv"><span data-ico="download"></span>CSV</button>
    <button class="btn" id="pvClose">Zamknij</button>
  </div>
  <div class="pvbody" id="pvBody"></div>
  <p class="note" id="pvNote"></p>
</div></dialog>

<dialog id="dlgJson" class="narrow"><div class="dlg">
  <h2>Kopia projektu (JSON)</h2>
  <p class="note">Skopiuj tekst i zachowaj go w pliku albo wklej tu tekst z innej kopii, żeby wczytać ją jako nowy projekt.</p>
  <textarea id="jsonBox" rows="10" spellcheck="false" aria-label="JSON projektu"></textarea>
  <div class="dlgbtns">
    <button class="btn" id="jsCopy"><span data-ico="copy"></span>Kopiuj</button>
    <button class="btn" id="jsSave"><span data-ico="download"></span>Zapisz plik</button>
    <button class="btn" id="jsLoad">Wczytaj jako nowy</button>
    <button class="btn" id="jsClose">Zamknij</button>
  </div>
</div></dialog>

<div id="toast" hidden></div>
<div id="printRoot"></div>
<script>
'use strict';
/* =====================  helpers  ===================== */
const $ = (s, r = document) => r.querySelector(s);
const $$ = (s, r = document) => Array.from(r.querySelectorAll(s));
const esc = s => String(s).replace(/[&<>"']/g, c => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;' }[c]));
const clamp = (v, a, b) => Math.min(b, Math.max(a, v));
const snap10 = v => Math.round(v / 10) * 10;
const uid = () => Math.random().toString(36).slice(2, 8) + Date.now().toString(36).slice(-3);
const num = (v, d = 0) => (typeof v === 'number' && isFinite(v) ? v : d);

const MONO = "'IBM Plex Mono',ui-monospace,Menlo,Consolas,monospace";
const HEADF = "'Barlow Condensed','Arial Narrow',Arial,sans-serif";
const SANSF = "'IBM Plex Sans',system-ui,Arial,sans-serif";
const INK = '#10202b', INK2 = '#556876', COPPER = '#d4a23a', ACC = '#0a8f9f';

const ICONS = {
  undo: '<path d="M9 14 4 9l5-5"/><path d="M4 9h10.5a5.5 5.5 0 0 1 0 11H11"/>',
  redo: '<path d="m15 14 5-5-5-5"/><path d="M20 9H9.5a5.5 5.5 0 0 0 0 11H13"/>',
  plus: '<path d="M12 5v14M5 12h14"/>',
  minus: '<path d="M5 12h14"/>',
  fit: '<path d="M8 3H5a2 2 0 0 0-2 2v3M21 8V5a2 2 0 0 0-2-2h-3M3 16v3a2 2 0 0 0 2 2h3M16 21h3a2 2 0 0 0 2-2v-3"/>',
  print: '<path d="M6 9V2h12v7"/><path d="M6 18H4a2 2 0 0 1-2-2v-5a2 2 0 0 1 2-2h16a2 2 0 0 1 2 2v5a2 2 0 0 1-2 2h-2"/><rect x="6" y="14" width="12" height="8"/>',
  trash: '<path d="M3 6h18M8 6V4h8v2M19 6l-1 14H6L5 6"/>',
  folder: '<path d="M3 7a2 2 0 0 1 2-2h4l2 2h8a2 2 0 0 1 2 2v9a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2z"/>',
  list: '<path d="M8 6h13M8 12h13M8 18h13M3 6h.01M3 12h.01M3 18h.01"/>',
  edit: '<path d="M12 20h9"/><path d="M16.5 3.5a2.1 2.1 0 0 1 3 3L7 19l-4 1 1-4z"/>',
  rotate: '<path d="M21 12a9 9 0 1 1-3-6.7L21 8"/><path d="M21 3v5h-5"/>',
  flip: '<path d="M8 3 4 7l4 4"/><path d="M4 7h16"/><path d="m16 21 4-4-4-4"/><path d="M20 17H4"/>',
  copy: '<rect x="9" y="9" width="12" height="12" rx="2"/><path d="M5 15V5a2 2 0 0 1 2-2h10"/>',
  download: '<path d="M12 3v12m0 0-4-4m4 4 4-4M4 21h16"/>'
};
const ico = n => `<svg viewBox="0 0 24 24" width="16" height="16" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true">${ICONS[n] || ''}</svg>`;

/* =====================  palette & data  ===================== */
const COLORS = {
  red: ['#d93a2f', 'czerwony'], black: ['#171b1f', 'czarny'], orange: ['#f08a1c', 'pomarańczowy'],
  yellow: ['#f0c81e', 'żółty'], green: ['#2fa14a', 'zielony'], blue: ['#2f6ee0', 'niebieski'],
  violet: ['#8250c8', 'fioletowy'], brown: ['#7d4b2c', 'brązowy'], gray: ['#97a1a8', 'szary'],
  white: ['#ffffff', 'biały'], pink: ['#e25aa0', 'różowy'], cyan: ['#16b1c4', 'turkusowy']
};
const SIGNAL_ORDER = ['orange', 'yellow', 'green', 'blue', 'violet', 'brown', 'pink', 'cyan', 'gray', 'white'];
const CAT = {
  'Płytki MCU': '#cfe3f1', 'Wyświetlacze': '#e1d8f1', 'Czujniki': '#d5ecd8',
  'Moduły': '#f4e3c2', 'Podstawowe': '#e3e7ea', 'Własne': '#f2d9d9'
};
const CAT_ORDER = ['Płytki MCU', 'Wyświetlacze', 'Czujniki', 'Moduły', 'Podstawowe'];

const TPL = [
  { id: 'esp32-30', cat: 'Płytki MCU', title: 'ESP32 DevKit V1', sub: '30 pinów · USB u dołu', short: 'ESP32',
    L: ['EN', 'GPIO36|VP', 'GPIO39|VN', 'GPIO34|IN', 'GPIO35|IN', 'GPIO32', 'GPIO33', 'GPIO25', 'GPIO26', 'GPIO27', 'GPIO14', 'GPIO12', 'GPIO13', 'GND', 'VIN'],
    R: ['GPIO23|MOSI', 'GPIO22|SCL', 'GPIO1|TX0', 'GPIO3|RX0', 'GPIO21|SDA', 'GPIO19|MISO', 'GPIO18|SCK', 'GPIO5|CS', 'GPIO17|TX2', 'GPIO16|RX2', 'GPIO4', 'GPIO2', 'GPIO15', 'GND', '3V3'] },
  { id: 'esp32-38', cat: 'Płytki MCU', title: 'ESP32 DevKitC', sub: '38 pinów · USB u dołu', short: 'ESP32',
    L: ['3V3', 'EN', 'GPIO36|VP', 'GPIO39|VN', 'GPIO34|IN', 'GPIO35|IN', 'GPIO32', 'GPIO33', 'GPIO25', 'GPIO26', 'GPIO27', 'GPIO14', 'GPIO12', 'GND', 'GPIO13', 'SD2|GPIO9', 'SD3|GPIO10', 'CMD|GPIO11', '5V'],
    R: ['GND', 'GPIO23|MOSI', 'GPIO22|SCL', 'GPIO1|TX0', 'GPIO3|RX0', 'GPIO21|SDA', 'GND', 'GPIO19|MISO', 'GPIO18|SCK', 'GPIO5|CS', 'GPIO17', 'GPIO16', 'GPIO4', 'GPIO0', 'GPIO2', 'GPIO15', 'SD1|GPIO8', 'SD0|GPIO7', 'CLK|GPIO6'] },
  { id: 's3mini', cat: 'Płytki MCU', title: 'ESP32-S3 Mini USB-C', sub: '18 pinów · USB u góry', short: 'S3 Mini',
    L: ['TX', 'RX', 'GPIO1', 'GPIO2', 'GPIO3', 'GPIO4', 'GPIO5', 'GPIO6', 'GPIO7'],
    R: ['5V', 'GND', '3V3', 'GPIO13', 'GPIO12', 'GPIO11', 'GPIO10', 'GPIO9', 'GPIO8'] },
  { id: 'esp32mini', cat: 'Płytki MCU', title: 'ESP32 Mini (D1 mini)', sub: 'WROOM-32 · USB-C · 16 pinów · USB u dołu', short: 'ESP32 Mini',
    L: ['RST', 'GPIO36|SVP', 'GPIO26', 'GPIO18|SCK', 'GPIO19|MISO', 'GPIO23|MOSI', 'GPIO5|CS', '3V3'],
    R: ['GPIO1|TXD', 'GPIO3|RXD', 'GPIO22|SCL', 'GPIO21|SDA', 'GPIO17', 'GPIO16', 'GND', '5V|VCC'] },
  { id: 's3devkitc', cat: 'Płytki MCU', title: 'ESP32-S3 DevKitC-1', sub: '44 piny · 2× USB-C · USB u dołu', short: 'S3',
    L: ['3V3', '3V3', 'RST', 'GPIO4', 'GPIO5', 'GPIO6', 'GPIO7', 'GPIO15', 'GPIO16', 'GPIO17', 'GPIO18', 'GPIO8', 'GPIO3', 'GPIO46', 'GPIO9', 'GPIO10', 'GPIO11', 'GPIO12', 'GPIO13', 'GPIO14', '5V', 'GND'],
    R: ['GND', 'TX|GPIO43', 'RX|GPIO44', 'GPIO1', 'GPIO2', 'GPIO42', 'GPIO41', 'GPIO40', 'GPIO39', 'GPIO38', 'GPIO37', 'GPIO36', 'GPIO35', 'GPIO0|BOOT', 'GPIO45', 'GPIO48', 'GPIO47', 'GPIO21', 'GPIO20|USB D+', 'GPIO19|USB D-', 'GND', 'GND'] },
  { id: 'nodemcu', cat: 'Płytki MCU', title: 'NodeMCU ESP8266', sub: 'V3 · 30 pinów', short: 'NodeMCU',
    L: ['A0', 'RSV', 'RSV', 'SD3', 'SD2', 'SD1', 'CMD', 'SD0', 'CLK', 'GND', '3V3', 'EN', 'RST', 'GND', 'VIN'],
    R: ['D0|GPIO16', 'D1|GPIO5', 'D2|GPIO4', 'D3|GPIO0', 'D4|GPIO2', '3V3', 'GND', 'D5|GPIO14', 'D6|GPIO12', 'D7|GPIO13', 'D8|GPIO15', 'RX|GPIO3', 'TX|GPIO1', 'GND', '3V3'] },
  { id: 'd1mini', cat: 'Płytki MCU', title: 'Wemos D1 mini', sub: 'ESP8266 · 16 pinów', short: 'D1 mini',
    L: ['RST', 'A0', 'D0|GPIO16', 'D5|GPIO14 SCK', 'D6|GPIO12 MISO', 'D7|GPIO13 MOSI', 'D8|GPIO15 CS', '3V3'],
    R: ['TX|GPIO1', 'RX|GPIO3', 'D1|GPIO5 SCL', 'D2|GPIO4 SDA', 'D3|GPIO0', 'D4|GPIO2', 'GND', '5V'] },
  { id: 'nano', cat: 'Płytki MCU', title: 'Arduino Nano', sub: 'ATmega328P · 30 pinów · USB u dołu', short: 'Nano',
    L: ['TX1|D1', 'RX0|D0', 'RST', 'GND', 'D2', 'D3', 'D4', 'D5', 'D6', 'D7', 'D8', 'D9', 'D10|SS', 'D11|MOSI', 'D12|MISO'],
    R: ['VIN', 'GND', 'RST', '5V', 'A7', 'A6', 'A5|SCL', 'A4|SDA', 'A3', 'A2', 'A1', 'A0', 'REF', '3V3', 'D13|SCK'] },

  { id: 'tft14', cat: 'Wyświetlacze', title: 'TFT SPI + dotyk', sub: 'ILI9341 / ILI9488 / ST7796 · 14 pinów', short: 'TFT',
    L: ['VCC', 'GND', 'CS', 'RESET', 'DC|RS', 'SDI|MOSI', 'SCK', 'LED|BL', 'SDO|MISO', 'T_CLK', 'T_CS', 'T_DIN', 'T_DO', 'T_IRQ'], R: [] },
  { id: 'tft9', cat: 'Wyświetlacze', title: 'TFT SPI', sub: 'bez dotyku · 9 pinów', short: 'TFT',
    L: ['VCC', 'GND', 'CS', 'RESET', 'DC|RS', 'SDI|MOSI', 'SCK', 'LED|BL', 'SDO|MISO'], R: [] },
  { id: 'st7789', cat: 'Wyświetlacze', title: 'TFT ST7789', sub: '240×240 · 7 pinów', short: 'ST7789',
    L: ['GND', 'VCC', 'SCL|SCK', 'SDA|MOSI', 'RES', 'DC', 'BLK'], R: [] },
  { id: 'gc9a01', cat: 'Wyświetlacze', title: 'GC9A01 okrągły', sub: '240×240 · SPI', short: 'GC9A01',
    L: ['VCC', 'GND', 'SCL|SCK', 'SDA|MOSI', 'DC', 'CS', 'RST', 'BLK'], R: [] },
  { id: 'oled', cat: 'Wyświetlacze', title: 'OLED SSD1306', sub: 'I²C · 128×64', short: 'OLED', L: ['GND', 'VCC', 'SCL', 'SDA'], R: [] },
  { id: 'max7219', cat: 'Wyświetlacze', title: 'MAX7219', sub: 'matryca LED · wejście', short: 'MAX7219', L: ['VCC', 'GND', 'DIN', 'CS', 'CLK'], R: [] },
  { id: 'hub75', cat: 'Wyświetlacze', title: 'Panel RGB HUB75', sub: 'złącze IDC 2×8', short: 'HUB75',
    L: ['R1', 'B1', 'R2', 'B2', 'A', 'C', 'CLK', 'OE'], R: ['G1', 'GND', 'G2', 'E', 'B', 'D', 'LAT', 'GND'] },

  { id: 'mpu6050', cat: 'Czujniki', title: 'MPU6050 (GY-521)', sub: 'żyroskop + akcelerometr', short: 'MPU6050', L: ['VCC', 'GND', 'SCL', 'SDA', 'XDA', 'XCL', 'AD0', 'INT'], R: [] },
  { id: 'bmp280', cat: 'Czujniki', title: 'BMP280', sub: 'ciśnienie · I²C', short: 'BMP280', L: ['VCC', 'GND', 'SCL', 'SDA', 'CSB', 'SDO'], R: [] },
  { id: 'ds18b20', cat: 'Czujniki', title: 'DS18B20', sub: '1-Wire · pull-up 4,7 kΩ', short: 'DS18B20', L: ['VCC', 'DQ', 'GND'], R: [] },
  { id: 'dht', cat: 'Czujniki', title: 'DHT11 / DHT22', sub: 'moduł 3-pinowy', short: 'DHT', L: ['VCC', 'DATA', 'GND'], R: [] },
  { id: 'guva', cat: 'Czujniki', title: 'GUVA-S12SD', sub: 'czujnik UV · analog', short: 'UV', L: ['VCC', 'GND', 'SIG'], R: [] },
  { id: 'ina219', cat: 'Czujniki', title: 'INA219', sub: 'prąd / napięcie · I²C', short: 'INA219', L: ['VCC', 'GND', 'SCL', 'SDA', 'VIN+', 'VIN-'], R: [] },
  { id: 'pir', cat: 'Czujniki', title: 'HC-SR501 PIR', sub: 'czujnik ruchu', short: 'PIR', L: ['VCC', 'OUT', 'GND'], R: [] },

  { id: 'sd', cat: 'Moduły', title: 'Czytnik kart SD', sub: 'SPI · 6 pinów', short: 'SD', L: ['CS', 'SCK', 'MOSI', 'MISO', 'VCC', 'GND'], R: [] },
  { id: 'gps', cat: 'Moduły', title: 'GNSS / GPS', sub: 'UART · NEO-6M / ATGM336H', short: 'GPS', L: ['VCC', 'GND', 'TX', 'RX', 'PPS'], R: [] },
  { id: 'relay4', cat: 'Moduły', title: 'Przekaźniki 4×', sub: 'moduł 4-kanałowy', short: 'Przekaźn.', L: ['GND', 'IN1', 'IN2', 'IN3', 'IN4', 'VCC'], R: [] },
  { id: 'ws2812', cat: 'Moduły', title: 'WS2812B', sub: 'pierścień / taśma LED', short: 'WS2812B', L: ['5V', 'GND', 'DIN'], R: [] },
  { id: 'tp4056', cat: 'Moduły', title: 'TP4056', sub: 'ładowarka Li-Ion', short: 'TP4056', L: ['IN+', 'IN-'], R: ['B+', 'B-', 'OUT+', 'OUT-'] },
  { id: 'li18650', cat: 'Moduły', title: 'Ogniwo 18650', sub: 'Li-Ion 3,7 V', short: '18650', L: [], R: ['+', '-'] },
  { id: 'psu5', cat: 'Moduły', title: 'Zasilacz 5 V', sub: 'USB / gniazdo DC', short: 'PSU', L: [], R: ['+5V', 'GND'] },

  { id: 'led', cat: 'Podstawowe', title: 'Dioda LED', sub: '', short: 'LED', L: ['A|anoda', 'K|katoda'], R: [] },
  { id: 'res', cat: 'Podstawowe', title: 'Rezystor', sub: 'wpisz wartość w nazwie', short: 'R', L: ['1'], R: ['2'] },
  { id: 'cap', cat: 'Podstawowe', title: 'Kondensator', sub: '', short: 'C', L: ['+'], R: ['-'] },
  { id: 'btn', cat: 'Podstawowe', title: 'Przycisk', sub: '', short: 'BTN', L: ['1'], R: ['2'] },
  { id: 'buzz', cat: 'Podstawowe', title: 'Buzzer', sub: '', short: 'Buzzer', L: ['+', '-'], R: [] },
  { id: 'rail-gnd', cat: 'Podstawowe', title: 'Szyna GND', sub: 'wspólna masa', short: 'GND', L: ['GND', 'GND', 'GND', 'GND', 'GND', 'GND'], R: [] },
  { id: 'rail-3v3', cat: 'Podstawowe', title: 'Szyna 3V3', sub: 'wspólne zasilanie', short: '3V3', L: ['3V3', '3V3', '3V3', '3V3', '3V3', '3V3'], R: [] },
  { id: 'rail-5v', cat: 'Podstawowe', title: 'Szyna 5V', sub: 'wspólne zasilanie', short: '5V', L: ['5V', '5V', '5V', '5V', '5V', '5V'], R: [] }
];

/* ---------- pin semantics ---------- */
const GND_RE = /^(gnd|agnd|dgnd|gnd\d|vss|masse|-|(in|out|bat|batt|b|v)-)$/i;
const PWR_RE = /^(3v3|3\.3v|3v|5v|\+5v|vin|vcc|vdd|vbus|vout|jd-vcc|vcc\d?|\+|(in|out|bat|batt|b|v)\+)$/i;
const kindOf = n => (GND_RE.test(n) ? 'gnd' : PWR_RE.test(n) ? 'pwr' : 'io');
const railOf = n => (/^(3v3|3\.3v|3v)$/i.test(n) ? '3V3' : /^(5v|\+5v|vin|vbus)$/i.test(n) ? '5V' : null);
const pinCache = new Map();
function parsePin(s) {
  let r = pinCache.get(s);
  if (!r) {
    const i = s.indexOf('|');
    const n = (i < 0 ? s : s.slice(0, i)).trim();
    const h = i < 0 ? '' : s.slice(i + 1).trim();
    r = { n, h, k: kindOf(n), raw: s };
    pinCache.set(s, r);
  }
  return r;
}
const pk = a => a.p + ':' + a.s + a.i;
const pinLabel = pin => pin.n + (pin.h ? ' (' + pin.h + ')' : '');

/* =====================  geometry  ===================== */
const ROW = 22, HEAD = 38, PADB = 8, CW = 6.9, HW = 5.4, STUB = 18;
const labW = p => p.n.length * CW + (p.h ? p.h.length * HW + 7 : 0);
)PLAN";

const char PAGE_2[] PROGMEM = R"PLAN(
function geom(part) {
  const back = !!(P && P.settings.back);
  const rot = (((part.rot | 0) % 4) + 4) % 4;                       // quarter turns clockwise (model)
  const q = back && rot % 2 ? (rot + 2) % 4 : rot;                   // orientation as displayed (the back view is a mirror)
  const mk = (list, s) => list.map((raw, i) => ({ ...parsePin(raw), s, i }));
  let L = back ? mk(part.R, 'R') : mk(part.L, 'L'), R = back ? mk(part.L, 'L') : mk(part.R, 'R');
  if (q === 2) { const t = L; L = R.slice().reverse(); R = t.slice().reverse(); }   // half turn: columns swap, each read from the other end
  const lw = Math.max(0, ...L.map(labW)), rw = Math.max(0, ...R.map(labW));
  const tw = Math.max(part.title.length * 7.6, (part.sub || '').length * 5.7) + 26;
  const both = L.length && R.length;
  const w = Math.ceil(Math.max(tw, lw + rw + 26 + (both ? 30 : 0), 90) / 10) * 10;
  const rows = Math.max(L.length, R.length, 1);
  const h = HEAD + rows * ROW + PADB;
  const odd = q % 2 === 1, W = odd ? h : w, H = odd ? w : h;         // displayed size
  const x0 = back ? -(part.x + W) : part.x, y0 = part.y;
  const pins = [];
  const put = (arr, ds) => arr.forEach((p, i) => {
    const lx = ds === 'L' ? 0 : w, ly = HEAD + ROW * (i + 0.5), sg = ds === 'L' ? -1 : 1;
    let x, y, dx = 0, dy = 0;
    if (q === 1) { x = x0 + h - ly; y = y0 + lx; dy = sg; }
    else if (q === 3) { x = x0 + ly; y = y0 + w - lx; dy = -sg; }
    else { x = x0 + lx; y = y0 + ly; dx = sg; }
    pins.push({ ...p, p: part.id, ds, lx, ly, x, y, dx, dy, d: dx });
  });
  put(L, 'L'); put(R, 'R');
  return { id: part.id, part, x: x0, y: y0, w: W, h: H, lw: w, lh: h, q, L, R, pins };
}

function rpath(pts, r) {
  const q = [pts[0]];
  for (let i = 1; i < pts.length; i++) {
    const a = q[q.length - 1], b = pts[i];
    if (Math.abs(a.x - b.x) > 0.01 || Math.abs(a.y - b.y) > 0.01) q.push(b);
  }
  if (q.length < 2) return { d: '', q };
  let d = `M${q[0].x},${q[0].y}`;
  for (let i = 1; i < q.length - 1; i++) {
    const p0 = q[i - 1], p1 = q[i], p2 = q[i + 1];
    const l1 = Math.hypot(p1.x - p0.x, p1.y - p0.y), l2 = Math.hypot(p2.x - p1.x, p2.y - p1.y);
    const rr = Math.min(r, l1 / 2, l2 / 2);
    d += ` L${p1.x - (p1.x - p0.x) / l1 * rr},${p1.y - (p1.y - p0.y) / l1 * rr} Q${p1.x},${p1.y} ${p1.x + (p2.x - p1.x) / l2 * rr},${p1.y + (p2.y - p1.y) / l2 * rr}`;
  }
  d += ` L${q[q.length - 1].x},${q[q.length - 1].y}`;
  return { d, q };
}

function longestMid(q) {
  let best = 0, at = { x: q[0].x, y: q[0].y };
  for (let i = 1; i < q.length; i++) {
    const l = Math.hypot(q[i].x - q[i - 1].x, q[i].y - q[i - 1].y);
    if (l > best) { best = l; at = { x: (q[i].x + q[i - 1].x) / 2, y: (q[i].y + q[i - 1].y) / 2 }; }
  }
  return at;
}

function routeOne(it, style, G) {
  const { s, e } = it;
  if (style === 'curve') {
    const along = (p, o) => (p.dx ? Math.abs(o.x - p.x) : Math.abs(o.y - p.y));
    const o1 = Math.max(50, along(s, e) * 0.5), o2 = Math.max(50, along(e, s) * 0.5);
    const c1 = { x: s.x + s.dx * o1, y: s.y + s.dy * o1 }, c2 = { x: e.x + e.dx * o2, y: e.y + e.dy * o2 };
    const d = `M${s.x},${s.y} C${c1.x},${c1.y} ${c2.x},${c2.y} ${e.x},${e.y}`;
    const t = 0.85, u = 1 - t, k0 = u * u * u, k1 = 3 * u * u * t, k2 = 3 * u * t * t, k3 = t * t * t;
    const label = { x: k0 * s.x + k1 * c1.x + k2 * c2.x + k3 * e.x, y: k0 * s.y + k1 * c1.y + k2 * c2.y + k3 * e.y };
    return { d, pts: [s, e], label, handle: null };
  }
  if (style === 'straight') {
    const a = { x: s.x + s.dx * STUB, y: s.y + s.dy * STUB }, b = { x: e.x + e.dx * STUB, y: e.y + e.dy * STUB };
    const { d, q } = rpath([s, a, b, e], 0);
    return { d, pts: q, label: longestMid(q), handle: null };
  }
  const hs = s.dy === 0, he = e.dy === 0;
  if (hs && he) return elbowAxis(it, G, false);
  if (!hs && !he) return elbowAxis(it, G, true);
  return elbowMixed(it);
}

// Both pins face along the same axis. The maths is written for horizontal stubs; vertical pairs are run
// through the same code with x and y swapped.
function elbowAxis(it, G, vert) {
  const cn = p => (vert ? { x: p.y, y: p.x, d: p.dy, p: p.p } : { x: p.x, y: p.y, d: p.dx, p: p.p });
  const un = c => (vert ? { x: c.y, y: c.x } : c);
  const s = cn(it.s), e = cn(it.e), w = it.w, d1 = s.d, d2 = e.d;
  const a = { x: s.x + d1 * STUB, y: s.y }, b = { x: e.x + d2 * STUB, y: e.y };
  const facing = d1 > 0 && d2 < 0, sameSide = s.p === e.p && d1 === d2;
  let pts, label = null, handle = null;
  if (!facing && !sameSide) {
    // each detour wire gets its own lane next to the source pin, its own lane next to the destination
    // pin and its own height above the parts, so that no two wires share a line
    const topOf = id => { const g = G.get(id); return vert ? g.x : g.y; };
    const yTop = Math.min(topOf(s.p), topOf(e.p)) - 22 - num(it.t) * 8;
    const ax = s.x + d1 * (STUB + num(it.rs) * 8), bx = e.x + d2 * (STUB + 26 + num(it.rd) * 8);
    pts = [s, { x: ax, y: s.y }, { x: ax, y: yTop }, { x: bx, y: yTop }, { x: bx, y: e.y }, e];
    // numbers sit on the top run, spread sideways so that neighbouring badges do not overlap
    const lo = Math.min(ax, bx) + 16, hi = Math.max(ax, bx) - 16;
    label = { x: lo <= hi ? clamp((ax + bx) / 2 + (num(it.t) - (num(it.tn, 1) - 1) / 2) * 26, lo, hi) : (ax + bx) / 2, y: yTop };
  } else {
    let xm;
    if (d1 === d2) xm = d1 > 0 ? Math.max(a.x, b.x) + 14 + it.k * 9 : Math.min(a.x, b.x) - 14 - it.k * 9;
    else xm = (a.x + b.x) / 2 + (it.k - (it.n - 1) / 2) * 9;
    xm += (vert ? 1 : (P.settings.back ? -1 : 1)) * num(w.dx);
    pts = [s, a, { x: xm, y: a.y }, { x: xm, y: b.y }, b, e];
    if (Math.abs(xm - e.x) > STUB + 36) label = { x: e.x + d2 * (STUB + 16), y: e.y };
    handle = { x: xm, y: (a.y + b.y) / 2 };
  }
  const { d, q } = rpath(pts.map(un), 7);
  return { d, pts: q, label: label ? un(label) : longestMid(q), handle: handle ? { ...un(handle), axis: vert ? 'y' : 'x' } : null };
}

// One pin faces sideways, the other up or down: a single bend when the geometry allows it, otherwise a
// detour with a lane of its own beside the sideways pin and a level of its own outside the other one.
function mixedPlan(it) {
  const { s, e } = it;
  const H = s.dy === 0 ? s : e, V = s.dy === 0 ? e : s;
  const aH = { x: H.x + H.dx * STUB, y: H.y }, aV = { x: V.x, y: V.y + V.dy * STUB };
  const okH = (aV.x - aH.x) * H.dx >= 0, okV = (aH.y - V.y) * V.dy >= STUB / 2;
  return { H, V, aH, aV, ok: okH && okV };
}
function elbowMixed(it) {
  const { e } = it, { H, V, aH, aV, ok } = it.mp;
  let pts, label = null;
  if (ok) pts = [H, { x: aV.x, y: aH.y }, V];
  else {
    const xl = H.x + H.dx * (STUB + num(it.mlH) * 8), yl = V.y + V.dy * (STUB + num(it.mlV) * 8);
    pts = [H, { x: xl, y: H.y }, { x: xl, y: yl }, { x: V.x, y: yl }, V];
    const lo = Math.min(xl, V.x) + 10, hi = Math.max(xl, V.x) - 10;
    label = { x: lo <= hi ? clamp(V.x - num(it.mDir, 1) * 18, lo, hi) : (xl + V.x) / 2, y: yl };
  }
  const { d, q } = rpath(pts, 7);
  if (!label) {
    // the number sits next to the destination pin when the last stretch is long enough
    const n = q.length, i = e === V ? n - 1 : 0, j = e === V ? n - 2 : 1;
    const last = n > 1 ? Math.hypot(q[i].x - q[j].x, q[i].y - q[j].y) : 0;
    label = last >= STUB + 30 ? { x: e.x + e.dx * (STUB + 16), y: e.y + e.dy * (STUB + 16) } : longestMid(q);
  }
  return { d, pts: q, label, handle: null };
}

function layout() {
  const G = new Map(), PIN = new Map();
  for (const p of P.parts) {
    const g = geom(p); G.set(p.id, g);
    for (const pin of g.pins) PIN.set(pk(pin), pin);
  }
  // 'h': both pins face sideways, 'v': both face up/down, 'm': one of each
  const cx = (p, kind) => (kind === 'v' ? { x: p.y, y: p.x, d: p.dy } : { x: p.x, y: p.y, d: p.dx });
  const items = [];
  for (const w of P.wires) {
    const a = PIN.get(pk(w.a)), b = PIN.get(pk(w.b));
    if (!a || !b) continue;
    const kind = a.dy !== 0 && b.dy !== 0 ? 'v' : a.dy === 0 && b.dy === 0 ? 'h' : 'm';
    const ca = cx(a, kind), cb = cx(b, kind);
    let s = a, e = b;
    if (cb.x < ca.x || (cb.x === ca.x && cb.y < ca.y)) { s = b; e = a; }
    const it = { w, s, e, kind };
    it.cs = cx(s, kind); it.ce = cx(e, kind);
    items.push(it);
  }
  const axial = items.filter(it => it.kind !== 'm');
  const groups = new Map();
  for (const it of axial) {
    const key = it.s.p + it.s.s + it.kind;
    if (!groups.has(key)) groups.set(key, []);
    groups.get(key).push(it);
  }
  for (const arr of groups.values()) {
    arr.forEach(it => { it.score = it.ce.y > it.cs.y ? -it.cs.y : it.cs.y; });
    arr.sort((x, y) => x.score - y.score);
    arr.forEach((it, k) => { it.k = k; it.n = arr.length; });
  }
  // wires that detour around the parts: lanes beside the pins (lower pin = lane further out, so nothing crosses
  // a neighbour's stub) and heights above the parts (outer lane = higher up)
  const tops = axial.filter(it => !(it.cs.d > 0 && it.ce.d < 0) && !(it.s.p === it.e.p && it.cs.d === it.ce.d));
  const rank = (keyOf, yOf, field) => {
    const gm = new Map();
    for (const it of tops) { const k = keyOf(it); if (!gm.has(k)) gm.set(k, []); gm.get(k).push(it); }
    for (const arr of gm.values()) { arr.sort((x, y) => yOf(x) - yOf(y)); arr.forEach((it, i) => { it[field] = i; }); }
  };
  rank(it => it.s.p + it.s.s + it.kind, it => it.cs.y, 'rs');
  rank(it => it.e.p + it.e.s + it.kind, it => it.ce.y, 'rd');
  const tkey = it => it.s.p + it.s.s + it.kind;
  [...tops].sort((x, y) => (tkey(x) < tkey(y) ? -1 : tkey(x) > tkey(y) ? 1 : x.rs - y.rs))
    .forEach((it, i, all) => { it.t = i; it.tn = all.length; });
  // mixed wires that need a detour: a lane beside the sideways pin (the pin that has to travel further gets the
  // outer lane) and a level outside the up/down pin
  const mixed = items.filter(it => it.kind === 'm');
  mixed.forEach(it => { it.mp = mixedPlan(it); });
  const fb = mixed.filter(it => !it.mp.ok);
  const lane = (keyOf, sortBy, field) => {
    const gm = new Map();
    for (const it of fb) { const k = keyOf(it); if (!gm.has(k)) gm.set(k, []); gm.get(k).push(it); }
    for (const arr of gm.values()) { arr.sort(sortBy); arr.forEach((it, i) => { it[field] = i; }); }
  };
  fb.forEach(it => { it.mUp = it.mp.aV.y < it.mp.H.y; it.mDir = it.mp.V.x >= it.mp.aH.x ? 1 : -1; });
  lane(it => it.mp.H.p + it.mp.H.s + it.mUp, (a, b) => (a.mUp ? a.mp.H.y - b.mp.H.y : b.mp.H.y - a.mp.H.y), 'mlH');
  lane(it => it.mp.V.p + it.mp.V.s + it.mDir, (a, b) => (a.mDir > 0 ? a.mp.V.x - b.mp.V.x : b.mp.V.x - a.mp.V.x), 'mlV');
  const R = new Map();
  for (const it of items) R.set(it.w.id, routeOne(it, P.settings.wireStyle, G));
  return { G, PIN, R };
}

function bboxOf(Lay) {
  let x0 = 1e9, y0 = 1e9, x1 = -1e9, y1 = -1e9;
  for (const g of Lay.G.values()) { x0 = Math.min(x0, g.x); y0 = Math.min(y0, g.y); x1 = Math.max(x1, g.x + g.w); y1 = Math.max(y1, g.y + g.h); }
  for (const r of Lay.R.values()) for (const p of r.pts) { x0 = Math.min(x0, p.x); y0 = Math.min(y0, p.y); x1 = Math.max(x1, p.x); y1 = Math.max(y1, p.y); }
  if (x0 > x1) return { x: 0, y: 0, w: 400, h: 300 };
  return { x: x0 - 18, y: y0 - 18, w: x1 - x0 + 36, h: y1 - y0 + 36 };
}

/* =====================  drawing (used live, in print, in export)  ===================== */
function lum(hex) {
  const n = parseInt(hex.slice(1), 16), r = (n >> 16) & 255, g = (n >> 8) & 255, b = n & 255;
  return (0.299 * r + 0.587 * g + 0.114 * b) / 255;
}

function drawPart(g, conn, o) {
  const { part, lw: w, lh: h, q } = g;
  // the part is drawn upright in its own frame; a quarter turn is just a transform of that frame
  const T = q === 1 ? `translate(${g.x + g.w} ${g.y}) rotate(90)` : q === 3 ? `translate(${g.x} ${g.y + g.h}) rotate(-90)` : `translate(${g.x} ${g.y})`;
  const col = CAT[part.cat] || CAT['Własne'];
  const selected = o.sel && o.sel.t === 'part' && o.sel.id === part.id;
  let s = `<g${o.live ? ` data-k="part" data-id="${part.id}" style="cursor:grab"` : ''} transform="${T}">`;
  if (selected) s += `<rect x="-4" y="-4" width="${w + 8}" height="${h + 8}" rx="9" fill="none" stroke="${ACC}" stroke-width="3" opacity=".6"/>`;
  s += `<rect x="0" y="0" width="${w}" height="${h}" rx="6" fill="#fff" stroke="${INK}" stroke-width="1.6"/>`;
  s += `<path d="M0,${HEAD} L0,6 Q0,0 6,0 L${w - 6},0 Q${w},0 ${w},6 L${w},${HEAD} Z" fill="${col}" stroke="${INK}" stroke-width="1.6"/>`;
  s += `<text x="11" y="17" font-family="${HEADF}" font-weight="700" font-size="15" letter-spacing=".4" fill="${INK}">${esc(part.title)}</text>`;
  if (part.sub) s += `<text x="11" y="31" font-family="${SANSF}" font-size="10" fill="${INK2}">${esc(part.sub)}</text>`;
  for (const pin of g.pins) {
    const key = pk(pin), ry = pin.ly - ROW / 2;
    if (o.hiPins && o.hiPins.has(key)) s += `<rect x="1" y="${ry}" width="${w - 2}" height="${ROW}" fill="#bfeaf0"/>`;
    else if (pin.k === 'gnd') s += `<rect x="1" y="${ry}" width="${w - 2}" height="${ROW}" fill="#eef1f3"/>`;
  }
  for (const pin of g.pins) {
    const nameCol = pin.k === 'pwr' ? '#a3261c' : INK;
    const nm = `<tspan font-weight="600" fill="${nameCol}">${esc(pin.n)}</tspan>`;
    if (pin.ds === 'L') {
      const hn = pin.h ? `<tspan font-size="9" fill="${INK2}" dx="5">${esc(pin.h)}</tspan>` : '';
      s += `<text x="12" y="${pin.ly + 4}" font-family="${MONO}" font-size="11.5">${nm}${hn}</text>`;
    } else {
      const nmR = `<tspan font-weight="600" fill="${nameCol}"${pin.h ? ' dx="5"' : ''}>${esc(pin.n)}</tspan>`;
      const hnR = pin.h ? `<tspan font-size="9" fill="${INK2}">${esc(pin.h)}</tspan>` : '';
      s += `<text x="${w - 12}" y="${pin.ly + 4}" text-anchor="end" font-family="${MONO}" font-size="11.5">${hnR}${nmR}</text>`;
    }
  }
  for (const pin of g.pins) {
    const key = pk(pin), used = conn.has(key);
    if (o.pending === key || o.hover === key) s += `<circle cx="${pin.lx}" cy="${pin.ly}" r="9.5" fill="none" stroke="${ACC}" stroke-width="2.4"/>`;
    s += `<circle cx="${pin.lx}" cy="${pin.ly}" r="4.6" fill="${used ? COPPER : '#fff'}" stroke="${INK}" stroke-width="1.4"/>`;
    if (used) s += `<circle cx="${pin.lx}" cy="${pin.ly}" r="1.5" fill="${INK}"/>`;
  }
  return s + '</g>';
}

function drawWire(w, r, o) {
  const hex = (COLORS[w.color] || COLORS.gray)[0];
  const sel = o.sel && o.sel.t === 'wire' && o.sel.id === w.id;
  const dim = o.dim && o.dim.has(w.id);
  let s = `<g opacity="${dim ? 0.2 : 1}"${o.live ? ` data-k="wire" data-id="${w.id}" style="cursor:pointer"` : ''}>`;
  if (o.live) s += `<path d="${r.d}" fill="none" stroke="transparent" stroke-width="13" stroke-linecap="round"/>`;
  if (sel) s += `<path d="${r.d}" fill="none" stroke="${ACC}" stroke-opacity=".38" stroke-width="11" stroke-linecap="round" stroke-linejoin="round"/>`;
  s += `<path d="${r.d}" fill="none" stroke="${INK}" stroke-opacity=".62" stroke-width="5.2" stroke-linecap="round" stroke-linejoin="round"/>`;
  s += `<path d="${r.d}" fill="none" stroke="${hex}" stroke-width="3" stroke-linecap="round" stroke-linejoin="round"/>`;
  return s + '</g>';
}

function drawBadge(w, r, idx, o) {
  const hex = (COLORS[w.color] || COLORS.gray)[0];
  const dim = o.dim && o.dim.has(w.id);
  const { x, y } = r.label;
  const label = String(idx + 1);
  const rad = label.length > 2 ? 10.5 : 8.6;
  let s = `<g opacity="${dim ? 0.2 : 1}"${o.live ? ` data-k="wire" data-id="${w.id}" style="cursor:pointer"` : ''}>`;
  s += `<circle cx="${x}" cy="${y}" r="${rad}" fill="${hex}" stroke="${INK}" stroke-width="1.3"/>`;
  s += `<text x="${x}" y="${y + 3.4}" text-anchor="middle" font-family="${MONO}" font-weight="600" font-size="9.5" fill="${lum(hex) > 0.6 ? INK : '#fff'}">${label}</text>`;
  if (w.done) s += `<circle cx="${x + 8}" cy="${y - 8}" r="5.6" fill="#1f7a3a" stroke="#fff" stroke-width="1.2"/><path d="M${x + 5.4},${y - 8} l2 2 l3.2 -3.8" fill="none" stroke="#fff" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round"/>`;
  return s + '</g>';
}

function drawScene(Lay, o) {
  const conn = new Set();
  P.wires.forEach(w => { conn.add(pk(w.a)); conn.add(pk(w.b)); });
  let out = '';
  for (const g of Lay.G.values()) out += drawPart(g, conn, o);
  P.wires.forEach(w => { const r = Lay.R.get(w.id); if (r) out += drawWire(w, r, o); });
  if (o.nums !== false) P.wires.forEach((w, i) => { const r = Lay.R.get(w.id); if (r) out += drawBadge(w, r, i, o); });
  return out;
}

/* =====================  print / export sheet  ===================== */
const shortOf = part => (part.short || part.title.split(/\s+/)[0] || '?').slice(0, 12);
function wireEnds(Lay, w) {
  const a = Lay.PIN.get(pk(w.a)), b = Lay.PIN.get(pk(w.b));
  if (!a || !b) return null;
  const pa = Lay.G.get(a.p).part, pb = Lay.G.get(b.p).part;
  return { a, b, pa, pb, ta: shortOf(pa) + ' ' + a.n, tb: shortOf(pb) + ' ' + b.n };
}

function sheetSVG(orient) {
  const Lay = layout();
  const W = orient === 'portrait' ? 794 : 1123, H = orient === 'portrait' ? 1123 : 794, M = 26;
  const wires = P.wires.map((w, i) => ({ w, i, e: wireEnds(Lay, w) })).filter(x => x.e);
  const cols = orient === 'portrait' ? 2 : (wires.length > 36 ? 4 : 3);
  const rowH = 16, rows = Math.ceil(wires.length / cols);
  const listH = wires.length ? 34 + rows * rowH + 6 : 0;
  const headH = 46;
  const bb = bboxOf(Lay);
  const availW = W - 2 * M, availH = Math.max(120, H - headH - listH - M);
  const sc = Math.min(availW / bb.w, availH / bb.h, 1.5);
  const ox = M + (availW - bb.w * sc) / 2 - bb.x * sc;
  const oy = headH + (availH - bb.h * sc) / 2 - bb.y * sc;
  const date = new Date().toLocaleDateString('pl-PL');
  let s = `<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 ${W} ${H}" width="${W}" height="${H}" font-family="${SANSF}">`;
  s += `<rect width="${W}" height="${H}" fill="#fff"/>`;
  s += `<text x="${M}" y="32" font-family="${HEADF}" font-weight="700" font-size="24" letter-spacing=".5" fill="${INK}">${esc(P.name)}</text>`;
  s += `<text x="${W - M}" y="30" text-anchor="end" font-family="${MONO}" font-size="10.5" fill="${INK2}">${date} · ${P.parts.length} el. · ${wires.length} przew. · skala ${Math.round(sc * 100)}%</text>`;
  if (P.settings.back) s += `<text x="${W - M}" y="16" text-anchor="end" font-family="${HEADF}" font-weight="700" font-size="13" letter-spacing="1.5" fill="#a8480a">WIDOK OD SPODU (ODBICIE LUSTRZANE)</text>`;
  s += `<line x1="${M}" y1="${headH - 6}" x2="${W - M}" y2="${headH - 6}" stroke="${INK}" stroke-width="1.2"/>`;
  s += `<g transform="translate(${ox} ${oy}) scale(${sc})">${drawScene(Lay, { nums: true })}</g>`;
  if (wires.length) {
    const top = H - listH - 4 + 6;
    s += `<line x1="${M}" y1="${top - 12}" x2="${W - M}" y2="${top - 12}" stroke="${INK}" stroke-width="1.2"/>`;
    s += `<text x="${M}" y="${top + 6}" font-family="${HEADF}" font-weight="700" font-size="15" letter-spacing="1" fill="${INK}">LISTA POŁĄCZEŃ</text>`;
    const colW = (W - 2 * M) / cols;
    const maxCh = Math.floor((colW - 62) / 6.4);
    wires.forEach((it, k) => {
      const col = Math.floor(k / rows), row = k % rows;
      const x = M + col * colW, y = top + 26 + row * rowH;
      const hex = (COLORS[it.w.color] || COLORS.gray)[0];
      s += `<rect x="${x}" y="${y - 8.5}" width="9" height="9" fill="none" stroke="${INK}" stroke-width="1"/>`;
      if (it.w.done) s += `<path d="M${x + 1.5},${y - 4} l2.4 2.6 l4.4 -6" fill="none" stroke="#1f7a3a" stroke-width="1.8"/>`;
      s += `<text x="${x + 27}" y="${y}" text-anchor="end" font-family="${MONO}" font-weight="600" font-size="10">${it.i + 1}</text>`;
      s += `<rect x="${x + 32}" y="${y - 8}" width="12" height="8" rx="2" fill="${hex}" stroke="${INK}" stroke-width="1"/>`;
      let t = `${it.e.ta} → ${it.e.tb}`;
      if (t.length > maxCh) t = t.slice(0, maxCh - 1) + '…';
      s += `<text x="${x + 50}" y="${y}" font-family="${MONO}" font-size="10" fill="${INK}">${esc(t)}</text>`;
    });
  }
  return { svg: s + '</svg>', scale: sc, W, H };
}

function csvText() {
  const Lay = layout();
  const rows = [['Nr', 'Element A', 'Pin A', 'Element B', 'Pin B', 'Kolor', 'Uwagi', 'Zlutowane']];
  P.wires.forEach((w, i) => {
    const e = wireEnds(Lay, w); if (!e) return;
    rows.push([i + 1, e.pa.title, pinLabel(e.a), e.pb.title, pinLabel(e.b), (COLORS[w.color] || COLORS.gray)[1], w.note || '', w.done ? 'tak' : '']);
  });
  return '﻿' + rows.map(r => r.map(c => '"' + String(c).replace(/"/g, '""') + '"').join(';')).join('\r\n');
}
/* =====================  state  ===================== */
let P = null;                 // current project
let sel = null;               // {t:'part'|'wire', id}
let hoverPin = null;          // pin key
let pending = null;           // pin ref waiting for its second end
let drag = null;
let view = { x: 60, y: 50, k: 1 };
let LAY = null;
let tab = 'props';
let dirty = false;

const hist = { stack: [], i: -1 };
const cv = $('#cv'), stage = $('#stage');

/* =====================  projects  ===================== */
function blankProject(name) {
  return { v: 1, id: uid(), name: name || 'Nowy schemat', created: Date.now(), updated: Date.now(),
    settings: { wireStyle: 'elbow', numbers: true, orient: 'landscape', back: false }, parts: [], wires: [] };
}

function makePart(t, x, y) {
  return { id: uid(), tpl: t.id || null, cat: t.cat || 'Własne', title: t.title, sub: t.sub || '', short: t.short || '',
    L: [...t.L], R: [...t.R], x, y };
}

function findPin(part, name, side) {
  for (const s of (side ? [side] : ['L', 'R'])) {
    const i = part[s].findIndex(r => parsePin(r).n === name);
    if (i >= 0) return { p: part.id, s, i };
  }
  return null;
}

function sampleProject() {
  const p = blankProject('Przykład: ESP32 + TFT ILI9341');
  p.isSample = true;
  const esp = makePart(TPL.find(t => t.id === 'esp32-30'), 40, 30);
  const tft = makePart(TPL.find(t => t.id === 'tft14'), 500, 30);
  p.parts.push(esp, tft);
  const set = [['VCC', '3V3', 'red'], ['GND', 'GND', 'black'], ['CS', 'GPIO15', 'yellow'], ['RESET', 'GPIO4', 'gray'],
    ['DC', 'GPIO2', 'brown'], ['SDI', 'GPIO23', 'blue'], ['SCK', 'GPIO18', 'green'], ['LED', '3V3', 'red'],
    ['SDO', 'GPIO19', 'violet'], ['T_CLK', 'GPIO18', 'green'], ['T_CS', 'GPIO21', 'pink'], ['T_DIN', 'GPIO23', 'blue'], ['T_DO', 'GPIO19', 'violet']];
  for (const [tp, ep, c] of set) {
    const a = findPin(tft, tp, 'L'), b = findPin(esp, ep, 'R');
    if (a && b) p.wires.push({ id: uid(), a, b, color: c, dx: 0, done: false, note: '' });
  }
  return p;
}

function sanitize(raw) {
  const p = blankProject(typeof raw.name === 'string' ? raw.name.slice(0, 60) : 'Schemat');
  if (typeof raw.id === 'string') p.id = raw.id;
  p.created = num(raw.created, Date.now()); p.updated = num(raw.updated, Date.now());
  const st = raw.settings || {};
  p.settings = { wireStyle: ['elbow', 'curve', 'straight'].includes(st.wireStyle) ? st.wireStyle : 'elbow',
    numbers: st.numbers !== false, orient: st.orient === 'portrait' ? 'portrait' : 'landscape', back: !!st.back };
  const ids = new Set();
  for (const r of Array.isArray(raw.parts) ? raw.parts : []) {
    if (!r || !Array.isArray(r.L) || !Array.isArray(r.R)) continue;
    const id = typeof r.id === 'string' && !ids.has(r.id) ? r.id : uid();
    ids.add(id);
    p.parts.push({ id, tpl: r.tpl || null, cat: CAT[r.cat] ? r.cat : 'Własne', title: String(r.title || 'Element').slice(0, 40),
      sub: String(r.sub || '').slice(0, 60), short: String(r.short || '').slice(0, 12),
      L: r.L.map(x => String(x).slice(0, 40)), R: r.R.map(x => String(x).slice(0, 40)), x: num(r.x), y: num(r.y),
      rot: (((Math.round(num(r.rot)) % 4) + 4) % 4) });
  }
  const okPin = a => a && p.parts.some(q => q.id === a.p) && (a.s === 'L' || a.s === 'R') &&
    Number.isInteger(a.i) && a.i >= 0 && a.i < p.parts.find(q => q.id === a.p)[a.s].length;
  for (const w of Array.isArray(raw.wires) ? raw.wires : []) {
    if (!w || !okPin(w.a) || !okPin(w.b)) continue;
    p.wires.push({ id: typeof w.id === 'string' ? w.id : uid(), a: { p: w.a.p, s: w.a.s, i: w.a.i }, b: { p: w.b.p, s: w.b.s, i: w.b.i },
      color: COLORS[w.color] ? w.color : 'gray', dx: num(w.dx), done: !!w.done, note: String(w.note || '').slice(0, 80) });
  }
  return p;
}

/* ---------- history ---------- */
const snapState = () => JSON.stringify({ parts: P.parts, wires: P.wires });
function resetHistory() { hist.stack = [snapState()]; hist.i = 0; }
function commit(opts = {}) {
  const s = snapState();
  if (hist.stack[hist.i] === s) return;
  hist.stack = hist.stack.slice(0, hist.i + 1);
  hist.stack.push(s);
  if (hist.stack.length > 80) hist.stack.shift();
  hist.i = hist.stack.length - 1;
  touched();
  render();
  if (!opts.keepSide) refreshSide();
}
function restore(str) {
  const o = JSON.parse(str);
  P.parts = o.parts; P.wires = o.wires;
  if (sel && !(sel.t === 'part' ? P.parts.some(x => x.id === sel.id) : P.wires.some(x => x.id === sel.id))) sel = null;
  pending = null; touched(); render(); refreshSide();
}
function undo() { if (hist.i > 0) { hist.i--; restore(hist.stack[hist.i]); } }
function redo() { if (hist.i < hist.stack.length - 1) { hist.i++; restore(hist.stack[hist.i]); } }

/* ---------- persistence (ESP32 + SD) ---------- */
const api = {
  async get(n) {
    const r = await fetch('/api/file?n=' + encodeURIComponent(n), { cache: 'no-store' });
    if (r.status === 404) return null;
    if (!r.ok) throw new Error('http ' + r.status);
    return r.text();
  },
  async put(n, t) {
    const r = await fetch('/api/file?n=' + encodeURIComponent(n), { method: 'POST', headers: { 'Content-Type': 'text/plain;charset=utf-8' }, body: t });
    if (!r.ok) throw new Error('http ' + r.status);
  },
  async del(n) {
    const r = await fetch('/api/del?n=' + encodeURIComponent(n), { method: 'POST' });
    if (!r.ok) throw new Error('http ' + r.status);
  }
};
let INDEX = { last: null, list: [] };
let saveTimer = null, saving = false, saveAgain = false;
function setStatus(t) { $('#stSave').textContent = t; }
function touched() {
  P.updated = Date.now(); dirty = true; delete P.isSample;
  setStatus('Zmiany…');
  clearTimeout(saveTimer);
  saveTimer = setTimeout(saveNow, 800);
  updateUndoButtons();
}
async function saveNow() {
  if (saving) { saveAgain = true; return; }
  saving = true;
  const data = JSON.parse(JSON.stringify(P));
  try {
    setStatus('Zapisuję na karcie SD…');
    await api.put('p_' + data.id, JSON.stringify(data));
    INDEX.list = INDEX.list.filter(x => x.id !== data.id);
    INDEX.list.push({ id: data.id, name: data.name, updated: data.updated });
    INDEX.last = data.id;
    await api.put('index', JSON.stringify(INDEX));
    dirty = false; setStatus('Zapisano na karcie SD');
  } catch (e) {
    setStatus('BŁĄD zapisu: sprawdź kartę SD i WiFi (ponowię za 5 s)');
    clearTimeout(saveTimer); saveTimer = setTimeout(saveNow, 5000);
  }
  saving = false;
  if (saveAgain) { saveAgain = false; saveNow(); }
}
async function loadProject(id) {
  try { const r = await api.get('p_' + id); return r ? sanitize(JSON.parse(r)) : null; } catch (e) { return null; }
}
function openProject(p, opts = {}) {
  P = p; sel = null; pending = null; hoverPin = null;
  resetHistory(); dirty = false;
  syncTop(); render(); refreshSide(); updateUndoButtons();
  if (!opts.noFit) requestAnimationFrame(fitView);
}
window.addEventListener('beforeunload', e => { if (dirty) { e.preventDefault(); e.returnValue = ''; } });

/* =====================  view  ===================== */
function updateBg() {
  const k = view.k, a = 100 * k, b = 20 * k;
  stage.style.backgroundSize = `${a}px ${a}px,${a}px ${a}px,${b}px ${b}px,${b}px ${b}px`;
  stage.style.backgroundPosition = `${view.x}px ${view.y}px`;
  $('#zLbl').textContent = Math.round(k * 100) + '%';
}
function toWorld(e) {
  const r = cv.getBoundingClientRect();
  return { x: (e.clientX - r.left - view.x) / view.k, y: (e.clientY - r.top - view.y) / view.k };
}
function zoomAt(mx, my, f) {
  const k = clamp(view.k * f, 0.25, 3);
  const wx = (mx - view.x) / view.k, wy = (my - view.y) / view.k;
  view = { k, x: mx - wx * k, y: my - wy * k };
  render();
}
function fitView() {
  const r = cv.getBoundingClientRect();
  if (!r.width) return;
  if (!P.parts.length) { view = { x: r.width / 2, y: r.height / 2, k: 1 }; render(); return; }
  const bb = bboxOf(layout());
  const k = clamp(Math.min((r.width - 60) / bb.w, (r.height - 60) / bb.h), 0.25, 1.4);
  view = { k, x: (r.width - bb.w * k) / 2 - bb.x * k, y: (r.height - bb.h * k) / 2 - bb.y * k };
  render();
}

/* =====================  render  ===================== */
function focusSets() {
  const hi = new Set(), hiPins = new Set();
  let active = false;
  if (sel && sel.t === 'wire') {
    active = true; hi.add(sel.id);
    const w = P.wires.find(x => x.id === sel.id);
    if (w) { hiPins.add(pk(w.a)); hiPins.add(pk(w.b)); }
  } else if (sel && sel.t === 'part') {
    P.wires.forEach(w => { if (w.a.p === sel.id || w.b.p === sel.id) hi.add(w.id); });
    active = hi.size > 0;
  }
  const fp = pending ? pk(pending) : hoverPin;
  if (fp) {
    const ws = P.wires.filter(w => pk(w.a) === fp || pk(w.b) === fp);
    if (ws.length || pending) {
      active = true;
      ws.forEach(w => { hi.add(w.id); hiPins.add(pk(w.a)); hiPins.add(pk(w.b)); });
    }
  }
  const dim = new Set();
  if (active) P.wires.forEach(w => { if (!hi.has(w.id)) dim.add(w.id); });
  return { dim, hiPins };
}

function render() {
  if (!P) return;
  LAY = layout();
  const { dim, hiPins } = focusSets();
  let inner = drawScene(LAY, { live: true, sel, dim, hiPins, pending: pending ? pk(pending) : null, hover: hoverPin, nums: P.settings.numbers });
  if (sel && sel.t === 'wire') {
    const r = LAY.R.get(sel.id);
    if (r && r.handle) inner += `<rect data-k="handle" data-id="${sel.id}" x="${r.handle.x - 6}" y="${r.handle.y - 6}" width="12" height="12" rx="3" fill="#fff" stroke="${ACC}" stroke-width="2.2" style="cursor:ew-resize"/>`;
  }
  if (drag && drag.mode === 'wire' && drag.moved) {
    const a = LAY.PIN.get(pk(drag.from));
    const t = drag.snapPin ? LAY.PIN.get(drag.snapPin) : null;
    const end = t ? { x: t.x, y: t.y } : drag.to;
    if (a) inner += `<path d="M${a.x},${a.y} L${a.x + a.dx * STUB},${a.y + a.dy * STUB} L${end.x},${end.y}" fill="none" stroke="${ACC}" stroke-width="2.6" stroke-dasharray="6 4" stroke-linecap="round" pointer-events="none"/><circle cx="${end.x}" cy="${end.y}" r="${t ? 10 : 4}" fill="none" stroke="${ACC}" stroke-width="2.4" pointer-events="none"/>`;
  }
  cv.innerHTML = `<g transform="translate(${view.x} ${view.y}) scale(${view.k})">${inner}</g>`;
  updateBg();
  $('#empty').hidden = P.parts.length > 0;
  stage.classList.toggle('back', !!P.settings.back); $('#backBadge').hidden = !P.settings.back;
  placePartBar();
  $('#cnt').textContent = P.wires.length;
}

/* =====================  hit testing  ===================== */
function pinAt(w, tol) {
  if (!LAY) return null;
  let best = null, bd = 1e9;
  for (const pin of LAY.PIN.values()) {
    // look at the pointer in the part's own (unrotated) frame
    const g = LAY.G.get(pin.p);
)PLAN";

const char PAGE_3[] PROGMEM = R"PLAN(    const lx = g.q === 1 ? w.y - g.y : g.q === 3 ? g.y + g.h - w.y : w.x - g.x;
    const ly = g.q === 1 ? g.x + g.w - w.x : g.q === 3 ? w.x - g.x : w.y - g.y;
    const dx = Math.abs(lx - pin.lx), dy = Math.abs(ly - pin.ly);
    // dot zone plus a slice of the row just inside the edge
    const inside = pin.ds === 'L' ? lx - pin.lx : pin.lx - lx;
    if (dy > ROW / 2 - 1 || inside < -tol || inside > 14) continue;
    const d = dx + dy * 1.5;
    if (d < bd) { bd = d; best = pin; }
  }
  return best;
}
function nearestPin(w, tol, exclude) {
  let best = null, bd = tol * tol;
  for (const pin of LAY.PIN.values()) {
    if (exclude && pk(pin) === exclude) continue;
    const d = (pin.x - w.x) ** 2 + (pin.y - w.y) ** 2;
    if (d < bd) { bd = d; best = pin; }
  }
  return best;
}
const refOf = pin => ({ p: pin.p, s: pin.s, i: pin.i });

/* =====================  edit operations  ===================== */
function autoColor(a, b) {
  const pa = parsePin(P.parts.find(x => x.id === a.p)[a.s][a.i]), pb = parsePin(P.parts.find(x => x.id === b.p)[b.s][b.i]);
  if (pa.k === 'gnd' || pb.k === 'gnd') return 'black';
  if (pa.k === 'pwr' || pb.k === 'pwr') return 'red';
  for (const w of P.wires) if ([pk(w.a), pk(w.b)].some(k => k === pk(a) || k === pk(b)) && w.color !== 'black' && w.color !== 'red') return w.color;
  const use = {}; SIGNAL_ORDER.forEach(c => { use[c] = 0; }); P.wires.forEach(w => { if (w.color in use) use[w.color]++; });
  return SIGNAL_ORDER.reduce((best, c) => (use[c] < use[best] ? c : best), SIGNAL_ORDER[0]);
}

function connect(a, b) {
  if (pk(a) === pk(b)) return;
  if (P.wires.some(w => (pk(w.a) === pk(a) && pk(w.b) === pk(b)) || (pk(w.a) === pk(b) && pk(w.b) === pk(a)))) { toast('Te dwa piny są już połączone.'); return; }
  const w = { id: uid(), a: { ...a }, b: { ...b }, color: autoColor(a, b), dx: 0, done: false, note: '' };
  P.wires.push(w);
  sel = { t: 'wire', id: w.id };
  commit();
}

function addPart(tid) {
  const t = TPL.find(x => x.id === tid); if (!t) return;
  placePart(makePart(t, 0, 0));
}
function placePart(part) {
  const g = geom(part);
  const r = cv.getBoundingClientRect();
  if (!P.parts.length) {
    const cx = (r.width / 2 - view.x) / view.k, cy = (r.height / 2 - view.y) / view.k;
    part.x = snap10(cx - g.w / 2); part.y = snap10(cy - g.h / 2);
  } else {
    let right = -1e9, top = 1e9;
    P.parts.forEach(q => { const h = geom(q); right = Math.max(right, q.x + h.w); top = Math.min(top, q.y); });
    part.x = snap10(right + 170); part.y = snap10(top);
  }
  P.parts.push(part);
  sel = { t: 'part', id: part.id };
  commit();
  const gg = LAY.G.get(part.id), sx = view.x + (gg.x + gg.w) * view.k;
  if (sx > r.width - 20 || sx < 0) fitView();
  closeDrawers();
}

function deleteSel() {
  if (!sel) return;
  if (sel.t === 'wire') P.wires = P.wires.filter(w => w.id !== sel.id);
  else { P.parts = P.parts.filter(p => p.id !== sel.id); P.wires = P.wires.filter(w => w.a.p !== sel.id && w.b.p !== sel.id); }
  sel = null; commit();
}
function duplicatePart() {
  const p = P.parts.find(x => x.id === sel.id); if (!p) return;
  const c = JSON.parse(JSON.stringify(p)); c.id = uid(); c.x += 30; c.y += 30;
  P.parts.push(c); sel = { t: 'part', id: c.id }; commit();
}
function mirrorPart() {
  const p = P.parts.find(x => x.id === sel.id); if (!p) return;
  [p.L, p.R] = [p.R, p.L];
  P.wires.forEach(w => { for (const e of [w.a, w.b]) if (e.p === p.id) e.s = e.s === 'L' ? 'R' : 'L'; });
  commit();
}

function rotatePart(dir = 1) {
  const p = P.parts.find(x => x.id === (sel && sel.id)); if (!p) return;
  // quarter turn clockwise (dir 1) or anticlockwise (dir -1) as it looks on screen; the model turn is
  // reversed in the back view, which is a mirror image
  const back = !!P.settings.back, f = r => (r % 2 ? (r + 2) % 4 : r);
  const g0 = geom(p), cx = g0.x + g0.w / 2, cy = g0.y + g0.h / 2;
  const shown = back ? f(p.rot | 0) : (p.rot | 0);
  const next = (((shown + dir) % 4) + 4) % 4;
  p.rot = back ? f(next) : next;
  const g1 = geom(p);                               // keep the centre where it was
  const nx = snap10(cx - g1.w / 2);
  p.x = back ? -(nx + g1.w) : nx; p.y = snap10(cy - g1.h / 2);
  commit();
}
function placePartBar() {
  const bar = $('#partBar');
  const g = sel && sel.t === 'part' && LAY ? LAY.G.get(sel.id) : null;
  if (!g) { bar.hidden = true; return; }
  bar.hidden = false;
  bar.style.left = clamp(view.x + (g.x + g.w) * view.k, bar.offsetWidth + 8, Math.max(bar.offsetWidth + 8, stage.clientWidth - 8)) + 'px';
  bar.style.top = Math.max(8, view.y + g.y * view.k - 40) + 'px';
}
$('#pbRot').addEventListener('click', e => { e.stopPropagation(); if (sel && sel.t === 'part') rotatePart(); });

/* ---------- part editor dialog ---------- */
let editingId = null;
function openPartDialog(id) {
  editingId = id;
  const p = id ? P.parts.find(x => x.id === id) : { title: 'Własny element', sub: '', short: 'Elem.', L: ['VCC', 'GND', 'SIG'], R: [] };
  $('#dpHead').textContent = id ? 'Edytuj element' : 'Nowy element';
  $('#dpName').value = p.title; $('#dpSub').value = p.sub || ''; $('#dpShort').value = p.short || '';
  $('#dpL').value = p.L.join('\n'); $('#dpR').value = p.R.join('\n');
  $('#dlgPart').showModal();
}
function pinLines(v) { return v.split('\n').map(s => s.trim()).filter(Boolean).slice(0, 60); }
$('#fPart').addEventListener('submit', e => {
  e.preventDefault();
  const title = $('#dpName').value.trim() || 'Element';
  const L = pinLines($('#dpL').value), R = pinLines($('#dpR').value);
  if (!L.length && !R.length) { toast('Dodaj przynajmniej jeden pin.'); return; }
  const short = $('#dpShort').value.trim() || title.split(/\s+/)[0];
  if (editingId) {
    const p = P.parts.find(x => x.id === editingId);
    Object.assign(p, { title, sub: $('#dpSub').value.trim(), short, L, R });
    P.wires = P.wires.filter(w => [w.a, w.b].every(e => e.p !== p.id || e.i < p[e.s].length));
    commit();
  } else {
    placePart({ id: uid(), tpl: null, cat: 'Własne', title, sub: $('#dpSub').value.trim(), short, L, R, x: 0, y: 0 });
  }
  $('#dlgPart').close();
});
$('#dpCancel').addEventListener('click', () => $('#dlgPart').close());

/* =====================  pointer interaction  ===================== */
cv.addEventListener('pointerdown', e => {
  if (e.button !== 0 && e.pointerType === 'mouse') return;
  closeMenu();
  const w = toWorld(e);
  const t = e.target.closest('[data-k]');
  const kind = t ? t.dataset.k : null;
  cv.setPointerCapture(e.pointerId);
  drag = { sx: e.clientX, sy: e.clientY, moved: false, w0: w };
  const pin = kind === 'handle' ? null : pinAt(w, 10 / view.k);
  if (pin) { drag.mode = 'wire'; drag.from = refOf(pin); drag.to = w; drag.snapPin = null; }
  else if (kind === 'handle') { drag.mode = 'handle'; drag.id = t.dataset.id; drag.dx0 = num(P.wires.find(x => x.id === drag.id).dx); }
  else if (kind === 'wire') { drag.mode = 'click'; sel = { t: 'wire', id: t.dataset.id }; pending = null; tab = 'props'; render(); refreshSide(); }
  else if (kind === 'part') {
    const part = P.parts.find(x => x.id === t.dataset.id);
    const g0 = LAY.G.get(part.id);
    drag.mode = 'part'; drag.id = part.id; drag.ox = w.x - g0.x; drag.oy = w.y - part.y; drag.gw = g0.w;
    sel = { t: 'part', id: part.id }; pending = null; tab = 'props'; render(); refreshSide();
  } else { drag.mode = 'pan'; drag.vx = view.x; drag.vy = view.y; }
});

cv.addEventListener('pointermove', e => {
  const w = toWorld(e);
  if (!drag) {
    const pin = pinAt(w, 8 / view.k), key = pin ? pk(pin) : null;
    if (key !== hoverPin) { hoverPin = key; render(); showPinInfo(pin); }
    return;
  }
  if (!drag.moved && Math.hypot(e.clientX - drag.sx, e.clientY - drag.sy) > 3) drag.moved = true;
  if (!drag.moved) return;
  if (drag.mode === 'pan') { view.x = drag.vx + e.clientX - drag.sx; view.y = drag.vy + e.clientY - drag.sy; updateBg(); render(); }
  else if (drag.mode === 'part') {
    const p = P.parts.find(x => x.id === drag.id);
    const nx = snap10(w.x - drag.ox); p.x = P.settings.back ? -(nx + drag.gw) : nx; p.y = snap10(w.y - drag.oy); render();
  } else if (drag.mode === 'handle') {
    const wi = P.wires.find(x => x.id === drag.id), hd = LAY.R.get(drag.id) && LAY.R.get(drag.id).handle;
    wi.dx = Math.round(drag.dx0 + (hd && hd.axis === 'y' ? (w.y - drag.w0.y) : (P.settings.back ? -1 : 1) * (w.x - drag.w0.x))); render();
  } else if (drag.mode === 'wire') {
    drag.to = w;
    const n = nearestPin(w, 16 / view.k, pk(drag.from));
    drag.snapPin = n ? pk(n) : null; hoverPin = drag.snapPin; render(); showPinInfo(n);
    setHint('Puść na docelowym pinie, żeby połączyć.');
  }
});

function endDrag(e, cancel) {
  if (!drag) return;
  const d = drag; drag = null;
  const w = toWorld(e);
  if (d.mode === 'wire') {
    if (d.moved && !cancel) {
      const t = nearestPin(w, 16 / view.k, pk(d.from));
      if (t) { pending = null; hoverPin = null; connect(d.from, refOf(t)); setHint(); return; }
      pending = null;
    } else if (!d.moved) {
      if (pending && pk(pending) !== pk(d.from)) { const a = pending; pending = null; connect(a, d.from); setHint(); return; }
      pending = pending && pk(pending) === pk(d.from) ? null : d.from;
      setHint(pending ? 'Wybrano pin. Kliknij drugi pin, żeby go połączyć. Esc anuluje.' : undefined);
    }
    hoverPin = null; render(); return;
  }
  if (d.mode === 'part') { if (d.moved) commit({ keepSide: true }); else render(); }
  else if (d.mode === 'handle') { if (d.moved) commit({ keepSide: true }); }
  else if (d.mode === 'pan') {
    if (!d.moved) { sel = null; pending = null; render(); refreshSide(); setHint(); }
  }
}
cv.addEventListener('pointerup', e => endDrag(e, false));
cv.addEventListener('pointercancel', e => endDrag(e, true));
cv.addEventListener('pointerleave', () => { if (!drag && hoverPin) { hoverPin = null; render(); $('#stPin').textContent = ''; } });
cv.addEventListener('wheel', e => {
  e.preventDefault();
  const r = cv.getBoundingClientRect();
  zoomAt(e.clientX - r.left, e.clientY - r.top, Math.exp(-e.deltaY * (e.ctrlKey ? 0.01 : 0.0015)));
}, { passive: false });

function showPinInfo(pin) {
  const el = $('#stPin');
  if (!pin) { el.textContent = ''; return; }
  const n = P.wires.filter(w => pk(w.a) === pk(pin) || pk(w.b) === pk(pin)).length;
  el.textContent = `${LAY.G.get(pin.p).part.title} · ${pinLabel(pin)}${n ? ` · przewodów: ${n}` : ''}`;
}
const DEFAULT_HINT = 'Przeciągnij od kropki pinu do drugiego pinu, żeby poprowadzić przewód. Przewód wybierzesz kliknięciem.';
function setHint(t) { $('#stHint').textContent = t || DEFAULT_HINT; }

document.addEventListener('keydown', e => {
  const tg = e.target;
  if (tg && /^(INPUT|TEXTAREA|SELECT)$/.test(tg.tagName)) return;
  if (document.querySelector('dialog[open]')) return;
  const mod = e.ctrlKey || e.metaKey;
  if (mod && e.key.toLowerCase() === 'z') { e.preventDefault(); e.shiftKey ? redo() : undo(); }
  else if (mod && e.key.toLowerCase() === 'y') { e.preventDefault(); redo(); }
  else if (mod && e.key.toLowerCase() === 'd' && sel && sel.t === 'part') { e.preventDefault(); duplicatePart(); }
  else if (e.key === 'Delete' || e.key === 'Backspace') { if (sel) { e.preventDefault(); deleteSel(); } }
  else if (e.key === 'Escape') { pending = null; sel = null; closeMenu(); render(); refreshSide(); setHint(); }
  else if (sel && sel.t === 'part' && !mod && e.key.toLowerCase() === 'r') { e.preventDefault(); rotatePart(e.shiftKey ? -1 : 1); }
  else if (sel && sel.t === 'part' && e.key.startsWith('Arrow')) {
    e.preventDefault();
    const p = P.parts.find(x => x.id === sel.id), s = (e.shiftKey ? 50 : 10) * (P.settings.back ? -1 : 1);
    if (e.key === 'ArrowLeft') p.x -= s; if (e.key === 'ArrowRight') p.x += s; if (e.key === 'ArrowUp') p.y -= s; if (e.key === 'ArrowDown') p.y += s;
    commit({ keepSide: true });
  }
});

/* =====================  side panel  ===================== */
function warnings() {
  const out = [];
  P.wires.forEach((w, i) => {
    const a = LAY.PIN.get(pk(w.a)), b = LAY.PIN.get(pk(w.b)); if (!a || !b) return;
    if ((a.k === 'gnd' && b.k === 'pwr') || (a.k === 'pwr' && b.k === 'gnd')) out.push(`Przewód ${i + 1}: masa połączona bezpośrednio z zasilaniem (zwarcie).`);
    const ra = railOf(a.n), rb = railOf(b.n);
    if (ra && rb && ra !== rb) out.push(`Przewód ${i + 1}: ${ra} połączone z ${rb}.`);
  });
  return out;
}
const wireTitle = (w, i) => { const e = wireEnds(LAY, w); return e ? `${e.ta} → ${e.tb}` : `przewód ${i + 1}`; };

function refreshSide() {
  if (!LAY) LAY = layout();
  $$('.tabs button').forEach(b => b.classList.toggle('on', b.dataset.tab === tab));
  const el = $('#sideBody');
  if (tab === 'list') { el.innerHTML = listHTML(); return; }
  if (sel && sel.t === 'wire') el.innerHTML = wireHTML();
  else if (sel && sel.t === 'part') el.innerHTML = partHTML();
  else el.innerHTML = projectHTML();
}

function projectHTML() {
  const done = P.wires.filter(w => w.done).length, n = P.wires.length;
  const wn = warnings();
  return `<div class="sec"><h3>Postęp lutowania</h3>
    <div class="meter"><i style="width:${n ? Math.round(done / n * 100) : 0}%"></i></div>
    <div class="stat"><span>zlutowane</span><span>${done} / ${n}</span></div>
    <div class="stat"><span>elementy</span><span>${P.parts.length}</span></div>
    ${wn.map(x => `<div class="warn">${esc(x)}</div>`).join('')}
    <p class="note">Zakładka Lista zawiera pola do odhaczania przewodów.</p></div>
  <div class="sec"><h3>Jak rysować</h3><div class="help">
    <p>Przeciągnij od kropki pinu do kropki drugiego pinu. Na ekranie dotykowym stuknij jeden pin, potem drugi.</p>
    <p>Kliknij przewód, żeby zmienić kolor. Kwadracik na wybranym przewodzie przesuwa jego załamanie.</p>
    <p>Przeciągnij element za nagłówek, żeby go przesunąć. Tło przesuwa cały arkusz, kółko myszy zbliża.</p>
    <p><kbd>Del</kbd> usuwa, <kbd>Ctrl</kbd>+<kbd>Z</kbd> cofa, <kbd>Esc</kbd> anuluje, strzałki przesuwają element.</p></div></div>`;
}

function wireHTML() {
  const i = P.wires.findIndex(x => x.id === sel.id), w = P.wires[i];
  if (!w) return projectHTML();
  const e = wireEnds(LAY, w);
  const cell = (pp, pin) => `<div class="end"><b>${esc(pin.n)}</b><small>${esc(pp.title)}${pin.h ? ' · ' + esc(pin.h) : ''}</small></div>`;
  return `<div class="sec"><h3>Przewód ${i + 1}</h3>
    ${e ? `<div class="ends">${cell(e.pa, e.a)}<span aria-hidden="true">→</span>${cell(e.pb, e.b)}</div>` : ''}</div>
  <div class="sec"><span class="lbl">Kolor przewodu</span>
    <div class="sw">${Object.entries(COLORS).map(([k, v]) => `<button type="button" data-c="${k}" class="${w.color === k ? 'on' : ''}" style="background:${v[0]}" title="${v[1]}" aria-label="${v[1]}"></button>`).join('')}</div>
    <div class="stat"><span>wybrany</span><span>${COLORS[w.color][1]}</span></div></div>
  <div class="sec">
    <label class="field"><span class="lbl">Uwagi</span><input class="fld" id="wNote" maxlength="80" placeholder="np. skrócić do 6 cm" value="${esc(w.note || '')}"></label>
    <label class="chk"><input type="checkbox" id="wDone" ${w.done ? 'checked' : ''}> Zlutowane</label>
    <div class="row2">
      <button class="btn sm" id="wReset" ${w.dx ? '' : 'disabled'}>Wyzeruj załamanie</button>
      <button class="btn sm danger" id="wDel">${ico('trash')}Usuń przewód</button>
    </div></div>`;
}

function partHTML() {
  const p = P.parts.find(x => x.id === sel.id);
  if (!p) return projectHTML();
  const used = new Set(); P.wires.forEach(w => { if (w.a.p === p.id) used.add(pk(w.a)); if (w.b.p === p.id) used.add(pk(w.b)); });
  const total = p.L.length + p.R.length;
  return `<div class="sec"><h3>Element</h3>
    <label class="field"><span class="lbl">Nazwa</span><input class="fld" id="pTitle" maxlength="40" value="${esc(p.title)}"></label>
    <label class="field"><span class="lbl">Podtytuł</span><input class="fld" id="pSub" maxlength="60" value="${esc(p.sub || '')}"></label>
    <label class="field"><span class="lbl">Skrót na liście</span><input class="fld" id="pShort" maxlength="12" value="${esc(p.short || '')}"></label>
    <div class="stat"><span>piny podłączone</span><span>${used.size} / ${total}</span></div></div>
  <div class="sec"><div class="row2">
    <button class="btn sm" id="pEdit">${ico('edit')}Edytuj piny</button>
    <button class="btn sm" id="pRot">${ico('rotate')}Obróć</button>
    <button class="btn sm" id="pMirror">${ico('flip')}Odwróć strony</button>
    <button class="btn sm" id="pDup">${ico('copy')}Duplikuj</button>
    <button class="btn sm danger" id="pDel">${ico('trash')}Usuń</button></div></div>`;
}

function listHTML() {
  if (!P.wires.length) return '<div class="sec"><p class="note">Brak przewodów. Połącz dwa piny, a pojawią się tutaj z polem do odhaczania.</p></div>';
  const done = P.wires.filter(w => w.done).length;
  return `<div class="sec"><div class="meter"><i style="width:${Math.round(done / P.wires.length * 100)}%"></i></div>
    <div class="stat"><span>zlutowane</span><span>${done} / ${P.wires.length}</span></div></div>
    <ul class="wl">${P.wires.map((w, i) => `<li data-w="${w.id}" class="${w.done ? 'done' : ''} ${sel && sel.id === w.id ? 'on' : ''}">
      <input type="checkbox" data-done="${w.id}" ${w.done ? 'checked' : ''} aria-label="Zlutowane: przewód ${i + 1}">
      <span class="n">${i + 1}</span><span class="c" style="background:${COLORS[w.color][0]}"></span>
      <span class="t">${esc(wireTitle(w, i))}</span></li>`).join('')}</ul>`;
}

$('#sideBody').addEventListener('click', e => {
  const t = e.target;
  const btn = t.closest('button'); const li = t.closest('li[data-w]');
  if (btn && btn.dataset.c && sel && sel.t === 'wire') { P.wires.find(x => x.id === sel.id).color = btn.dataset.c; commit(); return; }
  if (btn && btn.id === 'wDel') deleteSel();
  else if (btn && btn.id === 'wReset') { P.wires.find(x => x.id === sel.id).dx = 0; commit(); }
  else if (btn && btn.id === 'pEdit') openPartDialog(sel.id);
  else if (btn && btn.id === 'pRot') rotatePart();
  else if (btn && btn.id === 'pMirror') mirrorPart();
  else if (btn && btn.id === 'pDup') duplicatePart();
  else if (btn && btn.id === 'pDel') deleteSel();
  else if (t.matches('input[data-done]')) { const w = P.wires.find(x => x.id === t.dataset.done); w.done = t.checked; commit(); }
  else if (li) { sel = { t: 'wire', id: li.dataset.w }; render(); refreshSide(); }
});
$('#sideBody').addEventListener('input', e => {
  const t = e.target;
  if (t.id === 'wNote') P.wires.find(x => x.id === sel.id).note = t.value;
  else if (t.id === 'wDone') { P.wires.find(x => x.id === sel.id).done = t.checked; commit({ keepSide: true }); }
  else if (t.id === 'pTitle' || t.id === 'pSub' || t.id === 'pShort') {
    const p = P.parts.find(x => x.id === sel.id);
    if (t.id === 'pTitle') p.title = t.value || 'Element'; else if (t.id === 'pSub') p.sub = t.value; else p.short = t.value;
    render();
  }
});
$('#sideBody').addEventListener('change', e => { if (['wNote', 'pTitle', 'pSub', 'pShort'].includes(e.target.id)) commit({ keepSide: true }); });
$$('.tabs button').forEach(b => b.addEventListener('click', () => { tab = b.dataset.tab; refreshSide(); }));

/* =====================  library  ===================== */
function buildLib(q) {
  const term = (q || '').trim().toLowerCase();
  let html = '';
  for (const cat of CAT_ORDER) {
    const items = TPL.filter(t => t.cat === cat && (!term || (t.title + ' ' + t.sub + ' ' + t.short).toLowerCase().includes(term)));
    if (!items.length) continue;
    html += `<div class="cat"><h3><span style="background:${CAT[cat]}"></span>${cat}</h3>` +
      items.map(t => `<button class="item" data-tpl="${t.id}"><b>${esc(t.title)}</b><small>${t.L.length + t.R.length} pin</small></button>`).join('') + '</div>';
  }
  $('#libList').innerHTML = html || '<p class="note" style="padding-top:12px">Brak wyników. Dodaj własny element przyciskiem poniżej.</p>';
}
$('#libList').addEventListener('click', e => { const b = e.target.closest('[data-tpl]'); if (b) addPart(b.dataset.tpl); });
$('#q').addEventListener('input', e => buildLib(e.target.value));
$('#btnCustom').addEventListener('click', () => openPartDialog(null));

/* =====================  top bar  ===================== */
function syncTop() {
  $('#pname').value = P.name;
  $$('[data-ws]').forEach(b => b.classList.toggle('on', b.dataset.ws === P.settings.wireStyle));
  $('#optNums').checked = P.settings.numbers;
  $('#btnBack').classList.toggle('on', !!P.settings.back); $('#btnBack').setAttribute('aria-pressed', P.settings.back ? 'true' : 'false');
}
function updateUndoButtons() { $('#btnUndo').disabled = hist.i <= 0; $('#btnRedo').disabled = hist.i >= hist.stack.length - 1; }
$('#pname').addEventListener('input', e => { P.name = e.target.value.slice(0, 60); touched(); });
$$('[data-ws]').forEach(b => b.addEventListener('click', () => { P.settings.wireStyle = b.dataset.ws; syncTop(); touched(); render(); refreshSide(); }));
$('#optNums').addEventListener('change', e => { P.settings.numbers = e.target.checked; touched(); render(); });
$('#btnBack').addEventListener('click', () => { P.settings.back = !P.settings.back; sel = null; pending = null; syncTop(); touched(); refreshSide(); toast(P.settings.back ? 'Widok od spodu: schemat odbity lustrzanie, tak jak przy lutowaniu.' : 'Widok od góry.'); requestAnimationFrame(fitView); });
$('#btnUndo').addEventListener('click', undo);
$('#btnRedo').addEventListener('click', redo);
$('#zIn').addEventListener('click', () => { const r = cv.getBoundingClientRect(); zoomAt(r.width / 2, r.height / 2, 1.2); });
$('#zOut').addEventListener('click', () => { const r = cv.getBoundingClientRect(); zoomAt(r.width / 2, r.height / 2, 1 / 1.2); });
$('#zFit').addEventListener('click', fitView);
$('#btnSample').addEventListener('click', () => { const p = sampleProject(); if (P.parts.length === 0) { p.id = P.id; } openProject(p); touchedNew(); });
function touchedNew() { delete P.isSample; touched(); }
function closeDrawers() { $('#app').classList.remove('showLib', 'showSide'); }
$('#tgLib').addEventListener('click', () => { $('#app').classList.toggle('showLib'); $('#app').classList.remove('showSide'); });
$('#tgSide').addEventListener('click', () => { $('#app').classList.toggle('showSide'); $('#app').classList.remove('showLib'); });

/* ---------- toast ---------- */
let toastT = null;
function toast(msg) {
  const el = $('#toast'); el.textContent = msg; el.hidden = false;
  clearTimeout(toastT); toastT = setTimeout(() => { el.hidden = true; }, 3400);
}

/* ---------- projects menu ---------- */
const menu = $('#menu');
function closeMenu() { menu.hidden = true; }
let delArm = null;
function openMenu() {
  const ix = [...INDEX.list].sort((a, b) => b.updated - a.updated);
  const r = $('#btnProj').getBoundingClientRect();
  menu.style.left = Math.max(12, Math.min(r.left, innerWidth - 352)) + 'px'; menu.style.top = (r.bottom + 6) + 'px';
  menu.innerHTML = `<button class="mi" data-a="new">${ico('plus')}Nowy pusty schemat</button>
    <button class="mi" data-a="sample">Wczytaj przykład (ESP32 + TFT)</button>
    <button class="mi" data-a="dup">${ico('copy')}Duplikuj ten schemat</button>
    <button class="mi" data-a="json">Kopia projektu (JSON)…</button><hr><div class="h">Zapisane schematy</div>` +
    (ix.length ? ix.map(x => `<div style="display:flex;gap:4px"><button class="mi ${x.id === P.id ? 'cur' : ''}" style="flex:1;min-width:0" data-open="${x.id}"><span style="overflow:hidden;text-overflow:ellipsis;white-space:nowrap">${esc(x.name)}</span><small>${new Date(x.updated).toLocaleDateString('pl-PL')}</small></button>
      <button class="btn icon sm danger" style="align-self:center" data-del="${x.id}" aria-label="Usuń schemat" title="${delArm === x.id ? 'Kliknij jeszcze raz, żeby usunąć' : 'Usuń'}">${delArm === x.id ? '?' : ico('trash')}</button></div>`).join('')
      : '<div class="h" style="text-transform:none;letter-spacing:0;font-weight:400">Pierwszy zapis pojawi się po zmianie na arkuszu.</div>');
  menu.hidden = false;
}
$('#btnProj').addEventListener('click', e => { e.stopPropagation(); if (menu.hidden) { delArm = null; openMenu(); } else closeMenu(); });
document.addEventListener('pointerdown', e => { if (!menu.hidden && !menu.contains(e.target) && !$('#btnProj').contains(e.target)) closeMenu(); });
menu.addEventListener('click', async e => {
  const b = e.target.closest('button'); if (!b) return;
  if (dirty) await saveNow();
  if (b.dataset.a === 'new') { closeMenu(); const p = blankProject(); openProject(p); touchedNew(); }
  else if (b.dataset.a === 'sample') { closeMenu(); const p = sampleProject(); openProject(p); touchedNew(); }
  else if (b.dataset.a === 'dup') { closeMenu(); const p = sanitize(JSON.parse(JSON.stringify(P))); p.id = uid(); p.name = P.name + ' (kopia)'; openProject(p, { noFit: true }); touchedNew(); toast('Utworzono kopię schematu.'); }
  else if (b.dataset.a === 'json') { closeMenu(); $('#jsonBox').value = JSON.stringify(P, null, 1); $('#dlgJson').showModal(); }
  else if (b.dataset.open) { closeMenu(); const p = await loadProject(b.dataset.open); if (p) openProject(p); else toast('Nie udało się wczytać schematu z karty SD.'); }
  else if (b.dataset.del) {
    if (delArm !== b.dataset.del) { delArm = b.dataset.del; openMenu(); return; }
    const id = b.dataset.del; delArm = null;
    try { await api.del('p_' + id); } catch (err) { /* plik mógł już nie istnieć */ }
    INDEX.list = INDEX.list.filter(x => x.id !== id); if (INDEX.last === id) INDEX.last = null;
    try { await api.put('index', JSON.stringify(INDEX)); } catch (err) { toast('Nie udało się zaktualizować listy na karcie SD.'); }
    if (id === P.id) {
      const ix = [...INDEX.list].sort((a, c) => c.updated - a.updated);
      const nxt = ix.length ? await loadProject(ix[0].id) : null;
      openProject(nxt || blankProject()); if (!nxt) touchedNew();
    }
    openMenu();
  }
});

/* ---------- JSON dialog ---------- */
$('#jsClose').addEventListener('click', () => $('#dlgJson').close());
$('#jsCopy').addEventListener('click', async () => {
  const box = $('#jsonBox');
  try { await navigator.clipboard.writeText(box.value); toast('Skopiowano do schowka.'); }
  catch (e) { box.select(); try { document.execCommand('copy'); toast('Skopiowano do schowka.'); } catch (e2) { toast('Zaznacz tekst i skopiuj go ręcznie.'); } }
});
$('#jsSave').addEventListener('click', () => saveFile((P.name || 'schemat').replace(/[^\w\-. ąćęłńóśźżĄĆĘŁŃÓŚŹŻ]+/g, '_') + '.json', $('#jsonBox').value));
$('#jsLoad').addEventListener('click', () => {
  try {
    const raw = JSON.parse($('#jsonBox').value);
    const p = sanitize(raw);
    if (!p.parts.length) throw new Error('empty');
    p.id = uid(); $('#dlgJson').close(); openProject(p); touchedNew(); toast('Wczytano schemat jako nowy projekt.');
  } catch (e) { toast('Nie rozpoznano tekstu. Wklej cały JSON z kopii projektu.'); }
});

/* ---------- print & export ---------- */
function saveFile(name, data) {
  const type = name.endsWith('.svg') ? 'image/svg+xml' : name.endsWith('.png') ? 'image/png' : 'text/plain;charset=utf-8';
  const blob = data instanceof Blob ? data : new Blob([data], { type });
  const a = document.createElement('a');
  a.href = URL.createObjectURL(blob); a.download = name; document.body.appendChild(a); a.click(); a.remove();
  setTimeout(() => URL.revokeObjectURL(a.href), 4000);
  toast('Pobrano ' + name);
}
const fileBase = () => (P.name || 'schemat').replace(/[^\w\-. ąćęłńóśźżĄĆĘŁŃÓŚŹŻ]+/g, '_');
function buildSheet() {
  const orient = P.settings.orient;
  const r = sheetSVG(orient);
  $$('[data-or]').forEach(b => b.classList.toggle('on', b.dataset.or === orient));
  $('#pvBody').innerHTML = r.svg;
  $('#printRoot').innerHTML = r.svg;
  let st = $('#pgstyle'); if (!st) { st = document.createElement('style'); st.id = 'pgstyle'; document.head.appendChild(st); }
  st.textContent = `@page{size:A4 ${orient};margin:8mm}`;
  $('#pvNote').textContent = `Skala wydruku ok. ${Math.round(r.scale * 100)}% rozmiaru z ekranu. Jeśli okno drukowania się nie otworzy, pobierz PNG i wydrukuj go zwykłym sposobem.`;
  return r;
}
$('#btnPrint').addEventListener('click', () => { buildSheet(); $('#dlgPrint').showModal(); });
$('#pvClose').addEventListener('click', () => $('#dlgPrint').close());
$$('[data-or]').forEach(b => b.addEventListener('click', () => { P.settings.orient = b.dataset.or; touched(); buildSheet(); }));
$('#pvPrint').addEventListener('click', () => { buildSheet(); try { window.print(); } catch (e) { toast('Drukowanie jest zablokowane w tym widoku. Pobierz PNG albo użyj Ctrl+P.'); } });
window.addEventListener('beforeprint', () => { if (P) buildSheet(); });
$('#pvSvg').addEventListener('click', () => saveFile(fileBase() + '.svg', buildSheet().svg));
$('#pvCsv').addEventListener('click', () => saveFile(fileBase() + '-lista.csv', csvText()));
$('#pvPng').addEventListener('click', async () => {
  const r = buildSheet();
  try {
    const img = new Image();
    await new Promise((res, rej) => { img.onload = res; img.onerror = rej; img.src = 'data:image/svg+xml;charset=utf-8,' + encodeURIComponent(r.svg); });
    const c = document.createElement('canvas'); c.width = r.W * 2; c.height = r.H * 2;
    const x = c.getContext('2d'); x.fillStyle = '#fff'; x.fillRect(0, 0, c.width, c.height); x.drawImage(img, 0, 0, c.width, c.height);
    const blob = await new Promise(res => c.toBlob(res, 'image/png'));
    if (!blob) throw new Error('blob');
    saveFile(fileBase() + '.png', blob);
  } catch (e) { toast('Nie udało się wygenerować PNG.'); }
});

/* =====================  boot  ===================== */
$$('[data-ico]').forEach(el => el.insertAdjacentHTML('afterbegin', ico(el.dataset.ico)));
buildLib('');
setHint();

(async function boot() {
  openProject(sampleProject(), { noFit: true });
  setStatus('Łączenie z ESP32…');
  new ResizeObserver(() => { if (!boot.fitted && cv.getBoundingClientRect().width) { boot.fitted = true; fitView(); } }).observe(cv);
  try {
    const st = await (await fetch('/api/status', { cache: 'no-store' })).text();
    if (st.trim() !== 'ok') { setStatus('Brak karty SD: zapis niemożliwy'); return; }
    const t = await api.get('index');
    if (t) { const o = JSON.parse(t); if (o && Array.isArray(o.list)) INDEX = { last: o.last || null, list: o.list.filter(x => x && typeof x.id === 'string') }; }
    const ix = [...INDEX.list].sort((a, c) => c.updated - a.updated);
    const id = INDEX.last && INDEX.list.some(x => x.id === INDEX.last) ? INDEX.last : (ix[0] && ix[0].id);
    const p = id ? await loadProject(id) : null;
    if (p) { openProject(p); setStatus('Wczytano z karty SD'); }
    else setStatus('Przykład (niezapisany). Zapis nastąpi po pierwszej zmianie');
  } catch (e) { setStatus('Brak połączenia z ESP32'); }
})();
</script>
</body></html>
)PLAN";

// ===================== SERWER =====================
static bool validName(const String& n) {
  if (n.length() == 0 || n.length() > 40) return false;
  for (size_t i = 0; i < n.length(); i++) {
    char c = n[i];
    if (!(isalnum((unsigned char)c) || c == '_' || c == '-')) return false;
  }
  return true;
}

static String pathFor(const String& n) { return "/plan/" + n + ".json"; }

void handleRoot() {
  server.sendHeader("Cache-Control", "no-cache");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html; charset=utf-8", "");
  server.sendContent_P(PAGE_1);
  server.sendContent_P(PAGE_2);
  server.sendContent_P(PAGE_3);
  server.sendContent("");
}

void handleStatus() {
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "text/plain", sdOk ? "ok" : "nosd");
}

void handleFileGet() {
  if (!sdOk) { server.send(503, "text/plain", "brak karty SD"); return; }
  String n = server.arg("n");
  if (!validName(n)) { server.send(400, "text/plain", "zla nazwa"); return; }
  String p = pathFor(n);
  if (!SD.exists(p)) { server.send(404, "text/plain", "brak pliku"); return; }
  File f = SD.open(p, FILE_READ);
  if (!f) { server.send(500, "text/plain", "blad odczytu"); return; }
  server.sendHeader("Cache-Control", "no-store");
  server.streamFile(f, "application/json");
  f.close();
}

void handleFilePost() {
  if (!sdOk) { server.send(503, "text/plain", "brak karty SD"); return; }
  String n = server.arg("n");
  if (!validName(n)) { server.send(400, "text/plain", "zla nazwa"); return; }
  String body = server.arg("plain");
  if (body.length() == 0) { server.send(400, "text/plain", "pusta tresc"); return; }
  File f = SD.open(pathFor(n), FILE_WRITE);   // FILE_WRITE = nadpisz plik
  if (!f) { server.send(500, "text/plain", "blad zapisu"); return; }
  size_t w = f.write((const uint8_t*)body.c_str(), body.length());
  f.close();
  if (w != body.length()) { server.send(500, "text/plain", "zapis niepelny"); return; }
  server.send(200, "text/plain", "ok");
}

void handleFileDelete() {
  if (!sdOk) { server.send(503, "text/plain", "brak karty SD"); return; }
  String n = server.arg("n");
  if (!validName(n)) { server.send(400, "text/plain", "zla nazwa"); return; }
  String p = pathFor(n);
  if (SD.exists(p)) SD.remove(p);
  server.send(200, "text/plain", "ok");
}

void handleNotFound() { server.send(404, "text/plain", "404"); }

// ===================== START =====================
void startWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Lacze z WiFi");
  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 20000) {
    delay(400);
    Serial.print(".");
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Adres strony: http://");
    Serial.println(WiFi.localIP());
    if (MDNS.begin("plan")) Serial.println("albo: http://plan.local");
  } else {
    WiFi.mode(WIFI_AP);
    WiFi.softAP("PlanLutowania", "lutowanie");
    Serial.println("Brak WiFi - wlasna siec: PlanLutowania (haslo: lutowanie)");
    Serial.print("Adres strony: http://");
    Serial.println(WiFi.softAPIP());
  }
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\nPlan Lutowania - start");

  sdSPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
  sdOk = SD.begin(SD_CS, sdSPI, 4000000);
  if (sdOk) {
    if (!SD.exists("/plan")) SD.mkdir("/plan");
    Serial.println("Karta SD: OK");
  } else {
    Serial.println("Karta SD: BLAD (strona sie otworzy, ale zapis nie zadziala)");
  }

  startWifi();

  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/status", HTTP_GET, handleStatus);
  server.on("/api/file", HTTP_GET, handleFileGet);
  server.on("/api/file", HTTP_POST, handleFilePost);
  server.on("/api/del", HTTP_POST, handleFileDelete);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("Serwer WWW dziala");
}

void loop() {
  server.handleClient();
  delay(2);
}
