/* tron6.c
 *
 * Procés pare: controla l’usuari (en un thread), mesura el temps de joc,
 * gestiona memòria compartida, crea bústies per a cada oponent i els fills.
 * Quan l’usuari travessa el rastre d’un oponent dues vegades, envia un
 * missatge de trampa perquè aquell oponent s’elimini.
 *
 * Compilar:
 *   gcc tron6.c winsuport2.o memoria.o semafor.o missatge.o -o tron6 \
 *       -lcurses -lpthread
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <pthread.h>
#include "winsuport2.h"
#include "semafor.h"
#include "memoria.h"
#include "missatge.h"

#define MAX_OPONENTS 9

/* Estructures */
typedef struct { int f, c, d; } objecte;
typedef struct { int f, c;    } pos;
/* Missatge: 0 = repaint, 1 = trampa */
typedef struct { pos choque; int type; } msg_t;

/* Dimensions del taulell */
int      n_fil, n_col;

/* Tron de l’usuari */
objecte  usu;
/* Traça de l’usuari */
pos     *p_usu;
int      n_usu;

/* Comptador de travessies per a cada oponent */
int      visits[MAX_OPONENTS];

/* Increments de moviment */
static const int df[4] = { -1,  0,  1,  0 };
static const int dc[4] = {  0, -1,  0,  1 };

/* Semàfor per a curses */
int      sem_excl;

/* PID dels fills */
pid_t    tpid[MAX_OPONENTS];

/* Memòria compartida */
int      id_shm_tab, id_shm_fi;
int     *pos_fi;  /* [0]=usuari viu?, [1]=CPU vius, [2]=neteja? */

/* Bústies de missatges */
int      misq[MAX_OPONENTS];

/* Log */
FILE    *log_file;

/* Prototips */
void    esborrar_posicions(pos p_pos[], int n_pos);
void   *mou_usuari(void *arg);

int main(int argc, char *argv[])
{
    int        num_op, varia;
    int        ret_min, ret_max;
    int        size;
    void      *p_tab;
    pthread_t  th_usu;
    int        i;
    int        game_secs = 0;
    char       time_msg[21];
    int        f0, v, mins, secs;

    /* 1) Arguments i init */
    if (!(argc == 4 || argc == 6)) {
        fprintf(stderr, "Ús: %s num_op variabilitat fitxer [ret_min ret_max]\n", argv[0]);
        exit(1);
    }
    srand(getpid());
    num_op = atoi(argv[1]);
    if (num_op < 1) num_op = 1;
    if (num_op > MAX_OPONENTS) num_op = MAX_OPONENTS;
    varia = atoi(argv[2]);

    /* Log sense buffer */
    log_file = fopen(argv[3], "w");
    if (!log_file) { perror("log"); exit(1); }
    setvbuf(log_file, NULL, _IONBF, 0);

    /* 2) curses + mida */
    printf("Joc del Tron (Fase 6)\n\tTecles: '%c','%c','%c','%c', RETURN->sortir\n",
           TEC_AMUNT, TEC_AVALL, TEC_DRETA, TEC_ESQUER);
    printf("Prem una tecla per continuar:\n");
    getchar();
    size = win_ini(&n_fil, &n_col, '+', INVERS);
    if (size < 0) exit(2);

    /* 3) memòria taulell */
    id_shm_tab = ini_mem(size);
    p_tab      = map_mem(id_shm_tab);
    win_set(p_tab, n_fil, n_col);

    /* 4) semàfor + estat */
    sem_excl = ini_sem(1);
    id_shm_fi = ini_mem(3 * sizeof(int));
    pos_fi    = map_mem(id_shm_fi);
    pos_fi[0] = 0;          /* usuari viu */
    pos_fi[1] = num_op;     /* CPU vius */
    pos_fi[2] = 0;          /* flag neteja */

    /* 5) reserva traça usuari */
    p_usu = calloc(n_fil * n_col / 2, sizeof(pos));
    if (!p_usu) {
        win_fi(); fprintf(stderr, "Error memòria dinàmica\n"); exit(3);
    }
    n_usu = 0;

    /* 6) init visits a zero */
    for (i = 0; i < MAX_OPONENTS; ++i) visits[i] = 0;

    /* 7) crear bústies */
    for (i = 0; i < num_op; ++i) {
        misq[i] = ini_mis();
    }

    /* 8) fork fills */
    {
        char buf_id[16], buf_nfil[8], buf_ncol[8],
             buf_shmfi[16], buf_sem[8], buf_var[8],
             buf_rmin[8], buf_rmax[8], buf_idx[8], buf_misq[8];
        snprintf(buf_id,    sizeof(buf_id),    "%d", id_shm_tab);
        snprintf(buf_nfil,  sizeof(buf_nfil),  "%d", n_fil);
        snprintf(buf_ncol,  sizeof(buf_ncol),  "%d", n_col);
        snprintf(buf_shmfi, sizeof(buf_shmfi), "%d", id_shm_fi);
        snprintf(buf_sem,   sizeof(buf_sem),   "%d", sem_excl);
        snprintf(buf_var,   sizeof(buf_var),   "%d", varia);
        ret_min = (argc==6?atoi(argv[4]):100);
        ret_max = (argc==6?atoi(argv[5]):100);
        snprintf(buf_rmin, sizeof(buf_rmin), "%d", ret_min);
        snprintf(buf_rmax, sizeof(buf_rmax), "%d", ret_max);
        for (i = 0; i < num_op; ++i) {
            snprintf(buf_idx, sizeof(buf_idx), "%d", i);
            snprintf(buf_misq,sizeof(buf_misq),"%d", misq[i]);
            tpid[i] = fork();
            if (tpid[i] == 0) {
                execlp("./oponent6","oponent6",
                       buf_id, buf_nfil, buf_ncol,
                       buf_shmfi, buf_sem, buf_idx,
                       buf_var, buf_rmin, buf_rmax,
                       argv[3], buf_misq, (char*)NULL);
                exit(1);
            }
        }
    }

    /* 9) dibuixa posició inicial */
    usu.f = (n_fil - 1)/2;  usu.c = n_col/4;  usu.d = 3;
    waitS(sem_excl);
      win_escricar(usu.f, usu.c, '0', INVERS);
    signalS(sem_excl);
    p_usu[n_usu++] = (pos){usu.f, usu.c};

    /* 10) llança thread usuari */
    if (pthread_create(&th_usu, NULL, mou_usuari, NULL)!=0) {
        perror("pthread_create"); exit(4);
    }

    /* 11) temporitzador */
    do {
        waitS(sem_excl);
          f0 = pos_fi[0];
          v  = pos_fi[1];
        signalS(sem_excl);

        mins = game_secs/60;  secs = game_secs%60;
        snprintf(time_msg,sizeof(time_msg),"Temps: %02d:%02d",mins,secs);

        waitS(sem_excl);
          win_escristr(time_msg);
          win_update();
        signalS(sem_excl);

        if (f0==0 && v>0) {
            win_retard(1000);
            game_secs++;
        }
    } while (f0==0 && v>0);

    /* 12) espera thread usuari */
    pthread_join(th_usu,NULL);

    /* 13) refresc final i recollida fills */
    {
        int cleaning;
        do {
            waitS(sem_excl);
              f0 = pos_fi[0];
              v  = pos_fi[1];
              cleaning = pos_fi[2];
            signalS(sem_excl);
            if (!cleaning) {
                win_update();
                win_retard(ret_min);
            } else usleep(10000);
        } while (f0==0 && (v>0||cleaning));
    }
    for (i=0; i<num_op; ++i) {
        waitpid(tpid[i],NULL,0);
        elim_mis(misq[i]);
    }

    /* 14) neteja */
    win_fi();
    free(p_usu);
    fclose(log_file);
    elim_sem(sem_excl);
    elim_mem(id_shm_tab);
    elim_mem(id_shm_fi);

    /* 15) resultat */
    mins = game_secs/60;  secs = game_secs%60;
    printf("Temps: %02d:%02d\n",mins,secs);
    if      (pos_fi[0] == -1) printf("Sortida RETURN\n");
    else if (pos_fi[0] ==  1) printf("Ha guanyat l'ordinador!\n");
    else if (pos_fi[1] ==  0) printf("Ha guanyat l'usuari!\n");
    else                      printf("Ha guanyat l'ordinador!\n");
    return 0;
}

void esborrar_posicions(pos p_pos[], int n_pos)
{
    for (int i = n_pos-1; i>=0; --i) {
        waitS(sem_excl);
          win_escricar(p_pos[i].f,p_pos[i].c,' ',NO_INV);
        signalS(sem_excl);
        win_update();
        win_retard(10);
    }
}

void *mou_usuari(void *arg)
{
    (void)arg;
    bool end = false;
    int f0,v,cleaning,tecla;
    pos seg;
    while (!end) {
        /* 1) llegim estat */
        waitS(sem_excl);
          f0       = pos_fi[0];
          v        = pos_fi[1];
          cleaning = pos_fi[2];
        signalS(sem_excl);

        /* 2) condició final */
        if (f0!=0 || v<=0 || cleaning) {
            end = true;
        } else {
            /* 3) tecla */
            waitS(sem_excl);
              tecla = win_gettec();
            signalS(sem_excl);

            /* 4) processa tecla */
            if (tecla) {
                switch (tecla) {
                  case TEC_AMUNT:  usu.d=0; break;
                  case TEC_ESQUER: usu.d=1; break;
                  case TEC_AVALL:  usu.d=2; break;
                  case TEC_DRETA:  usu.d=3; break;
                  case TEC_RETURN:
                    waitS(sem_excl);
                      pos_fi[0]=-1;
                    signalS(sem_excl);
                    end=true;
                    break;
                  default: break;
                }
            }

            /* 5) moviment i missatge */
            if (!end) {
                seg.f = usu.f + df[usu.d];
                seg.c = usu.c + dc[usu.d];

                waitS(sem_excl);
                  char c = win_quincar(seg.f, seg.c);
                signalS(sem_excl);

                if (c=='+') {
                    /* mort per paret */
                    waitS(sem_excl);
                      pos_fi[0]=1;
                    signalS(sem_excl);
                    esborrar_posicions(p_usu,n_usu);
                    end=true;
                } else {
                    /* si xoca rastre oponent */
                    int op_idx = c - '1';
                    if (op_idx>=0 && op_idx<MAX_OPONENTS) {
                        visits[op_idx]++;             // compta travessies
                        msg_t m = {
                          .choque = seg,
                          .type   = (visits[op_idx]>=2 ? 1 : 0)
                        };
                        sendM(misq[op_idx], &m, sizeof(m));
                    }
                    /* sempre moure’s */
                    waitS(sem_excl);
                      usu.f=seg.f; usu.c=seg.c;
                      win_escricar(usu.f, usu.c, '0', INVERS);
                    signalS(sem_excl);

                    p_usu[n_usu++] = seg;
                    fprintf(log_file,"Usuari: (%d,%d) dir %d\n",
                            usu.f, usu.c, usu.d);
                }
            }

            /* 6) pantalla + retard */
            if (!end) {
                waitS(sem_excl);
                  win_update();
                signalS(sem_excl);
                win_retard(100);
            }
        }
    }
    return NULL;
}

