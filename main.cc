#include <ncurses.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include "utils.h"
#include "model/model.h"


int main(int argc, char * argv[]) {
    if(argc != 2) {
        std::cerr << "No file specified" << std::endl;
        return 1;
    }
    initscr();
    start_color();
    init_pair(1, COLOR_BLUE, COLOR_BLACK);
    init_pair(2, COLOR_WHITE, COLOR_RED);
    scrollok(stdscr, true);
    cbreak();
    noecho();
    std::ifstream f{argv[1]};
    f >> std::noskipws;
    char c;
    std::vector<std::string> lines;
    std::string cur_line;
    while(f >> c) {
        if(c == '\n') {
            lines.push_back(cur_line);
            cur_line = "";
        } else {
        cur_line += c;
        }
    }
    if(cur_line != "") lines.push_back(cur_line);  
    Logic logic;
    logic.lines = lines;
    Window *window = new Window;
    Bar *bar = new Bar;
    Keyboard *keyboard = new Keyboard;
    logic.addView(window);
    logic.addView(bar);
    logic.addController(keyboard);
    logic.filename = argv[1];
    logic.updateViews();
    logic.displayViews();
    while(!logic.complete) {
        logic.updateViews();
        logic.interpret_input();
        logic.displayViews();
    }
    endwin();
}

