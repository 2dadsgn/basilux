#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <time.h>
#include <Wire.h>
#include <Adafruit_ADS1X15.h>

// --- Wi-Fi Credentials ---
const char* ssid = "test";
const char* password = "test";

// --- Static IP Configuration ---
IPAddress local_IP(192, 168, 1, 150);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8);
IPAddress secondaryDNS(1, 1, 1, 1);

// --- Hardware ---
const int pwmPin = 4;       // GPIO 4 (D2) - Main Plant Light (TIP122)

// I2C Pins for ADS1115
const int sdaPin = 12;      // GPIO 12 (D6)
const int sclPin = 14;      // GPIO 14 (D5)

Adafruit_ADS1115 ads;       // Inizializza il modulo ADC a 16-bit

// --- Taratura Sensori (DA MODIFICARE IN BASE AI TUOI SENSORI) ---
const int airValue = 20000; 
const int waterValue = 10000; 

// Variabili per l'umidità calcolata (0-100%)
int moisture1 = 0;
int moisture2 = 0;
int moisture3 = 0;
unsigned long lastMoistureCheck = 0;

// --- Time & State Variables ---
const char* MY_TZ = "CET-1CEST,M3.5.0,M10.5.0/3";
int brightness = 1023; 
bool manualOverride = false;
bool manualState = false;

int onHour = 8;    
int offHour = 20;  

ESP8266WebServer server(80);

void applyLight(int duty) {
  duty = constrain(duty, 0, 1023);
  analogWrite(pwmPin, duty);
}

// Converte il valore grezzo dell'ADS1115 in percentuale 0-100%
int getMoisturePercent(int rawValue) {
  int percent = map(rawValue, airValue, waterValue, 0, 100);
  return constrain(percent, 0, 100);
}

// --- HTML & Frontend ---
const char HTML_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Basilux Controller</title>
  <style>
    * { box-sizing: border-box; font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif; }
    body { background: #121212; color: #f0f0f0; margin: 0; padding: 20px; display: flex; justify-content: center; }
    .card { background: #1e1e1e; border-radius: 16px; padding: 24px; width: 100%; max-width: 380px; box-shadow: 0 8px 24px rgba(0,0,0,0.4); }
    h2 { margin: 0 0 16px; font-size: 20px; text-align: center; color: #81c784; }
    
    /* Plant Indicators */
    .plants-grid { display: flex; justify-content: space-between; margin-bottom: 24px; background: #2a2a2a; padding: 16px 10px; border-radius: 12px; }
    .plant-box { text-align: center; width: 30%; }
    .plant-wrapper { position: relative; width: 60px; height: 60px; margin: 0 auto 8px; }
    .circular-chart { display: block; margin: 0 auto; width: 100%; height: 100%; }
    .circle-bg { fill: none; stroke: #3a3a3a; stroke-width: 3; }
    .circle { fill: none; stroke-width: 3; stroke-linecap: round; transition: stroke-dasharray 1s ease-out, stroke 0.5s; }
    .plant-icon { position: absolute; top: 50%; left: 50%; transform: translate(-50%, -50%); font-size: 24px; }
    .moisture-val { font-size: 14px; font-weight: bold; color: #fff; }
    
    /* General UI */
    .status-text { font-weight: bold; color: #4caf50; font-size: 16px; }
    label { display: block; font-size: 12px; color: #aaa; margin-top: 14px; margin-bottom: 6px; text-transform: uppercase; letter-spacing: 1px; }
    .slider { width: 100%; height: 8px; border-radius: 4px; background: #333; outline: none; -webkit-appearance: none; }
    .slider::-webkit-slider-thumb { -webkit-appearance: none; width: 22px; height: 22px; border-radius: 50%; background: #81c784; cursor: pointer; }
    .val-display { text-align: right; font-size: 14px; color: #81c784; font-weight: bold; float: right; }
    .time-grid, .btn-grid { display: flex; gap: 8px; margin-top: 6px; }
    select { flex: 1; padding: 10px; background: #2a2a2a; border: 1px solid #444; border-radius: 8px; color: #fff; font-size: 15px; }
    button { flex: 1; padding: 12px; border: none; border-radius: 8px; font-size: 13px; font-weight: bold; cursor: pointer; transition: 0.2s; }
    button:active { transform: scale(0.95); }
    
    .btn-save { background: #2e7d32; color: white; width: 100%; margin-top: 12px; }
    .btn-off { background: #c62828; color: white; }
    .btn-auto { background: #1565c0; color: white; }
    .btn-on { background: #388e3c; color: white; }
    .btn-low { background: #e67e22; color: white; }
    .btn-fast { background: #e65100; color: white; }
    .header-box { text-align: center; font-size: 14px; color: #aaa; margin-bottom: 16px; }
  </style>
</head>
<body>
  <div class="card">
    <h2>🌿 Basilux Control</h2>
    <div class="header-box">
      Clock: <span id="clock">--:--</span> &nbsp;|&nbsp; Luce: <span class="status-text" id="lightStatus">--</span>
    </div>

    <!-- Indicatori Piante Circolari -->
    <div class="plants-grid">
      <div class="plant-box">
        <div class="plant-wrapper">
          <svg viewBox="0 0 36 36" class="circular-chart">
            <path class="circle-bg" d="M18 2.0845 a 15.9155 15.9155 0 0 1 0 31.831 a 15.9155 15.9155 0 0 1 0 -31.831"/>
            <path class="circle" id="circle1" stroke="#4caf50" stroke-dasharray="0, 100" d="M18 2.0845 a 15.9155 15.9155 0 0 1 0 31.831 a 15.9155 15.9155 0 0 1 0 -31.831"/>
          </svg>
          <div class="plant-icon">🌱</div>
        </div>
        <div class="moisture-val" id="text1">--%</div>
      </div>
      
      <div class="plant-box">
        <div class="plant-wrapper">
          <svg viewBox="0 0 36 36" class="circular-chart">
            <path class="circle-bg" d="M18 2.0845 a 15.9155 15.9155 0 0 1 0 31.831 a 15.9155 15.9155 0 0 1 0 -31.831"/>
            <path class="circle" id="circle2" stroke="#4caf50" stroke-dasharray="0, 100" d="M18 2.0845 a 15.9155 15.9155 0 0 1 0 31.831 a 15.9155 15.9155 0 0 1 0 -31.831"/>
          </svg>
          <div class="plant-icon">🌿</div>
        </div>
        <div class="moisture-val" id="text2">--%</div>
      </div>

      <div class="plant-box">
        <div class="plant-wrapper">
          <svg viewBox="0 0 36 36" class="circular-chart">
            <path class="circle-bg" d="M18 2.0845 a 15.9155 15.9155 0 0 1 0 31.831 a 15.9155 15.9155 0 0 1 0 -31.831"/>
            <path class="circle" id="circle3" stroke="#4caf50" stroke-dasharray="0, 100" d="M18 2.0845 a 15.9155 15.9155 0 0 1 0 31.831 a 15.9155 15.9155 0 0 1 0 -31.831"/>
          </svg>
          <div class="plant-icon">🪴</div>
        </div>
        <div class="moisture-val" id="text3">--%</div>
      </div>
    </div>

    <label>Presets</label>
    <div class="btn-grid">
      <button class="btn-low" onclick="setPreset('low')">🌱 LOW GROWTH</button>
      <button class="btn-fast" onclick="setPreset('fast')">🚀 FAST GROWTH</button>
    </div>

    <label>Brightness <span class="val-display" id="brightVal">100%</span></label>
    <input type="range" min="0" max="1023" value="1023" class="slider" id="brightSlider" oninput="changeBright(this.value)">

    <label>Schedule (Daily)</label>
    <div class="time-grid">
      <select id="onSelect"></select>
      <select id="offSelect"></select>
    </div>
    <button class="btn-save" onclick="saveSchedule()">Save Custom Schedule</button>

    <label>Manual Override</label>
    <div class="btn-grid">
      <button class="btn-off" onclick="setMode('off')">OFF</button>
      <button class="btn-auto" onclick="setMode('auto')">AUTO</button>
      <button class="btn-on" onclick="setMode('on')">ON</button>
    </div>
  </div>

  <script>
    function populateDropdowns() {
      const onSel = document.getElementById('onSelect');
      const offSel = document.getElementById('offSelect');
      for (let i = 0; i < 24; i++) {
        let label = (i < 10 ? '0' : '') + i + ':00';
        onSel.options.add(new Option('ON: ' + label, i));
        offSel.options.add(new Option('OFF: ' + label, i));
      }
    }
    populateDropdowns();

    function updatePlantRing(id, percent) {
      document.getElementById('text' + id).innerText = percent + '%';
      let circle = document.getElementById('circle' + id);
      circle.setAttribute('stroke-dasharray', percent + ', 100');
      // Diventa rosso se sotto al 30%, altrimenti verde
      circle.setAttribute('stroke', percent < 30 ? '#ef5350' : '#4caf50');
    }

    function fetchStatus() {
      fetch('/api/status').then(r => r.json()).then(d => {
        document.getElementById('clock').innerText = d.time;
        document.getElementById('lightStatus').innerText = d.lamp;
        document.getElementById('lightStatus').style.color = d.lamp === 'ON' ? '#81c784' : '#ef5350';
        document.getElementById('brightVal').innerText = Math.round((d.bright / 1023) * 100) + '%';
        document.getElementById('brightSlider').value = d.bright;
        document.getElementById('onSelect').value = d.onH;
        document.getElementById('offSelect').value = d.offH;
        
        // Aggiorna icone
        updatePlantRing(1, d.m1);
        updatePlantRing(2, d.m2);
        updatePlantRing(3, d.m3);
      });
    }

    function changeBright(val) {
      document.getElementById('brightVal').innerText = Math.round((val / 1023) * 100) + '%';
      fetch('/api/set?bright=' + val);
    }

    function saveSchedule() {
      const onH = document.getElementById('onSelect').value;
      const offH = document.getElementById('offSelect').value;
      fetch(`/api/schedule?on=${onH}&off=${offH}`).then(() => fetchStatus());
    }

    function setMode(mode) {
      fetch('/api/mode?set=' + mode).then(() => fetchStatus());
    }

    function setPreset(type) {
      fetch('/api/preset?type=' + type).then(() => fetchStatus());
    }

    fetchStatus();
    setInterval(fetchStatus, 3000);
  </script>
</body>
</html>
)rawliteral";

// --- API Handlers ---

void handleRoot() {
  server.send_P(200, "text/html", HTML_PAGE);
}

void handleStatus() {
  time_t now = time(nullptr);
  struct tm* t = localtime(&now);

  char timeBuf[10];
  strftime(timeBuf, sizeof(timeBuf), "%H:%M:%S", t);

  bool lampOn = false;
  if (manualOverride) {
    lampOn = manualState;
  } else {
    int h = t->tm_hour;
    if (onHour < offHour) {
      lampOn = (h >= onHour && h < offHour);
    } else {
      lampOn = (h >= onHour || h < offHour);
    }
  }

  String json = "{";
  json += "\"time\":\"" + String(timeBuf) + "\",";
  json += "\"lamp\":\"" + String(lampOn ? "ON" : "OFF") + "\",";
  json += "\"bright\":" + String(brightness) + ",";
  json += "\"onH\":" + String(onHour) + ",";
  json += "\"offH\":" + String(offHour) + ",";
  json += "\"m1\":" + String(moisture1) + ",";
  json += "\"m2\":" + String(moisture2) + ",";
  json += "\"m3\":" + String(moisture3);
  json += "}";

  server.send(200, "application/json", json);
}

void handleSetBright() {
  if (server.hasArg("bright")) {
    brightness = server.arg("bright").toInt();
    brightness = constrain(brightness, 0, 1023);
  }
  server.send(200, "text/plain", "OK");
}

void handleSchedule() {
  if (server.hasArg("on") && server.hasArg("off")) {
    onHour = server.arg("on").toInt();
    offHour = server.arg("off").toInt();
    manualOverride = false;
  }
  server.send(200, "text/plain", "OK");
}

void handleMode() {
  if (server.hasArg("set")) {
    String m = server.arg("set");
    if (m == "on") {
      manualOverride = true;
      manualState = true;
    } else if (m == "off") {
      manualOverride = true;
      manualState = false;
    } else {
      manualOverride = false;
    }
  }
  server.send(200, "text/plain", "OK");
}

void handlePreset() {
  if (server.hasArg("type")) {
    String type = server.arg("type");
    if (type == "low") {
      onHour = 9;         
      offHour = 17;       
      brightness = 512;   
      manualOverride = false;
    } else if (type == "fast") {
      onHour = 8;         
      offHour = 20;       
      brightness = 1023;  
      manualOverride = false;
    }
  }
  server.send(200, "text/plain", "OK");
}

// --- Setup & Loop ---

void setup() {
  Serial.begin(115200);
  
  pinMode(pwmPin, OUTPUT);
  analogWriteRange(1023);
  analogWriteFreq(1000);
  applyLight(0);

  // Inizializza I2C sui pin custom (D6 per SDA, D5 per SCL)
  Wire.begin(sdaPin, sclPin);
  
  // Inizializza il modulo ADS1115
  if (!ads.begin()) {
    Serial.println("Errore: ADS1115 non trovato!");
  } else {
    Serial.println("ADS1115 inizializzato con successo.");
  }

  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("Static IP configuration failed!");
  }

  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi connected!");
  Serial.print("Access UI at: http://");
  Serial.println(WiFi.localIP());

  configTime(MY_TZ, "pool.ntp.org", "time.nist.gov");

  server.on("/", handleRoot);
  server.on("/api/status", handleStatus);
  server.on("/api/set", handleSetBright);
  server.on("/api/schedule", handleSchedule);
  server.on("/api/mode", handleMode);
  server.on("/api/preset", handlePreset);

  server.begin();
}

void loop() {
  server.handleClient();
  
  // Gestione della luce senza blocchi
  time_t now = time(nullptr);
  struct tm* t = localtime(&now);

  bool lampActive = false;
  if (manualOverride) {
    lampActive = manualState;
  } else {
    int h = t->tm_hour;
    if (onHour < offHour) {
      lampActive = (h >= onHour && h < offHour);
    } else {
      lampActive = (h >= onHour || h < offHour); 
    }
  }
  applyLight(lampActive ? brightness : 0);

  // Controllo Umidità non bloccante ogni 5 secondi
  if (millis() - lastMoistureCheck >= 5000) {
    lastMoistureCheck = millis();
    
    // Legge i 3 ingressi analogici
    int16_t adc0 = ads.readADC_SingleEnded(0); 
    int16_t adc1 = ads.readADC_SingleEnded(1); 
    int16_t adc2 = ads.readADC_SingleEnded(2); 

    // Converti in percentuale
    moisture1 = getMoisturePercent(adc0);
    moisture2 = getMoisturePercent(adc1);
    moisture3 = getMoisturePercent(adc2);

    // Stampa per debug
    Serial.printf("Raw ADS: A0=%d, A1=%d, A2=%d | ", adc0, adc1, adc2);
    Serial.printf("Percentuali: M1=%d%%, M2=%d%%, M3=%d%%\n", moisture1, moisture2, moisture3);
  }
}
