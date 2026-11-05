# 🛠️ Configuração da Arduino IDE

Passo a passo para compilar e gravar o código do robô. Siga na ordem.

---

## Resumo

| Item | Valor |
|------|-------|
| IDE | Arduino IDE 2.x |
| Pacote de placas | **esp32_bluepad32**, de Ricardo Quesada, versão **4.1.0** |
| Placa | **ESP32 Arduino + Bluepad32 → ESP32 Dev Module** |
| Bibliotecas extras | Nenhuma (o Bluepad32 já vem no pacote) |
| Monitor Serial | 115200 baud |

---

## 1. URLs adicionais de placas

Em **Arquivo → Preferências**, no campo **"URLs adicionais do Gerenciador de placas"**, adicione as duas URLs (uma por linha):

```text
https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
https://raw.githubusercontent.com/ricardoquesada/esp32-arduino-lib-builder/master/bluepad32_files/package_esp32_bluepad32_index.json
```

| URL | O que oferece | Usada por este projeto? |
|-----|---------------|-------------------------|
| `espressif/arduino-esp32` | Pacote **esp32** oficial da Espressif | Não diretamente. Pode ficar instalada, mas **não** é a placa a escolher |
| `ricardoquesada/...bluepad32...` | Pacote **esp32_bluepad32** (ESP32 + Bluepad32) | **Sim**. É o que compila este código |

---

## 2. Instalar o pacote Bluepad32

1. **Ferramentas → Placa → Gerenciador de placas...**
2. Pesquise `bluepad32`.
3. Instale **esp32_bluepad32** (Ricardo Quesada), versão **4.1.0**.

> Use a mesma versão em todos os computadores da equipe. Outra versão pode mudar a API e quebrar a compilação.

---

## 3. Selecionar a placa certa

**Ferramentas → Placa → ESP32 Arduino + Bluepad32 → ESP32 Dev Module**

### ⚠️ Armadilha: existem dois "ESP32 Dev Module"

Com as duas URLs instaladas, o menu de placas mostra **dois grupos** com uma placa chamada **ESP32 Dev Module**:

| Grupo no menu | Resultado |
|---------------|-----------|
| **ESP32 Arduino + Bluepad32** | ✅ Compila |
| **esp32** (Espressif) | ❌ Não compila: falta o `Bluepad32.h` e o núcleo 3.x não tem `ledcSetup`/`ledcAttachPin` |

Confira sempre o **nome do grupo**, não só o nome da placa.

---

## 4. Porta e gravação

1. Conecte o ESP32 pelo USB.
2. **Ferramentas → Porta**: escolha a porta do ESP32 (Linux: `/dev/ttyUSB0` ou `/dev/ttyACM0`; Windows: `COMx`).
3. Abra `Controle_BluePad/Controle_BluePad.ino`. As outras abas (`config.h`, `motores`, `controle`, `seguranca`) abrem junto.
4. Clique em **Carregar** (→).
5. Abra o **Monitor Serial** em **115200 baud** para ver as mensagens.

### Linux: permissão na porta serial

Se a gravação falhar com "Permission denied", adicione seu usuário ao grupo da porta serial e reinicie a sessão:

```bash
sudo usermod -aG uucp $USER
```

Em distribuições baseadas em Debian/Ubuntu, o grupo é `dialout` em vez de `uucp`.

---

## 5. Problemas comuns

| Sintoma | Causa provável | Solução |
|---------|----------------|---------|
| `Bluepad32.h: No such file or directory` | Placa do grupo Espressif selecionada | Seção 3 |
| `'ledcSetup' was not declared in this scope` | Placa do grupo Espressif (núcleo 3.x) | Seção 3 |
| `Failed to connect to ESP32: Wrong boot mode` | ESP32 não entrou em modo de gravação | Segure o botão **BOOT** da placa ao começar a gravação |
| ESP32 não inicia ou falha na gravação com o driver ligado | **GPIO12** (PWMB) em HIGH no boot | Desconecte o fio do GPIO12 para gravar; ver avisos de hardware em [PLANO.md](PLANO.md#8-avisos-de-hardware-fora-do-escopo) |
| Porta não aparece | Cabo só de carga ou driver USB faltando | Troque o cabo; no Windows, instale o driver CP210x ou CH340 |
| Monitor Serial com caracteres estranhos | Velocidade errada | Use 115200 baud |

---

Próximo passo depois de gravar: [CALIBRACAO.md](CALIBRACAO.md).
