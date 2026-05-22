//Integrantes: Enzo Carlo, João Henrique Macedo e Melina Mesquita

#include <ESP32Servo.h>

// Definição dos Pinos
const int VERT_PIN = 34;
const int HORZ_PIN = 35;
const int SERVOX_PIN = 23;
const int SERVOY_PIN = 22;
const int LED_VERDE_PIN = 18;
const int LED_VERMELHO_PIN = 19;
const int BOTAO_PIN = 21;

// Instanciação do Servo
Servo ServoX;
Servo ServoY;

const int Limite_Superior = 3000;
const int Limite_Inferior = 1000;

// Variáveis de Estado
bool roboLigado = true;
String ultimoComando = "";

// Controle de debounce
unsigned long ultimoClique = 0;
const int tempoDebounce = 300;

void setup() {
  Serial.begin(115200);
  
  // Configuração dos Pinos
  pinMode(BOTAO_PIN, INPUT_PULLUP);
  pinMode(LED_VERDE_PIN, OUTPUT);
  pinMode(LED_VERMELHO_PIN, OUTPUT);
  
  // Inicialização do Servo
  ServoX.attach(SERVOX_PIN);
  ServoY.attach(SERVOY_PIN);
  
  // Estado Inicial: Ligado
  digitalWrite(LED_VERDE_PIN, HIGH);
  digitalWrite(LED_VERMELHO_PIN, LOW);

  ServoX.write(90);
  ServoY.write(90);

  Serial.println("Sistema iniciado");

}

void loop() {
  if (digitalRead(BOTAO_PIN) == LOW &&
      millis() - ultimoClique > tempoDebounce) {

    ultimoClique = millis();

    roboLigado = false;

    digitalWrite(LED_VERDE_PIN, LOW);
    digitalWrite(LED_VERMELHO_PIN, HIGH);

    ServoX.write(90);
    ServoY.write(90);

    Serial.println("Comando enviado: DESLIGAR");
    Serial.println("Status: Robô DESLIGADO");
  }

  // Movimento do Robô
  if (roboLigado) {
    int valorX = analogRead(HORZ_PIN);
    int valorY = analogRead(VERT_PIN);

    String comandoAtual = "Parado";

    // Frente
    if (valorY > Limite_Superior) {
      comandoAtual = "Frente";
      ServoX.write(180);
      ServoY.write(180);
    }
    // Trás
    else if (valorY < Limite_Inferior) {
      comandoAtual = "Trás";
      ServoX.write(0);
      ServoY.write(0);
    }
    // Direita
    else if (valorX > Limite_Superior) {
      comandoAtual = "Direita";
      // Apenas motor esquerdo ativo
      ServoX.write(180);
      ServoY.write(90);
    }
    // Esquerda
    else if (valorX < Limite_Inferior) {
      comandoAtual = "Esquerda";
      // Apenas motor direito ativo
      ServoX.write(90);
      ServoY.write(180);
    }
    // Parado
    else {
      ServoX.write(90);
      ServoY.write(90);
    }

    // Mostrar apenas quando mudar comando
    if (comandoAtual != ultimoComando) {

      Serial.print("Comando: ");
      Serial.println(comandoAtual);

      Serial.print("Status: ");
      Serial.println("Robô LIGADO");

      ultimoComando = comandoAtual;
    }
  }

  delay(100);
}
