/*
 * tron5.c
 *
 * Procés pare: controla l’usuari (en un thread), mesura el temps de joc,
 * gestiona la memòria compartida del taulell, crea una bústia de missatges
 * per a cada fill "oponent5", i crea els fills. Quan l’usuari xoca amb un
 * tron oponent, envia un missatge amb les coordenades del xoc a la bústia
 * del procés fill corresponent.
 *
 * Compilar:
 *   gcc tron5.c winsuport2.o memoria.o semafor.o missatge.o -o tron5 \
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
#include "missatge.h"          /* ini_mis(), sendM() :contentReference[oaicite:0]{index=0}:contentReference[oaicite:1]{index=1}:contentReference[oaicite:2]{index=2}:contentReference[oaicite:3]{index=3} */

#define MAX_OPONENTS 9

/* Estructures per als objectes i posicions */
typedef struct { int f, c, d; } objecte;
typedef struct { int f, c;    } pos;

/* Dimensions del taulell */
int      n_fil;
int      n_col;

/* Tron de l'usuari */
objecte  usu;

/* Taula de posicions recorregudes pel tron de l'usuari */
pos     *p_usu;
int      n_usu;

/* Increments per a les 4 direccions: 0=amunt,1=esquerra,2=avall,3=dreta */
int df[4] = { -1,  0,  1,  0 };
int dc[4] = {  0, -1,  0,  1 };

/* Semàfor per a exclusió mútua */
int      sem_excl;

/* PID dels processos fill */
pid_t    tpid[MAX_OPONENTS];

/* Identificadors de memòria compartida */
int      id_shm_tab;
int      id_shm_fi;
int     *pos_fi;

/* Identificador de bústies de missatges per cada oponent */
int      misq[MAX_OPONENTS];

/* Fitxer de log */
FILE    *log_file;

/* Prototips de funcions */
void esborrar_posicions(pos p_pos[], int n_pos);
void *mou_usuari(void *arg);

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

    /* 1) Arguments i inicialització */
    if (!(argc == 4 || argc == 6)) {
        fprintf(stderr,
                "Ús: %s num_op variabilitat fitxer [ret_min ret_max]\n",
                argv[0]);
        exit(1);
    }
    srand(getpid());

    num_op = atoi(argv[1]);
    if (num_op < 1)            num_op = 1;
    if (num_op > MAX_OPONENTS) num_op = MAX_OPONENTS;
    varia   = atoi(argv[2]);

    /* Log no bufferitzat */
    log_file = fopen(argv[3], "w");
    if (!log_file) { perror("log"); exit(1); }
    setvbuf(log_file, NULL, _IONBF, 0);

    /* 2) Init curses i mida taulell */
    printf("Joc del Tron (Fase 5)\n"
           "\tTecles: '%c','%c','%c','%c', RETURN->sortir\n",
           TEC_AMUNT, TEC_AVALL, TEC_DRETA, TEC_ESQUER);
    printf("Prem una tecla per continuar:\n");
    getchar();

    size = win_ini(&n_fil, &n_col, '+', INVERS);
    if (size < 0) exit(2);

    /* 3) Tauler compartit */
    id_shm_tab = ini_mem(size);
    p_tab      = map_mem(id_shm_tab);
    win_set(p_tab, n_fil, n_col);

    /* 4) Semàfor i estat compartit */
    sem_excl = ini_sem(1);
    id_shm_fi = ini_mem(3 * sizeof(int));
    pos_fi    = map_mem(id_shm_fi);
    pos_fi[0] = 0;              /* 0 = usuari viu */
    pos_fi[1] = num_op;         /* CPU vius */
    pos_fi[2] = 0;              /* flag neteja */

    /* 5) Reserva taula d'usuari */
    p_usu = calloc(n_fil * n_col / 2, sizeof(pos));
    if (!p_usu) {
        win_fi();
        fprintf(stderr, "Error memòria dinàmica\n");
        exit(3);
    }
    n_usu = 0;

    /* 6) Crear bústies de missatges per a cada oponent */
    for (i = 0; i < num_op; ++i) {
        misq[i] = ini_mis();   /* bústia privada heretada pels fills :contentReference[oaicite:4]{index=4}:contentReference[oaicite:5]{index=5} */
    }

    /* 7) Crea fills oponent5 */
    {
        char buf_idtab[16], buf_nfil[8], buf_ncol[8];
        char buf_shmfi[16], buf_sem[8], buf_var[8];
        char buf_rmin[8], buf_rmax[8], buf_idx[8], buf_misq[8];

        snprintf(buf_idtab, sizeof(buf_idtab), "%d", id_shm_tab);
        snprintf(buf_nfil,   sizeof(buf_nfil),   "%d", n_fil);
        snprintf(buf_ncol,   sizeof(buf_ncol),   "%d", n_col);
        snprintf(buf_shmfi,  sizeof(buf_shmfi),  "%d", id_shm_fi);
        snprintf(buf_sem,    sizeof(buf_sem),    "%d", sem_excl);
        snprintf(buf_var,    sizeof(buf_var),    "%d", varia);

        ret_min = (argc == 6 ? atoi(argv[4]) : 100);
        ret_max = (argc == 6 ? atoi(argv[5]) : 100);
        snprintf(buf_rmin, sizeof(buf_rmin), "%d", ret_min);
        snprintf(buf_rmax, sizeof(buf_rmax), "%d", ret_max);

        for (i = 0; i < num_op; ++i) {
            snprintf(buf_idx, sizeof(buf_idx), "%d", i);
            snprintf(buf_misq, sizeof(buf_misq), "%d", misq[i]);

            tpid[i] = fork();
            if (tpid[i] == 0) {
                execlp("./oponent5", "oponent5",
                       buf_idtab, buf_nfil, buf_ncol,
                       buf_shmfi, buf_sem, buf_idx,
                       buf_var, buf_rmin, buf_rmax,
                       argv[3], buf_misq,
                       (char *)NULL);
                exit(1);
            }
        }
    }

    /* 8) Dibuixa posició inicial de l'usuari */
    usu.f = (n_fil - 1) / 2;
    usu.c =  n_col      / 4;
    usu.d =  3;
    waitS(sem_excl);
      win_escricar(usu.f, usu.c, '0', INVERS);
    signalS(sem_excl);
    p_usu[n_usu++] = (pos){usu.f, usu.c};

    /* 9) Llança el thread de moviment de l'usuari */
    if (pthread_create(&th_usu, NULL, mou_usuari, NULL) != 0) {
        perror("pthread_create");
        exit(4);
    }

    /* 10) Temporitzador principal (mm:ss) */
    do {
        waitS(sem_excl);
          f0 = pos_fi[0];
          v  = pos_fi[1];
        signalS(sem_excl);

        mins = game_secs / 60;
        secs = game_secs % 60;
        snprintf(time_msg, sizeof(time_msg), "Temps: %02d:%02d", mins, secs);

        waitS(sem_excl);
          win_escristr(time_msg);
          win_update();
        signalS(sem_excl);

        if (f0 == 0 && v > 0) {
            win_retard(1000);
            game_secs++;
        }
    } while (f0 == 0 && v > 0);

    /* 11) Esperem que el thread d’usuari acabi */
    pthread_join(th_usu, NULL);

    /* 12) Bucle de refresc fins a final dels fills */
    {
        int v, cleaning;
        do {
            waitS(sem_excl);
              f0       = pos_fi[0];
              v        = pos_fi[1];
              cleaning = pos_fi[2];
            signalS(sem_excl);

            if (!cleaning) {
                win_update();
                win_retard(ret_min);
            } else {
                usleep(10000);
            }
        } while (f0 == 0 && (v > 0 || cleaning));
    }

    /* 13) Esperar fills i eliminar bústies */
    for (i = 0; i < num_op; ++i) {
        waitpid(tpid[i], NULL, 0);
        elim_mis(misq[i]);  /* elimina la bústia quan l’oponent5 acaba :contentReference[oaicite:6]{index=6}:contentReference[oaicite:7]{index=7} */
    }

    /* 14) Tancar curses i netejar recursos */
    win_fi();
    free(p_usu);
    fclose(log_file);
    elim_sem(sem_excl);
    elim_mem(id_shm_tab);
    elim_mem(id_shm_fi);

    /* 15) Missatge final */
    mins = game_secs / 60;
    secs = game_secs % 60;
    printf("Temps de joc: %02d:%02d\n", mins, secs);
    if      (pos_fi[0] == -1) printf("S'ha aturat el joc amb RETURN!\n");
    else if (pos_fi[0] ==  1) printf("Ha guanyat l'ordinador!\n");
    else                      printf("Ha guanyat l'usuari!\n");

    return 0;
}

/* Esborra el rastre d’una taula de posicions */
void esborrar_posicions(pos p_pos[], int n_pos)
{
    for (int i = n_pos - 1; i >= 0; --i) {
        waitS(sem_excl);
          win_escricar(p_pos[i].f, p_pos[i].c, ' ', NO_INV);
          win_update();
        signalS(sem_excl);
        win_retard(10);
    }
}

/* Thread de moviment de l’usuari:
   només mor quan xoca amb les parets (‘+’), i quan xoca amb un oponent
   envia un missatge a la bústia del fill corresponent (coord. seg) */
void *mou_usuari(void *arg)
{
    (void)arg;
    bool end = false;

    while (!end) {
        int f0, v, cleaning, tecla;
        pos seg;

        /* 1) llegim estat global */
        waitS(sem_excl);
          f0       = pos_fi[0];
          v        = pos_fi[1];
          cleaning = pos_fi[2];
        signalS(sem_excl);

        /* 2) condició de final */
        if (f0 != 0 || v <= 0 || cleaning) {
            end = true;
        } else {
            /* 3) llegim tecla */
            waitS(sem_excl);
              tecla = win_gettec();
            signalS(sem_excl);

            /* 4) processem tecla */
            if (tecla) {
                switch (tecla) {
                  case TEC_AMUNT:  usu.d = 0; break;
                  case TEC_ESQUER: usu.d = 1; break;
                  case TEC_AVALL:  usu.d = 2; break;
                  case TEC_DRETA:  usu.d = 3; break;
                  case TEC_RETURN:
                    waitS(sem_excl);
                      pos_fi[0] = -1;
                    signalS(sem_excl);
                    end = true;
                    break;
                  default: break;
                }
            }

            /* 5) moviment i detecció */
            if (!end) {
                /* 5.1) càlcul de la nova posició */
                seg.f = usu.f + df[usu.d];
                seg.c = usu.c + dc[usu.d];

                /* 5.2) llegim contingut de destí */
                waitS(sem_excl);
                  char c = win_quincar(seg.f, seg.c);
                signalS(sem_excl);

                /* 5.3) si és paret, mort */
                if (c == '+') {
                    waitS(sem_excl);
                      pos_fi[0] = 1;
                    signalS(sem_excl);

                    esborrar_posicions(p_usu, n_usu);
                    end = true;

                } else {
                    /* 5.4) si és un oponent, enviem missatge */
                    int op_idx = c - '1';
                    if (op_idx >= 0 && op_idx < MAX_OPONENTS) {
                        sendM(misq[op_idx], &seg, sizeof(seg));
                    }
                    /* 5.5) en tots els altres casos (espai,
                           rastres d’oponents, X, x…), movem l’usuari */

                    waitS(sem_excl);
                      usu.f = seg.f;
                      usu.c = seg.c;
                      win_escricar(usu.f, usu.c, '0', INVERS);
                    signalS(sem_excl);

                    p_usu[n_usu++] = seg;
                    fprintf(log_file,
                            "Usuari: (%d,%d) dir %d\n",
                            usu.f, usu.c, usu.d);
                }
            }

            /* 6) actualització i retard */
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


