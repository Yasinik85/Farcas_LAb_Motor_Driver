#include "web_server.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp_http_server.h"
#include "esp_log.h"

static const char *TAG = "web_server";
static Motor *s_motor = nullptr;

static int get_query_int(httpd_req_t *req, const char *key, int fallback)
{
    char query[128];
    char value[32];
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) != ESP_OK) {
        return fallback;
    }
    if (httpd_query_key_value(query, key, value, sizeof(value)) != ESP_OK) {
        return fallback;
    }
    return atoi(value);
}

static int clamp_speed(int speed)
{
    if (speed < 0) return 0;
    if (speed > 100) return 100;
    return speed;
}

static const char *direction_str(Motor::Direction d)
{
    switch (d) {
        case Motor::Direction::forward: return "forward";
        case Motor::Direction::reverse: return "reverse";
        default: return "stopped";
    }
}

// ---------------------------------------------------------------------
// GUI page (HTML + CSS + JS in one file, no external assets/network needed
// since the ESP32 is offline in AP mode)
// ---------------------------------------------------------------------
static const char INDEX_HTML[] = R"HTML(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Motor Driver</title>
<style>
  :root{
    --bg:#0f1420; --panel:#161d2e; --panel2:#1d2740; --text:#e7ecf5;
    --muted:#8b96ab; --accent:#4f8cff; --accent2:#7bd88f; --danger:#ff5d6c;
    --warn:#ffb951;
  }
  *{box-sizing:border-box;}
  body{
    margin:0; font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;
    background:var(--bg); color:var(--text); min-height:100vh;
    display:flex; align-items:flex-start; justify-content:center; padding:24px 16px 60px;
  }
  .wrap{width:100%; max-width:520px;}
  h1{font-size:20px; font-weight:600; margin:0 0 4px;}
  .sub{color:var(--muted); font-size:13px; margin-bottom:20px;}
  .card{
    background:var(--panel); border-radius:16px; padding:20px; margin-bottom:16px;
    box-shadow:0 4px 20px rgba(0,0,0,0.25);
  }
  .status-row{display:flex; justify-content:space-between; align-items:center; margin-bottom:4px;}
  .status-row + .status-row{margin-top:10px;}
  .label{color:var(--muted); font-size:13px;}
  .value{font-size:15px; font-weight:600;}
  .badge{
    display:inline-block; padding:3px 10px; border-radius:999px; font-size:12px; font-weight:600;
  }
  .badge.forward{background:rgba(123,216,143,0.15); color:var(--accent2);}
  .badge.reverse{background:rgba(255,185,81,0.15); color:var(--warn);}
  .badge.stopped{background:rgba(139,150,171,0.15); color:var(--muted);}
  .slider-row{margin:18px 0 6px;}
  input[type=range]{
    width:100%; height:6px; border-radius:3px; background:var(--panel2);
    appearance:none; -webkit-appearance:none;
  }
  input[type=range]::-webkit-slider-thumb{
    -webkit-appearance:none; width:22px; height:22px; border-radius:50%;
    background:var(--accent); cursor:pointer; border:3px solid #0f1420;
  }
  .speed-readout{text-align:center; font-size:28px; font-weight:700; margin:8px 0 2px;}
  .btn-grid{display:grid; grid-template-columns:1fr 1fr; gap:10px; margin-top:16px;}
  button{
    border:none; border-radius:12px; padding:14px 10px; font-size:15px; font-weight:600;
    cursor:pointer; color:white; transition:transform .05s ease, opacity .15s ease;
  }
  button:active{transform:scale(0.97);}
  .btn-forward{background:var(--accent2); color:#0f1420;}
  .btn-reverse{background:var(--warn); color:#0f1420;}
  .btn-stop{background:var(--panel2); color:var(--text); border:1px solid #2a3550;}
  .btn-brake{background:var(--danger);}
  canvas{width:100%; height:180px; display:block;}
  .legend{display:flex; gap:18px; font-size:12px; color:var(--muted); margin-top:8px;}
  .dot{display:inline-block; width:9px; height:9px; border-radius:50%; margin-right:6px;}
  .conn{font-size:12px; color:var(--muted); text-align:center; margin-top:14px;}
  .conn.bad{color:var(--danger);}
</style>
</head>
<body>
<div class="wrap">
  <h1>Motor Driver</h1>
  <div class="sub">ESP32 SoftAP &middot; live control &amp; monitoring</div>

  <div class="card">
    <div class="status-row">
      <span class="label">Direction</span>
      <span id="dirBadge" class="badge stopped">stopped</span>
    </div>
    <div class="status-row">
      <span class="label">Running</span>
      <span id="runVal" class="value">no</span>
    </div>
  </div>

  <div class="card">
    <div class="slider-row">
      <input type="range" id="speedSlider" min="0" max="100" value="0">
    </div>
    <div class="speed-readout"><span id="speedReadout">0</span>%</div>

    <div class="btn-grid">
      <button class="btn-forward" onclick="drive('forward')">&#9650; Forward</button>
      <button class="btn-reverse" onclick="drive('reverse')">&#9660; Reverse</button>
      <button class="btn-stop" onclick="drive('stop')">Stop</button>
      <button class="btn-brake" onclick="drive('brake')">Brake</button>
    </div>
  </div>

  <div class="card">
    <canvas id="chart" width="480" height="180"></canvas>
    <div class="legend">
      <span><span class="dot" style="background:#4f8cff"></span>Speed (%)</span>
    </div>
  </div>

  <div id="conn" class="conn">connecting&hellip;</div>
</div>

<script>
const speedSlider = document.getElementById('speedSlider');
const speedReadout = document.getElementById('speedReadout');
const dirBadge = document.getElementById('dirBadge');
const runVal = document.getElementById('runVal');
const connEl = document.getElementById('conn');
const canvas = document.getElementById('chart');
const ctx = canvas.getContext('2d');

let currentDirection = 'stop';
const history = []; // {t, speed}
const MAX_POINTS = 60;

speedSlider.addEventListener('input', () => {
  speedReadout.textContent = speedSlider.value;
});
speedSlider.addEventListener('change', () => {
  fetch(`/api/setspeed?speed=${speedSlider.value}`);
});

function drive(action) {
  const speed = speedSlider.value;
  if (action === 'forward' || action === 'reverse') {
    fetch(`/api/${action}?speed=${speed}`);
  } else {
    fetch(`/api/${action}`);
  }
}

function badgeClass(dir) {
  if (dir === 'forward') return 'badge forward';
  if (dir === 'reverse') return 'badge reverse';
  return 'badge stopped';
}

function drawChart() {
  const w = canvas.width, h = canvas.height;
  ctx.clearRect(0, 0, w, h);
  ctx.strokeStyle = '#1d2740';
  ctx.lineWidth = 1;
  for (let i = 0; i <= 4; i++) {
    const y = (h / 4) * i;
    ctx.beginPath(); ctx.moveTo(0, y); ctx.lineTo(w, y); ctx.stroke();
  }
  if (history.length < 2) return;

  const stepX = w / (MAX_POINTS - 1);
  const startIdx = MAX_POINTS - history.length;

  function plot(getVal, maxVal, color) {
    ctx.beginPath();
    ctx.strokeStyle = color;
    ctx.lineWidth = 2;
    history.forEach((p, i) => {
      const x = (startIdx + i) * stepX;
      const y = h - (getVal(p) / maxVal) * h;
      if (i === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y);
    });
    ctx.stroke();
  }

  plot(p => p.speed, 100, '#4f8cff');
}

async function poll() {
  try {
    const res = await fetch('/api/status');
    const data = await res.json();
    connEl.textContent = 'connected';
    connEl.classList.remove('bad');

    dirBadge.textContent = data.direction;
    dirBadge.className = badgeClass(data.direction);
    runVal.textContent = data.running ? 'yes' : 'no';
    currentDirection = data.direction;

    // Only move the slider to match the device if the user isn't actively dragging it
    if (document.activeElement !== speedSlider) {
      speedSlider.value = data.speed;
      speedReadout.textContent = data.speed;
    }

    history.push({t: Date.now(), speed: data.speed});
    if (history.length > MAX_POINTS) history.shift();
    drawChart();
  } catch (e) {
    connEl.textContent = 'disconnected \u2014 retrying\u2026';
    connEl.classList.add('bad');
  }
}

setInterval(poll, 500);
poll();
</script>
</body>
</html>
)HTML";

static esp_err_t index_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, INDEX_HTML, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t status_get_handler(httpd_req_t *req)
{
    char resp[128];
    snprintf(resp, sizeof(resp),
             "{\"direction\":\"%s\",\"speed\":%d,\"running\":%s}",
             direction_str(s_motor->getDirection()),
             s_motor->getSpeed(),
             s_motor->isRunning() ? "true" : "false");
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t forward_get_handler(httpd_req_t *req)
{
    int speed = clamp_speed(get_query_int(req, "speed", s_motor->getSpeed()));
    s_motor->forward(speed);
    ESP_LOGI(TAG, "forward(%d)", speed);
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, "{\"ok\":true}", HTTPD_RESP_USE_STRLEN);
}

static esp_err_t reverse_get_handler(httpd_req_t *req)
{
    int speed = clamp_speed(get_query_int(req, "speed", s_motor->getSpeed()));
    s_motor->reverse(speed);
    ESP_LOGI(TAG, "reverse(%d)", speed);
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, "{\"ok\":true}", HTTPD_RESP_USE_STRLEN);
}

static esp_err_t setspeed_get_handler(httpd_req_t *req)
{
    int speed = clamp_speed(get_query_int(req, "speed", s_motor->getSpeed()));
    s_motor->setSpeed(speed); // driver keeps current direction, or stops if already stopped
    ESP_LOGI(TAG, "setSpeed(%d)", speed);
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, "{\"ok\":true}", HTTPD_RESP_USE_STRLEN);
}

static esp_err_t stop_get_handler(httpd_req_t *req)
{
    s_motor->stop();
    ESP_LOGI(TAG, "stop()");
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, "{\"ok\":true}", HTTPD_RESP_USE_STRLEN);
}

static esp_err_t brake_get_handler(httpd_req_t *req)
{
    s_motor->brake();
    ESP_LOGI(TAG, "brake()");
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, "{\"ok\":true}", HTTPD_RESP_USE_STRLEN);
}

void web_server_start(Motor *motor)
{
    s_motor = motor;

    httpd_handle_t server = nullptr;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 8;

    if (httpd_start(&server, &config) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start HTTP server");
        return;
    }

    httpd_uri_t routes[] = {
        {.uri = "/",                .method = HTTP_GET, .handler = index_get_handler,    .user_ctx = nullptr},
        {.uri = "/api/status",      .method = HTTP_GET, .handler = status_get_handler,   .user_ctx = nullptr},
        {.uri = "/api/forward",     .method = HTTP_GET, .handler = forward_get_handler,  .user_ctx = nullptr},
        {.uri = "/api/reverse",     .method = HTTP_GET, .handler = reverse_get_handler,  .user_ctx = nullptr},
        {.uri = "/api/setspeed",    .method = HTTP_GET, .handler = setspeed_get_handler, .user_ctx = nullptr},
        {.uri = "/api/stop",        .method = HTTP_GET, .handler = stop_get_handler,     .user_ctx = nullptr},
        {.uri = "/api/brake",       .method = HTTP_GET, .handler = brake_get_handler,    .user_ctx = nullptr},
    };

    for (auto &route : routes) {
        httpd_register_uri_handler(server, &route);
    }

    ESP_LOGI(TAG, "Web server started");
}
