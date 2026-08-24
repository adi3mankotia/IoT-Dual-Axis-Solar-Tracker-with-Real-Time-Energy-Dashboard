#include <ESP32Servo.h>
#include <Wire.h>
#include <Adafruit_INA219.h>
#include <WiFi.h>
#include <WebServer.h>

// -------------------------
// WiFi website setup
// -------------------------
const char* ssid = "Your_wifi's_name";
const char* password = "Your_wifi's_password";

WebServer server(80);

// Website display values
float latestVoltage = 0;
float latestCurrent = 0;
float latestPower = 0;
int latestHDiff = 0;
int latestVDiff = 0;

// INA219 sensor
Adafruit_INA219 ina219;
bool ina219OK = false;

// Servos
Servo horizontal;   // bottom servo = left/right
Servo vertical;     // top servo = up/down

// Servo positions
int servohori = 90;
int servovert = 90;

// Servo limits
int servohoriLimitHigh = 160;
int servohoriLimitLow  = 20;

int servovertLimitHigh = 160;
int servovertLimitLow  = 20;

// LDR pins
int ldrTL = 34;      // Top Left
int ldrTR = 35;      // Top Right
int ldrBL = 32;      // Bottom Left
int ldrBRPin = 33;   // Bottom Right

// Servo pins
int horizontalServoPin = 18;  // bottom servo
int verticalServoPin   = 19;  // top servo

// Settings that worked for BOTH
int tolerance = 500;
int stepSize = 2;
int dtime = 50;
int stableNeeded = 1;

bool invertLDR = false;

// Directions that worked
bool reverseHorizontal = false;
bool reverseVertical = false;

// Calibration values
int baseTL = 0;
int baseTR = 0;
int baseBL = 0;
int baseBRValue = 0;

// Stability tracking for bottom servo
int lastDirectionH = 0;
int stableCountH = 0;

// Stability tracking for top servo
int lastDirectionV = 0;
int stableCountV = 0;

// -------------------------
// Website page
// -------------------------
void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <title>Solar Tracker Dashboard</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">

  <style>
    body {
      margin: 0;
      font-family: Arial, sans-serif;
      background: radial-gradient(circle at top, #14532d 0%, #0f172a 45%, #020617 100%);
      color: white;
      min-height: 100vh;
      padding: 24px;
    }

    .hero {
      max-width: 1100px;
      margin: 0 auto 24px auto;
      padding: 28px;
      border-radius: 28px;
      background: rgba(15, 23, 42, 0.86);
      border: 1px solid rgba(255,255,255,0.15);
      box-shadow: 0 20px 60px rgba(0,0,0,0.35);
      text-align: center;
    }

    .badge {
      display: inline-block;
      padding: 8px 14px;
      border-radius: 999px;
      background: rgba(34, 197, 94, 0.16);
      border: 1px solid rgba(34, 197, 94, 0.45);
      color: #86efac;
      font-size: 14px;
      margin-bottom: 12px;
    }

    h1 {
      margin: 8px 0;
      font-size: 36px;
      color: #facc15;
    }

    .subtitle {
      color: #cbd5e1;
      font-size: 16px;
    }

    .grid {
      max-width: 1100px;
      margin: auto;
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(260px, 1fr));
      gap: 20px;
    }

    .card {
      background: rgba(15, 23, 42, 0.88);
      border: 1px solid rgba(255,255,255,0.14);
      border-radius: 24px;
      padding: 22px;
      box-shadow: 0 14px 35px rgba(0,0,0,0.28);
      text-align: center;
    }

    .label {
      color: #94a3b8;
      font-size: 14px;
      margin-bottom: 12px;
      letter-spacing: 0.4px;
      text-transform: uppercase;
    }

    .gauge {
      width: 170px;
      height: 170px;
      margin: 0 auto 14px auto;
      border-radius: 50%;
      background:
        conic-gradient(#22c55e calc(var(--p) * 1%), rgba(255,255,255,0.1) 0);
      display: flex;
      align-items: center;
      justify-content: center;
      position: relative;
    }

    .gauge::before {
      content: "";
      position: absolute;
      width: 125px;
      height: 125px;
      border-radius: 50%;
      background: #0f172a;
      border: 1px solid rgba(255,255,255,0.12);
    }

    .gauge-inner {
      position: relative;
      z-index: 2;
    }

    .gauge-value {
      font-size: 30px;
      font-weight: bold;
      color: #e0f2fe;
    }

    .unit {
      font-size: 14px;
      color: #94a3b8;
      margin-top: 4px;
    }

    .mini {
      max-width: 1100px;
      margin: 20px auto 0 auto;
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(220px, 1fr));
      gap: 14px;
    }

    .mini-card {
      background: rgba(15, 23, 42, 0.88);
      border: 1px solid rgba(255,255,255,0.14);
      border-radius: 20px;
      padding: 18px;
    }

    .mini-label {
      color: #94a3b8;
      font-size: 13px;
    }

    .mini-value {
      font-size: 24px;
      font-weight: bold;
      color: #38bdf8;
      margin-top: 6px;
    }


    .weather-section {
      max-width: 1100px;
      margin: 20px auto 0 auto;
      background: rgba(15, 23, 42, 0.88);
      border: 1px solid rgba(255,255,255,0.14);
      border-radius: 24px;
      padding: 22px;
      box-shadow: 0 14px 35px rgba(0,0,0,0.28);
    }

    .weather-top {
      display: flex;
      justify-content: space-between;
      gap: 16px;
      align-items: center;
      flex-wrap: wrap;
      margin-bottom: 18px;
    }

    .weather-title {
      font-size: 20px;
      font-weight: bold;
      color: #facc15;
    }

    .weather-select {
      background: #0f172a;
      color: white;
      border: 1px solid rgba(255,255,255,0.22);
      border-radius: 12px;
      padding: 10px 14px;
      font-size: 15px;
      outline: none;
    }

    .weather-grid {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(170px, 1fr));
      gap: 14px;
    }

    .weather-card {
      background: rgba(2, 6, 23, 0.38);
      border: 1px solid rgba(255,255,255,0.10);
      border-radius: 18px;
      padding: 16px;
    }

    .weather-label {
      color: #94a3b8;
      font-size: 13px;
      margin-bottom: 6px;
    }

    .weather-value {
      color: #38bdf8;
      font-size: 22px;
      font-weight: bold;
    }

    .status {
      text-align: center;
      margin-top: 22px;
      color: #86efac;
      font-weight: bold;
    }
  </style>
</head>

<body>
  <div class="hero">
    <div class="badge">Live IoT Energy Monitor</div>
    <h1>Welcome to My Dual-Axis Solar Tracker</h1>
    <div class="subtitle">
      Real-time voltage, current, power, and servo angle monitoring from an ESP32.
    </div>
  </div>

  <div class="grid">
    <div class="card">
      <div class="label">Solar Voltage</div>
      <div class="gauge" id="voltageGauge" style="--p:0">
        <div class="gauge-inner">
          <div class="gauge-value"><span id="voltage">0.00</span></div>
          <div class="unit">Volts</div>
        </div>
      </div>
    </div>

    <div class="card">
      <div class="label">Solar Current</div>
      <div class="gauge" id="currentGauge" style="--p:0">
        <div class="gauge-inner">
          <div class="gauge-value"><span id="current">0.00</span></div>
          <div class="unit">mA</div>
        </div>
      </div>
    </div>

    <div class="card">
      <div class="label">Solar Power</div>
      <div class="gauge" id="powerGauge" style="--p:0">
        <div class="gauge-inner">
          <div class="gauge-value"><span id="power">0.00</span></div>
          <div class="unit">mW</div>
        </div>
      </div>
    </div>
  </div>

  <div class="mini">
    <div class="mini-card">
      <div class="mini-label">Horizontal Servo Angle</div>
      <div class="mini-value"><span id="servoH">90</span>&deg;</div>
    </div>

    <div class="mini-card">
      <div class="mini-label">Vertical Servo Angle</div>
      <div class="mini-value"><span id="servoV">90</span>&deg;</div>
    </div>

    <div class="mini-card">
      <div class="mini-label">Horizontal Light Difference</div>
      <div class="mini-value"><span id="hDiff">0</span></div>
    </div>

    <div class="mini-card">
      <div class="mini-label">Vertical Light Difference</div>
      <div class="mini-value"><span id="vDiff">0</span></div>
    </div>
  </div>


  <div class="weather-section">
    <div class="weather-top">
      <div>
        <div class="weather-title">Weather Context</div>
        <div class="mini-label">Default city is Toronto. Change the city to compare solar conditions.</div>
      </div>

      <select class="weather-select" id="citySelect">
        <option value="Toronto" selected>Toronto</option>
        <option value="Vaughan">Vaughan</option>
        <option value="Saskatoon">Saskatoon</option>
        <option value="Ottawa">Ottawa</option>
      </select>
    </div>

    <div class="weather-grid">
      <div class="weather-card">
        <div class="weather-label">Selected City</div>
        <div class="weather-value" id="weatherCity">Toronto</div>
      </div>

      <div class="weather-card">
        <div class="weather-label">Temperature</div>
        <div class="weather-value"><span id="weatherTemp">--</span>&deg;C</div>
      </div>

      <div class="weather-card">
        <div class="weather-label">Cloud Cover</div>
        <div class="weather-value"><span id="weatherCloud">--</span>%</div>
      </div>

      <div class="weather-card">
        <div class="weather-label">Solar Condition</div>
        <div class="weather-value" id="solarCondition">Loading...</div>
      </div>
    </div>
  </div>

  <div class="status" id="status">Connecting to ESP32...</div>

  <script>
    function clamp(value, min, max) {
      return Math.max(min, Math.min(max, value));
    }

    async function updateDashboard() {
      try {
        const response = await fetch('/data');
        const data = await response.json();

        document.getElementById('voltage').innerText = data.voltage.toFixed(2);
        document.getElementById('current').innerText = data.current.toFixed(2);
        document.getElementById('power').innerText = data.power.toFixed(2);

        document.getElementById('servoH').innerText = data.servoH;
        document.getElementById('servoV').innerText = data.servoV;
        document.getElementById('hDiff').innerText = data.hDiff;
        document.getElementById('vDiff').innerText = data.vDiff;

        let voltagePercent = clamp((data.voltage / 6.0) * 100, 0, 100);
        let currentPercent = clamp((data.current / 2.0) * 100, 0, 100);
        let powerPercent = clamp((data.power / 5.0) * 100, 0, 100);

        document.getElementById('voltageGauge').style.setProperty('--p', voltagePercent);
        document.getElementById('currentGauge').style.setProperty('--p', currentPercent);
        document.getElementById('powerGauge').style.setProperty('--p', powerPercent);

        document.getElementById('status').innerText = "Live data connected";
      } catch (error) {
        document.getElementById('status').innerText = "Waiting for ESP32 data...";
      }
    }


    const cityCoordinates = {
      Toronto: { lat: 43.6532, lon: -79.3832 },
      Vaughan: { lat: 43.8563, lon: -79.5085 },
      Saskatoon: { lat: 52.1579, lon: -106.6702 },
      Ottawa: { lat: 45.4215, lon: -75.6972 }
    };

    function getSolarCondition(cloudCover, isDay) {
      if (isDay === 0) {
        return "Night / Low";
      }

      if (cloudCover <= 20) {
        return "Excellent";
      } else if (cloudCover <= 50) {
        return "Good";
      } else if (cloudCover <= 80) {
        return "Moderate";
      } else {
        return "Low";
      }
    }

    async function updateWeather(city) {
      try {
        const location = cityCoordinates[city];
        const url = "https://api.open-meteo.com/v1/forecast?latitude=" +
                    location.lat +
                    "&longitude=" +
                    location.lon +
                    "&current=temperature_2m,cloud_cover,weather_code,is_day&timezone=auto";

        const response = await fetch(url);
        const weather = await response.json();

        const temp = weather.current.temperature_2m;
        const cloud = weather.current.cloud_cover;
        const isDay = weather.current.is_day;

        document.getElementById('weatherCity').innerText = city;
        document.getElementById('weatherTemp').innerText = temp.toFixed(1);
        document.getElementById('weatherCloud').innerText = cloud;
        document.getElementById('solarCondition').innerText = getSolarCondition(cloud, isDay);
      } catch (error) {
        document.getElementById('solarCondition').innerText = "Weather unavailable";
      }
    }

    document.getElementById('citySelect').addEventListener('change', function() {
      updateWeather(this.value);
    });

    updateWeather('Toronto');

    setInterval(updateDashboard, 1000);
    updateDashboard();
  </script>
</body>
</html>
)rawliteral";

  server.send(200, "text/html", html);
}

void handleData() {
  String json = "{";
  json += "\"voltage\":" + String(latestVoltage, 3) + ",";
  json += "\"current\":" + String(latestCurrent, 3) + ",";
  json += "\"power\":" + String(latestPower, 3) + ",";
  json += "\"servoH\":" + String(servohori) + ",";
  json += "\"servoV\":" + String(servovert) + ",";
  json += "\"hDiff\":" + String(latestHDiff) + ",";
  json += "\"vDiff\":" + String(latestVDiff);
  json += "}";

  server.send(200, "application/json", json);
}

int readAverage(int pin) {
  long total = 0;

  for (int i = 0; i < 5; i++) {
    total += analogRead(pin);
    delay(1);
  }

  return total / 5;
}

int convertLight(int rawValue) {
  if (invertLDR) {
    return 4095 - rawValue;
  } else {
    return rawValue;
  }
}

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);

  // INA219 setup on ESP32 I2C pins
  Wire.begin(21, 22); // SDA = GPIO21, SCL = GPIO22

  if (ina219.begin()) {
    ina219OK = true;
    Serial.println("INA219 found.");
  } else {
    ina219OK = false;
    Serial.println("INA219 not found. Tracker will still move.");
  }

  // Website setup
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");

  int wifiTries = 0;
  while (WiFi.status() != WL_CONNECTED && wifiTries < 20) {
    delay(500);
    Serial.print(".");
    wifiTries++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.println("WiFi connected.");
    Serial.print("Open this IP address: ");
    Serial.println(WiFi.localIP());


    Serial.println("You have 15 seconds to copy the IP address...");
    delay(15000);


    server.on("/", handleRoot);
    server.on("/data", handleData);
    server.begin();
  } else {
    Serial.println();
    Serial.println("WiFi not connected. Tracker will still move.");
  }

  horizontal.attach(horizontalServoPin, 500, 2400);
  vertical.attach(verticalServoPin, 500, 2400);

  horizontal.write(servohori);
  vertical.write(servovert);

  Serial.println("Combined solar tracker starting.");
  Serial.println("Keep all 4 LDRs under similar light for calibration.");

  delay(2500);

  baseTL = convertLight(readAverage(ldrTL));
  baseTR = convertLight(readAverage(ldrTR));
  baseBL = convertLight(readAverage(ldrBL));
  baseBRValue = convertLight(readAverage(ldrBRPin));

  Serial.println("Calibration complete.");
  delay(1000);
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    server.handleClient();
  }

  // Read LDRs once
  int lightTL = convertLight(readAverage(ldrTL));
  int lightTR = convertLight(readAverage(ldrTR));
  int lightBL = convertLight(readAverage(ldrBL));
  int lightBRValue = convertLight(readAverage(ldrBRPin));

  // Apply calibration
  int topLeft = lightTL - baseTL;
  int topRight = lightTR - baseTR;
  int bottomLeft = lightBL - baseBL;
  int bottomRight = lightBRValue - baseBRValue;

  // Horizontal averages
  int avgLeft  = (topLeft + bottomLeft) / 2;
  int avgRight = (topRight + bottomRight) / 2;
  int dhoriz = avgLeft - avgRight;

  // Vertical averages
  int avgTop = (topLeft + topRight) / 2;
  int avgBottom = (bottomLeft + bottomRight) / 2;
  int dvert = avgTop - avgBottom;

  latestHDiff = dhoriz;
  latestVDiff = dvert;

  // -------------------------
  // Bottom servo logic
  // -------------------------
  int directionH = 0;

  if (abs(dhoriz) > tolerance) {
    if (avgLeft > avgRight) {
      directionH = -1;
    } else {
      directionH = 1;
    }
  } else {
    directionH = 0;
  }

  if (directionH == lastDirectionH && directionH != 0) {
    stableCountH++;
  } else {
    stableCountH = 0;
  }

  lastDirectionH = directionH;

  if (stableCountH >= stableNeeded) {
    if (directionH == -1) {
      if (!reverseHorizontal) servohori -= stepSize;
      else servohori += stepSize;
    }

    if (directionH == 1) {
      if (!reverseHorizontal) servohori += stepSize;
      else servohori -= stepSize;
    }

    servohori = constrain(servohori, servohoriLimitLow, servohoriLimitHigh);
    horizontal.write(servohori);

    stableCountH = 0;
  }

  // -------------------------
  // Top servo logic
  // -------------------------
  int directionV = 0;

  if (abs(dvert) > tolerance) {
    if (avgTop > avgBottom) {
      directionV = 1;
    } else {
      directionV = -1;
    }
  } else {
    directionV = 0;
  }

  if (directionV == lastDirectionV && directionV != 0) {
    stableCountV++;
  } else {
    stableCountV = 0;
  }

  lastDirectionV = directionV;

  if (stableCountV >= stableNeeded) {
    if (directionV == 1) {
      if (!reverseVertical) servovert += stepSize;
      else servovert -= stepSize;
    }

    if (directionV == -1) {
      if (!reverseVertical) servovert -= stepSize;
      else servovert += stepSize;
    }

    servovert = constrain(servovert, servovertLimitLow, servovertLimitHigh);
    vertical.write(servovert);

    stableCountV = 0;
  }

  // INA219 readings
  float busVoltage = 0;
  float current_mA = 0;
  float power_mW = 0;

  if (ina219OK) {
    busVoltage = ina219.getBusVoltage_V();
    current_mA = ina219.getCurrent_mA();
    power_mW = busVoltage * current_mA;
  }

  latestVoltage = busVoltage;
  latestCurrent = current_mA;
  latestPower = power_mW;

  // Serial monitor
  Serial.print("Left: "); Serial.print(avgLeft);
  Serial.print(" | Right: "); Serial.print(avgRight);
  Serial.print(" | H Diff: "); Serial.print(dhoriz);
  Serial.print(" | Servo H: "); Serial.print(servohori);

  Serial.print(" || Top: "); Serial.print(avgTop);
  Serial.print(" | Bottom: "); Serial.print(avgBottom);
  Serial.print(" | V Diff: "); Serial.print(dvert);
  Serial.print(" | Servo V: "); Serial.print(servovert);

  if (ina219OK) {
    Serial.print(" || Voltage: "); Serial.print(busVoltage);
    Serial.print(" V | Current: "); Serial.print(current_mA);
    Serial.print(" mA | Power: "); Serial.print(power_mW);
    Serial.println(" mW");
  } else {
    Serial.println(" || INA219 not found");
  }

  delay(dtime);
}