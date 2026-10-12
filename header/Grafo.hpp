#ifndef GRAFO_HPP
#define GRAFO_HPP

#include <cmath>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include "Cliente.hpp"


#include <cmath>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include "Cliente.hpp"

// Substitui a antiga classe Grafo: agrupa clientes, distancias e parametros do problema,
// de modo que todas as funcoes recebam um unico "const Grafo&".
struct Grafo {
    std::vector<Cliente> clientes;                // indice 0 = deposito
    std::vector<std::vector<double>> dist;
    int numLixos = 3;
    bool ponderado = true;
    int capacidade = 200;
    int maxVeiculos = 20;                         // frota por tipo de residuo
    double beta = 1.0;                            // buffer da restricao (1o)
    double horizonte = 1e18;                      // H: retorno ao deposito
    std::vector<std::vector<int>> lixosDoCliente; // tipos de residuo com demanda > 0

    int n() const { return (int)clientes.size(); }
    double Uw(int w) const { return ponderado ? (0.5 + 0.5 * (w + 1)) : 1.0; }
    int demanda(int c, int w) const { return clientes[c].demanda(w); }
    double bid(int c, int t) const { return clientes[c].bid(t); }

    void finalizar() {
        const int N = n();
        dist.assign(N, std::vector<double>(N, 0.0));
        for (int i = 0; i < N; ++i)
            for (int j = 0; j < N; ++j)
                dist[i][j] = std::hypot(clientes[i].x() - clientes[j].x(), clientes[i].y() - clientes[j].y());
        lixosDoCliente.assign(N, {});
        for (int c = 1; c < N; ++c)
            for (int w = 0; w < numLixos; ++w)
                if (clientes[c].demanda(w) > 0) lixosDoCliente[c].push_back(w);
        // Horizonte H = fim da janela base do deposito (linha 0 do arquivo)
        horizonte = clientes[0].due(1) > 0 ? clientes[0].due(1) : 1e18;
    }

    // Formato (uma linha por no, deposito = id 0):
    //   3 residuos: id x y d1 d2 d3 ready dueN due dueW service bidN bid bidW  (14 colunas)
    //   2 residuos: id x y d1 d2    ready dueN due dueW service bidN bid bidW  (13 colunas)
    // O numero de residuos e detectado pelo numero de colunas. Carrega ids 0..maxClientes.
    static bool carregar(const std::string& arq, int maxClientes, bool ponderado, Grafo& out, std::string& erro) {
        std::ifstream f(arq);
        if (!f) { erro = "nao foi possivel abrir " + arq; return false; }
        Grafo I;
        I.ponderado = ponderado;
        bool primeiro = true;
        std::string linha;
        while (std::getline(f, linha)) {
            std::istringstream iss(linha);
            std::vector<double> v;
            double x;
            while (iss >> x) v.push_back(x);
            if (v.size() != 13 && v.size() != 14) continue;
            const int nl = (v.size() == 14) ? 3 : 2;
            if (primeiro) { I.numLixos = nl; primeiro = false; }
            else if (nl != I.numLixos) { erro = "numero de colunas inconsistente"; return false; }
            const int id = (int)v[0];
            if (id == 0 && !I.clientes.empty()) continue;  // linha final = deposito de retorno (arquivos oficiais)
            if (id > maxClientes) continue;
            if (id != (int)I.clientes.size()) { erro = "ids fora de ordem (esperado " + std::to_string(I.clientes.size()) + ")"; return false; }
            std::array<int, 3> dem{0, 0, 0};
            for (int w = 0; w < nl; ++w) dem[w] = (int)v[3 + w];
            int k = 3 + nl;
            const double ready = v[k], dN = v[k + 1], d = v[k + 2], dW = v[k + 3], serv = v[k + 4];
            const double bN = v[k + 5], b = v[k + 6], bW = v[k + 7];
            I.clientes.emplace_back(id, v[1], v[2], dem, ready, std::array<double, 3>{dN, d, dW}, serv,
                                    std::array<double, 3>{bN, b, bW});
        }
        if (I.clientes.size() < 2) { erro = "instancia vazia"; return false; }
        I.finalizar();
        out = std::move(I);
        return true;
    }
};

#endif
