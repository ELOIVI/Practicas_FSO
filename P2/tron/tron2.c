/******************************************************************************
 *                              tron2.c
 *
 * Versió modificada del joc del Tron per a la Fase 2: Memòria compartida i
 * sincronització de processos.
 *
 * Aquesta versió s'ha adaptat a partir de tron1.c:
 *  - Incorpora semàfors (2) per a sincronitzar l'accés a la pantalla, a les taules
 *    globals i al fitxer de log.
 *  - Reserva una zona de memòria compartida amb 2 ints:
 *       pos_fi[0]: estat del jugador (0 = continuar, -1 o 1 = ha xocat o s'ha aturat
 *                amb RETURN)
 *       pos_fi[1]: comptador dels trons CPU vius (inicialment = num_oponents)
 *  - La detecció de col·lisió en els trons (oponent) es manté igual que a tron1.c:
 *    sempre es prova a esquivar si hi ha alguna cel·la buida en una de les 3 direccions
 *    possibles. Si no n'hi ha, llavors el tron xoca i es decrementa el comptador.
 *  - El joc finalitza quan el jugador xoca o quan el comptador de trons CPU arriba a 0.
 *
 * Compilar:
 *    $ gcc tron2.c winsuport.o semafor.o memoria.o -o tron2 -lcurses
 *
 * Execució:
 *    $ ./tron2 num_oponents variabilitat fitxer [retard_min retard_max]
 *****************************************************************************/
//
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/wait.h>
#include <assert.h>
#include "winsuport.h"
#include "semafor.h"
#include "memoria.h"

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

/* Variables globals relacionades amb el joc */
int n_fil, n_col;              /* dimensions del camp de joc */
objecte usu;                   /* informació del tron usuari */
objecte opo[MAX_OPONENTS];     /* vector d'oponents */
int num_oponents;              /* nombre d'oponents (de 1 a MAX_OPONENTS) */

pos *p_usu;                    /* taula de posicions recorregudes pel tron usuari */
pos *p_opo;                    /* taula de posicions recorregudes pels trons oponents */
int n_usu = 0, n_opo = 0;      /* nombre d'entrades en les taules */

/* Arrays per als increments de les 4 direccions (0: amunt, 1: esquerra, 2: avall, 3: dreta) */
int df[] = {-1, 0, 1, 0};
int dc[] = {0, -1, 0, 1};

/* Paràmetres de moviment */
int varia;         /* variabilitat per a l'oponent [0..3] */
int retard;        /* retard de moviment de l'usuari (en ms) */
int retard_min;    /* retard mínim per als oponents (en ms) */
int retard_max;    /* retard màxim per als oponents (en ms) */

/* Variables globals de finalització (compartides via memòria): 
   pos_fi[0]: estat del jugador (0 = continuar, -1 o 1 = xoc)
   pos_fi[1]: comptador dels trons CPU vius (inicialment = num_oponents)
*/
int shm_id_exec;
volatile int *pos_fi;  

/* Fitxer de log */
FILE *log_file;

/* Semàfors per la sincronització:
   sem_screen: protegeix les operacions a la pantalla (winsuport)
   sem_global: protegeix les taules globals, el log i les variables de finalització 
*/
int sem_screen;
int sem_global;

/* Prototips de funcions */
void esborrar_posicions(pos p_pos[], int n_pos);
void inicialitza_joc(void);
void mou_oponent(int index);
void mou_usuari(void);

/* Funció per esborrar el rastre d'un tron */
void esborrar_posicions(pos p_pos[], int n_pos) {
    int i;
    waitS(sem_screen);
    for(i = n_pos - 1; i >= 0; i--) {
        win_escricar(p_pos[i].f, p_pos[i].c, ' ', NO_INV);
        win_retard(10);
    }
    signalS(sem_screen);
}

/* Inicialitza el joc:
   - Posiciona el tron usuari.
   - Distribueix equidistant els trons oponents.
   - Escriu les instruccions al peu de la finestra.
*/
void inicialitza_joc(void) {
    char strin[80];
    int i, espai;

    /* Tron de l'usuari */
    usu.f = (n_fil - 1) / 2;
    usu.c = n_col / 4;
    usu.d = 3;
    
    waitS(sem_screen);
    win_escricar(usu.f, usu.c, '0', INVERS);
    signalS(sem_screen);
    
    waitS(sem_global);
    p_usu[n_usu].f = usu.f;
    p_usu[n_usu].c = usu.c;
    n_usu++;
    signalS(sem_global);

    /* Oponents */
    espai = (n_fil - 1) / (num_oponents + 1);
    for(i = 0; i < num_oponents; i++) {
        opo[i].f = espai * (i + 1);
        opo[i].c = (n_col * 3) / 4;
        opo[i].d = rand() % 4;  /* direcció inicial aleatòria */
        
        waitS(sem_screen);
        win_escricar(opo[i].f, opo[i].c, '1' + i, INVERS);
        signalS(sem_screen);
        
        waitS(sem_global);
        p_opo[n_opo].f = opo[i].f;
        p_opo[n_opo].c = opo[i].c;
        n_opo++;
        signalS(sem_global);
    }

    sprintf(strin, "Tecles: '%c','%c','%c','%c', RETURN-> sortir\n",
            TEC_AMUNT, TEC_AVALL, TEC_DRETA, TEC_ESQUER);
    waitS(sem_screen);
    win_escristr(strin);
    signalS(sem_screen);
}

/* Funció de moviment dels trons oponents (proces fill).
   El paràmetre 'index' indica quin tron oponent moure.
   L'algorisme és el mateix que a tron1.c:
   - Calcula la cel·la següent segons la direcció actual.
   - Si aquesta cel·la no és buida o, forçadament per la variabilitat, es volcanviar,
     es recorre la seqüència de candidates (des de -1 fins a +1) per trobar cel·les lliures.
   - Si hi ha alternatives, es selecciona una; si no n'hi ha, es declara la col·lisió.
   En aquest cas, quan el CPU tron col·lisiona, decrementa el comptador compartit de trons vius.
*/
void mou_oponent(int index) {
    char car_oponent = '1' + index;
    int delay;
    objecte seg;
    int k, vk, nd, vd[3];
    int canvi = 0, collision = 0;
    
    /* Condició: mentre el jugador estigui viu i encara hi hagi almenys un CPU actiu */
    while ( (pos_fi[0] == 0) && (pos_fi[1] > 0) ) {
        canvi = 0;
        collision = 0;
        /* Calcular la següent posició en la direcció actual */
        waitS(sem_screen);
        seg.f = opo[index].f + df[opo[index].d];
        seg.c = opo[index].c + dc[opo[index].d];
        char cars = win_quincar(seg.f, seg.c);
        signalS(sem_screen);
        
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
                waitS(sem_screen);
                seg.f = opo[index].f + df[vk];
                seg.c = opo[index].c + dc[vk];
                cars = win_quincar(seg.f, seg.c);
                signalS(sem_screen);
                if (cars == ' ')
                {
                    vd[nd] = vk;
                    nd++;
                }
            }
            if (nd == 0)
                collision = 1;
            else if (nd == 1)
                opo[index].d = vd[0];
            else
                opo[index].d = vd[rand() % nd];
        }
        
        if (!collision) {
            /* Actualitza posició i dibuixa el nou estat */
            waitS(sem_screen);
            opo[index].f += df[opo[index].d];
            opo[index].c += dc[opo[index].d];
            win_escricar(opo[index].f, opo[index].c, car_oponent, INVERS);
            signalS(sem_screen);
            
            waitS(sem_global);
            p_opo[n_opo].f = opo[index].f;
            p_opo[n_opo].c = opo[index].c;
            n_opo++;
            fprintf(log_file, "Oponent %d: (%d, %d) direcció %d\n",
                    index, opo[index].f, opo[index].c, opo[index].d);
            signalS(sem_global);
        } else {  /* Si no es troben cap alternativa, el tron xoca */
            esborrar_posicions(p_opo, n_opo);
            waitS(sem_global);
            pos_fi[1]--;   /* Decrementa el comptador de CPU actius */
            signalS(sem_global);
            break;
        }
        
        delay = (rand() % (retard_max - retard_min + 1)) + retard_min;
        win_retard(delay);
    }
    exit(0);
}

/* Funció de moviment del tron usuari (proces fill).
   Més o menys igual que a tron1.c; però la condició del bucle és:
   mentre el jugador segueixi viu i encara hi hagi algun CPU actiu.
   En cas de col·lisió, s'esborra el seu rastre i es marca la variable del jugador.
*/
void mou_usuari(void) {
    while ( (pos_fi[0] == 0) && (pos_fi[1] > 0) ) {
        int tecla = win_gettec();
        if (tecla != 0) {
            waitS(sem_global);
            switch (tecla) {
                case TEC_AMUNT:  usu.d = 0; break;
                case TEC_ESQUER: usu.d = 1; break;
                case TEC_AVALL:  usu.d = 2; break;
                case TEC_DRETA:  usu.d = 3; break;
                case TEC_RETURN: pos_fi[0] = -1; break;
            }
            signalS(sem_global);
        }
        objecte seg;
        waitS(sem_screen);
        seg.f = usu.f + df[usu.d];
        seg.c = usu.c + dc[usu.d];
        char cars = win_quincar(seg.f, seg.c);
        signalS(sem_screen);
        
        if (cars == ' ') {
            waitS(sem_screen);
            usu.f = seg.f;
            usu.c = seg.c;
            win_escricar(usu.f, usu.c, '0', INVERS);
            signalS(sem_screen);
            
            waitS(sem_global);
            p_usu[n_usu].f = usu.f;
            p_usu[n_usu].c = usu.c;
            n_usu++;
            fprintf(log_file, "Usuari: (%d, %d) direcció %d\n", usu.f, usu.c, usu.d);
            signalS(sem_global);
        } else {  /* Col·lisió detectada */
            esborrar_posicions(p_usu, n_usu);
            waitS(sem_global);
            pos_fi[0] = 1;  /* Marca que el tron usuari ha xocat */
            signalS(sem_global);
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
        fprintf(stderr, "Comanda: ./tron2 num_oponents variabilitat fitxer [retard_min retard_max]\n");
        fprintf(stderr, "         on 'num_oponents' de 1 a %d, 'variabilitat' de 0 a 3,\n", MAX_OPONENTS);
        fprintf(stderr, "         'fitxer' és el nom del fitxer de log,\n");
        fprintf(stderr, "         i 'retard_min' i 'retard_max' són retard (en ms).\n");
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

    /* Inicialitza els semàfors: un per a la pantalla i un per a les variables globals */
    sem_screen = ini_sem(1);
    sem_global = ini_sem(1);

    /* Creació de la zona de memòria compartida per les variables de finalització */
    /* Ara allotgem 2 ints: pos_fi[0] per al jugador; pos_fi[1] per comptar els trons CPU vius */
    shm_id_exec = ini_mem(2 * sizeof(int));
    pos_fi = map_mem(shm_id_exec);
    pos_fi[0] = 0;          /* 0 = continuar; -1 o 1 = jugador xocat */
    pos_fi[1] = num_oponents; /* inicialment tots els trons CPU estan vius */

    printf("Joc del Tron (Fase 2: amb sincronització)\n\tTecles: '%c','%c','%c','%c', RETURN -> sortir\n",
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

    /* Creació dels processos: un per al tron usuari i un per a cada tron CPU */
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
    /* El procés pare espera que tots els fills finalitzin */
    for (i = 0; i < total_children; i++) {
        wait(NULL);
    }

    win_fi();
    free(p_usu);
    free(p_opo);
    fclose(log_file);

    /* Neteja dels recursos IPC */
    elim_sem(sem_screen);
    elim_sem(sem_global);
    elim_mem(shm_id_exec);

    /* Missatge final:
         - Si el jugador ha xocat (pos_fi[0] != 0): ha guanyat l'ordinador.
         - Si el jugador encara viu però tots els trons CPU han mort (pos_fi[1] == 0): guanya el jugador.
    */
    if (pos_fi[0] != 0)
        printf("Ha guanyat l'ordinador!\n\n");
    else if (pos_fi[1] == 0)
        printf("Ha guanyat l'usuari!\n\n");
    else
        printf("Joc acabat!\n\n");

    return 0;
}

