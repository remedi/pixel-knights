// This file contains tests for server functionality
//
// Authors: Pixel Knights
// Date: 5.3.2015

#include <stdio.h>
#include <string.h>

#include "gamestate.h"

int main(void) {

    Gamestate game;
    memset(&game, 0, sizeof(Gamestate));
    Coord c;
    c.x = 2;
    c.y = 3;

    addPlayer(&game, 0x01, c, '#');
    printPlayers(&game);

    addPlayer(&game, 0x15, c, 'a');
    addPlayer(&game, 0xFF, c, 'b');

    changePlayerSign(&game, 0x01, '/');
    printPlayers(&game);

    c.x = 25;

    movePlayer(&game, 0xFF, c);
    printPlayers(&game);

    removePlayer(&game, 0x01);
    printPlayers(&game);

    removePlayer(&game, 0x01);
    removePlayer(&game, 0x15);
    removePlayer(&game, 0xFF);
    printPlayers(&game);

    return 0;
}
