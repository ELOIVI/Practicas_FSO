#!/bin/bash
# Això indica que l'script s'ha d'executar amb l'intèrpret Bash.

# Script per comparar dos directoris
# (verifica que existeixin, mostra fitxers únics de cada directori
# i comprova quins fitxers s'anomenen igual però són diferents.)

###############################################################################
### NOVA FUNCIONALITAT (Comparació avançada, % de similitud, getopts) ########
###############################################################################
# Amb aquest bloc incorporem opció -i (ignorar espais/linies buides amb 'diff')
# i -s <valor> (percentatge de similitud). A més, definim una funció per
# calcular la similitud entre dos fitxers i mostrar el nom complet dels que
# superin el llindar establert (90% per defecte).
#
# ARA, TAMBÉ:
# - Afegim param. per ignorar algunes extensions (ex: ".tmp,.bak")
# - Afegim param. per ignorar un subdirectori concret (ex: "ignorar")
# Amb això, no compararem ni mostrarem aquests fitxers/subdirectoris.

DIFF_OPTS=""
SIMILARITY_THRESHOLD="90"  # Percentatge per defecte
IGNORE_EXTS=""             # Llista d'extensions a ignorar, separades per comes
IGNORE_SUBDIR=""           # Nom d'un subdirectori a ignorar

# L'ús de getopts permet gestionar múltiples opcions (-i, -s, etc.).
# -e => extensions a ignorar (ex: ".tmp,.bak")
# -d => subdirectori a ignorar (ex: "ignorar")
while getopts ":is:e:d:" opt; do
  case "$opt" in
    i)
      # Si l'usuari demana ignorar espais i línies buides en 'diff'
      DIFF_OPTS="-w -B"
      ;;
    s)
      # Definim un percentatge de similitud diferent de 90
      SIMILARITY_THRESHOLD="$OPTARG"
      ;;
    e)
      # Extensions a ignorar
      IGNORE_EXTS="$OPTARG"
      ;;
    d)
      # Subdirectori a ignorar
      IGNORE_SUBDIR="$OPTARG"
      ;;
    \?)
      echo "Opció no reconeguda: -$OPTARG" >&2
      exit 1
      ;;
    :)
      echo "L'opció -$OPTARG requereix un valor." >&2
      exit 1
      ;;
  esac
done

# Elimina els paràmetres que ja ha processat getopts, deixant-ne només els
# obligatoris (directori1, directori2)
shift $(( OPTIND - 1 ))

###############################################################################
# Si el nombre de paràmetres (posicionals) no és exactament 2...
if [ "$#" -ne 2 ]; then

   # ...mostra un missatge d'ús correcte enviant-lo a l'error estàndard (>&2)
   echo "Ús: $0 [-i] [-s <similitud>] [-e <ext1,ext2,...>] [-d <subdir>] <directori1> <directori2>" >&2
   
   # ...i surt amb un codi d'error (1).
   exit 1
fi

# Assigna el primer paràmetre de l'script a la variable DIR1.
DIR1=$1

# Assigna el segon paràmetre de l'script a la variable DIR2.
DIR2=$2

# Comprova si DIR1 o DIR2 no són directoris existents
if [ ! -d "$DIR1" ] || [ ! -d "$DIR2" ]; then

   # Mostra un avís per sortida d'error
   echo "Un o ambdós directoris no existeixen." >&2
	
   # Surt de l'script amb codi d'error si no es compleix la condició
   exit 1
fi

###############################################################################
### FUNCIONS PER CONSTRUIR LLISTES DE FITXERS TENINT EN COMPTE IGNORAR ########
###############################################################################
# Afegeix en una sola funció la lògica per fer 'find' aplicant filtres:
# - Ignorar subdirectori (via -not -path "*/subdirIgnorar/*")
# - Ignorar extensions (.tmp, .bak, etc.)
# Retornem la comanda (en text) que caldrà executar amb 'eval' per obtenir la llista.
build_find_command() {
  local dir="$1"
  # Iniciem la comanda base
  local cmd="find \"$dir\" -type f"

  # Si s'ha indicat un subdirectori a ignorar, l'afegim al 'find' amb -not -path
  if [ -n "$IGNORE_SUBDIR" ]; then
    cmd="$cmd -not -path \"*/$IGNORE_SUBDIR/*\""
  fi

  # Si hi ha extensions a ignorar, convertim la llista separada per comes en condicions
  # Ex: ".tmp,.bak" => "-not -name '*.tmp' -not -name '*.bak'"
  if [ -n "$IGNORE_EXTS" ]; then
    IFS=',' read -r -a exts_array <<< "$IGNORE_EXTS"
    for ext in "${exts_array[@]}"; do
      # Treu possibles espais en blanc
      local trimmed_ext="$(echo "$ext" | xargs)"
      cmd="$cmd -not -name \"*$trimmed_ext\""
    done
  fi

  echo "$cmd"
}

###############################################################################
### FUNCIÓ PER CALCULAR LA SIMILITUD ENTRE DOS FITXERS (sense modificar) ######
###############################################################################
check_similarity() {
  local f1="$1"
  local f2="$2"

  # Comptem línies no buides
  local lines1=$(grep -vE '^[[:space:]]*$' "$f1" | wc -l)
  local lines2=$(grep -vE '^[[:space:]]*$' "$f2" | wc -l)
  local max_lines=$(( lines1 > lines2 ? lines1 : lines2 ))

  # Comptem línies diferents amb diff -u (usant DIFF_OPTS, ex. -w -B)
  local diff_lines=$(diff -u $DIFF_OPTS "$f1" "$f2" 2>/dev/null | grep '^[+-]' | wc -l)

  # Calculem el % de similitud
  local similarity=$(echo "scale=2; if($max_lines>0) (($max_lines - $diff_lines)/$max_lines)*100 else 0" | bc)

  # Compareu 'similarity' amb 'SIMILARITY_THRESHOLD'
  local compare=$(echo "$similarity >= $SIMILARITY_THRESHOLD" | bc -l)
  if [ "$compare" -eq 1 ]; then
    echo "El fitxer \"$(readlink -f "$f1")\" té una similitud del ${similarity}% amb \"$(readlink -f "$f2")\""
  fi
}

###############################################################################
### COMPARACIÓ DE FITXERS ÚNICS AMB 'comm', ARA USANT NOM RELATIU #############
###############################################################################
# Mostra per pantalla un text per indicar quins fitxers només es troben a DIR1.
echo "Fitxers només a $DIR1:"

# Abans d'usar 'comm', definim dues variables amb les comandes de find:
cmdFindDir1=$(build_find_command "$DIR1")
cmdFindDir2=$(build_find_command "$DIR2")

# 'comm' compara dues llistes línia a línia i pot mostrar quines línies
# apareixen només a la primera, només a la segona o a totes dues.
# -23 vol dir que NO mostri les línies que apareguin només a la segona (-2)
# ni les comunes (-3). Abans usàvem la ruta absoluta, però això feia
# que /dir1/igual1.txt i /dir2/igual1.txt fossin diferents. Ara convertim
# la ruta en "relativa" a DIR1 o DIR2, així comm sap que és el mateix nom.
comm -23 <( eval "$cmdFindDir1" | sed "s|^$DIR1/||" | sort ) \
         <( eval "$cmdFindDir2" | sed "s|^$DIR2/||" | sort )

# Ara mostrarem els fitxers que són únics de DIR2.
echo "Fitxers només a $DIR2:"
# De nou fem servir 'comm', però aquesta vegada amb -13,
# per no mostrar els fitxers que apareixen només a la primera llista (-1)
# ni els fitxers comuns (-3). També convertim a camí relatiu per comparar noms.
comm -13 <( eval "$cmdFindDir1" | sed "s|^$DIR1/||" | sort ) \
         <( eval "$cmdFindDir2" | sed "s|^$DIR2/||" | sort )

###############################################################################
### COMPARACIÓ DETALLADA (FITXERS QUE EXISTEIXEN ALS DOS) #####################
###############################################################################
# Fem un bucle per cadascun dels fitxers que hi ha a DIR1, executant la comanda
# emmagatzemada a 'cmdFindDir1'. Ara, fem "for file in $(eval "$cmdFindDir1")"
# per obtenir la llista. D'aquesta manera no interpretem cada línia com una ordre
# sinó com un camí de fitxer.
for file in $(eval "$cmdFindDir1"); do

   # Obtenim el camí relatiu respecte DIR1, per poder comparar
   # correctament el mateix fitxer dins DIR2.
   relative_path="${file#$DIR1/}"

   # Comprova si, amb el mateix camí relatiu, existeix un fitxer al directori DIR2.
   if [ -f "$DIR2/$relative_path" ]; then

      # 'diff -q' compara els dos fitxers silenciosament (quick).
      # Si són diferents, el codi de sortida és 1, i si són iguals, és 0.
      # El símbol '!' inverteix el resultat de la condició.
      # L'operador '> /dev/null' descarta la sortida de diff,
      # ja que ens interessa només el codi de retorn, no la comparativa detallada.
      if ! diff -q $DIFF_OPTS "$file" "$DIR2/$relative_path" > /dev/null 2>&1; then

         # Si la condició és certa (els fitxers són diferents),
         # mostra per pantalla el nom d'aquest fitxer que difereix.
         echo "Fitxer diferent: $relative_path"

         ########################################################################
         # NOVA FUNCIONALITAT: Mostra el contingut de les línies diferents i
         # crida la funció que calcula la similitud (%). Utilitzem 'diff'
         # sense -q per veure totes les línies que canvien.
         ########################################################################
         echo "----- Diferències ($file) vs. ($DIR2/$relative_path) -----"
         diff $DIFF_OPTS "$file" "$DIR2/$relative_path"

         # Cridem la funció per veure si els fitxers superen el llindar de
         # similitud encara que siguin diferents.
         check_similarity "$file" "$DIR2/$relative_path"

      else
         # Si són iguals, podem cridar igualment la funció per si supera
         # el llindar de similitud (serà 100% en aquest cas).
         check_similarity "$file" "$DIR2/$relative_path"
      fi
   fi
done
