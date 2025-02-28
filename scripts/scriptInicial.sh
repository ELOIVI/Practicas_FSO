#!/bin/bash
# Això indica que l'script s'ha d'executar amb l'intèrpret Bash.

# Script per comparar dos directoris
# (verifica que existeixin, mostra fitxers únics de cada directori
# i comprova quins fitxers s'anomenen igual però són diferents.)

# Si el nombre de paràmetres (posicionals) no és exactament 2...
if [ "$#" -ne 2 ]; then

   # ...mostra un missatge d'ús correcte enviant-lo a l'error estàndard (>&2)
   echo "Ús: $0 <directori1> <directori2>" >&2
   
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

# Mostra per pantalla un text per indicar quins fitxers només es troben a DIR1.
echo "Fitxers només a $DIR1:"


# 'comm' compara dues llistes línia a línia i pot mostrar quines línies
# apareixen només a la primera, només a la segona o a totes dues.
# -23 vol dir que NO mostri les línies que apareguin només a la segona (-2)
# ni les comunes (-3), de manera que només es mostren les que són pròpies
# del primer argument.
# Per obtenir aquesta comparació, fem servir dues substitucions de procés (<(...)):
#   <(ls "$DIR1" | sort) i <(ls "$DIR2" | sort)
#   Això llista els fitxers de cada directori i els ordena, perquè 'comm'
#   necessiti llistes ja classificades.
comm -23 <(ls "$DIR1" | sort) <(ls "$DIR2" | sort)

# Ara mostrarem els fitxers que són únics de DIR2.
echo "Fitxers només a $DIR2:"

# De nou fem servir 'comm', però aquesta vegada amb -13,
# per no mostrar els fitxers que apareixen només a la primera llista (-1)
# ni els fitxers comuns (-3).
# Així queden només els que apareixen a la segona llista, és a dir, únics de DIR2.
comm -13 <(ls "$DIR1" | sort) <(ls "$DIR2" | sort)


# Fem un bucle per cadascun dels fitxers que hi ha a DIR1.
# 'ls "$DIR1"' enumera els noms de fitxer, i a cada iteració
# la variable 'file' conté un d'aquests noms.
for file in $(ls "$DIR1"); do

   # Comprova si, amb el mateix nom, existeix un fitxer al directori DIR2.
   # (És a dir, només comparem si hi ha un fitxer a tots dos directoris.)
   if [ -f "$DIR2/$file" ]; then

      
      # 'diff -q' compara els dos fitxers silenciosament (quick).
      # Si són diferents, el codi de sortida és 1, i si són iguals, és 0.
      # El símbol '!' inverteix el resultat de la condició.
      # L'operador '> /dev/null' descarta la sortida de diff,
      # ja que ens interessa només el codi de retorn, no la comparativa detallada.
      if ! diff -q "$DIR1/$file" "$DIR2/$file" > /dev/null; then

         # Si la condició és certa (els fitxers són diferents),
         # mostra per pantalla el nom d'aquest fitxer que difereix.
         echo "Fitxer diferent: $file"
         
      fi
   fi
done

