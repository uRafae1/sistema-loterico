# Compilador
CC = gcc

# Nome do executável
TARGET = threads

# Quantidade de vezes que os clientes vão ter que passar no caixa
VEZES ?= 1 

# Código
SRC = main.c

# Nome da regra para ser compilada
all: $(TARGET) 

# Gera o executável
$(TARGET): $(SRC)
	$(CC) -o $(TARGET) $(SRC)

# Executar o comando junto com a quantidade de vezes
run: $(TARGET)
	./$(TARGET) $(VEZES)

# Apaga o executável
clean:
	rm -f $(TARGET)