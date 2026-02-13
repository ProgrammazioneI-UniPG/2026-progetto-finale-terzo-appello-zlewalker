#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gamelib.h"

#define SAVE_WINNERS_FILE "ultimi_vincitori.txt"
#define MAX_LINE 512

static char* my_strdup(const char *s) {
    size_t len = strlen(s) + 1;
    char *p = malloc(len);
    if (p) memcpy(p, s, len);
    return p;
}

/* ===== PROTOTIPI STATICI ====== */
static void prepara_round(void);
static Giocatore* prossimo_giocatore_round(void);
static void pulisci_buffer(void);
static int leggi_intero_range(const char *prompt, int min, int max);
static void leggi_stringa(const char *prompt, char *dest, size_t maxlen);
static int dado20(void);

static void libera_giocatori(void);
static void libera_mappe(void);
static void reset_partita(void);

static void menu_creazione_mappa(void);
static void avanza(Giocatore *g);
static void indietreggia(Giocatore *g);
static void cambia_mondo(Giocatore *g);
static int giocatori_vivi(void);
static void stampa_zona_corrente(Giocatore *g);
static void stampa_giocatore(Giocatore *g);
static int combatti(Giocatore *g);
static int utilizza_oggetto(Giocatore *g, int *bonus_att, int *bonus_def, int *bonus_fort);
static Tipo_nemico nemico_corrente(Giocatore *g);
static void rimuovi_nemico_zona(Giocatore *g);
static void elimina_giocatore(Giocatore *g);
static void raccogli_oggetto(Giocatore *g);
static int primo_slot_libero(Giocatore *g);


/* ==================
   VARIABILI GLOBALI
   ================== */

static int demotorzone_sconfitto = 0;
static int ordine_round[4];
static int num_round = 0;
static int idx_round = 0;

static Zona_mondoreale *prima_zona_mondoreale = NULL;
static Zona_soprasotto *prima_zona_soprasotto = NULL;

static Giocatore *giocatori[4] = { NULL, NULL, NULL, NULL };

static int gioco_impostato = 0;      /* 1 quando setup completato */
static int mappa_chiusa = 0;         /* 1 quando chiudi_mappa() */
static int undici_gia_scelta = 0;    /* 1 se qualcuno è UndiciVirgolaCinque */

/* ==============
   UTILITY MAPPA
   ==============*/

static const char* nome_zona(Tipo_zona t) {
    switch (t) {
        case bosco: return "bosco";
        case scuola: return "scuola";
        case laboratorio: return "laboratorio";
        case caverna: return "caverna";
        case strada: return "strada";
        case giardino: return "giardino";
        case supermercato: return "supermercato";
        case centrale_elettrica: return "centrale_elettrica";
        case deposito_abbandonato: return "deposito_abbandonato";
        case stazione_polizia: return "stazione_polizia";
        default: return "sconosciuta";
    }
}

static const char* nome_nemico(Tipo_nemico n) {
    switch (n) {
        case nessun_nemico: return "nessun_nemico";
        case billi: return "billi";
        case democane: return "democane";
        case demotorzone: return "demotorzone";
        default: return "sconosciuto";
    }
}

static const char* nome_oggetto(Tipo_oggetto o) {
    switch (o) {
        case nessun_oggetto: return "nessun_oggetto";
        case bicicletta: return "bicicletta";
        case maglietta_fuocoinferno: return "maglietta_fuocoinferno";
        case bussola: return "bussola";
        case schitarrata_metallica: return "schitarrata_metallica";
        default: return "sconosciuto";
    }
}

static int conta_zone_mondoreale(void) {
    int count = 0;
    Zona_mondoreale *cur = prima_zona_mondoreale;
    while (cur) { count++; cur = cur->avanti; }
    return count;
}

static Zona_mondoreale* zona_mondoreale_in_pos(int pos) {
    int i = 0;
    Zona_mondoreale *cur = prima_zona_mondoreale;
    while (cur && i < pos) { cur = cur->avanti; i++; }
    return cur;
}

static Zona_soprasotto* zona_soprasotto_in_pos(int pos) {
    int i = 0;
    Zona_soprasotto *cur = prima_zona_soprasotto;
    while (cur && i < pos) { cur = cur->avanti; i++; }
    return cur;
}

static Tipo_nemico leggi_nemico_mondoreale(void) {
    printf("Nemico Mondo Reale:\n");
    printf("0) nessun_nemico\n1) billi\n2) democane\n");
    int x = leggi_intero_range("Scelta: ", 0, 2);
    return (Tipo_nemico)x;
}

static Tipo_nemico leggi_nemico_soprasotto(void) {
    printf("Nemico Soprasotto:\n");
    printf("0) nessun_nemico\n2) democane\n3) demotorzone\n");
    int x;
    while (1) {
        x = leggi_intero_range("Scelta: ", 0, 3);
        if (x == 1) { /* billi non ammesso */
            printf("billi non e' presente nel Soprasotto. Riprova.\n");
            continue;
        }
        return (Tipo_nemico)x;
    }
}

static Tipo_oggetto leggi_oggetto_mondoreale(void) {
    printf("Oggetto (solo Mondo Reale):\n");
    printf("0) nessun_oggetto\n1) bicicletta\n2) maglietta_fuocoinferno\n3) bussola\n4) schitarrata_metallica\n");
    int x = leggi_intero_range("Scelta: ", 0, 4);
    return (Tipo_oggetto)x;
}

static Tipo_zona leggi_tipo_zona(void) {
    printf("Tipo zona:\n");
    printf("0) bosco\n1) scuola\n2) laboratorio\n3) caverna\n4) strada\n5) giardino\n6) supermercato\n7) centrale_elettrica\n8) deposito_abbandonato\n9) stazione_polizia\n");
    int x = leggi_intero_range("Scelta: ", 0, 9);
    return (Tipo_zona)x;
}

static Tipo_zona tipo_zona_random(void) {
    return (Tipo_zona)(rand() % 10);
}

static Tipo_oggetto oggetto_random(void) {
    int r = rand() % 100;
    if (r < 55) return nessun_oggetto;
    if (r < 70) return bicicletta;
    if (r < 80) return bussola;
    if (r < 90) return maglietta_fuocoinferno;
    return schitarrata_metallica;
}

static Tipo_nemico nemico_random_mondoreale(void) {
    int r = rand() % 100;
    if (r < 45) return nessun_nemico;
    if (r < 65) return democane;
    return billi;
}

static Tipo_nemico nemico_random_soprasotto(int lascia_spazio_demotorzone) {
    int r = rand() % 100;
    if (r < 55) return nessun_nemico;
    return democane;
}

/* ==========
   FREE MAPPE
   ========== */

static void libera_mappe(void) {
    /* libera lista mondoreale */
    Zona_mondoreale *cm = prima_zona_mondoreale;
    while (cm) {
        Zona_mondoreale *next = cm->avanti;
        free(cm);
        cm = next;
    }
    prima_zona_mondoreale = NULL;

    /* libera lista soprasotto */
    Zona_soprasotto *cs = prima_zona_soprasotto;
    while (cs) {
        Zona_soprasotto *next = cs->avanti;
        free(cs);
        cs = next;
    }
    prima_zona_soprasotto = NULL;
}

/* ==============
   CREAZIONE NODO
   ============== */

static Zona_mondoreale* crea_nodo_mondoreale(Tipo_zona t, Tipo_nemico n, Tipo_oggetto o) {
    Zona_mondoreale *z = malloc(sizeof(Zona_mondoreale));
    if (!z) { printf("Errore allocazione zona mondoreale.\n"); exit(1); }
    z->tipo = t;
    z->nemico = n;
    z->oggetto = o;
    z->avanti = NULL;
    z->indietro = NULL;
    z->link_soprasotto = NULL;
    return z;
}

static Zona_soprasotto* crea_nodo_soprasotto(Tipo_zona t, Tipo_nemico n) {
    Zona_soprasotto *z = malloc(sizeof(Zona_soprasotto));
    if (!z) { printf("Errore allocazione zona soprasotto.\n"); exit(1); }
    z->tipo = t;
    z->nemico = n;
    z->avanti = NULL;
    z->indietro = NULL;
    z->link_mondoreale = NULL;
    return z;
}

/* ======================================================
   genera_mappa 15 zone random + 1 demotorzone garantito
   ====================================================== */

static void genera_mappa(void) {
    libera_mappe();
    mappa_chiusa = 0;

    Zona_mondoreale *tail_m = NULL;
    Zona_soprasotto *tail_s = NULL;

    int pos_demo = rand() % 15;

    for (int i = 0; i < 15; i++) {
        Tipo_zona t = tipo_zona_random();

        Tipo_nemico nm = nemico_random_mondoreale();
        Tipo_oggetto om = oggetto_random();

        Tipo_nemico ns;
        if (i == pos_demo) ns = demotorzone;
        else ns = nemico_random_soprasotto(1);

        Zona_mondoreale *zm = crea_nodo_mondoreale(t, nm, om);
        Zona_soprasotto *zs = crea_nodo_soprasotto(t, ns);

        /* link speculare */
        zm->link_soprasotto = zs;
        zs->link_mondoreale = zm;

        /* append in lista mondoreale */
        if (!prima_zona_mondoreale) {
            prima_zona_mondoreale = zm;
            tail_m = zm;
        } else {
            tail_m->avanti = zm;
            zm->indietro = tail_m;
            tail_m = zm;
        }

        /* append in lista soprasotto */
        if (!prima_zona_soprasotto) {
            prima_zona_soprasotto = zs;
            tail_s = zs;
        } else {
            tail_s->avanti = zs;
            zs->indietro = tail_s;
            tail_s = zs;
        }
    }

    printf("\nMappa generata: 15 zone per Mondo Reale e 15 per Soprasotto.\n");
}

/* ==========================================
   inserisci_zona():
   scelte da tastiera per tipo/nemico/oggetto
   ==========================================*/

static void inserisci_zona(void) {
    int len = conta_zone_mondoreale();
    int pos = leggi_intero_range("Inserisci posizione (0...) dove inserire: ", 0, len);

    printf("\n-- Configura la NUOVA zona (Mondo Reale) --\n");
    Tipo_zona t = leggi_tipo_zona();
    Tipo_nemico nm = leggi_nemico_mondoreale();
    Tipo_oggetto om = leggi_oggetto_mondoreale();

    printf("\n-- Configura la NUOVA zona (Soprasotto) --\n");
    printf("Tipo zona: %s\n", nome_zona(t));
    Tipo_nemico ns = leggi_nemico_soprasotto();

    /* nodi */
    Zona_mondoreale *zm = crea_nodo_mondoreale(t, nm, om);
    Zona_soprasotto *zs = crea_nodo_soprasotto(t, ns);
    zm->link_soprasotto = zs;
    zs->link_mondoreale = zm;

    /* inserimento in lista mondoreale */
    if (pos == 0) {
        zm->avanti = prima_zona_mondoreale;
        if (prima_zona_mondoreale) prima_zona_mondoreale->indietro = zm;
        prima_zona_mondoreale = zm;
    } else {
        Zona_mondoreale *prev = zona_mondoreale_in_pos(pos - 1);
        Zona_mondoreale *next = prev ? prev->avanti : NULL;

        prev->avanti = zm;
        zm->indietro = prev;
        zm->avanti = next;
        if (next) next->indietro = zm;
    }

    /* inserimento in lista soprasotto */
    if (pos == 0) {
        zs->avanti = prima_zona_soprasotto;
        if (prima_zona_soprasotto) prima_zona_soprasotto->indietro = zs;
        prima_zona_soprasotto = zs;
    } else {
        Zona_soprasotto *prev = zona_soprasotto_in_pos(pos - 1);
        Zona_soprasotto *next = prev ? prev->avanti : NULL;

        prev->avanti = zs;
        zs->indietro = prev;
        zs->avanti = next;
        if (next) next->indietro = zs;
    }

    printf("\nZona inserita in posizione %d (in entrambi i mondi).\n", pos);
    mappa_chiusa = 0;
}

static void cancella_zona(void) {
    int len = conta_zone_mondoreale();
    if (len == 0) {
        printf("Mappa vuota.\n");
        return;
    }

    int pos = leggi_intero_range("Inserisci posizione (0..len-1) da cancellare: ", 0, len - 1);

    Zona_mondoreale *zm = zona_mondoreale_in_pos(pos);
    if (!zm) return;

    if (zm->indietro) zm->indietro->avanti = zm->avanti;
    else prima_zona_mondoreale = zm->avanti;

    if (zm->avanti) zm->avanti->indietro = zm->indietro;

    Zona_soprasotto *zs = zona_soprasotto_in_pos(pos);
    if (!zs) return;

    if (zs->indietro) zs->indietro->avanti = zs->avanti;
    else prima_zona_soprasotto = zs->avanti;

    if (zs->avanti) zs->avanti->indietro = zs->indietro;

    free(zm);
    free(zs);

    printf("Zona in posizione %d cancellata (in entrambi i mondi).\n", pos);
    mappa_chiusa = 0;
}

static void stampa_mappa(void) {
    printf("\nQuale mappa vuoi stampare?\n1) Mondo Reale\n2) Soprasotto\n");
    int scelta = leggi_intero_range("Scelta: ", 1, 2);

    if (scelta == 1) {
        Zona_mondoreale *cur = prima_zona_mondoreale;
        int i = 0;
        while (cur) {
            printf("[%d] tipo=%s nemico=%s oggetto=%s\n",
                   i, nome_zona(cur->tipo), nome_nemico(cur->nemico), nome_oggetto(cur->oggetto));
            cur = cur->avanti;
            i++;
        }
    } else {
        Zona_soprasotto *cur = prima_zona_soprasotto;
        int i = 0;
        while (cur) {
            printf("[%d] tipo=%s nemico=%s\n",
                   i, nome_zona(cur->tipo), nome_nemico(cur->nemico));
            cur = cur->avanti;
            i++;
        }
    }
}

static void stampa_zona_singola(void) {
    int len = conta_zone_mondoreale();
    if (len == 0) {
        printf("Mappa vuota.\n");
        return;
    }
    int pos = leggi_intero_range("Inserisci posizione (0..len-1): ", 0, len - 1);

    Zona_mondoreale *zm = zona_mondoreale_in_pos(pos);
    Zona_soprasotto *zs = zona_soprasotto_in_pos(pos);

    if (zm) {
        printf("\n[Mondo Reale - %d] tipo=%s nemico=%s oggetto=%s\n",
               pos, nome_zona(zm->tipo), nome_nemico(zm->nemico), nome_oggetto(zm->oggetto));
    }
    if (zs) {
        printf("[Soprasotto - %d] tipo=%s nemico=%s\n",
               pos, nome_zona(zs->tipo), nome_nemico(zs->nemico));
    }
}

static int conta_demotorzone_soprasotto(void) {
    int c = 0;
    Zona_soprasotto *cur = prima_zona_soprasotto;
    while (cur) {
        if (cur->nemico == demotorzone) c++;
        cur = cur->avanti;
    }
    return c;
}

static void chiudi_mappa(void) {
    int len = conta_zone_mondoreale();
    if (len < 15) {
        printf("Non puoi chiudere la mappa: ci sono meno di 15 zone (%d).\n", len);
        return;
    }
    int demo = conta_demotorzone_soprasotto();
    if (demo != 1) {
        printf("Non puoi chiudere la mappa: demotorzone nel Soprasotto deve essere esattamente 1 (attuale: %d).\n", demo);
        return;
    }

    mappa_chiusa = 1;
    gioco_impostato = 1;

    printf("\nMappa chiusa correttamente! Ora puoi giocare.\n");
}

/* ====================
   MENU CREAZIONE MAPPA
   ==================== */

static void modifica_zona(void) {
    int len = conta_zone_mondoreale();
    if (len == 0) {
        printf("Mappa vuota.\n");
        return;
    }

    int pos = leggi_intero_range("Inserisci posizione (0..len-1) da modificare: ", 0, len - 1);

    Zona_mondoreale *zm = zona_mondoreale_in_pos(pos);
    Zona_soprasotto  *zs = zona_soprasotto_in_pos(pos);

    if (!zm || !zs) {
        printf("Errore: zona non trovata.\n");
        return;
    }

    printf("\nZona attuale [%d]\n", pos);
    printf("MR: tipo=%s nemico=%s oggetto=%s\n", nome_zona(zm->tipo), nome_nemico(zm->nemico), nome_oggetto(zm->oggetto));
    printf("SS: tipo=%s nemico=%s\n", nome_zona(zs->tipo), nome_nemico(zs->nemico));

    printf("\n--- Nuovi valori ---\n");
    printf("Scegli nuovo TIPO (vale per entrambi i mondi):\n");
    Tipo_zona nuovo_tipo = leggi_tipo_zona();

    printf("\nScegli nuovo NEMICO per il Mondo Reale:\n");
    Tipo_nemico nuovo_nemico_mr = leggi_nemico_mondoreale();

    printf("\nScegli nuovo OGGETTO per il Mondo Reale:\n");
    Tipo_oggetto nuovo_oggetto_mr = leggi_oggetto_mondoreale();

    printf("\nScegli nuovo NEMICO per il Soprasotto:\n");
    Tipo_nemico nuovo_nemico_ss = leggi_nemico_soprasotto();

    zm->tipo = nuovo_tipo;
    zm->nemico = nuovo_nemico_mr;
    zm->oggetto = nuovo_oggetto_mr;

    zs->tipo = nuovo_tipo;
    zs->nemico = nuovo_nemico_ss;

    printf("\nZona [%d] modificata con successo.\n", pos);
    mappa_chiusa = 0; /*Richiedo la chiusura se modifichi la mappa*/
}


static void menu_creazione_mappa(void) {
    int scelta;
    do {
        printf("\n=== CREAZIONE MAPPA ===\n");
        printf("1) genera_mappa() (15 zone random)\n");
        printf("2) inserisci_zona() (scegli i campi)\n");
        printf("3) cancella_zona()\n");
        printf("4) modifica_zona() (cambia i campi della zona i)\n");
        printf("5) stampa_mappa()\n");
        printf("6) stampa_zona() (posizione i in entrambi i mondi)\n");
        printf("7) chiudi_mappa()\n");
        printf("Scelta: ");

        scelta = leggi_intero_range("", 1, 7);

        switch (scelta) {
            case 1: genera_mappa(); break;
            case 2: inserisci_zona(); break;
            case 3: cancella_zona(); break;
            case 4: modifica_zona(); break;
            case 5: stampa_mappa(); break;
            case 6: stampa_zona_singola(); break;
            case 7: chiudi_mappa(); break;
        }

    } while (!mappa_chiusa);
}



/* ==============
   FUNZIONI VARIE
   ==============*/

static void pulisci_buffer(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) {}
}

static int leggi_intero_range(const char *prompt, int min, int max) {
    int x;
    while (1) {
        printf("%s", prompt);
        if (scanf("%d", &x) != 1) {
            pulisci_buffer();
            printf("Input non valido. Inserisci un numero.\n");
            continue;
        }
        pulisci_buffer();
        if (x < min || x > max) {
            printf("Valore fuori range (%d-%d). Riprova.\n", min, max);
            continue;
        }
        return x;
    }
}

static void leggi_stringa(const char *prompt, char *dest, size_t maxlen) {
    printf("%s", prompt);
    if (fgets(dest, (int)maxlen, stdin) == NULL) {
        dest[0] = '\0';
        return;
    }
    size_t len = strlen(dest);
    if (len > 0 && dest[len - 1] == '\n') dest[len - 1] = '\0';
    if (dest[0] == '\0') {
        strncpy(dest, "Giocatore", maxlen);
        dest[maxlen - 1] = '\0';
    }
}

static int dado20(void) {
    return (rand() % 20) + 1;
}

static void svuota_zaino(Giocatore *g) {
    for (int i = 0; i < 3; i++) g->zaino[i] = nessun_oggetto;
}

static void libera_giocatori(void) {
    for (int i = 0; i < 4; i++) {
        if (giocatori[i] != NULL) {
            free(giocatori[i]);
            giocatori[i] = NULL;
        }
    }
}

static void reset_partita(void) {
    libera_giocatori();
    libera_mappe();
    gioco_impostato = 0;
    mappa_chiusa = 0;
    undici_gia_scelta = 0;
    demotorzone_sconfitto = 0;
    num_round = 0;
    idx_round = 0;
}


/* ===============
   SETUP GIOCATORI
   ===============*/

static void crea_giocatori(void) {
    int n = leggi_intero_range("Inserisci numero di giocatori (1-4): ", 1, 4);

    for (int i = 0; i < n; i++) {
        giocatori[i] = (Giocatore *)malloc(sizeof(Giocatore));
        if (giocatori[i] == NULL) {
            printf("Errore allocazione memoria.\n");
            exit(1);
        }

        /* Nome */
        leggi_stringa("Inserisci nome giocatore: ", giocatori[i]->nome, sizeof(giocatori[i]->nome));

        /* salva copia “originale” */
        strncpy(giocatori[i]->nome_originale, giocatori[i]->nome, sizeof(giocatori[i]->nome_originale));
        giocatori[i]->nome_originale[sizeof(giocatori[i]->nome_originale) - 1] = '\0';

        /* Mondo iniziale: Mondo Reale */
        giocatori[i]->mondo = 0;
        giocatori[i]->pos_mondoreale = NULL;
        giocatori[i]->pos_soprasotto = NULL;

        /* Tiri dadi abilità */
        giocatori[i]->attacco_pischico = dado20();
        giocatori[i]->difesa_pischica = dado20();
        giocatori[i]->fortuna = dado20();

        /* Zaino vuoto */
        svuota_zaino(giocatori[i]);

        printf("\n%s - valori iniziali: ATK=%d DEF=%d FORT=%d\n",
               giocatori[i]->nome,
               giocatori[i]->attacco_pischico,
               giocatori[i]->difesa_pischica,
               giocatori[i]->fortuna);

        /* Scelte */
        printf("\nScegli un'opzione:\n");
        printf("1) +3 Attacco, -3 Difesa\n");
        printf("2) +3 Difesa, -3 Attacco\n");
        if (!undici_gia_scelta) {
            printf("3) Diventa UndiciVirgolaCinque: +4 Attacco, +4 Difesa, -7 Fortuna (solo uno per partita)\n");
        }

        int scelta_bonus;
        while (1) {
            scelta_bonus = leggi_intero_range("Scelta: ", 1, (!undici_gia_scelta ? 3 : 2));
            if (scelta_bonus == 3 && undici_gia_scelta) {
                printf("Opzione non disponibile. Gia' scelta da un altro giocatore.\n");
                continue;
            }
            break;
        }

        if (scelta_bonus == 1) {
            giocatori[i]->attacco_pischico += 3;
            giocatori[i]->difesa_pischica -= 3;
        } else if (scelta_bonus == 2) {
            giocatori[i]->difesa_pischica += 3;
            giocatori[i]->attacco_pischico -= 3;
        } else if (scelta_bonus == 3) {
            undici_gia_scelta = 1;
            giocatori[i]->attacco_pischico += 4;
            giocatori[i]->difesa_pischica += 4;
            giocatori[i]->fortuna -= 7;

            /* cambia nome */
            strncpy(giocatori[i]->nome, "UndiciVirgolaCinque", sizeof(giocatori[i]->nome));
            giocatori[i]->nome[sizeof(giocatori[i]->nome) - 1] = '\0';
        }

        /* Evita Valori Esagerati */
        if (giocatori[i]->attacco_pischico < 1) giocatori[i]->attacco_pischico = 1;
        if (giocatori[i]->difesa_pischica < 1) giocatori[i]->difesa_pischica = 1;
        if (giocatori[i]->fortuna < 1) giocatori[i]->fortuna = 1;

        printf("-> Nuovi valori: ATK=%d DEF=%d FORT=%d (Nome: %s)\n",
               giocatori[i]->attacco_pischico,
               giocatori[i]->difesa_pischica,
               giocatori[i]->fortuna,
               giocatori[i]->nome);
    }
    for (int i = n; i < 4; i++) giocatori[i] = NULL;
}

static void stampa_giocatore(Giocatore *g) {
    printf("\n=== GIOCATORE ===\n");
    printf("Nome: %s\n", g->nome);
    printf("Mondo: %s\n", g->mondo == 0 ? "Mondo Reale" : "Soprasotto");
    printf("ATK=%d DEF=%d FORT=%d\n",
           g->attacco_pischico, g->difesa_pischica, g->fortuna);

    printf("Zaino: ");
    for (int i = 0; i < 3; i++)
        printf("%s ", nome_oggetto(g->zaino[i]));
    printf("\n");
}

static Tipo_nemico nemico_corrente(Giocatore *g) {
    if (!g) return nessun_nemico;
    if (g->mondo == 0) return g->pos_mondoreale ? g->pos_mondoreale->nemico : nessun_nemico;
    return g->pos_soprasotto ? g->pos_soprasotto->nemico : nessun_nemico;
}


static void rimuovi_nemico_zona(Giocatore *g) {
    if (g->mondo == 0) g->pos_mondoreale->nemico = nessun_nemico;
    else g->pos_soprasotto->nemico = nessun_nemico;
}

static void elimina_giocatore(Giocatore *g) {
    for (int i = 0; i < 4; i++) {
        if (giocatori[i] == g) {
            free(giocatori[i]);
            giocatori[i] = NULL;
            return;
        }
    }
}

static int utilizza_oggetto(Giocatore *g, int *bonus_att, int *bonus_def, int *bonus_fort) {
    printf("\n--- ZAINO ---\n");
    for (int i = 0; i < 3; i++) {
        printf("%d) %s\n", i + 1, nome_oggetto(g->zaino[i]));
    }
    printf("4) Annulla\n");

    int scelta = leggi_intero_range("Scegli oggetto da usare: ", 1, 4);
    if (scelta == 4) return 0;

    int idx = scelta - 1;
    Tipo_oggetto o = g->zaino[idx];
    if (o == nessun_oggetto) {
        printf("Non c'e' nessun oggetto in quello slot.\n");
        return 0;
    }

    /* Oggetti */
    switch (o) {
        case bicicletta:
            printf("Usi BICICLETTA: +3 fortuna (per tentare schivate/fughe) in questo combattimento.\n");
            *bonus_fort += 3;
            break;
        case bussola:
            printf("Usi BUSSOLA: +2 attacco (ti aiuta a colpire) in questo combattimento.\n");
            *bonus_att += 2;
            break;
        case maglietta_fuocoinferno:
            printf("Usi MAGLIETTA FUOCOINFERNO: +2 difesa in questo combattimento.\n");
            *bonus_def += 2;
            break;
        case schitarrata_metallica:
            printf("Usi SCHITARRATA METALLICA: +4 attacco in questo combattimento.\n");
            *bonus_att += 4;
            break;
        default:
            break;
    }

    /* Consuma oggetto */
    g->zaino[idx] = nessun_oggetto;
    return 1;
}

static int combatti(Giocatore *g) {
    if (!g) return 0;

    Tipo_nemico n = nemico_corrente(g);
    if (n == nessun_nemico) {
        printf("Non c'e' nessun nemico con cui combattere qui.\n");
        return 0; /* torna al menu senza consumare azione */
    }

    /* Stat nemici */
    int nem_hp, nem_att, nem_def;
    switch (n) {
        case billi:        nem_hp = 10; nem_att = 10; nem_def = 8;  break;
        case democane:     nem_hp = 14; nem_att = 12; nem_def = 10; break;
        case demotorzone:  nem_hp = 18; nem_att = 14; nem_def = 12; break;
        default:           nem_hp = 10; nem_att = 10; nem_def = 10; break;
    }

    int bonus_att = 0, bonus_def = 0, bonus_fort = 0;

    printf("\n COMBATTIMENTO! Nemico: %s\n", nome_nemico(n));

    while (1) {
        printf("\n--- Menu combattimento ---\n");
        printf("Nemico %s | HP: %d\n", nome_nemico(n), nem_hp);
        printf("Tu | ATK=%d DEF=%d FORT=%d (bonus: +%d ATK, +%d DEF, +%d FORT)\n",
               g->attacco_pischico, g->difesa_pischica, g->fortuna,
               bonus_att, bonus_def, bonus_fort);

        printf("1) Attacca\n");
        printf("2) Difenditi\n");
        printf("3) Usa oggetto\n");
        printf("4) Tenta fuga (prova fortuna)\n");

        int scelta = leggi_intero_range("Scelta: ", 1, 4);

        if (scelta == 3) {
            utilizza_oggetto(g, &bonus_att, &bonus_def, &bonus_fort);
            continue; /* usare oggetto non fa avanzare il turno */
        }

        if (scelta == 4) {
            int tiro = dado20();
            int fort_eff = g->fortuna + bonus_fort;
            printf("Tenta fuga: d20=%d (deve essere < %d)\n", tiro, fort_eff);
            if (tiro < fort_eff) {
                printf("Fuga riuscita! Rimani nella stanza, ma non puoi muovere finche' c'e' il nemico.\n");
                return 0; /* nemico resta -> niente movimento */
            } else {
                printf("Fuga fallita!\n");
                /* fallire la fuga = il nemico attacca */
            }
        }

        /* Turno giocatore */
        if (scelta == 1) {
            int tiro = dado20();
            int att_eff = g->attacco_pischico + bonus_att;
            int colpo = tiro + att_eff - nem_def;

            printf("Attacco: d20=%d + ATK(%d) - DEF_nemico(%d) = %d\n", tiro, att_eff, nem_def, colpo);

            if (colpo <= 0) {
                printf("Il tuo colpo non va a segno.\n");
            } else {
                nem_hp -= colpo;
                if (nem_hp < 0) nem_hp = 0;
                printf("Colpito! Danni=%d | HP nemico=%d\n", colpo, nem_hp);
            }
        }

        /* Vittoria */
        if (nem_hp <= 0) {
            printf("\n🏆 Nemico sconfitto!\n");

            /* 50% probabilità che scompaia dalla zona */
            int fifty = rand() % 2;
            if (fifty == 1) {
                printf("Il nemico scompare dalla zona.\n");
                rimuovi_nemico_zona(g);
            } else {
                printf("Il nemico NON scompare: chi entrera' qui in futuro dovra' combatterlo.\n");
            }
            if (n == demotorzone) {
                demotorzone_sconfitto = 1;
        }
            return 1; /* combattimento superato */
        }

        /* Turno nemico: se il giocatore ha scelto difenditi, riduce il danno */
        int difesa_attiva = (scelta == 2);

        int tiro_n = dado20();
        int def_eff = g->difesa_pischica + bonus_def;
        int danno = tiro_n + nem_att - def_eff;

        if (difesa_attiva) {
            printf("Ti difendi: riduci i danni.\n");
            danno -= 5;
        }

        printf("Nemico attacca: d20=%d + ATK_nem(%d) - DEF(%d) = %d\n",
               tiro_n, nem_att, def_eff, danno);

        if (danno <= 0) {
            printf("Para/Schivi l'attacco.\n");
        } else {
            printf("Subisci l'attacco! (danni=%d)\n", danno);

            /* Sopravvivenza basata su fortuna*/
            int tiro_f = dado20();
            int fort_eff = g->fortuna + bonus_fort;
            printf("Prova fortuna per resistere: d20=%d (deve essere <= %d)\n", tiro_f, fort_eff);

            if (tiro_f <= fort_eff) {
                printf("Resisti! Resti in vita.\n");
            } else {
                printf("Sei morto durante il combattimento.\n");
                elimina_giocatore(g);
                return 0;
            }
        }
    }
}

static void avanza(Giocatore *g) {
    if (!g) return;

    /* Se c'è un nemico nella zona corrente, devi combattere prima di muoverti */
    if (nemico_corrente(g) != nessun_nemico) {
        if (!combatti(g)) {
            printf("Non puoi muoverti finche' il nemico non e' sconfitto.\n");
            return;
        }
    }

    if (g->mondo == 0) {
        if (g->pos_mondoreale && g->pos_mondoreale->avanti) {
            g->pos_mondoreale = g->pos_mondoreale->avanti;
            printf("Avanzi nella zona successiva.\n");
        } else {
            printf("Non puoi avanzare oltre.\n");
        }
    } else {
        if (g->pos_soprasotto && g->pos_soprasotto->avanti) {
            g->pos_soprasotto = g->pos_soprasotto->avanti;
            printf("Avanzi nella zona successiva (Soprasotto).\n");
        } else {
            printf("Non puoi avanzare oltre.\n");
        }
    }
}

static void indietreggia(Giocatore *g) {
    if (!g) return;

    if (nemico_corrente(g) != nessun_nemico) {
        if (!combatti(g)) {
            printf("Non puoi muoverti finche' il nemico non e' sconfitto.\n");
            return;
    }
}
    if (g->mondo == 0) {
        if (g->pos_mondoreale && g->pos_mondoreale->indietro) {
            g->pos_mondoreale = g->pos_mondoreale->indietro;
            printf("Torni nella zona precedente.\n");
        } else {
            printf("Non puoi tornare indietro.\n");
        }
    } else {
        if (g->pos_soprasotto && g->pos_soprasotto->indietro) {
            g->pos_soprasotto = g->pos_soprasotto->indietro;
            printf("Torni nella zona precedente (Soprasotto).\n");
        } else {
            printf("Non puoi tornare indietro.\n");
        }
    }
}

static void cambia_mondo(Giocatore *g) {
    if (!g) return;

    /* Da MONDO REALE -> SOPRASOTTO: vale come movimento */
    if (g->mondo == 0) {
        if (nemico_corrente(g) != nessun_nemico) {
            printf("Non puoi cambiare mondo: devi prima sconfiggere il nemico nel Mondo Reale.\n");
            return;
        }

        if (g->pos_mondoreale && g->pos_mondoreale->link_soprasotto) {
            g->mondo = 1;
            g->pos_soprasotto = g->pos_mondoreale->link_soprasotto;
            printf("Sei passato nel Soprasotto.\n");
        } else {
            printf("Impossibile cambiare mondo da qui.\n");
        }
        return;
    }

    /* Da SOPRASOTTO -> MONDO REALE: serve prova fortuna (tiro < fortuna) */
    {
        int tiro = dado20();
        printf("Prova fortuna per tornare al Mondo Reale: d20=%d (deve essere < %d)\n", tiro, g->fortuna);
        if (tiro < g->fortuna) {
            if (g->pos_soprasotto && g->pos_soprasotto->link_mondoreale) {
                g->mondo = 0;
                g->pos_mondoreale = g->pos_soprasotto->link_mondoreale;
                printf("Sei tornato nel Mondo Reale.\n");
            } else {
                printf("Impossibile cambiare mondo da qui.\n");
            }
        } else {
            printf("Fortuna insufficiente: resti nel Soprasotto.\n");
        }
    }
}

static int giocatori_vivi(void) {
    int c = 0;
    for (int i = 0; i < 4; i++)
        if (giocatori[i] != NULL) c++;
    return c;
}

static void stampa_zona_corrente(Giocatore *g) {
    if (!g) {
        printf("\n=== ZONA CORRENTE ===\nGiocatore non valido.\n");
        return;
    }

    printf("\n=== ZONA CORRENTE ===\n");

    if (g->mondo == 0) {
        if (!g->pos_mondoreale) {
            printf("Mondo Reale | posizione non valida (NULL)\n");
            return;
        }

        Zona_mondoreale *z = g->pos_mondoreale;
        printf("Mondo Reale | tipo=%s nemico=%s oggetto=%s\n",
               nome_zona(z->tipo), nome_nemico(z->nemico), nome_oggetto(z->oggetto));

    } else {
        if (!g->pos_soprasotto) {
            printf("Soprasotto | posizione non valida (NULL)\n");
            return;
        }

        Zona_soprasotto *z = g->pos_soprasotto;
        printf("Soprasotto | tipo=%s nemico=%s\n",
               nome_zona(z->tipo), nome_nemico(z->nemico));
    }
}

/* ==================
   FUNZIONI PUBBLICHE
   ==================*/

static void salva_vincitori_su_file(void) {
    FILE *fp = fopen(SAVE_WINNERS_FILE, "a");
    if (!fp) return;

    /* una riga = una partita vinta */
    fprintf(fp, "VINCITORI: ");

    int first = 1;
    for (int i = 0; i < 4; i++) {
        if (giocatori[i]) {
            if (!first) fprintf(fp, " | ");
            first = 0;

            /* formato: NomeOriginale (NomeAttuale) se diverso */
            if (strcmp(giocatori[i]->nome_originale, giocatori[i]->nome) != 0) {
                fprintf(fp, "%s (%s)", giocatori[i]->nome_originale, giocatori[i]->nome);
            } else {
                fprintf(fp, "%s", giocatori[i]->nome_originale);
            }
        }
    }

    fprintf(fp, "\n");
    fclose(fp);
}

static void stampa_ultimi_vincitori_da_file(int quante_righe) {
    FILE *fp = fopen(SAVE_WINNERS_FILE, "r");
    if (!fp) {
        printf("Ultimi vincitori: (nessuno salvato)\n");
        return;
    }

    char **ring = calloc(quante_righe, sizeof(char*));
    if (!ring) { fclose(fp); return; }

    int count = 0;
    char buf[MAX_LINE];

    while (fgets(buf, sizeof(buf), fp)) {
        int idx = count % quante_righe;
        free(ring[idx]);
        ring[idx] = my_strdup(buf);
        count++;
    }
    fclose(fp);

    int start = (count > quante_righe) ? (count - quante_righe) : 0;
    int end = count;

    if (count == 0) {
        printf("Ultimi vincitori: (nessuno salvato)\n");
    } else {
        printf("Ultimi vincitori:\n");
        for (int i = start; i < end; i++) {
            int idx = i % quante_righe;
            if (ring[idx]) printf("- %s", ring[idx]); 
        }
    }

    for (int i = 0; i < quante_righe; i++) free(ring[i]);
    free(ring);
}

void imposta_gioco(void) {
    reset_partita();

    printf("\n=== IMPOSTA GIOCO ===\n");
    crea_giocatori();

    menu_creazione_mappa();

    for (int i = 0; i < 4; i++) {
        if (giocatori[i] != NULL) {
            giocatori[i]->mondo = 0;
            giocatori[i]->pos_mondoreale = prima_zona_mondoreale;
            giocatori[i]->pos_soprasotto = NULL;
        }
    }
}

static int primo_slot_libero(Giocatore *g) {
    for (int i = 0; i < 3; i++) {
        if (g->zaino[i] == nessun_oggetto) return i;
    }
    return -1;
}

static void raccogli_oggetto(Giocatore *g) {
    if (!g) return;

    if (g->mondo != 0) {
        printf("Nel Soprasotto non ci sono oggetti da raccogliere.\n");
        return;
    }

    if (!g->pos_mondoreale) {
        printf("Posizione non valida.\n");
        return;
    }

    Tipo_oggetto o = g->pos_mondoreale->oggetto;
    if (o == nessun_oggetto) {
        printf("Non c'e' nessun oggetto qui.\n");
        return;
    }

    printf("Hai trovato: %s\n", nome_oggetto(o));

    int slot = primo_slot_libero(g);
    if (slot != -1) {
        g->zaino[slot] = o;
        g->pos_mondoreale->oggetto = nessun_oggetto;
        printf("Oggetto raccolto nello slot %d dello zaino.\n", slot + 1);
        return;
    }

    /* Zaino pieno */
    printf("Zaino pieno. Vuoi sostituire un oggetto?\n");
    for (int i = 0; i < 3; i++) {
        printf("%d) %s\n", i + 1, nome_oggetto(g->zaino[i]));
    }
    printf("4) Non raccogliere\n");

    int scelta = leggi_intero_range("Scelta: ", 1, 4);
    if (scelta == 4) {
        printf("Non raccogli l'oggetto.\n");
        return;
    }

    int idx = scelta - 1;
    printf("Sostituisci %s con %s.\n", nome_oggetto(g->zaino[idx]), nome_oggetto(o));
    g->zaino[idx] = o;
    g->pos_mondoreale->oggetto = nessun_oggetto;
}

static void prepara_round(void) {
    num_round = 0;
    idx_round = 0;

    for (int i = 0; i < 4; i++) {
        if (giocatori[i] != NULL) ordine_round[num_round++] = i;
    }

    /* Ordine casuale */
    for (int i = num_round - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int tmp = ordine_round[i];
        ordine_round[i] = ordine_round[j];
        ordine_round[j] = tmp;
    }
}

static Giocatore* prossimo_giocatore_round(void) {
    if (num_round == 0) return NULL;

    if (idx_round >= num_round) {
        prepara_round(); /* nuovo giro */
        if (num_round == 0) return NULL;
    }

    int idx = ordine_round[idx_round++];
    return giocatori[idx];
}

void gioca(void) {
    if (!gioco_impostato || !mappa_chiusa) {
        printf("\nIl gioco non e' pronto. Imposta e chiudi la mappa prima di giocare.\n");
        return;
    }

    printf("\n=== INIZIO GIOCO ===\n");

    prepara_round();

    while (giocatori_vivi() > 0 && !demotorzone_sconfitto) {
        Giocatore *g = prossimo_giocatore_round();
        if (!g) continue; 

        int azione_fatta = 0;
        int combattuto_in_turno = 0;
        int scelta;

        printf("\n--- TURNO DI %s ---\n", g->nome);

        do {
            printf("\n1) Avanza\n");
            printf("2) Indietreggia\n");
            printf("3) Cambia mondo\n");
            printf("4) Combatti\n");
            printf("5) Stampa giocatore\n");
            printf("6) Stampa Zona\n");
            printf("7) Raccogli oggetto\n");
            printf("8) Passa turno\n");
            printf("Scelta: ");

            scelta = leggi_intero_range("", 1, 8);

            switch (scelta) {
                case 1:
                    if (!azione_fatta) {
                        avanza(g);
                        azione_fatta = 1;
                    } else {
                        printf("Hai gia' fatto un'azione di movimento.\n");
                    }
                    break;

                case 2:
                    if (!azione_fatta) {
                        indietreggia(g);
                        azione_fatta = 1;
                    } else {
                        printf("Hai gia' fatto un'azione di movimento.\n");
                    }
                    break;

                case 3:
                    if (!azione_fatta) {
                        cambia_mondo(g);
                        azione_fatta = 1;
                    } else {
                        printf("Hai gia' fatto un'azione di movimento.\n");
                    }
                    break;

                case 4:
                    if (combattuto_in_turno) {
                        printf("Hai gia' combattuto in questo turno.\n");
                        break;
                    }
                    if (nemico_corrente(g) == nessun_nemico) {
                        printf("Non c'e' nessun nemico con cui combattere qui.\n");
                        break;
                    }

                    if (combatti(g)) {
                        printf("Combattimento concluso. Ora puoi muoverti.\n");
                    } else {
                        printf("Verrai ricordato come un eroe...\n");
                    }

                    combattuto_in_turno = 1;

                    if (demotorzone_sconfitto) {
                        scelta = 8;   
                    }
                    break;

                case 5:
                    stampa_giocatore(g);
                    break;

                case 6:
                    stampa_zona_corrente(g);
                    break;

                case 7:
                    raccogli_oggetto(g);
                    break;

                case 8:
                    printf("Passi il turno.\n");
                    break;
            }

        } while (scelta != 8);
    }

    if (demotorzone_sconfitto) {
    printf("\nVITTORIA! Il demotorzone e' stato sconfitto!\n");
    salva_vincitori_su_file();
} else {
    printf("\nSCONFITTA! Tutti i giocatori sono morti.\n");
}
    printf("\n=== Gioco Finito ===\n");
}

void termina_gioco(void) {
    printf("\nTermino il gioco. A presto!\n");
    reset_partita();
}

void crediti(void) {
    printf("\n=== CREDITI ===\n");
    printf("Creatore: Leonardo Sapora\n");
    stampa_ultimi_vincitori_da_file(5);
}
