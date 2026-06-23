
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define TRUE 1
#define FALSE 0

#define NUM_PESSOAS 8

// Structs
typedef struct {
    char *nome;
    int prioridade_base;
    pthread_cond_t pode_ir;
} Pessoa;

typedef struct No {
    int prioridade;
    Pessoa *pessoa;
    struct No *proximo;
} No;

typedef struct Fila {
    No *inicio;
} Fila;

typedef struct {
    Fila fila;
    int usando;
    pthread_mutex_t mutex;
    pthread_cond_t caixa_ocupado;
    pthread_cond_t caixa_livre;
    pthread_cond_t usar_fila;
} monitor_buffer_t;

// Variáveis
monitor_buffer_t *caixa;
monitor_buffer_t *fila;
int vezes_interagem;
int finalizou_interacoes = 0;
int alguem_em_atendimento = 0;

pthread_mutex_t mutex_broadcast = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_broadcast = PTHREAD_COND_INITIALIZER;
int pronto = 0;

// Insere um elemento mantendo a ordem de prioridade
void inserir_fila(Pessoa *pessoa) {
    No *novo = (No *) malloc(sizeof(No));

    novo->prioridade = pessoa->prioridade_base;
    novo->pessoa = pessoa;
    novo->proximo = NULL;

    if (caixa->fila.inicio == NULL || pessoa->prioridade_base > caixa->fila.inicio->prioridade) {
        novo->proximo = caixa->fila.inicio;
        caixa->fila.inicio = novo;

    } else {
        No *atual = caixa->fila.inicio;
        while (atual->proximo != NULL && atual->proximo->prioridade >= pessoa->prioridade_base) {
            atual = atual->proximo;
        }

        novo->proximo = atual->proximo;
        atual->proximo = novo;

        // No *ultrapassado = novo->proximo;
        // while (ultrapassado != NULL) {
        //     printf("Gerente detectou inanição, aumentando prioridade de %s\n", ultrapassado->pessoa->nome);
        //     ultrapassado->prioridade++;
        //     ultrapassado = ultrapassado->proximo;
        // }
    }
}

// Remove o elemento de maior prioridade (primeiro da lista)
void remover_fila() {
    if (caixa->fila.inicio == NULL) {
        printf("Fila vazia!\n");
        return;
    }

    No *removido = caixa->fila.inicio;
    caixa->fila.inicio = removido->proximo;
}

// Função responsável por inicializar o monitor do caixa
void inicia_monitor_caixa() {
    caixa = (monitor_buffer_t*) malloc(sizeof(monitor_buffer_t));
    if (caixa == NULL) {
        printf("Erro ao alocar memória.\n");
        exit(1);
    }

    // Inicializar os campos da struct
    caixa->usando = FALSE;
    caixa->fila.inicio = NULL;

    // Inicializar o mutex e as variáveis de condição
    if (pthread_mutex_init(&caixa->mutex, NULL) != 0) {
        printf("Erro ao inicializar mutex.\n");
        free(caixa);
        exit(1);
    }

    if (pthread_cond_init(&caixa->caixa_ocupado, NULL) != 0) {
        printf("Erro ao inicializar condição produzida.\n");
        pthread_mutex_destroy(&caixa->mutex);
        free(caixa);
        exit(1);
    }

    if (pthread_cond_init(&caixa->caixa_livre, NULL) != 0) {
        printf("Erro ao inicializar condição consumida.\n");
        pthread_cond_destroy(&caixa->caixa_ocupado);
        pthread_mutex_destroy(&caixa->mutex);
        free(caixa);
        exit(1);
    }  
}

// Função responsável por inicializar o monitor da fila
void inicia_monitor_fila() {
    fila = (monitor_buffer_t*) malloc(sizeof(monitor_buffer_t));
    if (fila == NULL) {
        printf("Erro ao alocar memória.\n");
        free(caixa);
        exit(1);
    }

    // Inicializar os campos da struct
    fila->usando = FALSE;

    // Inicializar o mutex e as variáveis de condição
    if (pthread_mutex_init(&fila->mutex, NULL) != 0) {
        printf("Erro ao inicializar mutex.\n");
        free(fila);
        free(caixa);
        exit(1);
    }

    if (pthread_cond_init(&fila->caixa_ocupado, NULL) != 0) {
        printf("Erro ao inicializar condição produzida.\n");
        pthread_mutex_destroy(&fila->mutex);
        free(fila);
        free(caixa);
        exit(1);
    }

    if (pthread_cond_init(&fila->caixa_livre, NULL) != 0) {
        printf("Erro ao inicializar condição consumida.\n");
        pthread_cond_destroy(&fila->caixa_ocupado);
        pthread_mutex_destroy(&fila->mutex);
        free(fila);
        free(caixa);
        exit(1);
    }  
}

// Função responsável por esperar para ser atendido
void esperar(Pessoa *pessoa) {
    pthread_mutex_lock(&caixa->mutex);

    while (caixa->usando == TRUE) {
        pthread_cond_wait(&pessoa->pode_ir, &caixa->mutex);
    }

    caixa->usando = TRUE;
    remover_fila();
    printf("%s está sendo atendido(a)\n", pessoa->nome);

    pthread_mutex_unlock(&caixa->mutex);
}

// Função responsável por remover pessoa na fila
void liberar(Pessoa *pessoa) {
    pthread_mutex_lock(&caixa->mutex);
    caixa->usando = FALSE;
    printf("%s vai para casa\n", pessoa->nome);

    if (caixa->fila.inicio != NULL) {
        pthread_cond_signal(&caixa->fila.inicio->pessoa->pode_ir);
    }
    pthread_mutex_unlock(&caixa->mutex);
}

// Função responsável para entrar na fila
void entrar_fila(Pessoa *pessoa) {
    pthread_mutex_lock(&fila->mutex);
    while (fila->usando == TRUE) {
        pthread_cond_wait(&fila->usar_fila, &fila->mutex);
    }

    fila->usando = TRUE;
    inserir_fila(pessoa);
    printf("%s está na fila do caixa\n", pessoa->nome);
    fila->usando = FALSE;

    pthread_cond_signal(&fila->usar_fila);
    pthread_mutex_unlock(&fila->mutex);
}

// Função responsável controlar interações do gerente
int *gerente() {
    while (finalizou_interacoes == 0) {
        sleep(5);
        if (caixa->usando = FALSE && caixa->fila.inicio != NULL) {
            printf("Gerente detectou deadlock, liberando %s para atendimento\n", caixa->fila.inicio->pessoa->nome);
            pthread_cond_signal(&caixa->fila.inicio->pessoa->pode_ir);
        }   
    }
}

// Função responsável controlar interações da pessoa
int *pessoa(void *args) {

    // Pega informações da pessoa
    Pessoa *info_pessoa = (Pessoa *) args;

    // Espera broadcast para iniciar
    pthread_mutex_lock(&mutex_broadcast);
    if (pronto == 0) {
        pthread_cond_wait(&cond_broadcast, &mutex_broadcast);
    }
    pthread_mutex_unlock(&mutex_broadcast);

    // Tenta interagir com o atendimento
    for (int i = 0; i < vezes_interagem; i++) {
        entrar_fila(info_pessoa);
        esperar(info_pessoa);
        sleep(5);
        liberar(info_pessoa);

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
    pthread_t thread_gerente;
    Pessoa pessoas[NUM_PESSOAS];

    // Inicializando monitores
    inicia_monitor_caixa();
    inicia_monitor_fila();

    // Cria threads das pessoas e gerente
    char *nomes[] = {"Maria", "Marcos", "Vanda", "Valter", "Paula", "Pedro", "Sueli", "Silas"};
    int prioridades[] = {4, 4, 3, 3, 2, 2, 1, 1};

    for (int i = 0; i < NUM_PESSOAS; i++) {
        pessoas[i].nome = nomes[i];
        pessoas[i].prioridade_base = prioridades[i];
        pthread_cond_init(&pessoas[i].pode_ir, NULL);

        pthread_create(&threads[i], NULL, pessoa, &pessoas[i]);
    }
    // pthread_create(&thread_gerente, NULL, gerente, NULL);

    // Transmite o broadcast para as pessoas iniciarem
    pthread_mutex_lock(&mutex_broadcast);
    pthread_cond_broadcast(&cond_broadcast);
    pronto = 1;
    pthread_mutex_unlock(&mutex_broadcast);

    // Faz o programa principal esperar as threads
    for (int i= 0; i < NUM_PESSOAS; i++) {
        pthread_join(threads[i], NULL);
    }

    // Avisa para o gerente que finalizou as interações
    finalizou_interacoes = 1;
    pthread_join(thread_gerente, NULL);

    // Finaliza sistema
    free(caixa);
    free(fila);
    return 0; 
}