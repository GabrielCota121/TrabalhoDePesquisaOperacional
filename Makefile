# Nome do executável final
TARGET = programa

# Compilador e flags de compilação
CXX = g++
# A flag -Iheader avisa o compilador para procurar os arquivos .hpp dentro da pasta 'header'
CXXFLAGS = -Wall -Wextra -std=c++17 -O2 -Iheader

# Encontra automaticamente todos os arquivos .cpp dentro da pasta 'src'
SRCS = $(wildcard src/*.cpp)

# Define os arquivos de objeto (.o) correspondentes salvos dentro de 'src'
OBJS = $(SRCS:.cpp=.o)

# -------------------------------------------------------------
# DETECÇÃO AUTOMÁTICA DE SISTEMA OPERACIONAL
# -------------------------------------------------------------
ifeq ($(OS),Windows_NT)
    EXEC = $(TARGET).exe
    RM_CMD = if exist src\*.o del /f /q src\*.o && if exist $(EXEC) del /f /q $(EXEC)
else
    EXEC = ./$(TARGET)
    RM_CMD = rm -f src/*.o $(TARGET)
endif
# -------------------------------------------------------------

# Regra principal
all: $(TARGET)

# Linkagem do executável final
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

# Compilação dos arquivos de código-fonte dentro de 'src' em objetos
src/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Limpeza inteligente baseada no Sistema Operacional
clean:
	@$(RM_CMD)

# Regra para limpar e compilar do zero
re: clean all

.PHONY: all clean re
