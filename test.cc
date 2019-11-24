#include <ncurses.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>




class View {

    virtual void updateView(int ch) = 0;
    virtual void displayView() = 0;

};


class Window : public View {
    public:

    std::vector<std::string> lines;
    int offset;
    int width;
    int height;
    int cursor_x = 0;
    int cursor_y = 0;

    void updateOffset(int ch) {
        if(ch == 'w') {
            if(cursor_y > 0) scrl(-1);
            if(offset > 0) offset -= 1;
        }
        if(ch == 's') {
            scrl(1);
            if(offset + height < lines.size() - 1) offset += 1;
        }
    }

    void updateView(int ch) override {
        if(ch == 'h') {
            if(cursor_x > 0) {
                int line_size = lines[cursor_y + offset].size();
                cursor_x = std::min(cursor_x - 1, line_size);
            }
        }
        if(ch == 'l') {
            if(cursor_x < lines[cursor_y + offset].size() - 1) ++cursor_x;
        }
        if(ch == 'j') {
            if(cursor_y > 0 && cursor_y <= 5) {
                scrl(-1);
                if(offset > 0) offset -= 1;
            }
            else --cursor_y;
        }
        if(ch == 'k') {
            if(height - cursor_y <= 5) {
                scrl(1);
                if(offset + height < lines.size() - 1) offset += 1;
            }
            else ++cursor_y;
        }
    }
    void displayView() override {
        move(0, 0);
        for(int i = 0; i <= height; ++i) {
            printw(lines[i + offset].c_str());
            std::string new_line = "\n";
            printw(new_line.c_str());
        }
        int line_length = lines[cursor_y + offset].size();
        move(cursor_y, std::min(cursor_x, line_length));
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
        window.updateView(ch);
        window.displayView();
    }
    endwin();
}
