#ifndef ILS_HPP
#define ILS_HPP

#include <algorithm>
#include <cmath>
#include "Perturbacao.hpp"
#include "Rvnd.hpp"

class Ils {
public:
    static Solucao otimizar(const Solucao& inicial, int iteracoes, const Grafo& I, Rng& rng) {
        Rvnd rvnd(rng);
        Solucao atual = inicial;
        rvnd.otimizar(atual, I);
        Solucao melhor = atual;

        const double T0 = std::max(1.0, 0.005 * std::fabs(atual.lucroTotal));  // temperatura relativa ao lucro
        double T = T0;
        int semMelhora = 0;
        const int base = std::max(2, (int)(0.1 * I.n()));

        for (int it = 0; it < iteracoes; ++it) {
            Solucao cand = atual;
            perturbar(cand, semMelhora > 100 ? 2 * base : base, I, rng);
            rvnd.otimizar(cand, I);

            const double delta = cand.lucroTotal - atual.lucroTotal;
            if (delta > EPS || rng.real() < std::exp(delta / T)) atual = cand;
            if (cand.lucroTotal > melhor.lucroTotal + EPS) { melhor = cand; semMelhora = 0; }
            else ++semMelhora;

            if (it % 10 == 0) T *= 0.98;
            if (semMelhora > 300) { T = T0; semMelhora = 0; }
        }
        return melhor;
    }
};

#endif
