# Calibrazione online e real-time dell'accelerometro con RBF su ISPU

Questo progetto implementa la **calibrazione online e in tempo reale dell'accelerometro** utilizzando una **rete neurale RBF (Radial Basis Function)** che **non usa backpropagation**.

L'obiettivo è correggere dinamicamente le misure dell'accelerometro direttamente sulla **ISPU**, eseguendo inferenza e, quando previsto, aggiornamento online del modello.

---

## Struttura del progetto

Il progetto è composto da **due cartelle principali**:

### 1. `read_accelerometer_ISPU`
Questa cartella contiene il progetto ISPU.

All'interno si trova:

- `ISPU/`
  - `src/`
    - `main.c`

Il file **`main.c`** è il file principale da modificare per cambiare il comportamento della ISPU, cioè:

- lettura dei dati dell'accelerometro
- preprocessamento degli input
- inferenza della rete RBF
- eventuale fase di learning
- scrittura degli output nei registri ISPU

---

### 2. `nucleo_ISPU_calib_acc`
Questa cartella contiene il progetto STM32 per la scheda Nucleo.

Qui viene eseguito il codice lato host, che:

- inizializza il sensore
- carica il programma ISPU
- legge gli output della ISPU
- stampa i risultati a terminale

---

## Workflow di utilizzo

### Step 1 — Modifica del codice ISPU

Aprire il file:

read_accelerometer_ISPU/ISPU/src/main.c

### Step 2 — Build del progetto ISPU
Dopo aver modificato main.c, entrare nella cartella:
read_accelerometer_ISPU/ISPU/make
e lanciare il comando:
make
La build va eseguita usando MSYS2.
Al termine della compilazione verranno generati i file di output nella cartella:
read_accelerometer_ISPU/ISPU/make/bin
Tra questi, il file più importante è:
ispu.h

### Step 3 — Copia del file ispu.h
Il file:
read_accelerometer_ISPU/ISPU/make/bin/ispu.h
deve essere copiato dentro il progetto Nucleo, nel seguente percorso:
nucleo_ISPU_calib_acc/Core/Inc/

### Step 4 — Apertura del progetto STM32
Una volta copiato ispu.h, aprire la cartella del progetto Nucleo con STM32CubeIDE.
A questo punto è possibile:


fare build del progetto


eseguire il flash sulla scheda Nucleo


avviare il programma


Il firmware sulla Nucleo caricherà quindi il programma sulla ISPU e ne leggerà gli output.

