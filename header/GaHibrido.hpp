#ifndef GA_HIBRIDO_HPP
#define GA_HIBRIDO_HPP

#include <algorithm>
#include <vector>
#include "Insercao.hpp"
#include "Rvnd.hpp"

// BRKGA: o cromossomo (chaves aleatorias) define a ORDEM de insercao; o decodificador insere cada
// cliente (melhor janela, rota existente x nova por residuo) se o lucro marginal for positivo.
// Nota: o RVND melhora a solucao do individuo mas nao e re-codificado no cromossomo (fitness = lucro
// da solucao melhorada).
class GaHibrido {
    struct Individuo { std::vector<double> cromo; Solucao sol; double fit = 0.0; };
    int tam, elite, mutantes;
    double probX, probRvnd;

    static Solucao decodificar(const std::vector<double>& cromo, const Grafo& I) {
        Solucao s(I.n(), I.numLixos);
        std::vector<std::pair<double, int>> ordem;
        for (int c = 1; c < I.n(); ++c) ordem.push_back({cromo[c - 1], c});
        std::sort(ordem.begin(), ordem.end());
        for (const auto& par : ordem) {
            const int c = par.second;
            if (I.lixosDoCliente[c].empty()) continue;
            const Plano p = melhorPlanoCliente(s, c, I);
            if (p.viavel && p.lucro > EPS) aplicarPlano(s, p);
        }
        s.calcularObjetivo(I);
        return s;
    }

    static std::vector<double> chavesAleatorias(int n, Rng& rng) {
        std::vector<double> v(n);
        for (auto& x : v) x = rng.real();
        return v;
    }

public:
    GaHibrido(int pop = 40, int elite_ = 8, int mut = 6, double pX = 0.7, double pRvnd = 0.3)
        : tam(pop), elite(elite_), mutantes(mut), probX(pX), probRvnd(pRvnd) {}

    Solucao otimizar(int geracoes, const Grafo& I, Rng& rng) {
        const int genes = I.n() - 1;
        const int numElite = std::min(elite, tam - 1);
        Rvnd rvnd(rng, tam);  // populacao inicial = fase de treino do RVND
        std::vector<Individuo> pop(tam);
        for (auto& ind : pop) {
            ind.cromo = chavesAleatorias(genes, rng);
            ind.sol = decodificar(ind.cromo, I);
            rvnd.otimizar(ind.sol, I);
            ind.fit = ind.sol.lucroTotal;
        }
        auto ordenar = [&](std::vector<Individuo>& p) {
            std::sort(p.begin(), p.end(), [](const Individuo& a, const Individuo& b) { return a.fit > b.fit; });
        };
        ordenar(pop);

        for (int g = 0; g < geracoes; ++g) {
            std::vector<Individuo> prox(pop.begin(), pop.begin() + numElite);
            for (int m = 0; m < mutantes; ++m) {
                Individuo ind;
                ind.cromo = chavesAleatorias(genes, rng);
                ind.sol = decodificar(ind.cromo, I);
                if (rng.real() < probRvnd) rvnd.otimizar(ind.sol, I);
                ind.fit = ind.sol.lucroTotal;
                prox.push_back(std::move(ind));
            }
            while ((int)prox.size() < tam) {
                const Individuo& pe = pop[rng.inteiro(numElite)];
                const Individuo& pg = pop[numElite + rng.inteiro(tam - numElite)];
                Individuo f;
                f.cromo.resize(genes);
                for (int k = 0; k < genes; ++k) f.cromo[k] = (rng.real() < probX) ? pe.cromo[k] : pg.cromo[k];
                f.sol = decodificar(f.cromo, I);
                if (rng.real() < probRvnd) rvnd.otimizar(f.sol, I);
                f.fit = f.sol.lucroTotal;
                prox.push_back(std::move(f));
            }
            pop = std::move(prox);
            ordenar(pop);
        }
        return pop[0].sol;
    }
};

#endif
