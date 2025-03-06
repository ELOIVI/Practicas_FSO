# Pràctica 1: Comparador de Directoris Avançat

## Descripció
Aquest projecte implementa un script de Bash per comparar directoris de manera avançada, amb funcionalitats com comparació recursiva, detecció de similitud entre fitxers, comprovació de permisos, ignorar fitxers i subdirectoris, i opcionalment guardar els resultats en un fitxer.
#
## Estructura de Fitxers
#
```
P1-FSO/
├── README.md                    # Aquest fitxer
├── scripts/
│   ├── comparador_directoris.sh # Script principal amb les funcionalitats implementades
│   ├── crear_proves.sh          # Script per crear l'entorn de proves
│   └── scriptInicial.sh         # Versió inicial del script (opcional)
├── joc_proves/                  # Directori amb els jocs de proves
│   ├── dir1/                    # Primer directori de proves
│   │   ├── backup.bak
│   │   ├── diferent1.txt
│   │   ├── executable.txt
│   │   ├── fitxer_gran.txt
│   │   ├── ignorar/
│   │   │   └── ignorar_file.txt
│   │   ├── ignorar2/
│   │   │   └── ignorar2_file.txt
│   │   ├── igual1.txt
│   │   ├── igual2.txt
│   │   ├── mateix_nom.txt
│   │   ├── permisos.txt
│   │   ├── similar1.txt
│   │   ├── similar_90.txt
│   │   ├── subdir/
│   │   │   ├── sub_diff.txt
│   │   │   ├── sub_igual.txt
│   │   │   └── unic_sub1.txt
│   │   └── temporal.tmp
│   ├── dir2/                    # Segon directori de proves
│   │   ├── backup.bak
│   │   ├── diferent2.txt
│   │   ├── executable.txt
│   │   ├── fitxer_gran.txt
│   │   ├── ignorar/
│   │   │   └── ignorar_file.txt
│   │   ├── ignorar2/
│   │   │   └── ignorar2_file.txt
│   │   ├── igual1.txt
│   │   ├── igual2.txt
│   │   ├── mateix_nom.txt
│   │   ├── permisos.txt
│   │   ├── similar2.txt
│   │   ├── similar_90.txt
│   │   ├── subdir/
│   │   │   ├── sub_diff.txt
│   │   │   ├── sub_igual.txt
│   │   │   └── unic_sub2.txt
│   │   └── temporal.tmp
│   └── resultats.txt            # Exemple de fitxer de sortida generat
├── resultats.txt                # Exemple de fitxer de sortida generat fora de joc_proves
└── doc/
    └── documentacio.pdf         # Documentació completa de la pràctica (si existeix)
```
#
## Instruccions d'ús
#
### Preparació de l'entorn de proves
Per crear l'entorn de proves, executa:
#
```bash
chmod +x scripts/crear_proves.sh
./scripts/crear_proves.sh
```
#
Això genera els directoris `joc_proves/dir1` i `joc_proves/dir2` amb fitxers de prova.
#
### Execució del comparador
El script principal `comparador_directoris.sh` suporta diverses opcions:
#
```bash
chmod +x scripts/comparador_directoris.sh
./scripts/comparador_directoris.sh [opcions] <directori1> <directori2>
```
#
## Opcions disponibles
#
| Opció            | Descripció                                                                 |
|------------------|---------------------------------------------------------------------------|
| `-r`            | Activa la comparació recursiva (inclou subdirectoris)                     |
| `-s PERCENTATGE`| Busca fitxers amb similitud >= PERCENTATGE (0-100, defecte: 90)           |
| `-e EXTENSIONS` | Ignora fitxers amb aquestes extensions (separades per comes, ex: tmp,bak) |
| `-d DIRECTORI`  | Ignora aquest subdirectori en la comparació (ex: ignorar)                 |
| `-p`            | Comprova també diferències en permisos                                    |
| `-o FITXER`     | Guarda la sortida en un fitxer en lloc de mostrar-la per pantalla         |
#
**Nota**: No hi ha opció `-h` explícita per ajuda; en lloc d'això, es mostra l'ús amb errors d'arguments.
#
## Exemples d'ús
#
### Comparació bàsica (només nivell superior):
```bash
./scripts/comparador_directoris.sh joc_proves/dir1 joc_proves/dir2
```
#
### Comparació recursiva:
```bash
./scripts/comparador_directoris.sh -r joc_proves/dir1 joc_proves/dir2
```
#
### Comparació amb permisos:
```bash
./scripts/comparador_directoris.sh -r -p joc_proves/dir1 joc_proves/dir2
```
#
### Buscar fitxers amb similitud >= 80%:
```bash
./scripts/comparador_directoris.sh -r -s 80 joc_proves/dir1 joc_proves/dir2
```
#
### Ignorar fitxers temporals i de backup:
```bash
./scripts/comparador_directoris.sh -r -e tmp,bak joc_proves/dir1 joc_proves/dir2
```
#
### Ignorar el subdirectori "ignorar":
```bash
./scripts/comparador_directoris.sh -r -d ignorar joc_proves/dir1 joc_proves/dir2
```
#
### Guardar els resultats en un fitxer:
```bash
./scripts/comparador_directoris.sh -r -o resultats.txt joc_proves/dir1 joc_proves/dir2
```
#
### Combinació de diverses opcions:
```bash
./scripts/comparador_directoris.sh -r -p -e tmp,bak -d ignorar -s 75 -o resultats_complets.txt joc_proves/dir1 joc_proves/dir2
```
#
## Autors
- Álvaro Pérez Caballer
- Eloi Viciana Gómez


