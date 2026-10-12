#ifndef SOLUCAO_HPP
#define SOLUCAO_HPP

#include <algorithm>
#include <vector>
#include "Grafo.hpp"

inline constexpr double EPS = 1e-6;

// Representacao: R[w][v] = sequencia de clientes (SEM zeros de deposito) do veiculo v do residuo w.
// Cada rota comeca e termina implicitamente no deposito. Rotas vazias nao existem (normalizar()).
// b[c] = janela escolhida (0/1/2) ou -1 se o cliente nao foi atendido; vale para TODOS os residuos.
struct Solucao {
    std::vector<std::vector<std::vector<int>>> R;
    std::vector<int> b;
    double lucroTotal = 0.0, receitaBids = 0.0, custoTransporte = 0.0;

    Solucao() = default;
    Solucao(int numClientes, int numLixos) : R(numLixos), b(numClientes, -1) {}

    int numRotas(int w) const { return (int)R[w].size(); }

    int numAceitos() const {
        int k = 0;
        for (size_t i = 1; i < b.size(); ++i) if (b[i] != -1) ++k;
        return k;
    }

    void normalizar() {
        for (auto& rw : R)
            rw.erase(std::remove_if(rw.begin(), rw.end(), [](const std::vector<int>& r) { return r.empty(); }), rw.end());
    }

    bool localizar(int w, int c, int& v, int& pos) const {
        for (v = 0; v < (int)R[w].size(); ++v)
            for (pos = 0; pos < (int)R[w][v].size(); ++pos)
                if (R[w][v][pos] == c) return true;
        return false;
    }

    // Distancia total da rota deposito -> ... -> deposito (sem peso Uw)
    static double distRota(const std::vector<int>& r, const Grafo& G) {
        if (r.empty()) return 0.0;
        double d = G.dist[0][r.front()];
        for (size_t k = 1; k < r.size(); ++k) d += G.dist[r[k - 1]][r[k]];
        return d + G.dist[r.back()][0];
    }

    void calcularObjetivo(const Grafo& G) {
        receitaBids = 0.0;
        custoTransporte = 0.0;
        for (size_t i = 1; i < b.size(); ++i)
            if (b[i] >= 0) receitaBids += G.bid((int)i, b[i]);
        for (size_t w = 0; w < R.size(); ++w) {
            double d = 0.0;
            for (const auto& r : R[w]) d += distRota(r, G);
            custoTransporte += G.Uw((int)w) * d;
        }
        lucroTotal = receitaBids - custoTransporte;
    }
};

#endif
