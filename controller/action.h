#ifndef ACTION_H
#define ACTION_H
#include <ncurses.h>

class Action {
    public:
    int ch;

    Action(int ch): ch{ch} {} 

    int getchar() {
        return ch;
    }
};

#endif