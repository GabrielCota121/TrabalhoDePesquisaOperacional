#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include "Grafo.hpp"
#include "Rng.hpp"
#include "Solucao.hpp"
#include "Verificador.hpp"
#include "Grasp.hpp"
#include "GraspReativo.hpp"
#include "GraspRvnd.hpp"
#include "Rvnd.hpp"
#include "Ils.hpp"
#include "Vns.hpp"
#include "GaHibrido.hpp"
#include "AcoSincronizado.hpp"
#include "BuscaTabu.hpp"
#include "AlnsSincronizado.hpp"

struct Parametros {
    int nBase = 50, multiStart = 1000, treino = 300, ils = 1000, vns = 1000, alns = 1000, tabu = 500;
    int tenure = 10, gaGeracoes = 50, acoCiclos = 30;
};

struct Resultado {
    std::string nome;
    std::vector<double> lucros, tempos;
    int invalidas = 0;
};

using Clock = std::chrono::steady_clock;
static double seg(Clock::time_point a) { return std::chrono::duration<double>(Clock::now() - a).count(); }

static Resultado& obter(std::vector<Resultado>& rs, const std::string& nome) {
    for (auto& r : rs) if (r.nome == nome) return r;
    rs.push_back({nome, {}, {}, 0});
    return rs.back();
}

// Toda solucao passa pelo validador independente antes de entrar nas estatisticas.
static void registrar(std::vector<Resultado>& rs, const std::string& nome, const Solucao& s, double t, const Grafo& I) {
    Resultado& r = obter(rs, nome);
    std::string msg;
    if (!Verificador::validar(s, I, msg)) {
        r.invalidas++;
        std::fprintf(stderr, "[INVALIDA] %s: %s\n", nome.c_str(), msg.c_str());
        return;
    }
    r.lucros.push_back(s.lucroTotal);
    r.tempos.push_back(t);
}

int main(int argc, char** argv) {
    std::string arquivo = "Instances/f_data/f_rc103.txt";
    int nClientes = 15, execucoes = 5;
    bool ponderado = false, rapido = false;   // padrao = nao ponderado (tabela principal do artigo, familia f2)
    unsigned long semente = 2026;
    int pos = 0;
    for (int i = 1; i < argc; ++i) {
        if (!std::strcmp(argv[i], "--rapido")) rapido = true;
        else if (!std::strcmp(argv[i], "--nao-ponderado")) ponderado = false;
        else if (!std::strcmp(argv[i], "--ponderado")) ponderado = true;
        else if (!std::strcmp(argv[i], "--semente") && i + 1 < argc) semente = std::stoul(argv[++i]);
        else if (!std::strcmp(argv[i], "--execucoes") && i + 1 < argc) execucoes = std::stoi(argv[++i]);
        else if (!std::strcmp(argv[i], "--clientes") && i + 1 < argc) nClientes = std::stoi(argv[++i]);
        else if (pos == 0) { arquivo = argv[i]; ++pos; }
        else { std::fprintf(stderr, "argumento desconhecido: %s\n", argv[i]); return 1; }
    }
    Parametros P;
    if (rapido) P = {10, 40, 12, 30, 20, 100, 30, 10, 5, 4};

    Grafo I;
    std::string erro;
    if (!Grafo::carregar(arquivo, nClientes, ponderado, I, erro)) {
        std::cerr << "Erro: " << erro << "\n";
        return 1;
    }
    std::printf("Instancia %s: %d clientes, %d residuos, %s, H=%.0f\n", arquivo.c_str(), I.n() - 1, I.numLixos,
                ponderado ? "ponderado" : "nao ponderado", I.horizonte);
    std::printf("%d execucoes (sementes %lu..%lu)%s\n\n", execucoes, semente, semente + execucoes - 1, rapido ? " [modo rapido]" : "");

    std::vector<Resultado> rs;
    for (int e = 0; e < execucoes; ++e) {
        Rng rng(semente + e);

        // Solucao-base: melhor de nBase construcoes (mesmo esforco para GRASP estatico e reativo)
        for (int modo = 0; modo < 2; ++modo) {
            const std::string pre = modo == 0 ? "Estatico" : "Reativo";
            auto t0 = Clock::now();
            Solucao base;
            bool tem = false;
            GraspReativo gr({1, 2, 3, 5, 8}, 20);
            Grasp gs(3);
            for (int i = 0; i < P.nBase; ++i) {
                Solucao s;
                int idx = 0;
                if (modo == 0) s = gs.construirSolucaoInicial(I, rng);
                else { idx = gr.escolherK(rng); s = gr.construirSolucaoInicial(I, idx, rng); gr.registrarResultado(idx, s.lucroTotal); }
                if (!tem || s.lucroTotal > base.lucroTotal) { base = s; tem = true; }
            }
            registrar(rs, pre + ": GRASP (base)", base, seg(t0), I);

            Solucao s = base;
            t0 = Clock::now(); { Rvnd rv(rng); rv.otimizar(s, I); }
            registrar(rs, pre + ": base + RVND", s, seg(t0), I);

            t0 = Clock::now();
            s = Ils::otimizar(base, P.ils, I, rng);
            registrar(rs, pre + ": base + ILS", s, seg(t0), I);

            t0 = Clock::now();
            s = Vns::otimizar(base, P.vns, I, rng);
            registrar(rs, pre + ": base + VNS", s, seg(t0), I);

            t0 = Clock::now();
            { AlnsSincronizado alns(rng); s = alns.otimizar(base, P.alns, I); }
            registrar(rs, pre + ": base + ALNS", s, seg(t0), I);

            t0 = Clock::now();
            s = BuscaTabu::otimizar(base, P.tabu, P.tenure, I, rng);
            registrar(rs, pre + ": base + Tabu", s, seg(t0), I);

            // Metodo do artigo: multi-start GRASP -> RVND com aprendizado online
            t0 = Clock::now();
            if (modo == 0) s = graspRvnd(I, P.multiStart, P.treino, rng, 3);
            else { GraspReativo g2({1, 2, 3, 5, 8}, 20); s = graspRvnd(I, P.multiStart, P.treino, rng, 3, &g2); }
            registrar(rs, pre + ": GRASP-RVND multi-start", s, seg(t0), I);
        }

        // Metodos que nao dependem da solucao-base (executados uma unica vez)
        auto t0 = Clock::now();
        { GaHibrido ga(40, 8, 6, 0.7, 0.3); Solucao s = ga.otimizar(P.gaGeracoes, I, rng); registrar(rs, "GA hibrido (BRKGA)", s, seg(t0), I); }
        t0 = Clock::now();
        { AcoSincronizado aco(10, 1.0, 2.0, 0.1, 1.0); Solucao s = aco.otimizar(P.acoCiclos, I, rng); registrar(rs, "ACO memetico", s, seg(t0), I); }
        std::fprintf(stderr, "execucao %d/%d concluida\n", e + 1, execucoes);
    }

    std::printf("%-34s %12s %12s %10s %10s %6s\n", "Metodo", "Melhor", "Media", "Desvio", "Tempo(s)", "Invalid");
    std::printf("%s\n", std::string(90, '-').c_str());
    for (const auto& r : rs) {
        if (r.lucros.empty()) { std::printf("%-34s %12s %12s %10s %10s %6d\n", r.nome.c_str(), "-", "-", "-", "-", r.invalidas); continue; }
        double mx = r.lucros[0], m = 0, t = 0;
        for (double x : r.lucros) { mx = std::max(mx, x); m += x; }
        m /= r.lucros.size();
        double v = 0;
        for (double x : r.lucros) v += (x - m) * (x - m);
        v = std::sqrt(v / r.lucros.size());
        for (double x : r.tempos) t += x;
        t /= r.tempos.size();
        std::printf("%-34s %12.2f %12.2f %10.2f %10.2f %6d\n", r.nome.c_str(), mx, m, v, t, r.invalidas);
    }
    return 0;
}
