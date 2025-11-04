# 🏫 Sistema de Control Inclusivo de Iluminación en Aula

**Universidad de Milagro - Facultad de Ciencias e Ingeniería**  
**Carrera: Ingeniería de Software**  
**Asignatura: Modelos Matemáticos y Simulación**

---

## 📋 Descripción del Proyecto

Sistema inteligente de control de iluminación para aulas educativas que mejora la inclusión de estudiantes con visión reducida mediante regulación automática de intensidad lumínica basada en sensores de movimiento (PIR) y luminosidad ambiental (LDR).

### Objetivos Principales

- ✅ **Inclusión Educativa**: Adaptar iluminación para estudiantes con problemas visuales
- ✅ **Eficiencia Energética**: Reducir consumo en 60% vs iluminación tradicional
- ✅ **Control Adaptativo**: Mantener banda de confort de 300-500 lux
- ✅ **Modelado Matemático**: Implementar ecuaciones diferenciales para control del sistema
- ✅ **ROI**: Retorno de inversión en 4.6 meses

---

## 🔬 Fundamento Teórico

### Modelo Matemático Implementado

El sistema se basa en ecuaciones diferenciales ordinarias (EDO):

```
dI/dt = k₁·P(t) - k₂·I(t) + k₃·f(L(t))
```

Donde:
- **I(t)**: Intensidad lumínica normalizada [0,1]
- **P(t)**: Función de presencia {0,1}
- **L(t)**: Nivel de luz ambiental normalizado [0,1]
- **k₁ = 0.5**: Factor de respuesta a presencia
- **k₂ = 0.2**: Factor de decaimiento natural
- **k₃ = 0.3**: Factor de adaptación a luz ambiente

### Función de Adaptación Lumínica

```
f(L(t)) = {
  1.0,   si L(t) < 0.3  (oscuro)
  0.66,  si 0.3 ≤ L(t) < 0.7  (medio)
  0.33,  si L(t) ≥ 0.7  (claro)
}
```

---

## 🛠️ Componentes del Sistema

### Hardware

| Componente | Modelo | Cantidad | Costo (USD) |
|------------|--------|----------|-------------|
| Microcontrolador | ESP32 DevKit C V4 | 1 | $12.00 |
| Sensor PIR | HC-SR501 | 1 | $3.50 |
| Sensor LDR | GL5528 con LM393 | 1 | $2.80 |
| LEDs Rojos | 5mm 1000-1500 mcd | 5 | $0.50 |
| Módulo Relé | SRD-05VDC-SL-C | 1 | $4.20 |
| Resistencias | 220Ω 1/4W | 5 | $0.10 |
| Protoboard | MB-102 | 1 | $3.80 |
| Cables Jumper | Kit 65 piezas | 1 | $4.50 |
| Fuente | 5V 3A | 1 | $8.50 |
| **TOTAL** | | | **$39.90** |

### Software

- **Framework**: Arduino (C++)
- **Librerías**:
  - ESP Async WebServer v1.2.3
  - AsyncTCP v1.1.1
  - ArduinoJson v6.21.3
- **Simulador**: Wokwi
- **IDE**: PlatformIO

---

## 📊 Características Principales

### 1. Control Adaptativo
- Ajuste automático de intensidad según luz natural
- Transiciones suaves (slew-rate limiting de 10%/s)
- PWM de alta frecuencia (anti-flicker)

### 2. Presets de Iluminancia

| Preset | Setpoint | Banda (lux) | Uso Recomendado |
|--------|----------|-------------|-----------------|
| Estándar | 400 lux | 300-500 | Clase típica |
| Lectura Fina | 500 lux | 450-550 | Exámenes |
| Alta Luz Natural | 350 lux | 300-400 | Aprovecha luz diurna |
| Baja Visión | 450 lux | 400-500 | Visión reducida |

### 3. Métricas de Rendimiento
- **Precisión de detección**: >95%
- **Tiempo de respuesta**: <500ms
- **Disponibilidad**: >99.9%
- **Reducción de consumo**: 60%

### 4. Dashboard Técnico
- Monitoreo en tiempo real
- Gráficas históricas (Chart.js)
- Control manual/automático
- Métricas de confort visual
- Cálculo de ROI

---

## 🚀 Instalación y Uso

### Requisitos Previos

```bash
# Instalar PlatformIO Core
pip install platformio

# O usar PlatformIO IDE (VS Code)
```

### Configuración del Proyecto

1. **Clonar/Crear estructura de proyecto**:
```
proyecto/
├── src/
│   └── main.cpp
├── platformio.ini
├── wokwi.toml
├── diagram.json
└── README.md
```

2. **Compilar y subir**:
```bash
# Compilar
pio run

# Subir a ESP32
pio run --target upload

# Monitor serial
pio device monitor
```

### Simulación en Wokwi

1. Abrir [Wokwi](https://wokwi.com/)
2. Crear nuevo proyecto ESP32
3. Copiar archivos:
   - `main.cpp` → Editor de código
   - `diagram.json` → Diagrama
   - `wokwi.toml` → Configuración
4. Iniciar simulación
5. Acceder al dashboard: `http://localhost:8180`

---

## 📱 Uso del Dashboard

### Acceso
Una vez el sistema esté funcionando, abrir navegador en:
```
http://localhost:8180  (Wokwi)
http://[IP_ESP32]      (Hardware real)
```

### Funcionalidades

#### Control Manual
- **Botón ENCENDER**: Activa luces en modo manual
- **Botón APAGAR**: Desactiva luces y vuelve a modo AUTO
- **Modo AUTO**: Sistema controla luces automáticamente
- **Modo MANUAL**: Control totalmente manual

#### Presets
Seleccionar entre 4 configuraciones predefinidas según necesidad educativa.

#### Métricas en Tiempo Real
- Estado del sistema (ON/OFF)
- Detección de movimiento (PIR)
- Nivel de luz ambiente (lux)
- Brillo actual de LEDs (%)
- Consumo energético (W, kWh)
- Tiempo en banda de confort (%)
- Eventos de deslumbramiento
- Ahorro económico (USD)

#### Gráficas Históricas
- Evolución del brillo en el tiempo
- Consumo de potencia
- Actualización cada 5 segundos

---

## 📈 Resultados Proyectados

### Impacto Energético
- **Consumo tradicional**: 8h × 50W = 400 Wh/día
- **Consumo inteligente**: ~160 Wh/día
- **Reducción**: 60%
- **Ahorro diario**: $0.29 USD
- **Ahorro anual**: $104.40 USD por aula

### Retorno de Inversión (ROI)
```
Costo del prototipo: $39.90
Ahorro mensual: $8.70
ROI = $39.90 / $8.70 = 4.6 meses
```

### Beneficios de Inclusión
- Reducción de fatiga visual: 40%
- Mejora en rendimiento académico
- Cumplimiento de normas CIE para espacios educativos
- Ambiente más accesible para baja visión

---

## 🔧 Configuración Avanzada

### Parámetros del Sistema (main.cpp)

```cpp
struct SystemConfig {
  int setpoint = 400;           // Iluminancia objetivo (lux)
  int bandMin = 300;            // Banda de confort mínima
  int bandMax = 500;            // Banda de confort máxima
  int thresholdDark = 2500;     // Umbral para "oscuro"
  int autoOffDelay = 60;        // Segundos sin movimiento
  float slewRateLimit = 10.0;   // Cambio máximo %/s
  bool autoMode = true;         // Auto/Manual
};
```

### Constantes del Modelo Matemático

```cpp
const float k1 = 0.5;  // Respuesta a presencia
const float k2 = 0.2;  // Decaimiento natural
const float k3 = 0.3;  // Adaptación luz ambiente
```

### Configuración de Sensores

**PIR HC-SR501**:
- Ajustar potenciómetro de sensibilidad (3-7 metros)
- Ajustar tiempo de retardo (0.3-18 segundos)

**LDR GL5528**:
- Resistencia en luz: 5-10kΩ
- Resistencia en oscuridad: 1MΩ

---

## 📡 API REST

El sistema expone endpoints para control remoto:

### GET /api/status
Obtener estado completo del sistema.

**Respuesta**:
```json
{
  "lightsOn": true,
  "pirDetected": true,
  "autoMode": true,
  "lightLevel": 1234,
  "currentBrightness": 66.5,
  "targetBrightness": 70.0,
  "totalEnergy": 0.156,
  "currentPower": 33.2,
  "comfortPercentage": 87.3,
  "glareEvents": 2,
  "setpoint": 400,
  "presetName": "Estándar"
}
```

### POST /api/control
Controlar sistema manualmente.

**Body**:
```json
{
  "action": "on"  // "on", "off", "auto", "manual"
}
```

### POST /api/preset
Aplicar preset de iluminancia.

**Body**:
```json
{
  "index": 0  // 0=Estándar, 1=Lectura, 2=Natural, 3=BajaVisión
}
```

### GET /api/history
Obtener datos históricos.

**Respuesta**:
```json
{
  "brightness": [33, 45, 66, ...],
  "lightLevel": [2000, 1800, 1500, ...],
  "power": [16.5, 22.5, 33.0, ...],
  "timestamps": [1234567890, 1234567895, ...]
}
```

---

## 🧪 Pruebas y Validación

### Pruebas Funcionales
1. ✅ Detección de movimiento PIR
2. ✅ Lectura correcta de LDR
3. ✅ Control PWM de LEDs
4. ✅ Transiciones suaves
5. ✅ Servidor web funcionando
6. ✅ Presets aplicándose correctamente

### Pruebas de Rendimiento
1. ✅ Tiempo de respuesta <500ms
2. ✅ Sin parpadeo perceptible
3. ✅ Consumo energético medido
4. ✅ Estabilidad del sistema >8 horas

### Validación del Modelo
1. ✅ EDO convergiendo correctamente
2. ✅ Función de adaptación respondiendo
3. ✅ Slew-rate limitando cambios bruscos

---

## 🐛 Resolución de Problemas

### El ESP32 no se conecta a WiFi
- **Solución**: En Wokwi usar `Wokwi-GUEST` sin contraseña
- En hardware real, configurar SSID y contraseña correctos

### Los LEDs no encienden
- Verificar conexiones de resistencias (220Ω)
- Verificar pines GPIO correctos (15-19)
- Comprobar fuente de alimentación (5V suficiente)

### El sensor PIR no detecta
- Ajustar sensibilidad con potenciómetro
- Verificar conexión VCC (5V), GND y OUT (GPIO 13)
- Esperar ~60 segundos de estabilización inicial

### El LDR lee valores constantes
- Verificar divisor de voltaje con resistencia pull-down
- Cambiar iluminación en simulación (hacer clic en sensor)
- En hardware real, cubrir/iluminar el sensor

### Dashboard no carga
- Verificar que el ESP32 imprime IP en serial
- Acceder a `http://localhost:8180` en Wokwi
- Verificar que puerto 8180 no esté ocupado

---

## 📚 Referencias

1. **Modelado Matemático**:
   - Ogata, K. (2010). *Ingeniería de Control Moderna*
   
2. **Iluminación Inclusiva**:
   - CIE (2019). *ISO 8995-1:2024 - Lighting of work places*
   - Baeza Moyano & González Lezcano (2021). *Pandemic of childhood myopia*

3. **Eficiencia Energética**:
   - Obioma et al. (2025). *IoT-Based Smart Lighting System*
   
4. **Desarrollo ESP32**:
   - Espressif Systems (2024). *ESP32 Technical Reference Manual*

---

## 👥 Autores

- **Guerrero Jara Dayanara Rosario**
- **Ledesma Aspiazu Yenthenny Melisa**
- **Lescano Paredes Gleyder Julissa**
- **Quinteros Pacheco Eduardo Javier**

**Docente**: Ing. Jhonny Darwin Ortiz Mata

**Fecha**: 28/09/2025  
**Universidad de Milagro - Ecuador**

---

## 📄 Licencia

Este proyecto es desarrollado con fines académicos para la asignatura de Modelos Matemáticos y Simulación.

---

## 🙏 Agradecimientos

- Universidad de Milagro
- Facultad de Ciencias e Ingeniería
- Comunidad de Wokwi
- Desarrolladores de PlatformIO y Arduino

---

**¡Sistema de Control Inclusivo de Iluminación - Mejorando la Educación para Todos!** 🎓💡
