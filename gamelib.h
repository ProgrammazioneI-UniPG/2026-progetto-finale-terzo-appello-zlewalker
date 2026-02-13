#ifndef GAMELIB_H
#define GAMELIB_H

typedef enum {
    bosco,
    scuola,
    laboratorio,
    caverna,
    strada,
    giardino,
    supermercato,
    centrale_elettrica,
    deposito_abbandonato,
    stazione_polizia
} Tipo_zona;

typedef enum {
    nessun_nemico,
    billi,
    democane,
    demotorzone
} Tipo_nemico;

typedef enum {
    nessun_oggetto,
    bicicletta,
    maglietta_fuocoinferno,
    bussola,
    schitarrata_metallica
} Tipo_oggetto;


/* ====================
   FORWARD DECLARATIONS
   ==================== */

typedef struct Zona_mondoreale Zona_mondoreale;
typedef struct Zona_soprasotto Zona_soprasotto;
struct Zona_mondoreale;
struct Zona_soprasotto;


/* =========
   STRUTTURE
   =========*/

typedef struct Giocatore {
    char nome[50];
    char nome_originale[50];
    int attacco_pischico;
    int difesa_pischica;
    int fortuna;
    Tipo_oggetto zaino[3];
    int mondo;
    Zona_mondoreale *pos_mondoreale;
    Zona_soprasotto *pos_soprasotto;
} Giocatore;

struct Zona_mondoreale {
    Tipo_zona tipo;
    Tipo_nemico nemico;
    Tipo_oggetto oggetto;
    struct Zona_mondoreale *avanti;
    struct Zona_mondoreale *indietro;
    struct Zona_soprasotto *link_soprasotto;
};

struct Zona_soprasotto {
    Tipo_zona tipo;
    Tipo_nemico nemico;
    struct Zona_soprasotto *avanti;
    struct Zona_soprasotto *indietro;
    struct Zona_mondoreale *link_mondoreale;
};


/* ==================
   FUNZIONI PUBBLICHE
   ================== */

void imposta_gioco(void);
void gioca(void);
void termina_gioco(void);
void crediti(void);

#endif