/******************************************************************************
 *                              tron1.c
 *
 * Versió modificada del joc del Tron per a la Fase 1 amb múltiples oponents.
 * Aquesta versió incorpora:
 *   - Conversió de la variable global 'opo' en un vector (màxim 9 oponents).
 *   - Inicialització del joc per posicionar l'usuari i distribuir equidistant els oponents.
 *   - Modificació de les funcions de moviment per a que s'executin com a processos independents.
 *   - Retard aleatori per als oponents entre 'retard_min' i 'retard_max'.
 *   - Registre de moviments en un fitxer de log sense buffering.
 *
 * Compilar:
 *    $ gcc tron1.c winsuport.o -o tron1 -lcurses
 *
 * Execució:
 *    $ ./tron1 num_oponents variabilitat fitxer [retard_min retard_max]
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/wait.h>
#include "winsuport.h"

/* Definicions constants */
#define MAX_OPONENTS 9

/* Estructures per als objectes (usuari i oponents) */
typedef struct {
    int f;  /* posició actual: fila */
    int c;  /* posició actual: columna */
    int d;  /* direcció actual: [0..3] */
} objecte;

typedef struct {
    int f;
    int c;
} pos;

/* Variables globals */
int n_fil, n_col;     /* dimensions del camp de joc */
objecte usu;          /* informació de l'usuari */
objecte opo[MAX_OPONENTS]; /* vector d'oponents */
int num_oponents;     /* nombre d'oponents (de 1 a MAX_OPONENTS) */

pos *p_usu;           /* taula de posicions recorrudes per l'usuari */
pos *p_opo;           /* taula de posicions recorrudes pels oponents */
int n_usu = 0, n_opo = 0; /* nombre d'entrades en les taules */

/* Arrays per als increments de les 4 direccions (0: amunt, 1: esquerra, 2: avall, 3: dreta) */
int df[] = {-1, 0, 1, 0};
int dc[] = {0, -1, 0, 1};

/* Paràmetres de moviment */
int varia;    /* variabilitat per a l'oponent [0..3] */
int retard;   /* retard de moviment de l'usuari (en ms) */
int retard_min; /* retard mínim per als oponents (en ms) */
int retard_max; /* retard màxim per als oponents (en ms) */

/* Variables globals de finalització */
volatile int fi1 = 0;  /* condició de finalització per l'usuari (-1: tecla RETURN, 1: col·lisió) */
volatile int fi2 = 0;  /* condició de finalització per als oponents (1: col·lisió) */

/* Fitxer de log */
FILE *log_file;

/* Prototips de funcions */
void esborrar_posicions(pos p_pos[], int n_pos);
void inicialitza_joc(void);
void mou_oponent(int index);
void mou_usuari(void);

/* Funció per esborrar el rastre d'un objecte */
void esborrar_posicions(pos p_pos[], int n_pos) {
    int i;
    for(i = n_pos - 1; i >= 0; i--) {
        win_escricar(p_pos[i].f, p_pos[i].c, ' ', NO_INV);
        win_retard(10);
    }
}

/* Inicialitza el joc:
   - Posiciona l'usuari.
   - Distribueix els oponents de forma equidistant verticalment.
*/
void inicialitza_joc(void) {
    char strin[80];
    int i;
    int espai;

    /* Tron de l'usuari */
    usu.f = (n_fil - 1) / 2;
    usu.c = n_col / 4;
    usu.d = 3;
    win_escricar(usu.f, usu.c, '0', INVERS);
    p_usu[n_usu].f = usu.f;
    p_usu[n_usu].c = usu.c;
    n_usu++;

    /* Oponents */
    espai = (n_fil - 1) / (num_oponents + 1);
    for(i = 0; i < num_oponents; i++) {
        opo[i].f = espai * (i + 1);
        opo[i].c = (n_col * 3) / 4;
        opo[i].d = rand() % 4;  /* direcció inicial aleatòria */
        win_escricar(opo[i].f, opo[i].c, '1' + i, INVERS);
        p_opo[n_opo].f = opo[i].f;
        p_opo[n_opo].c = opo[i].c;
        n_opo++;
    }

    sprintf(strin, "Tecles: '%c', '%c', '%c', '%c', RETURN-> sortir\n",
            TEC_AMUNT, TEC_AVALL, TEC_DRETA, TEC_ESQUER);
    win_escristr(strin);
}

/* Funció de moviment dels oponents (executa com a procés independent).
   El paràmetre 'index' indica quin oponent moure.
   Es registra el moviment en el fitxer de log.
   S'introdueix un retard aleatori entre 'retard_min' i 'retard_max'.
*/
void mou_oponent(int index) {
    char car_oponent = '1' + index;
    int delay;
    while (!fi1 && !fi2) {
        char cars;
        objecte seg;
        int k, vk, nd, vd[3];
        int canvi = 0;
        int collision = 0;

        seg.f = opo[index].f + df[opo[index].d];
        seg.c = opo[index].c + dc[opo[index].d];
        cars = win_quincar(seg.f, seg.c);
        if (cars != ' ')
            canvi = 1;
        else if (varia > 0) {
            k = rand() % 10;
            if (k < varia)
                canvi = 1;
        }

        if (canvi) {
            nd = 0;
            for (k = -1; k <= 1; k++) {
                vk = (opo[index].d + k) % 4;
                if (vk < 0)
                    vk += 4;
                seg.f = opo[index].f + df[vk];
                seg.c = opo[index].c + dc[vk];
                cars = win_quincar(seg.f, seg.c);
                if (cars == ' ') {
                    vd[nd] = vk;
                    nd++;
                }
            }
            if (nd == 0)
                collision = 1;
            else {
                if (nd == 1)
                    opo[index].d = vd[0];
                else
                    opo[index].d = vd[rand() % nd];
            }
        }
        if (!collision) {
            opo[index].f += df[opo[index].d];
            opo[index].c += dc[opo[index].d];
            win_escricar(opo[index].f, opo[index].c, car_oponent, INVERS);
            p_opo[n_opo].f = opo[index].f;
            p_opo[n_opo].c = opo[index].c;
            n_opo++;
            fprintf(log_file, "Oponent %d: (%d, %d) direcció %d\n",
                    index, opo[index].f, opo[index].c, opo[index].d);
        } else {
            esborrar_posicions(p_opo, n_opo);
            fi2 = 1;
            break;
        }
        delay = (rand() % (retard_max - retard_min + 1)) + retard_min;
        win_retard(delay);
    }
    exit(0);
}

/* Funció de moviment de l'usuari (executa com a procés independent).
   Escriu moviments al fitxer de log.
*/
void mou_usuari(void) {
    while (!fi1 && !fi2) {
        int tecla = win_gettec();
        if (tecla != 0) {
            switch (tecla) {
                case TEC_AMUNT:  usu.d = 0; break;
                case TEC_ESQUER: usu.d = 1; break;
                case TEC_AVALL:  usu.d = 2; break;
                case TEC_DRETA:  usu.d = 3; break;
                case TEC_RETURN: fi1 = -1; break;
            }
        }
        objecte seg;
        seg.f = usu.f + df[usu.d];
        seg.c = usu.c + dc[usu.d];
        char cars = win_quincar(seg.f, seg.c);
        if (cars == ' ') {
            usu.f = seg.f;
            usu.c = seg.c;
            win_escricar(usu.f, usu.c, '0', INVERS);
            p_usu[n_usu].f = usu.f;
            p_usu[n_usu].c = usu.c;
            n_usu++;
            fprintf(log_file, "Usuari: (%d, %d) direcció %d\n", usu.f, usu.c, usu.d);
        } else {
            esborrar_posicions(p_usu, n_usu);
            fi1 = 1;
            break;
        }
        win_retard(retard);
    }
    exit(0);
}

/* Funció principal.
   Arguments:
      argv[1]: num_oponents (1..MAX_OPONENTS)
      argv[2]: variabilitat (0..3)
      argv[3]: nom del fitxer de log
      [argv[4]: retard_min] [argv[5]: retard_max]
*/
int main(int n_args, char *ll_args[]) {
    int retwin;
    int i;
    pid_t pid;
    int total_children;

    srand(getpid());

    if (n_args != 4 && n_args != 6) {
        fprintf(stderr, "Comanda: ./tron1 num_oponents variabilitat fitxer [retard_min retard_max]\n");
        fprintf(stderr, "         on 'num_oponents' de 1 a %d, 'variabilitat' de 0 a 3,\n", MAX_OPONENTS);
        fprintf(stderr, "         'fitxer' és el nom del fitxer de log,\n");
        fprintf(stderr, "         i 'retard_min' i 'retard_max' són retard (en ms) per als oponents.\n");
        exit(1);
    }

    num_oponents = atoi(ll_args[1]);
    if (num_oponents < 1)
        num_oponents = 1;
    if (num_oponents > MAX_OPONENTS)
        num_oponents = MAX_OPONENTS;

    varia = atoi(ll_args[2]);
    if (varia < 0)
        varia = 0;
    if (varia > 3)
        varia = 3;

    log_file = fopen(ll_args[3], "w");
    if (log_file == NULL) {
        perror("Error obrint el fitxer de log");
        exit(1);
    }
    setvbuf(log_file, NULL, _IONBF, 0);

    if (n_args == 6) {
        retard_min = atoi(ll_args[4]);
        retard_max = atoi(ll_args[5]);
        if (retard_min < 10)
            retard_min = 10;
        if (retard_max > 1000)
            retard_max = 1000;
    } else {
        retard_min = retard = 100;
        retard_max = retard;
    }

    printf("Joc del Tron (Fase 1 amb múltiples oponents)\n\tTecles: '%c', '%c', '%c', '%c', RETURN-> sortir\n",
           TEC_AMUNT, TEC_AVALL, TEC_DRETA, TEC_ESQUER);
    printf("Prem una tecla per continuar:\n");
    getchar();

    n_fil = 0;
    n_col = 0;
    retwin = win_ini(&n_fil, &n_col, '+', INVERS);
    if (retwin < 0) {
        fprintf(stderr, "Error en la creació del taulell de joc:\t");
        switch (retwin) {
            case -1: fprintf(stderr, "camp de joc ja creat!\n"); break;
            case -2: fprintf(stderr, "no s'ha pogut inicialitzar l'entorn de curses!\n"); break;
            case -3: fprintf(stderr, "les mides del camp demanades són massa grans!\n"); break;
            case -4: fprintf(stderr, "no s'ha pogut crear la finestra!\n"); break;
        }
        exit(2);
    }

    p_usu = calloc(n_fil * n_col / 2, sizeof(pos));
    p_opo = calloc(n_fil * n_col / 2, sizeof(pos));
    if (!p_usu || !p_opo) {
        win_fi();
        if (p_usu)
            free(p_usu);
        if (p_opo)
            free(p_opo);
        fprintf(stderr, "Error en l'assignació de memòria dinàmica.\n");
        exit(3);
    }

    inicialitza_joc();

    /* Creació dels processos: un per a l'usuari i un per cada oponent */
    total_children = 1 + num_oponents;
    pid = fork();
    if (pid == 0) {
        mou_usuari();
    } else if (pid < 0) {
        perror("fork");
        exit(1);
    }
    for (i = 0; i < num_oponents; i++) {
        pid = fork();
        if (pid == 0) {
            mou_oponent(i);
        } else if (pid < 0) {
            perror("fork");
            exit(1);
        }
    }

    for (i = 0; i < total_children; i++) {
        wait(NULL);
    }

    win_fi();
    free(p_usu);
    free(p_opo);
    fclose(log_file);

    if (fi1 == -1)
        printf("S'ha aturat el joc amb tecla RETURN!\n\n");
    else {
        if (fi2)
            printf("Ha guanyat l'usuari!\n\n");
        else
            printf("Ha guanyat l'ordinador!\n\n");
    }

    return 0;
}

