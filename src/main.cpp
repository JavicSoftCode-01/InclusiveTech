#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <ArduinoJson.h>

// Definición de pines
#define PIR_PIN 13              // Sensor PIR
#define RELAY_PIN 12            // Módulo relé (opcional, para control externo)
#define LIGHT_SENSOR_PIN 36     // Fotoresistor analógico (A0 = GPIO36)
const int LED_PINS[] = {15, 16, 17, 18, 19};  // Pines para los 5 LEDs rojos unitarios
const int NUM_LEDS = 5;

// Configuración WiFi para Wokwi
const char* ssid = "Wokwi-GUEST";
const char* password = "";

// Servidor web
AsyncWebServer server(80);

// Variables de estado del sistema
struct SystemState {
  bool pirDetected = false;
  bool lightsOn = false;
  bool manualOverride = false;
  int lightLevel = 0;
  int brightnessLevel = 0; // 0 (apagado), 33, 66, 100 (%)
  unsigned long lastMotion = 0;
  unsigned long lastPirChange = 0; // Para debouncing
  unsigned long systemStartTime = 0;
  unsigned long totalOnTime = 0;
  unsigned long lastOnTime = 0;
  float totalEnergyConsumed = 0.0;
  float currentPowerConsumption = 0.0;
} state;

// Configuración del sistema
const int LIGHT_THRESHOLD = 2500;    // Umbral para encender LEDs en modo automático (ajustado para simulación)
const int LIGHT_LOW_THRESHOLD = 3000; // Umbral para brillo bajo
const int LIGHT_MEDIUM_THRESHOLD = 1000; // Umbral para brillo medio
const unsigned long AUTO_OFF_DELAY = 60000;  // 1 minuto
const unsigned long PIR_DEBOUNCE = 100;     // Debounce para PIR
const float POWER_CONSUMPTION_WATTS = 50.0;  // Consumo máximo en Watts de las luces LED
const float ELECTRICITY_RATE_ECUADOR = 0.092; // USD por kWh en Ecuador

// Funciones del sistema
void setupWiFi();
void setupWebServer();
void handleSensors();
void updateEnergyMetrics();
void turnOnLights(bool manual = false);
void turnOffLights();
void setLedBrightness(int percentage);
String getSystemStatus();

void setup() {
  Serial.begin(115200);
  
  // Configuración de pines
  pinMode(PIR_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(LIGHT_SENSOR_PIN, INPUT);
  for (int i = 0; i < NUM_LEDS; i++) {
    pinMode(LED_PINS[i], OUTPUT);
    analogWrite(LED_PINS[i], 0);
  }
  
  // Estados iniciales
  digitalWrite(RELAY_PIN, LOW);
  
  // Inicializar variables de tiempo
  state.systemStartTime = millis();
  state.lastPirChange = millis();
  
  Serial.println("=== SISTEMA DE ILUMINACION INCLUSIVA PARA AULAS ===");
  Serial.println("Configurando WiFi...");
  
  setupWiFi();
  setupWebServer();
  
  Serial.println("Sistema listo. Accede al dashboard en: http://localhost:8180");
  Serial.println("====================================================");
}

void loop() {
  handleSensors();
  updateEnergyMetrics();
  
  // Pequeña pausa para estabilidad
  delay(100);
}

void setupWiFi() {
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.print("WiFi conectado. IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Error: No se pudo conectar a WiFi");
  }
}

void setupWebServer() {
  // Servir página principal
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    String html = R"rawliteral(
<!DOCTYPE html>
<html lang="es">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Control de Iluminacion Inclusiva - Aula Inteligente</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }
        
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            padding: 20px;
        }
        
        .container {
            max-width: 1400px;
            margin: 0 auto;
            background: rgba(255, 255, 255, 0.95);
            border-radius: 20px;
            box-shadow: 0 20px 40px rgba(0, 0, 0, 0.1);
            padding: 30px;
        }
        
        .header {
            text-align: center;
            margin-bottom: 30px;
            padding-bottom: 20px;
            border-bottom: 3px solid #667eea;
        }
        
        .header h1 {
            color: #2c3e50;
            font-size: 2.5em;
            margin-bottom: 10px;
        }
        
        .header p {
            color: #7f8c8d;
            font-size: 1.2em;
        }
        
        .dashboard {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
            gap: 20px;
            margin-bottom: 30px;
        }
        
        .card {
            background: white;
            border-radius: 15px;
            padding: 25px;
            box-shadow: 0 10px 30px rgba(0, 0, 0, 0.1);
            transition: transform 0.3s ease;
        }
        
        .card:hover {
            transform: translateY(-5px);
        }
        
        .card h3 {
            color: #2c3e50;
            margin-bottom: 15px;
            font-size: 1.3em;
            display: flex;
            align-items: center;
            gap: 10px;
        }
        
        .status-card {
            background: linear-gradient(135deg, #3498db, #2980b9);
            color: white;
        }
        
        .energy-card {
            background: linear-gradient(135deg, #27ae60, #219a52);
            color: white;
        }
        
        .control-card {
            background: linear-gradient(135deg, #f39c12, #e67e22);
            color: white;
        }
        
        .sensors-card {
            background: linear-gradient(135deg, #9b59b6, #8e44ad);
            color: white;
        }
        
        .metric {
            display: flex;
            justify-content: space-between;
            align-items: center;
            margin: 10px 0;
            padding: 10px;
            background: rgba(255, 255, 255, 0.1);
            border-radius: 8px;
        }
        
        .metric-value {
            font-weight: bold;
            font-size: 1.2em;
        }
        
        .status-indicator {
            width: 20px;
            height: 20px;
            border-radius: 50%;
            display: inline-block;
            margin-right: 10px;
        }
        
        .status-on { background-color: #27ae60; }
        .status-off { background-color: #e74c3c; }
        .status-motion { background-color: #f39c12; }
        .status-no-motion { background-color: #95a5a6; }
        
        .control-buttons {
            display: flex;
            gap: 15px;
            margin-top: 20px;
        }
        
        .btn {
            flex: 1;
            padding: 15px;
            border: none;
            border-radius: 10px;
            font-size: 1.1em;
            font-weight: bold;
            cursor: pointer;
            transition: all 0.3s ease;
            text-transform: uppercase;
        }
        
        .btn-on {
            background: linear-gradient(135deg, #27ae60, #2ecc71);
            color: white;
        }
        
        .btn-off {
            background: linear-gradient(135deg, #e74c3c, #c0392b);
            color: white;
        }
        
        .btn:hover {
            transform: translateY(-2px);
            box-shadow: 0 5px 15px rgba(0, 0, 0, 0.2);
        }
        
        .progress-bar {
            width: 100%;
            height: 10px;
            background: rgba(255, 255, 255, 0.2);
            border-radius: 5px;
            overflow: hidden;
            margin: 10px 0;
        }
        
        .progress-fill {
            height: 100%;
            background: rgba(255, 255, 255, 0.8);
            border-radius: 5px;
            transition: width 0.3s ease;
        }
        
        .benefits-section {
            margin-top: 30px;
            padding: 25px;
            background: linear-gradient(135deg, #74b9ff, #0984e3);
            border-radius: 15px;
            color: white;
        }
        
        .benefits-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(250px, 1fr));
            gap: 20px;
            margin-top: 20px;
        }
        
        .benefit-item {
            background: rgba(255, 255, 255, 0.1);
            padding: 20px;
            border-radius: 10px;
            text-align: center;
        }
        
        .benefit-icon {
            font-size: 2.5em;
            margin-bottom: 10px;
        }
        
        @media (max-width: 768px) {
            .container {
                padding: 20px;
            }
            
            .header h1 {
                font-size: 2em;
            }
            
            .dashboard {
                grid-template-columns: 1fr;
            }
            
            .control-buttons {
                flex-direction: column;
            }
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>Control de Iluminacion Inclusiva</h1>
            <p>Sistema Inteligente para Aulas - Mejorando la Inclusion Educativa</p>
        </div>
        
        <div class="dashboard">
            <!-- Estado del Sistema -->
            <div class="card status-card">
                <h3>Estado del Sistema</h3>
                <div class="metric">
                    <span>Estado de LEDs:</span>
                    <span class="metric-value">
                        <span class="status-indicator" id="ledStatus"></span>
                        <span id="ledStatusText">APAGADO</span>
                    </span>
                </div>
                <div class="metric">
                    <span>Deteccion de Movimiento:</span>
                    <span class="metric-value">
                        <span class="status-indicator" id="motionStatus"></span>
                        <span id="motionStatusText">SIN MOVIMIENTO</span>
                    </span>
                </div>
                <div class="metric">
                    <span>Modo de Control:</span>
                    <span class="metric-value" id="controlMode">AUTOMATICO</span>
                </div>
                <div class="metric">
                    <span>Tiempo Activo:</span>
                    <span class="metric-value" id="uptime">00:00:00</span>
                </div>
            </div>
            
            <!-- Sensores -->
            <div class="card sensors-card">
                <h3>Sensores Ambientales</h3>
                <div class="metric">
                    <span>Nivel de Luz:</span>
                    <span class="metric-value" id="lightLevel">0</span>
                </div>
                <div class="progress-bar">
                    <div class="progress-fill" id="lightProgress"></div>
                </div>
                <div class="metric">
                    <span>Umbral de Activacion:</span>
                    <span class="metric-value">2500 lux</span>
                </div>
                <div class="metric">
                    <span>Ultimo Movimiento:</span>
                    <span class="metric-value" id="lastMotion">Nunca</span>
                </div>
                <div class="metric" style="margin-top: 20px;">
                    <span>Brillo de LEDs:</span>
                    <span class="metric-value" id="brightness">0%</span>
                </div>
                <div class="progress-bar">
                    <div class="progress-fill" id="brightnessProgress" style="width: 0%;"></div>
                </div>
            </div>
            
            <!-- Metricas Energeticas -->
            <div class="card energy-card">
                <h3>Consumo Energetico</h3>
                <div class="metric">
                    <span>Consumo Actual:</span>
                    <span class="metric-value" id="currentPower">0 W</span>
                </div>
                <div class="metric">
                    <span>Tiempo Encendido:</span>
                    <span class="metric-value" id="onTime">00:00:00</span>
                </div>
                <div class="metric">
                    <span>Energia Total:</span>
                    <span class="metric-value" id="totalEnergy">0.000 kWh</span>
                </div>
                <div class="metric">
                    <span>Costo Total (USD):</span>
                    <span class="metric-value" id="totalCost">$0.00</span>
                </div>
                <div class="metric">
                    <span>Ahorro Estimado/Dia:</span>
                    <span class="metric-value" id="savings">$0.00</span>
                </div>
            </div>
            
            <!-- Controles -->
            <div class="card control-card">
                <h3>Control Manual</h3>
                <div class="metric">
                    <span>Control Remoto:</span>
                    <span class="metric-value">ACTIVO</span>
                </div>
                <div class="control-buttons">
                    <button class="btn btn-on" onclick="controlLights(true)">
                        ENCENDER
                    </button>
                    <button class="btn btn-off" onclick="controlLights(false)">
                        APAGAR
                    </button>
                </div>
            </div>
        </div>
        
        <!-- Seccion de Beneficios -->
        <div class="benefits-section">
            <h2>Beneficios del Sistema de Iluminacion Inclusiva</h2>
            <div class="benefits-grid">
                <div class="benefit-item">
                    <div class="benefit-icon">👁</div>
                    <h4>Mejora Visual</h4>
                    <p>Iluminacion adaptativa para estudiantes con problemas de vision, reduciendo la fatiga ocular</p>
                </div>
                <div class="benefit-item">
                    <div class="benefit-icon">💡</div>
                    <h4>Eficiencia Energetica</h4>
                    <p>Ahorro de hasta 60% en consumo electrico mediante deteccion inteligente</p>
                </div>
                <div class="benefit-item">
                    <div class="benefit-icon">🌍</div>
                    <h4>Sostenibilidad</h4>
                    <p>Reduce la huella de carbono del centro educativo</p>
                </div>
                <div class="benefit-item">
                    <div class="benefit-icon">🎓</div>
                    <h4>Inclusion Educativa</h4>
                    <p>Crea un ambiente de aprendizaje mas accesible para todos</p>
                </div>
            </div>
        </div>
    </div>

    <script>
        let systemData = {
            lightsOn: false,
            pirDetected: false,
            manualOverride: false,
            lightLevel: 0,
            brightnessLevel: 0,
            timeSinceLastMotion: 0,
            totalOnTime: 0,
            totalEnergy: 0,
            currentPower: 0,
            uptime: 0
        };

        // Actualizar datos del sistema
        async function updateSystemData() {
            try {
                const response = await fetch('/api/status');
                if (response.ok) {
                    systemData = await response.json();
                    updateUI();
                }
            } catch (error) {
                console.error('Error fetching data:', error);
            }
        }

        // Actualizar interfaz de usuario
        function updateUI() {
            // Estado de LEDs
            const ledStatus = document.getElementById('ledStatus');
            const ledStatusText = document.getElementById('ledStatusText');
            if (systemData.lightsOn) {
                ledStatus.className = 'status-indicator status-on';
                ledStatusText.textContent = 'ENCENDIDO';
            } else {
                ledStatus.className = 'status-indicator status-off';
                ledStatusText.textContent = 'APAGADO';
            }

            // Estado de movimiento
            const motionStatus = document.getElementById('motionStatus');
            const motionStatusText = document.getElementById('motionStatusText');
            if (systemData.pirDetected) {
                motionStatus.className = 'status-indicator status-motion';
                motionStatusText.textContent = 'DETECTADO';
            } else {
                motionStatus.className = 'status-indicator status-no-motion';
                motionStatusText.textContent = 'SIN MOVIMIENTO';
            }

            // Modo de control
            document.getElementById('controlMode').textContent = 
                systemData.manualOverride ? 'MANUAL' : 'AUTOMATICO';

            // Tiempo activo
            document.getElementById('uptime').textContent = formatTime(systemData.uptime);

            // Nivel de luz
            document.getElementById('lightLevel').textContent = systemData.lightLevel + ' lux';
            const lightProgress = (systemData.lightLevel / 4095) * 100;
            document.getElementById('lightProgress').style.width = lightProgress + '%';

            // Ultimo movimiento
            let lastMotion = 'Nunca';
            if (systemData.timeSinceLastMotion > 0) {
                if (systemData.timeSinceLastMotion > 86400) {
                    lastMotion = 'Hace más de un día';
                } else {
                    lastMotion = formatTime(systemData.timeSinceLastMotion) + ' hace';
                }
            }
            document.getElementById('lastMotion').textContent = lastMotion;

            // Consumo energetico
            document.getElementById('currentPower').textContent = systemData.currentPower.toFixed(1) + ' W';
            document.getElementById('onTime').textContent = formatTime(systemData.totalOnTime);
            document.getElementById('totalEnergy').textContent = systemData.totalEnergy.toFixed(3) + ' kWh';
            
            const totalCost = systemData.totalEnergy * 0.092; // Tarifa Ecuador
            document.getElementById('totalCost').textContent = '$' + totalCost.toFixed(3);
            
            // Calculo de ahorro estimado por dia (comparado con iluminacion tradicional)
            const traditionalConsumption = 8 * 50 / 1000; // 8 horas * 50W
            const smartConsumption = systemData.totalOnTime / 3600 * 50 * (systemData.brightnessLevel / 100) / 1000; // horas reales * 50W * factor brillo
            const dailySavings = Math.max(0, (traditionalConsumption - smartConsumption) * 0.092);
            document.getElementById('savings').textContent = '$' + dailySavings.toFixed(3);

            // Brillo
            document.getElementById('brightness').textContent = systemData.brightnessLevel + '%';
            document.getElementById('brightnessProgress').style.width = systemData.brightnessLevel + '%';
        }

        // Controlar luces
        async function controlLights(turnOn) {
            try {
                const response = await fetch('/api/control', {
                    method: 'POST',
                    headers: {
                        'Content-Type': 'application/json',
                    },
                    body: JSON.stringify({ action: turnOn ? 'on' : 'off' })
                });
                
                if (response.ok) {
                    console.log('Comando enviado:', turnOn ? 'encender' : 'apagar');
                }
            } catch (error) {
                console.error('Error controlling lights:', error);
            }
        }

        // Formatear tiempo en HH:MM:SS
        function formatTime(seconds) {
            const hours = Math.floor(seconds / 3600);
            const minutes = Math.floor((seconds % 3600) / 60);
            const secs = Math.floor(seconds % 60);
            
            return hours.toString().padStart(2, '0') + ':' + 
                   minutes.toString().padStart(2, '0') + ':' + 
                   secs.toString().padStart(2, '0');
        }

        // Inicializar y actualizar cada segundo
        updateSystemData();
        setInterval(updateSystemData, 1000);
    </script>
</body>
</html>
    )rawliteral";
    request->send(200, "text/html", html);
  });

  // API para obtener estado del sistema
  server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "application/json", getSystemStatus());
  });

  // API para controlar las luces
  server.on("/api/control", HTTP_POST, [](AsyncWebServerRequest *request) {
    // Esta función se ejecutará después de procesar el body
  }, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    // Procesar datos JSON
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, (char*)data);
    
    String action = doc["action"];
    
    if (action == "on") {
      turnOnLights(true); // Manual override
      state.manualOverride = true;
      Serial.println("Control manual: ENCENDER luces (permanece ON hasta apagar)");
    } else if (action == "off") {
      turnOffLights();
      state.manualOverride = false;
      Serial.println("Control manual: APAGAR luces (vuelve a modo AUTO)");
    }
    
    request->send(200, "application/json", "{\"status\":\"ok\"}");
  });

  server.begin();
  Serial.println("Servidor web iniciado en puerto 80");
}

void handleSensors() {
  // Leer sensores
  bool rawPirState = digitalRead(PIR_PIN);
  unsigned long now = millis();
  int analogValue = analogRead(LIGHT_SENSOR_PIN);
  state.lightLevel = analogValue; // Valor crudo (0-4095, bajo = oscuro)

  // Debouncing para PIR
  if ((rawPirState != state.pirDetected) && (now - state.lastPirChange > PIR_DEBOUNCE)) {
    state.pirDetected = rawPirState;
    state.lastPirChange = now;
    
    if (state.pirDetected) {
      Serial.println("MOVIMIENTO DETECTADO!");
      state.lastMotion = now;
    } else {
      Serial.println("Fin del movimiento");
    }
  }

  // Ajustar brillo según nivel de luz (siempre, incluso en modo manual)
  if (state.lightsOn) {
    if (state.lightLevel > LIGHT_LOW_THRESHOLD) {
      setLedBrightness(33); // Bajo (mucha luz)
    } else if (state.lightLevel >= LIGHT_MEDIUM_THRESHOLD) {
      setLedBrightness(66); // Medio
    } else {
      setLedBrightness(100); // Intenso (poca luz)
    }
  }

  // Lógica de control automático (solo si no hay override manual)
  if (!state.manualOverride) {
    // Encender luces si hay movimiento y está oscuro
    if (state.pirDetected && state.lightLevel < LIGHT_THRESHOLD && !state.lightsOn) {
      Serial.printf("Oscuro (%d) + Movimiento -> ENCENDER luces\n", state.lightLevel);
      turnOnLights(false);
    }
    
    // Apagar luces por timeout o mucha luz natural
    if (state.lightsOn) {
      if (now - state.lastMotion > AUTO_OFF_DELAY) {
        Serial.println("Timeout sin movimiento -> APAGAR luces");
        turnOffLights();
      } else if (state.lightLevel >= LIGHT_THRESHOLD) {
        Serial.printf("Suficiente luz natural (%d) -> APAGAR luces\n", state.lightLevel);
        turnOffLights();
      }
    }
  }
  
  // Mostrar estado cada 5 segundos
  static unsigned long lastPrint = 0;
  if (now - lastPrint > 5000) {
    Serial.printf("PIR: %s | Luz: %d | LEDs: %s | Modo: %s | Brillo: %d%%\n",
                  state.pirDetected ? "DETECTADO" : "NINGUNO",
                  state.lightLevel,
                  state.lightsOn ? "ON" : "OFF",
                  state.manualOverride ? "MANUAL" : "AUTO",
                  state.brightnessLevel);
    lastPrint = now;
  }
}

void updateEnergyMetrics() {
  static unsigned long lastUpdate = 0;
  unsigned long now = millis();
  unsigned long delta = now - lastUpdate;
  
  if (delta >= 1000) { // Actualizar cada segundo
    if (state.lightsOn) {
      state.totalOnTime += delta / 1000.0;
      state.currentPowerConsumption = POWER_CONSUMPTION_WATTS * (state.brightnessLevel / 100.0);
      
      // Calcular energía consumida (kWh) con delta preciso
      state.totalEnergyConsumed += (state.currentPowerConsumption * (delta / 1000.0)) / 3600000.0;
    } else {
      state.currentPowerConsumption = 0.0;
      state.brightnessLevel = 0;
    }
    
    lastUpdate = now;
  }
}

void turnOnLights(bool manual) {
  if (!state.lightsOn) {
    Serial.println("=== ENCENDIENDO SISTEMA DE ILUMINACION ===");
    
    // Encender relé (si se usa)
    digitalWrite(RELAY_PIN, HIGH);
    
    // Establecer brillo inicial basado en luz ambiente
    if (state.lightLevel > LIGHT_LOW_THRESHOLD) {
      setLedBrightness(33); // Bajo
    } else if (state.lightLevel >= LIGHT_MEDIUM_THRESHOLD) {
      setLedBrightness(66); // Medio
    } else {
      setLedBrightness(100); // Intenso
    }
    
    state.lightsOn = true;
    state.lastOnTime = millis();
    state.lastMotion = millis(); // Resetear timer para mantener 1 minuto en AUTO
    
    if (manual) {
      state.manualOverride = true;
      Serial.println("Modo MANUAL activado (luces permanecerán encendidas)");
    } else {
      Serial.println("Modo AUTO: Luces encendidas por movimiento");
    }
    
    Serial.println("Sistema de iluminación ENCENDIDO");
    Serial.println("===========================================");
  }
}

void turnOffLights() {
  if (state.lightsOn) {
    Serial.println("=== APAGANDO SISTEMA DE ILUMINACION ===");
    
    // Apagar los 5 LEDs unitarios
    for (int i = 0; i < NUM_LEDS; i++) {
      analogWrite(LED_PINS[i], 0);
    }
    
    // Apagar relé
    digitalWrite(RELAY_PIN, LOW);
    
    state.lightsOn = false;
    state.brightnessLevel = 0;
    
    // Resetear override manual al apagar
    if (state.manualOverride) {
      state.manualOverride = false;
      Serial.println("Modo AUTOMATICO restablecido");
    }
    
    Serial.println("Sistema de iluminación APAGADO");
    Serial.println("==========================================");
  }
}

void setLedBrightness(int percentage) {
  int pwmValue = (percentage * 255) / 100; // Convertir porcentaje a valor PWM (0-255)
  for (int i = 0; i < NUM_LEDS; i++) {
    analogWrite(LED_PINS[i], pwmValue);
  }
  state.brightnessLevel = percentage;
  Serial.printf("Brillo ajustado a %d%% (PWM: %d)\n", percentage, pwmValue);
}

String getSystemStatus() {
  DynamicJsonDocument doc(1024);
  
  doc["lightsOn"] = state.lightsOn;
  doc["pirDetected"] = state.pirDetected;
  doc["manualOverride"] = state.manualOverride;
  doc["lightLevel"] = state.lightLevel;
  doc["brightnessLevel"] = state.brightnessLevel;
  doc["timeSinceLastMotion"] = (millis() - state.lastMotion) / 1000.0;
  doc["totalOnTime"] = state.totalOnTime;
  doc["totalEnergy"] = state.totalEnergyConsumed;
  doc["currentPower"] = state.currentPowerConsumption;
  doc["uptime"] = (millis() - state.systemStartTime) / 1000.0;
  
  String response;
  serializeJson(doc, response);
  return response;
}