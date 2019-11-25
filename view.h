#ifndef VIEW_H
#define VIEW_H
#include <ncurses.h>
#include <string>
#include <vector>
#include "utils.h"




class View {
  public:
    virtual int getHeight() = 0;
    virtual int getWidth() = 0;
    virtual void updateView() = 0;
    virtual void displayView(std::vector<std::string> &lines, const int &cursor_y, const int &cursor_x, const int &offset, const std::string &cmdstr) = 0;
};



class Window : public View {
    public:
    
    int height;
    int width;

    Window() { updateView(); }

    int getHeight() { return height; }

    int getWidth() { return width; }

    void updateView() override {
        getmaxyx(stdscr, height, width);
        height -= 2;
    }

    void displayView(std::vector<std::string> &lines, const int &cursor_y,const  int &cursor_x, const int &offset, const std::string &cmdstr) override {
        move(0, 0);
        for(int i = 0; i <= height; ++i) {
            if(i + offset < static_cast<int>(lines.size())) printw(lines[i + offset]); // print out line
            else {
                attron(COLOR_PAIR(1));
                printw("~");
            }
            printw("\n");
        }
        attroff(COLOR_PAIR(1));
        move(cursor_y, std::min(cursor_x, std::max(static_cast<int>(lines[cursor_y + offset].size() - 1), 0))); // take min as we might have overshoot from previous line
        refresh();
    }
};


class Bar : public View {
    public:
    
    int height;
    int width;

    Bar() { updateView(); }

    int getHeight() { return height; }

    int getWidth() { return width; }

    void updateView() override {
        getmaxyx(stdscr, height, width);
        height -= 2;
    }

    void displayView(std::vector<std::string> &lines, const int &cursor_y, const int &cursor_x, const int &offset, const std::string &cmdstr) override {
        clearbottom(height);
        move(height + 1, 0);
        printw(cmdstr);

        if (cmdstr == "") {
            move (height + 1, width - 4);
            printw("All");

            move (height + 1, width - 18);
            std::string y = std::to_string(cursor_y + offset + 1);
            std::string x = std::to_string(std::min(cursor_x + 1, std::max(static_cast<int>(lines[cursor_y+offset].size()), 1)));
            std::string loc = y + "," + x;
            std::string blan = "";

            for(int i = 0; i < static_cast<int>(13 - loc.size()); ++i) {
                blan += " ";
            }

            loc += blan;
            printw(loc);
        }

        move(cursor_y, std::min(cursor_x, std::max(static_cast<int>(lines[cursor_y + offset].size() - 1), 0))); // take min as we might have overshoot from previous line
        refresh();
        
    }
};



#endif
