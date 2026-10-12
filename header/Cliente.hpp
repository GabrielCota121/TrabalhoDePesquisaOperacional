#ifndef CLIENTE_HPP
#define CLIENTE_HPP

#include <array>

// Janelas: indice 0 = Narrow, 1 = Base, 2 = Wide (mesma convencao do artigo/instancias).
class Cliente {
    int id_;
    double x_, y_;
    std::array<int, 3> dem_;     // demanda por tipo de residuo
    double ready_;               // abertura da janela (unica nos arquivos)
    std::array<double, 3> due_;  // fechamento por janela
    double service_;
    std::array<double, 3> bid_;  // lance por janela

public:
    Cliente(int id, double x, double y, const std::array<int, 3>& dem, double ready,
            const std::array<double, 3>& due, double service, const std::array<double, 3>& bid)
        : id_(id), x_(x), y_(y), dem_(dem), ready_(ready), due_(due), service_(service), bid_(bid) {}

    int id() const { return id_; }
    double x() const { return x_; }
    double y() const { return y_; }
    int demanda(int w) const { return dem_[w]; }
    double ready() const { return ready_; }
    double due(int t) const { return due_[t]; }
    double service() const { return service_; }
    double bid(int t) const { return bid_[t]; }
};

#endif
