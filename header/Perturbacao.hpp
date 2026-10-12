#ifndef PERTURBACAO_HPP
#define PERTURBACAO_HPP

#include <algorithm>
#include <vector>
#include "Insercao.hpp"
#include "Rng.hpp"

// Shake ESTRUTURAL (substitui o ruido na matriz de distancias): remove `forca` clientes atendidos
// aleatorios e tenta reinseri-los (junto com alguns excluidos) numa ordem aleatoria, comecando por
// uma janela sorteada, o que tambem diversifica a escolha de janelas.
inline void perturbar(Solucao& s, int forca, const Grafo& I, Rng& rng) {
    std::vector<int> aceitos, excluidos;
    for (int c = 1; c < I.n(); ++c) {
        if (I.lixosDoCliente[c].empty()) continue;
        (s.b[c] >= 0 ? aceitos : excluidos).push_back(c);
    }
    rng.embaralhar(aceitos.begin(), aceitos.end());
    rng.embaralhar(excluidos.begin(), excluidos.end());
    const int k = std::min(forca, (int)aceitos.size());
    std::vector<int> pool(aceitos.begin(), aceitos.begin() + k);
    for (int c : pool) removerCliente(s, c);
    const int extra = std::min((int)excluidos.size(), std::max(1, forca / 2));
    pool.insert(pool.end(), excluidos.begin(), excluidos.begin() + extra);
    rng.embaralhar(pool.begin(), pool.end());
    for (int c : pool) {
        const int t0 = rng.inteiro(3);
        for (int q = 0; q < 3; ++q) {
            const Plano p = planoInsercao(s, c, (t0 + q) % 3, I);
            if (p.viavel && p.lucro > EPS) { aplicarPlano(s, p); break; }
        }
    }
    s.calcularObjetivo(I);
}

#endif
