# Compilador
CC = gcc

# Nome do executável
TARGET = formigopolis

# Quantidade de vezes que os clientes vão ter que passar no caixa
VEZES ?= 2

# Código
SRC = formigopolis.c

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