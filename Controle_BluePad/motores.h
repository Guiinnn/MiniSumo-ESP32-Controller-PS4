#pragma once

// DRIVER TB6612FNG: PWM, sentido, freio, standby e rampa.
//
// Recebe a força de cada motor já com a marcha aplicada (-1.0 a 1.0) e
// faz o resto do pipeline (docs/PLANO.md, seção 4.2):
//   trim -> força mínima -> rampa -> inversão -> driver
// Força zero com o driver habilitado = freio curto (IN1 = IN2 = HIGH).

// Configura pinos e PWM. O driver começa desabilitado (STBY em LOW).
void motoresIniciar();

// true = STBY em HIGH, motores obedecem a motoresAplicar().
// false = STBY em LOW na hora, sem rampa (motores soltos, driver desligado).
void motoresHabilitar(bool habilitar);

// Chamar uma vez por ciclo. Ignorado com o driver desabilitado.
void motoresAplicar(float esquerdo, float direito);

// Força aplicada no último ciclo (-1.0 a 1.0, antes da inversão). Para o log.
float motoresSaidaEsquerdo();
float motoresSaidaDireito();
