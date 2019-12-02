#ifndef UNDO_H
#define UNDO_H
#include <ncurses.h>

class Undo {
    
    int start;
    std::vector<std::string> prevaction;

  public:
    int getStart() {
        return start;
    }
    std::vector<std::string> getChange() {
        return prevaction;
    }
    void setStart(int i) {
        start = i;
    }
    void pushChange(std::string s) {
        prevaction.push_back(s);        
    }
    
};

#endif
