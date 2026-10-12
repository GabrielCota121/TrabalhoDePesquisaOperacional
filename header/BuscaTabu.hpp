#ifndef BUSCA_TABU_HPP
#define BUSCA_TABU_HPP

#include <vector>
#include "Insercao.hpp"
#include "Rng.hpp"

// Busca Tabu sobre a vizinhanca "realocar cliente":
//   (a) cliente atendido c e removido de TODAS as rotas e reinserido com uma janela t (mesma para
//       todos os residuos -> sincronizacao garantida) na melhor posicao viavel;
//   (b) cliente atendido c e apenas removido;
//   (c) cliente excluido u e inserido com janela t.
// Atributo tabu = cliente movido (nao pode ser movido de novo durante `tenure` iteracoes), com
// criterio de aspiracao (melhor que o recorde global). Nao ha relaxamento do buffer beta:
// toda solucao visitada e viavel segundo o Verificador.
class BuscaTabu {
    struct Candidato { int tipo = -1, c = -1, t = -1; double lucro = -1e18; };

public:
    static Solucao otimizar(const Solucao& inicial, int iteracoes, int tenure, const Grafo& I, Rng& rng) {
        Solucao atual = inicial, melhor = inicial;
        atual.calcularObjetivo(I);
        melhor = atual;
        std::vector<int> tabuAte(I.n(), 0);

        for (int it = 1; it <= iteracoes; ++it) {
            Candidato best;
            auto avaliar = [&](int tipo, int c, int t, double lucro) {
                const bool tabu = tabuAte[c] > it;
                if (tabu && lucro <= melhor.lucroTotal + EPS) return;  // aspiracao
                if (lucro > best.lucro) best = {tipo, c, t, lucro};
            };
            for (int c = 1; c < I.n(); ++c) {
                if (I.lixosDoCliente[c].empty()) continue;
                if (atual.b[c] >= 0) {
                    Solucao tmp = atual;
                    removerCliente(tmp, c);
                    tmp.calcularObjetivo(I);
                    avaliar(1, c, -1, tmp.lucroTotal);
                    for (int t = 0; t < 3; ++t) {
                        const Plano p = planoInsercao(tmp, c, t, I);
                        if (!p.viavel) continue;
                        const double lucro = tmp.lucroTotal + p.lucro;
                        if (t == atual.b[c] && std::fabs(lucro - atual.lucroTotal) < EPS) continue;  // movimento nulo
                        avaliar(0, c, t, lucro);
                    }
                } else {
                    for (int t = 0; t < 3; ++t) {
                        const Plano p = planoInsercao(atual, c, t, I);
                        if (p.viavel) avaliar(2, c, t, atual.lucroTotal + p.lucro);
                    }
                }
            }
            if (best.tipo < 0) {  // vizinhanca vazia: libera a memoria
                std::fill(tabuAte.begin(), tabuAte.end(), 0);
                continue;
            }
            if (best.tipo == 0 || best.tipo == 1) removerCliente(atual, best.c);
            if (best.tipo != 1) aplicarPlano(atual, planoInsercao(atual, best.c, best.t, I));
            atual.calcularObjetivo(I);
            tabuAte[best.c] = it + tenure + rng.inteiro(3);
            if (atual.lucroTotal > melhor.lucroTotal + EPS) melhor = atual;
        }
        return melhor;
    }
};

#endif
