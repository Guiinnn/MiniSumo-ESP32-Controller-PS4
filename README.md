# 🤖 Mini Sumo ESP32 + Controle PS4 via Bluepad32

Firmware de um robô mini sumô **rádio-controlado** (500 g, 10×10 cm). Um ESP32 recebe os comandos de um controle PS4 por Bluetooth, usando a biblioteca Bluepad32, e aciona dois motores N20 pelo driver TB6612FNG.

> **Status:** código refatorado conforme o [plano](docs/PLANO.md). Falta a calibração na bancada ([CALIBRACAO.md](docs/CALIBRACAO.md)).

---

## 📚 Documentação

| Documento | Conteúdo |
|-----------|----------|
| [docs/PLANO.md](docs/PLANO.md) | Decisões, comportamento detalhado, estrutura do código e correções |
| [docs/ESTRUTURA-DO-CODIGO.md](docs/ESTRUTURA-DO-CODIGO.md) | Explicação simples de como o código está dividido e por quê |
| [docs/SETUP-ARDUINO-IDE.md](docs/SETUP-ARDUINO-IDE.md) | Como instalar, compilar e gravar (**leia antes de compilar**) |
| [docs/CALIBRACAO.md](docs/CALIBRACAO.md) | Roteiro de bancada para ajustar o `config.h` e checklist de segurança |

---

## 🎮 Comandos

| Comando | Função |
|---------|--------|
| **Analógico esquerdo** | Pilota (frente, ré e curva) |
| **R1** | Trava / destrava o robô |
| **△** | Sobe a marcha |
| **✕** | Desce a marcha |

| Marcha | Força máxima | Luz do controle |
|--------|--------------|-----------------|
| 1ª | 25% | 🔵 |
| 2ª | 50% | 🟣 |
| 3ª | 75% | 🟡 |
| 4ª (inicial) | 100% | 🟢 |
| Travado | — | 🔴 |

---

## 🔒 Segurança

- O robô **sempre começa travado**, ao ligar e ao conectar o controle. Aperte **R1** para liberar.
- Travado, o driver fica desligado (`STBY` em LOW).
- Liberado e com o analógico solto, os motores **freiam**.
- Se o controle desconectar, os motores param na hora.
- Após **5 minutos** sem comandos, o robô trava (o controle continua conectado).
- Só os **2 controles da equipe** podem conectar, identificados pelo MAC, um de cada vez.

---

## 🔌 Pinagem

| Função | GPIO | Observação |
|--------|------|------------|
| AIN1 | 25 | Sentido do motor esquerdo |
| AIN2 | 33 | Sentido do motor esquerdo |
| PWMA | 32 | PWM do motor esquerdo |
| BIN1 | 27 | Sentido do motor direito |
| BIN2 | 14 | Sentido do motor direito |
| PWMB | 12 | PWM do motor direito ⚠️ |
| STBY | 26 | Habilita o driver TB6612FNG |
| BOTAO | 34 | Botão físico (desativado por padrão) ⚠️ |

### ⚠️ Avisos de hardware

- **GPIO12** é pino de *strapping*: se estiver em HIGH durante o boot, o ESP32 pode não iniciar ou falhar na gravação. Se acontecer, desconecte o fio do GPIO12 para gravar.
- **GPIO34** não tem pull-up interno. O botão só funciona com resistor de pull-up externo; por isso fica desativado no `config.h` (`BOTAO_ATIVO = false`).

Mais detalhes em [PLANO.md](docs/PLANO.md#8-avisos-de-hardware-fora-do-escopo).

---

## 🧩 Hardware

- 1× ESP32
- 1× Driver TB6612FNG
- 2× Motores N20 de 256 rpm
- 1× Bateria 2S
- 1× Botão
- 1× Controle PS4 (+ 1 reserva)

---

## 📁 Estrutura

```text
Controle_BluePad/
├── Controle_BluePad.ino   setup() e loop()
├── config.h               pinos, MACs, marchas e ajustes (o único arquivo a editar na calibração)
├── motores.h / .cpp       driver TB6612FNG
├── controle.h / .cpp      controle PS4 via Bluepad32
└── seguranca.h / .cpp     trava, timeout e botão físico
```

Explicação para quem não é da programação: [ESTRUTURA-DO-CODIGO.md](docs/ESTRUTURA-DO-CODIGO.md).

---

## 🚀 Início rápido

1. Configure a Arduino IDE: [SETUP-ARDUINO-IDE.md](docs/SETUP-ARDUINO-IDE.md). Use a placa **ESP32 Arduino + Bluepad32 → ESP32 Dev Module**, não a da Espressif.
2. Grave o código com `CONTROLES_AUTORIZADOS` vazio e cadastre os controles.
3. Calibre na bancada: [CALIBRACAO.md](docs/CALIBRACAO.md).
4. Faça o checklist de segurança antes de usar o robô.

---

## 🤝 Contribuição

- Commits, push e PRs são feitos **manualmente** pela equipe, nunca de forma automática por ferramentas ou assistentes.
- Ajustes de calibração vão no `config.h`.

---

Última revisão do README: 13/10/2026
