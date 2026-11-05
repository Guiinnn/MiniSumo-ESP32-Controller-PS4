#pragma once

// CONTROLE PS4 via Bluepad32: conexão, autorização por MAC, leitura,
// marchas, luz e vibração.
//
// Faz a primeira parte do pipeline (docs/PLANO.md, seção 4.2):
//   leitura -> zona morta circular -> curva (EXPO) -> mistura -> marcha
// O resto (trim, força mínima, rampa, inversão) fica em motores.

struct Comandos {
  bool conectado;     // há um controle autorizado conectado
  bool conectou;      // o controle conectou neste ciclo
  bool desconectou;   // o controle desconectou neste ciclo
  bool apertouTrava;  // R1 foi apertado neste ciclo (segurar não repete)
  bool atividade;     // analógico fora da zona morta ou algum botão apertado
  float esquerdo;     // força do motor esquerdo (-1.0 a 1.0), já com a marcha
  float direito;      // força do motor direito (-1.0 a 1.0), já com a marcha
  int marcha;         // marcha atual (1 a NUM_MARCHAS)
};

// Liga o Bluetooth e registra os callbacks. Chamar uma vez no setup().
void controleIniciar();

// Atualiza o Bluepad32 e lê o controle. Chamar uma vez por ciclo.
// △/✕ só trocam a marcha com o robô liberado.
Comandos controleLer();

// Informa o estado da trava. Chamar uma vez por ciclo.
// Ajusta a luz (vermelho = travado, senão a cor da marcha) e vibra
// curto quando o estado muda.
void controleMostrarEstado(bool travado);

// Apaga os pareamentos salvos no ESP32.
void controleEsquecerPareamentos();
