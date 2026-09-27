#include "Cliente.hpp"
#include <vector>
#include <cmath> // Garante o escopo de sqrt e pow

#ifndef GRAFO_HPP
#define GRAFO_HPP

class Grafo{
    private:
        std::vector<std::vector<double>> matrizAdjacencia;
        std::vector<Cliente> clientes;
    public:    

        Grafo() = default;

        std::vector<Cliente> getClientes() const {
            return clientes;
        }

        Cliente criaSolucaoCliente(std::vector<Cliente>& clientes, int id, int xCoord, int yCoord, 
                                   int demandWasteType1, int demandWasteType2, int readyTime, 
                                   int dueDateNarrow, int dueDate, int dueDateWide, int service, 
                                   int bidNarrow, int bid, int bidWide, int demandWasteType3 = 0){
            
                                    
            Cliente cliente(id, xCoord, yCoord, demandWasteType1, demandWasteType2, readyTime, 
                            dueDateNarrow, dueDate, dueDateWide, service, bidNarrow, bid, 
                            bidWide, demandWasteType3);
            
            clientes.push_back(cliente);
            return cliente;
        }

        double distancia(int x1, int y1, int x2, int y2){
            return sqrt(pow(x2 - x1, 2) + pow(y2 - y1, 2));
        }

        std::vector<std::vector<double>> getMatrizAdjacencia() const {
            return matrizAdjacencia;
        }

        std::vector<std::vector<double>> criarMatrizAdjacencia(const std::vector<Cliente>& clientes){
            int numClientes = clientes.size();
            matrizAdjacencia = std::vector<std::vector<double>>(numClientes, std::vector<double>(numClientes));
            for(int i = 0; i < numClientes; ++i){
                for(int j = 0; j < numClientes; ++j){
                    if(i == j){
                        matrizAdjacencia[i][j] = 0.0;
                    } else {
                        matrizAdjacencia[i][j] = distancia(clientes[i].getXCoord(), clientes[i].getYCoord(), clientes[j].getXCoord(), clientes[j].getYCoord());
                    }
                }
            }

            return matrizAdjacencia;
        }
};

#endif
