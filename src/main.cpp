#include <iostream>
#include "Grafo.hpp"
#include "Cliente.hpp"
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <cctype> // Necessário para std::isalpha

int main(){
    // Exemplo usando o caminho relativo correto baseado nas suas pastas do VS Code
    std::string filename = "Instances/f_data/f_c101.txt";

    // O programa descobre se tem 3 lixos verificando se "f2" NÃO está no nome/caminho do arquivo
    bool tem3Demandas = (filename.find("f2") == std::string::npos);

    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Erro ao abrir o arquivo: " << filename << std::endl;
        return 1;
    }

    std::vector<Cliente> clientes;
    Grafo grafo;
    std::string line;

    while (std::getline(file, line)) {
        std::istringstream iss(line);
        
        int id, xCoord, yCoord, demandWasteType1, demandWasteType2;
        
        // Tenta ler os 5 primeiros inteiros. 
        if (!(iss >> id >> xCoord >> yCoord >> demandWasteType1 >> demandWasteType2)) {
            continue; 
        }

        int demandWasteType3 = 0; // Valor padrão inicial

        // Se a instância possuir 3 lixos, lê a coluna d3
        if (tem3Demandas) {
            if (!(iss >> demandWasteType3)) {
                std::cerr << "Erro ao ler a terceira demanda do cliente ID: " << id << std::endl;
                continue;
            }
        }

        int readyTime, dueDateNarrow, dueDate, dueDateWide, service, bidNarrow, bid, bidWide;

        // Lê o restante das colunas na ordem correta
        if (!(iss >> readyTime >> dueDateNarrow >> dueDate >> dueDateWide >> service >> bidNarrow >> bid >> bidWide)) {
            std::cerr << "Erro ao ler os dados complementares do cliente ID: " << id << std::endl;
            continue;
        }

        // Cria e armazena o cliente
        grafo.criaSolucaoCliente(clientes, id, xCoord, yCoord, demandWasteType1, demandWasteType2, 
                                 readyTime, dueDateNarrow, dueDate, dueDateWide, service, 
                                 bidNarrow, bid, bidWide, demandWasteType3);
    }

    file.close();

    std::cout << "Sucesso: " << clientes.size() << " clientes carregados." << std::endl;
    std::cout << "Tipo de instancia detectada: " << (tem3Demandas ? "3 tipos de residuos" : "2 tipos de residuos") << std::endl;

    // Calcula a matriz de distâncias
    std::cout << "Calculando matriz de adjacencia..." << std::endl;
    std::vector<std::vector<double>> matriz = grafo.criarMatrizAdjacencia(clientes);

    // Exibe a matriz de adjacência completa na tela
    std::cout << "\n--- MATRIZ DE ADJACENCIA COMPLETA (Tempos de Viagem) ---" << std::endl;
    
    // Configura a impressão para mostrar apenas 2 casas decimais
    std::cout << std::fixed;
    std::cout.precision(2);

    for (size_t i = 0; i < matriz.size(); ++i) {
        std::cout << "Cliente " << i << ": ";
        for (size_t j = 0; j < matriz[i].size(); ++j) {
            std::cout << matriz[i][j] << "\t";
        }
        std::cout << std::endl; 
    }
    std::cout << "--------------------------------------------------------\n" << std::endl;

    
    std::cout << "--- DADOS DETALHADOS DOS CLIENTES ---" << std::endl;
    for (const auto& cliente : clientes) {
        std::cout << "Cliente ID: " << cliente.getId() 
                  << ", Coordenadas: (" << cliente.getXCoord() << ", " << cliente.getYCoord() << ")"
                  << ", Demanda Tipo 1: " << cliente.getDemandWasteType1()
                  << ", Demanda Tipo 2: " << cliente.getDemandWasteType2();
        if (tem3Demandas) {
            std::cout << ", Demanda Tipo 3: " << cliente.getDemandWasteType3();
        }
        std::cout << ", Ready Time: " << cliente.getReadyTime()
                  << ", Due Date Narrow: " << cliente.getDueDateNarrow()
                  << ", Due Date: " << cliente.getDueDate()
                  << ", Due Date Wide: " << cliente.getDueDateWide()
                  << ", Service: " << cliente.getService()
                  << ", Bid Narrow: " << cliente.getBidNarrow()
                  << ", Bid: " << cliente.getBid()
                  << ", Bid Wide: " << cliente.getBidWide()
                  << std::endl;
    }

    return 0;
}
