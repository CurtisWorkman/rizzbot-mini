#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <FluxGarage_RoboEyes.h>
#include "DFPlayerRandomSequencer.h"

/* ================= OLED ================= */
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_SDA 7
#define OLED_SCL 6

#define SERIAL_RX 5
#define SERIAL_TX 4

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
RoboEyes<Adafruit_SSD1306> roboEyes(display);
DFPlayerRandomSequencer soundPlayer(Serial1);

/* ================= MOTOR PIN ================= */
#define LF   0
#define LB   1
#define RF   2
#define RB   3
#define STBY 10

/* ================= WIFI ================= */
WebServer server(80);
DNSServer dnsServer;

/* ================= CONTROL MODE ================= */
enum ControlMode {
  CONTROL_MANUAL,
  CONTROL_AUTO
};

volatile ControlMode controlMode = CONTROL_MANUAL;

/* ================= RANDOM MODE (for auto control) ================= */
enum RandomMode {
  RANDOM_OFF,
  RANDOM_SOFT,
  RANDOM_NORMAL
};

volatile RandomMode randomMode = RANDOM_NORMAL;

/* ================= MOTOR CONTROL ================= */
void motorControl(byte c) {
  digitalWrite(STBY, HIGH);
  switch (c) {
    case 0:  // STOP
      digitalWrite(LF,LOW); digitalWrite(LB,LOW);
      digitalWrite(RF,LOW); digitalWrite(RB,LOW);
      break;
    case 1:  // FORWARD
      digitalWrite(LF,HIGH); digitalWrite(LB,LOW);
      digitalWrite(RF,LOW);  digitalWrite(RB,HIGH);
      break;
    case 2:  // BACKWARD
      digitalWrite(LF,LOW);  digitalWrite(LB,HIGH);
      digitalWrite(RF,HIGH); digitalWrite(RB,LOW);
      break;
    case 3:  // LEFT
      digitalWrite(LF,LOW);  digitalWrite(LB,HIGH);
      digitalWrite(RF,LOW);  digitalWrite(RB,HIGH);
      break;
    case 4:  // RIGHT
      digitalWrite(LF,HIGH); digitalWrite(LB,LOW);
      digitalWrite(RF,HIGH); digitalWrite(RB,LOW);
      break;
  }
}

/* ================= AUTO MOTOR (with PWM timing) ================= */
void MOTOR(byte c,int t1,int t2,int Time){
  for(int i=0;i<Time;i++){
    switch (c) {
      case 0: digitalWrite(LF,LOW); digitalWrite(LB,LOW); digitalWrite(RF,LOW); digitalWrite(RB,LOW); break;
      case 1: digitalWrite(LF,LOW); digitalWrite(LB,HIGH);digitalWrite(RF,LOW); digitalWrite(RB,HIGH);break;
      case 2: digitalWrite(LF,HIGH);digitalWrite(LB,LOW); digitalWrite(RF,HIGH);digitalWrite(RB,LOW); break;
      case 3: digitalWrite(LF,LOW); digitalWrite(LB,HIGH);digitalWrite(RF,HIGH);digitalWrite(RB,LOW); break;
      case 4: digitalWrite(LF,HIGH);digitalWrite(LB,LOW); digitalWrite(RF,LOW); digitalWrite(RB,HIGH);break;
      case 5: digitalWrite(LF,LOW); digitalWrite(LB,HIGH);digitalWrite(RF,LOW); digitalWrite(RB,LOW); break;
      case 6: digitalWrite(LF,LOW); digitalWrite(LB,LOW); digitalWrite(RF,LOW); digitalWrite(RB,HIGH);break;
      case 7: digitalWrite(LF,HIGH);digitalWrite(LB,LOW); digitalWrite(RF,LOW); digitalWrite(RB,LOW); break;
      case 8: digitalWrite(LF,LOW); digitalWrite(LB,LOW); digitalWrite(RF,HIGH);digitalWrite(RB,LOW); break;
    }

    delay(t1);

    digitalWrite(LF,LOW); digitalWrite(LB,LOW);
    digitalWrite(RF,LOW); digitalWrite(RB,LOW);

    delay(t2);
  }
}

void playSound(int file) {
  soundPlayer.playFolderSound(2, file);
}

/* ================= WEB UI ================= */
void handleRoot() {
  String page = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>
body{
  margin:0;min-height:100vh;
  background:radial-gradient(circle at top,#0f2027,#000);
  color:#00ffe1;
  font-family:Arial;
  display:flex;align-items:center;justify-content:center;
  padding:20px 0;
}
.container{
  display:flex;
  flex-direction:column;
  gap:20px;
  max-width:800px;
}
.panel{
  width:260px;
  padding:20px;
  border-radius:18px;
  background:rgba(0,255,225,0.05);
  border:1px solid rgba(0,255,225,0.4);
  box-shadow:0 0 25px rgba(0,255,225,0.3);
}
.sound-panel{
  width:auto;
  min-width:300px;
}
h2{text-align:center;margin:0 0 14px;letter-spacing:2px;font-size:18px;}
h3{text-align:center;margin:0 0 10px;letter-spacing:1px;font-size:14px;opacity:0.8;}
.grid{
  display:grid;
  grid-template-columns:1fr 1fr 1fr;
  grid-template-rows:60px 60px 60px;
  gap:10px;
}
.sound-grid{
  display:grid;
  grid-template-columns:repeat(5, 1fr);
  gap:8px;
}
button{
  border:none;border-radius:12px;
  font-size:16px;font-weight:bold;
  background:linear-gradient(145deg,#0ff,#00b3a4);
  cursor:pointer;
  transition: all 0.2s;
}
button:active{
  transform:scale(0.95);
}
.stop{background:linear-gradient(145deg,#ff5555,#aa0000);color:#fff;}
.empty{background:none;}
.sound-btn{
  height:50px;
  font-size:14px;
  background:linear-gradient(145deg,#ff9500,#c77300);
  color:#fff;
}

.control-toggle{
  margin-bottom:12px;
  display:flex;
  gap:6px;
}
.control-toggle button{
  flex:1;
  font-size:14px;
  opacity:0.6;
  height:45px;
}
.control-toggle button.active{
  opacity:1;
  background:linear-gradient(145deg,#00d4ff,#0088cc);
  box-shadow:0 0 15px rgba(0,212,255,0.8);
}

.mode{
  margin-top:12px;
  display:flex;
  gap:6px;
}
.mode button{
  flex:1;
  font-size:13px;
  opacity:0.6;
}
.mode button.active{
  opacity:1;
  background:linear-gradient(145deg,#00ff9c,#00c46a);
  box-shadow:0 0 12px rgba(0,255,180,0.8);
}
.mode button:disabled{
  opacity:0.3;
  cursor:not-allowed;
}

.status{
  text-align:center;
  margin-top:10px;
  font-size:12px;
  opacity:0.7;
}

@media (min-width: 600px) {
  .container{
    flex-direction:row;
    align-items:flex-start;
  }
}
</style>
</head>
<body>

<div class="container">

<div class="panel">
<h2>RI|| BOT MINI</h2>

<!-- Control Mode Toggle -->
<div class="control-toggle">
  <button id="btn_manual" class="active" onclick="setControlMode('manual')">MANUAL</button>
  <button id="btn_auto" onclick="setControlMode('auto')">AUTO</button>
</div>

<!-- Manual Controls -->
<div id="manual_controls">
<h3>MANUAL CONTROL</h3>
<div class="grid">
  <div class="empty"></div>
  <button onclick="fetch('/f')">UP</button>
  <div class="empty"></div>

  <button onclick="fetch('/l')">LEFT</button>
  <button class="stop" onclick="fetch('/s')">STOP</button>
  <button onclick="fetch('/r')">RIGHT</button>

  <div class="empty"></div>
  <button onclick="fetch('/b')">DOWN</button>
  <div class="empty"></div>
</div>
</div>

<!-- Auto Mode Settings -->
<div id="auto_settings" style="display:none;">
<h3>AUTO MODE</h3>
<div class="mode">
  <button id="btn_sleep" onclick="setRandomMode('off')">SLEEP</button>
  <button id="btn_wiggle" onclick="setRandomMode('soft')">WIGGLE</button>
  <button id="btn_curious" class="active" onclick="setRandomMode('normal')">CURIOUS</button>
</div>
<div class="status" id="auto_status">Robot moving randomly</div>
</div>

</div>

<div class="panel sound-panel">
<h2>SOUND EFFECTS</h2>
<div class="sound-grid">
  <button class="sound-btn" onclick="fetch('/sound1')">1</button>
  <button class="sound-btn" onclick="fetch('/sound2')">2</button>
  <button class="sound-btn" onclick="fetch('/sound3')">3</button>
  <button class="sound-btn" onclick="fetch('/sound4')">4</button>
  <button class="sound-btn" onclick="fetch('/sound5')">5</button>
  <button class="sound-btn" onclick="fetch('/sound6')">6</button>
  <button class="sound-btn" onclick="fetch('/sound7')">7</button>
  <button class="sound-btn" onclick="fetch('/sound8')">8</button>
  <button class="sound-btn" onclick="fetch('/sound9')">9</button>
  <button class="sound-btn" onclick="fetch('/sound10')">10</button>
  <button class="sound-btn" onclick="fetch('/sound11')">11</button>
  <button class="sound-btn" onclick="fetch('/sound12')">12</button>
  <button class="sound-btn" onclick="fetch('/sound13')">13</button>
  <button class="sound-btn" onclick="fetch('/sound14')">14</button>
  <button class="sound-btn" onclick="fetch('/sound15')">15</button>
</div>
</div>

</div>

<script>
function setControlMode(mode){
  fetch('/control_' + mode);
  
  // Update toggle buttons
  document.getElementById('btn_manual').classList.remove('active');
  document.getElementById('btn_auto').classList.remove('active');
  
  if(mode === 'manual'){
    document.getElementById('btn_manual').classList.add('active');
    document.getElementById('manual_controls').style.display = 'block';
    document.getElementById('auto_settings').style.display = 'none';
  } else {
    document.getElementById('btn_auto').classList.add('active');
    document.getElementById('manual_controls').style.display = 'none';
    document.getElementById('auto_settings').style.display = 'block';
  }
}

function setRandomMode(mode){
  fetch('/mode_' + mode);
  
  document.getElementById('btn_sleep').classList.remove('active');
  document.getElementById('btn_wiggle').classList.remove('active');
  document.getElementById('btn_curious').classList.remove('active');

  if(mode === 'off'){
    document.getElementById('btn_sleep').classList.add('active');
    document.getElementById('auto_status').textContent = 'Robot sleeping';
  }
  if(mode === 'soft'){
    document.getElementById('btn_wiggle').classList.add('active');
    document.getElementById('auto_status').textContent = 'Robot wiggling gently';
  }
  if(mode === 'normal'){
    document.getElementById('btn_curious').classList.add('active');
    document.getElementById('auto_status').textContent = 'Robot exploring curiously';
  }
}
</script>

</body>
</html>
)rawliteral";

  server.send(200, "text/html", page);
}

/* ================= SERVER ================= */
void setupServer() {
  server.on("/", handleRoot);

  // Control mode toggle
  server.on("/control_manual", [](){ 
    controlMode = CONTROL_MANUAL;
    motorControl(0); // Stop motors when switching to manual
    soundPlayer.stopSequencing(); // Stop auto sound sequences
    soundPlayer.setVolume(30);
    server.send(200); 
  });
  
  server.on("/control_auto", [](){ 
    soundPlayer.setVolume(15);
    controlMode = CONTROL_AUTO;
    // Start sequencing if in RANDOM_NORMAL mode
    if(randomMode == RANDOM_NORMAL) {
      soundPlayer.startSequencing();
    }
    server.send(200); 
  });

  // Manual control endpoints (only work in manual mode)
  server.on("/f", [](){ 
    if(controlMode == CONTROL_MANUAL) motorControl(1); 
    server.send(200); 
  });
  server.on("/b", [](){ 
    if(controlMode == CONTROL_MANUAL) motorControl(2); 
    server.send(200); 
  });
  server.on("/l", [](){ 
    if(controlMode == CONTROL_MANUAL) motorControl(3); 
    server.send(200); 
  });
  server.on("/r", [](){ 
    if(controlMode == CONTROL_MANUAL) motorControl(4); 
    server.send(200); 
  });
  server.on("/s", [](){ 
    if(controlMode == CONTROL_MANUAL) motorControl(0); 
    server.send(200); 
  });

  // Random mode settings (only affect auto mode)
  server.on("/mode_off", [](){ 
    randomMode = RANDOM_OFF; 
    setRobotMood(RANDOM_OFF);
    server.send(200); 
  });
  
  server.on("/mode_soft", [](){ 
    randomMode = RANDOM_SOFT; 
    setRobotMood(RANDOM_SOFT);
    server.send(200); 
  });
  
  server.on("/mode_normal", [](){ 
    randomMode = RANDOM_NORMAL; 
    setRobotMood(RANDOM_NORMAL);
    server.send(200); 
  });

  // Sound playback endpoints (work in both modes)
  for (int i = 1; i <= 15; i++) {
    String path = "/sound" + String(i);
    server.on(path.c_str(), [i]() {
      playSound(i);
      server.send(200);
    });
  }

  server.onNotFound(handleRoot);
  server.begin();
}

void setRobotMood(RandomMode mode) {
  switch (mode) {
    case RANDOM_OFF:
      roboEyes.setMood(DEFAULT);
      roboEyes.setHeight(5,5); 
      roboEyes.setCuriosity(false);
      roboEyes.setAutoblinker(ON,5,2);
      roboEyes.setIdleMode(ON,5,2);
      soundPlayer.stopSequencing();
      break;
    case RANDOM_SOFT:
      roboEyes.setMood(HAPPY);
      roboEyes.setHeight(30,30); 
      roboEyes.setCuriosity(false);
      roboEyes.setAutoblinker(ON,3,2);
      roboEyes.setIdleMode(ON,2,2);
      soundPlayer.stopSequencing();
      break;
    case RANDOM_NORMAL:
      roboEyes.setMood(DEFAULT);
      roboEyes.setHeight(30,30); 
      roboEyes.setCuriosity(false);
      roboEyes.setAutoblinker(ON,3,2);
      roboEyes.setIdleMode(ON,2,2);
      soundPlayer.startSequencing();
      break;
  }
}

/* ================= SETUP ================= */
void setup() {
  pinMode(STBY,OUTPUT); digitalWrite(STBY,LOW);
  pinMode(LF,OUTPUT); pinMode(LB,OUTPUT);
  pinMode(RF,OUTPUT); pinMode(RB,OUTPUT);

  Wire.begin(OLED_SDA, OLED_SCL);
  display.begin(SSD1306_SWITCHCAPVCC,0x3C);
  display.clearDisplay(); display.display();

  roboEyes.begin(SCREEN_WIDTH,SCREEN_HEIGHT,100);
  setRobotMood(RANDOM_NORMAL);

  randomSeed(esp_random());

  // Sound player configuration
  if (!soundPlayer.begin(SERIAL_RX, SERIAL_TX, 15)) {
    Serial.println(F("Failed to initialize voice module!"));
  }

  soundPlayer.enableDebug(true);
  
  soundPlayer.setSequenceInterval(10000);
  soundPlayer.setSoundDelayRange(80, 600);
  soundPlayer.setSequenceLengthRange(3, 8);
  soundPlayer.setMaxSoundNumber(50);
  soundPlayer.setFolder(1);
  soundPlayer.setVolume(15);

  WiFi.softAP("RizzBot Mini");
  dnsServer.start(53,"*",WiFi.softAPIP());
  setupServer();

  digitalWrite(STBY,HIGH);
}

/* ================= LOOP ================= */
void loop() {
  roboEyes.update();
  soundPlayer.update();
  server.handleClient();
  dnsServer.processNextRequest();

  // Auto movement only runs in AUTO mode
  if (controlMode == CONTROL_AUTO) {
    static unsigned long lastTick = 0;
    if (millis() - lastTick > 40) {
      lastTick = millis();

      if (randomMode == RANDOM_SOFT) {
        if (random(120) == 1) {
          MOTOR(random(9), random(6,18), random(40,90), 1);
        }
      }
      else if (randomMode == RANDOM_NORMAL) {
        if (random(100) == 1) {
          MOTOR(random(9), random(5,50), random(10,100), random(20));
        }
      }
    }
  }
}