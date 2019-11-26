#ifndef PRINTING_H
#define PRINTING_H
#include <ncurses.h>
#include <string>
#include <regex>


void init_colours(bool code_file) {
    init_pair(1, COLOR_BLUE, COLOR_BLACK);
    init_pair(2, COLOR_WHITE, COLOR_RED);
    if(code_file) {
        init_pair(3, COLOR_GREEN, COLOR_BLACK);
        init_pair(4, COLOR_CYAN, COLOR_BLACK);
    } else {
        init_pair(3, COLOR_WHITE, COLOR_BLACK);
        init_pair(4, COLOR_WHITE, COLOR_BLACK);
    }
}


void myprintw(std::string line) {
    std::regex preprocessor("^( )*#( )*include( )*<.+>|^( )*#( )*include( )*\".+\"|^( )*#( )*include( )*<.+>|^( )*#( )*include( )*\".+\"");
    std::smatch match_preprocessor;
    if(line.find("//") != std::string::npos) { //contains comment
        int comment_idx = line.find("//");
        myprintw(line.substr(0, comment_idx));
        attron(COLOR_PAIR(3));
        printw(line.substr(comment_idx, static_cast<int>(line.size()) - comment_idx));
        attroff(COLOR_PAIR(3));
    } else if(std::regex_search(line, match_preprocessor, preprocessor)) {
        attron(COLOR_PAIR(4));
        printw(match_preprocessor[0]);
        attroff(COLOR_PAIR(4));
        size_t match_start = line.find(match_preprocessor[0]);
        printw(line.substr(0, std::min(match_start + match_preprocessor[0].length(), line.length())));
        //myprintw(line.substr(0, index_after));
    } else {
        printw(line);
    }
}




#endif