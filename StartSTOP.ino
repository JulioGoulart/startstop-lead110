// =========================================================
// AMBIENTE DE COMPILAÇÃO E VERSÕES DAS BIBLIOTECAS
// =========================================================
// Este código foi desenvolvido e testado com as seguintes 
// versões de ferramentas e bibliotecas. O uso de versões mais
// recentes pode apresentar problema de compatibilidade.
// Código construido ao som de "repulsive in its splendid beauty" - Crowbar
// Placa: ESP32 Dev Module (FQBN: esp32:esp32:esp32)
// Core ESP32 (Arduino):      v2.0.14
// Biblioteca WiFi:            v2.0.0
// Biblioteca Wire:            v2.0.0
// Biblioteca Blynk:           v1.3.2
// Biblioteca LiquidCrystal_I2C: v2.0.0
// =========================================================

// =========================================================
// 1. BIBLIOTECAS E CONFIGURAÇÕES GLOBAIS
// =========================================================

// DEFINIÇÕES DO BLYNK

#define BLYNK_TEMPLATE_ID   "TEMPLATE_ID_AQUI"
#define BLYNK_TEMPLATE_NAME "TEMPLATE_NAME_AQUI"

// Bibliotecas 
#include <freertos/FreeRTOS.h> // RTOS (obrigatório para Tasks, Mutex)
#include <freertos/task.h>     // Funções para Tasks
#include <freertos/semphr.h>   // Semáforos e Mutex
#include <WiFi.h>              // Para conectividade
#include <BlynkSimpleEsp32.h>  // Biblioteca Blynk
#include <Wire.h> 
#include <LiquidCrystal_I2C.h> // Display I2C
#include <math.h>              // Para PI (usado no cálculo de velocidade)
#include <Arduino.h>
#include "esp32-hal-ledc.h"


//CREDENCIAIS BLYNK E WIFI

char auth[] = "TOKEN_BLYNK_AQUI"; // TOKEN BLYNK
char ssid[] = "SSID_AQUI"; // SSID DO WI-FI (2.4GHz)
char pass[] = "SENHA_WIFI_AQUI"; // SENHA DO WI-FI

//DISPLAY LCD 
#define ENDERECO_I2C 0x27
#define NUM_COLUNAS 16
#define NUM_LINHAS 2
LiquidCrystal_I2C lcd(ENDERECO_I2C, NUM_COLUNAS, NUM_LINHAS);

// PINAGEM  
const int S1_PIN = 25;       // Switch Start/Stop
const int SF_PIN = 26;       // Switch Freio
const int INB_PIN = 27;      // Switch Inibir Aceleração
const int MOTOR_ON_PIN = 34; // Sensor Motor Ligado (Entrada Analógica/Digital)
const int HALL_RPM_PIN = 14; // Sensor Hall (Pulsos)
const int DM_PIN = 13;       // Desligar Motor
const int LM_PIN = 12;       // Ligar Motor
const int SERVO_PIN = 2;     // Pino para Servomotor (Rs/As)

// CONSTANTES GLOBAIS 
// Constantes FSM 
const float Vmax = 31; // Limite de velocidade 
const float Vmin = 16;  // Limite de velocidade 

// Constantes de Cálculo 
const float RAIO_RODA_METROS = 0.30;
const float PULSOS_POR_REVOLUCAO = 2.0;

// Constantes de Hardware
const int ADC_THRESHOLD = 1000; 
const int servoChannel = 0; 
const int freq = 50;        
const int resolution = 10;  

// Constantes de Rampa e Tempo
const int DELAY_PASSO_RAMPA_MS = 300; 
const int PASSO_ANGULO = 5;          
const int ANGULO_FECHADO = 90;       
const int ANGULO_ABERTO_MAX = 170;   
const long TEMPO_MAX_ARRANQUE_MS = 1000;
const unsigned long INTERVALO_DEBOUNCE = 50; 

// Constantes de Tasks 
const long INTERVALO_CALCULO_MS = 1000; 
const long INTERVALO_ENVIO_BLYNK_MS = 1000; 
const int PINO_BLYNK_VELOCIDADE = V4; // Pino V4 

// Definição da Máquina de Estados 
enum EstadoSistema { 
    ESTADO_ON_OFF, 
    ESTADO_START_APTO, 
    ESTADO_MONITORA_VELOCIDADE, 
    ESTADO_LIGA_MOTOR,
    ESTADO_DESLIGA_MOTOR,
    ESTADO_INCREMENTA_ACELERACAO, 
    ESTADO_FREIO_ACIONADO,
    ESTADO_INIBIDOR
};
volatile EstadoSistema estadoAtual = ESTADO_ON_OFF;


// =========================================================
// 2. VARIÁVEIS GLOBAIS COMPARTILHADAS
// =========================================================
SemaphoreHandle_t xMutexDisplay; // Mutex para o Display
volatile unsigned long contadorPulsos = 0; // Preenchido pela ISR
volatile float velocidadeKmH = 0.0; // Preenchido pela TaskVelocidade
volatile int anguloAtualServo = ANGULO_FECHADO; // Preenchido pela TaskEstados

// Flags de Entrada
volatile bool ignicaoLigada = false; // Preenchido pela TaskEstados
volatile bool s1Pressionado = false; // Preenchido pela ISR
volatile bool sfPressionado = false; // Preenchido pela ISR
volatile bool inbPressionado = false; // Preenchido pela ISR
volatile bool wifiConectado = false;  // Preenchido pela TaskTelemetria
volatile bool blynkConectado = false; // Preenchido pela TaskTelemetria

// Timers Não-Bloqueantes
volatile unsigned long tempoInicioArranque = 0; // Usado na FSM
volatile unsigned long tempoUltimoDebounce[3] = {0, 0, 0}; // Usado na ISR


// =========================================================
// 3. INTERRUPÇÕES E FUNÇÕES AUXILIARES
// =========================================================

//  Interrupção para RPM (Contagem de Pulsos)
void IRAM_ATTR contarPulsoHall() {
  contadorPulsos++;
}

//Interrupções com Debounce para Switches 
void IRAM_ATTR handleSwitch(int index) {
  unsigned long agora = xTaskGetTickCount() * portTICK_PERIOD_MS;
  if ((agora - tempoUltimoDebounce[index]) > INTERVALO_DEBOUNCE) {
    if (index == 0) s1Pressionado = (digitalRead(S1_PIN) == LOW); 
    if (index == 1) sfPressionado = (digitalRead(SF_PIN) == LOW); 
    if (index == 2) inbPressionado = (digitalRead(INB_PIN) == LOW); 
    tempoUltimoDebounce[index] = agora;
  }
}
void IRAM_ATTR handleS1() { handleSwitch(0); }
void IRAM_ATTR handleSF() { handleSwitch(1); }
void IRAM_ATTR handleINB() { handleSwitch(2); }

// Funções do Servo 
void acionarServo(int angle) {
    int duty = map(angle, 0, 180, 51, 256); 
    ledcWrite(servoChannel, duty); 
}

void controlarAceleracaoPorRampa(int passos_desejados) {
  int angulo_alvo = anguloAtualServo + (passos_desejados * PASSO_ANGULO);
    if (angulo_alvo > ANGULO_ABERTO_MAX) {
        angulo_alvo = ANGULO_ABERTO_MAX;
    }
  while (anguloAtualServo < angulo_alvo) {
        if (sfPressionado || inbPressionado) {           // checagem de segurança a cada passo
      acionarServo(ANGULO_FECHADO);
      anguloAtualServo = ANGULO_FECHADO;
      return;
    }
        anguloAtualServo += PASSO_ANGULO;
        acionarServo(anguloAtualServo);
        vTaskDelay(pdMS_TO_TICKS(DELAY_PASSO_RAMPA_MS));
    }
}

// =========================================================
// 4. DEFINIÇÃO DAS TASKS (FreeRTOS)
// =========================================================

//  TASK VELOCIDADE 
void TaskVelocidade(void * parameter) {
  
  for (;;) {
    // Pausa a task pelo intervalo de cálculo 
    vTaskDelay(pdMS_TO_TICKS(INTERVALO_CALCULO_MS));

    unsigned long pulsos;
    portDISABLE_INTERRUPTS(); // Protege a leitura da variável da ISR
    pulsos = contadorPulsos;
    contadorPulsos = 0;
    portENABLE_INTERRUPTS();

    if (pulsos > 0) {
      float voltas = pulsos / PULSOS_POR_REVOLUCAO;
      float circunferencia = 2.0 * PI * RAIO_RODA_METROS;
      float distancia = circunferencia * voltas;
      float velocidadeMs = distancia / ((float)INTERVALO_CALCULO_MS / 1000.0);
      velocidadeKmH = velocidadeMs * 3.6; 
    } else {
      velocidadeKmH = 0.0;
    }
  }
}

//TASK TELEMETRIA 
void TaskTelemetria(void * parameter) {
  Serial.println("TaskTelemetria: Iniciada.");
  unsigned long ultimoTentativa = 0;
  unsigned long ultimoTentativaWiFi = 0;
  unsigned long tempoUltimoEnvio = 0;
  
  // Conexão inicial 
  Serial.print("TaskTelemetria: Conectando ao WiFi: ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true);
  delay(100);
  WiFi.begin(ssid, pass);

  unsigned long inicio = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (millis() - inicio > 15000) { // 15s timeout
      Serial.println("\nTaskTelemetria: Falha ao conectar WiFi na inicialização!");
      wifiConectado = false;
      break; // Sai do while de conexão
    }
  }

  if(WiFi.status() == WL_CONNECTED) {
    wifiConectado = true;
    Serial.println("\nTaskTelemetria: WiFi conectado!");
    Blynk.config(auth);
    if (Blynk.connect(10000)) { // 10s timeout
      blynkConectado = true;
      Serial.println("TaskTelemetria: Blynk conectado!");
    } else {
      blynkConectado = false;
      Serial.println("TaskTelemetria: Falha ao conectar Blynk na inicialização.");
    }
  }

  // Loop principal da TaskTelemetria
  for (;;) {
    if (wifiConectado) {
      if (blynkConectado) {
        Blynk.run();
        // Lógica de envio 
        if (millis() - tempoUltimoEnvio >= INTERVALO_ENVIO_BLYNK_MS) {
          Blynk.virtualWrite(PINO_BLYNK_VELOCIDADE, velocidadeKmH);
          tempoUltimoEnvio = millis();
        }
      } else {
        // Tenta reconectar Blynk 
        if (millis() - ultimoTentativa > 10000) {
          Serial.println("TaskTelemetria: Tentando reconectar Blynk...");
          blynkConectado = Blynk.connect(5000);
          ultimoTentativa = millis();
        }
      }
    } else {
      // Tenta reconectar WiFi 
      if (millis() - ultimoTentativaWiFi > 10000) {
        Serial.println("TaskTelemetria: Tentando reconectar WiFi...");
        // Re-chama a lógica de conexão
        WiFi.disconnect(true); delay(100); WiFi.begin(ssid, pass);
      
        ultimoTentativaWiFi = millis();
      }
    }
    
    vTaskDelay(pdMS_TO_TICKS(500)); // Pausa a task por 500ms
  }
}


// --- TASK DISPLAY  ---
void TaskDisplay(void * parameter) {
  for (;;) {
    if (xSemaphoreTake(xMutexDisplay, portMAX_DELAY) == pdTRUE) {
      
      // Linha 0 -> ESTADO
      lcd.setCursor(0, 0);
      switch (estadoAtual) {
      case ESTADO_ON_OFF:                lcd.print("SISTEMA OFF     "); break;
      case ESTADO_START_APTO:            lcd.print("AGUARDANDO      "); break;
      case ESTADO_MONITORA_VELOCIDADE:   lcd.print("MONITORANDO     "); break;
      case ESTADO_LIGA_MOTOR:            lcd.print("LIGANDO MOTOR   "); break;
      case ESTADO_DESLIGA_MOTOR:         lcd.print("MOTOR DESLIGADO "); break;
      case ESTADO_INCREMENTA_ACELERACAO: lcd.print("ACELERANDO      "); break;
      case ESTADO_FREIO_ACIONADO:        lcd.print("FREIO ACIONADO  "); break;
      case ESTADO_INIBIDOR:              lcd.print("ACEL. INIBIDA   "); break;
      default:                           lcd.print("ESTADO INVALIDO "); break;
      }
      
      // Linha 1 _>VELOCIDADE 
      lcd.setCursor(0, 1);
      char buffer[NUM_COLUNAS + 1];
      // Formata a velocidade 
      snprintf(buffer, sizeof(buffer), "V: %04.1f Km/h    ", velocidadeKmH);
      lcd.print(buffer);
      
    
      xSemaphoreGive(xMutexDisplay);
    }
    
    vTaskDelay(pdMS_TO_TICKS(200)); // Frequência de atualização do LCD
  }
}

//  TASK ESTADOS 
void TaskEstados(void * parameter) {
  for (;;) {

    s1Pressionado  = (digitalRead(S1_PIN)  == LOW); // ressincroniza as chaves
    sfPressionado  = (digitalRead(SF_PIN)  == LOW);
    inbPressionado = (digitalRead(INB_PIN) == LOW);
  
    switch (estadoAtual) {
      
      case ESTADO_ON_OFF:
      {
        // Reseta o sistema, verifica condições iniciais
        digitalWrite(LM_PIN, LOW); // Garante que o motor não está sendo ligado
        digitalWrite(DM_PIN, LOW); 
        acionarServo(ANGULO_FECHADO);// garantir que o servo não está acelerando
        anguloAtualServo = ANGULO_FECHADO;

        if (s1Pressionado == true) {
          estadoAtual = ESTADO_START_APTO;
        }
        break;
      }
        
      case ESTADO_START_APTO:
      {
       
        if (!s1Pressionado) {
          estadoAtual = ESTADO_ON_OFF;
        }else if(s1Pressionado && sfPressionado) {
          estadoAtual = ESTADO_FREIO_ACIONADO;
        }else if(s1Pressionado && !sfPressionado && velocidadeKmH > 0) { // Usa a velocidade da TaskVelocidade
          estadoAtual = ESTADO_MONITORA_VELOCIDADE;
        }

        break;
        }

      case ESTADO_MONITORA_VELOCIDADE:
      {
        //Reseta o sistema, verifica condições iniciais
        digitalWrite(LM_PIN, LOW); // Garante que o motor não está sendo ligado
        digitalWrite(DM_PIN, LOW); // Desenergiza o relé responsável por desligar o motor.

        //Leitura do circuito conversor de frequência em tensão
        int leituraADC = analogRead(MOTOR_ON_PIN);
        if (leituraADC > ADC_THRESHOLD) { 
        ignicaoLigada = true;
        } else {
        ignicaoLigada = false;
        }
        
        if (!s1Pressionado) {
          estadoAtual = ESTADO_ON_OFF;
        }else if(s1Pressionado && sfPressionado) {
          estadoAtual = ESTADO_FREIO_ACIONADO;
        }else if(s1Pressionado && !sfPressionado && inbPressionado) {
          estadoAtual = ESTADO_INIBIDOR;
        }else if(s1Pressionado && !sfPressionado && !inbPressionado && velocidadeKmH > Vmax && ignicaoLigada ) {
          estadoAtual = ESTADO_DESLIGA_MOTOR;
        }else if(s1Pressionado && !sfPressionado && !inbPressionado && velocidadeKmH < Vmin && !ignicaoLigada ) {
          estadoAtual = ESTADO_LIGA_MOTOR;
        }else if(s1Pressionado && !sfPressionado && !inbPressionado && velocidadeKmH < Vmax && ignicaoLigada ) {
          estadoAtual = ESTADO_INCREMENTA_ACELERACAO;
        }

        break;
      }

      case ESTADO_LIGA_MOTOR:
      {
        if (tempoInicioArranque == 0) {
        digitalWrite(LM_PIN, HIGH); // Garante que o motor está sendo ligado
        tempoInicioArranque = millis();
        }
        
        int leituraADC = analogRead(MOTOR_ON_PIN);
        bool motorJaLigou = leituraADC > ADC_THRESHOLD;
        unsigned long tempoAtual = millis();

        if(sfPressionado) {
        digitalWrite(LM_PIN, LOW); // Para de acionar o arranque
        estadoAtual = ESTADO_FREIO_ACIONADO;
        } else if (motorJaLigou) {     // motor pegou         
        digitalWrite(LM_PIN, LOW);
        tempoInicioArranque = 0;
        estadoAtual = ESTADO_MONITORA_VELOCIDADE;
        } else if ((tempoAtual - tempoInicioArranque >= TEMPO_MAX_ARRANQUE_MS)) {
        digitalWrite(LM_PIN, LOW); // Para de acionar o arranque
        tempoInicioArranque = 0;   // Reseta o timer
        // Se falhou, volta assim para monitorar e tentar de novo.
        estadoAtual = ESTADO_MONITORA_VELOCIDADE; 
        }
        break;
      }


      case ESTADO_DESLIGA_MOTOR:
      {
        acionarServo(ANGULO_FECHADO);// garantir que o servo não está acelerando
        anguloAtualServo = ANGULO_FECHADO;
        digitalWrite(DM_PIN , HIGH); //aciona relé pra desligar o motor 
     
        if(sfPressionado) {
        estadoAtual = ESTADO_FREIO_ACIONADO;
        }else{
        estadoAtual = ESTADO_MONITORA_VELOCIDADE;
        }
        break;
      }


      case ESTADO_INCREMENTA_ACELERACAO:
      {
        if (sfPressionado) {
        estadoAtual = ESTADO_FREIO_ACIONADO;
        } else if (inbPressionado) {
        estadoAtual = ESTADO_INIBIDOR;
        } else if (!s1Pressionado) {
        estadoAtual = ESTADO_MONITORA_VELOCIDADE;  
        } else {
        controlarAceleracaoPorRampa(3);
        estadoAtual = ESTADO_MONITORA_VELOCIDADE;
        }
        break;
      }


      case ESTADO_INIBIDOR:
      {
        acionarServo(ANGULO_FECHADO);// garantir que o servo não está acelerando
        anguloAtualServo = ANGULO_FECHADO;

        if(sfPressionado) {
          estadoAtual = ESTADO_FREIO_ACIONADO;
        }else if (!sfPressionado && !inbPressionado) {
          estadoAtual = ESTADO_MONITORA_VELOCIDADE;
        }
        
        break;
      }

      case ESTADO_FREIO_ACIONADO:
      { 
        acionarServo(ANGULO_FECHADO);// garantir que o servo não está acelerando
        anguloAtualServo = ANGULO_FECHADO;

        if (!s1Pressionado) {
          estadoAtual = ESTADO_ON_OFF;
        }else if(s1Pressionado && !sfPressionado) {
          estadoAtual = ESTADO_START_APTO;
        }

        break;
      }

      default: 
        estadoAtual = ESTADO_ON_OFF;
        break;
    }
    
    vTaskDelay(50 / portTICK_PERIOD_MS); // Frequência de atualização da FSM
  }
}


// =========================================================
// 5. SETUP E LOOP PRINCIPAL
// =========================================================

void setup() {
  Serial.begin(115200);
  Serial.println("--- INICIO DO SETUP ---");
  
  // Configuração de Pinos de I/O
  pinMode(DM_PIN, OUTPUT);
  pinMode(LM_PIN, OUTPUT);
  // pinMode(SERVO_PIN, OUTPUT); 
  
  pinMode(S1_PIN, INPUT_PULLUP);
  pinMode(SF_PIN, INPUT_PULLUP);
  pinMode(INB_PIN, INPUT_PULLUP);
  pinMode(MOTOR_ON_PIN, INPUT); 
  pinMode(HALL_RPM_PIN, INPUT_PULLUP);

  // Inicialização I2C e Display
  Wire.begin(); 
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.print("Iniciando RTOS...");

  // Criação do Mutex
  xMutexDisplay = xSemaphoreCreateMutex(); 

  // Configuração das Interrupções
  attachInterrupt(digitalPinToInterrupt(HALL_RPM_PIN), contarPulsoHall, RISING);
  attachInterrupt(digitalPinToInterrupt(S1_PIN), handleS1, CHANGE); 
  attachInterrupt(digitalPinToInterrupt(SF_PIN), handleSF, CHANGE);
  attachInterrupt(digitalPinToInterrupt(INB_PIN), handleINB, CHANGE);

  // Configuração do PWM para o Servomotor
  ledcSetup(servoChannel, freq, resolution); 
  ledcAttachPin(SERVO_PIN, servoChannel);    
  acionarServo(ANGULO_FECHADO); // Garante posição inicial

  // Criação das Tasks / Ajuste de Stack e Prioridades para estabilidade
  xTaskCreatePinnedToCore(TaskVelocidade, "TaskVelocidade", 4096, NULL, 4, NULL, 0); // Core 0, Prio 4 (Mais alta p/ cálculo)
  xTaskCreatePinnedToCore(TaskDisplay,    "TaskDisplay",    4096, NULL, 2, NULL, 0); // Core 0, Prio 2
  xTaskCreatePinnedToCore(TaskEstados,    "TaskEstados",    8192, NULL, 3, NULL, 0); // Core 0, Prio 3 
  xTaskCreatePinnedToCore(TaskTelemetria, "TaskTelemetria", 8192, NULL, 3, NULL, 1); // Core 1

  Serial.println("--- Setup Concluído ---");
  
}

void loop() {
  vTaskDelay(1000 / portTICK_PERIOD_MS); 
}