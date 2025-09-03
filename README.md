# Sistema de Iluminación Inteligente

Este proyecto implementa un sistema de iluminación inteligente utilizando una ESP32 que combina sensores de movimiento y luz para automatizar la iluminación de manera eficiente y adaptativa.

## Características

- Detección de movimiento mediante sensor PIR
- Medición de nivel de luz ambiente mediante fotoresistor
- Control de iluminación LED mediante tira NeoPixel y relé
- Indicador LED de estado
- Apagado automático después de período de inactividad
- Modo de simulación compatible con Wokwi
- Interfaz serial para monitoreo y depuración

## Componentes Necesarios

### Hardware Principal
- ESP32 DevKit
- Sensor PIR (Pin 13)
- Fotoresistor analógico (Pin A0)
- Módulo Relé (Pin 12)
- Tira LED NeoPixel WS2812B (Pin 27)
- LED indicador (Pin 14)

### Componentes Específicos y Versiones Compatibles

#### Sensores
- **Sensor PIR**: HC-SR501 o compatible
- **Fotoresistor**: GL5516 o similar
- **Sensor de luz digital**: BH1750 (opcional, actualmente desactivado en el código)

#### Actuadores
- **Tira LED**: WS2812B NeoPixel (8 LEDs por defecto)
- **Módulo Relé**: Relé de 5V compatible con nivel lógico 3.3V
- **LED Indicador**: LED estándar de 3mm o 5mm con resistencia apropiada

## Requisitos de Software

### Entorno de Desarrollo
- PlatformIO IDE (Visual Studio Code + extensión PlatformIO)
- Visual Studio Code

### Bibliotecas Necesarias
```ini
adafruit/Adafruit INA219 @ ^1.0.6
adafruit/Adafruit NeoPixel @ ^1.1.5
adafruit/Adafruit GFX Library @ ^1.12.1
adafruit/Adafruit SSD1306 @ ^2.5.15
claws/BH1750 @ ^1.3.0
```

## Estructura del Proyecto
```
InclusiveTech/
├── src/
│   └── main.cpp          # Código principal
├── include/              # Archivos de cabecera
├── lib/                  # Bibliotecas
├── platformio.ini        # Configuración del proyecto
└── wokwi.toml           # Configuración para simulación
```

## Instalación y Configuración

1. **Preparación del Entorno**
   - Instalar Visual Studio Code
   - Instalar la extensión PlatformIO IDE
   - Clonar o descargar este repositorio

2. **Configuración del Hardware**
   - Conectar los componentes según los pines definidos en el código:
     - PIR -> Pin 13
     - Relé -> Pin 12
     - NeoPixel -> Pin 27
     - LED indicador -> Pin 14
     - Fotoresistor -> Pin A0

3. **Configuración del Software**
   ```bash
   # Abrir el proyecto en VS Code
   code InclusiveTech/
   
   # PlatformIO instalará automáticamente las dependencias
   ```

## Uso

1. **Compilación y Carga**
   - Abrir el proyecto en PlatformIO
   - Conectar la ESP32 via USB
   - Hacer clic en el botón "Upload" en la barra de PlatformIO

2. **Monitoreo**
   - Usar el Monitor Serial de PlatformIO (115200 baud)
   - Observar los mensajes de estado y debugging

3. **Ajustes**
   - El umbral de luz se puede ajustar modificando `lightThreshold`
   - El tiempo de apagado automático se ajusta en `lightDelay`
   - La cantidad de LEDs NeoPixel se configura en `NUM_PIXELS`

## Características de Funcionamiento

- **Detección de Movimiento**: El sistema detecta movimiento y activa la iluminación
- **Control de Luz**: Solo enciende si el nivel de luz ambiente está por debajo del umbral
- **Apagado Automático**: Se apaga después de 30 segundos sin detectar movimiento
- **Efectos de Iluminación**: Incluye efectos suaves de encendido/apagado
- **Monitoreo**: Muestra estado del sistema por puerto serial

## Simulación

El proyecto incluye soporte para simulación en Wokwi:
1. Abrir el proyecto en PlatformIO
2. Compilar el proyecto
3. Usar los archivos generados en `.pio/build/esp32dev/` para la simulación

## Solución de Problemas

- **LED NeoPixel no enciende**: Verificar conexión y alimentación independiente
- **Sensor PIR no detecta**: Ajustar sensibilidad y tiempo de calibración
- **Relé no conmuta**: Verificar conexión y voltaje de control

## Licencia

Este proyecto está disponible bajo licencia MIT.

## Contribuciones

Las contribuciones son bienvenidas. Por favor, crear un issue o pull request para sugerencias y mejoras.
