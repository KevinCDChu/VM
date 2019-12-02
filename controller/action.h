#ifndef ACTION_H
#define ACTION_H
#include <ncurses.h>

class Action {
    int ch;
  public:
    Action(int ch): ch{ch} {} 

    int getchar() {
        return ch;
    }

};

#endif