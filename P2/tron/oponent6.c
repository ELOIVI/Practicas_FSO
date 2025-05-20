/*
 * oponent6.c
 *
 * Procés fill: mapeja taulell+estat, fa win_set(), mou l’oponent
 * i escriu log. Llegeix missatges de xoc a la seva bústia i:
 *   - si m.type == 0: repintat com abans
 *   - si m.type == 1: “trampa”: esborra el rastre, decrementa CPU vius
 *     i acaba el procés
 *
 * Compilar:
 *   gcc oponent6.c winsuport2.o memoria.o semafor.o missatge.o -o oponent6 \
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

/* Estructures per als objectes i posicions */
typedef struct { int f, c, d; } objecte;
typedef struct { int f, c;    } pos;
/* Missatge: 0 = repaint, 1 = trampa */
typedef struct { pos choque; int type; } msg_t;

/* Globals de sincronització i estat */
int       n_fil, n_col;      /* dimensions del taulell */
int       sem_excl;          /* semàfor per curses */
int      *pos_fi;            /* [0]=usuari viu?,[1]=CPU vius,[2]=neteja? */

/* Rastre de l’oponent */
pos      *pos_opo;
int       n_opo;

/* Increments de moviment: 0=amunt,1=esquerra,2=avall,3=dreta */
static const int df[4] = { -1,  0,  1,  0 };
static const int dc[4] = {  0, -1,  0,  1 };

/* Fitxer de log */
FILE     *log_file;

/* Bústia de missatges per a aquest oponent */
int       misq;

/* Mutex per protegir pos_opo en els threads de pinta */
static pthread_mutex_t opo_mutex   = PTHREAD_MUTEX_INITIALIZER;
/* Mutex per “congelar” aquest oponent durant l’esborrat de trampa */
static pthread_mutex_t alive_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Prototips */
void    esborrar_posicions(pos p_pos[], int n_pos);
void   *thread_recv(void *arg);
void   *thread_pinta_avant(void *arg);
void   *thread_pinta_endre(void *arg);

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
    id_tab   = atoi(argv[1]);
    n_fil    = atoi(argv[2]);
    n_col    = atoi(argv[3]);
    id_shmfi = atoi(argv[4]);
    sem_excl = atoi(argv[5]);
    idx      = atoi(argv[6]);
    varia    = atoi(argv[7]);
    rmin     = atoi(argv[8]);
    rmax     = atoi(argv[9]);
    logpath  = argv[10];
    misq     = atoi(argv[11]);

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

    /* 6) Llança thread receptor de missatges */
    if (pthread_create(&th_msg, NULL, thread_recv, NULL) != 0) {
        perror("pthread_create missatges");
        return 1;
    }
    pthread_detach(th_msg);

    /* 7) Bucle de moviment de l’oponent */
    waitS(sem_excl); f0 = pos_fi[0]; v = pos_fi[1]; signalS(sem_excl);
    while (f0 == 0 && v > 0) {
        /* 7.0) Espera si ja s’ha iniciat l’esborrat de trampa */
        pthread_mutex_lock(&alive_mutex);
        pthread_mutex_unlock(&alive_mutex);

        /* 7.1) Llegeix estat global */
        waitS(sem_excl);
          f0 = pos_fi[0];
          v  = pos_fi[1];
        signalS(sem_excl);

        /* 7.2) Calcula seguent posició */
        canvi = 0;
        seg.f = opo.f + df[opo.d];
        seg.c = opo.c + dc[opo.d];
        if (win_quincar(seg.f, seg.c) != ' ') 
            canvi = 1;
        else if (varia > 0 && rand() % 10 < varia) 
            canvi = 1;

        /* 7.3) Gir si cal */
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
                /* sense sortida: surt */
                break;
            }
        }

        /* 7.4) Mou i registra el rastre */
        opo.f = seg.f;
        opo.c = seg.c;
        pthread_mutex_lock(&opo_mutex);
          pos_opo[n_opo++] = seg;
        pthread_mutex_unlock(&opo_mutex);

        waitS(sem_excl);
          win_escricar(opo.f, opo.c, '1' + idx, INVERS);
          fprintf(log_file, "Oponent %d: (%d,%d) dir %d\n",
                  idx, opo.f, opo.c, opo.d);
        signalS(sem_excl);

        usleep((rand() % (rmax - rmin + 1) + rmin) * 1000);
    }

    /* 8) Neteja rastre final i decrementa CPU vius */
    esborrar_posicions(pos_opo, n_opo);
    waitS(sem_excl);
      pos_fi[1]--;
    signalS(sem_excl);

    free(pos_opo);
    return 0;
}

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

void *thread_recv(void *arg)
{
    (void)arg;
    msg_t m;
    int i;

    /* Processa tots els missatges de xoc */
    while (receiveM(misq, &m) > 0) {
        if (m.type == 0) {
            /* repaint normal */
            pthread_t th_forw, th_back;
            pos *pchoque = &m.choque;
            pthread_create(&th_forw, NULL, thread_pinta_avant, pchoque);
            pthread_create(&th_back, NULL, thread_pinta_endre,  pchoque);
            pthread_join(th_forw, NULL);
            pthread_join(th_back, NULL);

        } else {
            /* trampa: esborra rastre, decrementa CPU vius i acaba */
            pthread_mutex_lock(&alive_mutex);
            for (i = n_opo - 1; i >= 0; --i) {
                waitS(sem_excl);
                  win_escricar(pos_opo[i].f,
                               pos_opo[i].c,
                               ' ', NO_INV);
                  win_update();
                signalS(sem_excl);
                win_retard(10);
            }
            waitS(sem_excl);
              pos_fi[1]--;
            signalS(sem_excl);
            free(pos_opo);
            /* no fem unlock: sortim directament */
            exit(0);
        }
    }
    return NULL;
}

void *thread_pinta_avant(void *arg)
{
    pos *pchoque = arg;
    int start = 0, i;
    pthread_mutex_lock(&opo_mutex);
    for (i = 0; i < n_opo; ++i) {
        if (pos_opo[i].f == pchoque->f &&
            pos_opo[i].c == pchoque->c) {
            start = i;
            break;
        }
    }
    for (i = start; i < n_opo; ++i) {
        waitS(sem_excl);
          win_escricar(pos_opo[i].f, pos_opo[i].c, 'X', INVERS);
        signalS(sem_excl);
        win_update();
        win_retard(5);
    }
    pthread_mutex_unlock(&opo_mutex);
    return NULL;
}

void *thread_pinta_endre(void *arg)
{
    pos *pchoque = arg;
    int start = 0, i;
    pthread_mutex_lock(&opo_mutex);
    for (i = 0; i < n_opo; ++i) {
        if (pos_opo[i].f == pchoque->f &&
            pos_opo[i].c == pchoque->c) {
            start = i;
            break;
        }
    }
    for (i = start; i >= 0; --i) {
        waitS(sem_excl);
          win_escricar(pos_opo[i].f, pos_opo[i].c, 'X', INVERS);
        signalS(sem_excl);
        win_update();
        win_retard(5);
    }
    pthread_mutex_unlock(&opo_mutex);
    return NULL;
}

