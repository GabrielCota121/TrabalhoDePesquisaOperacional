#ifndef RVND_HPP
#define RVND_HPP

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include "Movimentos.hpp"
#include "Rng.hpp"

// RVND com selecao adaptativa de operadores (Algoritmo 2 do artigo), 10 vizinhancas:
//  0 intra-rota reinsercao   1 inter-rota reinsercao   2 reinsercao em nova rota
//  3 remocao de cliente      4 inclusao em nova rota   5 inclusao em rota existente
//  6 intra-rota swap         7 inter-rota swap         8 swap com cliente excluido   9 2-opt
// As matrizes MU/MI sao membros do objeto (nao static): cada execucao aprende do zero.
class Rvnd {
public:
    static constexpr int NOP = 10;

    // treinoChamadas: nas primeiras chamadas a escolha e uniforme (fase de treino do artigo).
    explicit Rvnd(Rng& rng, int treinoChamadas = 0)
        : rng_(rng), treino_(treinoChamadas), MU(NOP + 1, std::vector<double>(NOP, 1.0)),
          MI(NOP + 1, std::vector<double>(NOP, 1.0)) {}

    void otimizar(Solucao& s, const Grafo& I) {
        const bool emTreino = chamadas_++ < treino_;
        s.normalizar();
        s.calcularObjetivo(I);
        std::vector<int> disp(NOP, 1);
        int estado = 0;  // N0
        while (std::find(disp.begin(), disp.end(), 1) != disp.end()) {
            int op = -1;
            if (emTreino) {
                std::vector<int> idx;
                for (int j = 0; j < NOP; ++j) if (disp[j]) idx.push_back(j);
                op = idx[rng_.inteiro((int)idx.size())];
            } else {
                double soma = 0.0;
                std::vector<double> rho(NOP, 0.0);
                for (int j = 0; j < NOP; ++j) if (disp[j]) { rho[j] = MI[estado][j] / MU[estado][j]; soma += rho[j]; }
                double ponteiro = rng_.real() * soma, acum = 0.0;
                for (int j = 0; j < NOP; ++j) {
                    if (!disp[j]) continue;
                    acum += rho[j];
                    op = j;
                    if (ponteiro <= acum) break;
                }
            }
            MU[estado][op] += 1.0;
            if (buscaLocal(op, s, I)) {
                MI[estado][op] += 1.0;
                estado = op + 1;
                std::fill(disp.begin(), disp.end(), 1);  // reativa todos...
                disp[op] = 0;                            // ...exceto o que acabou de melhorar (linha 11)
            } else {
                disp[op] = 0;
            }
        }
    }

private:
    Rng& rng_;
    int treino_, chamadas_ = 0;
    std::vector<std::vector<double>> MU, MI;

    // Aplica o operador ate o otimo local da propria vizinhanca
    static bool buscaLocal(int op, Solucao& s, const Grafo& I) {
        bool alguma = false;
        for (int it = 0; it < 100000; ++it) {
            const double antes = s.lucroTotal;
            if (!passo(op, s, I)) break;
#ifdef VERIFICAR
            std::string msg;
            if (!Verificador::validar(s, I, msg) || s.lucroTotal <= antes + EPS * 0.5) {
                std::fprintf(stderr, "[VERIFICAR] operador %d gerou solucao ruim: %s (antes %.4f depois %.4f)\n", op,
                             msg.c_str(), antes, s.lucroTotal);
                std::abort();
            }
#else
            (void)antes;
#endif
            alguma = true;
        }
        return alguma;
    }

    static bool passo(int op, Solucao& s, const Grafo& I) {
        switch (op) {
            case 0: return reinsercao(s, I, 0);
            case 1: return reinsercao(s, I, 1);
            case 2: return reinsercao(s, I, 2);
            case 3: return remocao(s, I);
            case 4: return inclusao(s, I, false, true);
            case 5: return inclusao(s, I, true, false);
            case 6: return trocaIntra(s, I);
            case 7: return trocaInter(s, I);
            case 8: return trocaExcluido(s, I);
            case 9: return doisOpt(s, I);
        }
        return false;
    }

    static bool aplicarMelhor(Solucao& s, const Grafo& I, Melhor& mb) {
        if (!mb.has) return false;
        const double antes = s.lucroTotal;
        aplicarMov(s, mb.m);
        s.calcularObjetivo(I);
#ifdef VERIFICAR
        if (std::fabs(s.lucroTotal - (antes + mb.bestD)) > 1e-6) {
            std::fprintf(stderr, "[VERIFICAR] delta inconsistente: esperado %.6f obtido %.6f\n", antes + mb.bestD, s.lucroTotal);
            std::abort();
        }
#else
        (void)antes;
#endif
        return true;
    }

    // modo 0 = intra-rota, 1 = inter-rota, 2 = nova rota. Janela do cliente pode mudar (3 opcoes).
    static bool reinsercao(Solucao& s, const Grafo& I, int modo) {
        for (int w = 0; w < I.numLixos; ++w) {
            const double Uw = I.Uw(w);
            for (int v = 0; v < (int)s.R[w].size(); ++v) {
                const std::vector<int> rota = s.R[w][v];
                for (int i = 0; i < (int)rota.size(); ++i) {
                    const int c = rota[i], bc = s.b[c];
                    std::vector<int> base = rota;
                    base.erase(base.begin() + i);
                    const double ganho = custoRemDist(rota, i, I);
                    Melhor mb(s, I);
                    for (int t = 0; t < 3; ++t) {
                        const double dRec = (t == bc) ? 0.0 : I.bid(c, t) - I.bid(c, bc);
                        auto fecha = [&](Mov& m) { if (t != bc) { m.cOv = c; m.tOv = t; } m.dReceita = dRec; };
                        if (modo == 0) {
                            for (int p = 0; p <= (int)base.size(); ++p) {
                                if (p == i && t == bc) continue;
                                const double delta = dRec - Uw * (custoInsDist(base, p, c, I) - ganho);
                                if (delta <= mb.bestD) continue;
                                Mov m;
                                std::vector<int> r = base;
                                r.insert(r.begin() + p, c);
                                m.tr.push_back(Troca{w, v, std::move(r)});
                                fecha(m);
                                mb.considerar(std::move(m), delta);
                                if (mb.ajustado) return true;
                            }
                        } else if (modo == 1) {
                            for (int v2 = 0; v2 < (int)s.R[w].size(); ++v2) {
                                if (v2 == v) continue;
                                const std::vector<int> r2 = s.R[w][v2];
                                if (cargaRota(r2, w, I) + I.demanda(c, w) > I.capacidade) continue;
                                for (int p = 0; p <= (int)r2.size(); ++p) {
                                    const double delta = dRec - Uw * (custoInsDist(r2, p, c, I) - ganho);
                                    if (delta <= mb.bestD) continue;
                                    Mov m;
                                    std::vector<int> r = r2;
                                    r.insert(r.begin() + p, c);
                                    m.tr.push_back(Troca{w, v, base});
                                    m.tr.push_back(Troca{w, v2, std::move(r)});
                                    fecha(m);
                                    mb.considerar(std::move(m), delta);
                                    if (mb.ajustado) return true;
                                }
                            }
                        } else {
                            if (rota.size() < 2) continue;
                            const double delta = dRec - Uw * (2.0 * I.dist[0][c] - ganho);
                            if (delta <= mb.bestD) continue;
                            Mov m;
                            m.tr.push_back(Troca{w, v, base});
                            m.tr.push_back(Troca{w, -1, std::vector<int>{c}});
                            fecha(m);
                            mb.considerar(std::move(m), delta);
                            if (mb.ajustado) return true;
                        }
                    }
                    if (aplicarMelhor(s, I, mb)) return true;
                }
            }
        }
        return false;
    }

    // Remove o cliente que mais "da prejuizo" (economia de transporte > lance perdido)
    static bool remocao(Solucao& s, const Grafo& I) {
        int melhorC = -1;
        double melhorD = EPS;
        for (int c = 1; c < I.n(); ++c) {
            if (s.b[c] < 0) continue;
            double sav = 0.0;
            for (int w : I.lixosDoCliente[c]) {
                int v = 0, i = 0;
                s.localizar(w, c, v, i);
                sav += I.Uw(w) * custoRemDist(s.R[w][v], i, I);
            }
            const double delta = sav - I.bid(c, s.b[c]);
            if (delta > melhorD) { melhorD = delta; melhorC = c; }
        }
        if (melhorC < 0) return false;
        removerCliente(s, melhorC);
        s.calcularObjetivo(I);
        return true;
    }

    // Inclusao de cliente excluido: em rotas novas (novas=true) ou em rotas existentes (existentes=true)
    static bool inclusao(Solucao& s, const Grafo& I, bool existentes, bool novas) {
        Plano best;
        double melhorL = EPS;
        bool achou = false;
        for (int u = 1; u < I.n(); ++u) {
            if (s.b[u] >= 0 || I.lixosDoCliente[u].empty()) continue;
            for (int t = 0; t < 3; ++t) {
                Plano pl = planoInsercao(s, u, t, I, existentes, novas);
                if (pl.viavel && pl.lucro > melhorL) { melhorL = pl.lucro; best = pl; achou = true; }
            }
        }
        if (!achou) return false;
        aplicarPlano(s, best);
        s.calcularObjetivo(I);
        return true;
    }

    static bool trocaIntra(Solucao& s, const Grafo& I) {
        for (int w = 0; w < I.numLixos; ++w) {
            const double Uw = I.Uw(w);
            for (int v = 0; v < (int)s.R[w].size(); ++v) {
                std::vector<int> r = s.R[w][v];
                if (r.size() < 2) continue;
                const double velho = Solucao::distRota(r, I);
                Melhor mb(s, I);
                for (int i = 0; i + 1 < (int)r.size(); ++i)
                    for (int j = i + 1; j < (int)r.size(); ++j) {
                        std::swap(r[i], r[j]);
                        const double d0 = Uw * (velho - Solucao::distRota(r, I));
                        // Sem mudar janela e, como no artigo, mudando a janela de um dos dois clientes trocados
                        for (int op = 0; op < 7; ++op) {
                            const int x = op == 0 ? -1 : (op <= 3 ? r[i] : r[j]);
                            const int t = op == 0 ? -1 : (op - 1) % 3;
                            if (x >= 0 && t == s.b[x]) continue;
                            const double delta = d0 + (x >= 0 ? I.bid(x, t) - I.bid(x, s.b[x]) : 0.0);
                            if (delta <= mb.bestD) continue;
                            Mov m;
                            m.tr.push_back(Troca{w, v, r});
                            if (x >= 0) { m.cOv = x; m.tOv = t; m.dReceita = delta - d0; }
                            mb.considerar(std::move(m), delta);
                            if (mb.ajustado) return true;
                        }
                        std::swap(r[i], r[j]);
                    }
                if (aplicarMelhor(s, I, mb)) return true;
            }
        }
        return false;
    }

    static bool trocaInter(Solucao& s, const Grafo& I) {
        for (int w = 0; w < I.numLixos; ++w) {
            const double Uw = I.Uw(w);
            for (int v1 = 0; v1 < (int)s.R[w].size(); ++v1)
                for (int v2 = v1 + 1; v2 < (int)s.R[w].size(); ++v2) {
                    std::vector<int> r1 = s.R[w][v1], r2 = s.R[w][v2];
                    const int q1 = cargaRota(r1, w, I), q2 = cargaRota(r2, w, I);
                    const double velho = Solucao::distRota(r1, I) + Solucao::distRota(r2, I);
                    Melhor mb(s, I);
                    for (int i = 0; i < (int)r1.size(); ++i)
                        for (int j = 0; j < (int)r2.size(); ++j) {
                            const int d1 = I.demanda(r1[i], w), d2 = I.demanda(r2[j], w);
                            if (q1 - d1 + d2 > I.capacidade || q2 - d2 + d1 > I.capacidade) continue;
                            std::swap(r1[i], r2[j]);
                            const double d0 = Uw * (velho - Solucao::distRota(r1, I) - Solucao::distRota(r2, I));
                            for (int op = 0; op < 7; ++op) {
                                const int x = op == 0 ? -1 : (op <= 3 ? r1[i] : r2[j]);
                                const int t = op == 0 ? -1 : (op - 1) % 3;
                                if (x >= 0 && t == s.b[x]) continue;
                                const double delta = d0 + (x >= 0 ? I.bid(x, t) - I.bid(x, s.b[x]) : 0.0);
                                if (delta <= mb.bestD) continue;
                                Mov m;
                                m.tr.push_back(Troca{w, v1, r1});
                                m.tr.push_back(Troca{w, v2, r2});
                                if (x >= 0) { m.cOv = x; m.tOv = t; m.dReceita = delta - d0; }
                                mb.considerar(std::move(m), delta);
                                if (mb.ajustado) return true;
                            }
                            std::swap(r1[i], r2[j]);
                        }
                    if (aplicarMelhor(s, I, mb)) return true;
                }
        }
        return false;
    }

    // Troca um cliente atendido c por um excluido u: remove c e reinsere u (melhor janela/posicao)
    static bool trocaExcluido(Solucao& s, const Grafo& I) {
        std::vector<int> excl;
        for (int u = 1; u < I.n(); ++u)
            if (s.b[u] < 0 && !I.lixosDoCliente[u].empty()) excl.push_back(u);
        if (excl.empty()) return false;
        for (int c = 1; c < I.n(); ++c) {
            if (s.b[c] < 0) continue;
            double sav = 0.0;
            for (int w : I.lixosDoCliente[c]) {
                int v = 0, i = 0;
                s.localizar(w, c, v, i);
                sav += I.Uw(w) * custoRemDist(s.R[w][v], i, I);
            }
            const double deltaRem = sav - I.bid(c, s.b[c]);
            Solucao tmp;
            bool pronto = false;
            for (int u : excl) {
                const double maxBid = std::max({I.bid(u, 0), I.bid(u, 1), I.bid(u, 2)});
                if (deltaRem + maxBid <= EPS) continue;  // limite otimista (custo de insercao >= 0)
                if (!pronto) { tmp = s; removerCliente(tmp, c); tmp.calcularObjetivo(I); pronto = true; }
                Plano best;
                for (int t = 0; t < 3; ++t) {
                    Plano pl = planoInsercao(tmp, u, t, I);
                    if (pl.viavel && pl.lucro > best.lucro) best = pl;
                }
                if (best.viavel && tmp.lucroTotal + best.lucro > s.lucroTotal + EPS) {
                    aplicarPlano(tmp, best);
                    tmp.calcularObjetivo(I);
                    s = tmp;
                    return true;
                }
            }
        }
        return false;
    }

    static bool doisOpt(Solucao& s, const Grafo& I) {
        for (int w = 0; w < I.numLixos; ++w) {
            const double Uw = I.Uw(w);
            for (int v = 0; v < (int)s.R[w].size(); ++v) {
                const std::vector<int> r = s.R[w][v];
                if (r.size() < 2) continue;
                Melhor mb(s, I);
                for (int i = 0; i + 1 < (int)r.size(); ++i)
                    for (int j = i + 1; j < (int)r.size(); ++j) {
                        const int prev = i == 0 ? 0 : r[i - 1];
                        const int next = j + 1 == (int)r.size() ? 0 : r[j + 1];
                        const double velho = I.dist[prev][r[i]] + I.dist[r[j]][next];
                        const double novo = I.dist[prev][r[j]] + I.dist[r[i]][next];
                        const double delta = Uw * (velho - novo);
                        if (delta <= mb.bestD) continue;
                        Mov m;
                        std::vector<int> rr = r;
                        std::reverse(rr.begin() + i, rr.begin() + j + 1);
                        m.tr.push_back(Troca{w, v, std::move(rr)});
                        mb.considerar(std::move(m), delta);
                    }
                if (aplicarMelhor(s, I, mb)) return true;
            }
        }
        return false;
    }
};

#endif
