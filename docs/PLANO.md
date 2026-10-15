# 📐 Plano de Refatoração do Código

Este documento registra **o que vai mudar no código**, **por quê** e **em que ordem**. Ele é a referência para a implementação e para a revisão das mudanças.

> **Status:** código implementado (etapas 3 a 5), compilando sem erros nem avisos. Próximos passos: compilar na Arduino IDE da equipe (etapa 6) e calibrar na bancada (etapa 7).

---

## 1. Escopo

| Item | Decisão |
|------|---------|
| Tipo de robô | Mini sumô **rádio-controlado (RC)**, operado por controle PS4 |
| Limites físicos | 500 g, base 10×10 cm |
| O que muda | **Somente o código** (firmware do ESP32) e a documentação |
| O que **não** muda | Hardware: pinos, driver TB6612FNG, motores N20, PCB, bateria 2S |
| Fora do escopo | Modo autônomo, sensores, mudanças mecânicas e elétricas |
| Ferramenta de compilação | Arduino IDE com a placa do pacote **esp32-bluepad32 4.1.0** (ver [SETUP-ARDUINO-IDE.md](SETUP-ARDUINO-IDE.md)) |

Problemas que só se resolvem no hardware aparecem como **avisos** na seção 8, não como tarefas.

---

## 2. Regras de trabalho

- **Sem git automático.** Nenhuma ferramenta ou assistente deve executar `git commit`, `git push`, criar branch ou abrir PR automaticamente. As alterações são feitas nos arquivos, e **o commit, o push e o PR são feitos manualmente pela equipe**, depois de revisar.
- Fluxo sugerido (manual): trabalhar na branch `refatoracao-controle`, com commits pequenos por funcionalidade e PR para a `main`.
- O documento de projeto da equipe (PDF) não é versionado (vai para o `.gitignore`).
- Todo valor que pode precisar de ajuste fica no `config.h`, nunca espalhado pelo código.

---

## 3. Comandos do controle

| Comando | Função |
|---------|--------|
| **Analógico esquerdo** | Pilota: para cima/baixo = frente/ré, esquerda/direita = curva |
| **R1** | Trava/destrava o robô (age no **momento do aperto**, segurar não repete) |
| **△ (triângulo)** | Sobe uma marcha |
| **✕ (xis)** | Desce uma marcha |

> No Bluepad32 os botões seguem o padrão Xbox: **△ = `y()`** e **✕ = `a()`**.

### Marchas

| Marcha | Força máxima | Cor da luz do controle |
|--------|--------------|------------------------|
| 1ª | 25% | 🔵 Azul |
| 2ª | 50% | 🟣 Roxo |
| 3ª | 75% | 🟡 Amarelo |
| 4ª | 100% | 🟢 Verde |

- O robô **começa sempre na 4ª marcha**.
- As marchas **não dão a volta**: na 4ª, apertar △ não faz nada; na 1ª, apertar ✕ não faz nada. Isso evita cair de 100% para 25% por um toque acidental durante a luta.
- Os percentuais ficam em `config.h`.

### Sinais para o piloto

| Situação | Luz do controle | Vibração |
|----------|-----------------|----------|
| Travado | 🔴 Vermelho | curta, ao travar |
| Liberado | cor da marcha atual | curta, ao destravar |
| Troca de marcha | muda para a cor da nova marcha | — |

---

## 4. Comportamento do robô

### 4.1 Estados de segurança

```text
                ┌──────────────────────────────┐
  liga / conecta│                              │
  ─────────────►│   TRAVADO                    │◄── R1
                │   STBY = LOW (driver off)    │◄── 5 min sem comando
                │   luz vermelha               │◄── controle desconectou
                └──────────────┬───────────────┘
                               │ R1
                               ▼
                ┌──────────────────────────────┐
                │   LIBERADO                   │
                │   STBY = HIGH                │
                │   analógico move o robô      │
                │   luz = cor da marcha        │
                └──────────────────────────────┘
```

| Evento | Resultado |
|--------|-----------|
| ESP32 liga | **Travado** |
| Controle conecta | **Travado** (precisa de R1 para liberar) |
| R1 apertado | Alterna travado ↔ liberado |
| 5 min sem nenhum comando | **Travado**, controle **continua conectado** |
| Controle desconecta | Motores param **na hora**, estado vai para **travado** |

- **Travado** = `STBY` em LOW. O driver fica desligado de verdade (motores soltos, driver esfria).
- **Liberado e analógico solto** = motores **freiam** (freio curto do TB6612: `IN1 = IN2 = HIGH`).
- Conta como "comando" para o timeout: analógico fora da zona morta ou qualquer botão usado (R1, △, ✕).

### 4.2 Do analógico até o motor (pipeline)

A cada ciclo, o valor do analógico passa por estas etapas, nesta ordem:

```text
 analógico ─► zona morta ─► curva ─► mistura ─► marcha ─► trim ─► força ─► rampa ─► inversão ─► driver
 (X, Y)       circular      expo     (esq/dir)  (25-100%) (por    mínima   (limita   (por
                                     normaliz.            motor)           variação) motor)
```

| Etapa | O que faz | Por quê |
|-------|-----------|---------|
| **Leitura** | Lê X e Y (−511 a 512). Inverte o Y para que "para cima" seja positivo | No Bluepad32 o Y é negativo para cima |
| **Zona morta circular** | Se o analógico estiver dentro de um círculo de raio `ZONA_MORTA`, vale zero. Fora dele, o valor é reescalado para começar do zero | Evita tremer parado sem distorcer as diagonais |
| **Curva exponencial** | Aplica `EXPO` (0 = linear, 1 = muito suave no começo) | Controle fino em pouca deflexão, força total no fim do curso |
| **Mistura** | `esq = Y + X`, `dir = Y − X`. Se algum passar de 100%, **os dois são divididos pelo maior** | Analógico no máximo dá 100%, e a curva mantém a proporção entre os motores |
| **Marcha** | Multiplica pelo percentual da marcha atual | Ajuste de velocidade pelo piloto |
| **Trim** | Multiplica cada motor pelo seu `TRIM_*` (ex.: 100% e 94%) | Compensa motor mais forte que o outro |
| **Força mínima** | Se o valor não for zero, mapeia `0–100%` para `FORCA_MINIMA–100%` | O motor gira assim que sai da zona morta, em vez de só zumbir |
| **Rampa** | Limita quanto a força muda por ciclo (`RAMPA_MS` = tempo de 0 a 100%; `0` desliga) | Suaviza trancos e a queda de tensão na inversão de sentido |
| **Inversão** | Troca o sinal se `INVERTER_*` for `true` | Corrige o sentido sem mexer na fiação |
| **Driver** | Sinal → `IN1/IN2`; módulo → PWM de 8 bits a 20 kHz; zero → freio | — |

### 4.3 Controles autorizados

- Só os **2 controles da equipe** podem conectar, identificados pelo endereço Bluetooth (MAC) em `CONTROLES_AUTORIZADOS`.
- **Um por vez:** com um conectado, novas conexões são bloqueadas (`BP32.enableNewBluetoothConnections(false)`). Ao desconectar, voltam a ser aceitas.
- Um controle fora da lista é **desconectado imediatamente** e o MAC dele aparece no Serial.
- **Modo de cadastro:** se a lista estiver vazia, qualquer controle é aceito e o Serial mostra um aviso com o MAC dele, para copiar para o `config.h`. Usar isso **só na bancada**.

### 4.4 Bluetooth e botão físico

- O Bluetooth **começa ligado**. Não depende de botão.
- O botão do GPIO34 fica **desativado por padrão** (`BOTAO_ATIVO = false`), porque o GPIO34 não tem pull-up interno e ainda não se sabe se a placa tem resistor externo.
- Se ativado: segurar o botão por 2 s **esquece os pareamentos** salvos (`BP32.forgetBluetoothKeys()`).

### 4.5 Ciclo principal

- Sem `delay()`. O loop roda em período fixo de **10 ms (~100 Hz)**, medido com `millis()`.
- O log no Serial pode ser ligado e desligado (`LOG_SERIAL`). Quando ligado, imprime no máximo a cada `LOG_PERIODO_MS`, não a cada ciclo.

---

## 5. Estrutura do código

Todos os arquivos ficam na mesma pasta do sketch. A Arduino IDE abre cada um em uma aba.

Uma explicação sem termos técnicos está em [ESTRUTURA-DO-CODIGO.md](ESTRUTURA-DO-CODIGO.md).

```text
Controle_BluePad/
├── Controle_BluePad.ino   setup() e loop(): só orquestra os módulos
├── config.h               pinos, MACs, marchas, ajustes de resposta, timeouts
├── motores.h / .cpp       driver TB6612FNG: PWM, sentido, freio, STBY, rampa
├── controle.h / .cpp      Bluepad32: conexão, autorização, leitura, luz, vibração
└── seguranca.h / .cpp     estados travado/liberado, timeout, botão físico
```

### Parâmetros do `config.h`

| Parâmetro | Valor inicial | Descrição |
|-----------|---------------|-----------|
| `AIN1`, `AIN2`, `PWMA` | 25, 33, 32 | Motor esquerdo |
| `BIN1`, `BIN2`, `PWMB` | 27, 14, 12 | Motor direito |
| `STBY` | 26 | Standby do driver |
| `BOTAO_PIN` | 34 | Botão físico |
| `BOTAO_ATIVO` | `false` | Liga a função do botão |
| `BOTAO_SEGURAR_MS` | 2000 | Tempo segurando o botão para esquecer os pareamentos |
| `CONTROLES_AUTORIZADOS` | vazio | MACs dos 2 controles |
| `MARCHAS` | 25, 50, 75, 100 | Percentual de cada marcha |
| `MARCHA_INICIAL` | 4ª | Marcha ao ligar/conectar |
| `ZONA_MORTA` | 40 | Raio da zona morta (escala do analógico, 0–512) |
| `EXPO` | 0.4 | Curva da resposta (0 a 1) |
| `FORCA_MINIMA` | 15% | Força mínima ao sair da zona morta |
| `RAMPA_MS` | 100 | Tempo de 0 a 100% (0 = sem rampa) |
| `TRIM_ESQ`, `TRIM_DIR` | 100%, 100% | Compensação entre motores |
| `INVERTER_ESQ`, `INVERTER_DIR` | `false`, `false` | Inverte o sentido de cada motor |
| `PWM_FREQ` | 20000 | Frequência do PWM (Hz) |
| `PWM_RESOLUCAO` | 8 | Bits do PWM (0–255) |
| `TIMEOUT_INATIVIDADE_MS` | 300000 | 5 min sem comando → trava |
| `LOOP_PERIODO_MS` | 10 | Período do ciclo principal |
| `LOG_SERIAL` | `true` | Liga o log no Serial |
| `LOG_PERIODO_MS` | 200 | Intervalo entre linhas de log |

Os valores iniciais são um ponto de partida. Os definitivos saem da calibração ([CALIBRACAO.md](CALIBRACAO.md)).

---

## 6. Correções

### 6.1 Problemas da seção "Programação" do documento de projeto

| # | Problema relatado | Causa no código atual | Solução |
|---|-------------------|-----------------------|---------|
| 1 | Dificuldade na comunicação ESP32 ↔ PS4 | O "Bluetooth desligado" era só uma variável; controle recusado não era desconectado | Bluetooth sempre ligado, lista de MACs, bloqueio de novas conexões (4.3, 4.4) |
| 2 | Instabilidade na leitura dos comandos | Zona morta aplicada depois da mistura, por motor | Zona morta circular no analógico (4.2) |
| 3 | Interpretação de joysticks e botões | Eixo Y invertido; segurar R1 alternava a trava a cada 300 ms | Y corrigido na leitura; botões por detecção de aperto (3) |
| 4 | Controle simultâneo dos dois motores | Mistura cortada; analógico no máximo dava ~50% | Mistura normalizada (4.2) |
| 5 | Lógica de sentido de rotação | Sentido fixo no código | `INVERTER_ESQ/DIR` (4.2) |
| 6 | Calibração do PWM | PWM de 5 kHz (audível) e sem força mínima | PWM de 20 kHz, `FORCA_MINIMA`, curva e rampa (4.2) |
| 7 | Diferença de resposta entre os motores | Sem compensação | `TRIM_ESQ/DIR` (4.2) |
| 8 | Resposta lenta aos comandos | `delay(50)` e `Serial.print` a cada ciclo | Loop de 10 ms sem `delay`, log limitado (4.5) |
| 9 | Conflito de funções e pinos | GPIO12 (strapping) e GPIO34 (sem pull-up) | Hardware: viram avisos (8); botão desativado por padrão (4.4) |

### 6.2 Outros erros encontrados no código atual

| Erro | Correção |
|------|----------|
| Mensagem "Bluetooth SEMPRE ATIVADO" com o Bluetooth começando desligado | Mensagens refletem o estado real |
| `controlePareado` e `myController` guardavam a mesma informação | Uma única fonte de verdade |
| Robô liberado logo ao conectar | Começa travado (4.1) |
| Timeout desconectava o controle e desligava o Bluetooth, e só o botão religava | Timeout apenas trava (4.1) |
| `STBY` sempre HIGH, mesmo travado | `STBY` LOW quando travado (4.1) |
| Parar deixava o motor solto (`IN1 = IN2 = LOW`) | Freio curto quando liberado e parado (4.1) |
| Marcha não existia | △/✕ com 4 marchas (3) |

---

## 7. Ordem de execução

| Etapa | Responsável | Entrega |
|-------|-------------|---------|
| 1. Documentação | Programação | Este plano, setup e calibração |
| 2. Revisão do plano | Equipe | Plano aprovado |
| 3. `config.h` + `motores` | Programação | Motores com rampa, freio, STBY, PWM 20 kHz |
| 4. `controle` | Programação | Autorização por MAC, botões, luz, vibração |
| 5. `seguranca` + `.ino` | Programação | Estados, timeout, loop de 10 ms |
| 6. Compilar | Equipe | Compila sem erros na Arduino IDE |
| 7. Calibrar na bancada | Equipe | Valores finais no `config.h` ([CALIBRACAO.md](CALIBRACAO.md)) |
| 8. Teste na arena | Equipe | Piloto aprova marchas, curva e rampa |
| 9. Commit e PR | Equipe (manual) | Mudanças na `main` |

---

## 8. Avisos de hardware (fora do escopo)

| Ponto | Risco | Recomendação para uma revisão futura |
|-------|-------|--------------------------------------|
| **PWMB no GPIO12** | É pino de *strapping*: se estiver em HIGH no boot, o ESP32 pode não iniciar ou falhar na gravação | Mover o PWMB para um GPIO livre sem função de boot (ex.: 13, 18, 19, 23) |
| **Botão no GPIO34** | Pino só de entrada e **sem pull-up interno**; sem resistor externo a leitura flutua | Resistor de pull-up externo (10 kΩ para 3,3 V) |
| **Quedas de tensão** | O ESP32 pode reiniciar quando os motores puxam corrente | A rampa ajuda, mas a solução é na alimentação (capacitores, regulador) |
| **Aquecimento do driver** | O TB6612FNG aguenta ~1,2 A contínuo por canal | Desligar no travado ajuda; avaliar o consumo dos motores |
