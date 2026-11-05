#pragma once

// CONFIGURAÇÃO DO ROBÔ
// Todo valor que pode precisar de ajuste fica aqui.
// Roteiro de ajuste: docs/CALIBRACAO.md | Detalhes: docs/PLANO.md

#include <Arduino.h>

// ---------------------------------------------------------------
// PINOS DO TB6612FNG
// Motor A = Motor Esquerdo | Motor B = Motor Direito
// ---------------------------------------------------------------
constexpr uint8_t AIN1 = 25;  // Sentido do motor esquerdo
constexpr uint8_t AIN2 = 33;  // Sentido do motor esquerdo
constexpr uint8_t PWMA = 32;  // PWM do motor esquerdo

constexpr uint8_t BIN1 = 27;  // Sentido do motor direito
constexpr uint8_t BIN2 = 14;  // Sentido do motor direito
constexpr uint8_t PWMB = 12;  // PWM do motor direito (GPIO12 é pino de strapping, ver PLANO.md seção 8)

constexpr uint8_t STBY = 26;  // HIGH = driver ligado | LOW = driver desligado (travado)

// ---------------------------------------------------------------
// BOTÃO FÍSICO
// GPIO34 é só entrada e não tem pull-up interno: só ative com
// resistor de pull-up externo na placa (apertado = LOW).
// ---------------------------------------------------------------
constexpr uint8_t BOTAO_PIN = 34;
constexpr bool BOTAO_ATIVO = false;         // true = segurar o botão esquece os pareamentos
constexpr uint32_t BOTAO_SEGURAR_MS = 2000; // Tempo segurando para esquecer os pareamentos

// ---------------------------------------------------------------
// CONTROLES AUTORIZADOS (MAC Bluetooth, formato "AA:BB:CC:DD:EE:FF")
// Lista vazia = modo de cadastro: aceita qualquer controle e mostra o
// MAC no Serial. Usar só na bancada.
// ---------------------------------------------------------------
static const char* const CONTROLES_AUTORIZADOS[] = {
  // "AA:BB:CC:DD:EE:FF",  // controle principal
  // "AA:BB:CC:DD:EE:FF",  // controle reserva
  nullptr  // fim da lista (não remover)
};

// ---------------------------------------------------------------
// MARCHAS (força máxima de cada marcha, em %)
// ---------------------------------------------------------------
constexpr int MARCHAS[] = {25, 50, 75, 100};
constexpr int NUM_MARCHAS = sizeof(MARCHAS) / sizeof(MARCHAS[0]);
constexpr int MARCHA_INICIAL = 4;  // 1 a NUM_MARCHAS

// ---------------------------------------------------------------
// RESPOSTA DO ANALÓGICO E DOS MOTORES
// ---------------------------------------------------------------
constexpr int ZONA_MORTA = 40;      // Raio da zona morta (escala do analógico, 0 a 512)
constexpr float EXPO = 0.4f;        // Curva: 0 = linear, 1 = muito suave no começo
constexpr int FORCA_MINIMA = 15;    // % de força ao sair da zona morta
constexpr int RAMPA_MS = 100;       // Tempo de 0 a 100% de força (0 = sem rampa)

constexpr int TRIM_ESQ = 100;       // % de força do motor esquerdo (só diminua o mais forte)
constexpr int TRIM_DIR = 100;       // % de força do motor direito

constexpr bool INVERTER_ESQ = false;  // Inverte o sentido do motor esquerdo
constexpr bool INVERTER_DIR = false;  // Inverte o sentido do motor direito

// ---------------------------------------------------------------
// PWM (LEDC do ESP32)
// ---------------------------------------------------------------
constexpr uint32_t PWM_FREQ = 20000;  // Hz (acima da faixa audível)
constexpr uint8_t PWM_RESOLUCAO = 8;  // bits (0 a 255)

// ---------------------------------------------------------------
// TEMPOS E LOG
// ---------------------------------------------------------------
constexpr uint32_t TIMEOUT_INATIVIDADE_MS = 300000;  // 5 min sem comando -> trava
constexpr uint32_t LOOP_PERIODO_MS = 10;             // Período do ciclo principal (~100 Hz)
constexpr bool LOG_SERIAL = true;                    // Liga o log no Serial
constexpr uint32_t LOG_PERIODO_MS = 200;             // Intervalo entre linhas de log

// ---------------------------------------------------------------
// Verificações em tempo de compilação (pegam erro de digitação na calibração)
// ---------------------------------------------------------------
static_assert(MARCHA_INICIAL >= 1 && MARCHA_INICIAL <= NUM_MARCHAS, "MARCHA_INICIAL fora da faixa");
static_assert(ZONA_MORTA >= 0 && ZONA_MORTA < 512, "ZONA_MORTA deve ficar entre 0 e 511");
static_assert(EXPO >= 0.0f && EXPO <= 1.0f, "EXPO deve ficar entre 0 e 1");
static_assert(FORCA_MINIMA >= 0 && FORCA_MINIMA < 100, "FORCA_MINIMA deve ficar entre 0 e 99");
static_assert(RAMPA_MS >= 0, "RAMPA_MS não pode ser negativo");
static_assert(TRIM_ESQ > 0 && TRIM_ESQ <= 100, "TRIM_ESQ deve ficar entre 1 e 100");
static_assert(TRIM_DIR > 0 && TRIM_DIR <= 100, "TRIM_DIR deve ficar entre 1 e 100");
static_assert(PWM_RESOLUCAO >= 1 && PWM_RESOLUCAO <= 16, "PWM_RESOLUCAO deve ficar entre 1 e 16");
