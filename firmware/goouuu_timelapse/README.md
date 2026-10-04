# Timelapse amb GOOUUU ESP32-S3-CAM

Prototip inicial que desperta la placa, estabilitza la càmera, desa una
fotografia JPEG a la microSD i entra en `deep sleep`. Per defecte repeteix el
cicle cada **5 minuts** i manté el Wi-Fi apagat per poder mesurar primer el
consum base del conjunt.

## On són els fitxers?

Els fitxers formen part d'aquest mateix repositori. Des de l'arrel del projecte
`Cartellera-Digital`, la carpeta és:

```text
Cartellera-Digital/
└── firmware/
    └── goouuu_timelapse/
        ├── README.md
        ├── platformio.ini
        ├── src/
        │   └── main.cpp
        └── arduino/
            └── goouuu_timelapse/
                └── goouuu_timelapse.ino
```

No cal descarregar tots dos formats. Escolliu-ne un:

- **Arduino IDE:** descarregueu la carpeta
  `firmware/goouuu_timelapse/arduino/goouuu_timelapse` sencera. El fitxer que
  s'ha d'obrir és `goouuu_timelapse.ino` i ha de continuar dins d'una carpeta
  que també es digui `goouuu_timelapse`.
- **Visual Studio Code + PlatformIO:** descarregueu la carpeta
  `firmware/goouuu_timelapse` sencera i obriu aquesta carpeta des de VS Code.

### Descarregar-los des de GitHub

Si esteu mirant el repositori o el *pull request* a GitHub, la manera més
senzilla és:

1. Obriu la pàgina principal del repositori.
2. Premeu el botó verd **Code**.
3. Seleccioneu **Download ZIP**.
4. Descomprimiu el ZIP.
5. Entreu a `firmware/goouuu_timelapse` i seguiu una de les dues opcions
   anteriors.

També es pot descarregar només el `.ino`: obriu el fitxer a GitHub, premeu
**Raw** o **Download raw file** i deseu-lo com
`goouuu_timelapse/goouuu_timelapse.ino`. Arduino IDE exigeix que el nom de la
carpeta coincideixi amb el nom principal del sketch.

Aquest entorn de treball no té cap repositori remot configurat i, per tant,
aquest document no pot incloure un enllaç de descàrrega específic. L'enllaç
serà el de la branca o el *pull request* quan els canvis es publiquin al vostre
servidor Git.

## Material

- GOOUUU ESP32-S3-CAM N16R8 amb càmera OV2640 o compatible.
- Targeta microSD formatada com FAT32.
- Cable USB-C per programar-la.
- Multímetre amb mesura de corrent continu.

La implementació utilitza el pinout de càmera d'`ESP32S3_EYE` i el lector
microSD integrat en mode SDMMC d'un bit (`CLK=39`, `CMD=38`, `D0=40`). Com que
hi ha diferents revisions i venedors, cal comparar aquests pins amb la placa
abans d'alimentar-la amb bateria.

## Quin editor es pot utilitzar?

### Visual Studio Code + PlatformIO (recomanat)

El projecte es pot editar amb **Visual Studio Code** (VS Code), que no és el
mateix programa que Visual Studio. Instal·leu l'extensió «PlatformIO IDE» a VS
Code i obriu la carpeta `firmware/goouuu_timelapse`; PlatformIO detectarà
automàticament el fitxer `platformio.ini`. No cal instal·lar PlatformIO Core per
separat quan s'utilitza l'extensió de VS Code.

També és possible utilitzar Visual Studio complet, però per a aquest projecte no
aporta cap avantatge i necessita més configuració.

### Arduino IDE

Sí, també es pot utilitzar **Arduino IDE 2**. La versió preparada per a Arduino
és aquest sketch:

```text
arduino/goouuu_timelapse/goouuu_timelapse.ino
```

El fitxer `.ino` conté el mateix firmware que `src/main.cpp`. Si es modifica la
lògica del programa, cal aplicar el mateix canvi als dos fitxers per mantenir
les dues versions sincronitzades.

## Compilar i carregar amb PlatformIO

Des d'un terminal situat en aquest directori, executeu:

```bash
pio run
pio run --target upload
pio device monitor
```

Si PlatformIO no detecta automàticament el port, es pot indicar amb
`--upload-port /dev/ttyACM0` (adaptant el nom al sistema).

## Compilar i carregar amb Arduino IDE 2

1. Instal·leu Arduino IDE 2.
2. A **Fitxer > Preferències**, afegiu aquest URL a «URLs addicionals del gestor
   de plaques»:

   ```text
   https://espressif.github.io/arduino-esp32/package_esp32_index.json
   ```

3. A **Eines > Placa > Gestor de plaques**, cerqueu `esp32` i instal·leu el
   paquet d'**Espressif Systems**.
4. Obriu `arduino/goouuu_timelapse/goouuu_timelapse.ino`. És important obrir la
   carpeta/sketch complet i no copiar només fragments del programa.
5. Seleccioneu **Eines > Placa > ESP32 Arduino > ESP32S3 Dev Module**.
6. Configureu aquestes opcions, si apareixen a la versió instal·lada del core:
   - **Flash Size:** `16MB`;
   - **PSRAM:** `OPI PSRAM`;
   - **USB CDC On Boot:** `Enabled`;
   - **Upload Mode:** `UART0 / Hardware CDC`;
   - una partició compatible amb 16 MB de flash.
7. Seleccioneu el port sèrie de la placa i premeu **Carrega**. Si no comença la
   càrrega, manteniu premut `BOOT`, premeu i deixeu anar `RESET`, i deixeu anar
   `BOOT` quan Arduino IDE mostri que està connectant.
8. Obriu el monitor sèrie a **115200 bauds**.

Els noms exactes d'algunes opcions poden variar entre versions del core ESP32.
La configuració essencial és seleccionar un ESP32-S3, 16 MB de flash i PSRAM
OPI. Si el port sèrie desapareix quan entra en `deep sleep`, és normal: torneu a
posar la placa en mode de càrrega amb els botons `BOOT` i `RESET`.

Les fotografies queden a:

```text
/timelapse/img_00000001.jpg
/timelapse/img_00000002.jpg
...
```

Per canviar l'interval a 10 minuts, editeu el fitxer que utilitzeu:

- Arduino IDE: `arduino/goouuu_timelapse/goouuu_timelapse.ino`;
- PlatformIO: `src/main.cpp`.

```cpp
constexpr uint64_t CAPTURE_INTERVAL_MINUTES = 10;
```

## Primera prova

1. Inseriu la microSD amb la placa desconnectada.
2. Carregueu el programa i obriu el monitor sèrie a 115200 bauds.
3. Comproveu que apareixen `Foto desada` i `Entrant en deep sleep`.
4. Espereu almenys dos cicles i verifiqueu els JPEG de la microSD.
5. No afegiu encara Wi-Fi ni alimentació solar: primer convé obtenir una
   mesura base repetible.

## Mesurar el consum

El multímetre s'ha de connectar **en sèrie**, mai en paral·lel, entre la font i
la placa. Comenceu sempre per l'escala de corrent més alta i utilitzeu una font
estable. No intenteu mesurar corrent des del connector USB mentre també
alimenteu la placa per un altre pin.

Mesureu separadament:

1. el pic durant l'arrencada i la captura;
2. el consum mentre escriu a la microSD;
3. el valor estable durant el `deep sleep`;
4. el temps despert, que també surt pel monitor sèrie.

Un multímetre convencional pot no registrar bé els pics curts. A més, en
escales de microampers pot introduir prou caiguda de tensió perquè la placa es
reiniciï. Si passa, mesureu els pics a l'escala de mA/A i el repòs a l'escala
de µA en proves separades.

Anoteu la tensió d'alimentació, el corrent actiu aproximat, el corrent de repòs
i els mil·lisegons desperta. Amb aquestes quatre dades es podrà dimensionar la
bateria i la placa solar amb més criteri.

## Wi-Fi

Aquest primer firmware no activa el Wi-Fi. És deliberat: així la primera
mesura separa el consum de càmera/microSD del consum de xarxa. El pas següent
pot afegir una pujada HTTP dels JPEG i comparar dos perfils:

- només microSD;
- microSD més pujada per Wi-Fi en cada captura o en lots.

Enviar les fotos en lots acostuma a reduir el nombre de connexions, però obliga
a mantenir una cua i gestionar els errors de xarxa.
