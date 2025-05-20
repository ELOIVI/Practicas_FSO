/*
 * oponent5.c
 *
 * Procés fill: mapeja taulell+estat, fa win_set(), mou l’oponent
 * i escriu log. Llegeix missatges de xoc a la seva bústia i, en rebre’n
 * un, llança dos threads que repinten el rastre cap endavant i endarrere.
 *
 * Compilar:
 *   gcc oponent5.c winsuport2.o memoria.o semafor.o missatge.o -o oponent5 \
 *       -lcurses -lpthread
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include "winsuport2.h"
#include "semafor.h"
#include "memoria.h"
#include "missatge.h"

#define MAX_OPONENTS 9

typedef struct { int f, c, d; } objecte;
typedef struct { int f, c;    } pos;

/* Dimensions del camp */
int       n_fil, n_col;

/* Sincronització i estat global */
int       sem_excl;
int      *pos_fi;

/* Rastre de l’oponent */
pos      *pos_opo;
int       n_opo;

/* Increments de moviment */
int df[4] = { -1,  0,  1,  0 };
int dc[4] = {  0, -1,  0,  1 };

/* Log */
FILE     *log_file;

/* Bústia de missatges */
int       misq;

/* Mutex per protegir pos_opo */
static pthread_mutex_t opo_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Prototips */
void esborrar_posicions(pos p_pos[], int n_pos);
void *thread_recv(void *arg);
void *thread_pinta_avant(void *arg);
void *thread_pinta_endre(void *arg);

int main(int argc, char *argv[])
{
    int id_tab, id_shmfi, idx, varia, rmin, rmax;
    char *logpath;
    void *p_tab;
    objecte opo;
    pos seg;
    int f0, v, canvi, i;
    pthread_t th_msg;

    /* 1) Arguments */
    if (argc != 12) {
        fprintf(stderr,
            "Ús: %s id_tab n_fil n_col id_shmfi sem idx varia rmin rmax log misq\n",
            argv[0]);
        return 1;
    }
    id_tab    = atoi(argv[1]);
    n_fil     = atoi(argv[2]);
    n_col     = atoi(argv[3]);
    id_shmfi  = atoi(argv[4]);
    sem_excl  = atoi(argv[5]);
    idx       = atoi(argv[6]);
    varia     = atoi(argv[7]);
    rmin      = atoi(argv[8]);
    rmax      = atoi(argv[9]);
    logpath   = argv[10];
    misq      = atoi(argv[11]);

    /* 2) Inicialitza rastre i log */
    pos_opo = calloc(n_fil * n_col / 2, sizeof(pos));
    if (!pos_opo) return 1;
    n_opo = 0;

    srand(getpid());
    log_file = fopen(logpath, "a");
    if (!log_file) return 1;
    setvbuf(log_file, NULL, _IONBF, 0);

    /* 3) Mapatge taulell i curses */
    p_tab = map_mem(id_tab);
    win_set(p_tab, n_fil, n_col);

    /* 4) Mapatge estat compartit */
    pos_fi = map_mem(id_shmfi);

    /* 5) Dibuixa posició inicial */
    opo.f = (n_fil - 1) / (idx + 2) * (idx + 1);
    opo.c = (n_col * 3) / 4;
    opo.d = rand() % 4;
    waitS(sem_excl);
      win_escricar(opo.f, opo.c, '1' + idx, INVERS);
      pos_opo[n_opo++] = (pos){ opo.f, opo.c };
    signalS(sem_excl);

    /* 6) Thread receptor de missatges */
    if (pthread_create(&th_msg, NULL, thread_recv, NULL) != 0) {
        perror("pthread_create missatges");
        return 1;
    }
    /* 6b) Desenganxa el thread perquè no bloquegi l’acabament */
    pthread_detach(th_msg);

    /* 7) Bucle de moviment */
    f0 = pos_fi[0];
    v  = pos_fi[1];
    while (f0 == 0 && v > 0) {
        waitS(sem_excl);
          f0 = pos_fi[0];
          v  = pos_fi[1];
        signalS(sem_excl);

        /* càlcul següent posició */
        canvi = 0;
        seg.f = opo.f + df[opo.d];
        seg.c = opo.c + dc[opo.d];
        if (win_quincar(seg.f, seg.c) != ' ') canvi = 1;
        else if (varia > 0 && rand() % 10 < varia) canvi = 1;

        /* gir si cal */
        if (canvi) {
            int vd[3], nd = 0, vk;
            for (i = -1; i <= 1; ++i) {
                vk = (opo.d + i + 4) % 4;
                pos test = { opo.f + df[vk], opo.c + dc[vk] };
                if (win_quincar(test.f, test.c) == ' ') {
                    vd[nd++] = vk;
                }
            }
            if (nd > 0) {
                opo.d = vd[rand() % nd];
                seg.f = opo.f + df[opo.d];
                seg.c = opo.c + dc[opo.d];
            } else {
                /* sense sortida, acaba */
                break;
            }
        }

        /* mou i registra */
        opo.f = seg.f;
        opo.c = seg.c;
        pthread_mutex_lock(&opo_mutex);
          pos_opo[n_opo++] = seg;
        pthread_mutex_unlock(&opo_mutex);

        waitS(sem_excl);
          win_escricar(opo.f, opo.c, '1' + idx, INVERS);
          fprintf(log_file,
                  "Oponent %d: (%d,%d) dir %d\n",
                  idx, opo.f, opo.c, opo.d);
        signalS(sem_excl);

        usleep((rand() % (rmax - rmin + 1) + rmin) * 1000);
    }

    /* 8) Neteja rastre i actualitza CPU vius */
    esborrar_posicions(pos_opo, n_opo);
    waitS(sem_excl);
      pos_fi[1]--;
    signalS(sem_excl);

    /* 9) Allibera memòria i acaba */
    free(pos_opo);
    return 0;
}

/*
 * esborrar_posicions: neteja el rastre complet de l’oponent
 * (igual que en fases anteriors, sense canvis aquí)
 */
void esborrar_posicions(pos p_pos[], int n_pos)
{
    for (int i = n_pos - 1; i >= 0; --i) {
        waitS(sem_excl);
          win_escricar(p_pos[i].f, p_pos[i].c, ' ', NO_INV);
        signalS(sem_excl);
        win_update();
        win_retard(10);
    }
}

/*
 * thread_recv: funció thread desenganxada que espera UN sol missatge
 * de xoc enviat pel tron usuari. Quan rep la posició de xoc:
 *   1) llança dos threads que pinten el rastre repintat:
 *      - thread_pinta_avant: des del punt de xoc fins al final del rastre
 *      - thread_pinta_endre: des del punt de xoc fins a l’inici del rastre
 *   2) fa pthread_join() sobre aquests dos threads per sincronitzar
 */
void *thread_recv(void *arg)
{
    (void)arg;
    pos choque;

    /* Processa tots els missatges de xoc que arribin */
    while (receiveM(misq, &choque) > 0) {
        /* Per cada col·lisió, llancem els threads de repintat */
        pthread_t th_forw, th_back;

        /* repintat cap endavant */
        pthread_create(&th_forw, NULL,
                       thread_pinta_avant, &choque);
        /* repintat cap enrere */
        pthread_create(&th_back, NULL,
                       thread_pinta_endre, &choque);

        /* Esperem fins que ambdós acabin */
        pthread_join(th_forw, NULL);
        pthread_join(th_back, NULL);
    }

    /* Quan el procés fill acabi, aquest thread també sortirà */
    return NULL;
}


/*
 * thread_pinta_avant: repinta el rastre de l’oponent des de l’índex
 * on es va produir el xoc fins a la darrera posició registrada.
 * Protegeix accés a pos_opo amb opo_mutex i usa sem_excl
 * per cridar curses de forma segura.
 */
void *thread_pinta_avant(void *arg)
{
    pos *pchoque = arg;
    int start = 0, i;

    /* Busquem l’índex dins de pos_opo on coincideix la posició de xoc */
    pthread_mutex_lock(&opo_mutex);
    for (i = 0; i < n_opo; ++i) {
        if (pos_opo[i].f == pchoque->f &&
            pos_opo[i].c == pchoque->c) {
            start = i;
            break;
        }
    }
    /* Després de trobar l’índex, repintem cap endavant */
    for (i = start; i < n_opo; ++i) {
        waitS(sem_excl);
          /* caràcter 'X' indica repintat INVERSA */
          win_escricar(pos_opo[i].f, pos_opo[i].c,
                        'X', INVERS);
        signalS(sem_excl);
        win_update();
        win_retard(10);
    }
    pthread_mutex_unlock(&opo_mutex);

    return NULL;
}

/*
 * thread_pinta_endre: repinta el rastre de l’oponent des de l’índex
 * on es va produir el xoc fins a la primera posició registrada.
 * Funciona simètricament a thread_pinta_avant, però pinta amb 'x'
 * i NO_INV per diferenciar la direcció inversa.
 */
void *thread_pinta_endre(void *arg)
{
    pos *pchoque = arg;
    int start = 0, i;

    /* Busquem l’índex del xoc de la mateixa manera */
    pthread_mutex_lock(&opo_mutex);
    for (i = 0; i < n_opo; ++i) {
        if (pos_opo[i].f == pchoque->f &&
            pos_opo[i].c == pchoque->c) {
            start = i;
            break;
        }
    }
    /* Repintat cap enrere: de start fins a 0 */
    for (i = start; i >= 0; --i) {
        waitS(sem_excl);
          /* caràcter 'x' indica repintat NO_INVERSA */
          win_escricar(pos_opo[i].f, pos_opo[i].c,
                        'x', NO_INV);
        signalS(sem_excl);
        win_update();
        win_retard(10);
    }
    pthread_mutex_unlock(&opo_mutex);

    return NULL;
}


