

#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>
#include <ESP32Servo.h>


const char* SSID         = "SEU_WIFI";
const char* WIFI_PASS    = "SUA_SENHA";

const char* BACKEND_URL  = "http://192.168.0.100:5000/leituras";


const char* WHATSAPP_NUM = "55SEU_NUMERO";   
const char* CALLMEBOT_KEY = "SUA_APIKEY";


#define DHTPIN         4      
#define DHTTYPE        DHT22

#define LDR_PIN        34     
#define PIR_PIN        27     

#define LED_VERDE      25
#define LED_VERMELHO   26

#define SERVO_ESQ_PIN  18     
#define SERVO_DIR_PIN  19     


#define JOY_X_PIN      35     
#define JOY_Y_PIN      32     
#define JOY_BTN_PIN    33     


#define LUZ_LIMITE     500    
#define ALERTA_LIMITE  75     

DHT dht(DHTPIN, DHTTYPE);
Servo servoEsq;
Servo servoDir;

bool roboLigado = true;
unsigned long ultimoEnvio = 0;
const unsigned long INTERVALO = 2000; // 2 segundos


void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(PIR_PIN,        INPUT);
  pinMode(LED_VERDE,      OUTPUT);
  pinMode(LED_VERMELHO,   OUTPUT);
  pinMode(JOY_BTN_PIN,    INPUT_PULLUP);

  servoEsq.attach(SERVO_ESQ_PIN);
  servoDir.attach(SERVO_DIR_PIN);
  pararMotores();


  dht.begin();


  conectarWiFi();

  Serial.println("=== ROBÔ EXPLORADOR INICIADO ===");
  digitalWrite(LED_VERDE, HIGH);
}


void loop() {
  if (digitalRead(JOY_BTN_PIN) == LOW) {
    roboLigado = !roboLigado;
    delay(300); 
    if (!roboLigado) {
      pararMotores();
      digitalWrite(LED_VERDE,    LOW);
      digitalWrite(LED_VERMELHO, HIGH);
      Serial.println(">>> Robô DESLIGADO pelo joystick. <<<");
    } else {
      digitalWrite(LED_VERMELHO, LOW);
      digitalWrite(LED_VERDE,    HIGH);
      Serial.println(">>> Robô LIGADO. <<<");
    }
  }

  if (roboLigado) {
    controlarMotores();
  }

  if (millis() - ultimoEnvio >= INTERVALO) {
    ultimoEnvio = millis();
    lerESalvar();
  }
}


void lerESalvar() {
  // Leituras
  float temperatura = dht.readTemperature();
  float umidade     = dht.readHumidity();
  int   luminosidade = analogRead(LDR_PIN);
  bool  presenca    = digitalRead(PIR_PIN);

  // Valida DHT
  if (isnan(temperatura) || isnan(umidade)) {
    Serial.println("[ERRO] DHT22 não respondeu. Pulando ciclo.");
    return;
  }


  int prob = calcularProbabilidade(temperatura, umidade, luminosidade, presenca);

  // Estado do robô
  String estado;
  if (!roboLigado)        estado = "desligado";
  else if (prob > ALERTA_LIMITE) estado = "alerta";
  else                    estado = "ligado";


  Serial.println("----------------------------------");
  Serial.printf("Temperatura : %.1f °C\n", temperatura);
  Serial.printf("Umidade     : %.1f %%\n", umidade);
  Serial.printf("Luminosidade: %d\n", luminosidade);
  Serial.printf("Presença    : %s\n", presenca ? "Detectada" : "Sem presença");
  Serial.printf("Estado      : %s\n", estado.c_str());
  Serial.printf("Prob. vida  : %d%%\n", prob);


  if (roboLigado) {
    if (prob > ALERTA_LIMITE) {
      digitalWrite(LED_VERDE,    LOW);
      digitalWrite(LED_VERMELHO, HIGH);
      Serial.println("ALERTA! Alta probabilidade de vida detectada!");
      enviarWhatsApp("Alerta! Alta probabilidade de vida detectada no planeta.");
    } else {
      digitalWrite(LED_VERDE,    HIGH);
      digitalWrite(LED_VERMELHO, LOW);
      Serial.println("Exploração normal. Nenhum indício relevante detectado.");
    }
  }


  enviarBackend(temperatura, umidade, luminosidade, presenca, (float)prob);
}

int calcularProbabilidade(float temp, float umid, int luz, bool presenca) {
  int prob = 0;
  if (temp >= 15.0 && temp <= 30.0)  prob += 25;
  if (umid >= 40.0 && umid <= 70.0)  prob += 25;
  if (luz > LUZ_LIMITE)              prob += 20;
  if (presenca)                       prob += 30;
  return prob;
}


void controlarMotores() {
  int x = analogRead(JOY_X_PIN);
  int y = analogRead(JOY_Y_PIN); 

  // Zona morta central (evita tremido)
  int centroMin = 1800, centroMax = 2200;

  int velEsq = 90, velDir = 90; 


  if (y < centroMin) {

    int vel = map(y, centroMin, 0, 0, 45);
    velEsq = 90 + vel;
    velDir = 90 - vel;
  } else if (y > centroMax) {
  
    int vel = map(y, centroMax, 4095, 0, 45);
    velEsq = 90 - vel;
    velDir = 90 + vel;
  }

  
  if (x < centroMin) {]

    int delta = map(x, centroMin, 0, 0, 30);
    velEsq -= delta;
    velDir += delta;
  } else if (x > centroMax) {

    int delta = map(x, centroMax, 4095, 0, 30);
    velEsq += delta;
    velDir -= delta;
  }

  velEsq = constrain(velEsq, 45, 135);
  velDir = constrain(velDir, 45, 135);

  servoEsq.write(velEsq);
  servoDir.write(velDir);
}

void pararMotores() {
  servoEsq.write(90);
  servoDir.write(90);
}

void enviarBackend(float temp, float umid, int luz, bool presenca, float prob) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[WIFI] Sem conexão. Dado não enviado.");
    return;
  }

  HTTPClient http;
  http.begin(BACKEND_URL);
  http.addHeader("Content-Type", "application/json");

  String json = "{";
  json += "\"temperatura_c\":" + String(temp, 1) + ",";
  json += "\"umidade_pct\":"   + String(umid, 1) + ",";
  json += "\"luminosidade\":"  + String(luz)     + ",";
  json += "\"presenca\":"      + String(presenca ? 1 : 0) + ",";
  json += "\"probabilidade_vida\":" + String(prob, 1);
  json += "}";

  int resposta = http.POST(json);
  if (resposta > 0) {
    Serial.printf("[HTTP] Dados enviados. Resposta: %d\n", resposta);
  } else {
    Serial.printf("[HTTP] Falha no envio: %s\n", http.errorToString(resposta).c_str());
  }
  http.end();
}


void enviarWhatsApp(String mensagem) {
  if (WiFi.status() != WL_CONNECTED) return;


  mensagem.replace(" ", "%20");

  String url = "https://api.callmebot.com/whatsapp.php?phone=";
  url += WHATSAPP_NUM;
  url += "&text=" + mensagem;
  url += "&apikey=" + String(CALLMEBOT_KEY);

  HTTPClient http;
  http.begin(url);
  int resposta = http.GET();
  Serial.printf("[WhatsApp] Resposta: %d\n", resposta);
  http.end();
}

void conectarWiFi() {
  Serial.printf("Conectando ao Wi-Fi: %s", SSID);
  WiFi.begin(SSID, WIFI_PASS);
  int tentativas = 0;
  while (WiFi.status() != WL_CONNECTED && tentativas < 20) {
    delay(500);
    Serial.print(".");
    tentativas++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("\nConectado! IP: %s\n", WiFi.localIP().toString().c_str());
  } else {
    Serial.println("\n[ERRO] Não foi possível conectar ao Wi-Fi.");
  }
}
