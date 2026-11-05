#include "motores.h"
#include "config.h"

namespace {

// Canais PWM (LEDC) do ESP32
constexpr uint8_t PWMA_CANAL = 0;
constexpr uint8_t PWMB_CANAL = 1;
constexpr uint32_t PWM_MAX = (1UL << PWM_RESOLUCAO) - 1;

struct Motor {
  uint8_t in1;
  uint8_t in2;
  uint8_t canal;
  float trim;     // 0.0 a 1.0
  bool inverter;
  float saida;    // força atual, depois da rampa (-1.0 a 1.0)
};

Motor motorEsquerdo = {AIN1, AIN2, PWMA_CANAL, TRIM_ESQ / 100.0f, INVERTER_ESQ, 0.0f};
Motor motorDireito  = {BIN1, BIN2, PWMB_CANAL, TRIM_DIR / 100.0f, INVERTER_DIR, 0.0f};

bool habilitado = false;
unsigned long ultimaAtualizacao = 0;

// Trim e força mínima: 0 continua 0; o resto vai de FORCA_MINIMA a 100% (vezes o trim).
float aplicarTrimEForcaMinima(const Motor& m, float forca) {
  forca = constrain(forca, -1.0f, 1.0f) * m.trim;
  if (forca == 0.0f) return 0.0f;

  const float minima = FORCA_MINIMA / 100.0f;
  float modulo = minima + (1.0f - minima) * fabsf(forca);
  return forca > 0.0f ? modulo : -modulo;
}

// Aproxima "atual" de "alvo" andando no máximo "passo".
float aplicarRampa(float atual, float alvo, float passo) {
  if (alvo > atual) return fminf(alvo, atual + passo);
  return fmaxf(alvo, atual - passo);
}

void escrever(const Motor& m) {
  float forca = m.inverter ? -m.saida : m.saida;
  uint32_t duty = lroundf(fabsf(forca) * PWM_MAX);

  if (duty == 0) {
    // Freio curto
    digitalWrite(m.in1, HIGH);
    digitalWrite(m.in2, HIGH);
  } else if (forca > 0.0f) {
    digitalWrite(m.in1, HIGH);
    digitalWrite(m.in2, LOW);
  } else {
    digitalWrite(m.in1, LOW);
    digitalWrite(m.in2, HIGH);
  }

  ledcWrite(m.canal, duty);
}

// Motor solto (sem freio) e sem PWM.
void soltar(Motor& m) {
  m.saida = 0.0f;
  digitalWrite(m.in1, LOW);
  digitalWrite(m.in2, LOW);
  ledcWrite(m.canal, 0);
}

}  // namespace

void motoresIniciar() {
  // STBY primeiro: o driver fica desligado durante toda a configuração.
  pinMode(STBY, OUTPUT);
  digitalWrite(STBY, LOW);

  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);
  pinMode(BIN1, OUTPUT);
  pinMode(BIN2, OUTPUT);

  ledcSetup(PWMA_CANAL, PWM_FREQ, PWM_RESOLUCAO);
  ledcAttachPin(PWMA, PWMA_CANAL);
  ledcSetup(PWMB_CANAL, PWM_FREQ, PWM_RESOLUCAO);
  ledcAttachPin(PWMB, PWMB_CANAL);

  habilitado = false;
  soltar(motorEsquerdo);
  soltar(motorDireito);
  ultimaAtualizacao = millis();
}

void motoresHabilitar(bool habilitar) {
  if (!habilitar) {
    // Corta o driver antes de mexer nos pinos.
    digitalWrite(STBY, LOW);
    habilitado = false;
    soltar(motorEsquerdo);
    soltar(motorDireito);
    return;
  }

  if (habilitado) return;

  // Liga já freado, com a rampa começando do zero.
  motorEsquerdo.saida = 0.0f;
  motorDireito.saida = 0.0f;
  escrever(motorEsquerdo);
  escrever(motorDireito);
  ultimaAtualizacao = millis();
  habilitado = true;
  digitalWrite(STBY, HIGH);
}

void motoresAplicar(float esquerdo, float direito) {
  unsigned long agora = millis();
  unsigned long dt = agora - ultimaAtualizacao;
  ultimaAtualizacao = agora;

  if (!habilitado) return;

  // Sem rampa, o passo de 2.0 cobre qualquer troca (de -100% a 100%).
  float passo = (RAMPA_MS == 0) ? 2.0f : (float)dt / RAMPA_MS;

  motorEsquerdo.saida = aplicarRampa(motorEsquerdo.saida, aplicarTrimEForcaMinima(motorEsquerdo, esquerdo), passo);
  motorDireito.saida  = aplicarRampa(motorDireito.saida,  aplicarTrimEForcaMinima(motorDireito,  direito),  passo);

  escrever(motorEsquerdo);
  escrever(motorDireito);
}

float motoresSaidaEsquerdo() {
  return motorEsquerdo.saida;
}

float motoresSaidaDireito() {
  return motorDireito.saida;
}
