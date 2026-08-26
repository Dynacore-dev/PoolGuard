#include "ServerManager.hpp"

// Formatiert eine Dosierzeit (Sek.) als "M:SS Min = X,XXXL" bzw. "SS Sek = X,XXXL"
String ServerManager::formatDoseLabel(int durationSec) {
  String timeStr;
  int mm = durationSec / 60;
  int ss = durationSec % 60;
  if (mm > 0) timeStr = String(mm) + ":" + (ss < 10 ? "0" : "") + String(ss) + " Min";
  else timeStr = String(ss) + " Sek";

  float liters = durationSec / 440.0f;  // linear: 55 Sek. = 0,125 L
  String literStr = String(liters, 3);
  while (literStr.endsWith("0")) literStr.remove(literStr.length() - 1);
  if (literStr.endsWith(".")) literStr.remove(literStr.length() - 1);
  literStr.replace(".", ",");

  return timeStr + " = " + literStr + "L";
}

ServerManager::ServerManager(int port)
  : _server(port), tempSensor(16) {}

void ServerManager::begin(DailyTimeSync *ts) {
  _timeSync = ts;
  tempSensor.begin();

  // Pins initialisieren
  pinMode(_pool.pin, OUTPUT);
  pinMode(_ph.pin, OUTPUT);
  pinMode(_cl.pin, OUTPUT);

  _server.on("/", HTTP_GET, [this](AsyncWebServerRequest *request) {
    String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";

    html += "<style>";
    html += "body{font-family:sans-serif; text-align:center; background:#f4f4f4;} .box{background:white; border:1px solid #ccc; margin:10px auto; padding:15px; border-radius:10px; max-width:400px;} ";
    html += "button{padding:10px; margin:5px; width:70px; cursor:pointer; border-radius:5px; border:1px solid #999;} .active{background-color:#4CAF50; color:white; border:none;}";
    html += "#time{font-size: 16px; margin: 5px; padding:0px;}";
    html += "#head{font-size: 40px; margin:5px; padding: 0px;}";
    html += "#pumpLabel{margin: 5px; padding: 0px;}";
    html += "</style>";

    html += "<script>";
    html += "  function setMode(dev, m) { fetch('/set?dev=' + dev + '&m=' + m); }";
    html += "  function update() { fetch('/status').then(r => r.json()).then(data => {";
    html += "    document.getElementById('time').innerText = data.time;";
    html += "    ['pool','ph','cl'].forEach(d => { ";
    html += "         let stateTxt = data[d].state ? 'LÄUFT' : 'AUS'; ";
    html += "         let modeVal = data[d].mode; ";
    // Spezialfall: Verriegelung aktiv (Modus EIN, aber Pumpe AUS weil Poolpumpe AUS)
    html += "         if (modeVal == 1 && !data[d].state && d !== 'pool') {";
    html += "             stateTxt = 'WARTET (Sperre)';";
    html += "}";
    html += "         document.getElementById(d + '_st').innerText = stateTxt; ";
    html += "         document.getElementById(d + '_md').innerText = (modeVal == 2) ? 'AUTO' : 'HAND'; ";  // Logik für Anzeige
    html += "         document.getElementById(d + '_md').style.color = (data[d].mode == 2) ? 'blue' : 'orange'; ";
    html += "    });";
    html += "  }); } setInterval(update, 1000);";
    html += "</script></head><body onload='update()'>";

    if(isPoolpumpActiv){
      html += "<h1 id='head'>Pool Steuerung</h1><div id='time'>--:--:--</div>";
      html += "<div id='temp'> Wassertemperatur: "+String(tempSensor.getLatestTemperature()) +"°C";
    }
    else html += "<h1 id='head'>Pool Steuerung</h1><div id='time'>--:--:--</div>";


    auto createBox = [&](String id, String label, Device &d, int doseSec = 0) {
      String s = "<div class='box'><h3 id=pumpLabel>" + label + "</h3>";
      s += "<p>Status: <b id='" + id + "_st'>-</b> | Modus: <b id='" + id + "_md'>-</b></p>";

      // Anzeige der Intervalle
      s += "<div style='font-size:0.85em; color:#666; margin-bottom:10px;'>";
      if (id == "pool") {
        s += "Intervalle: " + String(d.times[0].startH) + ":00-" + String(d.times[0].endH) + ":00, ";
        s += String(d.times[1].startH) + ":00-" + String(d.times[1].endH) + ":00, ";
        s += String(d.times[2].startH) + ":00-" + String(d.times[2].endH) + ":00";
      } else {
        s += "Auto-Laufzeit: " + String(d.times[0].startH) + ":" + (d.times[0].startM < 10 ? "0" : "") + String(d.times[0].startM);
        s += " (für " + formatDoseLabel(doseSec) + ")";
      }
      s += "</div>";

      s += "<button onclick=\"setMode('" + id + "',0)\">OFF</button>";
      s += "<button onclick=\"setMode('" + id + "',1)\">ON</button>";
      s += "<button onclick=\"setMode('" + id + "',2)\">AUTO</button></div>";
      return s;
    };

    html += createBox("pool", "Poolpumpe", _pool);
    html += createBox("ph", "PH-Pumpe", _ph, _phDoseSec);
    html += createBox("cl", "Chlor-Pumpe", _cl, _clDoseSec);

    html += "</body></html>";
    request->send(200, "text/html", html);
  });

  _server.on("/set", HTTP_GET, [this](AsyncWebServerRequest *request) {
    if (request->hasParam("dev") && request->hasParam("m")) {
      String dev = request->getParam("dev")->value();
      DeviceMode m = (DeviceMode)request->getParam("m")->value().toInt();
      if (dev == "pool") _pool.mode = m;
      else if (dev == "ph") _ph.mode = m;
      else if (dev == "cl") _cl.mode = m;
    }
    request->send(200, "text/plain", "OK");
  });

  // Der Status-Endpunkt liefert nun auch den Modus (0, 1 oder 2)
  _server.on("/status", HTTP_GET, [this](AsyncWebServerRequest *request) {
    String j = "{ \"time\":\"" + _timeSync->getFormattedTime() + "\",";
    j += "\"pool\":{\"state\":" + String(digitalRead(_pool.pin)) + ",\"mode\":" + String(_pool.mode) + "},";
    j += "\"ph\":{\"state\":" + String(digitalRead(_ph.pin)) + ",\"mode\":" + String(_ph.mode) + "},";
    j += "\"cl\":{\"state\":" + String(digitalRead(_cl.pin)) + ",\"mode\":" + String(_cl.mode) + "} }";
    request->send(200, "application/json", j);
  });

  _server.begin();
}

// Hilfsfunktion zur Zeitprüfung (Stunde, Minute, Sekunde)
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
  int h = _timeSync->getHour();
  int m = _timeSync->getMinute();
  int s = _timeSync->getSecond();

  // 1. Poolpumpe (Master)
  bool poolShouldRun = false;
  if (_pool.mode == MODE_ON) {
    isPoolpumpActiv = true;
    poolShouldRun = true;
  } else if (_pool.mode == MODE_OFF) {
    isPoolpumpActiv = false;
    poolShouldRun = false;
    }
  else if (_pool.mode == MODE_AUTO) {
    for (int i = 0; i < _pool.activeIntervals; i++) {
      if (isInside(h, m, s, _pool.times[i])) {
        isPoolpumpActiv = true;
        poolShouldRun = true;
      }
      else isPoolpumpActiv = false;
    }
  }
  digitalWrite(_pool.pin, poolShouldRun);

  // 2. Sicherheits-Verriegelung (Ist die Poolpumpe wirklich AN?)
  bool flowOk = digitalRead(_pool.pin);

  // 3. Chemie-Pumpen (PH & Chlor), Dauer über _phDoseSec/_clDoseSec einstellbar
  auto processChemie = [&](Device &d, int durationSec) {
    bool run = false;
    if (d.mode == MODE_ON) run = true;
    else if (d.mode == MODE_OFF) run = false;
    else if (d.mode == MODE_AUTO) {
      // Läuft im Auto-Modus NUR wenn auch die Poolpumpe läuft
      if (flowOk && isInside(h, m, s, d.times[0], durationSec)) run = true;
    }
    // Zusätzliche harte Verriegelung: Chemie IMMER AUS wenn Pool AUS
    digitalWrite(d.pin, (run && flowOk) ? HIGH : LOW);
  };

  processChemie(_ph, _phDoseSec);
  processChemie(_cl, _clDoseSec);
}