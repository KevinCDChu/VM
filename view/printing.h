#ifndef PRINTING_H
#define PRINTING_H
#include <ncurses.h>
#include <string>


void init_colours(bool code_file) {
    init_pair(1, COLOR_BLUE, COLOR_BLACK);
    init_pair(2, COLOR_WHITE, COLOR_RED);
    if(code_file) {
        init_pair(3, COLOR_GREEN, COLOR_BLACK);
    } else {
        init_pair(3, COLOR_WHITE, COLOR_BLACK);
    }
}


void myprintw(std::string line) {
    if(line.find("//") != std::string::npos) { //contains comment
        int comment_idx = line.find("//");
        myprintw(line.substr(0, comment_idx));
        attron(COLOR_PAIR(3));
        printw(line.substr(comment_idx, static_cast<int>(line.size()) - comment_idx));
        attroff(COLOR_PAIR(3));
    } else {
        printw(line);
    }
}




#endif