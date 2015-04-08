// This file contains tests for server functionality
//
// Authors: Pixel Knights
// Date: 5.3.2015

#include <stdio.h>
#include <string.h>

#include "../server/gamestate.h"
#include "../typedefs.h"

int main(void) {

    Gamestate game;
    memset(&game, 0, sizeof(Gamestate));
    Coord c;
    c.x = 2;
    c.y = 3;

    addObject(&game, 0x01, c, 0, '#', PLAYER);
    printObjects(&game);

    addObject(&game, 0x15, c, 0, 'a', PLAYER);
    addObject(&game, 0xFF, c, 0, 'b', BULLET);

    changePlayerSign(&game, 0x01, '/');
    printObjects(&game);

    c.x = 25;

    addObject(&game, 0x55, c, 0, '$', POINT);
    printObjects(&game);

    removeObject(&game, 0x01);
    printObjects(&game);

    removeObject(&game, 0x01);
    removeObject(&game, 0x15);
    removeObject(&game, 0xFF);
    printObjects(&game);

    freeGamestate(&game);

    return 0;
}
