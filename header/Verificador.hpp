#ifndef VERIFICADOR_HPP
#define VERIFICADOR_HPP

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>
#include "Solucao.hpp"

class Verificador {
public:
    // Simula uma rota (capacidade, janelas com buffer beta e retorno ao deposito dentro do horizonte H).
    // Opcoes sem copiar vetores:
    //   insCli >= 0 : insere virtualmente insCli antes do indice insPos (0..tamanho)
    //   ovCli/ovJan : o cliente ovCli deve ser avaliado com a janela ovJan (em vez de b[ovCli])
    static bool simular(const std::vector<int>& rota, int w, const std::vector<int>& b, const Grafo& I,
                        int insPos = -1, int insCli = -1, int ovCli = -1, int ovJan = -1) {
        const int len = (int)rota.size();
        const int total = len + (insCli >= 0 ? 1 : 0);
        int carga = 0;
        double t = 0.0;
        int prev = 0;
        for (int k = 0; k < total; ++k) {
            int c;
            if (insCli >= 0) c = (k == insPos) ? insCli : rota[k < insPos ? k : k - 1];
            else c = rota[k];
            carga += I.clientes[c].demanda(w);
            if (carga > I.capacidade) return false;
            t += I.dist[prev][c];
            const int j = (c == ovCli) ? ovJan : b[c];
            if (j < 0) return false;
            const double ini = std::max(t, I.clientes[c].ready());
            if (ini > I.clientes[c].due(j) - I.beta + 1e-9) return false;  // restricao (1o)
            t = ini + I.clientes[c].service();
            prev = c;
        }
        return t + I.dist[prev][0] <= I.horizonte + 1e-9;                  // retorno dentro de H
    }

    // Validador INDEPENDENTE (nao usa simular): recalcula tudo do zero a partir de R e b.
    static bool validar(const Solucao& s, const Grafo& I, std::string& msg) {
        const int n = I.n();
        auto falha = [&](const std::string& m) { msg = m; return false; };
        if ((int)s.b.size() != n) return falha("tamanho de b incorreto");
        if ((int)s.R.size() != I.numLixos) return falha("numero de residuos incorreto");
        if (s.b[0] != -1) return falha("deposito com janela");

        std::vector<std::vector<int>> cont(I.numLixos, std::vector<int>(n, 0));
        double custo = 0.0, receita = 0.0;
        for (int w = 0; w < I.numLixos; ++w) {
            if ((int)s.R[w].size() > I.maxVeiculos) return falha("frota excedida no residuo " + std::to_string(w));
            for (const auto& r : s.R[w]) {
                if (r.empty()) return falha("rota vazia");
                int carga = 0, prev = 0;
                double t = 0.0;
                for (int c : r) {
                    if (c <= 0 || c >= n) return falha("id de cliente invalido na rota");
                    cont[w][c]++;
                    carga += I.clientes[c].demanda(w);
                    if (carga > I.capacidade) return falha("capacidade violada (residuo " + std::to_string(w) + ")");
                    t += I.dist[prev][c];
                    custo += I.Uw(w) * I.dist[prev][c];
                    const int j = s.b[c];
                    if (j < 0 || j > 2) return falha("cliente " + std::to_string(c) + " em rota sem janela valida");
                    const double inicio = std::max(t, I.clientes[c].ready());
                    if (inicio > I.clientes[c].due(j) - I.beta + 1e-9)
                        return falha("janela violada: cliente " + std::to_string(c) + " residuo " + std::to_string(w));
                    t = inicio + I.clientes[c].service();
                    prev = c;
                }
                t += I.dist[prev][0];
                custo += I.Uw(w) * I.dist[prev][0];
                if (t > I.horizonte + 1e-9) return falha("retorno ao deposito apos o horizonte");
            }
        }
        for (int c = 1; c < n; ++c) {
            const bool aceito = s.b[c] != -1;
            if (aceito && I.lixosDoCliente[c].empty()) return falha("cliente sem demanda aceito: " + std::to_string(c));
            for (int w = 0; w < I.numLixos; ++w) {
                const int esperado = (aceito && I.clientes[c].demanda(w) > 0) ? 1 : 0;
                if (cont[w][c] != esperado)
                    return falha("cliente " + std::to_string(c) + " aparece " + std::to_string(cont[w][c]) +
                                 "x no residuo " + std::to_string(w) + " (esperado " + std::to_string(esperado) + ")");
            }
            if (aceito) receita += I.bid(c, s.b[c]);
        }
        const double lucro = receita - custo;
        if (std::fabs(lucro - s.lucroTotal) > 1e-6 * std::max(1.0, std::fabs(lucro)))
            return falha("lucro armazenado (" + std::to_string(s.lucroTotal) + ") difere do recalculado (" + std::to_string(lucro) + ")");
        return true;
    }
};

#endif
