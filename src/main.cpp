#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <ArduinoJson.h>
#include <vector>
#include <cmath>

// ============================================================================
// CONFIGURACIÓN GLOBAL DE PINES Y PWM
// ============================================================================
#define PIR_PIN 13
#define RELAY_PIN 26
#define LIGHT_SENSOR_PIN 36 // Ahora es una entrada DIGITAL

// Pines de LED y configuración del controlador PWM por hardware (LEDC)
constexpr int LED_PINS[] = {15, 16, 17, 18, 19};
constexpr int NUM_LEDS = sizeof(LED_PINS) / sizeof(LED_PINS[0]);
constexpr int PWM_CHANNEL_START = 0;
constexpr int PWM_FREQ = 5000;
constexpr int PWM_RESOLUTION = 8;

// ============================================================================
// CONFIGURACIÓN WIFI
// ============================================================================
constexpr const char *SSID = "Javier";
constexpr const char *PASSWORD = "12345678";

AsyncWebServer server(80);

// ============================================================================
// ESTRUCTURAS DE DATOS
// ============================================================================
struct SystemConfig
{
    int autoOffDelaySec = 60;       // Segundos de inactividad para apagar
    float fixedBrightness = 80.0f;  // Brillo fijo cuando las luces se encienden (en %)
    float slewRateLimit = 25.0f;    // Rampa más rápida para ON/OFF
    bool autoMode = true;
    String presetName = "Digital";
};

struct SystemState
{
    bool pirDetected = false;
    bool isDark = false;             // Nuevo estado: true si el LDR dice que está oscuro
    bool lightsOn = false;
    float targetBrightnessPercent = 0.0f;
    float currentBrightnessPercent = 0.0f;
    unsigned long lastMotionTime = 0;
    unsigned long systemStartTime = 0;
    unsigned long totalOnTimeSec = 0;
    unsigned long lastEnergyUpdate = 0;
    float totalEnergyConsumedKWh = 0.0f;
    float currentPowerConsumptionW = 0.0f;
};

struct HistoricalData
{
    std::vector<float> brightness;
    std::vector<bool> isDarkHistory; // Guardamos el estado digital
    std::vector<float> power;
    std::vector<unsigned long> timestamps;
    static constexpr int MAX_POINTS = 100;

    void addPoint(float b, bool isDark, float p, unsigned long t)
    {
        if (brightness.size() >= MAX_POINTS)
        {
            brightness.erase(brightness.begin());
            isDarkHistory.erase(isDarkHistory.begin());
            power.erase(power.begin());
            timestamps.erase(timestamps.begin());
        }
        brightness.push_back(b);
        isDarkHistory.push_back(isDark);
        power.push_back(p);
        timestamps.push_back(t);
    }
};

SystemConfig config;
SystemState state;
HistoricalData history;

// ============================================================================
// CONSTANTES
// ============================================================================
constexpr float POWER_MAX_WATTS = 25.0f;
constexpr float ELECTRICITY_RATE_KWH = 0.092f;

// ============================================================================
// DECLARACIÓN DE FUNCIONES
// ============================================================================
void setupWiFi();
void setupWebServer();
void readSensors();
void handleSystemLogic();
void updateActuators(float deltaTime);
void updateMetrics(float deltaTime);
void setBrightness(float percentage);
void turnOnLights();
void turnOffLights();
String getSystemStatusJson();
String getHistoricalDataJson();

// ============================================================================
// SETUP
// ============================================================================
void setup()
{
    Serial.begin(115200);
    delay(500);

    Serial.println(F("\n=== SISTEMA DE ILUMINACION INCLUSIVA (MODO DIGITAL) V3.2 ==="));

    pinMode(PIR_PIN, INPUT_PULLDOWN);
    pinMode(LIGHT_SENSOR_PIN, INPUT_PULLUP); 
    pinMode(RELAY_PIN, OUTPUT);

    for (int i = 0; i < NUM_LEDS; ++i)
    {
        ledcSetup(PWM_CHANNEL_START + i, PWM_FREQ, PWM_RESOLUTION);
        ledcAttachPin(LED_PINS[i], PWM_CHANNEL_START + i);
    }

    digitalWrite(RELAY_PIN, LOW);
    setBrightness(0);

    state.systemStartTime = millis();
    state.lastMotionTime = state.systemStartTime;

    setupWiFi();
    setupWebServer();

    Serial.println(F("\n[INFO] Sensor PIR necesita ~60s para calibrarse..."));
    Serial.println(F("[INFO] Asegúrese de ajustar el potenciómetro del sensor LDR."));
    Serial.println(F("[INFO] Sistema listo."));
}

// ============================================================================
// LOOP PRINCIPAL
// ============================================================================
void loop()
{
    static unsigned long lastUpdateTime = 0;
    unsigned long now = millis();

    if (now - lastUpdateTime >= 50) 
    {
        float deltaTime = (now - lastUpdateTime) / 1000.0f;
        
        readSensors();

        if (config.autoMode) {
            handleSystemLogic();
        }

        updateActuators(deltaTime);
        updateMetrics(deltaTime);

        lastUpdateTime = now;
    }
}

// ============================================================================
// LÓGICA DE CONTROL (ADAPTADA A DIGITAL)
// ============================================================================
void readSensors() {
    state.isDark = (digitalRead(LIGHT_SENSOR_PIN) == HIGH);
    
    state.pirDetected = digitalRead(PIR_PIN);
    if (state.pirDetected) {
        state.lastMotionTime = millis();
    }
}

void handleSystemLogic() {
    unsigned long timeSinceLastMotionSec = (millis() - state.lastMotionTime) / 1000UL;

    if (state.pirDetected && state.isDark && !state.lightsOn) {
        turnOnLights();
    } 
    else if (state.lightsOn && (timeSinceLastMotionSec >= (unsigned long)config.autoOffDelaySec || !state.isDark)) {
        turnOffLights();
    }

    if (state.lightsOn) {
        state.targetBrightnessPercent = config.fixedBrightness;
    } else {
        state.targetBrightnessPercent = 0.0f;
    }
}

// ============================================================================
// CONTROL DE ACTUADORES
// ============================================================================
void updateActuators(float deltaTime) {
    float diff = state.targetBrightnessPercent - state.currentBrightnessPercent;
    float maxChange = config.slewRateLimit * deltaTime;

    if (abs(diff) < maxChange) {
        state.currentBrightnessPercent = state.targetBrightnessPercent;
    } else {
        state.currentBrightnessPercent += (diff > 0) ? maxChange : -maxChange;
    }

    setBrightness(state.currentBrightnessPercent);
}

void setBrightness(float percentage) {
    int pwmValue = map(percentage, 0, 100, 0, 255);
    pwmValue = constrain(pwmValue, 0, 255);
    
    for (int i = 0; i < NUM_LEDS; ++i) {
        ledcWrite(PWM_CHANNEL_START + i, pwmValue);
    }
}

void turnOnLights() {
    if (!state.lightsOn) {
        Serial.println(F("[CONTROL] Encendiendo luces (Relé ON)"));
        digitalWrite(RELAY_PIN, HIGH);
        state.lightsOn = true;
    }
}

void turnOffLights() {
    if (state.lightsOn) {
        Serial.println(F("[CONTROL] Apagando luces (Relé OFF)"));
        digitalWrite(RELAY_PIN, LOW);
        state.lightsOn = false;
    }
}

// ============================================================================
// MÉTRICAS Y TELEMETRÍA
// ============================================================================
void updateMetrics(float deltaTime) {
    static unsigned long lastHistorySave = 0;

    if (millis() - state.lastEnergyUpdate >= 1000) {
        if (state.lightsOn) {
            state.totalOnTimeSec++;
            state.currentPowerConsumptionW = POWER_MAX_WATTS * (state.currentBrightnessPercent / 100.0f);
            state.totalEnergyConsumedKWh += (state.currentPowerConsumptionW * 1.0f) / 3600000.0f;
        } else {
            state.currentPowerConsumptionW = 0.0f;
        }
        state.lastEnergyUpdate = millis();
    }
    
    if (millis() - lastHistorySave >= 5000) {
        history.addPoint(state.currentBrightnessPercent, state.isDark, state.currentPowerConsumptionW, millis() / 1000);
        lastHistorySave = millis();
    }
}

String getSystemStatusJson() {
    DynamicJsonDocument doc(2048);
    unsigned long uptime = (millis() - state.systemStartTime) / 1000UL;
    unsigned long timeSinceMotion = (millis() - state.lastMotionTime) / 1000UL;

    doc["lightsOn"] = state.lightsOn;
    doc["pirDetected"] = state.pirDetected;
    doc["isDark"] = state.isDark;
    doc["autoMode"] = config.autoMode;
    doc["currentBrightness"] = state.currentBrightnessPercent;
    doc["targetBrightness"] = state.targetBrightnessPercent;
    doc["timeSinceLastMotion"] = timeSinceMotion;
    doc["totalOnTime"] = state.totalOnTimeSec;
    doc["totalEnergy"] = state.totalEnergyConsumedKWh;
    doc["currentPower"] = state.currentPowerConsumptionW;
    doc["uptime"] = uptime;
    doc["fixedBrightness"] = config.fixedBrightness;
    doc["autoOffDelay"] = config.autoOffDelaySec;
    doc["presetName"] = config.presetName;
    
    String response;
    serializeJson(doc, response);
    return response;
}

String getHistoricalDataJson() {
    DynamicJsonDocument doc(4096);
    JsonArray b = doc.createNestedArray("brightness");
    JsonArray l = doc.createNestedArray("isDark");
    JsonArray p = doc.createNestedArray("power");
    JsonArray t = doc.createNestedArray("timestamps");

    for (size_t i = 0; i < history.brightness.size(); ++i) {
        b.add(history.brightness[i]);
        // ========================================================
        // AQUI ESTÁ LA CORRECCIÓN
        // ========================================================
        l.add(static_cast<bool>(history.isDarkHistory[i]));
        // ========================================================
        p.add(history.power[i]);
        t.add(history.timestamps[i]);
    }

    String response;
    serializeJson(doc, response);
    return response;
}

// ============================================================================
// WIFI Y SERVIDOR WEB
// ============================================================================
void setupWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(SSID, PASSWORD);
    Serial.print(F("Conectando a WiFi"));
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - start < 10000)) {
        delay(500); Serial.print(F("."));
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println(F("\n\n¡WiFi conectado exitosamente!"));
        Serial.print(F("Dirección IP: http://"));
        Serial.println(WiFi.localIP());
    } else {
        Serial.println(F("\nError: WiFi no conectado. Creando AP de respaldo."));
        WiFi.softAP("Sistema-Iluminacion-Aula", "");
        Serial.print("AP IP: http://");
        Serial.println(WiFi.softAPIP());
    }
}

void setupWebServer() {
    server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *req){ req->send(200, "application/json", getSystemStatusJson()); });
    server.on("/api/history", HTTP_GET, [](AsyncWebServerRequest *req){ req->send(200, "application/json", getHistoricalDataJson()); });
    server.on("/api/control", HTTP_POST, [](AsyncWebServerRequest *req) {}, nullptr, [](AsyncWebServerRequest *req, uint8_t *data, size_t len, size_t, size_t) {
        DynamicJsonDocument doc(512);
        if (deserializeJson(doc, data, len) == DeserializationError::Ok) {
            String action = doc["action"].as<String>();
            if (action == "on") { config.autoMode = false; turnOnLights(); } 
            else if (action == "off") { config.autoMode = false; turnOffLights(); } 
            else if (action == "auto") { config.autoMode = true; } 
            else if (action == "manual") { config.autoMode = false; }
        }
        req->send(200, "application/json", "{\"status\":\"ok\"}");
    });
     server.on("/", HTTP_GET, [](AsyncWebServerRequest *req){
        String html = R"rawliteral(
<!DOCTYPE html>
<html lang="es">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Sistema de Iluminación Inclusiva - Dashboard Técnico v3.0</title>
    <script src="https://cdn.jsdelivr.net/npm/chart.js@4.4.0/dist/chart.umd.min.js"></script>
    <link href="https://fonts.googleapis.com/css2?family=Inter:wght@300;400;500;600;700&display=swap" rel="stylesheet">
    <style>
        :root {
            --bg-primary: #0f172a;
            --bg-secondary: #1e293b;
            --bg-card: #334155;
            --text-primary: #f1f5f9;
            --text-secondary: #cbd5e1;
            --accent-primary: #3b82f6;
            --accent-success: #10b981;
            --accent-warning: #f59e0b;
            --accent-danger: #ef4444;
            --border: #475569;
            --shadow: 0 10px 25px rgba(0,0,0,0.5);
            --radius: 12px;
        }

        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }

        body {
            font-family: 'Inter', sans-serif;
            background: linear-gradient(135deg, var(--bg-primary) 0%, var(--bg-secondary) 100%);
            min-height: 100vh;
            color: var(--text-primary);
            line-height: 1.6;
        }

        .container {
            max-width: 1400px;
            margin: 0 auto;
            padding: 20px;
        }

        .header {
            background: linear-gradient(135deg, var(--bg-card), #475569);
            border-radius: var(--radius);
            padding: 30px;
            margin-bottom: 30px;
            box-shadow: var(--shadow);
            border: 1px solid var(--border);
        }

        .header h1 {
            font-size: 2.2em;
            font-weight: 700;
            background: linear-gradient(135deg, var(--accent-primary), #60a5fa);
            -webkit-background-clip: text;
            -webkit-text-fill-color: transparent;
            margin-bottom: 8px;
        }

        .header .subtitle {
            font-size: 1em;
            color: var(--text-secondary);
            font-weight: 400;
        }

        .status-badge {
            display: inline-flex;
            align-items: center;
            gap: 8px;
            padding: 8px 16px;
            border-radius: 20px;
            font-weight: 600;
            font-size: 0.95em;
            backdrop-filter: blur(10px);
        }

        .status-active {
            background: rgba(16, 185, 129, 0.2);
            border: 1px solid var(--accent-success);
            color: var(--accent-success);
        }

        .status-inactive {
            background: rgba(107, 114, 128, 0.2);
            border: 1px solid var(--text-secondary);
            color: var(--text-secondary);
        }

        .grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(320px, 1fr));
            gap: 20px;
            margin-bottom: 20px;
        }

        .card {
            background: var(--bg-card);
            border-radius: var(--radius);
            padding: 24px;
            box-shadow: var(--shadow);
            border: 1px solid var(--border);
            transition: all 0.3s cubic-bezier(0.4, 0, 0.2, 1);
            position: relative;
            overflow: hidden;
        }

        .card::before {
            content: '';
            position: absolute;
            top: 0;
            left: 0;
            right: 0;
            height: 3px;
            background: linear-gradient(90deg, var(--accent-primary), transparent);
        }

        .card:hover {
            transform: translateY(-4px);
            box-shadow: 0 20px 40px rgba(0,0,0,0.6);
        }

        .card.primary::before { background: linear-gradient(90deg, var(--accent-primary), transparent); }
        .card.success::before { background: linear-gradient(90deg, var(--accent-success), transparent); }
        .card.warning::before { background: linear-gradient(90deg, var(--accent-warning), transparent); }
        .card.info::before { background: linear-gradient(90deg, #06b6d4, transparent); }

        .card-header {
            display: flex;
            align-items: center;
            justify-content: space-between;
            margin-bottom: 20px;
            padding-bottom: 12px;
            border-bottom: 1px solid var(--border);
        }

        .card-title {
            font-size: 1.2em;
            font-weight: 600;
            color: var(--text-primary);
            display: flex;
            align-items: center;
            gap: 8px;
        }

        .card-icon {
            width: 32px;
            height: 32px;
            border-radius: 8px;
            display: flex;
            align-items: center;
            justify-content: center;
            font-size: 1.2em;
            background: rgba(59, 130, 246, 0.2);
            color: var(--accent-primary);
        }

        .metric-row {
            display: flex;
            justify-content: space-between;
            align-items: center;
            padding: 12px 0;
            border-bottom: 1px solid rgba(71, 85, 105, 0.3);
        }

        .metric-row:last-child { border-bottom: none; }

        .metric-label {
            font-weight: 500;
            color: var(--text-secondary);
            font-size: 0.95em;
        }

        .metric-value {
            font-weight: 600;
            font-size: 1.1em;
            color: var(--text-primary);
        }

        .metric-unit {
            font-size: 0.85em;
            color: var(--text-secondary);
            margin-left: 4px;
        }

        .progress-container {
            margin: 16px 0;
        }

        .progress-label {
            display: flex;
            justify-content: space-between;
            margin-bottom: 8px;
            font-size: 0.9em;
            color: var(--text-secondary);
        }

        .progress-bar {
            width: 100%;
            height: 8px;
            background: var(--border);
            border-radius: 4px;
            overflow: hidden;
        }

        .progress-fill {
            height: 100%;
            transition: width 0.4s ease;
            position: relative;
        }

        .progress-fill::after {
            content: '';
            position: absolute;
            top: 0;
            left: 0;
            right: 0;
            bottom: 0;
            background: linear-gradient(90deg, transparent, rgba(255,255,255,0.2), transparent);
            animation: shimmer 1.5s infinite;
        }

        @keyframes shimmer {
            0% { transform: translateX(-100%); }
            100% { transform: translateX(100%); }
        }

        .progress-primary { background: linear-gradient(90deg, var(--accent-primary), #60a5fa); }
        .progress-success { background: linear-gradient(90deg, var(--accent-success), #34d399); }
        .progress-warning { background: linear-gradient(90deg, var(--accent-warning), #fbbf24); }

        .btn-group {
            display: flex;
            gap: 12px;
            margin-top: 16px;
        }

        .btn {
            flex: 1;
            padding: 12px 20px;
            border: none;
            border-radius: 8px;
            font-size: 0.95em;
            font-weight: 600;
            cursor: pointer;
            transition: all 0.3s cubic-bezier(0.4, 0, 0.2, 1);
            text-transform: uppercase;
            letter-spacing: 0.5px;
            position: relative;
            overflow: hidden;
        }

        .btn::before {
            content: '';
            position: absolute;
            top: 0;
            left: -100%;
            width: 100%;
            height: 100%;
            background: linear-gradient(90deg, transparent, rgba(255,255,255,0.2), transparent);
            transition: left 0.5s;
        }

        .btn:hover::before { left: 100%; }

        .btn:hover { transform: translateY(-1px); }

        .btn-primary {
            background: linear-gradient(135deg, var(--accent-primary), #2563eb);
            color: white;
            box-shadow: 0 4px 12px rgba(59, 130, 246, 0.3);
        }

        .btn-success {
            background: linear-gradient(135deg, var(--accent-success), #059669);
            color: white;
            box-shadow: 0 4px 12px rgba(16, 185, 129, 0.3);
        }

        .btn-danger {
            background: linear-gradient(135deg, var(--accent-danger), #dc2626);
            color: white;
            box-shadow: 0 4px 12px rgba(239, 68, 68, 0.3);
        }

        .btn-warning {
            background: linear-gradient(135deg, var(--accent-warning), #d97706);
            color: white;
            box-shadow: 0 4px 12px rgba(245, 158, 11, 0.3);
        }

        .preset-grid {
            display: grid;
            grid-template-columns: repeat(2, 1fr);
            gap: 10px;
            margin-top: 12px;
        }

        .preset-btn {
            padding: 12px;
            border: 1px solid var(--border);
            border-radius: 8px;
            background: transparent;
            cursor: pointer;
            transition: all 0.3s;
            text-align: left;
            color: var(--text-secondary);
        }

        .preset-btn:hover {
            border-color: var(--accent-primary);
            background: rgba(59, 130, 246, 0.1);
            color: var(--text-primary);
        }

        .preset-btn.active {
            border-color: var(--accent-primary);
            background: rgba(59, 130, 246, 0.2);
            color: var(--text-primary);
        }

        .preset-name {
            font-weight: 600;
            margin-bottom: 4px;
        }

        .preset-desc {
            font-size: 0.8em;
            opacity: 0.8;
        }

        .chart-container {
            position: relative;
            height: 280px;
            margin-top: 16px;
        }

        .wide-card { grid-column: span 2; }

        .indicator {
            width: 10px;
            height: 10px;
            border-radius: 50%;
            display: inline-block;
            margin-right: 6px;
            animation: pulse 1.5s infinite;
        }

        @keyframes pulse {
            0%, 100% { opacity: 1; }
            50% { opacity: 0.4; }
        }

        .indicator-on { background: var(--accent-success); }
        .indicator-off { background: var(--text-secondary); }
        .indicator-motion { background: var(--accent-warning); }

        .config-section {
            margin-top: 20px;
            padding-top: 20px;
            border-top: 1px solid var(--border);
        }

        .config-input {
            display: flex;
            align-items: center;
            gap: 12px;
            margin: 10px 0;
        }

        .config-label {
            flex: 1;
            font-weight: 500;
            color: var(--text-secondary);
            font-size: 0.95em;
        }

        input[type="number"], input[type="range"] {
            padding: 8px 12px;
            border: 1px solid var(--border);
            border-radius: 6px;
            background: var(--bg-secondary);
            color: var(--text-primary);
            font-size: 0.95em;
            transition: border-color 0.3s;
        }

        input[type="number"]:focus, input[type="range"]:focus {
            outline: none;
            border-color: var(--accent-primary);
        }

        input[type="range"] {
            flex: 2;
            height: 6px;
            background: var(--border);
        }

        input[type="range"]::-webkit-slider-thumb {
            appearance: none;
            width: 16px;
            height: 16px;
            border-radius: 50%;
            background: var(--accent-primary);
            cursor: pointer;
        }

        .stats-grid {
            display: grid;
            grid-template-columns: repeat(2, 1fr);
            gap: 12px;
            margin-top: 16px;
        }

        .stat-box {
            background: linear-gradient(135deg, rgba(59, 130, 246, 0.1), rgba(59, 130, 246, 0.05));
            padding: 16px;
            border-radius: 8px;
            text-align: center;
            border: 1px solid rgba(59, 130, 246, 0.2);
        }

        .stat-value {
            font-size: 2em;
            font-weight: 700;
            background: linear-gradient(135deg, var(--accent-primary), #60a5fa);
            -webkit-background-clip: text;
            -webkit-text-fill-color: transparent;
            margin-bottom: 4px;
        }

        .stat-label {
            font-size: 0.8em;
            color: var(--text-secondary);
            text-transform: uppercase;
            letter-spacing: 0.5px;
            font-weight: 500;
        }

        .alert {
            padding: 16px 20px;
            border-radius: 8px;
            margin: 16px 0;
            display: flex;
            align-items: center;
            gap: 12px;
            font-weight: 500;
            backdrop-filter: blur(10px);
        }

        .alert-success {
            background: rgba(16, 185, 129, 0.1);
            border: 1px solid rgba(16, 185, 129, 0.3);
            color: var(--accent-success);
        }

        .alert-warning {
            background: rgba(245, 158, 11, 0.1);
            border: 1px solid rgba(245, 158, 11, 0.3);
            color: var(--accent-warning);
        }

        @media (max-width: 1200px) {
            .wide-card { grid-column: span 1; }
        }

        @media (max-width: 768px) {
            .grid { grid-template-columns: 1fr; }
            .btn-group, .preset-grid, .stats-grid { grid-template-columns: 1fr; flex-direction: column; }
            .header h1 { font-size: 1.8em; }
        }

        .gauge-container {
            position: relative;
            width: 100%;
            height: 120px;
            margin: 10px 0;
        }

        .gauge {
            width: 100%;
            height: 100%;
        }

        .gauge-label {
            position: absolute;
            bottom: -20px;
            left: 50%;
            transform: translateX(-50%);
            font-size: 0.8em;
            color: var(--text-secondary);
            text-align: center;
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>Iluminación Inclusiva - Control Técnico</h1>
            <p class="subtitle">Dashboard Avanzado | Modelo Dinámico Optimizado | v3.0</p>
            <div style="margin-top: 15px;">
                <span class="status-badge" id="systemStatus">
                    <span class="indicator" id="systemIndicator"></span>
                    <span id="systemStatusText">Inicializando...</span>
                </span>
            </div>
        </div>

        <div class="grid">
            <div class="card primary">
                <div class="card-header">
                    <div class="card-title">
                        <div class="card-icon">⚡</div>
                        Estado Principal
                    </div>
                </div>
                <div class="metric-row">
                    <span class="metric-label">Luces</span>
                    <span class="metric-value">
                        <span class="indicator" id="ledIndicator"></span>
                        <span id="ledStatus">Off</span>
                    </span>
                </div>
                <div class="metric-row">
                    <span class="metric-label">PIR</span>
                    <span class="metric-value">
                        <span class="indicator" id="pirIndicator"></span>
                        <span id="pirStatus">Idle</span>
                    </span>
                </div>
                <div class="metric-row">
                    <span class="metric-label">Modo</span>
                    <span class="metric-value" id="controlMode">Auto</span>
                </div>
                <div class="metric-row">
                    <span class="metric-label">Uptime</span>
                    <span class="metric-value" id="uptime">00:00:00</span>
                </div>
                <div class="progress-container">
                    <div class="progress-label">
                        <span>Brillo PWM</span>
                        <span><strong id="brightness">0%</strong></span>
                    </div>
                    <div class="progress-bar">
                        <div class="progress-fill progress-primary" id="brightnessBar" style="width: 0%"></div>
                    </div>
                </div>
            </div>

            <div class="card info">
                <div class="card-header">
                    <div class="card-title">
                        <div class="card-icon">📡</div>
                        Sensores
                    </div>
                </div>
                <div class="metric-row">
                    <span class="metric-label">Iluminancia</span>
                    <span class="metric-value">
                        <span id="lightLevel">0</span>
                        <span class="metric-unit">lux</span>
                    </span>
                </div>
                <div class="progress-container">
                    <div class="progress-label">
                        <span>ADC Raw</span>
                        <span id="lightPercent">0%</span>
                    </div>
                    <div class="progress-bar">
                        <div class="progress-fill progress-info" id="lightBar" style="width: 0%"></div>
                    </div>
                </div>
                <div class="metric-row">
                    <span class="metric-label">Motion Timeout</span>
                    <span class="metric-value" id="lastMotion">N/A</span>
                </div>
                <div class="metric-row">
                    <span class="metric-label">Setpoint</span>
                    <span class="metric-value">
                        <span id="setpointDisplay">400</span>
                        <span class="metric-unit">lux</span>
                    </span>
                </div>
            </div>

            <div class="card success">
                <div class="card-header">
                    <div class="card-title">
                        <div class="card-icon">🔋</div>
                        Energía
                    </div>
                </div>
                <div class="metric-row">
                    <span class="metric-label">Potencia</span>
                    <span class="metric-value">
                        <span id="currentPower">0.0</span>
                        <span class="metric-unit">W</span>
                    </span>
                </div>
                <div class="metric-row">
                    <span class="metric-label">On-Time</span>
                    <span class="metric-value" id="onTime">00:00:00</span>
                </div>
                <div class="metric-row">
                    <span class="metric-label">Energía</span>
                    <span class="metric-value">
                        <span id="totalEnergy">0.000</span>
                        <span class="metric-unit">kWh</span>
                    </span>
                </div>
                <div class="metric-row">
                    <span class="metric-label">Costo</span>
                    <span class="metric-value" id="totalCost">$0.00</span>
                </div>
                <div class="metric-row">
                    <span class="metric-label">Ahorro</span>
                    <span class="metric-value" style="color: var(--accent-success);" id="savings">$0.00</span>
                </div>
            </div>

            <div class="card warning">
                <div class="card-header">
                    <div class="card-title">
                        <div class="card-icon">👁️</div>
                        Confort
                    </div>
                </div>
                <div class="metric-row">
                    <span class="metric-label">Comfort Time</span>
                    <span class="metric-value">
                        <span id="comfortPercent">0</span>
                        <span class="metric-unit">%</span>
                    </span>
                </div>
                <div class="progress-container">
                    <div class="progress-label">
                        <span>Comfort Band</span>
                    </div>
                    <div class="progress-bar">
                        <div class="progress-fill progress-success" id="comfortBar" style="width: 0%"></div>
                    </div>
                </div>
                <div class="metric-row">
                    <span class="metric-label">Glare Events</span>
                    <span class="metric-value" id="glareEvents">0</span>
                </div>
                <div class="metric-row">
                    <span class="metric-label">ROI</span>
                    <span class="metric-value">
                        <span id="roi">-</span>
                        <span class="metric-unit">mos</span>
                    </span>
                </div>
            </div>

            <div class="card primary">
                <div class="card-header">
                    <div class="card-title">
                        <div class="card-icon">⚙️</div>
                        Controles
                    </div>
                </div>
                <div class="btn-group">
                    <button class="btn btn-success" onclick="controlSystem('on')">ON</button>
                    <button class="btn btn-danger" onclick="controlSystem('off')">OFF</button>
                </div>
                <div class="btn-group">
                    <button class="btn btn-primary" onclick="controlSystem('auto')">AUTO</button>
                    <button class="btn btn-warning" onclick="controlSystem('manual')">MANUAL</button>
                </div>
            </div>

            <div class="card info">
                <div class="card-header">
                    <div class="card-title">
                        <div class="card-icon">🎛️</div>
                        Presets
                    </div>
                </div>
                <p style="color: var(--text-secondary); margin-bottom: 12px; font-size: 0.9em;">
                    Active: <strong id="activePreset">Estandar</strong>
                </p>
                <div class="preset-grid">
                    <div class="preset-btn active" onclick="applyPreset(0)">
                        <div class="preset-name">Estandar</div>
                        <div class="preset-desc">400 lux</div>
                    </div>
                    <div class="preset-btn" onclick="applyPreset(1)">
                        <div class="preset-name">Lectura</div>
                        <div class="preset-desc">500 lux</div>
                    </div>
                    <div class="preset-btn" onclick="applyPreset(2)">
                        <div class="preset-name">Natural</div>
                        <div class="preset-desc">350 lux</div>
                    </div>
                    <div class="preset-btn" onclick="applyPreset(3)">
                        <div class="preset-name">Baja Vis.</div>
                        <div class="preset-desc">450 lux</div>
                    </div>
                </div>
            </div>

            <div class="card wide-card">
                <div class="card-header">
                    <div class="card-title">
                        <div class="card-icon">📊</div>
                        Telemetría
                    </div>
                </div>
                <div class="chart-container">
                    <canvas id="historyChart"></canvas>
                </div>
            </div>

            <div class="card wide-card success">
                <div class="card-header">
                    <div class="card-title">
                        <div class="card-icon">📈</div>
                        Métricas
                    </div>
                </div>
                <div class="stats-grid">
                    <div class="stat-box">
                        <div class="stat-value">60</div>
                        <div class="stat-label">Eficiencia %</div>
                    </div>
                    <div class="stat-box">
                        <div class="stat-value" id="roiMonths">4.6</div>
                        <div class="stat-label">ROI mos</div>
                    </div>
                    <div class="stat-box">
                        <div class="stat-value">$39.90</div>
                        <div class="stat-label">Costo USD</div>
                    </div>
                    <div class="stat-box">
                        <div class="stat-value" id="annualSavings">104</div>
                        <div class="stat-label">Ahorro Anual</div>
                    </div>
                </div>
                <div class="alert alert-success">
                    <span style="font-size: 1.5em;">✓</span>
                    <div>Validado CIE | Inclusivo Educativo</div>
                </div>
            </div>
        </div>
    </div>

    <script>
        let systemData = {};
        let historyChart = null;
        let updateInterval = null;

        document.addEventListener('DOMContentLoaded', () => {
            initChart();
            startUpdates();
        });

        function startUpdates() {
            updateSystemData();
            updateInterval = setInterval(updateSystemData, 1000);
        }

        async function updateSystemData() {
            try {
                const res = await fetch('/api/status');
                if (res.ok) {
                    systemData = await res.json();
                    updateUI();
                }
            } catch (e) {
                console.error('Fetch error:', e);
            }
        }

        function updateUI() {
            // System status
            const sysStatus = document.getElementById('systemStatus');
            const sysInd = document.getElementById('systemIndicator');
            const sysText = document.getElementById('systemStatusText');
            if (systemData.lightsOn) {
                sysStatus.className = 'status-badge status-active';
                sysInd.className = 'indicator indicator-on';
                sysText.textContent = 'ACTIVE';
            } else {
                sysStatus.className = 'status-badge status-inactive';
                sysInd.className = 'indicator indicator-off';
                sysText.textContent = 'INACTIVE';
            }

            // LED
            const ledInd = document.getElementById('ledIndicator');
            const ledStatus = document.getElementById('ledStatus');
            if (systemData.lightsOn) {
                ledInd.className = 'indicator indicator-on';
                ledStatus.textContent = 'ON';
            } else {
                ledInd.className = 'indicator indicator-off';
                ledStatus.textContent = 'OFF';
            }

            // PIR
            const pirInd = document.getElementById('pirIndicator');
            const pirStatus = document.getElementById('pirStatus');
            if (systemData.pirDetected) {
                pirInd.className = 'indicator indicator-motion';
                pirStatus.textContent = 'DETECTED';
            } else {
                pirInd.className = 'indicator indicator-off';
                pirStatus.textContent = 'IDLE';
            }

            // Mode
            document.getElementById('controlMode').textContent = systemData.autoMode ? 'AUTO' : 'MANUAL';

            // Uptime
            document.getElementById('uptime').textContent = formatTime(systemData.uptime);

            // Brightness
            const bright = Math.round(systemData.currentBrightness);
            document.getElementById('brightness').textContent = bright + '%';
            document.getElementById('brightnessBar').style.width = bright + '%';

            // Light
            const lightLux = Math.round(systemData.lightLevel * (1100 / 4095));
            document.getElementById('lightLevel').textContent = lightLux;
            const lightPct = Math.round((systemData.lightLevel / 4095) * 100);
            document.getElementById('lightPercent').textContent = lightPct + '%';
            document.getElementById('lightBar').style.width = lightPct + '%';

            // Motion
            const lastMot = systemData.timeSinceLastMotion;
            document.getElementById('lastMotion').textContent = lastMot < 60 ? Math.round(lastMot) + 's' : formatTime(lastMot);

            // Setpoint
            document.getElementById('setpointDisplay').textContent = systemData.setpoint;

            // Energy
            document.getElementById('currentPower').textContent = systemData.currentPower.toFixed(1);
            document.getElementById('onTime').textContent = formatTime(systemData.totalOnTime);
            document.getElementById('totalEnergy').textContent = systemData.totalEnergy.toFixed(3);
            document.getElementById('totalCost').textContent = '$' + systemData.totalCost.toFixed(3);
            document.getElementById('savings').textContent = '$' + systemData.savings.toFixed(3);

            // Comfort
            const compPct = Math.round(systemData.comfortPercentage);
            document.getElementById('comfortPercent').textContent = compPct;
            document.getElementById('comfortBar').style.width = compPct + '%';
            document.getElementById('glareEvents').textContent = systemData.glareEvents;
            const roi = systemData.roi;
            document.getElementById('roi').textContent = roi > 0 ? roi.toFixed(1) : '-';
            document.getElementById('roiMonths').textContent = roi > 0 ? roi.toFixed(1) : '4.6';

            // Preset
            document.getElementById('activePreset').textContent = systemData.presetName || 'Estandar';

            // Annual
            const annSav = systemData.savings * 365;
            document.getElementById('annualSavings').textContent = Math.round(annSav);
        }

        async function controlSystem(action) {
            try {
                const res = await fetch('/api/control', {
                    method: 'POST',
                    headers: {'Content-Type': 'application/json'},
                    body: JSON.stringify({action})
                });
                if (res.ok) {
                    setTimeout(updateSystemData, 100);
                }
            } catch (e) {
                console.error('Control error:', e);
            }
        }

        async function applyPreset(index) {
            try {
                const res = await fetch('/api/preset', {
                    method: 'POST',
                    headers: {'Content-Type': 'application/json'},
                    body: JSON.stringify({index})
                });
                if (res.ok) {
                    document.querySelectorAll('.preset-btn').forEach((btn, i) => {
                        btn.classList.toggle('active', i === index);
                    });
                    setTimeout(updateSystemData, 100);
                }
            } catch (e) {
                console.error('Preset error:', e);
            }
        }

        function initChart() {
            const ctx = document.getElementById('historyChart').getContext('2d');
            historyChart = new Chart(ctx, {
                type: 'line',
                data: {
                    labels: [],
                    datasets: [
                        {
                            label: 'Brillo (%)',
                            data: [],
                            borderColor: 'rgb(59, 130, 246)',
                            backgroundColor: 'rgba(59, 130, 246, 0.1)',
                            tension: 0.3,
                            fill: true,
                            yAxisID: 'y'
                        },
                        {
                            label: 'Potencia (W)',
                            data: [],
                            borderColor: 'rgb(16, 185, 129)',
                            backgroundColor: 'rgba(16, 185, 129, 0.1)',
                            tension: 0.3,
                            fill: true,
                            yAxisID: 'y1'
                        }
                    ]
                },
                options: {
                    responsive: true,
                    maintainAspectRatio: false,
                    interaction: { mode: 'index', intersect: false },
                    scales: {
                        y: { type: 'linear', position: 'left', title: { display: true, text: 'Brillo (%)' } },
                        y1: { type: 'linear', position: 'right', title: { display: true, text: 'Potencia (W)' }, grid: { drawOnChartArea: false } },
                        x: { title: { display: true, text: 'Tiempo' } }
                    },
                    plugins: { legend: { position: 'top' } }
                }
            });
            setInterval(updateChart, 5000);
        }

        async function updateChart() {
            try {
                const res = await fetch('/api/history');
                if (res.ok) {
                    const data = await res.json();
                    historyChart.data.labels = data.timestamps.map(t => new Date(t * 1000).toLocaleTimeString('es-ES', {hour: '2-digit', minute: '2-digit'}));
                    historyChart.data.datasets[0].data = data.brightness;
                    historyChart.data.datasets[1].data = data.power;
                    historyChart.update('none');
                }
            } catch (e) {
                console.error('Chart error:', e);
            }
        }

        function formatTime(seconds) {
            const h = Math.floor(seconds / 3600);
            const m = Math.floor((seconds % 3600) / 60);
            const s = Math.floor(seconds % 60);
            return `${h.toString().padStart(2, '0')}:${m.toString().padStart(2, '0')}:${s.toString().padStart(2, '0')}`;
        }
    </script>
</body>
</html>
        )rawliteral";
        req->send(200, "text/html", html); });
    server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *req) { req->send(404); });
    server.begin();
}