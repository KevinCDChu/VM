#include <ncurses.h>
#include <iostream>
#include <fstream>

int main() {
    size_t x = 0;
    size_t y = 0;
    initscr();
    scrollok(stdscr, true);
    cbreak();
    noecho();
    std::ifstream f{"test.cc"};
    f >> std::noskipws;
    char c;
    std::string str;
    while(f >> c) {
        str += c;
    }
	printw(str.c_str());
    wmove(stdscr, y, x);
	refresh();
    int ch = getch();
    while(ch != 'p') {
        ch = getch();
        if(ch == 'w') scrl(-1);
        if(ch == 's') scrl(1);
        if(ch == 'h') x -= 1;
        if(ch == 'k') y -= 1;
        if(ch == 'j') y += 1;
        if(ch == 'l') x += 1;
        if(x < 1) x = 0;
        if(y < 1) y = 0;
        wmove(stdscr, y, x);
        // printw(str.c_str());
        refresh();
    }
	endwin();
    }

