#ifndef GRASP_REATIVO_HPP
#define GRASP_REATIVO_HPP

#include <algorithm>
#include <vector>
#include "Grasp.hpp"

// GRASP reativo: o tamanho k da RCL e sorteado de um conjunto; a cada bloco de iteracoes as
// probabilidades sao reajustadas pela media de lucro obtida com cada k.
class GraspReativo {
    std::vector<int> valoresK;
    std::vector<double> prob, somaLucros;
    std::vector<int> uso;
    int bloco, contador = 0;

    void atualizar() {
        const int m = (int)valoresK.size();
        double melhorMedia = 1e-9;
        for (int i = 0; i < m; ++i) if (uso[i] > 0) melhorMedia = std::max(melhorMedia, somaLucros[i] / uso[i]);
        std::vector<double> q(m);
        double soma = 0.0;
        for (int i = 0; i < m; ++i) {
            q[i] = uso[i] > 0 ? (somaLucros[i] / uso[i]) / melhorMedia : 0.1;
            q[i] = std::max(q[i], 0.05);  // nunca zera a probabilidade de um k
            soma += q[i];
        }
        for (int i = 0; i < m; ++i) { prob[i] = q[i] / soma; somaLucros[i] = 0.0; uso[i] = 0; }
    }

public:
    explicit GraspReativo(const std::vector<int>& opcoesK = {1, 2, 3, 5, 8}, int tamanhoBloco = 20)
        : valoresK(opcoesK), prob(opcoesK.size(), 1.0 / opcoesK.size()), somaLucros(opcoesK.size(), 0.0),
          uso(opcoesK.size(), 0), bloco(tamanhoBloco) {}

    int escolherK(Rng& rng) const {
        double p = rng.real(), acum = 0.0;
        for (size_t i = 0; i < prob.size(); ++i) { acum += prob[i]; if (p <= acum) return (int)i; }
        return (int)prob.size() - 1;
    }

    void registrarResultado(int idxK, double lucro) {
        uso[idxK]++;
        somaLucros[idxK] += lucro;
        if (++contador >= bloco) { atualizar(); contador = 0; }
    }

    Solucao construirSolucaoInicial(const Grafo& I, int idxK, Rng& rng) const {
        return construirGrasp(I, valoresK[idxK], rng);
    }
    const std::vector<double>& probabilidades() const { return prob; }
    const std::vector<int>& ks() const { return valoresK; }
};

#endif
