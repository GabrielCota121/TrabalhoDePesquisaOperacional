#ifndef GRASP_RVND_HPP
#define GRASP_RVND_HPP

#include "GraspReativo.hpp"
#include "Rvnd.hpp"

// Metodo central do artigo: multi-start GRASP -> RVND, guardando a melhor solucao.
// As primeiras `treino` iteracoes escolhem operadores uniformemente (treino do aprendizado online).
inline Solucao graspRvnd(const Grafo& I, int iteracoes, int treino, Rng& rng, int k = 3,
                         GraspReativo* reativo = nullptr) {
    Rvnd rvnd(rng, treino);
    Solucao melhor;
    bool tem = false;
    for (int it = 0; it < iteracoes; ++it) {
        int idx = -1;
        Solucao s;
        if (reativo) { idx = reativo->escolherK(rng); s = reativo->construirSolucaoInicial(I, idx, rng); }
        else s = construirGrasp(I, k, rng);
        rvnd.otimizar(s, I);
        if (reativo) reativo->registrarResultado(idx, s.lucroTotal);
        if (!tem || s.lucroTotal > melhor.lucroTotal) { melhor = s; tem = true; }
    }
    return melhor;
}

#endif
