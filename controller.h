#ifndef CNTL_H
#define CNTL_H
#include <ncurses.h>
#include "action.h"

class Controller {
    public:
    Action *action;
    virtual Action *getAction() = 0;
    virtual void genAction() = 0;
};

class Keyboard: public Controller {
    public:
    Action *getAction() override {
        return action;
    }
    void genAction() override {
        int ch = getch();
        Action tmp = Action(ch);
        action = &tmp;
    }

};

#endif