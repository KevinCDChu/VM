#ifndef CNTL_H
#define CNTL_H
#include <ncurses.h>
#include "action.h"

class Controller {
  protected:
    std::unique_ptr<Action> action;
  public:
    virtual std::unique_ptr<Action> getAction() = 0;
    virtual void genAction() = 0;
};

class Keyboard: public Controller {
  public:
    std::unique_ptr<Action> getAction() override {
        return std::move(action);
    }
    void genAction() override {
        int ch = getch();
        Action tmp = Action(ch);
        action = std::make_unique<Action>(tmp);
    }

};

#endif