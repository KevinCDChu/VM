#include <ncurses.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include "view.h"
#include "utils.h"
#include "model.h"

int main() {
    initscr();
    start_color();
    init_pair(1, COLOR_BLUE, COLOR_BLACK);
    scrollok(stdscr, true);
    cbreak();
    noecho();
    std::ifstream f{"test.txt"};
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
    logic.views.push_back(bar);
    logic.views.push_back(window);
    logic.updateViews();
    logic.displayViews();
    int ch = 'a';
    while(ch != 'p') {
        ch = getch();
        logic.updateViews();
        logic.interpret_input(ch);
        logic.displayViews();
    }
    std::ofstream myfile;
    myfile.open ("test.txt");
    for(size_t i = 0; i < logic.lines.size(); ++i) {
        myfile << logic.lines[i];
        if(i != lines.size() - 1) myfile << "\n";
    }
    myfile.close();
    endwin();
}
