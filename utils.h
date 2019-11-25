#ifndef UTILS_H
#define UTILS_H
#include <ncurses.h>


void printw(std::string str) {
    printw(str.c_str());
}

void clearbottom(int height) {
    move(height + 1, 0);
    clrtoeol();
}


#endif