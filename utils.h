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

void clearline() {
    clrtoeol();
}

bool isnum(const std::string& s)
{
    for(int i = 0; i < static_cast<int>(s.size()); ++i) {
        if (!isdigit(s[i])) return false;
    }
    return true;
}

#endif