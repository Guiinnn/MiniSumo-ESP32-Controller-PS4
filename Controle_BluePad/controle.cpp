#include "controle.h"
#include "config.h"
#include <Bluepad32.h>

namespace {

struct Cor {
  uint8_t r, g, b;
};

constexpr Cor COR_TRAVADO = {255, 0, 0};  // Vermelho
constexpr Cor COR_MARCHAS[] = {
  {0, 0, 255},    // 1ª: azul
  {128, 0, 255},  // 2ª: roxo
  {255, 160, 0},  // 3ª: amarelo
  {0, 255, 0},    // 4ª: verde
};
static_assert(sizeof(COR_MARCHAS) / sizeof(COR_MARCHAS[0]) >= NUM_MARCHAS,
              "Falta cor para alguma marcha em COR_MARCHAS (controle.cpp)");

constexpr uint16_t VIBRACAO_MS = 150;
constexpr uint8_t VIBRACAO_FORCA = 128;     // 0 a 255
constexpr uint32_t REENVIO_LUZ_MS = 1000;   // reenvia a luz após conectar (o controle pode sobrescrever no início)
constexpr float EIXO_MAX = 512.0f;          // analógico vai de -511 a 512

ControllerPtr controlador = nullptr;  // única fonte de verdade: nullptr = sem controle
bool conectouAgora = false;
bool desconectouAgora = false;
unsigned long reenviarLuzEm = 0;      // 0 = nada pendente

int marcha = MARCHA_INICIAL;
bool travado = true;

// Estado anterior dos botões, para detectar o aperto. Começam "apertados"
// para que um botão segurado durante a conexão não conte como aperto.
bool r1Antes = true;
bool trianguloAntes = true;
bool xisAntes = true;

void formatarMac(const uint8_t* mac, char* texto) {
  snprintf(texto, 18, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

bool listaVazia() {
  return CONTROLES_AUTORIZADOS[0] == nullptr;
}

bool autorizado(const char* mac) {
  for (int i = 0; CONTROLES_AUTORIZADOS[i] != nullptr; i++) {
    if (strcasecmp(CONTROLES_AUTORIZADOS[i], mac) == 0) return true;
  }
  return false;
}

void atualizarLuz() {
  if (controlador == nullptr) return;
  const Cor& cor = travado ? COR_TRAVADO : COR_MARCHAS[marcha - 1];
  controlador->setColorLED(cor.r, cor.g, cor.b);
}

void aoConectar(ControllerPtr ctl) {
  char mac[18];
  formatarMac(ctl->getProperties().btaddr, mac);

  if (!ctl->isGamepad()) {
    Serial.printf("RECUSADO: dispositivo %s não é um controle\n", mac);
    ctl->disconnect();
    return;
  }

  if (controlador != nullptr) {
    Serial.printf("RECUSADO: %s tentou conectar, mas já há um controle conectado\n", mac);
    ctl->disconnect();
    return;
  }

  if (listaVazia()) {
    Serial.println("\n!!! MODO DE CADASTRO: CONTROLES_AUTORIZADOS está vazio !!!");
    Serial.printf("!!! Controle aceito: %s  -> copie para o config.h\n", mac);
    Serial.println("!!! Use só na bancada.\n");
  } else if (!autorizado(mac)) {
    Serial.printf("RECUSADO: controle %s não está em CONTROLES_AUTORIZADOS\n", mac);
    ctl->disconnect();
    return;
  }

  controlador = ctl;
  conectouAgora = true;
  BP32.enableNewBluetoothConnections(false);  // um por vez

  marcha = MARCHA_INICIAL;
  r1Antes = trianguloAntes = xisAntes = true;
  reenviarLuzEm = millis() + REENVIO_LUZ_MS;

  Serial.printf("Controle conectado: %s (%s)\n", mac, ctl->getModelName().c_str());
}

void aoDesconectar(ControllerPtr ctl) {
  if (ctl != controlador) return;  // um controle recusado saindo

  controlador = nullptr;
  desconectouAgora = true;
  BP32.enableNewBluetoothConnections(true);
  Serial.println("Controle desconectado! Aguardando reconexão...");
}

// Lê o analógico esquerdo com zona morta circular e curva EXPO.
// x e y saem de -1.0 a 1.0 (y positivo = para cima).
// Retorna true se o analógico estiver fora da zona morta.
bool lerAnalogico(float& x, float& y) {
  float eixoX = controlador->axisX();
  float eixoY = -controlador->axisY();  // No Bluepad32 o Y é negativo para cima

  float raio = sqrtf(eixoX * eixoX + eixoY * eixoY);
  if (raio <= ZONA_MORTA) {
    x = y = 0.0f;
    return false;
  }

  // Reescala para começar do zero na borda da zona morta (diagonais preservadas)
  float modulo = fminf((raio - ZONA_MORTA) / (EIXO_MAX - ZONA_MORTA), 1.0f);
  modulo = (1.0f - EXPO) * modulo + EXPO * modulo * modulo * modulo;

  x = eixoX / raio * modulo;
  y = eixoY / raio * modulo;
  return true;
}

}  // namespace

void controleIniciar() {
  BP32.setup(&aoConectar, &aoDesconectar);
  BP32.enableVirtualDevice(false);  // sem o touchpad do PS4 como mouse
  BP32.enableNewBluetoothConnections(true);

  Serial.printf("Bluepad32 %s | Bluetooth ligado\n", BP32.firmwareVersion());
  if (listaVazia()) {
    Serial.println("!!! CONTROLES_AUTORIZADOS vazio: modo de cadastro (qualquer controle conecta) !!!");
  }
}

Comandos controleLer() {
  conectouAgora = false;
  desconectouAgora = false;
  BP32.update();  // os callbacks rodam aqui dentro

  Comandos c = {};
  c.conectou = conectouAgora;
  c.desconectou = desconectouAgora;
  c.marcha = marcha;

  if (controlador == nullptr || !controlador->isConnected()) return c;
  c.conectado = true;

  if (c.conectou) atualizarLuz();
  if (reenviarLuzEm != 0 && (long)(millis() - reenviarLuzEm) >= 0) {
    reenviarLuzEm = 0;
    atualizarLuz();
  }

  if (!controlador->hasData()) return c;

  // Botões: só conta o momento do aperto
  bool r1 = controlador->r1();
  bool triangulo = controlador->y();  // △ = y() no Bluepad32
  bool xis = controlador->a();        // ✕ = a() no Bluepad32

  c.apertouTrava = r1 && !r1Antes;
  bool sobe = triangulo && !trianguloAntes;
  bool desce = xis && !xisAntes;

  r1Antes = r1;
  trianguloAntes = triangulo;
  xisAntes = xis;

  // Marchas não dão a volta (na 4ª, △ não faz nada; na 1ª, ✕ não faz nada)
  if (!travado) {
    int nova = marcha;
    if (sobe && nova < NUM_MARCHAS) nova++;
    if (desce && nova > 1) nova--;
    if (nova != marcha) {
      marcha = nova;
      atualizarLuz();
    }
  }
  c.marcha = marcha;

  // Analógico -> mistura normalizada -> marcha
  float x, y;
  bool foraDaZonaMorta = lerAnalogico(x, y);

  float esquerdo = y + x;
  float direito = y - x;
  float maior = fmaxf(fabsf(esquerdo), fabsf(direito));
  if (maior > 1.0f) {
    esquerdo /= maior;
    direito /= maior;
  }

  const float fatorMarcha = MARCHAS[marcha - 1] / 100.0f;
  c.esquerdo = esquerdo * fatorMarcha;
  c.direito = direito * fatorMarcha;

  c.atividade = foraDaZonaMorta || c.apertouTrava || sobe || desce;
  return c;
}

void controleMostrarEstado(bool estaTravado) {
  if (estaTravado == travado) return;
  travado = estaTravado;

  if (controlador == nullptr) return;
  atualizarLuz();
  controlador->playDualRumble(0, VIBRACAO_MS, VIBRACAO_FORCA, VIBRACAO_FORCA);
}

void controleEsquecerPareamentos() {
  BP32.forgetBluetoothKeys();
}
