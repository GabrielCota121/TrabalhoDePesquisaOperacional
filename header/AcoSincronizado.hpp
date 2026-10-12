#ifndef ACO_SINCRONIZADO_HPP
#define ACO_SINCRONIZADO_HPP

#include <algorithm>
#include <cmath>
#include <vector>
#include "Insercao.hpp"
#include "Rvnd.hpp"

// ACO memetico. Feromonio tau[w][i][j] nas arestas. As duas componentes da probabilidade ficam em
// escalas comparaveis: heuristica = lucro marginal / maior lucro marginal do passo (em (0,1]) e
// feromonio limitado a [TAU_MIN, TAU_MAX]; o deposito e normalizado pelo melhor lucro global.
class AcoSincronizado {
    int formigas;
    double alpha, beta, rho, q;
    static constexpr double TAU_MIN = 0.05, TAU_MAX = 5.0;
    std::vector<std::vector<std::vector<double>>> tau;

    struct Opcao { Plano pl; double peso; };

    static int predecessor(const Solucao& s, const Posicao& p) {
        if (p.v < 0 || p.pos == 0) return 0;
        return s.R[p.w][p.v][p.pos - 1];
    }

    Solucao construir(const Grafo& I, Rng& rng) {
        Solucao s(I.n(), I.numLixos);
        while (true) {
            std::vector<Opcao> ops;
            double maxL = 0.0;
            for (int c = 1; c < I.n(); ++c) {
                if (s.b[c] != -1 || I.lixosDoCliente[c].empty()) continue;
                for (int t = 0; t < 3; ++t) {
                    Plano p = planoInsercao(s, c, t, I);
                    if (!p.viavel || p.lucro <= EPS) continue;
                    maxL = std::max(maxL, p.lucro);
                    ops.push_back({std::move(p), 0.0});
                }
            }
            if (ops.empty()) break;
            double soma = 0.0;
            for (auto& o : ops) {
                double tm = 0.0;
                for (const auto& p : o.pl.pos) tm += tau[p.w][predecessor(s, p)][o.pl.cliente];
                tm /= o.pl.pos.size();
                o.peso = std::max(1e-12, std::pow(tm, alpha) * std::pow(o.pl.lucro / maxL, beta));
                soma += o.peso;
            }
            double ponteiro = rng.real() * soma, acum = 0.0;
            size_t esc = ops.size() - 1;
            for (size_t k = 0; k < ops.size(); ++k) { acum += ops[k].peso; if (ponteiro <= acum) { esc = k; break; } }
            aplicarPlano(s, ops[esc].pl);
        }
        s.calcularObjetivo(I);
        return s;
    }

    void evaporar() {
        for (auto& m : tau) for (auto& l : m) for (auto& x : l) x = std::max(TAU_MIN, x * (1.0 - rho));
    }

    void depositar(const Solucao& s, double lucroRef) {
        if (s.lucroTotal <= 0 || lucroRef <= 0) return;
        const double delta = q * s.lucroTotal / lucroRef;
        for (size_t w = 0; w < s.R.size(); ++w)
            for (const auto& r : s.R[w]) {
                int prev = 0;
                for (int c : r) { tau[w][prev][c] = std::min(TAU_MAX, tau[w][prev][c] + delta); prev = c; }
                tau[w][prev][0] = std::min(TAU_MAX, tau[w][prev][0] + delta);
            }
    }

public:
    AcoSincronizado(int ants = 10, double a = 1.0, double b = 2.0, double evap = 0.1, double Q = 1.0)
        : formigas(ants), alpha(a), beta(b), rho(evap), q(Q) {}

    Solucao otimizar(int ciclos, const Grafo& I, Rng& rng) {
        tau.assign(I.numLixos, std::vector<std::vector<double>>(I.n(), std::vector<double>(I.n(), 1.0)));
        Rvnd rvnd(rng);
        Solucao melhorGlobal;
        bool tem = false;
        for (int c = 0; c < ciclos; ++c) {
            Solucao melhorCiclo;
            bool temCiclo = false;
            for (int f = 0; f < formigas; ++f) {
                Solucao s = construir(I, rng);
                rvnd.otimizar(s, I);
                if (!temCiclo || s.lucroTotal > melhorCiclo.lucroTotal) { melhorCiclo = s; temCiclo = true; }
            }
            if (!tem || melhorCiclo.lucroTotal > melhorGlobal.lucroTotal) { melhorGlobal = melhorCiclo; tem = true; }
            evaporar();
            depositar(melhorCiclo, melhorGlobal.lucroTotal);
        }
        return melhorGlobal;
    }
};

#endif
