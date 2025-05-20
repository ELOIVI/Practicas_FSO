#!/bin/bash
# Script per comparar dos directoris, mostrant els fitxers únics
# i els que tenen el mateix nom però són diferents.
# Inclou funcionalitats avançades com la similitud entre fitxers 
# i ignorar determinats fitxers i permisos.

###############################################################################
### CONFIGURACIÓ I VARIABLES GLOBALS #########################################
###############################################################################
DIFF_OPTS="-w -B -u"
# DIFF_OPTS defineix les opcions que fem servir per a la comanda 'diff':
#   -w : ignora canvis en espais en blanc
#   -B : ignora línies completament en blanc
#   -u : mostra el format "unificat" en les diferències

SIMILARITY_THRESHOLD="90"
# Percentatge de similitud per defecte, emprat per la funció check_similarity

IGNORE_EXTS=""
# Llista d'extensions a ignorar, separades per comes (ex: ".tmp,.bak"), que s'usaran a 'build_find_command'

IGNORE_SUBDIR=""
# Nom d'un subdirectori a ignorar (ex: "ignorar"), també s'exclou amb 'build_find_command'

CHECK_PERMS=false
# Indica si cal comparar permisos dels fitxers (paràmetre -p)

OUTPUT_FILE=""
# Si val "" (buit), la sortida va a pantalla; si té un nom de fitxer, la sortida s'hi redirigeix (paràmetre -o)

RECURSIVE=false
# Indica si la comparació s'ha de fer recursivament (paràmetre -r, per entrar dins subdirectoris)

###############################################################################
### FUNCIÓ D'AJUDA PER MOSTRAR ÚS DE L'SCRIPT #################################
###############################################################################
usage() {
    # Mostra un text d'ajuda explicant les opcions i la seva sintaxi
    echo "Ús: $0 [-r] [-s <similitud>] [-e <ext1,ext2,...>] [-d <subdir>] [-p] [-o <output>] <directori1> <directori2>" >&2
    #  -r: indica cerca recursiva
    #  -s: percentatge de similitud que volem per considerar dos fitxers "similars"
    #  -e: llista d'extensions separades per comes a ignorar
    #  -d: subdirectori a ignorar
    #  -p: compara també els permisos
    #  -o: fitxer de sortida on es registrarà tota la informació
    echo "  -r: Cerca recursiva en subdirectoris" >&2
    echo "  -s <valor>: Percentatge de similitud (defecte: 90)" >&2
    echo "  -e <ext1,ext2,...>: Extensions a ignorar" >&2
    echo "  -d <subdir>: Subdirectori a ignorar" >&2
    echo "  -p: Comparar permisos" >&2
    echo "  -o <fitxer>: Fitxer de sortida" >&2
    exit 1
    # Finalitza mostrant que s'ha produït un "error" o que manca info
}

###############################################################################
### PROCESSAMENT D'ARGUMENTS ##################################################
###############################################################################
# Amb getopts capturem les opcions -r, -s, -e, -d, -p i -o
# i les assignem a les variables corresponents.
while getopts ":rs:e:d:po:" opt; do
    case "$opt" in
        r) RECURSIVE=true ;;                       # -r => cerca recursiva
        s) SIMILARITY_THRESHOLD="$OPTARG" ;;        # -s => llindar de similitud
        e) IGNORE_EXTS="$OPTARG" ;;                 # -e => extensions a ignorar
        d) IGNORE_SUBDIR="$OPTARG" ;;               # -d => subdirectori a ignorar
        p) CHECK_PERMS=true ;;                      # -p => compara permisos
        o) OUTPUT_FILE="$OPTARG" ;;                 # -o => fitxer de sortida
        \?) echo "Opció no reconeguda: -$OPTARG" >&2; usage ;;  # Error d'opció
        :) echo "L'opció -$OPTARG requereix un valor." >&2; usage ;; # Falta valor
    esac
done

shift $((OPTIND - 1))
# Elimina els paràmetres ja processats per getopts, de manera que
# només quedin els paràmetres obligatoris (en aquest cas, <dir1> i <dir2>)

###############################################################################
### VALIDACIÓ D'ARGUMENTS #####################################################
###############################################################################
if [ "$#" -ne 2 ]; then
    # Si, després de processar, no hi ha exactament 2 paràmetres,
    # mostrem l'ajuda i sortim.
    usage
fi

DIR1=$1
DIR2=$2
# Agafem els dos directors restants

# Comprova si els directoris existeixen
if [ ! -d "$DIR1" ] || [ ! -d "$DIR2" ]; then
    # Si un o ambdós no són directoris, mostrem error i sortim
    echo "Un o ambdós directoris no existeixen." >&2  # Aquesta sortida va a pantalla (error)
    echo "" >&2
    exit 1
fi

###############################################################################
### FUNCIONS D'UTILITAT #######################################################
###############################################################################

<<<<<<< HEAD
echo -n "" > "$OUTPUT_FILE"

=======
>>>>>>> 097ac1484edb7c68305c984e694c536b86eebcc5
output() {
    # Funció que, en lloc d'usar 'echo' directe, envia la sortida
    # a un fitxer (si OUTPUT_FILE no és buit) o per pantalla
    # (si no s'ha especificat -o).
    if [ -n "$OUTPUT_FILE" ]; then
        echo "$1" >> "$OUTPUT_FILE"
    else
        echo "$1"
    fi
}

build_find_command() {
    # Construeix la comanda 'find' per llistar fitxers
    # segons si és recursiva, si cal ignorar subdirectori,
    # o extensions concretes.
    local dir="$1"
    local cmd="find \"$dir\""

    if [ "$RECURSIVE" = false ]; then
        # Si NO és recursiu, limitem la cerca a -maxdepth 1
        cmd="$cmd -maxdepth 1"
    fi

    cmd="$cmd -type f"
    # Només fitxers regulars

    if [ -n "$IGNORE_SUBDIR" ]; then
        # Si tenim subdirectori a ignorar,
        # indiquem a find que no agafi cap fitxer
        # dins la ruta "*/$IGNORE_SUBDIR/*"
        cmd="$cmd -not -path \"*/$IGNORE_SUBDIR/*\""
    fi

    if [ -n "$IGNORE_EXTS" ]; then
        # Si hi ha extensions a ignorar, convertim la llista separada per comes
        # en múltiples condicions '-not -name "*.<ext>"'
        IFS=',' read -r -a exts_array <<< "$IGNORE_EXTS"
        for ext in "${exts_array[@]}"; do
            local trimmed_ext="$(echo "$ext" | xargs)"
            # xargs elimina espais sobrants al voltant
            cmd="$cmd -not -name \"*.$trimmed_ext\""
        done
    fi

    echo "$cmd"
    # Retornem la cadena amb la comanda 'find' completa
}

check_similarity() { 
    # Compara la semblança entre dos fitxers, calculant un % de similitud
    # i si aquest % >= SIMILARITY_THRESHOLD, mostra un missatge.
    local f1="$1"
    local f2="$2"

    # Comptem el nombre de línies no buides de cada fitxer
    local lines1=$(grep -vE '^[[:space:]]*$' "$f1" | wc -l)
    local lines2=$(grep -vE '^[[:space:]]*$' "$f2" | wc -l)
    local max_lines=$(( lines1 > lines2 ? lines1 : lines2 ))
    # max_lines és el màxim entre lines1 i lines2

    # Fem servir 'diff' per veure quantes línies realment difereixen.
    # grep '^[+-]' filtra les línies que comencen amb + o - al format unificat.
    local diff_lines=$(diff -u $DIFF_OPTS "$f1" "$f2" 2>/dev/null | grep '^[+-]' | wc -l)
    
    # Calculem la similitud com:
    #   ((max_lines - diff_lines) / max_lines) * 100
    local similarity=$(echo "scale=2; if($max_lines>0) (($max_lines - $diff_lines)/$max_lines)*100 else 0" | bc)

    # Compareu la 'similarity' amb el llindar definit (SIMILARITY_THRESHOLD)
    local compare=$(echo "$similarity >= $SIMILARITY_THRESHOLD" | bc -l)
    if [ "$compare" -eq 1 ]; then
        # Si és >=, enviem el missatge
        output "El fitxer \"$(readlink -f "$f1")\" té una similitud del ${similarity}% amb \"$(readlink -f "$f2")\""
    fi
}

comparar_permissos() {
    # Comprova si dos fitxers tenen els mateixos permisos
    # i, si no és així, ho mostra.
    local fitxer1="$1"
    local fitxer2="$2"
    
    # Obtenim els permisos en format octal (ex: 644, 755...)
    local permis1=$(stat -c "%a" "$fitxer1")
    local permis2=$(stat -c "%a" "$fitxer2")

    if [ "$permis1" != "$permis2" ]; then 
        # Si són diferents, ho escrivim a la sortida (fitxer o pantalla)
        output "Permisos diferents per $fitxer1 i $fitxer2:"
        output "$fitxer1: $permis1"
        output "$fitxer2: $permis2"
    fi
}

###############################################################################
### COMPARACIÓ DE FITXERS ÚNICS ###############################################
###############################################################################
# Ara, amb 'comm', trobem els fitxers que només són a DIR1 i a DIR2
# (comparant noms relatius).
output ""
output "Fitxers només a $DIR1:"

cmdFindDir1=$(build_find_command "$DIR1")
cmdFindDir2=$(build_find_command "$DIR2")
# Guardem en dues variables les comandes per trobar fitxers a cada directori

comm -23 <( eval "$cmdFindDir1" | sed "s|^$DIR1/||" | sort ) \
         <( eval "$cmdFindDir2" | sed "s|^$DIR2/||" | sort ) | while read -r file; do
    # 'comm -23' treu només les línies presents a la primera llista i no a la segona
    # - Els <(...) són substitucions de procés; executem 'eval "$cmdFindDir1"'
    #   i ens quedem la seva sortida, transformant la ruta absoluta a una ruta
    #   relativa amb sed "s|^$DIR1/||"
    # - A sort final, comparem les dues llistes i la diferència surt per la canal
    output "  - $file"
done

output ""
output "Fitxers només a $DIR2:"

comm -13 <( eval "$cmdFindDir1" | sed "s|^$DIR1/||" | sort ) \
         <( eval "$cmdFindDir2" | sed "s|^$DIR2/||" | sort ) | while read -r file; do
    # 'comm -13' treu només les línies de la segona llista (no a la primera).
    output "  - $file"
done

###############################################################################
### COMPARACIÓ DETALLADA DE FITXERS ###########################################
###############################################################################
# En aquest bloc, mirem quins fitxers (a DIR1) també existeixen a DIR2,
# i, si són diferents, mostrem la comparació de 'diff'.
eval "$cmdFindDir1" | while read -r file; do
    # Per a cadascun dels fitxers trobats a DIR1
    relative_path="${file#$DIR1/}"
    # El camí relatiu (traient la part /path/dir1/)

    if [ -f "$DIR2/$relative_path" ]; then
        # Si el fitxer també existeix a DIR2 amb el mateix nom relatiu
        if ! diff -q $DIFF_OPTS "$file" "$DIR2/$relative_path" > /dev/null 2>&1; then
            # diff -q : mode ràpid => si ret 1, hi ha diferències
            # '!' => invertim el resultat: entrem aquí si 'diff -q' troba diferències
            output ""
            output "Fitxer diferent: $relative_path"
            output "----- Diferències ($file) vs ($DIR2/$relative_path) -----"
            # Mostrem totes les diferències amb el format complet
            diff $DIFF_OPTS "$file" "$DIR2/$relative_path" | while read -r line; do
                output "$line"
            done
        fi

        # Si s'ha demanat comparar permisos (CHECK_PERMS=true)
        if [ "$CHECK_PERMS" = true ]; then  
            comparar_permissos "$file" "$DIR2/$relative_path"
            output ""
        fi
    fi
done

###############################################################################
### COMPARACIÓ DE SIMILITUD ENTRE TOTS ELS FITXERS ############################
###############################################################################
# Compara la similitud (>= SIMILARITY_THRESHOLD) entre TOTS els fitxers
# de DIR1 i DIR2 (no sols aquells que tinguin el mateix nom).
output ""
output "Comparació de similitud entre tots els fitxers (>= $SIMILARITY_THRESHOLD%):"
eval "$cmdFindDir1" | while read -r file1; do
    eval "$cmdFindDir2" | while read -r file2; do
        # Si no són el mateix fitxer (ruta absoluta) ni el mateix nom relatiu,
        # aleshores intentem comprovar si hi pot haver similitud de contingut.
        # Això permet descobrir, per exemple, si 'similar1.txt' i 'similar2.txt'
        # tenen un 95% de semblança (encara que tinguin noms diferents).
        if [ "$file1" != "$file2" ] || [ "${file1#$DIR1/}" != "${file2#$DIR2/}" ]; then
            check_similarity "$file1" "$file2"
        fi
    done
done
