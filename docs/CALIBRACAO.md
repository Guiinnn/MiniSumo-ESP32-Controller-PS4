# 🔧 Calibração na Bancada

Roteiro para ajustar o `config.h` ao robô real. Faça na ordem: cada etapa depende da anterior.

Depois de cada mudança no `config.h`, grave de novo e repita o teste.

---

## ⚠️ Antes de começar

- **Robô suspenso:** apoie o chassi de forma que **as rodas não toquem a mesa**.
- **Bateria carregada:** com a bateria fraca, os motores respondem diferente e a calibração fica errada.
- **Monitor Serial aberto** em 115200 baud, com `LOG_SERIAL = true`.
- Tenha os **2 controles** da equipe em mãos.
- O robô começa **travado**: aperte **R1** para liberar e de novo para travar.

---

## 1. Cadastrar os controles

1. No `config.h`, deixe `CONTROLES_AUTORIZADOS` **vazio** (modo de cadastro).
2. Grave e ligue o ESP32.
3. Coloque o controle principal em modo de pareamento: segure **SHARE + PS** até a luz piscar rápido.
4. No Serial, aparece um aviso de modo de cadastro com o MAC do controle (formato `AA:BB:CC:DD:EE:FF`). Anote.
5. Desligue o controle (segure **PS** por 10 s) e repita com o controle reserva.
6. Coloque os dois MACs em `CONTROLES_AUTORIZADOS` e grave de novo.

**Teste:** os dois controles conectam (um de cada vez). Um terceiro controle qualquer é recusado e o MAC dele aparece no Serial.

> Nunca use o robô em luta com a lista vazia: qualquer controle por perto poderia assumir o robô.

---

## 2. Sentido dos motores

Com o robô liberado (R1), na 4ª marcha:

| Ação no analógico | Esperado |
|-------------------|----------|
| Para cima | As **duas rodas** giram para **frente** |
| Para baixo | As duas giram para **trás** |
| Para a direita | Roda **esquerda** para frente, **direita** para trás (gira no lugar para a direita) |
| Para a esquerda | Roda **direita** para frente, **esquerda** para trás |

Se algo estiver errado:

| Sintoma | Ajuste |
|---------|--------|
| Uma roda gira ao contrário em "para cima" | Inverta só aquele motor: `INVERTER_ESQ` ou `INVERTER_DIR` = `true` |
| As duas giram ao contrário em "para cima" | Inverta os dois |
| Frente/trás certos, mas a curva sai para o lado oposto | Os motores estão trocados (esquerdo ligado como direito): troque os blocos de pinos A ↔ B no `config.h` |

Refaça a tabela até tudo bater.

---

## 3. Zona morta

1. Solte o analógico. As rodas devem ficar **paradas**, sem tremer nem zumbir.
2. Se tremerem: aumente `ZONA_MORTA` de 10 em 10.
3. Se for preciso mover muito o analógico para começar a andar: diminua `ZONA_MORTA`.

Teste com os **dois controles**: o analógico de um pode estar mais gasto que o do outro. Use o valor que funciona para os dois.

---

## 4. Força mínima

1. Na 1ª marcha, mova o analógico **bem devagar** para cima, saindo da zona morta.
2. As rodas devem **começar a girar logo**, sem zumbir paradas.

| Sintoma | Ajuste |
|---------|--------|
| Zumbe, mas não gira | Aumente `FORCA_MINIMA` de 5 em 5 |
| Dá um tranco ao sair do zero | Diminua `FORCA_MINIMA` |

> Repita com o robô **no chão**: com peso, a força mínima necessária é maior.

---

## 5. Trim (equilíbrio entre motores)

Esta etapa é feita **no chão**, em uma superfície lisa.

1. Na 4ª marcha, empurre o analógico **reto para cima** e solte o robô por ~1 m.
2. Observe para que lado ele puxa.

| Sintoma | Ajuste |
|---------|--------|
| Puxa para a **direita** (esquerdo mais forte) | Diminua `TRIM_ESQ` (ex.: 100 → 95) |
| Puxa para a **esquerda** (direito mais forte) | Diminua `TRIM_DIR` |

- Ajuste de 2 em 2 ou de 5 em 5.
- **Só diminua o motor mais forte**: o trim nunca passa de 100%.
- Confira também na 2ª marcha. Se o desvio mudar muito entre marchas, a diferença está na força mínima de cada motor.

---

## 6. Curva de resposta (EXPO)

Peça para o **piloto** testar. É uma questão de preferência.

| Sensação do piloto | Ajuste |
|--------------------|--------|
| "Qualquer toque já dispara o robô" | Aumente `EXPO` (ex.: 0.4 → 0.6) |
| "Preciso empurrar muito para ganhar força" | Diminua `EXPO` (ex.: 0.4 → 0.2) |

`EXPO = 0` é linear (como era antes).

---

## 7. Rampa

1. No chão, na 4ª marcha, faça **inversões bruscas**: analógico todo para cima e, de uma vez, todo para baixo.
2. Observe o robô e o Serial.

| Sintoma | Ajuste |
|---------|--------|
| ESP32 **reinicia** (Serial mostra o boot de novo) ou o controle desconecta | Aumente `RAMPA_MS` (ex.: 100 → 150) |
| Robô "empina" ou patina nas arrancadas | Aumente `RAMPA_MS` |
| Piloto sente o robô lento para reagir | Diminua `RAMPA_MS` (ex.: 100 → 60) |

Mantenha a rampa **o mais curta possível** sem reiniciar o ESP32. `RAMPA_MS = 0` desliga a rampa.

---

## 8. Marchas

1. Com o piloto, teste as 4 marchas com △ (sobe) e ✕ (desce).
2. Confira a cor da luz: 🔵 1ª, 🟣 2ª, 🟡 3ª, 🟢 4ª, 🔴 travado.
3. Ajuste os percentuais de `MARCHAS` conforme a necessidade.

Sugestão de uso: uma marcha baixa para posicionar com precisão e a 4ª para empurrar.

---

## 9. Segurança (checklist final)

Faça todos os itens antes de usar o robô em luta:

- [ ] Ao ligar o ESP32, o robô está **travado** (luz vermelha, motores sem força).
- [ ] Ao conectar o controle, o robô está **travado**.
- [ ] R1 trava e destrava, com uma vibração curta a cada troca.
- [ ] **Segurar** o R1 não fica alternando a trava.
- [ ] Travado, o analógico **não** move as rodas, e as rodas giram livres com a mão.
- [ ] Liberado e com o analógico solto, as rodas estão **freadas**.
- [ ] Desligando o controle (segure PS por 10 s) com o robô andando, os motores **param na hora**.
- [ ] O controle reconecta sozinho ao ser ligado de novo, e o robô volta **travado**.
- [ ] Um controle que não está na lista é recusado.
- [ ] Para testar o timeout, coloque temporariamente `TIMEOUT_INATIVIDADE_MS = 10000` (10 s): sem comandos, o robô trava e o controle **continua conectado**. **Volte para 300000 depois.**
- [ ] `CONTROLES_AUTORIZADOS` contém os 2 MACs (não está vazio).

---

## Registro da calibração

Anote os valores finais aqui ou no próprio `config.h`, com a data:

| Parâmetro | Valor | Data | Observação |
|-----------|-------|------|------------|
| `ZONA_MORTA` | | | |
| `FORCA_MINIMA` | | | |
| `TRIM_ESQ` / `TRIM_DIR` | | | |
| `INVERTER_ESQ` / `INVERTER_DIR` | | | |
| `EXPO` | | | |
| `RAMPA_MS` | | | |
| `MARCHAS` | | | |
