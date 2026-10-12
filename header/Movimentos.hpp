#ifndef MOVIMENTOS_HPP
#define MOVIMENTOS_HPP

#include <algorithm>
#include <vector>
#include "Insercao.hpp"

// Um movimento = substituicao de rotas (+ mudanca de janela de um cliente + clientes excluidos).
struct Troca { int w; int v; std::vector<int> rota; };  // v == -1: acrescenta nova rota

struct Mov {
    std::vector<Troca> tr;
    int cOv = -1, tOv = -1;       // cliente que muda/recebe janela tOv
    std::vector<int> excluir;     // clientes que deixam a solucao
    double dReceita = 0.0;
};

inline void aplicarMov(Solucao& s, const Mov& m) {
    for (const auto& t : m.tr) {
        if (t.v < 0) s.R[t.w].push_back(t.rota);
        else s.R[t.w][t.v] = t.rota;
    }
    if (m.cOv >= 0) s.b[m.cOv] = m.tOv;
    for (int c : m.excluir) s.b[c] = -1;
    s.normalizar();
}

// 0 = inviavel; 1 = viavel; 2 = rotas do proprio movimento viaveis, mas a nova janela do cliente
// quebra a rota dele em OUTRO residuo (precisa de ajuste/reinsercao nessa rota - ver ajustar()).
inline int verificarMov(const Solucao& s, const Mov& m, const Grafo& I) {
    for (int w = 0; w < I.numLixos; ++w) {
        int delta = 0;
        for (const auto& t : m.tr) if (t.w == w) { if (t.v < 0) ++delta; else if (t.rota.empty()) --delta; }
        if (s.numRotas(w) + delta > I.maxVeiculos) return 0;
    }
    for (const auto& t : m.tr)
        if (!t.rota.empty() && !Verificador::simular(t.rota, t.w, s.b, I, -1, -1, m.cOv, m.tOv)) return 0;

    if (m.cOv >= 0 && s.b[m.cOv] >= 0 && s.b[m.cOv] != m.tOv) {
        for (int w : I.lixosDoCliente[m.cOv]) {
            bool coberto = false;
            for (const auto& t : m.tr)
                if (t.w == w && std::find(t.rota.begin(), t.rota.end(), m.cOv) != t.rota.end()) { coberto = true; break; }
            if (coberto) continue;
            int v, p;
            if (!s.localizar(w, m.cOv, v, p)) return 0;
            if (!Verificador::simular(s.R[w][v], w, s.b, I, -1, -1, m.cOv, m.tOv)) return 2;
        }
    }
    return 1;
}

// Aplica o movimento numa copia e, nos residuos cuja rota ficou inviavel com a nova janela,
// retira o cliente e o reinsere na melhor posicao viavel (rota existente ou nova).
inline bool ajustar(Solucao& out, const Solucao& s, const Mov& m, const Grafo& I) {
    out = s;
    aplicarMov(out, m);
    const int c = m.cOv, t = m.tOv;
    for (int w : I.lixosDoCliente[c]) {
        int v, p;
        if (!out.localizar(w, c, v, p)) return false;
        if (Verificador::simular(out.R[w][v], w, out.b, I)) continue;
        out.R[w][v].erase(out.R[w][v].begin() + p);
        out.normalizar();
        double custo;
        const Posicao pos = melhorPosicaoResiduo(out, c, t, w, I, true, true, custo);
        if (pos.v == -2) return false;
        aplicarPosicao(out, pos, c);
    }
    out.calcularObjetivo(I);
    return true;
}

// Guarda o melhor movimento viavel de um "item" da vizinhanca (primeira melhoria por cliente/rota).
struct Melhor {
    Solucao& s;
    const Grafo& I;
    double bestD = EPS;
    bool has = false, ajustado = false;
    Mov m;
    Melhor(Solucao& sol, const Grafo& inst) : s(sol), I(inst) {}

    void considerar(Mov&& mv, double delta) {
        if (delta <= bestD) return;
        const int st = verificarMov(s, mv, I);
        if (st == 1) { bestD = delta; m = std::move(mv); has = true; }
        else if (st == 2) {
            Solucao out;
            if (ajustar(out, s, mv, I) && out.lucroTotal - s.lucroTotal > bestD) { s = out; ajustado = true; }
        }
    }
};

#endif
