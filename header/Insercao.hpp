#ifndef INSERCAO_HPP
#define INSERCAO_HPP

#include <vector>
#include "Solucao.hpp"
#include "Verificador.hpp"

// ---------- utilitarios de distancia ----------
inline double custoInsDist(const std::vector<int>& r, int pos, int c, const Grafo& I) {
    const int prev = pos == 0 ? 0 : r[pos - 1];
    const int next = pos == (int)r.size() ? 0 : r[pos];
    return I.dist[prev][c] + I.dist[c][next] - I.dist[prev][next];
}
inline double custoRemDist(const std::vector<int>& r, int i, const Grafo& I) {
    const int c = r[i];
    const int prev = i == 0 ? 0 : r[i - 1];
    const int next = i + 1 == (int)r.size() ? 0 : r[i + 1];
    return I.dist[prev][c] + I.dist[c][next] - I.dist[prev][next];
}
inline int cargaRota(const std::vector<int>& r, int w, const Grafo& I) {
    int c = 0;
    for (int id : r) c += I.clientes[id].demanda(w);
    return c;
}

// ---------- insercao de um cliente com UMA janela para TODOS os seus residuos ----------
struct Posicao { int w; int v; int pos; };   // v == -1: nova rota; v == -2: inviavel

struct Plano {
    int cliente = -1, janela = -1;
    bool viavel = false;
    double custo = 0.0, lucro = -1e18;
    std::vector<Posicao> pos;                // uma por residuo com demanda
};

// Melhor posicao do cliente c (janela t) no residuo w. A decisao "rota existente x nova rota"
// e tomada POR residuo, pelo menor custo.
inline Posicao melhorPosicaoResiduo(const Solucao& s, int c, int t, int w, const Grafo& I,
                                    bool existentes, bool novas, double& custo) {
    static const std::vector<int> vazio;
    Posicao best{w, -2, 0};
    double bestC = 1e18;
    const double Uw = I.Uw(w);
    const int dem = I.demanda(c, w);
    if (existentes) {
        for (int v = 0; v < (int)s.R[w].size(); ++v) {
            const auto& r = s.R[w][v];
            if (cargaRota(r, w, I) + dem > I.capacidade) continue;
            for (int p = 0; p <= (int)r.size(); ++p) {
                const double d = Uw * custoInsDist(r, p, c, I);
                if (d >= bestC) continue;
                if (Verificador::simular(r, w, s.b, I, p, c, c, t)) { bestC = d; best = {w, v, p}; }
            }
        }
    }
    if (novas && s.numRotas(w) < I.maxVeiculos) {
        const double d = Uw * 2.0 * I.dist[0][c];
        if (d < bestC && Verificador::simular(vazio, w, s.b, I, 0, c, c, t)) { bestC = d; best = {w, -1, 0}; }
    }
    custo = bestC;
    return best;
}

inline Plano planoInsercao(const Solucao& s, int c, int t, const Grafo& I,
                           bool existentes = true, bool novas = true) {
    Plano pl;
    pl.cliente = c;
    pl.janela = t;
    if (I.lixosDoCliente[c].empty()) return pl;
    double total = 0.0;
    for (int w : I.lixosDoCliente[c]) {
        double custo;
        const Posicao p = melhorPosicaoResiduo(s, c, t, w, I, existentes, novas, custo);
        if (p.v == -2) return pl;
        total += custo;
        pl.pos.push_back(p);
    }
    pl.viavel = true;
    pl.custo = total;
    pl.lucro = I.bid(c, t) - total;
    return pl;
}

inline void aplicarPosicao(Solucao& s, const Posicao& p, int c) {
    if (p.v == -1) s.R[p.w].push_back(std::vector<int>{c});
    else s.R[p.w][p.v].insert(s.R[p.w][p.v].begin() + p.pos, c);
}

inline void aplicarPlano(Solucao& s, const Plano& pl) {
    for (const auto& p : pl.pos) aplicarPosicao(s, p, pl.cliente);
    s.b[pl.cliente] = pl.janela;
}

// Remove o cliente de todas as rotas e o marca como nao atendido (mantem solucao normalizada)
inline void removerCliente(Solucao& s, int c) {
    for (auto& rw : s.R)
        for (auto& r : rw) {
            auto it = std::find(r.begin(), r.end(), c);
            if (it != r.end()) { r.erase(it); break; }
        }
    s.b[c] = -1;
    s.normalizar();
}

// Melhor janela para o cliente (maior lucro marginal); Plano.viavel == false se nenhuma serve.
inline Plano melhorPlanoCliente(const Solucao& s, int c, const Grafo& I) {
    Plano best;
    for (int t = 0; t < 3; ++t) {
        Plano p = planoInsercao(s, c, t, I);
        if (p.viavel && p.lucro > best.lucro) best = p;
    }
    return best;
}

#endif
