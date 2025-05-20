/*
 * tron3.c
 *
 * Procés pare: controla l’usuari, gestiona la memòria compartida del taulell
 * i crea els fills "oponent3". Utilitza winsuport2 per actualitzar la pantalla.
 *
 * Compilar:
 *   gcc tron3.c winsuport2.o memoria.o semafor.o -o tron3 -lcurses
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/wait.h>
#include <stdbool.h>
#include "winsuport2.h"
#include "semafor.h"
#include "memoria.h"

#define MAX_OPONENTS 9

/* Estructures per als objectes i posicions */
typedef struct { int f, c, d; } objecte;
typedef struct { int f, c;    } pos;

/* Dimensions del taulell */
int n_fil;
int n_col;

/* Tron de l'usuari */
objecte usu;

/* Taula de posicions recorregudes pel tron de l'usuari */
pos *p_usu;
int n_usu;

/* Increments per a les 4 direccions: 0=amunt,1=esquerra,2=avall,3=dreta */
int df[4] = { -1,  0,  1,  0 };
int dc[4] = {  0, -1,  0,  1 };

/* Semàfor per a exclusió mútua */
int sem_excl;

/* PID dels processos fill */
pid_t tpid[MAX_OPONENTS];

/* Identificadors de memòria compartida */
int id_shm_tab;
int id_shm_fi;
int *pos_fi;

/* Fitxer de log */
FILE    *log_file;

/* Prototips de funcions */
void esborrar_posicions(pos p_pos[], int n_pos);
void mou_usuari(void);

int main(int argc, char *argv[])
{
    int  num_op, varia;
    int  ret_min, ret_max;
    int  size;
    char buf_idtab[16], buf_nfil[8], buf_ncol[8];
    char buf_shmfi[16], buf_sem[8], buf_var[8];
    char buf_rmin[8], buf_rmax[8];
    void *p_tab;
    int   i;

    /* Arguments */
    if (!(argc == 4 || argc == 6)) {
        fprintf(stderr,
                "Ús: %s num_op variabilitat fitxer [ret_min ret_max]\n",
                argv[0]);
        exit(1);
    }
    srand(getpid());

    /* 1) Paràmetres */
    num_op = atoi(argv[1]);
    if (num_op < 1)            num_op = 1;
    if (num_op > MAX_OPONENTS) num_op = MAX_OPONENTS;
    varia   = atoi(argv[2]);

    /* Log no bufferitzat */
    log_file = fopen(argv[3], "w");
    if (!log_file) { perror("log"); exit(1); }
    setvbuf(log_file, NULL, _IONBF, 0);

    /* 2) Init curses i mida taulell */
    printf("Joc del Tron (Fase 3)\n"
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

    /* 6) Crea fills oponent3 */
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
        tpid[i] = fork();
        if (tpid[i] == 0) {
            char buf_idx[8];
            snprintf(buf_idx, sizeof(buf_idx), "%d", i);
            execlp("./oponent3", "oponent3",
                   buf_idtab, buf_nfil, buf_ncol,
                   buf_shmfi, buf_sem, buf_idx,
                   buf_var, buf_rmin, buf_rmax,
                   argv[3], (char *)NULL);
            exit(1);
        }
    }

    /* 7) Dibuixa posició inicial de l'usuari */
    usu.f = (n_fil - 1) / 2;
    usu.c =  n_col      / 4;
    usu.d =  3;
    waitS(sem_excl);
      win_escricar(usu.f, usu.c, '0', INVERS);
    signalS(sem_excl);
    p_usu[n_usu++] = (pos){usu.f, usu.c};

    /* 8) Control de l’usuari */
    mou_usuari();

    /* 9) Bucle de refresc fins a final */
    {
        int f0, v, cleaning;
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

    /* 10) Esperar fills */
    for (i = 0; i < num_op; ++i) {
        waitpid(tpid[i], NULL, 0);
    }

    /* 11) Finalitzar */
    win_fi();
    free(p_usu);
    fclose(log_file);
    elim_sem(sem_excl);
    elim_mem(id_shm_tab);
    elim_mem(id_shm_fi);

    /* 12) Missatge final */
    if      (pos_fi[0] == -1) printf("S'ha aturat el joc amb RETURN!\n");
    else if (pos_fi[0] ==  1) printf("Ha guanyat l'ordinador!\n");
    else                      printf("Ha guanyat usuari!\n");

    return 0;
}

void esborrar_posicions(pos p_pos[], int n_pos)
{
    int i;
    for (i = n_pos - 1; i >= 0; --i) {
        waitS(sem_excl);
        win_escricar(p_pos[i].f, p_pos[i].c, ' ', NO_INV);
        win_update();
        signalS(sem_excl);
        win_retard(10);
    }
}

void mou_usuari(void)
{
    int  f0, v, cleaning;
    int  tecla;
    pos  seg;
    bool end = false;

    while (!end) {
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
                    case TEC_AMUNT:
                        usu.d = 0;
                        break;
                    case TEC_ESQUER:
                        usu.d = 1;
                        break;
                    case TEC_AVALL:
                        usu.d = 2;
                        break;
                    case TEC_DRETA:
                        usu.d = 3;
                        break;
                    case TEC_RETURN:
                        waitS(sem_excl);
                          pos_fi[0] = -1;
                        signalS(sem_excl);
                        end = true;
                        break;
                    default:
                        break;
                }
            }

            /* 5) moviment i detecció de col·lisió */
            if (!end) {
                seg.f = usu.f + df[usu.d];
                seg.c = usu.c + dc[usu.d];

                waitS(sem_excl);
                  int c = win_quincar(seg.f, seg.c);
                signalS(sem_excl);

                if (c == ' ') {
                    /* moviment vàlid */
                    waitS(sem_excl);
                      usu.f = seg.f;
                      usu.c = seg.c;
                      win_escricar(usu.f, usu.c, '0', INVERS);
                    signalS(sem_excl);

                    p_usu[n_usu++] = seg;
                    fprintf(log_file,
                            "Usuari: (%d,%d) dir %d\n",
                            usu.f, usu.c, usu.d);
                } else {
                    /* col·lisió */
                    waitS(sem_excl);
                      pos_fi[0] = 1;
                    signalS(sem_excl);

                    esborrar_posicions(p_usu, n_usu);
                    end = true;
                }
            }

            /* 6) actualitzem pantalla i retard */
            if (!end) {
                waitS(sem_excl);
                  win_update();
                signalS(sem_excl);
                win_retard(100);
            }
        }
    }

    /* Només un return per funció */
    return;
}

