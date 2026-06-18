
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define BUFFER_SIZE 1
#define NUM_PESSOAS 8

// Structs
typedef struct {
    int prioridade_base;
    char *nome;
    int id;
} Pessoa;

typedef struct No {
    int prioridade;
    Pessoa pessoa;
    struct No *proximo;
} No;

typedef struct Fila {
    No *inicio;
} Fila;

typedef struct {
    Fila fila;
    int contador, entrada, saida;
    pthread_mutex_t mutex;
    pthread_cond_t nao_vazio;
    pthread_cond_t nao_cheio;
    pthread_cond_t usar_fila;
} monitor_buffer_t;

// Variáveis
monitor_buffer_t *caixa;
monitor_buffer_t *fila;
int vezes_interagem;
int finalizou_interacoes = 0;

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
int pronto = 0;

// Insere um elemento mantendo a ordem de prioridade
void inserir_fila(Pessoa pessoa) {
    No *novo = (No *) malloc(sizeof(No));

    novo->prioridade = pessoa.prioridade_base;
    novo->pessoa = pessoa;
    novo->proximo = NULL;

    if (caixa->fila.inicio == NULL || pessoa.prioridade_base > caixa->fila.inicio->prioridade) {
        novo->proximo = caixa->fila.inicio;
        caixa->fila.inicio = novo;

    } else {
        No *atual = caixa->fila.inicio;
        while (atual->proximo != NULL && atual->proximo->prioridade >= pessoa.prioridade_base) {
            atual = atual->proximo;
        }

        novo->proximo = atual->proximo;
        atual->proximo = novo;

        No *ultrapassado = novo->proximo;
        while (ultrapassado != NULL) {
            printf("Gerente detectou inanição, aumentando prioridade de %s\n", ultrapassado->pessoa.nome);
            ultrapassado->prioridade++;
            ultrapassado = ultrapassado->proximo;
        }
    }
}

// Remove o elemento de maior prioridade (primeiro da lista)
Pessoa remover_fila() {
    Pessoa vazia;
    
    if (caixa->fila.inicio == NULL) {
        printf("Fila vazia!\n");
        return vazia;
    }

    No *removido = caixa->fila.inicio;
    caixa->fila.inicio = removido->proximo;
    Pessoa pessoa = removido->pessoa;
    return pessoa;
}

// Função responsável por inicializar os monitores
void inicia_monitor_caixa() {
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
void inicia_monitor_fila() {
    fila = (monitor_buffer_t*) malloc(sizeof(monitor_buffer_t));
    if (fila == NULL) {
        printf("Erro ao alocar memória.\n");
        exit(1);
    }

    // Inicializar os campos da struct
    fila->contador = 0;
    fila->entrada = 0;
    fila->saida = 0;

    // Inicializar o mutex e as variáveis de condição
    if (pthread_mutex_init(&fila->mutex, NULL) != 0) {
        printf("Erro ao inicializar mutex.\n");
        free(fila);
        exit(1);
    }

    if (pthread_cond_init(&fila->nao_vazio, NULL) != 0) {
        printf("Erro ao inicializar condição produzida.\n");
        pthread_mutex_destroy(&fila->mutex);
        free(fila);
        exit(1);
    }

    if (pthread_cond_init(&fila->nao_cheio, NULL) != 0) {
        printf("Erro ao inicializar condição consumida.\n");
        pthread_cond_destroy(&fila->nao_vazio);
        pthread_mutex_destroy(&fila->mutex);
        free(fila);
        exit(1);
    }  
}

// Função responsável por esperar para ser atendido
void esperar(Pessoa pessoa) {
    pthread_mutex_lock(&caixa->mutex);

    while (caixa->contador == BUFFER_SIZE || (pessoa.id != caixa->fila.inicio->pessoa.id)) {
        pthread_cond_wait(&caixa->nao_cheio, &caixa->mutex);
    }

    Pessoa atendendo = remover_fila();
    caixa->entrada = (caixa->entrada + 1) % BUFFER_SIZE;
    caixa->contador++;

    pthread_cond_signal(&caixa->nao_vazio);
    pthread_mutex_unlock(&caixa->mutex);
}

// Função responsável por remover pessoa na fila
void liberar() {
    pthread_mutex_lock(&caixa->mutex);

    while (caixa->contador == 0) {
        pthread_cond_wait(&caixa->nao_vazio, &caixa->mutex);
    }

    caixa->saida = (caixa->saida + 1) % BUFFER_SIZE;
    caixa->contador--;

    pthread_cond_signal(&caixa->nao_cheio);
    pthread_mutex_unlock(&caixa->mutex);
}

// Função responsável para entrar na fila
void entrar_fila(Pessoa pessoa) {
    pthread_mutex_lock(&fila->mutex);

    while (fila->contador == BUFFER_SIZE) {
        pthread_cond_wait(&fila->usar_fila, &fila->mutex);
    }

    inserir_fila(pessoa);

    pthread_cond_signal(&fila->usar_fila);
    pthread_mutex_unlock(&fila->mutex);
}

// Função responsável para evitar deadlocks (Gerente)
int verificar() {
    while (finalizou_interacoes == 0) {
        sleep(5);
        pthread_cond_signal(&caixa->nao_cheio);
    }
}

// Função responsável controlar interações com o caixa
int *pessoa(void *args) {

    // Espera broadcast para iniciar
    pthread_mutex_lock(&lock);
    while (pronto == 0) {
        pthread_cond_wait(&cond, &lock);
    }
    pthread_mutex_unlock(&lock);

    Pessoa *info_pessoa = (Pessoa *) args;

    // Tenta interagir com o atendimento
    for (int i = 0; i < vezes_interagem; i++) {\
        printf("%s está na fila do caixa\n", info_pessoa->nome);
        entrar_fila(*info_pessoa);
        esperar(*info_pessoa);
    
        printf("%s está sendo atendido(a)\n", info_pessoa->nome);
        sleep(5);
    
        printf("%s vai para casa\n", info_pessoa->nome);
        liberar();

        // Espera final antes de recomeçar
        if (i == vezes_interagem - 1) {
            sleep(((rand() % 3) + 1));
        }
    }
}

// Main 
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
    pthread_t gerente;
    Pessoa pessoas[NUM_PESSOAS];

    // Inicializando monitores
    inicia_monitor_caixa();
    inicia_monitor_fila();

    // Cria threads das pessoas
    int prioridades[] = {4, 4, 3, 3, 2, 2, 1, 1};
    char *nomes[] = {"Maria", "Marcos", "Vanda", "Valter", "Paula", "Pedro", "Sueli", "Silas"};

    for (int i = 0; i < NUM_PESSOAS; i++) {
        pessoas[i].prioridade_base = prioridades[i];
        pessoas[i].nome = nomes[i];
        pessoas[i].id = i;

        pthread_create(&threads[i], NULL, pessoa, &pessoas[i]);
    }

    pthread_create(&gerente, NULL, verificar, NULL);

    // Transmite o broadcast
    pthread_mutex_lock(&lock);
    pronto = 1;
    pthread_cond_broadcast(&cond);
    pthread_mutex_unlock(&lock);

    // Faz o programa principal esperar as threads
    for (int i= 0; i < NUM_PESSOAS; i++) {
        pthread_join(threads[i], NULL);
    }

    // Avisa para o gerente que finalizou as interações
    finalizou_interacoes = 1;
    pthread_join(gerente, NULL);

    free(caixa);
    return 0; 
}