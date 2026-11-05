# 🧩 Estrutura do Código (explicação simples)

Este documento explica, sem termos técnicos, por que o código está dividido em vários arquivos e o que cada um faz. Para os detalhes técnicos, veja o [PLANO.md](PLANO.md).

---

## A ideia

Pense no robô como um **carro de controle remoto com uma equipe dentro**. Em vez de uma pessoa fazendo tudo, cada função ficou com um "funcionário", e cada funcionário tem seu arquivo.

---

## Os 5 "funcionários"

| Arquivo | Quem seria no carro | O que faz |
|---------|---------------------|-----------|
| **`config.h`** | **O painel de regulagens** | Guarda todos os números ajustáveis: em que pino cada fio está ligado, a força de cada marcha, a sensibilidade do analógico, quais controles podem conectar. Não faz nada sozinho; os outros só consultam. |
| **`motores`** | **O mecânico do motor** | Só sabe girar as rodas: para frente, para trás, frear ou desligar. Também acelera aos poucos para não dar tranco. Não sabe nada de controle nem de Bluetooth; só recebe "roda esquerda a 50%, direita a 30%" e executa. |
| **`controle`** | **O rádio-operador** | Conversa com o controle PS4: aceita só os controles da equipe, lê o analógico e os botões, troca a marcha, acende a luz e faz o controle vibrar. Ele transforma "o piloto empurrou o analógico para a direita" em "roda esquerda para frente, direita para trás". |
| **`seguranca`** | **O fiscal de segurança** | Decide se o robô pode andar ou não. Começa sempre com o robô travado, trava se o controle desconectar ou se ninguém mexer em nada por 5 minutos, e alterna a trava com o R1. |
| **`Controle_BluePad.ino`** | **O chefe da equipe** | Não faz o trabalho pesado. 100 vezes por segundo, ele pergunta ao rádio-operador o que o piloto mandou, ao fiscal se pode andar, e passa a ordem ao mecânico. |

---

## Como eles conversam (a cada 10 milésimos de segundo)

```text
  Piloto mexe no controle
          │
          ▼
   📻 controle ──── "piloto quer: esq 80%, dir 80%, apertou R1"
          │
          ▼
   🦺 seguranca ─── "R1 apertado → agora está LIBERADO"
          │
          ▼
   🔧 motores ───── gira as duas rodas para frente
          │
          ▼
   📻 controle ──── acende a luz verde e vibra
```

O **chefe** (`Controle_BluePad.ino`) é quem passa a informação de um para o outro, na ordem certa.

---

## Por que separar em vez de deixar tudo em um arquivo só?

O código antigo era como **uma pessoa só fazendo tudo ao mesmo tempo**: dirigir, falar no rádio, cuidar da segurança e regular o motor. Quando algo dava errado, era difícil saber onde estava o problema.

Separado:

- **Quem calibra mexe só no painel (`config.h`).** Não precisa entender o resto do código para trocar "zona morta 40" por "zona morta 50".
- **Fica fácil achar um problema:**

  | Sintoma | Onde olhar |
  |---------|------------|
  | Roda girando errado | `motores` (ou `INVERTER_*` no `config.h`) |
  | Controle não conecta | `controle` (ou `CONTROLES_AUTORIZADOS` no `config.h`) |
  | Robô não trava ou não libera | `seguranca` |

- **Mexer em uma parte não quebra a outra.** Se um dia trocarem o controle PS4 por outro, só o "rádio-operador" muda; o mecânico continua igual.

---

## E por que alguns têm dois arquivos (`.h` e `.cpp`)?

É como um **restaurante**:

- O **`.h`** é o **cardápio**: lista o que aquele funcionário sabe fazer ("girar rodas", "frear", "desligar"). É o que os outros consultam.
- O **`.cpp`** é a **cozinha**: o modo de fazer de cada prato. Ninguém de fora precisa entrar lá; basta pedir pelo cardápio.

O `config.h` só tem o "cardápio" porque não faz nada, apenas guarda valores. O `.ino` não precisa de cardápio porque é o chefe: ninguém dá ordens para ele.

Na Arduino IDE, cada um desses arquivos aparece como uma **aba** separada, mas todos viram um único programa quando você grava no ESP32.
