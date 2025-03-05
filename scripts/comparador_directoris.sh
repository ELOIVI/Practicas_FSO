#!/bin/bash
# Script per comparar dos directoris, mostrant els fitxers únics
# i els que tenen el mateix nom però són diferents.
# Inclou funcionalitats avançades com la similitud entre fitxers 
# i ignorar determinats fitxers i permisos.

###############################################################################
### CONFIGURACIÓ I VARIABLES GLOBALS #########################################
###############################################################################
DIFF_OPTS="-w -B -u"  # Ignorar espais i línies buides, amb format unificat
SIMILARITY_THRESHOLD="90"  # Percentatge de similitud per defecte
IGNORE_EXTS=""             # Llista d'extensions a ignorar, separades per comes
IGNORE_SUBDIR=""           # Nom d'un subdirectori a ignorar
CHECK_PERMS=false          # Bandera per comprovar permisos
OUTPUT_FILE=""             # Fitxer de sortida
RECURSIVE=false            # Bandera per cerca recursiva

###############################################################################
### FUNCIÓ D'AJUDA PER MOSTRAR ÚS DE L'SCRIPT #################################
###############################################################################
usage() {
    echo "Ús: $0 [-r] [-s <similitud>] [-e <ext1,ext2,...>] [-d <subdir>] [-p] [-o <output>] <directori1> <directori2>" >&2
    echo "  -r: Cerca recursiva en subdirectoris" >&2
    echo "  -s <valor>: Percentatge de similitud (defecte: 90)" >&2
    echo "  -e <ext1,ext2,...>: Extensions a ignorar" >&2
    echo "  -d <subdir>: Subdirectori a ignorar" >&2
    echo "  -p: Comparar permisos" >&2
    echo "  -o <fitxer>: Fitxer de sortida" >&2
    exit 1
}

###############################################################################
### PROCESSAMENT D'ARGUMENTS ##################################################
###############################################################################
while getopts ":rs:e:d:po:" opt; do
    case "$opt" in
        r) RECURSIVE=true ;;
        s) SIMILARITY_THRESHOLD="$OPTARG" ;;
        e) IGNORE_EXTS="$OPTARG" ;;
        d) IGNORE_SUBDIR="$OPTARG" ;;
        p) CHECK_PERMS=true ;;
        o) OUTPUT_FILE="$OPTARG" ;;
        \?) echo "Opció no reconeguda: -$OPTARG" >&2; usage ;;
        :) echo "L'opció -$OPTARG requereix un valor." >&2; usage ;;
    esac
done

shift $((OPTIND - 1))

###############################################################################
### VALIDACIÓ D'ARGUMENTS #####################################################
###############################################################################
if [ "$#" -ne 2 ]; then
    usage
fi

DIR1=$1
DIR2=$2

# Comprova si els directoris existeixen
if [ ! -d "$DIR1" ] || [ ! -d "$DIR2" ]; then
    echo "Un o ambdós directoris no existeixen." >&2  # Aquesta sortida sempre va a pantalla perquè és un error
    echo "" >&2
    exit 1
fi

###############################################################################
### FUNCIONS D'UTILITAT #######################################################
###############################################################################
output() {
    if [ -n "$OUTPUT_FILE" ]; then
        echo "$1" >> "$OUTPUT_FILE"
    else
        echo "$1"
    fi
}

build_find_command() {
    local dir="$1"
    local cmd="find \"$dir\""

    if [ "$RECURSIVE" = false ]; then
        cmd="$cmd -maxdepth 1"
    fi

    cmd="$cmd -type f"

    if [ -n "$IGNORE_SUBDIR" ]; then
        cmd="$cmd -not -path \"*/$IGNORE_SUBDIR/*\""
    fi

    if [ -n "$IGNORE_EXTS" ]; then
        IFS=',' read -r -a exts_array <<< "$IGNORE_EXTS"
        for ext in "${exts_array[@]}"; do
            local trimmed_ext="$(echo "$ext" | xargs)"
            cmd="$cmd -not -name \"*.$trimmed_ext\""
        done
    fi

    echo "$cmd"
}

check_similarity() { 
    local f1="$1"
    local f2="$2"

    local lines1=$(grep -vE '^[[:space:]]*$' "$f1" | wc -l)
    local lines2=$(grep -vE '^[[:space:]]*$' "$f2" | wc -l)
    local max_lines=$(( lines1 > lines2 ? lines1 : lines2 ))

    local diff_lines=$(diff -u $DIFF_OPTS "$f1" "$f2" 2>/dev/null | grep '^[+-]' | wc -l)
    local similarity=$(echo "scale=2; if($max_lines>0) (($max_lines - $diff_lines)/$max_lines)*100 else 0" | bc)

    local compare=$(echo "$similarity >= $SIMILARITY_THRESHOLD" | bc -l)
    if [ "$compare" -eq 1 ]; then
        output "El fitxer \"$(readlink -f "$f1")\" té una similitud del ${similarity}% amb \"$(readlink -f "$f2")\""
    fi
}

comparar_permissos() {
    local fitxer1="$1"
    local fitxer2="$2"
    
    local permis1=$(stat -c "%a" "$fitxer1")
    local permis2=$(stat -c "%a" "$fitxer2")

    if [ "$permis1" != "$permis2" ]; then 
        output "Permisos diferents per $fitxer1 i $fitxer2:"
        output "$fitxer1: $permis1"
        output "$fitxer2: $permis2"
    fi
}

###############################################################################
### COMPARACIÓ DE FITXERS ÚNICS ###############################################
###############################################################################
output ""
output "Fitxers només a $DIR1:"
cmdFindDir1=$(build_find_command "$DIR1")
cmdFindDir2=$(build_find_command "$DIR2")

comm -23 <( eval "$cmdFindDir1" | sed "s|^$DIR1/||" | sort ) \
         <( eval "$cmdFindDir2" | sed "s|^$DIR2/||" | sort ) | while read -r file; do
    output "  - $file"
done

output ""
output "Fitxers només a $DIR2:"
comm -13 <( eval "$cmdFindDir1" | sed "s|^$DIR1/||" | sort ) \
         <( eval "$cmdFindDir2" | sed "s|^$DIR2/||" | sort ) | while read -r file; do
    output "  - $file"
done

###############################################################################
### COMPARACIÓ DETALLADA DE FITXERS ###########################################
###############################################################################
eval "$cmdFindDir1" | while read -r file; do
    relative_path="${file#$DIR1/}"

    if [ -f "$DIR2/$relative_path" ]; then
        if ! diff -q $DIFF_OPTS "$file" "$DIR2/$relative_path" > /dev/null 2>&1; then
            output ""
            output "Fitxer diferent: $relative_path"
            output "----- Diferències ($file) vs ($DIR2/$relative_path) -----"
            diff $DIFF_OPTS "$file" "$DIR2/$relative_path" | while read -r line; do
                output "$line"
            done
        fi

        if [ "$CHECK_PERMS" = true ]; then  
            comparar_permissos "$file" "$DIR2/$relative_path"
            output ""
        fi
    fi
done

###############################################################################
### COMPARACIÓ DE SIMILITUD ENTRE TOTS ELS FITXERS ############################
###############################################################################
output ""
output "Comparació de similitud entre tots els fitxers (>= $SIMILARITY_THRESHOLD%):"
eval "$cmdFindDir1" | while read -r file1; do
    eval "$cmdFindDir2" | while read -r file2; do
        if [ "$file1" != "$file2" ] || [ "${file1#$DIR1/}" != "${file2#$DIR2/}" ]; then
            check_similarity "$file1" "$file2"
        fi
    done
done
