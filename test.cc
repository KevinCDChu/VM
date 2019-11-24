#include <ncurses.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>



void printw(std::string str) {
    printw(str.c_str());
}




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

    void updateView(int ch) override {
        if(ch == 'h') { // Move cursor left
            if(cursor_x > 0) {
                cursor_x = std::min(cursor_x - 1, static_cast<int>(lines[cursor_y + offset].size())); // Take min as we might still be greater from previous line
            }
        }
        if(ch == 'l') { // Move cursor right
            if(cursor_x < static_cast<int>(lines[cursor_y + offset].size()) - 1) ++cursor_x;
        }
        if(ch == 'k') { // move cursor up
            if(cursor_y > 0 && cursor_y <= 5) { 
                if(offset > 0) {
                    offset -= 1;
                    scrl(-1);
                }
                if(offset == 0) --cursor_y;
            }
            else cursor_y = std::max(cursor_y - 1, 0);
        }
        if(ch == 'j') { // move cursor down
            if(height - cursor_y <= 5) {
                scrl(1);
                if(offset + height < static_cast<int>(lines.size()) - 1) offset += 1;
                if(offset + height == static_cast<int>(lines.size()) - 1) cursor_y = std::min(cursor_y + 1, height);
            }
            else cursor_y = std::min(std::min(cursor_y + 1, height), static_cast<int>(lines.size()) - 1 - offset);
        }
    }

    void displayView() override {
        move(0, 0);
        for(int i = 0; i <= height; ++i) {
            if(i + offset < static_cast<int>(lines.size())) printw(lines[i + offset].c_str());
            else {
                init_pair(1, COLOR_BLUE, COLOR_BLACK);
                attron(COLOR_PAIR(1));
                printw("~");
            }
            printw("\n");
        }
        attroff(COLOR_PAIR(1));

        move(cursor_y, std::min(cursor_x, static_cast<int>(lines[cursor_y + offset].size()))); // take min as we might have overshoot from previous line
        refresh();
    }
};




int main() {
    initscr();
    start_color();
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
