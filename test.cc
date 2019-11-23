#include <ncurses.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>

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
    lines.push_back(cur_line);
    std::cout << lines[0];

    
    int w;
    int h;
    getmaxyx(stdscr, w, h);
    w -= 2;
    for(int i = 0; i <= w; ++i) {
        printw(lines[i].c_str());
        std::string new_line = "\n";
        printw(new_line.c_str());
    }
    int offset = 0;
    wmove(stdscr, y, x);
	refresh();
    int ch = getch();
    while(ch != 'p') {
        ch = getch();
        if(ch == 'w') {
            scrl(-1);
            if(offset > 0) offset -= 1;
        }

        if(ch == 's') {
            scrl(1);
            if(offset + w < lines.size() - 1) {
                offset += 1;
            }
        }
        if(ch == 'h') x -= 1;
        if(ch == 'k') y -= 1;
        if(ch == 'j') y += 1;
        if(ch == 'l') x += 1;
        if(x < 1) x = 0;
        if(y < 1) y = 0;
        wmove(stdscr, y, x);
        // printw(str.c_str());
        getmaxyx(stdscr, w, h);
        w -= 2;
        refresh();
        for(int i = 0; i <= w; ++i) {
        printw(lines[i + offset].c_str());
        std::string new_line = "\n";
        printw(new_line.c_str());
    }
        //getmaxyx(stdscr, w, h);
        refresh();
    }
	endwin();
    }