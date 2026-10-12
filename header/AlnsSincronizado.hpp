#ifndef ALNS_SINCRONIZADO_HPP
#define ALNS_SINCRONIZADO_HPP

#include <algorithm>
#include <cmath>
#include <vector>
#include "Insercao.hpp"
#include "Rng.hpp"

// ALNS: 3 destruidores (aleatorio, pior contribuicao, relacionado por distancia) x 2 reparadores
// (guloso, regret-2), pesos adaptativos por segmentos e aceitacao por recozimento simulado.
class AlnsSincronizado {
    Rng& rng_;
    std::vector<double> pesoD{1, 1, 1}, pesoR{1, 1};
    std::vector<double> scoreD = std::vector<double>(3, 0.0), scoreR = std::vector<double>(2, 0.0);
    std::vector<int> usoD = std::vector<int>(3, 0), usoR = std::vector<int>(2, 0);
    static constexpr int SEGMENTO = 50;
    static constexpr double REACAO = 0.1, SIG1 = 33.0, SIG2 = 9.0, SIG3 = 3.0;

    int roleta(const std::vector<double>& w) {
        double soma = 0.0;
        for (double x : w) soma += x;
        double p = rng_.real() * soma, acum = 0.0;
        for (size_t i = 0; i < w.size(); ++i) { acum += w[i]; if (p <= acum) return (int)i; }
        return (int)w.size() - 1;
    }

    static std::vector<int> aceitos(const Solucao& s, const Grafo& I) {
        std::vector<int> a;
        for (int c = 1; c < I.n(); ++c) if (s.b[c] >= 0) a.push_back(c);
        return a;
    }

    void destruir(Solucao& s, int op, int q, const Grafo& I) {
        std::vector<int> a = aceitos(s, I), rem;
        q = std::min(q, (int)a.size());
        if (q <= 0) return;
        if (op == 0) {
            rng_.embaralhar(a.begin(), a.end());
            rem.assign(a.begin(), a.begin() + q);
        } else if (op == 1) {  // pior contribuicao: lance - economia de transporte ao remover
            std::vector<std::pair<double, int>> contrib;
            for (int c : a) {
                double sav = 0.0;
                for (int w : I.lixosDoCliente[c]) {
                    int v = 0, i = 0;
                    s.localizar(w, c, v, i);
                    sav += I.Uw(w) * custoRemDist(s.R[w][v], i, I);
                }
                contrib.push_back({I.bid(c, s.b[c]) - sav, c});
            }
            std::sort(contrib.begin(), contrib.end());
            for (int k = 0; k < q; ++k) {  // aleatorizacao y^3 (Ropke & Pisinger)
                const double y = rng_.real();
                const int idx = std::min((int)(std::pow(y, 3.0) * contrib.size()), (int)contrib.size() - 1);
                rem.push_back(contrib[idx].second);
                contrib.erase(contrib.begin() + idx);
            }
        } else {  // relacionado: os q clientes mais proximos de uma semente
            const int semente = a[rng_.inteiro((int)a.size())];
            std::sort(a.begin(), a.end(), [&](int x, int y) { return I.dist[semente][x] < I.dist[semente][y]; });
            rem.assign(a.begin(), a.begin() + q);
        }
        for (int c : rem) removerCliente(s, c);
    }

    // Reconstrucao sobre o pool = todos os clientes elegiveis hoje excluidos.
    // modo 0 = guloso (maior lucro marginal), modo 1 = regret-2 (maior perda por nao inserir agora).
    void reparar(Solucao& s, int modo, const Grafo& I) {
        std::vector<int> pool;
        for (int c = 1; c < I.n(); ++c) if (s.b[c] < 0 && !I.lixosDoCliente[c].empty()) pool.push_back(c);
        rng_.embaralhar(pool.begin(), pool.end());
        while (!pool.empty()) {
            int idxMelhor = -1;
            Plano melhorPl;
            double melhorChave = -1e18;
            std::vector<int> manter;
            for (int c : pool) {
                Plano b1, b2;  // melhor e segunda melhor janela (lucro > 0)
                for (int t = 0; t < 3; ++t) {
                    Plano p = planoInsercao(s, c, t, I);
                    if (!p.viavel || p.lucro <= EPS) continue;
                    if (p.lucro > b1.lucro) { b2 = b1; b1 = p; }
                    else if (p.lucro > b2.lucro) b2 = p;
                }
                if (!b1.viavel) continue;  // sem opcao lucrativa agora: sai do pool
                manter.push_back(c);
                const double regret = b1.lucro - (b2.viavel ? b2.lucro : 0.0);
                const double chave = (modo == 0) ? b1.lucro : regret + 1e-6 * b1.lucro;
                if (chave > melhorChave) { melhorChave = chave; melhorPl = b1; idxMelhor = c; }
            }
            if (idxMelhor < 0) break;
            aplicarPlano(s, melhorPl);
            pool.clear();
            for (int c : manter) if (c != idxMelhor) pool.push_back(c);
        }
    }

    void fimSegmento() {
        for (size_t i = 0; i < pesoD.size(); ++i) {
            if (usoD[i] > 0) pesoD[i] = (1 - REACAO) * pesoD[i] + REACAO * scoreD[i] / usoD[i];
            pesoD[i] = std::max(pesoD[i], 0.05);
            scoreD[i] = 0; usoD[i] = 0;
        }
        for (size_t i = 0; i < pesoR.size(); ++i) {
            if (usoR[i] > 0) pesoR[i] = (1 - REACAO) * pesoR[i] + REACAO * scoreR[i] / usoR[i];
            pesoR[i] = std::max(pesoR[i], 0.05);
            scoreR[i] = 0; usoR[i] = 0;
        }
    }

public:
    explicit AlnsSincronizado(Rng& rng) : rng_(rng) {}

    Solucao otimizar(const Solucao& inicial, int iteracoes, const Grafo& I) {
        Solucao atual = inicial, melhor = inicial;
        atual.calcularObjetivo(I);
        melhor = atual;

        // Temperatura: aceita com prob. 50% uma solucao 5% pior no inicio; resfria geometricamente
        const double T0 = std::max(1.0, 0.05 * std::fabs(atual.lucroTotal) / std::log(2.0));
        const double alfa = std::pow(0.01, 1.0 / std::max(1, iteracoes));
        double T = T0;
        const int qMin = std::max(2, (int)std::ceil(0.1 * I.n()));
        const int qMax = std::max(qMin, (int)std::ceil(0.4 * I.n()));

        for (int it = 0; it < iteracoes; ++it) {
            const int d = roleta(pesoD), r = roleta(pesoR);
            Solucao cand = atual;
            destruir(cand, d, qMin + rng_.inteiro(qMax - qMin + 1), I);
            reparar(cand, r, I);
            cand.calcularObjetivo(I);

            double score = 0.0;
            const double delta = cand.lucroTotal - atual.lucroTotal;
            if (cand.lucroTotal > melhor.lucroTotal + EPS) { melhor = cand; atual = cand; score = SIG1; }
            else if (delta > EPS) { atual = cand; score = SIG2; }
            else if (rng_.real() < std::exp(delta / T)) { atual = cand; score = SIG3; }

            scoreD[d] += score; scoreR[r] += score;
            usoD[d]++; usoR[r]++;
            if ((it + 1) % SEGMENTO == 0) fimSegmento();
            T *= alfa;
        }
        return melhor;
    }
};

#endif
