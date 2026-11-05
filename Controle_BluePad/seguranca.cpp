#include "seguranca.h"
#include "config.h"

namespace {

bool travado = true;
unsigned long ultimaAtividade = 0;

bool botaoSegurando = false;
bool botaoJaAgiu = false;  // age uma vez por aperto
unsigned long botaoApertadoDesde = 0;

void travar(const char* motivo) {
  if (travado) return;
  travado = true;
  Serial.printf("\nROBÔ TRAVADO (%s) - Pressione R1 para liberar\n", motivo);
}

void liberar() {
  if (!travado) return;
  travado = false;
  Serial.println("\nROBÔ LIBERADO");
}

// Segurar o botão por BOTAO_SEGURAR_MS esquece os pareamentos salvos.
void lerBotao(unsigned long agora) {
  if (!BOTAO_ATIVO) return;

  bool apertado = digitalRead(BOTAO_PIN) == LOW;
  if (!apertado) {
    botaoSegurando = false;
    botaoJaAgiu = false;
    return;
  }

  if (!botaoSegurando) {
    botaoSegurando = true;
    botaoApertadoDesde = agora;
  }

  if (!botaoJaAgiu && agora - botaoApertadoDesde >= BOTAO_SEGURAR_MS) {
    botaoJaAgiu = true;
    controleEsquecerPareamentos();
    Serial.println("\nPareamentos apagados. Pareie o controle de novo (SHARE + PS).");
  }
}

}  // namespace

void segurancaIniciar() {
  travado = true;
  ultimaAtividade = millis();

  if (BOTAO_ATIVO) {
    pinMode(BOTAO_PIN, INPUT);  // GPIO34 não tem pull-up interno
  }
}

bool segurancaAtualizar(const Comandos& comandos) {
  unsigned long agora = millis();

  lerBotao(agora);

  if (comandos.conectou) {
    travar("controle conectou");
    ultimaAtividade = agora;
  }

  if (!comandos.conectado) {
    travar("controle desconectado");
    return travado;
  }

  if (comandos.atividade) ultimaAtividade = agora;

  if (comandos.apertouTrava) {
    if (travado) liberar();
    else travar("R1");
  }

  if (!travado && agora - ultimaAtividade >= TIMEOUT_INATIVIDADE_MS) {
    travar("sem comandos por muito tempo");
  }

  return travado;
}
