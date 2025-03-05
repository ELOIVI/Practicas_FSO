#!/bin/bash

# Script per crear un joc de proves.

# Netegem els directoris anteriors, si ja existeixen.
# Es fa recursivament per eliminar subdirectoris.

# Eliminem la variable per evitar conflictes
unset BASE_DIR  

# Creem el directori base per al joc de proves
# Veiem la ruta relativa de l'script, és a dir, on s'executa.
SCRIPT_DIR="$(dirname "$0")"

# Agafem el path absolut, i anem nivell enrere
BASE_DIR="$(realpath "$SCRIPT_DIR/..")"

rm -rf "$BASE_DIR/joc_proves"

# Creem l'estructura de directoris dins de joc_proves
mkdir -p "$BASE_DIR/joc_proves/dir1/subdir"
mkdir -p "$BASE_DIR/joc_proves/dir2/subdir"
mkdir -p "$BASE_DIR/joc_proves/dir1/ignorar"
mkdir -p "$BASE_DIR/joc_proves/dir2/ignorar"
mkdir -p "$BASE_DIR/joc_proves/dir1/ignorar2"
mkdir -p "$BASE_DIR/joc_proves/dir2/ignorar2"  

# Fitxers iguals en ambdós directoris
echo "Aquest contingut és exactament igual" > "$BASE_DIR/joc_proves/dir1/igual1.txt"
echo "Aquest contingut és exactament igual" > "$BASE_DIR/joc_proves/dir2/igual1.txt"
echo "Un altre contingut igual" > "$BASE_DIR/joc_proves/dir1/igual2.txt"
echo "Un altre contingut igual" > "$BASE_DIR/joc_proves/dir2/igual2.txt"

# Fitxers amb el mateix nom però diferent contingut
echo "Contingut al directori 1" > "$BASE_DIR/joc_proves/dir1/mateix_nom.txt"
echo "Contingut al directori 2" > "$BASE_DIR/joc_proves/dir2/mateix_nom.txt"

# Fitxers únics a cada directori
echo "Aquest fitxer només existeix a dir1" > "$BASE_DIR/joc_proves/dir1/diferent1.txt" 
echo "Aquest fitxer només existeix a dir2" > "$BASE_DIR/joc_proves/dir2/diferent2.txt"

# Fitxers amb contingut similar però no idèntic (80% similitud)
echo "Línia 1 - Igual" > "$BASE_DIR/joc_proves/dir1/similar1.txt"
echo "Línia 2 - Igual" >> "$BASE_DIR/joc_proves/dir1/similar1.txt"
echo "Línia 3 - Igual" >> "$BASE_DIR/joc_proves/dir1/similar1.txt"
echo "Línia 4 - Només a l'arxiu 1" >> "$BASE_DIR/joc_proves/dir1/similar1.txt"
echo "Línia 5 - Igual" >> "$BASE_DIR/joc_proves/dir1/similar1.txt"

echo "Línia 1 - Igual" > "$BASE_DIR/joc_proves/dir2/similar2.txt"
echo "Línia 2 - Igual" >> "$BASE_DIR/joc_proves/dir2/similar2.txt"
echo "Línia 3 - Igual" >> "$BASE_DIR/joc_proves/dir2/similar2.txt"
echo "Línia 4 - Només a l'arxiu 2" >> "$BASE_DIR/joc_proves/dir2/similar2.txt"
echo "Línia 5 - Igual" >> "$BASE_DIR/joc_proves/dir2/similar2.txt"

# Fitxers amb similitud exactament del 90%
echo -e "Línia 1\nLínia 2\nLínia 3\nLínia 4\nLínia 5\nLínia 6\nLínia 7\nLínia 8\nLínia 9\nLínia 10 - Diferent" > "$BASE_DIR/joc_proves/dir1/similar_90.txt"
echo -e "Línia 1\nLínia 2\nLínia 3\nLínia 4\nLínia 5\nLínia 6\nLínia 7\nLínia 8\nLínia 9\nLínia 10 - Diferent a dir2" > "$BASE_DIR/joc_proves/dir2/similar_90.txt"

# Fitxers amb extensions per provar el filtrat
echo "Contingut temporal" > "$BASE_DIR/joc_proves/dir1/temporal.tmp"
echo "Contingut de backup" > "$BASE_DIR/joc_proves/dir1/backup.bak"
echo "Contingut temporal" > "$BASE_DIR/joc_proves/dir2/temporal.tmp"
echo "Contingut de backup" > "$BASE_DIR/joc_proves/dir2/backup.bak"

# Fitxers en subdirectoris (per provar recursivitat)
echo "Contingut en subdirectori igual" > "$BASE_DIR/joc_proves/dir1/subdir/sub_igual.txt"
echo "Contingut en subdirectori igual" > "$BASE_DIR/joc_proves/dir2/subdir/sub_igual.txt"
echo "Contingut diferent subdir1" > "$BASE_DIR/joc_proves/dir1/subdir/sub_diff.txt"
echo "Contingut diferent subdir2" > "$BASE_DIR/joc_proves/dir2/subdir/sub_diff.txt"

# Fitxers únics en subdirectoris
echo "Únic en subdir1" > "$BASE_DIR/joc_proves/dir1/subdir/unic_sub1.txt"
echo "Únic en subdir2" > "$BASE_DIR/joc_proves/dir2/subdir/unic_sub2.txt"

# Fitxers en directori a ignorar
echo "Aquest directori hauria d'ignorar-se" > "$BASE_DIR/joc_proves/dir1/ignorar/ignorar_file.txt"
echo "Aquest directori hauria d'ignorar-se" > "$BASE_DIR/joc_proves/dir2/ignorar/ignorar_file.txt"
echo "Fitxer a ignorar2" > "$BASE_DIR/joc_proves/dir1/ignorar2/ignorar2_file.txt"  # Afegit per simetria
echo "Fitxer a ignorar2" > "$BASE_DIR/joc_proves/dir2/ignorar2/ignorar2_file.txt"

# Fitxers amb diferents permisos
echo "Fitxer amb permisos diferents" > "$BASE_DIR/joc_proves/dir1/permisos.txt"
echo "Fitxer amb permisos diferents" > "$BASE_DIR/joc_proves/dir2/permisos.txt"
chmod 644 "$BASE_DIR/joc_proves/dir1/permisos.txt"
chmod 755 "$BASE_DIR/joc_proves/dir2/permisos.txt"

# Segon cas de permisos diferents
echo "Executable vs no executable" > "$BASE_DIR/joc_proves/dir1/executable.txt"
echo "Executable vs no executable" > "$BASE_DIR/joc_proves/dir2/executable.txt"
chmod 600 "$BASE_DIR/joc_proves/dir1/executable.txt"
chmod 700 "$BASE_DIR/joc_proves/dir2/executable.txt"

# Fitxer gran per provar redirecció
for i in {1..100}; do echo "Línia $i d'un fitxer gran" >> "$BASE_DIR/joc_proves/dir1/fitxer_gran.txt"; done
for i in {1..100}; do echo "Línia $i d'un fitxer gran" >> "$BASE_DIR/joc_proves/dir2/fitxer_gran.txt"; done

echo "Joc de proves creat correctament"
echo "Pots provar l'script amb els directoris joc_proves/dir1 i joc_proves/dir2"
