# 🤖 Mini Sumo ESP32 + Controle PS4 via Bluepad32

Este projeto controla dois motores de um robô mini sumô usando um controle PS4 conectado via Bluetooth ao ESP32. A lógica foi implementada em [Controle_BluePad/Controle_BluePad.ino](Controle_BluePad/Controle_BluePad.ino).

---

## 📋 O que o código faz

- Conecta um controle PS4 usando Bluepad32
- Lê o joystick esquerdo e calcula a movimentação dos motores
- Controla o driver TB6612FNG com PWM
- Possui trava de segurança com o botão R1
- Desliga por segurança após 5 minutos sem input
- Permite ligar/desligar o Bluetooth por um botão físico
- Aceita apenas um controle por vez

---

## 🔌 Pinagem atual do hardware

| Função | GPIO | Observação |
|--------|------|-----------|
| AIN1 | 25 | Direção do motor esquerdo |
| AIN2 | 33 | Direção do motor esquerdo |
| PWMA | 32 | PWM do motor esquerdo |
| BIN1 | 27 | Direção do motor direito |
| BIN2 | 14 | Direção do motor direito |
| PWMB | 12 | PWM do motor direito |
| STBY | 26 | Standby do driver TB6612FNG |
| BUTTON_PIN | 34 | Botão para ligar/desligar Bluetooth |

> Atenção: o GPIO 12 é um pino de strapping em alguns módulos ESP32. Se o hardware não iniciar corretamente ou o PWM do motor direito não funcionar, troque esse pino por outro disponível e ajuste o código.

---

## 🧠 Como o controle funciona

A lógica de movimentação usa o eixo esquerdo do controle:

- `axisY()` controla avanço e ré
- `axisX()` controla direção esquerda/direita

A mistura é feita assim:

```cpp
int leftRaw  = LStickY - LStickX;
int rightRaw = LStickY + LStickX;
```

Depois disso, os valores são convertidos para a faixa de -255 a 255 e enviados ao driver TB6612FNG.

A zona morta atual é:

```cpp
const int DEADZONE = 20;
```

Ou seja, pequenos movimentos próximos de zero são ignorados para evitar vibração do joystick mexendo o robô sem querer.

---

## 🔒 Segurança

### Botão R1

Quando o botão R1 é pressionado, o código alterna entre:

- `fechado = true` → robô travado, motores desligados
- `fechado = false` → robô liberado, joystick volta a funcionar

### Timeout de inatividade

Se o controle ficar sem movimento por 5 minutos, o código:

1. para os motores
2. desconecta o controle
3. desativa o Bluetooth

---

## 🛠️ Como configurar o Arduino IDE

Para funcionar corretamente, o pacote Bluepad32 precisa ser adicionado nas URLs extras do Arduino IDE.

### 1) Adicionar a URL do pacote Bluepad32

No Arduino IDE:

- Arquivo → Preferências
- Em "URLs adicionais do Gerenciador de placas", adicione:

```text
https://raw.githubusercontent.com/ricardoquesada/esp32-arduino-lib-builder/master/bluepad32_files/package_esp32_bluepad32_index.json
```

### 2) Instalar o pacote

- Ferramentas → Placa → Gerenciador de placas...
- Procure por: `esp32 Bluepad32` ou `ESP32 Bluepad32`
- Instale o pacote correspondente

### 3) Selecionar a placa

- Ferramentas → Placa → ESP32 Arduino → ESP32 Dev Module

### 4) Instalar a biblioteca do código

O exemplo usa `#include <Bluepad32.h>`, então a biblioteca Bluepad32 deve estar disponível após instalar o pacote do passo anterior.

---

## 🔧 Pinagem do motor e driver TB6612FNG

O código usa o driver TB6612FNG com o padrão:

- `IN1/IN2` para direção
- `PWM` para velocidade
- `STBY` ligado em HIGH para ativar o driver

Função usada:

```cpp
void driveMotor(int in1Pin, int in2Pin, int pwmChannel, int speed) {
  speed = constrain(speed, -255, 255);

  if (speed > 0) {
    digitalWrite(in1Pin, HIGH);
    digitalWrite(in2Pin, LOW);
  } else if (speed < 0) {
    digitalWrite(in1Pin, LOW);
    digitalWrite(in2Pin, HIGH);
  } else {
    digitalWrite(in1Pin, LOW);
    digitalWrite(in2Pin, LOW);
  }

  ledcWrite(pwmChannel, abs(speed));
}
```

---

## 🐛 Problemas comuns

### Controle não conecta
- Verifique se o botão de ativação do Bluetooth foi pressionado.
- Verifique se o pacote Bluepad32 foi corretamente instalado.
- Confirme se a placa correta foi selecionada no Arduino IDE.

### Motores não mexem
- Verifique se `STBY` está em HIGH.
- Verifique a alimentação do TB6612FNG.
- Confira se os fios de direção e PWM estão corretos.
- Veja se o joystick está fora da zona morta.

### Robô trava sozinho
- Pode estar em estado `fechado` depois do botão R1.
- Pode ter ativado o timeout de 5 minutos sem input.

### GPIO 12 não funciona
- Troque o pino de `PWMB` para outro GPIO disponível.
- Ajuste a definição no código e confirme o pino na placa.

---

## 📁 Arquivos do projeto

- [Controle_BluePad/Controle_BluePad.ino](Controle_BluePad/Controle_BluePad.ino) — código principal

---

## 🔎 Resumo rápido

```text
ESP32 liga
  ↓
Configura pinos, PWM, serial e BT
  ↓
Botão físico liga/desliga Bluetooth
  ↓
Controle conectado?
  ├── Não → motores parados
  └── Sim → lê joystick e move robô
      ↓
      R1 → trava/destrava
      ↓
      Inatividade > 5 min → desconecta por segurança
```

---

Última revisão do README: 24/09/2026
