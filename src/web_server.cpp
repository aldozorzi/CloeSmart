// web_server.cpp
//
// Local web interface and HTTP API. It calls the same processCommand()
// used by Telegram, so every command behaves identically.
//
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>
#include <ArduinoJson.h>

#include "web_server.h"
#include "config.h"
#include "secrets.h"
#include "command_handler.h"
#include "stove_fsm.h"
#include "crono.h"
#include "thermometer.h"

static WebServer server(WEB_PORT);

static bool otaUploadAuthorized = false;
static bool otaUploadRejected = false;
static bool otaUploadSuccess = false;
static int otaRejectStatus = 500;
static String otaRejectMessage;

static const char PAGE_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>CloeSmart</title>
<style>
  :root { color-scheme: dark; }
  * { box-sizing: border-box; }
  body { margin: 0; font-family: system-ui, -apple-system, Segoe UI, Roboto, sans-serif;
         background: #101417; color: #e8eef2; }
  main { max-width: 760px; margin: 0 auto; padding: 20px; }
  h1 { font-size: 1.5rem; margin: 0 0 18px; }
  h2 { font-size: 1rem; margin: 0 0 12px; color: #9fb3c0; text-transform: uppercase;
       letter-spacing: .06em; }
  section { background: #182027; border: 1px solid #26333d; border-radius: 12px;
            padding: 16px; margin-bottom: 14px; }
  .grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(140px, 1fr));
          gap: 12px; }
  .item .label { display: block; font-size: .75rem; color: #8aa0ad;
                 text-transform: uppercase; letter-spacing: .05em; }
  .item .value { display: block; margin-top: 3px; font-size: 1rem; }
  button, input[type=submit] { background: #e8732f; color: #101417; border: 0;
       border-radius: 8px; padding: 10px 14px; font-size: .95rem; font-weight: 700;
       cursor: pointer; }
  button.secondary { background: #33434f; color: #e8eef2; }
  button:active { transform: translateY(1px); }
  .row { display: flex; flex-wrap: wrap; gap: 10px; align-items: end; }
  label { display: flex; flex-direction: column; gap: 5px; font-size: .8rem;
          color: #8aa0ad; }
  input[type=number], input[type=time], input[type=file] {
       background: #0d1216; color: #e8eef2; border: 1px solid #33434f;
       border-radius: 8px; padding: 9px 10px; font-size: .95rem; }
  input[type=file] { width: 100%; }
  .hint { color: #8aa0ad; font-size: .85rem; margin: 0 0 12px; }
  #message { min-height: 1.4em; margin-top: 4px; font-size: .9rem; }
  #message.error { color: #ff7b72; }
  .message { color: #7ee787; }
</style>
</head>
<body>
<main>
  <h1>CloeSmart</h1>

  <section>
    <h2>Status</h2>
    <div class="grid">
      <div class="item"><span class="label">Power</span><span class="value" id="enabled">-</span></div>
      <div class="item"><span class="label">State</span><span class="value" id="state">-</span></div>
      <div class="item"><span class="label">Temperature</span><span class="value" id="temperature_c">-</span></div>
      <div class="item"><span class="label">Target</span><span class="value" id="target_c">-</span></div>
      <div class="item"><span class="label">System time</span><span class="value" id="time">-</span></div>
      <div class="item"><span class="label">Firmware</span><span class="value" id="version">-</span></div>
      <div class="item"><span class="label">IP address</span><span class="value" id="ip">-</span></div>
      <div class="item"><span class="label">WiFi signal</span><span class="value" id="rssi">-</span></div>
    </div>
  </section>

  <section>
    <h2>Power</h2>
    <div class="row">
      <button data-cmd="/accendi">Turn ON</button>
      <button class="secondary" data-cmd="/spegni">Turn OFF</button>
      <button class="secondary" data-cmd="/stato">Refresh status</button>
    </div>
  </section>

  <section>
    <h2>Target temperature</h2>
    <form id="setpoint-form" class="row">
      <label>Setpoint (10-30 &deg;C)
        <input id="setpoint" type="number" min="10" max="30" step="0.1" required>
      </label>
      <button type="submit">Set target</button>
    </form>
  </section>

  <section>
    <h2>Schedule</h2>
    <div class="row" style="margin-bottom: 12px;">
      <button id="crono-on">Enable schedule</button>
      <button id="crono-off" class="secondary">Disable schedule</button>
    </div>
    <p class="hint">Crono: <strong id="crono_enabled">-</strong></p>
    <form id="schedule-form" class="row">
      <label>ON time <input id="on-time" type="time" required></label>
      <label>OFF time <input id="off-time" type="time" required></label>
      <button type="submit">Save schedule</button>
    </form>
    <p class="hint" style="margin-top: 12px;">Current schedule:
      <strong id="crono_on">-</strong> - <strong id="crono_off">-</strong></p>
  </section>

  <section>
    <h2>Firmware update</h2>
    <p class="hint">The stove must be OFF. Select a <code>firmware.bin</code> built for this board;
      the device will reboot automatically after a successful upload.</p>
    <form id="ota-form" method="POST" action="/update" enctype="multipart/form-data">
      <div class="row">
        <input type="file" name="update" accept=".bin" required>
        <button type="submit">Upload and reboot</button>
      </div>
    </form>
  </section>

  <div id="message"></div>
</main>

<script>
const $ = id => document.getElementById(id);
let messageTimer;

function show(text, isError) {
  const el = $('message');
  el.textContent = text || '';
  el.className = isError ? 'error' : 'message';
  clearTimeout(messageTimer);
  if (text) messageTimer = setTimeout(() => { el.textContent = ''; }, 8000);
}

async function refresh() {
  try {
    const r = await fetch('/api/status', { cache: 'no-store' });
    if (!r.ok) throw new Error(r.status === 401 ? 'Authentication required' : 'Status request failed');
    const s = await r.json();
    $('enabled').textContent = s.enabled ? 'ON' : 'OFF';
    $('state').textContent = s.state;
    $('temperature_c').textContent = Number(s.temperature_c).toFixed(1) + ' \u00b0C';
    $('target_c').textContent = Number(s.target_c).toFixed(1) + ' \u00b0C';
    $('crono_enabled').textContent = s.crono_enabled ? 'ENABLED' : 'DISABLED';
    $('crono_on').textContent = s.crono_on;
    $('crono_off').textContent = s.crono_off;
    $('time').textContent = s.time;
    $('version').textContent = s.version;
    $('ip').textContent = s.ip;
    $('rssi').textContent = s.rssi + ' dBm';
    if (document.activeElement !== $('setpoint')) $('setpoint').value = Number(s.target_c).toFixed(1);
    if (document.activeElement !== $('on-time')) $('on-time').value = s.crono_on;
    if (document.activeElement !== $('off-time')) $('off-time').value = s.crono_off;
  } catch (e) {
    show(e.message, true);
  }
}

async function send(cmd) {
  try {
    const body = new URLSearchParams({ cmd });
    const r = await fetch('/api/command', {
      method: 'POST',
      headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
      body
    });
    const data = await r.json();
    if (!r.ok) throw new Error(data.error || 'Command failed');
    show(data.response, false);
    await refresh();
  } catch (e) {
    show(e.message, true);
  }
}

document.querySelectorAll('[data-cmd]').forEach(btn => {
  btn.addEventListener('click', () => send(btn.dataset.cmd));
});

$('setpoint-form').addEventListener('submit', e => {
  e.preventDefault();
  send('/set_temp ' + $('setpoint').value);
});

$('crono-on').addEventListener('click', () => send('/crono_on'));
$('crono-off').addEventListener('click', () => send('/crono_off'));

$('schedule-form').addEventListener('submit', async e => {
  e.preventDefault();
  await send('/set_on ' + $('on-time').value);
  await send('/set_off ' + $('off-time').value);
});

$('ota-form').addEventListener('submit', e => {
  if (!confirm('The stove must be OFF. Upload the firmware and reboot now?')) e.preventDefault();
});

refresh();
setInterval(refresh, 5000);
</script>
</body>
</html>
)rawliteral";

static const char *stateName() {
  switch (currentStoveState) {
    case STATE_OFF:         return "off";
    case STATE_WORK:        return "working";
    case STATE_MODULATING:  return "modulating";
    case STATE_KICKSTART:   return "kickstart";
    default:                return "unknown";
  }
}

static bool requireAuth() {
  if (server.authenticate(WEB_AUTH_USER, DEVICE_PASSWORD)) return true;
  server.requestAuthentication();
  return false;
}

static void sendJson(int status, const String &body) {
  server.sendHeader("Cache-Control", "no-store");
  server.send(status, "application/json", body);
}

static void handleRoot() {
  if (!requireAuth()) return;
  server.sendHeader("Cache-Control", "no-store");
  server.send_P(200, "text/html", PAGE_HTML);
}

static void handleStatus() {
  if (!requireAuth()) return;

  StaticJsonDocument<512> doc;
  doc["state"]          = stateName();
  doc["enabled"]        = isStoveEnabled;
  doc["temperature_c"]  = currentTemperature;
  doc["target_c"]       = targetTemperature;
  doc["crono_enabled"]  = isCronoEnabled;
  doc["crono_on"]       = cronoOnTime;
  doc["crono_off"]      = cronoOffTime;
  doc["time"]           = getCurrentTime();
  doc["uptime_s"]       = millis() / 1000;
  doc["version"]        = FW_VERSION;
  doc["ip"]             = WiFi.localIP().toString();
  doc["rssi"]           = WiFi.RSSI();

  String out;
  serializeJson(doc, out);
  sendJson(200, out);
}

static void handleCommand() {
  if (!requireAuth()) return;

  String cmd;
  if (server.hasArg("cmd")) {
    cmd = server.arg("cmd");
  } else if (server.hasArg("plain")) {
    StaticJsonDocument<256> in;
    String body = server.arg("plain");
    if (deserializeJson(in, body) == DeserializationError::Ok && in.containsKey("cmd")) {
      cmd = in["cmd"].as<String>();
    } else {
      cmd = body;
    }
  }
  cmd.trim();

  if (cmd.isEmpty()) {
    sendJson(400, "{\"ok\":false,\"error\":\"Missing command\"}");
    return;
  }

  String response = processCommand(cmd);

  StaticJsonDocument<1024> doc;
  doc["ok"]       = true;
  doc["response"] = response;

  String out;
  serializeJson(doc, out);
  sendJson(200, out);
}

static void handleOtaUpload() {
  HTTPUpload &upload = server.upload();

  if (upload.status == UPLOAD_FILE_START) {
    otaUploadAuthorized  = server.authenticate(WEB_AUTH_USER, DEVICE_PASSWORD);
    otaUploadRejected    = false;
    otaUploadSuccess     = false;
    otaRejectStatus      = 500;
    otaRejectMessage     = "";

    if (!otaUploadAuthorized) {
      otaUploadRejected = true;
      otaRejectStatus   = 401;
      otaRejectMessage  = "Unauthorized";
      return;
    }

    if (currentStoveState != STATE_OFF || isStoveEnabled) {
      otaUploadRejected = true;
      otaRejectStatus   = 409;
      otaRejectMessage  = "Turn the stove off before updating the firmware";
      return;
    }

    isOtaInProgress = true;

    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
      otaUploadRejected = true;
      otaRejectStatus   = 500;
      otaRejectMessage  = String("Update begin failed: ") + Update.errorString();
      isOtaInProgress   = false;
    }
    return;
  }

  if (otaUploadRejected) return;

  if (upload.status == UPLOAD_FILE_WRITE) {
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      otaUploadRejected = true;
      otaRejectStatus   = 500;
      otaRejectMessage  = String("Update write failed: ") + Update.errorString();
      Update.abort();
      isOtaInProgress = false;
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (Update.end(true)) {
      otaUploadSuccess = true;
    } else {
      otaUploadRejected = true;
      otaRejectStatus   = 500;
      otaRejectMessage  = String("Update end failed: ") + Update.errorString();
      isOtaInProgress   = false;
    }
  } else if (upload.status == UPLOAD_FILE_ABORTED) {
    Update.abort();
    otaUploadRejected = true;
    otaRejectStatus   = 400;
    otaRejectMessage  = "Upload aborted";
    isOtaInProgress   = false;
  }
}

static void handleOtaFinished() {
  if (!server.authenticate(WEB_AUTH_USER, DEVICE_PASSWORD)) {
    server.requestAuthentication();
    return;
  }

  if (otaUploadRejected) {
    isOtaInProgress = false;
    server.sendHeader("Connection", "close");
    server.send(otaRejectStatus, "text/plain", otaRejectMessage);
    return;
  }

  if (!otaUploadSuccess || Update.hasError()) {
    isOtaInProgress = false;
    server.sendHeader("Connection", "close");
    server.send(500, "text/plain", String("Update failed: ") + Update.errorString());
    return;
  }

  server.sendHeader("Connection", "close");
  server.send(200, "text/plain", "Firmware updated successfully. Rebooting...");
  delay(1000);
  ESP.restart();
}

static void handleNotFound() {
  if (!requireAuth()) return;
  sendJson(404, "{\"ok\":false,\"error\":\"Not found\"}");
}

void taskWebServer(void *pvParameters) {
  LOG("Web server task started.");

  if (strlen(DEVICE_PASSWORD) == 0) {
    LOG("Web server disabled: DEVICE_PASSWORD is empty in secrets.h");
    vTaskDelete(NULL);
    return;
  }

  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/status", HTTP_GET, handleStatus);
  server.on("/api/command", HTTP_POST, handleCommand);
  server.on("/update", HTTP_POST, handleOtaFinished, handleOtaUpload);
  server.onNotFound(handleNotFound);
  server.begin();

  LOG("Web UI: http://" + WiFi.localIP().toString() + "/");

  if (MDNS.begin(HOSTNAME)) {
    MDNS.addService("http", "tcp", WEB_PORT);
    LOG("Web UI: http://" + String(HOSTNAME) + ".local/");
  } else {
    LOG("mDNS failed to start");
  }

  for (;;) {
    server.handleClient();
    vTaskDelay(pdMS_TO_TICKS(2));
  }
}
