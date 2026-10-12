#ifndef VNS_HPP
#define VNS_HPP

#include <algorithm>
#include "Perturbacao.hpp"
#include "Rvnd.hpp"

class Vns {
public:
    static Solucao otimizar(const Solucao& inicial, int iteracoes, const Grafo& I, Rng& rng) {
        Rvnd rvnd(rng);
        Solucao atual = inicial;
        rvnd.otimizar(atual, I);
        Solucao melhor = atual;
        const int kMax = 3;
        const int passo = std::max(2, (int)(0.06 * I.n()));  // k-esima vizinhanca = k*passo clientes removidos

        for (int it = 0; it < iteracoes; ++it) {
            int k = 1;
            while (k <= kMax) {
                Solucao cand = atual;
                perturbar(cand, k * passo, I, rng);
                rvnd.otimizar(cand, I);
                if (cand.lucroTotal > atual.lucroTotal + EPS) {
                    atual = cand;
                    if (cand.lucroTotal > melhor.lucroTotal) melhor = cand;
                    k = 1;
                } else ++k;
            }
        }
        return melhor;
    }
};

#endif
