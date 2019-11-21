#include <ncurses.h>
#include <iostream>
#include <fstream>
int main() {
    initscr();
    cbreak();
    noecho();
    std::ifstream f{"test.cc"};
    f >> std::noskipws;
    char c;
    std::string str;
    while(f >> c) {
        if(c != '\n') str += c;
    }
	printw(str.c_str());
	refresh();	
	getch();
	endwin();
    }