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
    init_colours(code_file(argv[1]));
    scrollok(stdscr, true);
    keypad(stdscr, true);
    cbreak();
    noecho();
    ESCDELAY = 0;
    std::vector<std::string> lines;
    if(file_exists(argv[1])) {
        std::ifstream f{argv[1]};
        f >> std::noskipws;
        char c;
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
    } else {
        lines.push_back("");
    }
    Logic logic;
    logic.lines = lines;
    logic.addView(std::make_unique<Window>());
    logic.addView(std::make_unique<Bar>());
    logic.addController(std::make_unique<Keyboard>());
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