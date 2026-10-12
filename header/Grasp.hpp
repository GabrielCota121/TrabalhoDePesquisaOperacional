#ifndef GRASP_HPP
#define GRASP_HPP

#include <algorithm>
#include <vector>
#include "Insercao.hpp"
#include "Rng.hpp"

// Fase construtiva do artigo (Algoritmo 1): a cada passo avalia todos os clientes ainda nao atendidos
// (melhor janela, rota existente x nova POR residuo), monta a RCL com os k mais lucrativos e sorteia um.
// UMA janela por cliente, igual para todos os residuos (sincronizacao).
inline Solucao construirGrasp(const Grafo& I, int k, Rng& rng) {
    Solucao s(I.n(), I.numLixos);
    while (true) {
        std::vector<Plano> cands;
        for (int c = 1; c < I.n(); ++c) {
            if (s.b[c] != -1 || I.lixosDoCliente[c].empty()) continue;
            Plano p = melhorPlanoCliente(s, c, I);
            // Aceita qualquer insercao viavel (mesmo com lucro marginal <= 0): a rota de um cliente pode so
            // se pagar depois, quando outros entrarem; o RVND (remocao) descarta o que continuar deficitario.
            if (p.viavel) cands.push_back(std::move(p));
        }
        if (cands.empty()) break;
        std::sort(cands.begin(), cands.end(), [](const Plano& a, const Plano& b) { return a.lucro > b.lucro; });
        const int rcl = std::min(k, (int)cands.size());
        aplicarPlano(s, cands[rng.inteiro(rcl)]);
    }
    s.calcularObjetivo(I);
    return s;
}

class Grasp {
    int k_;
public:
    explicit Grasp(int k = 3) : k_(k) {}
    Solucao construirSolucaoInicial(const Grafo& I, Rng& rng) const { return construirGrasp(I, k_, rng); }
};

#endif
