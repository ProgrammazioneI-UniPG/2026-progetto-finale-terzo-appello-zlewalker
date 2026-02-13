#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "gamelib.h"

static void pulisci_buffer(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) {}
}

static int leggi_intero_menu(void) {
    int scelta;
    if (scanf("%d", &scelta) != 1) {
        pulisci_buffer();
        return -1; // input non numerico
    }
    pulisci_buffer();
    return scelta;
}

int main(void) {
    srand((unsigned)time(NULL));  // inizializza seed RNG con orario

    int scelta;
    do {
        printf("\n--- MENU PRINCIPALE ---\n");
        printf("1) Imposta gioco\n");
        printf("2) Gioca\n");
        printf("3) Termina gioco\n");
        printf("4) Visualizza crediti\n");
        printf("Scegli un'opzione: ");

        scelta = leggi_intero_menu();

        switch (scelta) {
            case 1:
                imposta_gioco();
                break;
            case 2:
                gioca();
                break;
            case 3:
                termina_gioco();
                break;
            case 4:
                crediti();
                break;
            default:
                printf("\nComando non valido. Inserisci 1, 2, 3 o 4.\n");
                break;
        }
    } while (scelta != 3);

    return 0;
}

