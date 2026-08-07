# Robô Explorador — ESP32 + Python + SQLite

> Projeto acadêmico de um robô explorador autônomo capaz de detectar condições ambientais,
> calcular a probabilidade de existência de vida, emitir alertas via WhatsApp e armazenar
> todos os dados coletados em um banco de dados local.

---

## Sobre o Projeto

O **Robô Explorador** é um sistema embarcado desenvolvido com o microcontrolador **ESP32 WROVER**,
projetado para simular uma missão de exploração planetária. O robô coleta dados do ambiente
em tempo real através de sensores físicos, processa essas informações localmente e toma decisões
autônomas com base em um algoritmo simples de aprendizado de máquina.

Quando a probabilidade de vida detectada ultrapassa 75%, o robô entra em modo de alerta:
acende o LED vermelho, para os motores e envia automaticamente uma notificação via WhatsApp
usando a API do CallmeBot.

Todos os dados coletados são enviados via Wi-Fi para um backend Python rodando localmente,
onde são armazenados em um banco de dados SQLite e exibidos em um dashboard web em tempo real.

---

##  Hardware Utilizado

| Componente              | Função                                                   |
|-------------------------|----------------------------------------------------------|
| ESP32 WROVER             | Microcontrolador principal — processa e transmite dados  |
| DHT22                   | Mede temperatura (°C) e umidade relativa (%)             |
| Fotorresistor (LDR)     | Mede a intensidade de luz do ambiente                    |
| Sensor PIR              | Detecta presença/movimento no entorno do robô            |
| LED Verde                | Indica que o robô está operando normalmente              |
| LED Vermelho             | Indica alerta ou que o robô foi desligado remotamente    |
| Servo Motor Esquerdo    | Controla a roda esquerda do robô                         |
| Servo Motor Direito     | Controla a roda direita do robô                          |
| Joystick Analógico      | Controla a direção do robô manualmente                   |

---

##  Mapeamento de Pinos — ESP32 WROVER

| Componente      | Pino GPIO | Observação                          |
|-----------------|-----------|-------------------------------------|
| DHT22           | 4         | Resistor 10kΩ entre VCC e DATA      |
| LDR             | 34        | ADC somente leitura — divisor de tensão |
| PIR             | 27        | Alimentar com 5V                    |
| LED Verde       | 25        | Resistor 220Ω em série              |
| LED Vermelho    | 26        | Resistor 220Ω em série              |
| Servo Esquerdo  | 18        | Sinal PWM                           |
| Servo Direito   | 19        | Sinal PWM                           |
| Joystick Eixo X | 35        | ADC somente leitura                 |
| Joystick Eixo Y | 32        | ADC somente leitura                 |
| Joystick Botão  | 33        | INPUT_PULLUP — pressionar desliga o robô |

>  Os pinos 6–11 e 16–17 do ESP32 WROVER são reservados para flash e PSRAM internos.
> Nunca os utilize para periféricos externos.

---

##  Como Funciona?

### Coleta de Dados
A cada 2 segundos, o ESP32 lê todos os sensores e exibe os valores no Monitor Serial:
- Temperatura em °C
- Umidade em %
- Intensidade de luz (valor analógico 0–4095)
- Estado do sensor de presença (detectada / sem presença)
- Estado do robô (ligado / desligado / alerta)
- Probabilidade de vida calculada (%)

### Algoritmo de Probabilidade de Vida
O ESP32 usa as leituras dos sensores para calcular uma pontuação de 0% a 100%:

| Condição                          | Pontos |
|-----------------------------------|--------|
| Temperatura entre 15°C e 30°C    | +25%   |
| Umidade entre 40% e 70%          | +25%   |
| Luminosidade acima do limite      | +20%   |
| Presença detectada pelo PIR       | +30%   |

### Tomada de Decisão
- **Probabilidade ≤ 75%** → LED verde aceso, operação normal
- **Probabilidade > 75%** → LED vermelho aceso, alerta no Monitor Serial e mensagem no WhatsApp

### Controle pelos Motores
O joystick analógico controla os dois servos de roda contínua em tempo real.
Pressionar o botão do joystick desliga/liga o robô remotamente.

---

##  Como Rodar o Backend Python

### Pré-requisitos
- Python 3.x instalado ([python.org](https://www.python.org/downloads/))
- Flask instalado

### Instalação

```bash
pip install flask
```

### Executar o servidor

```bash
python backend.py
```

O terminal deve exibir:
```
[DB] Banco de dados pronto: robo_explorador.db
[SERVER] Rodando em http://0.0.0.0:5000
```

### Acessar o Dashboard

Abra o navegador e acesse:
```
http://localhost:5000
```

O dashboard exibe as últimas 100 leituras do robô em tempo real, atualizando automaticamente
a cada 5 segundos. Leituras com probabilidade de vida acima de 75% são destacadas em vermelho.

### Endpoints da API

| Método | Rota        | Descrição                              |
|--------|-------------|----------------------------------------|
| POST   | `/leituras` | Recebe dados do ESP32 (JSON)           |
| GET    | `/leituras` | Retorna as últimas 100 leituras (JSON) |
| GET    | `/`         | Dashboard web                          |

---

##  Banco de Dados

O banco de dados é um arquivo SQLite (`robo_explorador.db`) criado automaticamente
na mesma pasta do `backend.py` quando o servidor é iniciado pela primeira vez.

### Estrutura da Tabela `leituras`

```sql
CREATE TABLE IF NOT EXISTS leituras (
    id                 INTEGER PRIMARY KEY AUTOINCREMENT,
    timestamp          TEXT    NOT NULL,
    temperatura_c      REAL    NOT NULL,
    umidade_pct        REAL    NOT NULL,
    luminosidade       INTEGER NOT NULL,
    presenca           INTEGER NOT NULL,
    probabilidade_vida REAL    NOT NULL
);
```

### Como Consultar os Dados Salvos

Instale o SQLite em [sqlite.org/download.html](https://sqlite.org/download.html) e rode:

```bash
sqlite3 robo_explorador.db
```

Comandos úteis dentro do SQLite:

```sql
-- Ver as 10 leituras mais recentes
SELECT * FROM leituras ORDER BY id DESC LIMIT 10;

-- Ver apenas leituras de alerta (prob. > 75%)
SELECT * FROM leituras WHERE probabilidade_vida > 75;

-- Contar total de leituras salvas
SELECT COUNT(*) FROM leituras;

-- Sair
.quit
```

---

##  Configurar Alertas via WhatsApp (CallmeBot)

1. Adicione o número **+34 644 78 33 97** aos contatos do WhatsApp
2. Envie a mensagem: `I allow callmebot to send me messages`
3. Aguarde a resposta automática com sua **APIKEY**
4. Insira a APIKEY no firmware do ESP32

---

## 🔧 Configuração do Firmware

Antes de gravar o firmware no ESP32, edite as seguintes linhas no topo do arquivo `firmware_robo.ino`:

```cpp
const char* SSID          = "nome_da_sua_rede_wifi";
const char* WIFI_PASS     = "senha_do_wifi";
const char* BACKEND_URL   = "http://SEU_IP_LOCAL:5000/leituras";
const char* WHATSAPP_NUM  = "55DDD9XXXXXXXX";
const char* CALLMEBOT_KEY = "sua_apikey_aqui";
```

Para descobrir o IP local do seu PC, abra o terminal e execute:
- **Windows:** `ipconfig` → procure "Endereço IPv4"
- **Linux/Mac:** `hostname -I`

### Bibliotecas Necessárias (Arduino IDE)

Instale via `Sketch → Include Library → Manage Libraries`:
- `DHT sensor library` (Adafruit)
- `Adafruit Unified Sensor`
- `ESP32Servo`

### Placa e Porta

- **Board:** `Tools → Board → ESP32 Arduino → ESP32 Wrover Module`
- **Port:** `Tools → Port → COMx`
- **Baud Rate (Monitor Serial):** `115200`

---

##  Estrutura do Repositório

```
robo-explorador/
├── firmware_robo.ino     # Código do ESP32
├── backend.py            # Servidor Flask + banco de dados
├── schema.sql            # Script SQL de criação da tabela
└── README.md             # Este arquivo
```

---

##  Tecnologias Utilizadas

- **C++ / Arduino Core** — Firmware do ESP32
- **Python 3 + Flask** — Backend e API REST
- **SQLite** — Banco de dados local
- **CallmeBot API** — Notificações via WhatsApp
- **Wi-Fi HTTP** — Comunicação ESP32 ↔ Backend

---

## Disciplina

Projeto desenvolvido para a disciplina de **Sistemas Embarcados / IoT**
como parte do curso de Engenharia de Computação.
