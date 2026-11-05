#pragma once

// SEGURANÇA: estados travado/liberado, timeout de inatividade e botão físico
// (docs/PLANO.md, seção 4.1 e 4.4).
//
//   Liga / controle conecta / controle desconecta / timeout -> TRAVADO
//   R1 -> alterna travado <-> liberado

#include "controle.h"

// Configura o botão físico (se BOTAO_ATIVO). O robô começa travado.
void segurancaIniciar();

// Atualiza o estado com os comandos do ciclo. Chamar uma vez por ciclo.
// Retorna true se o robô estiver travado.
bool segurancaAtualizar(const Comandos& comandos);
