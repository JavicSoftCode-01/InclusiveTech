#include <Adafruit_NeoPixel.h>
// #include <BH1750.h>  // Comentado temporalmente
// #include <Wire.h>     // Comentado temporalmente

// Definición de pines
#define LED_PIN 14              // LED indicador rojo
#define PIR_PIN 13              // Sensor PIR
#define RELAY_PIN 12            // Módulo relé 
#define NEOPIXEL_PIN 27         // Tira de LEDs NeoPixel
#define LIGHT_SENSOR_PIN A0     // Fotoresistor analógico (backup)
#define NUM_PIXELS 8            // Número de LEDs en la tira

// Configuración NeoPixel
Adafruit_NeoPixel strip(NUM_PIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
// BH1750 lightMeter;  // Comentado temporalmente

// Variables de estado
int pirState = LOW;             // Estado inicial sin movimiento
int val = 0;                    // Lectura del PIR
float lightLevel = 0;           // Nivel de luz ambiente
bool lightsOn = false;          // Estado de las luces
unsigned long lastMotion = 0;   // Tiempo del último movimiento
unsigned long lightDelay = 30000; // 30 segundos antes de apagar

// Umbral de luz (ajustable según necesidad)
float lightThreshold = 300;    // Para sensor analógico (0-4095)

// Declaración de funciones
void turnOnLights();
void turnOffLights();
void colorWipe(uint32_t color, int wait);

void setup() {
  // Configuración de pines
  pinMode(LED_PIN, OUTPUT);
  pinMode(PIR_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(LIGHT_SENSOR_PIN, INPUT);
  
  // Estados iniciales
  digitalWrite(LED_PIN, LOW);
  digitalWrite(RELAY_PIN, LOW);
  
  /* SENSOR BH1750 DESACTIVADO TEMPORALMENTE
  // Inicializar I2C con pines específicos
  Wire.begin(21, 22); // SDA=21, SCL=22
  
  // Inicializar sensor BH1750 con modo de alta resolución
  if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
    Serial.println("Sensor BH1750 inicializado correctamente");
    lightMeter.configure(BH1750::CONTINUOUS_HIGH_RES_MODE);
  } else {
    Serial.println("Error: No se pudo inicializar el sensor BH1750");
    Serial.println("Usando sensor analógico de respaldo");
  }
  */
  
  // Inicializar NeoPixel
  strip.begin();
  strip.show(); // Inicializar todos los píxeles apagados
  strip.setBrightness(100); // Brillo al 40%
  
  Serial.begin(9600);
  Serial.println("=== SISTEMA DE ILUMINACIÓN INTELIGENTE ===");
  Serial.println("Componentes: PIR + Sensor Analógico + Relé + NeoPixel");
  Serial.println("BH1750 desactivado temporalmente");
  Serial.println("==========================================");
  
  delay(2000); // Tiempo para estabilizar sensores
}

void loop() {
  // Leer sensores
  val = digitalRead(PIR_PIN);
  
  // Leer sensor de luz analógico (fotoresistor)
  int analogValue = analogRead(LIGHT_SENSOR_PIN);
  lightLevel = analogValue; // Convertir a escala 0-4095
  
  // Mostrar lecturas en monitor serial cada segundo
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 1000) {
    Serial.print("PIR: ");
    Serial.print(val ? "MOVIMIENTO" : "SIN MOVIMIENTO");
    Serial.print(" | Luz: ");
    Serial.print(lightLevel);
    Serial.print(" (0-4095) | Luces: ");
    Serial.println(lightsOn ? "ENCENDIDAS" : "APAGADAS");
    lastPrint = millis();
  }
  
  // Detección de movimiento
  if (val == HIGH) {
    digitalWrite(LED_PIN, HIGH); // Encender LED indicador
    
    if (pirState == LOW) {
      Serial.println("¡MOVIMIENTO DETECTADO!");
      pirState = HIGH;
    }
    
    // Actualizar tiempo del último movimiento
    lastMotion = millis();
    
    // Encender luces solo si está oscuro
    if (lightLevel < lightThreshold && !lightsOn) {
      Serial.print("Está oscuro (");
      Serial.print(lightLevel);
      Serial.println(") - ENCENDIENDO LUCES");
      turnOnLights();
      lightsOn = true;
    } else if (lightLevel >= lightThreshold && !lightsOn) {
      Serial.print("Suficiente luz natural (");
      Serial.print(lightLevel);
      Serial.println(") - No se encienden las luces");
    }
    
  } else {
    digitalWrite(LED_PIN, LOW); // Apagar LED indicador
    
    if (pirState == HIGH) {
      Serial.println("Fin del movimiento");
      pirState = LOW;
    }
  }
  
  // Auto-apagado después del delay sin movimiento
  if (lightsOn && (millis() - lastMotion > lightDelay)) {
    Serial.println("Tiempo agotado sin movimiento - APAGANDO LUCES");
    turnOffLights();
    lightsOn = false;
  }
  
  // Si hay suficiente luz natural, apagar las luces artificiales
  if (lightLevel >= lightThreshold && lightsOn) {
    Serial.print("Suficiente luz natural (");
    Serial.print(lightLevel);
    Serial.println(") - APAGANDO LUCES");
    turnOffLights();
    lightsOn = false;
  }
  
  delay(100); // Pequeña pausa para estabilidad
}

void turnOnLights() {
  Serial.println("Ejecutando secuencia de encendido...");
  
  // Encender relé (para tira LED convencional)
  digitalWrite(RELAY_PIN, HIGH);
  Serial.println("Relé activado");
  
  // Encender tira NeoPixel con efecto suave
  for (int brightness = 0; brightness <= 100; brightness += 10) {
    strip.setBrightness(brightness);
    colorWipe(strip.Color(255, 255, 200), 5); // Blanco cálido
    delay(30);
  }
  Serial.println("Tira NeoPixel encendida");
}

void turnOffLights() {
  Serial.println("Ejecutando secuencia de apagado...");
  
  // Apagar con efecto suave
  for (int brightness = 100; brightness >= 0; brightness -= 15) {
    strip.setBrightness(brightness);
    strip.show();
    delay(50);
  }
  
  // Apagar completamente
  strip.clear();
  strip.show();
  digitalWrite(RELAY_PIN, LOW);
  Serial.println("Luces completamente apagadas");
}

// Función para efecto de encendido suave
void colorWipe(uint32_t color, int wait) {
  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, color);
    strip.show();
    delay(wait);
  }
}