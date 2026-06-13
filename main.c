
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define BUFFER_SIZE 1
#define NUM_PESSOAS 8

// Structs
typedef struct {
    int prioridade_base;
    int prioridade_atual;
    char *nome;
} Pessoa;

typedef struct {
    Pessoa fila[BUFFER_SIZE];
    int contador, entrada, saida;
    pthread_mutex_t mutex;
    pthread_cond_t nao_vazio;
    pthread_cond_t nao_cheio;
} monitor_buffer_t;

// Variáveis
monitor_buffer_t *caixa;
int vezes_interagem;

// Função responsável por inicializar o monitor
void inicia_monitor() {
    caixa = (monitor_buffer_t*) malloc(sizeof(monitor_buffer_t));
    if (caixa == NULL) {
        printf("Erro ao alocar memória.\n");
        exit(1);
    }

    // Inicializar os campos da struct
    caixa->contador = 0;
    caixa->entrada = 0;
    caixa->saida = 0;

    // Inicializar o mutex e as variáveis de condição
    if (pthread_mutex_init(&caixa->mutex, NULL) != 0) {
        printf("Erro ao inicializar mutex.\n");
        free(caixa);
        exit(1);
    }

    if (pthread_cond_init(&caixa->nao_vazio, NULL) != 0) {
        printf("Erro ao inicializar condição produzida.\n");
        pthread_mutex_destroy(&caixa->mutex);
        free(caixa);
        exit(1);
    }

    if (pthread_cond_init(&caixa->nao_cheio, NULL) != 0) {
        printf("Erro ao inicializar condição consumida.\n");
        pthread_cond_destroy(&caixa->nao_vazio);
        pthread_mutex_destroy(&caixa->mutex);
        free(caixa);
        exit(1);
    }  
}

// Função responsável por adicionar pessoa na fila
void esperar(Pessoa pessoa) {
    printf("%s está na fila do caixa\n", pessoa.nome);

    pthread_mutex_lock(&caixa->mutex);

    while (caixa->contador == BUFFER_SIZE)
        pthread_cond_wait(&caixa->nao_cheio, &caixa->mutex);

    // TODO: ADICIONAR FILA DE PRIORIDADE
    caixa->fila[caixa->entrada] = pessoa;

    caixa->entrada = (caixa->entrada + 1) % BUFFER_SIZE;
    caixa->contador++;

    pthread_cond_signal(&caixa->nao_vazio);
    pthread_mutex_unlock(&caixa->mutex);
}

// Função responsável por remover pessoa na fila
void liberar() {
    pthread_mutex_lock(&caixa->mutex);

    while (caixa->contador == 0)
        pthread_cond_wait(&caixa->nao_vazio, &caixa->mutex);

    Pessoa pessoa = caixa->fila[caixa->saida];
    caixa->saida = (caixa->saida + 1) % BUFFER_SIZE;
    caixa->contador--;

    pthread_cond_signal(&caixa->nao_cheio);
    pthread_mutex_unlock(&caixa->mutex);

    printf("%s vai para casa\n", pessoa.nome);
}

// Função responsável para evitar deadlocks (Gerente)
int verificar() {}

// Função responsável controlar interações com o caixa
int *pessoa(void *args) {
    // Espera inicial antes de começar
    sleep(((rand() % 3) + 1));

    Pessoa *info_pessoa = (Pessoa *) args;

    for (int i = 0; i < vezes_interagem; i++) {
        esperar(*info_pessoa);
    
        // Fica 5 segundos no caixa
        // TODO: VE CO TONTON MIO COMO FAIS
        printf("%s está sendo atendido(a)\n", info_pessoa->nome);
        sleep(5);
    
        liberar();

        // Espera final antes de recomeçar
        sleep(((rand() % 3) + 1));
    }
}

int main (int argc, char *argv []) {
    // Inicializa a geração de números pseudoaleatórios
    srand(time(NULL)); 

    // Pega a quantidade de vezes que teremos interações das pessoas com o caixa
    if (argc != 2) {
        printf("Argumentos inválidos\n");
        exit(1);
    }
    vezes_interagem = atoi(argv[1]);
    
    // Inicializa variáveis
    pthread_t threads[NUM_PESSOAS];
    Pessoa pessoas[NUM_PESSOAS];

    // Inicializando monitor (Caixa)
    inicia_monitor();

    // Cria threads das pessoas
    int prioridades[] = {4, 4, 3, 3, 2, 2, 1, 1};
    char *nomes[] = {"Vanda", "Valter", "Maria", "Marcos", "Paula", "Pedro", "Sueli", "Silas"};

    for (int i = 0; i < NUM_PESSOAS; i++) {
        pessoas[i].prioridade_base = prioridades[i];
        pessoas[i].prioridade_atual = prioridades[i];
        pessoas[i].nome = nomes[i];

        pthread_create(&threads[i], NULL, pessoa, &pessoas[i]);
    }

    // Faz o programa principal esperar as threads
    for (int i= 0; i < NUM_PESSOAS; i++) {
        pthread_join(threads[i], NULL);
    }

    free(caixa);
    return 0; 
}