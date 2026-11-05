// Mini sumô RC: ESP32 + controle PS4 (Bluepad32) + driver TB6612FNG
//
// Este arquivo só orquestra os módulos:
//   config.h     pinos, MACs, marchas e ajustes (o único arquivo a editar na calibração)
//   motores      driver TB6612FNG: PWM, sentido, freio, STBY, rampa
//   controle     Bluepad32: conexão, autorização, leitura, luz, vibração
//   seguranca    travado/liberado, timeout, botão físico
// Detalhes em docs/PLANO.md.

#include "config.h"
#include "motores.h"
#include "controle.h"
#include "seguranca.h"

void imprimirLog(const Comandos& comandos, bool travado) {
  static unsigned long ultimoLog = 0;
  if (!LOG_SERIAL || !comandos.conectado) return;

  unsigned long agora = millis();
  if (agora - ultimoLog < LOG_PERIODO_MS) return;
  ultimoLog = agora;

  Serial.printf("%s | marcha %d | esq %+5.2f dir %+5.2f\n",
                travado ? "TRAVADO " : "LIBERADO",
                comandos.marcha,
                motoresSaidaEsquerdo(),
                motoresSaidaDireito());
}

void setup() {
  // Motores primeiro: o driver fica desligado (STBY em LOW) desde o início.
  motoresIniciar();

  Serial.begin(115200);

  Serial.println("\n\n========================================");
  Serial.println("Mini sumô RC - ESP32 + PS4 (TB6612FNG)");
  Serial.println("========================================");
  Serial.println("Analógico esquerdo = pilota");
  Serial.println("R1 = trava / destrava");
  Serial.println("△ = sobe marcha | ✕ = desce marcha");
  Serial.printf("Trava sozinho após %lu s sem comandos\n", (unsigned long)(TIMEOUT_INATIVIDADE_MS / 1000));
  Serial.println("========================================");

  controleIniciar();
  segurancaIniciar();

  Serial.println("Robô TRAVADO. Conecte o controle e pressione R1 para liberar.\n");
}

void loop() {
  // Ciclo de período fixo, sem delay()
  static unsigned long proximoCiclo = 0;
  unsigned long agora = millis();
  if ((long)(agora - proximoCiclo) < 0) return;
  proximoCiclo = agora + LOOP_PERIODO_MS;

  Comandos comandos = controleLer();
  bool travado = segurancaAtualizar(comandos);

  motoresHabilitar(!travado);
  motoresAplicar(comandos.esquerdo, comandos.direito);

  controleMostrarEstado(travado);
  imprimirLog(comandos, travado);
}
