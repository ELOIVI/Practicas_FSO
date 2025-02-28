# Pràctica 1: Comparador de Directoris Avançat

## Descripció
Aquest projecte implementa un script de Bash per comparar directoris de manera avançada, amb diverses funcionalitats com comparació recursiva, detecció de similitud entre fitxers, comprovació de permisos i més.

## Estructura de Fitxers

```
P1-FSO/
├── README.md                    # Aquest fitxer
├── scripts/
│   ├── comparador.sh            # Script principal amb les funcionalitats implementades
│   └── crear_proves.sh          # Script per crear l'entorn de proves
├── joc_proves/                  # Directori amb els jocs de proves
│   ├── dir1/                    # Primer directori de proves
│   │   ├── igual1.txt
│   │   ├── igual2.txt
│   │   ├── diferent1.txt
│   │   ├── mateix_nom.txt
│   │   ├── similar1.txt
│   │   ├── temporal.tmp
│   │   ├── backup.bak
│   │   └── subdir/
│   │       ├── sub_igual.txt
│   │       ├── sub_diff.txt
│   │       └── unic_sub1.txt
│   └── dir2/                    # Segon directori de proves
│       ├── igual1.txt
│       ├── igual2.txt
│       ├── diferent2.txt
│       ├── mateix_nom.txt
│       ├── similar2.txt
│       ├── temporal.tmp
│       ├── backup.bak
│       └── subdir/
│           ├── sub_igual.txt
│           ├── sub_diff.txt
│           └── unic_sub2.txt
└── doc/
    └── documentacio.pdf         # Documentació completa de la pràctica
```

## Instruccions d'ús

### Preparació de l'entorn de proves
Per crear l'entorn de proves, executa:

```bash
chmod +x scripts/crear_proves.sh
./scripts/crear_proves.sh
```

### Execució del comparador
El script principal `comparador.sh` suporta diverses opcions:

```bash
chmod +x scripts/comparador.sh
./scripts/comparador.sh [opcions] <directori1> <directori2>
```

## Opcions disponibles

| Opció | Descripció |
|--------|------------|
| `-h`   | Mostra l'ajuda |
| `-r`   | Activa la comparació recursiva (inclou subdirectoris) |
| `-d`   | Mostra les diferències entre fitxers amb el mateix nom |
| `-s PERCENTATGE` | Busca fitxers amb similitud >= PERCENTATGE (0-100) |
| `-e EXTENSIONS` | Ignora fitxers amb aquestes extensions (separades per comes) |
| `-i DIRECTORI` | Ignora aquest subdirectori en la comparació |
| `-p`   | Comprova també diferències en permisos |
| `-o FITXER` | Guarda la sortida en un fitxer en lloc de mostrar-la per pantalla |

## Exemples d'ús

### Comparació bàsica:
```bash
./scripts/comparador.sh joc_proves/dir1 joc_proves/dir2
```

### Comparació recursiva:
```bash
./scripts/comparador.sh -r joc_proves/dir1 joc_proves/dir2
```

### Mostrar diferències de contingut:
```bash
./scripts/comparador.sh -d joc_proves/dir1 joc_proves/dir2
```

### Buscar fitxers amb similitud >= 80%:
```bash
./scripts/comparador.sh -s 80 joc_proves/dir1 joc_proves/dir2
```

### Ignorar fitxers temporals i de backup:
```bash
./scripts/comparador.sh -e tmp,bak joc_proves/dir1 joc_proves/dir2
```

### Guardar els resultats en un fitxer:
```bash
./scripts/comparador.sh -o resultats.txt joc_proves/dir1 joc_proves/dir2
```

### Combinació de diverses opcions:
```bash
./scripts/comparador.sh -r -d -p -e tmp,bak -o resultats_complets.txt joc_proves/dir1 joc_proves/dir2
```

## Autors

[Álvaro Pérez Caballer i Eloi Viciana Gómez]


