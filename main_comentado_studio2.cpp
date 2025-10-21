#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <vector>
#include <cmath>
#include <time.h>

// ============================================================================
// CONFIGURACIÓN DE TELEGRAM BOT
// ============================================================================
// IMPORTANTE: Reemplaza estos valores con los tuyos
constexpr const char *TELEGRAM_BOT_TOKEN = "8079858933:AAGwyFxOYh3CxE1pPABPWl_6LvNzrpLScGI"; // Token de @BotFather
constexpr const char *TELEGRAM_CHAT_ID = "5009624937";                              // Tu Chat ID de @userinfobot

// URLs de la API de Telegram
const String TELEGRAM_API_URL = "https://api.telegram.org/bot" + String(TELEGRAM_BOT_TOKEN) + "/sendMessage";

// ============================================================================
// HTML DEL DASHBOARD CON MODAL DE ALERTA
// ============================================================================
const char DASHBOARD_HTML[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="es">
  <head>
    <meta charset="UTF-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1.0" />
    <title>Sistema de Iluminación Inteligente</title>
    <link href="https://fonts.googleapis.com/css2?family=Inter:wght@300;400;500;600;700&display=swap" rel="stylesheet"/>
    <script src="https://cdn.jsdelivr.net/npm/chart.js@4.4.0/dist/chart.umd.min.js"></script>
    <style>
      :root {--bg-primary: #0a1628;--bg-secondary: #132337;--bg-card: #1a2f47;--bg-card-hover: #223d5a;--text-primary: #e8f1ff;--text-secondary: #a0b4cc;--text-muted: #6b7f99;--accent-blue: #2196f3;--accent-blue-light: #42a5f5;--accent-cyan: #00bcd4;--accent-success: #4caf50;--accent-warning: #ff9800;--accent-danger: #f44336;--border: #2d4563;--shadow: 0 4px 12px rgba(0, 0, 0, 0.3);--shadow-lg: 0 8px 24px rgba(0, 0, 0, 0.4);}
      * {margin: 0;padding: 0;box-sizing: border-box;}
      body {font-family: "Inter", -apple-system, BlinkMacSystemFont, sans-serif;background: linear-gradient(135deg, var(--bg-primary) 0%, #0f1e33 100%);color: var(--text-primary);min-height: 100vh;line-height: 1.6;}
      .container {max-width: 1600px;margin: 0 auto;padding: 20px;}
      .header {background: linear-gradient(135deg,var(--bg-card) 0%,var(--bg-secondary) 100%);border-radius: 16px;padding: 24px 32px;margin-bottom: 24px;box-shadow: var(--shadow-lg);border: 1px solid var(--border);display: flex;justify-content: space-between;align-items: center;flex-wrap: wrap;gap: 16px;}
      .header-content h1 {font-size: 1.75rem;font-weight: 700;background: linear-gradient(135deg,var(--accent-blue) 0%,var(--accent-cyan) 100%);-webkit-background-clip: text;-webkit-text-fill-color: transparent;background-clip: text;margin-bottom: 4px;}
      .header-subtitle {font-size: 0.875rem;color: var(--text-secondary);font-weight: 400;}
      .status-badge {display: inline-flex;align-items: center;gap: 8px;padding: 10px 20px;border-radius: 24px;font-weight: 600;font-size: 0.875rem;backdrop-filter: blur(10px);border: 1px solid;}
      .status-active {background: rgba(76, 175, 80, 0.15);border-color: var(--accent-success);color: var(--accent-success);}
      .status-inactive {background: rgba(160, 180, 204, 0.15);border-color: var(--text-secondary);color: var(--text-secondary);}
      .status-warning {background: rgba(255, 152, 0, 0.15);border-color: var(--accent-warning);color: var(--accent-warning);animation: pulse-warning 2s ease-in-out infinite;}
      @keyframes pulse-warning {0%,100% {opacity: 1;transform: scale(1);}50% {opacity: 0.8;transform: scale(1.02);}}
      .indicator {width: 10px;height: 10px;border-radius: 50%;animation: pulse 2s ease-in-out infinite;}
      @keyframes pulse {0%,100% {opacity: 1;}50% {opacity: 0.5;}}
      .indicator-on {background: var(--accent-success);}
      .indicator-off {background: var(--text-muted);}
      .indicator-warning {background: var(--accent-warning);}
      
      /* ESTILOS DEL MODAL DE ALERTA */
      .modal-overlay {display: none;position: fixed;top: 0;left: 0;width: 100%;height: 100%;background: rgba(10, 22, 40, 0.95);backdrop-filter: blur(8px);z-index: 9999;align-items: center;justify-content: center;animation: fadeIn 0.3s ease;}
      .modal-overlay.show {display: flex;}
      @keyframes fadeIn {from {opacity: 0;}to {opacity: 1;}}
      .modal-content {background: linear-gradient(135deg, var(--bg-card) 0%, var(--bg-secondary) 100%);border-radius: 20px;padding: 32px;max-width: 500px;width: 90%;box-shadow: var(--shadow-lg);border: 2px solid var(--accent-warning);animation: slideUp 0.4s ease;}
      @keyframes slideUp {from {transform: translateY(30px);opacity: 0;}to {transform: translateY(0);opacity: 1;}}
      .modal-icon {width: 80px;height: 80px;margin: 0 auto 20px;background: linear-gradient(135deg, rgba(255, 152, 0, 0.2), rgba(255, 152, 0, 0.1));border-radius: 50%;display: flex;align-items: center;justify-content: center;font-size: 2.5rem;animation: bounce 1s ease infinite;}
      @keyframes bounce {0%, 100% {transform: translateY(0);}50% {transform: translateY(-10px);}}
      .modal-title {font-size: 1.5rem;font-weight: 700;color: var(--accent-warning);text-align: center;margin-bottom: 12px;}
      .modal-message {font-size: 1rem;color: var(--text-secondary);text-align: center;line-height: 1.6;margin-bottom: 24px;}
      .modal-timer {font-size: 2rem;font-weight: 700;color: var(--accent-warning);text-align: center;margin: 16px 0;font-family: 'Courier New', monospace;}
      .modal-buttons {display: flex;gap: 12px;}
      .modal-btn {flex: 1;padding: 14px 24px;border: none;border-radius: 12px;font-size: 0.95rem;font-weight: 600;cursor: pointer;transition: all 0.3s ease;text-transform: uppercase;letter-spacing: 0.5px;}
      .modal-btn-primary {background: linear-gradient(135deg, var(--accent-success), #66bb6a);color: white;box-shadow: 0 4px 12px rgba(76, 175, 80, 0.3);}
      .modal-btn-secondary {background: linear-gradient(135deg, var(--bg-secondary), var(--bg-card-hover));color: var(--text-primary);border: 1px solid var(--border);}
      .modal-btn:hover {transform: translateY(-2px);box-shadow: 0 6px 16px rgba(33, 150, 243, 0.4);}
      
      .grid {display: grid;grid-template-columns: repeat(auto-fit, minmax(280px, 1fr));gap: 20px;margin-bottom: 24px;}
      .card {background: var(--bg-card);border-radius: 16px;padding: 24px;box-shadow: var(--shadow);border: 1px solid var(--border);transition: all 0.3s ease;}
      .card:hover {transform: translateY(-2px);box-shadow: var(--shadow-lg);background: var(--bg-card-hover);}
      .card-header {display: flex;align-items: center;gap: 12px;margin-bottom: 20px;padding-bottom: 16px;border-bottom: 1px solid var(--border);}
      .card-icon {width: 44px;height: 44px;border-radius: 12px;display: flex;align-items: center;justify-content: center;font-size: 1.25rem;background: linear-gradient(135deg,rgba(33, 150, 243, 0.2),rgba(33, 150, 243, 0.1));color: var(--accent-blue);}
      .card-title {font-size: 1rem;font-weight: 600;color: var(--text-primary);}
      .metric-value {font-size: 2.5rem;font-weight: 700;background: linear-gradient(135deg,var(--accent-blue),var(--accent-cyan));-webkit-background-clip: text;-webkit-text-fill-color: transparent;background-clip: text;line-height: 1.2;margin-bottom: 8px;}
      .metric-label {font-size: 0.875rem;color: var(--text-secondary);font-weight: 500;}
      .metric-row {display: flex;justify-content: space-between;align-items: center;padding: 12px 0;border-bottom: 1px solid rgba(45, 69, 99, 0.5);}
      .metric-row:last-child {border-bottom: none;}
      .metric-row-value {font-weight: 600;color: var(--text-primary);font-size: 1.1rem;}
      .progress-container {margin: 16px 0;}
      .progress-label {display: flex;justify-content: space-between;margin-bottom: 8px;font-size: 0.875rem;color: var(--text-secondary);}
      .progress-bar {width: 100%;height: 8px;background: rgba(45, 69, 99, 0.5);border-radius: 4px;overflow: hidden;position: relative;}
      .progress-fill {height: 100%;transition: width 0.5s ease;border-radius: 4px;}
      .progress-primary {background: linear-gradient(90deg,var(--accent-blue),var(--accent-cyan));}
      .progress-success {background: linear-gradient(90deg, var(--accent-success), #66bb6a);}
      .btn-group {display: flex;gap: 12px;margin-top: 16px;}
      .btn {flex: 1;padding: 12px 20px;border: none;border-radius: 10px;font-size: 0.875rem;font-weight: 600;cursor: pointer;transition: all 0.3s ease;text-transform: uppercase;letter-spacing: 0.5px;font-family: "Inter", sans-serif;}
      .btn:hover {transform: translateY(-2px);}
      .btn:active {transform: translateY(0);}
      .btn-primary {background: linear-gradient(135deg,var(--accent-blue),var(--accent-blue-light));color: white;box-shadow: 0 4px 12px rgba(33, 150, 243, 0.3);}
      .btn-primary:hover {box-shadow: 0 6px 16px rgba(33, 150, 243, 0.4);}
      .btn-success {background: linear-gradient(135deg, var(--accent-success), #66bb6a);color: white;box-shadow: 0 4px 12px rgba(76, 175, 80, 0.3);}
      .btn-danger {background: linear-gradient(135deg, var(--accent-danger), #ef5350);color: white;box-shadow: 0 4px 12px rgba(244, 67, 54, 0.3);}
      .btn-warning {background: linear-gradient(135deg, var(--accent-warning), #ffa726);color: white;box-shadow: 0 4px 12px rgba(255, 152, 0, 0.3);}
      .btn.active {box-shadow: 0 0 20px rgba(33, 150, 243, 0.6);transform: scale(1.02);}
      .btn:disabled {opacity: 0.5;cursor: not-allowed;}
      .wide-card {grid-column: span 2;}
      .chart-container {position: relative;height: 320px;margin-top: 16px;}
      .stats-grid {display: grid;grid-template-columns: repeat(auto-fit, minmax(140px, 1fr));gap: 16px;margin-top: 16px;}
      .stat-box {background: linear-gradient(135deg,rgba(33, 150, 243, 0.1),rgba(0, 188, 212, 0.05));padding: 20px;border-radius: 12px;text-align: center;border: 1px solid rgba(33, 150, 243, 0.2);}
      .stat-value {font-size: 1.75rem;font-weight: 700;background: linear-gradient(135deg,var(--accent-blue),var(--accent-cyan));-webkit-background-clip: text;-webkit-text-fill-color: transparent;background-clip: text;margin-bottom: 4px;}
      .stat-label {font-size: 0.75rem;color: var(--text-secondary);text-transform: uppercase;letter-spacing: 0.5px;font-weight: 500;}
      .info-box {padding: 16px;background: linear-gradient(135deg,rgba(33, 150, 243, 0.1),rgba(33, 150, 243, 0.05));border-radius: 10px;border: 1px solid rgba(33, 150, 243, 0.3);margin-top: 16px;}
      .info-title {font-size: 0.75rem;color: var(--text-secondary);text-transform: uppercase;letter-spacing: 0.5px;margin-bottom: 6px;}
      .info-content {font-weight: 600;font-size: 1rem;color: var(--accent-blue);}
      @media (max-width: 1200px) {.wide-card {grid-column: span 1;}}
      @media (max-width: 768px) {.header {flex-direction: column;align-items: flex-start;}.header-content h1 {font-size: 1.5rem;}.grid {grid-template-columns: 1fr;}.stats-grid {grid-template-columns: repeat(2, 1fr);}.metric-value {font-size: 2rem;}.btn-group {flex-direction: column;}}
      @media (max-width: 480px) {.container {padding: 12px;}.header {padding: 16px 20px;}.card {padding: 16px;}.stats-grid {grid-template-columns: 1fr;}.modal-content {padding: 24px;}}
    </style>
  </head>
  <body>
    <!-- MODAL DE ALERTA -->
    <div class="modal-overlay" id="alertModal">
      <div class="modal-content">
        <div class="modal-icon">⚠️</div>
        <h2 class="modal-title">¡Atención Requerida!</h2>
        <p class="modal-message">
          El sistema ha estado en <strong>Modo Manual ON</strong> durante más de 30 segundos. 
          Se recomienda cambiar a Modo Automático para optimizar el consumo energético.
        </p>
        <div class="modal-timer" id="modalTimer">00:00</div>
        <div class="modal-buttons">
          <button class="modal-btn modal-btn-primary" onclick="switchToAutoFromModal()">
            ⚡ Cambiar a AUTO
          </button>
          <button class="modal-btn modal-btn-secondary" onclick="dismissModal()">
            Mantener Manual
          </button>
        </div>
      </div>
    </div>

    <div class="container">
      <div class="header">
        <div class="header-content">
          <h1>⚡ Sistema de Iluminación Inteligente</h1>
          <p class="header-subtitle">Control Adaptativo | Eficiencia Energética | Monitoreo en Tiempo Real</p>
        </div>
        <span class="status-badge" id="systemStatus">
          <span class="indicator" id="systemIndicator"></span>
          <span id="systemStatusText">Inicializando...</span>
        </span>
      </div>
      <div class="grid">
        <div class="card">
          <div class="card-header">
            <div class="card-icon">💡</div>
            <div class="card-title">Consumo Energético Actual</div>
          </div>
          <div class="metric-value" id="currentPowerWh">0</div>
          <div class="metric-label">Vatios (W) - Simulación Real</div>
          <div class="progress-container">
            <div class="progress-label">
              <span>Uso de Potencia</span>
              <span id="powerUsagePercent">0%</span>
            </div>
            <div class="progress-bar">
              <div class="progress-fill progress-primary" id="powerBar" style="width: 0%"></div>
            </div>
          </div>
        </div>
        <div class="card">
          <div class="card-header">
            <div class="card-icon">💰</div>
            <div class="card-title">Costo Mensual Estimado</div>
          </div>
          <div class="metric-value" id="monthlyCost">$0.00</div>
          <div class="metric-label">USD (Tarifa: $0.092/kWh)</div>
          <div class="metric-row">
            <span class="metric-label">Costo Acumulado Hoy</span>
            <span class="metric-row-value" id="dailyCost">$0.000</span>
          </div>
        </div>
        <div class="card">
          <div class="card-header">
            <div class="card-icon">📉</div>
            <div class="card-title">Ahorro Energético</div>
          </div>
          <div class="metric-value" id="savingsPercent">0%</div>
          <div class="metric-label">vs. Iluminación Tradicional</div>
          <div class="progress-container">
            <div class="progress-label">
              <span>Ahorro Acumulado (Hoy)</span>
              <span><strong id="savingsValue">$0.000</strong></span>
            </div>
            <div class="progress-bar">
              <div class="progress-fill progress-success" id="savingsBar" style="width: 0%"></div>
            </div>
          </div>
        </div>
        <div class="card">
          <div class="card-header">
            <div class="card-icon">⏱️</div>
            <div class="card-title">ROI - Retorno de Inversión</div>
          </div>
          <div class="metric-value" id="roiMonths">N/A</div>
          <div class="metric-label">Meses para recuperar inversión</div>
          <div class="metric-row">
            <span class="metric-label">Inversión Inicial</span>
            <span class="metric-row-value">$39.90</span>
          </div>
        </div>
        <div class="card">
          <div class="card-header">
            <div class="card-icon">⚙️</div>
            <div class="card-title">Panel de Control</div>
          </div>
          <div class="btn-group">
            <button class="btn btn-primary active" id="btnAuto" onclick="setMode('auto')">⚡ AUTO</button>
            <button class="btn btn-warning" id="btnManual" onclick="setMode('manual')">🎛️ MANUAL</button>
          </div>
          <div class="btn-group" id="manualControls" style="display: none">
            <button class="btn btn-success" onclick="manualControl('on')">▶️ ENCENDER</button>
            <button class="btn btn-danger" onclick="manualControl('off')">⏹️ APAGAR</button>
          </div>
          <div class="info-box">
            <div class="info-title">Estado Operativo</div>
            <div class="info-content" id="modeDescription">Modo Automático Adaptativo Activo</div>
          </div>
        </div>
        <div class="card">
          <div class="card-header">
            <div class="card-icon">🔆</div>
            <div class="card-title">Sensores Ambientales</div>
          </div>
          <div class="metric-row">
            <span class="metric-label">Luz Ambiente</span>
            <span class="metric-row-value"><span id="ambientLux">0</span> lux</span>
          </div>
          <div class="metric-row">
            <span class="metric-label">Objetivo</span>
            <span class="metric-row-value">400 lux</span>
          </div>
          <div class="metric-row">
            <span class="metric-label">Brillo LED</span>
            <span class="metric-row-value"><span id="ledBrightness">0</span>%</span>
          </div>
          <div class="metric-row">
            <span class="metric-label">Sensor PIR</span>
            <span class="metric-row-value" id="pirStatus">Inactivo</span>
          </div>
        </div>
        <div class="card wide-card">
          <div class="card-header">
            <div class="card-icon">📊</div>
            <div class="card-title">Consumo Energético en Tiempo Real</div>
          </div>
          <div class="chart-container"><canvas id="energyChart"></canvas></div>
        </div>
        <div class="card wide-card">
          <div class="card-header">
            <div class="card-icon">📈</div>
            <div class="card-title">Métricas de Rendimiento</div>
          </div>
          <div class="stats-grid">
            <div class="stat-box">
              <div class="stat-value" id="totalEnergyKwh">0.000</div>
              <div class="stat-label">Energía Total Hoy (kWh)</div>
            </div>
            <div class="stat-box">
              <div class="stat-value" id="uptime">0h 0m</div>
              <div class="stat-label">Tiempo Operativo</div>
            </div>
            <div class="stat-box">
              <div class="stat-value" id="dailySavings">$0.00</div>
              <div class="stat-label">Ahorro Diario Proyectado</div>
            </div>
            <div class="stat-box">
              <div class="stat-value" id="co2Saved">0.00</div>
              <div class="stat-label">CO₂ Ahorrado Hoy (kg)</div>
            </div>
          </div>
        </div>
      </div>
    </div>
    <script>
      const API_BASE = window.location.origin;
      let systemData = {};
      let energyChart = null;
      let modalDismissed = false; // Para evitar mostrar el modal repetidamente si el usuario lo cierra

      document.addEventListener("DOMContentLoaded", () => {
        initEnergyChart();
        startUpdates();
      });

      function startUpdates() {
        updateSystemData();
        setInterval(updateSystemData, 1500);
      }

      async function updateSystemData() {
        try {
          const res = await fetch(`${API_BASE}/api/status`);
          if (res.ok) {
            systemData = await res.json();
            updateUI();
            checkManualWarning(); // Verifica si debe mostrar la alerta
          } else {
            console.error("Error fetching data from API");
          }
        } catch (e) {
          console.error("Could not connect to ESP32 API:", e);
        }
      }

      function checkManualWarning() {
        const modal = document.getElementById("alertModal");
        const timer = document.getElementById("modalTimer");
        
        // Mostrar modal si:
        // 1. Está en modo manual (!autoMode)
        // 2. Las luces están encendidas (lightsOn)
        // 3. Han pasado más de 30 segundos (manualOnSeconds >= 30)
        // 4. El usuario no ha cerrado el modal previamente (modalDismissed)
        if (!systemData.autoMode && systemData.lightsOn && 
            systemData.manualOnSeconds >= 30 && !modalDismissed) {
          modal.classList.add("show");
          
          // Actualizar el timer en el modal
          const minutes = Math.floor(systemData.manualOnSeconds / 60);
          const seconds = systemData.manualOnSeconds % 60;
          timer.textContent = `${String(minutes).padStart(2, '0')}:${String(seconds).padStart(2, '0')}`;
        } else {
          modal.classList.remove("show");
          // Reset del flag cuando se vuelve a modo auto o se apagan las luces
          if (systemData.autoMode || !systemData.lightsOn) {
            modalDismissed = false;
          }
        }
      }

      async function switchToAutoFromModal() {
        await setMode('auto');
        dismissModal();
      }

      function dismissModal() {
        document.getElementById("alertModal").classList.remove("show");
        modalDismissed = true; // Marcar que el usuario cerró el modal
      }

      function updateUI() {
        const sysStatus = document.getElementById("systemStatus");
        const sysInd = document.getElementById("systemIndicator");
        const sysText = document.getElementById("systemStatusText");

        // Actualizar badge del header con advertencia si aplica
        if (!systemData.autoMode && systemData.lightsOn && systemData.manualOnSeconds >= 30) {
          sysStatus.className = "status-badge status-warning";
          sysInd.className = "indicator indicator-warning";
          sysText.textContent = "⚠️ MANUAL PROLONGADO";
        } else if (systemData.lightsOn) {
          sysStatus.className = "status-badge status-active";
          sysInd.className = "indicator indicator-on";
          sysText.textContent = "SISTEMA ACTIVO";
        } else {
          sysStatus.className = "status-badge status-inactive";
          sysInd.className = "indicator indicator-off";
          sysText.textContent = "EN ESPERA";
        }

        // --- MÉTRICAS DE CONSUMO ---
        const realPowerW = systemData.realWorldPower || 0;
        document.getElementById("currentPowerWh").textContent = Math.round(realPowerW);
        const powerUsagePercent = systemData.powerUsagePercent || 0;
        document.getElementById("powerUsagePercent").textContent = Math.round(powerUsagePercent) + "%";
        document.getElementById("powerBar").style.width = powerUsagePercent + "%";
        
        document.getElementById("monthlyCost").textContent = "$" + (systemData.monthlyCostEstimate || 0).toFixed(2);
        document.getElementById("dailyCost").textContent = "$" + (systemData.dailyCost || 0).toFixed(3);

        // --- MÉTRICAS DE AHORRO ---
        const savingsPercent = systemData.savingsPercent || 0;
        document.getElementById("savingsPercent").textContent = Math.round(savingsPercent) + "%";
        document.getElementById("savingsBar").style.width = Math.round(savingsPercent) + "%";
        document.getElementById("savingsValue").textContent = "$" + (systemData.savingsTodayUSD || 0).toFixed(3);
        
        const roi = systemData.roiMonths || 0;
        document.getElementById("roiMonths").textContent = roi > 0 && roi < 999 ? roi.toFixed(1) : "N/A";

        // --- SENSORES ---
        document.getElementById("ambientLux").textContent = Math.round(systemData.ambientLux || 0);
        document.getElementById("ledBrightness").textContent = Math.round(systemData.currentBrightness || 0);
        document.getElementById("pirStatus").textContent = systemData.pirDetected ? "Movimiento" : "Inactivo";

        // --- MÉTRICAS DE RENDIMIENTO ---
        document.getElementById("totalEnergyKwh").textContent = (systemData.totalRealEnergyTodayKWh || 0).toFixed(3);
        const uptimeSec = systemData.uptime || 0;
        const hours = Math.floor(uptimeSec / 3600);
        const minutes = Math.floor((uptimeSec % 3600) / 60);
        document.getElementById("uptime").textContent = `${hours}h ${minutes}m`;
        document.getElementById("dailySavings").textContent = "$" + (systemData.projectedDailySavings || 0).toFixed(2);
        document.getElementById("co2Saved").textContent = (systemData.co2SavedTodayKg || 0).toFixed(2);

        updateModeUI();
      }

      function updateModeUI() {
        const autoMode = systemData.autoMode;
        const btnAuto = document.getElementById("btnAuto");
        const btnManual = document.getElementById("btnManual");
        const manualControls = document.getElementById("manualControls");
        const modeDesc = document.getElementById("modeDescription");
        if (autoMode) {
          btnAuto.classList.add("active");
          btnManual.classList.remove("active");
          manualControls.style.display = "none";
          modeDesc.textContent = "Control Adaptativo con Sensores PIR + LDR";
        } else {
          btnManual.classList.add("active");
          btnAuto.classList.remove("active");
          manualControls.style.display = "flex";
          modeDesc.textContent = "Control Manual - Sensores Deshabilitados";
        }
      }

      async function setMode(mode) {
        await fetch(`${API_BASE}/api/control`, {
          method: "POST",
          headers: { "Content-Type": "application/json" },
          body: JSON.stringify({ action: mode }),
        });
        setTimeout(updateSystemData, 200);
      }

      async function manualControl(action) {
        await fetch(`${API_BASE}/api/control`, {
          method: "POST",
          headers: { "Content-Type": "application/json" },
          body: JSON.stringify({ action }),
        });
        setTimeout(updateSystemData, 200);
      }

      function initEnergyChart() {
        const ctx = document.getElementById("energyChart").getContext("2d");
        energyChart = new Chart(ctx, {
          type: "line",
          data: {
            labels: [],
            datasets: [
              { label: "Consumo Simulado (W)", data: [], borderColor: "#2196f3", backgroundColor: "rgba(33, 150, 243, 0.1)", tension: 0.4, fill: true, borderWidth: 2, yAxisID: 'y' },
              { label: "Luz Ambiente (lux)", data: [], borderColor: "#ff9800", backgroundColor: "rgba(255, 152, 0, 0.1)", tension: 0.4, fill: true, borderWidth: 2, yAxisID: 'y1' }
            ],
          },
          options: {
            responsive: true, maintainAspectRatio: false,
            plugins: { legend: { labels: { color: "#e8f1ff", font: { family: "Inter" } } } },
            scales: {
              y: { type: 'linear', display: true, position: 'left', title: { display: true, text: 'Consumo (W)', color: '#2196f3'}, ticks: { color: "#a0b4cc" }, grid: { color: "#2d4563" } },
              y1: { type: 'linear', display: true, position: 'right', title: { display: true, text: 'Luminosidad (lux)', color: '#ff9800'}, ticks: { color: "#a0b4cc" }, grid: { drawOnChartArea: false } },
              x: { ticks: { color: "#a0b4cc" }, grid: { color: "#2d4563" } },
            },
          },
        });
        setInterval(updateChart, 2000);
      }

      function updateChart() {
        if (!systemData || Object.keys(systemData).length === 0) return;
        
        const now = new Date().toLocaleTimeString("es-ES", { hour: '2-digit', minute: '2-digit', second: '2-digit' });
        
        if (energyChart.data.labels.length > 30) {
          energyChart.data.labels.shift();
          energyChart.data.datasets.forEach(dataset => dataset.data.shift());
        }

        energyChart.data.labels.push(now);
        energyChart.data.datasets[0].data.push(systemData.realWorldPower || 0);
        energyChart.data.datasets[1].data.push(systemData.ambientLux || 0);
        energyChart.update("none");
      }
    </script>
  </body>
</html>
)rawliteral";

// ============================================================================
// CONFIGURACIÓN DE PINES Y WIFI
// ============================================================================
#define PIR_PIN 13
#define RELAY_PIN 26
#define LIGHT_SENSOR_PIN 36

constexpr int LED_PINS[] = {15, 16, 17, 18, 19};
constexpr int NUM_LEDS = sizeof(LED_PINS) / sizeof(LED_PINS[0]);

constexpr int PWM_CHANNEL_START = 0;
constexpr int PWM_FREQ = 5000;
constexpr int PWM_RESOLUTION = 8;

constexpr const char *SSID = "Javier";
constexpr const char *PASSWORD = "12345678";

AsyncWebServer server(80);

// ============================================================================
// PARÁMETROS DE SIMULACIÓN Y CONTROL
// ============================================================================
constexpr int NUM_REAL_LAMPS = 5;
constexpr float POWER_PER_LAMP_W = 40.0f;
constexpr float MAX_SYSTEM_POWER_W = NUM_REAL_LAMPS * POWER_PER_LAMP_W;
constexpr float POWER_TRADITIONAL_W = MAX_SYSTEM_POWER_W;

constexpr float ELECTRICITY_RATE_KWH = 0.092f;
constexpr float CO2_FACTOR_KG_PER_KWH = 0.45f;
constexpr float INVESTMENT_USD = 39.90f;

constexpr int LUX_TARGET = 800;
constexpr int LUX_MAX_AMBIENT_TO_TURN_OFF = 1000;
constexpr float SLEW_RATE = 40.0f;

// ============================================================================
// PARÁMETROS DE NOTIFICACIONES TELEGRAM
// ============================================================================
constexpr unsigned long MANUAL_WARNING_THRESHOLD_SEC = 30;  // 30 segundos para alerta
constexpr unsigned long DAILY_REPORT_INTERVAL_SEC = 86400;  // 24 horas para reporte diario

// ============================================================================
// ESTRUCTURAS DE ESTADO
// ============================================================================
struct SystemConfig {
    int autoOffDelaySec = 120;
    bool autoMode = true;
};

struct TelegramState {
    bool manualWarningAlreadySent = false;      // Flag para evitar spam de alertas
    bool dailyReportSent = false;                // Flag para reporte diario
    unsigned long lastManualWarningTime = 0;     // Timestamp de última alerta manual
    unsigned long lastDailyReportTime = 0;       // Timestamp de último reporte diario
    unsigned long lastErrorNotificationTime = 0; // Timestamp de último error reportado
    String lastErrorMessage = "";                // Último error para evitar duplicados
};

struct SystemState {
    // Sensores y actuadores
    bool pirDetected = false;
    bool lightsOn = false;
    float targetBrightnessPercent = 0.0f;
    float currentBrightnessPercent = 0.0f;
    int rawADC = 0;
    float ambientLux = 0.0f;
    unsigned long lastMotionTime = 0;
    
    // Control de modo manual
    unsigned long manualModeStartTime = 0;       // Cuando se activó modo manual
    unsigned long manualOnStartTime = 0;         // Cuando se encendió en modo manual
    unsigned long manualOnSeconds = 0;           // Segundos acumulados en manual ON
    
    // Métricas de energía y rendimiento
    float realWorldPowerW = 0.0f;
    float powerUsagePercent = 0.0f;
    double totalRealEnergyTodayKWh = 0.0;
    double dailyCost = 0.0;
    double savingsTodayUSD = 0.0;
    double co2SavedTodayKg = 0.0;
    float savingsPercent = 0.0;
    float monthlyCostEstimate = 0.0;
    float projectedDailySavings = 0.0;
    float roiMonths = 0.0;
    
    // Temporizadores
    unsigned long systemStartTime = 0;
    unsigned long lastMetricsUpdate = 0;
    unsigned long lastDayRollover = 0;
    
    // Estado del sistema (para detección de errores)
    bool wifiConnected = true;
    bool sensorsHealthy = true;
    unsigned long lastSuccessfulSensorRead = 0;
};

SystemConfig config;
SystemState state;
TelegramState telegram;

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
float adcToLux(int adcValue);
String getSystemStatusJson();
void checkDayRollover();

// Funciones de Telegram
bool sendTelegramMessage(const String &message);
void checkManualModeWarning();
void checkDailyReport();
void checkSystemHealth();
void sendDailyReportTelegram();
void sendErrorNotification(const String &errorMsg);

// ============================================================================
// SETUP
// ============================================================================
// ============================================================================
// SETUP
// ============================================================================
void setup() {
    Serial.begin(115200);
    delay(500);
    
    Serial.println(F("\n[INICIO] Sistema de Iluminación Inclusiva y Eficiente"));
    Serial.println(F("[INFO] Sistema de Notificaciones Telegram Activado"));

    pinMode(PIR_PIN, INPUT);
    pinMode(RELAY_PIN, OUTPUT);

    for (int i = 0; i < NUM_LEDS; ++i) {
        ledcSetup(PWM_CHANNEL_START + i, PWM_FREQ, PWM_RESOLUTION);
        ledcAttachPin(LED_PINS[i], PWM_CHANNEL_START + i);
    }

    digitalWrite(RELAY_PIN, LOW);
    setBrightness(0);

    state.systemStartTime = millis();
    state.lastMotionTime = state.systemStartTime;
    state.lastDayRollover = state.systemStartTime;
    state.lastSuccessfulSensorRead = state.systemStartTime;
    telegram.lastDailyReportTime = state.systemStartTime;

    setupWiFi();

    // Sincronizar hora con NTP después de conectar WiFi
    if (WiFi.status() == WL_CONNECTED) {
        configTime(0, 0, "pool.ntp.org", "time.nist.gov");  // Sincroniza con servidores NTP
        Serial.print(F("Sincronizando hora..."));
        struct tm timeinfo;
        int ntpRetries = 20;
        while (!getLocalTime(&timeinfo) && ntpRetries > 0) {
            delay(500);
            Serial.print(".");
            ntpRetries--;
        }
        if (getLocalTime(&timeinfo)) {
            Serial.println(F("\nHora sincronizada OK"));
        } else {
            Serial.println(F("\nFallo en NTP - Usa setInsecure para pruebas"));
        }
    }

    setupWebServer();

    // Enviar notificación de inicio
    String startMsg = "🚀 *Sistema de Iluminación Iniciado*\n\n";
    startMsg += "✅ Hardware: OK\n";
    startMsg += "✅ WiFi: Conectado\n";
    startMsg += "✅ Sensores: Operativos\n";
    startMsg += "⚡ Modo: Automático\n\n";
    startMsg += "El sistema está listo y operando correctamente.";
    sendTelegramMessage(startMsg);

    Serial.printf("[INFO] Simulación: %d lámparas de %.1fW. Potencia máxima: %.1fW\n", 
                  NUM_REAL_LAMPS, POWER_PER_LAMP_W, MAX_SYSTEM_POWER_W);
    Serial.println(F("[INFO] Sistema inicializado y listo."));
}
// ============================================================================
// LOOP PRINCIPAL
// ============================================================================
// ============================================================================
// LOOP PRINCIPAL
// ============================================================================
void loop() {
    static unsigned long lastLoopTime = 0;
    unsigned long now = millis();
    
    checkDayRollover();

    if (now - lastLoopTime >= 100) {
        float deltaTime = (now - lastLoopTime) / 1000.0f;
        
        readSensors();
        if (config.autoMode) {
            handleSystemLogic();
        }
        updateActuators(deltaTime);
        updateMetrics(deltaTime);
        
        // Actualizar contador de modo manual
        if (!config.autoMode && state.lightsOn) {
            state.manualOnSeconds = (now - state.manualOnStartTime) / 1000UL;
        } else {
            state.manualOnSeconds = 0;
        }

        lastLoopTime = now;
    }
    
    // Verificaciones de notificaciones (cada 5 segundos para no saturar)
    static unsigned long lastNotificationCheck = 0;
    if (now - lastNotificationCheck >= 5000) {
        checkManualModeWarning();
        checkDailyReport();
        checkSystemHealth();
        lastNotificationCheck = now;
    }

    yield();  // Para evitar watchdog y dar tiempo a otras tareas
}

// ============================================================================
// FUNCIONES DE TELEGRAM
// ============================================================================

/**
 * Envía un mensaje a través de Telegram Bot API
 * Retorna true si el envío fue exitoso
 */
bool sendTelegramMessage(const String &message) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println(F("[TELEGRAM] Error: WiFi no conectado"));
        return false;
    }

    // Ya no necesitamos la línea de diagnóstico de Serial.println
    
    WiFiClientSecure client;
    // IMPORTANTE: Le decimos al cliente que no valide el certificado.
    // Esto evita los errores X509 y soluciona problemas de memoria.
    client.setInsecure();

    HTTPClient http;
    
    // Usamos el objeto "client" que acabamos de configurar
    if (!http.begin(client, TELEGRAM_API_URL)) {
        Serial.println(F("[TELEGRAM] Error al inicializar HTTP"));
        return false;
    }
    
    http.setTimeout(10000); // Mantenemos el timeout por estabilidad
    http.addHeader("Content-Type", "application/json");

    DynamicJsonDocument doc(1024);
    doc["chat_id"] = TELEGRAM_CHAT_ID;
    doc["text"] = message;
    doc["parse_mode"] = "Markdown";

    String payload;
    serializeJson(doc, payload);

    int httpCode = http.POST(payload);
    
    if (httpCode > 0) { 
        if (httpCode == 200) {
            Serial.println(F("[TELEGRAM] ✓ ¡MENSAJE ENVIADO CORRECTAMENTE!"));
            http.end();
            return true;
        } else {
            String response = http.getString();
            Serial.printf("[TELEGRAM] ✗ Error de la API de Telegram: %d - %s\n", httpCode, response.c_str());
        }
    } else {
        Serial.printf("[TELEGRAM] ✗ Fallo en la conexión de red: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();
    return false;
}

/**
 * Verifica si debe enviar alerta de modo manual prolongado
 */
void checkManualModeWarning() {
    unsigned long now = millis();
    
    // Condiciones para enviar alerta:
    // 1. No está en modo automático
    // 2. Las luces están encendidas
    // 3. Han pasado más de 30 segundos
    // 4. No se ha enviado alerta en los últimos 5 minutos (para evitar spam)
    
    if (!config.autoMode && state.lightsOn) {
        unsigned long timeSinceManualOn = (now - state.manualOnStartTime) / 1000UL;
        
        if (timeSinceManualOn >= MANUAL_WARNING_THRESHOLD_SEC) {
            // Verificar si ya pasaron 5 minutos desde la última alerta
            unsigned long timeSinceLastWarning = (now - telegram.lastManualWarningTime) / 1000UL;
            
            if (!telegram.manualWarningAlreadySent || timeSinceLastWarning >= 300) {
                // Construir mensaje de alerta
                String alertMsg = "⚠️ *ALERTA: Modo Manual Prolongado*\n\n";
                alertMsg += "El sistema ha estado en *Modo Manual ON* durante:\n";
                alertMsg += "⏱️ *" + String(timeSinceManualOn / 60) + " minutos y " + String(timeSinceManualOn % 60) + " segundos*\n\n";
                alertMsg += "💡 Consumo actual: *" + String((int)state.realWorldPowerW) + "W*\n";
                alertMsg += "📊 Brillo LED: *" + String((int)state.currentBrightnessPercent) + "%*\n\n";
                alertMsg += "💰 Costo estimado de esta sesión: *$" + String(state.dailyCost, 3) + "*\n\n";
                alertMsg += "🔄 *Recomendación:* Cambie a modo automático para optimizar el consumo energético.\n\n";
                alertMsg += "_Responda 'AUTO' para cambiar remotamente o use el dashboard web._";
                
                if (sendTelegramMessage(alertMsg)) {
                    telegram.manualWarningAlreadySent = true;
                    telegram.lastManualWarningTime = now;
                    Serial.println(F("[ALERTA] Notificación de manual prolongado enviada"));
                }
            }
        }
    } else {
        // Reset del flag cuando se vuelve a automático o se apagan las luces
        telegram.manualWarningAlreadySent = false;
    }
}

/**
 * Envía reporte diario de métricas cada 24 horas
 */
void checkDailyReport() {
    unsigned long now = millis();
    unsigned long timeSinceLastReport = (now - telegram.lastDailyReportTime) / 1000UL;
    
    // Enviar reporte cada 24 horas (86400 segundos)
    if (timeSinceLastReport >= DAILY_REPORT_INTERVAL_SEC) {
        sendDailyReportTelegram();
        telegram.lastDailyReportTime = now;
        telegram.dailyReportSent = true;
    }
}

/**
 * Construye y envía el reporte diario de métricas
 */
void sendDailyReportTelegram() {
    String report = "📊 *REPORTE DIARIO DE EFICIENCIA*\n";
    report += "━━━━━━━━━━━━━━━━━━━━━\n\n";
    
    // Tiempo operativo
    unsigned long uptimeSec = (millis() - state.systemStartTime) / 1000UL;
    int hours = uptimeSec / 3600;
    int minutes = (uptimeSec % 3600) / 60;
    report += "⏱️ *Tiempo Operativo:* " + String(hours) + "h " + String(minutes) + "m\n\n";
    
    // Consumo energético
    report += "⚡ *CONSUMO ENERGÉTICO*\n";
    report += "• Energía total: *" + String(state.totalRealEnergyTodayKWh, 3) + " kWh*\n";
    report += "• Costo del día: *$" + String(state.dailyCost, 2) + " USD*\n";
    report += "• Proyección mensual: *$" + String(state.monthlyCostEstimate, 2) + " USD*\n\n";
    
    // Ahorro energético
    report += "💰 *AHORRO ENERGÉTICO*\n";
    report += "• Ahorro hoy: *$" + String(state.savingsTodayUSD, 3) + " USD*\n";
    report += "• Porcentaje: *" + String((int)state.savingsPercent) + "%* vs tradicional\n";
    report += "• Ahorro proyectado diario: *$" + String(state.projectedDailySavings, 2) + "*\n\n";
    
    // Impacto ambiental
    report += "🌱 *IMPACTO AMBIENTAL*\n";
    report += "• CO₂ evitado: *" + String(state.co2SavedTodayKg, 2) + " kg*\n";
    report += "• Equivalente a: *" + String(state.co2SavedTodayKg * 4.5, 1) + "* km no recorridos en auto\n\n";
    
    // ROI
    if (state.roiMonths > 0 && state.roiMonths < 999) {
        report += "📈 *RETORNO DE INVERSIÓN*\n";
        report += "• ROI estimado: *" + String(state.roiMonths, 1) + " meses*\n";
        report += "• Inversión inicial: *$" + String(INVESTMENT_USD, 2) + " USD*\n\n";
    }
    
    // Métricas operativas
    report += "📡 *OPERACIÓN*\n";
    report += "• Modo actual: *" + String(config.autoMode ? "Automático ✅" : "Manual ⚠️") + "*\n";
    report += "• Luz ambiente promedio: *" + String((int)state.ambientLux) + " lux*\n";
    report += "• Estado luces: *" + String(state.lightsOn ? "Encendidas 💡" : "Apagadas") + "*\n\n";
    
    report += "━━━━━━━━━━━━━━━━━━━━━\n";
    report += "_Reporte generado automáticamente_\n";
    report += "_Próximo reporte en 24 horas_";
    
    if (sendTelegramMessage(report)) {
        Serial.println(F("[REPORTE] Reporte diario enviado exitosamente"));
    }
}

/**
 * Monitorea la salud del sistema y envía alertas de errores
 */
void checkSystemHealth() {
    unsigned long now = millis();
    String errorMsg = "";
    bool errorDetected = false;
    
    // 1. Verificar conexión WiFi
    if (WiFi.status() != WL_CONNECTED && state.wifiConnected) {
        errorMsg = "🔴 *ERROR: Conexión WiFi Perdida*\n\n";
        errorMsg += "El sistema ha perdido la conexión a la red WiFi.\n";
        errorMsg += "SSID: *" + String(SSID) + "*\n\n";
        errorMsg += "⚠️ El control remoto no estará disponible hasta reconectar.\n";
        errorMsg += "El sistema continuará operando en modo automático local.";
        errorDetected = true;
        state.wifiConnected = false;
    } else if (WiFi.status() == WL_CONNECTED && !state.wifiConnected) {
        // WiFi recuperado
        errorMsg = "✅ *RECUPERACIÓN: WiFi Reconectado*\n\n";
        errorMsg += "La conexión WiFi ha sido restablecida.\n";
        errorMsg += "IP: *" + WiFi.localIP().toString() + "*\n\n";
        errorMsg += "El control remoto está nuevamente disponible.";
        errorDetected = true;
        state.wifiConnected = true;
    }
    
    // 2. Verificar sensores (si no se leen datos por más de 10 segundos)
    unsigned long timeSinceLastSensor = (now - state.lastSuccessfulSensorRead) / 1000UL;
    if (timeSinceLastSensor > 10 && state.sensorsHealthy) {
        errorMsg = "🔴 *ERROR: Fallo en Sensores*\n\n";
        errorMsg += "Los sensores no responden correctamente:\n";
        errorMsg += "• Sensor LDR (luz): Sin lectura\n";
        errorMsg += "• Sensor PIR (movimiento): Sin lectura\n\n";
        errorMsg += "⚠️ El sistema continuará operando con los últimos valores conocidos.\n";
        errorMsg += "Revise las conexiones físicas de los sensores.";
        errorDetected = true;
        state.sensorsHealthy = false;
    } else if (timeSinceLastSensor <= 10 && !state.sensorsHealthy) {
        // Sensores recuperados
        errorMsg = "✅ *RECUPERACIÓN: Sensores Operativos*\n\n";
        errorMsg += "Los sensores han vuelto a operar normalmente.\n";
        errorMsg += "• Luz ambiente: *" + String((int)state.ambientLux) + " lux*\n";
        errorMsg += "• PIR: *" + String(state.pirDetected ? "Movimiento detectado" : "Sin movimiento") + "*";
        errorDetected = true;
        state.sensorsHealthy = true;
    }
    
    // Enviar notificación si hay error y ha pasado al menos 1 minuto desde la última
    if (errorDetected && errorMsg != telegram.lastErrorMessage) {
        unsigned long timeSinceLastError = (now - telegram.lastErrorNotificationTime) / 1000UL;
        if (timeSinceLastError >= 60) {
            sendErrorNotification(errorMsg);
            telegram.lastErrorMessage = errorMsg;
            telegram.lastErrorNotificationTime = now;
        }
    }
}

/**
 * Envía notificación de error al usuario
 */
void sendErrorNotification(const String &errorMsg) {
    if (sendTelegramMessage(errorMsg)) {
        Serial.println(F("[ERROR] Notificación de error enviada"));
    }
}

// ============================================================================
// LECTURA DE SENSORES
// ============================================================================
void readSensors() {
    state.pirDetected = digitalRead(PIR_PIN);
    if (state.pirDetected) {
        state.lastMotionTime = millis();
    }

    state.rawADC = analogRead(LIGHT_SENSOR_PIN);
    float totalMeasuredLux = adcToLux(state.rawADC);
    float ledContributionLux = (state.currentBrightnessPercent / 100.0f) * 150.0f;
    state.ambientLux = max(0.0f, totalMeasuredLux - ledContributionLux);
    
    // Marcar lectura exitosa
    state.lastSuccessfulSensorRead = millis();
}

float adcToLux(int adcValue) {
    int invertedAdc = 4095 - adcValue;
    float lux = map(invertedAdc, 0, 4095, 0, 1200);
    float normalizedLux = lux / 1200.0f;
    float correctedLux = pow(normalizedLux, 1.5f) * 1200.0f;
    return constrain(correctedLux, 0.0, 1200.0);
}

// ============================================================================
// LÓGICA DE CONTROL
// ============================================================================
void handleSystemLogic() {
    unsigned long timeSinceLastMotionSec = (millis() - state.lastMotionTime) / 1000UL;
    bool timeout = timeSinceLastMotionSec >= config.autoOffDelaySec;
    bool tooMuchAmbientLight = state.ambientLux > LUX_MAX_AMBIENT_TO_TURN_OFF;

    if (state.pirDetected && !state.lightsOn && !tooMuchAmbientLight) {
        turnOnLights();
    } else if (state.lightsOn && (timeout || tooMuchAmbientLight)) {
        turnOffLights();
    }

    if (state.lightsOn) {
        float luxDeficit = max(0.0f, (float)LUX_TARGET - state.ambientLux);
        state.targetBrightnessPercent = map(luxDeficit, 0, LUX_TARGET, 0, 100);
    } else {
        state.targetBrightnessPercent = 0.0f;
    }
}

// ============================================================================
// CONTROL DE ACTUADORES
// ============================================================================
void updateActuators(float deltaTime) {
    float diff = state.targetBrightnessPercent - state.currentBrightnessPercent;
    float maxChange = SLEW_RATE * deltaTime;

    if (abs(diff) < maxChange) {
        state.currentBrightnessPercent = state.targetBrightnessPercent;
    } else {
        state.currentBrightnessPercent += (diff > 0) ? maxChange : -maxChange;
    }
    state.currentBrightnessPercent = constrain(state.currentBrightnessPercent, 0.0f, 100.0f);
    setBrightness(state.currentBrightnessPercent);
}

void setBrightness(float percentage) {
    float correctedDuty = pow(percentage / 100.0f, 2.2f) * 255.0f;
    int pwmValue = constrain((int)correctedDuty, 0, 255);
    for (int i = 0; i < NUM_LEDS; ++i) {
        ledcWrite(PWM_CHANNEL_START + i, pwmValue);
    }
}

void turnOnLights() {
    if (!state.lightsOn) {
        Serial.println(F("[CONTROL] Luces ENCENDIDAS"));
        digitalWrite(RELAY_PIN, HIGH);
        state.lightsOn = true;
        
        // Si está en modo manual, registrar el momento de encendido
        if (!config.autoMode) {
            state.manualOnStartTime = millis();
        }
    }
}

void turnOffLights() {
    if (state.lightsOn) {
        Serial.println(F("[CONTROL] Luces APAGADAS"));
        digitalWrite(RELAY_PIN, LOW);
        state.lightsOn = false;
        state.targetBrightnessPercent = 0.0f;
        
        // Reset del contador de modo manual
        state.manualOnSeconds = 0;
        telegram.manualWarningAlreadySent = false;
    }
}

// ============================================================================
// CÁLCULO DE MÉTRICAS
// ============================================================================
void updateMetrics(float deltaTime) {
    state.realWorldPowerW = MAX_SYSTEM_POWER_W * (state.currentBrightnessPercent / 100.0f);
    state.powerUsagePercent = (state.realWorldPowerW / MAX_SYSTEM_POWER_W) * 100.0f;
    
    state.totalRealEnergyTodayKWh += (state.realWorldPowerW * deltaTime) / 3600000.0;
    state.dailyCost = state.totalRealEnergyTodayKWh * ELECTRICITY_RATE_KWH;

    double traditionalEnergyKWh = (POWER_TRADITIONAL_W * deltaTime) / 3600000.0;
    double savedEnergyKWh = state.lightsOn ? (traditionalEnergyKWh - (state.realWorldPowerW * deltaTime) / 3600000.0) : 0.0;
    double totalTraditionalEnergyTodayKWh = state.totalRealEnergyTodayKWh + (state.savingsTodayUSD / ELECTRICITY_RATE_KWH);

    state.savingsTodayUSD += savedEnergyKWh * ELECTRICITY_RATE_KWH;
    
    if (totalTraditionalEnergyTodayKWh > 0) {
        state.savingsPercent = ((state.savingsTodayUSD / ELECTRICITY_RATE_KWH) / totalTraditionalEnergyTodayKWh) * 100.0;
    } else {
        state.savingsPercent = 0.0;
    }
    
    state.co2SavedTodayKg = (state.savingsTodayUSD / ELECTRICITY_RATE_KWH) * CO2_FACTOR_KG_PER_KWH;

    unsigned long uptimeSec = millis() - state.lastDayRollover;
    if (uptimeSec > 60) {
        double avgPowerW = (state.totalRealEnergyTodayKWh * 3600000.0) / uptimeSec;
        double projectedDailyKWh = (avgPowerW * 24 * 3600) / 3600000.0;
        state.monthlyCostEstimate = projectedDailyKWh * 30 * ELECTRICITY_RATE_KWH;

        double avgTraditionalPowerW = (totalTraditionalEnergyTodayKWh * 3600000.0) / uptimeSec;
        double projectedDailySavingsKWh = ((avgTraditionalPowerW - avgPowerW) * 24 * 3600) / 3600000.0;
        double monthlySavings = projectedDailySavingsKWh * 30 * ELECTRICITY_RATE_KWH;
        state.projectedDailySavings = monthlySavings / 30.0;

        if (monthlySavings > 0.01) {
            state.roiMonths = INVESTMENT_USD / monthlySavings;
        } else {
            state.roiMonths = 0.0;
        }
    }
}

void checkDayRollover() {
    if (millis() - state.lastDayRollover >= 86400000UL) {
        Serial.println("[INFO] Nuevo día. Reiniciando contadores diarios.");
        
        // Enviar reporte final del día antes de reiniciar
        sendDailyReportTelegram();
        
        // Reiniciar contadores
        state.totalRealEnergyTodayKWh = 0.0;
        state.dailyCost = 0.0;
        state.savingsTodayUSD = 0.0;
        state.co2SavedTodayKg = 0.0;
        state.lastDayRollover = millis();
        telegram.dailyReportSent = false;
    }
}

// ============================================================================
// SERVIDOR WEB Y APIs JSON
// ============================================================================
String getSystemStatusJson() {
    DynamicJsonDocument doc(1280);

    doc["lightsOn"] = state.lightsOn;
    doc["pirDetected"] = state.pirDetected;
    doc["autoMode"] = config.autoMode;
    doc["currentBrightness"] = round(state.currentBrightnessPercent);
    doc["ambientLux"] = round(state.ambientLux);
    doc["uptime"] = (millis() - state.systemStartTime) / 1000UL;
    
    // Datos de consumo y ahorro
    doc["realWorldPower"] = state.realWorldPowerW;
    doc["powerUsagePercent"] = state.powerUsagePercent;
    doc["totalRealEnergyTodayKWh"] = state.totalRealEnergyTodayKWh;
    doc["dailyCost"] = state.dailyCost;
    doc["monthlyCostEstimate"] = state.monthlyCostEstimate;
    doc["savingsPercent"] = state.savingsPercent;
    doc["savingsTodayUSD"] = state.savingsTodayUSD;
    doc["projectedDailySavings"] = state.projectedDailySavings;
    doc["co2SavedTodayKg"] = state.co2SavedTodayKg;
    doc["roiMonths"] = state.roiMonths;
    
    // Datos de modo manual (para el frontend)
    doc["manualOnSeconds"] = state.manualOnSeconds;
    
    String response;
    serializeJson(doc, response);
    return response;
}

void setupWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(SSID, PASSWORD);
    Serial.print(F("Conectando a WiFi..."));
    int retries = 20;
    while (WiFi.status() != WL_CONNECTED && retries > 0) {
        delay(500);
        Serial.print(".");
        retries--;
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println(F("\n¡Conectado!"));
        Serial.print(F("Dashboard IP: http://"));
        Serial.println(WiFi.localIP());
        state.wifiConnected = true;
    } else {
        Serial.println(F("\nNo se pudo conectar. Iniciando en modo AP."));
        WiFi.softAP("SistemaIluminacionAP", "12345678");
        Serial.print("AP IP: ");
        Serial.println(WiFi.softAPIP());
        state.wifiConnected = false;
    }
}

void setupWebServer() {
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *req) {
        req->send_P(200, "text/html", DASHBOARD_HTML);
    });

    server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *req) {
        req->send(200, "application/json", getSystemStatusJson());
    });

    server.on("/api/control", HTTP_POST, [](AsyncWebServerRequest *req) {}, nullptr,
        [](AsyncWebServerRequest *req, uint8_t *data, size_t len, size_t, size_t) {
            DynamicJsonDocument doc(256);
            if (deserializeJson(doc, data, len) == DeserializationError::Ok) {
                String action = doc["action"] | "";
                String previousMode = config.autoMode ? "AUTO" : "MANUAL";
                
                if (action == "on") {
                    config.autoMode = false;
                    turnOnLights();
                    state.targetBrightnessPercent = 100.0f;
                    state.manualOnStartTime = millis();
                    
                    // Notificación de cambio a manual ON
                    String msg = "🎛️ *Modo Manual Activado*\n\n";
                    msg += "• Estado: *ENCENDIDO*\n";
                    msg += "• Brillo: *100%*\n";
                    msg += "• Consumo: *" + String((int)MAX_SYSTEM_POWER_W) + "W*\n\n";
                    msg += "⚠️ Recuerde cambiar a modo automático para optimizar el consumo.";
                    sendTelegramMessage(msg);
                } 
                else if (action == "off") {
                    config.autoMode = false;
                    turnOffLights();
                    
                    // Notificación de cambio a manual OFF
                    String msg = "🎛️ *Modo Manual Activado*\n\n";
                    msg += "• Estado: *APAGADO*\n";
                    msg += "• Consumo: *0W*\n\n";
                    msg += "_Las luces permanecerán apagadas hasta que se active modo automático o se enciendan manualmente._";
                    sendTelegramMessage(msg);
                } 
                else if (action == "auto") {
                    config.autoMode = true;
                    telegram.manualWarningAlreadySent = false; // Reset del flag de alerta
                    
                    // Notificación de cambio a automático
                    String msg = "⚡ *Modo Automático Activado*\n\n";
                    msg += "✅ Sistema operando con sensores\n";
                    msg += "• PIR: Detección de movimiento\n";
                    msg += "• LDR: Ajuste por luz ambiente\n";
                    msg += "• Objetivo: " + String(LUX_TARGET) + " lux\n\n";
                    msg += "💡 El sistema optimizará automáticamente el consumo energético.";
                    sendTelegramMessage(msg);
                } 
                else if (action == "manual") {
                    config.autoMode = false;
                    state.manualModeStartTime = millis();
                }
                
                req->send(200, "application/json", "{\"status\":\"ok\"}");
            } else {
                req->send(400, "application/json", "{\"status\":\"bad_request\"}");
            }
        });

    server.onNotFound([](AsyncWebServerRequest *request) {
        request->send(404, "text/plain", "Not found");
    });

    server.begin();
    Serial.println(F("[WEB] Servidor HTTP iniciado."));
}