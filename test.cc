#include <ncurses.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>




class View {

    virtual void updateView() = 0;
    virtual void displayView() = 0;

};


class Window : public View {
    public:

    std::vector<std::string> lines;
    int offset;
    int width;
    int height;
    int cursor_x;
    int cursor_y;

    void updateOffset(int ch) {
        if(ch == 'w') {
            scrl(-1);
            if(offset > 0) offset -= 1;
        }
        if(ch == 's') {
            scrl(1);
            if(offset + height < lines.size() - 1) {
                offset += 1;
            }
        }
    }

    void updateView() override {}
    void displayView() override {
        for(int i = 0; i <= height; ++i) {
            printw(lines[i + offset].c_str());
            std::string new_line = "\n";
            printw(new_line.c_str());
        }
        refresh();
    }

};




int main() {
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
    Window window;
    window.lines = lines;  
    int w;
    int h;
    getmaxyx(stdscr, h, w);
    h -= 2;
    window.height = h;
    window.offset = 0;
    window.displayView();
    int ch = getch();
    while(ch != 'p') {
        ch = getch();
        getmaxyx(stdscr, h, w);
        h -= 2;
        window.height = h;
        window.updateOffset(ch);
        window.displayView();
    }
	endwin();
    }