#include "ServerManager.hpp"

// formats a dosing time (sec) as "M:SS Min = X,XXXL" (DE) or "M:SS min = X.XXXL" (EN)
String ServerManager::formatDoseLabel(int durationSec) {
  bool en = Config::DASHBOARD_LANGUAGE == Config::Language::EN;

  String timeStr;
  int mm = durationSec / 60;
  int ss = durationSec % 60;
  if (mm > 0) timeStr = String(mm) + ":" + (ss < 10 ? "0" : "") + String(ss) + (en ? " min" : " Min");
  else timeStr = String(ss) + (en ? " sec" : " Sek");

  float liters = durationSec / 440.0f;  // linear: 55 sec = 0,125 L
  String literStr = String(liters, 3);
  while (literStr.endsWith("0")) literStr.remove(literStr.length() - 1);
  if (literStr.endsWith(".")) literStr.remove(literStr.length() - 1);
  if (!en) literStr.replace(".", ",");

  return timeStr + " = " + literStr + "L";
}

// picks the German or English text depending on Config::DASHBOARD_LANGUAGE
static String T(const char* de, const char* en) {
  return Config::DASHBOARD_LANGUAGE == Config::Language::EN ? String(en) : String(de);
}

// HTTP Basic Auth guard for the dashboard/control endpoints; sends the 401
// challenge itself when unauthenticated, so callers just need to return.
static bool requireAuth(AsyncWebServerRequest *request) {
  if (!request->authenticate(Config::WEB_AUTH_USER, Config::WEB_AUTH_PASSWORD)) {
    request->requestAuthentication();
    return false;
  }
  return true;
}

ServerManager::ServerManager(int port)
  : _server(port), _timeSync(nullptr), tempSensor(Config::PIN_TEMP_SENSOR), isPoolpumpActiv(false) {}

void ServerManager::begin(DailyTimeSync *ts) {
  _timeSync = ts;
  tempSensor.begin();

  // initialize pins
  pinMode(_pool.pin, OUTPUT);
  pinMode(_ph.pin, OUTPUT);
  pinMode(_cl.pin, OUTPUT);

  _server.on("/", HTTP_GET, [this](AsyncWebServerRequest *request) {
    if (!requireAuth(request)) return;

    String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";

    html += "<style>";
    html += "body{font-family:sans-serif; text-align:center; background:#f4f4f4;} .box{background:white; border:1px solid #ccc; margin:10px auto; padding:15px; border-radius:10px; max-width:400px;} ";
    html += "button{padding:10px; margin:5px; width:70px; cursor:pointer; border-radius:5px; border:1px solid #999;}";
    html += "#time{font-size: 16px; margin: 5px; padding:0px;}";
    html += "#head{font-size: 40px; margin:5px; padding: 0px;}";
    html += "#pumpLabel{margin: 5px; padding: 0px;}";
    html += "</style>";

    html += "<script>";
    html += "  function setMode(dev, m) { fetch('/set?dev=' + dev + '&m=' + m); }";
    html += "  function update() { fetch('/status').then(r => r.json()).then(data => {";
    html += "    document.getElementById('time').innerText = data.time;";
    html += "    ['pool','ph','cl'].forEach(d => { ";
    html += "         let stateTxt = data[d].state ? '" + T("LÄUFT", "RUNNING") + "' : '" + T("AUS", "OFF") + "'; ";
    html += "         let modeVal = data[d].mode; ";
    // special case: interlock active (mode ON, but pump OFF because pool pump is OFF)
    html += "         if (modeVal == 1 && !data[d].state && d !== 'pool') {";
    html += "             stateTxt = '" + T("WARTET (Sperre)", "WAITING (interlock)") + "';";
    html += "}";
    html += "         document.getElementById(d + '_st').innerText = stateTxt; ";
    html += "         document.getElementById(d + '_md').innerText = (modeVal == 2) ? 'AUTO' : '" + T("HAND", "MANUAL") + "'; ";  // display logic
    html += "         document.getElementById(d + '_md').style.color = (data[d].mode == 2) ? 'blue' : 'orange'; ";
    html += "    });";
    html += "  }); } setInterval(update, 1000);";
    html += "</script></head><body onload='update()'>";

    if(isPoolpumpActiv){
      html += "<h1 id='head'>PoolGuard</h1><div id='time'>--:--:--</div>";
      html += "<div id='temp'> " + T("Wassertemperatur", "Water temperature") + ": "+String(tempSensor.getLatestTemperature()) +"°C";
    }
    else html += "<h1 id='head'>PoolGuard</h1><div id='time'>--:--:--</div>";


    auto createBox = [&](String id, String label, Device &d, int doseSec = 0) {
      String s = "<div class='box'><h3 id=pumpLabel>" + label + "</h3>";
      s += "<p>" + T("Status", "Status") + ": <b id='" + id + "_st'>-</b> | " + T("Modus", "Mode") + ": <b id='" + id + "_md'>-</b></p>";

      // display the intervals
      s += "<div style='font-size:0.85em; color:#666; margin-bottom:10px;'>";
      if (id == "pool") {
        s += T("Intervalle", "Intervals") + ": " + String(d.times[0].startH) + ":00-" + String(d.times[0].endH) + ":00, ";
        s += String(d.times[1].startH) + ":00-" + String(d.times[1].endH) + ":00, ";
        s += String(d.times[2].startH) + ":00-" + String(d.times[2].endH) + ":00";
      } else {
        s += T("Auto-Laufzeit", "Auto runtime") + ": " + String(d.times[0].startH) + ":" + (d.times[0].startM < 10 ? "0" : "") + String(d.times[0].startM);
        s += " (" + T("für ", "for ") + formatDoseLabel(doseSec) + ")";
      }
      s += "</div>";

      s += "<button onclick=\"setMode('" + id + "',0)\">OFF</button>";
      s += "<button onclick=\"setMode('" + id + "',1)\">ON</button>";
      s += "<button onclick=\"setMode('" + id + "',2)\">AUTO</button></div>";
      return s;
    };

    html += createBox("pool", T("Poolpumpe", "Pool pump"), _pool);
    html += createBox("ph", T("PH-Pumpe", "pH pump"), _ph, _phDoseSec);
    html += createBox("cl", T("Chlor-Pumpe", "Chlorine pump"), _cl, _clDoseSec);

    html += "</body></html>";
    request->send(200, "text/html", html);
  });

  _server.on("/set", HTTP_GET, [this](AsyncWebServerRequest *request) {
    if (!requireAuth(request)) return;

    if (request->hasParam("dev") && request->hasParam("m")) {
      String dev = request->getParam("dev")->value();
      int mVal = request->getParam("m")->value().toInt();
      if (mVal >= MODE_OFF && mVal <= MODE_AUTO) {
        DeviceMode m = (DeviceMode)mVal;
        if (dev == "pool") _pool.mode = m;
        else if (dev == "ph") _ph.mode = m;
        else if (dev == "cl") _cl.mode = m;
      }
    }
    request->send(200, "text/plain", "OK");
  });

  // the status endpoint now also returns the mode (0, 1, or 2)
  _server.on("/status", HTTP_GET, [this](AsyncWebServerRequest *request) {
    if (!requireAuth(request)) return;

    String j = "{ \"time\":\"" + _timeSync->getFormattedTime() + "\",";
    j += "\"pool\":{\"state\":" + String(digitalRead(_pool.pin)) + ",\"mode\":" + String(_pool.mode) + "},";
    j += "\"ph\":{\"state\":" + String(digitalRead(_ph.pin)) + ",\"mode\":" + String(_ph.mode) + "},";
    j += "\"cl\":{\"state\":" + String(digitalRead(_cl.pin)) + ",\"mode\":" + String(_cl.mode) + "} }";
    request->send(200, "application/json", j);
  });

  _server.begin();
}

// helper function for time checks (hour, minute, second)
bool ServerManager::isInside(int h, int m, int s, TimeRange tr, int durationSec) {
  long current = h * 3600L + m * 60L + s;
  long start = tr.startH * 3600L + tr.startM * 60L;

  if (durationSec > 0) {
    return (current >= start && current < (start + durationSec));
  } else {
    long end = tr.endH * 3600L + tr.endM * 60L;
    return (current >= start && current < end);
  }
}

void ServerManager::handleLogic() {
  tempSensor.update();  // non-blocking, checks the measurement interval internally

  int h = _timeSync->getHour();
  int m = _timeSync->getMinute();
  int s = _timeSync->getSecond();

  // 1. pool pump (master)
  bool poolShouldRun = false;
  if (_pool.mode == MODE_ON) {
    poolShouldRun = true;
  } else if (_pool.mode == MODE_OFF) {
    poolShouldRun = false;
  } else if (_pool.mode == MODE_AUTO) {
    for (int i = 0; i < _pool.activeIntervals; i++) {
      if (isInside(h, m, s, _pool.times[i])) {
        poolShouldRun = true;
      }
    }
  }
  isPoolpumpActiv = poolShouldRun;
  digitalWrite(_pool.pin, poolShouldRun);

  // 2. safety interlock (is the pool pump really ON?)
  bool flowOk = digitalRead(_pool.pin);

  // 3. chemical pumps (pH & chlorine), duration adjustable via _phDoseSec/_clDoseSec
  auto processChemie = [&](Device &d, int durationSec) {
    bool run = false;
    if (d.mode == MODE_ON) run = true;
    else if (d.mode == MODE_OFF) run = false;
    else if (d.mode == MODE_AUTO) {
      // in auto mode this ONLY runs if the pool pump is also running
      if (flowOk && isInside(h, m, s, d.times[0], durationSec)) run = true;
    }
    // additional hard interlock: chemical pumps ALWAYS OFF when pool is OFF
    digitalWrite(d.pin, (run && flowOk) ? HIGH : LOW);
  };

  processChemie(_ph, _phDoseSec);
  processChemie(_cl, _clDoseSec);
}