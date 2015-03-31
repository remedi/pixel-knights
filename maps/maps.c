#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>

#include "maps.h"

//Allocate memory for map. Initialize memory as map tiles. Return two-dimensional character array as the map.
int createMap(Mapdata *map_data, int map_nr) {
    int width = 0, height = 0;
    int fd = 0, i;
    char *map_path;

    fd = open("../maps/square.map", O_RDONLY);
    if (fd != -1)
        close(fd);

    switch(map_nr) {
        case 1:
            if (fd == -1)
                map_path = "maps/square.map";
            else 
                map_path = "../maps/square.map";
            break;
        case 2:
            if (fd == -1)
                map_path = "maps/split.map";
            else
                map_path = "../maps/split.map";
            break;
        case 3:
            if (fd == -1)
                map_path = "maps/hall.map";
            else
                map_path = "../maps/hall.map";
            break;
        default:
            if (fd == -1)
                map_path = "maps/square.map";
            else
                map_path = "../maps/square.map";
            map_nr = 1;
            break;
    }

    fd = open(map_path, O_RDONLY);
    if(fd == -1) {
        perror("open");
        return -1;
    }
    read(fd, &width, 1);
    read(fd, &height, 1);

    //Skip the newline
    lseek(fd, 1, SEEK_CUR);

    // Allocate memory for map
    char **rows = malloc(sizeof(char *) * height);
    if(rows == NULL) {
        perror("drawMap, malloc");
        return -1;
    }

    //Allocate memory for each row and set initial tiles
    for(i = 0; i<height; i++) {
        rows[i] = malloc(sizeof(char) * width + 1);
        if(rows[i] == NULL) {
            perror("drawMap, malloc");
            free(rows);
            return -1;
        }
        // Initialize rows
        memset(rows[i], ' ', width-1);
        rows[i][width] = '\0';
    }

    for(i = 0; i<height; i++) {
        read(fd, rows[i], width);
        //Skip the newline
        lseek(fd, 1, SEEK_CUR);
    }


    //Fill the sruct:
    map_data->height = height;
    map_data->width = width;
    map_data->map = rows;
    map_data->map_nr = map_nr;
    return 0;
}

//Free memory reserved by createMap
void freeMap(void *arg) {
    int i = 0;
    Mapdata *map_data = arg;
    char **rows = map_data->map;
    // Free memory allocated for map
    for(i = 0; i<map_data->height; i++) {
        free(rows[i]);
    }
    free(rows);
    return;
}

//Check for a wall from Mapdata struct created by createMap. Return 0 for no map
int checkWall(Mapdata *map_data, Coord c) {
    int x = c.x;
    int y = c.y;
    //Check for walls
    if(map_data->map[y][x] == '#') {
        return 1;
    }
    return 0;
}
