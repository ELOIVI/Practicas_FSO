/*
 * oponent3.c
 *
 * Procés fill: mapeja taulell+estat, fa win_set(), mou l’oponent
 * i escriu log. No pot usar win_gettec.
 *
 * Compilar:
 *   gcc oponent3.c winsuport2.o memoria.o semafor.o -o oponent3 -lcurses
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include "winsuport2.h"
#include "semafor.h"
#include "memoria.h"

#define MAX_OPONENTS 9

typedef struct { int f; int c; int d; } objecte;
typedef struct { int f; int c; } pos;

/* Globals */
int n_fil, n_col;                /* dimensions del camp de joc */
int sem_excl;                    /* semàfor d'exclusió */
int *pos_fi;                     /* estat global (usuari, CPUs vius, flag neteja) */
pos *pos_opo;                   /* taula de posicions recorregudes */
int n_opo = 0;                   /* comptador de posicions emmagatzemades */
int df[] = { -1,  0,  1,  0 };    /* increments fila per direcció */
int dc[] = {  0, -1,  0,  1 };    /* increments columna per direcció */
FILE *log_file;                  /* fitxer de log */

/* Prototipus */
void esborrar_posicions(pos p_pos[], int n_pos);

int main(int argc, char *argv[]) {
    /* Variables locals */
    int id_tab, id_shmfi, idx;
    int varia, rmin, rmax;
    char *logpath;
    void *p_tab;
    objecte opo;
    pos seg;
    int f0, v, collision, canvi;
    int i;

    /* 1) Arguments */
    if (argc != 11) {
        fprintf(stderr, "Ús: %s id_tab n_fil n_col id_shmfi sem idx varia rmin rmax log\n", argv[0]);
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

    /* Allocació taula de posicions */
    pos_opo = calloc(n_fil * n_col / 2, sizeof(pos));
    if (!pos_opo) return 1;

    /* Inicialitza aleatoris i log */
    srand(getpid());
    log_file = fopen(logpath, "a");
    if (!log_file) return 1;
    setvbuf(log_file, NULL, _IONBF, 0);

    /* 2) Mapatge taulell i set */
    p_tab = map_mem(id_tab);
    win_set(p_tab, n_fil, n_col);

    /* 3) Mapatge estat compartit */
    pos_fi = map_mem(id_shmfi);

    /* 4) Estat inicial del tron */
    opo.f = (n_fil - 1) / (idx + 2) * (idx + 1);
    opo.c = (n_col * 3) / 4;
    opo.d = rand() % 4;
    waitS(sem_excl);
      win_escricar(opo.f, opo.c, '1' + idx, INVERS);
      pos_opo[n_opo++] = (pos){ opo.f, opo.c };
    signalS(sem_excl);

    /* Preparació moviments */
    f0 = pos_fi[0];
    v  = pos_fi[1];
    collision = 0;

    /* 5) Bucle de moviment */
    while (f0 == 0 && v > 0 && !collision) {
        waitS(sem_excl);
          f0 = pos_fi[0];
          v  = pos_fi[1];
        signalS(sem_excl);

        /* Calcular seguent posició */
        canvi = 0;
        seg.f = opo.f + df[opo.d];
        seg.c = opo.c + dc[opo.d];
        if (win_quincar(seg.f, seg.c) != ' ') {
            canvi = 1;
        } else if (varia > 0 && rand() % 10 < varia) {
            canvi = 1;
        }

        /* Canvi de direcció si cal */
        if (canvi) {
            int vd[3], nd = 0;
            for (i = -1; i <= 1; ++i) {
                int vk = (opo.d + i + 4) % 4;
                pos test = { opo.f + df[vk], opo.c + dc[vk] };
                if (win_quincar(test.f, test.c) == ' ') {
                    vd[nd++] = vk;
                }
            }
            if (nd == 0) {
                collision = 1;
            } else if (nd == 1) {
                opo.d = vd[0];
            } else {
                opo.d = vd[rand() % nd];
            }
            if (!collision) {
                seg.f = opo.f + df[opo.d];
                seg.c = opo.c + dc[opo.d];
            }
        }

        /* Mou i escriu */
        opo.f = seg.f;
        opo.c = seg.c;
        pos_opo[n_opo] = seg;
        n_opo++;
        waitS(sem_excl);
          win_escricar(opo.f, opo.c, '1' + idx, INVERS);
          fprintf(log_file, "Oponent %d: (%d,%d) dir %d\n",
                  idx, opo.f, opo.c, opo.d);
        signalS(sem_excl);

        /* Retard aleatori */
        usleep((rand() % (rmax - rmin + 1) + rmin) * 1000);
    }

    /* Esborra rastre i decrementar CPU vius */
    esborrar_posicions(pos_opo, n_opo);
    waitS(sem_excl);
      pos_fi[1]--;
    signalS(sem_excl);
    free(pos_opo);
    return 0;
}

/* esborrar_posicions: neteja el rastre d'una taula de posicions */
void esborrar_posicions(pos p_pos[], int n_pos) {
    int i;
    for (i = n_pos - 1; i >= 0; --i) {
        waitS(sem_excl);
          win_escricar(p_pos[i].f, p_pos[i].c, ' ', NO_INV);
        signalS(sem_excl);
        win_update();
        win_retard(10);
    }
}

