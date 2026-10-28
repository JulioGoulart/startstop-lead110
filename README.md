# Sistema start/stop para veículo de eficiência energética

Código-fonte do sistema start/stop e scripts de ciclo para simulação no ADVISOR, desenvolvidos para o protótipo Urban Concept da equipe Milhagem UFMG.


O ESP32 mede a velocidade do veículo por um sensor Hall e, com base nela, decide quando desligar e religar o motor e quando acelerar, por meio de uma máquina de estados finitos:

- acima de Vmax, o motor é desligado (relé que interrompe o negativo do bico injetor);
- abaixo de Vmin, o motor é religado (relé do motor de arranque);
- entre os dois limites, um servomotor aciona o acelerador em rampa;
- o freio e o botão inibidor de aceleração interrompem a aceleração imediatamente.

Estrutura do repositório

  StartSTOP.ino                 Firmware do ESP32 
  ciclo_prova_completa.m        Ciclo de velocidade da prova para o ADVISOR
README.md

Hardware

- ESP32 Dev Module
- Sensor Hall (pulsos de velocidade), 2 pulsos por revolução da roda
- Display LCD 16x2 com módulo I2C (endereço `0x27`)
- Servomotor (acionamento do acelerador)
- Relés de partida e de desligamento do motor
- Circuito conversor de frequência em tensão (detecção de motor ligado)
- Chaves: S1 (liga/desliga o sistema), freio e inibidor de aceleração

Configuração antes de compilar

O código não acompanha credenciais

TEMPLATE_ID_AQUI | ID do template criado no Blynk |
TEMPLATE_NAME_AQUI | Nome do *template* no Blynk |
TOKEN_BLYNK_AQUI | Token do dispositivo no Blynk |
SSID_AQU | Nome da rede Wi-Fi (2,4 GHz) |
SENHA_WIFI_AQUI | Senha da rede Wi-Fi |

Simulação (ADVISOR)

A simulação usa o ADVISOR 2002/2003 em MATLAB/Simulink(2015).
O ciclo CYC_RIOV6 é uma aproximação da prova da SHELL-Eco-Marathon.
É necessário carregador o ciclo no ADVISOR


