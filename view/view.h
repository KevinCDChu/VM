#ifndef VIEW_H
#define VIEW_H
#include <ncurses.h>
#include <string>
#include <vector>
#include "../utils.h"
#include "printing.h"

class View {
  public:
    int height;
    int width;
    virtual int getHeight() = 0;
    virtual int getWidth() = 0;
    virtual void updateView() = 0;
    virtual void displayView(std::vector<std::string> &lines, const int &cursor_y, const int &cursor_x, const int &offset, const std::string &cmdstr) = 0;
};



class Window : public View {
    public:
    
    std::vector<int> offsetv;


    Window() { updateView(); }

    int getHeight() { return height; }

    int getWidth() { return width; }

    void updateView() override {
        getmaxyx(stdscr, height, width);
        height -= 2;
    }

    void displayView(std::vector<std::string> &lines, const int &cursor_y,const  int &cursor_x, const int &offset, const std::string &cmdstr) override {
        move(0, 0);
        int offs = 0;
        std::vector<int> tmp (height + 2, 0);
        offsetv = std::move(tmp);
        for(int i = 0; i <= height - offs; ++i) {
            if(i + offset < static_cast<int>(lines.size())) { 
                myprintw(lines[i + offset]); // print out line
                int off = std::max(static_cast<int>(lines[i + offset].size() - 1), 0)/(width - 1);
                offs += off;
                offsetv[i + 1] = offs;
            }
            else {
                attron(COLOR_PAIR(1));
                printw("~");
            }
            printw("\n");
        }
        attroff(COLOR_PAIR(1));
        if(cmdstr != "-- INSERT --") move(adjusty(cursor_x, cursor_y, lines, offset), adjustx(cursor_x, cursor_y, lines, offset));
        else move(adjusty(cursor_x, cursor_y, lines, offset), adjustxin(cursor_x, cursor_y, lines, offset));
        refresh();
    }

    int adjustx(int x, int y, std::vector<std::string> lines, int offset) {
        int minx = x%(width-1);
        int maxy = 0;
        if (x/(width-1) < offsetv[y + 1] - offsetv[y]) {
            maxy = width - 1;
        }
        else {
            maxy = std::max(static_cast<int>(lines[y + offset].size() - 1), 0)%(width-1);
        }
        return std::min(minx, maxy); 
    }

    int adjustxin(int x, int y, std::vector<std::string> lines, int offset) {
        int minx = x%(width-1);
        int maxy = 0;
        if (x/(width-1) < offsetv[y + 1] - offsetv[y]) {
            maxy = width - 1;
        }
        else {
            maxy = std::max(static_cast<int>(lines[y + offset].size()), 0)%(width-1);
        }
        return std::min(minx, maxy); 
    }

    int adjusty(int x, int y, std::vector<std::string> lines, int offset) {
        return y + offsetv[y] + std::min(x/(width-1), offsetv[y + 1] - offsetv[y]);
    }
};


class Bar : public View {
    public:

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
        if(cmdstr == "E37: No write since last change (add ! to override)") attron(COLOR_PAIR(2));
        printw(cmdstr);
        attroff(COLOR_PAIR(2));

        if (cmdstr == "" || cmdstr[0] != ':') {
            move (height + 1, width - 4);
            if ((static_cast<int>(lines.size()) < height + 2) && (offset == 0)) {
                printw("All");
            }
            else {
                if (offset == 0) {
                    printw("Top");
                }
                else if ((static_cast<int>(lines.size()) - height - 1) == offset) {
                    printw("Bot");
                }
                else {
                    int perc = (100*offset)/(static_cast<int>(lines.size()) - height - 1);
                    std::string percs = std::to_string(perc) + "%%";
                    printw(percs);
                }
            }
            

            move (height + 1, width - 18);
            std::string y = std::to_string(cursor_y + offset + 1);
            std::string x = std::to_string(std::min(cursor_x + 1, std::max(static_cast<int>(lines[cursor_y+offset].size()), 1)));
            std::string loc = y + "," + x;
            printw(loc);
            if(cmdstr != "-- INSERT --") move(cursor_y, std::min(cursor_x, std::max(static_cast<int>(lines[cursor_y + offset].size() - 1), 0)));
            else move(cursor_y, std::min(cursor_x, std::max(static_cast<int>(lines[cursor_y + offset].size()), 0)));
        }
        else {
            move(cursor_y, std::min(cursor_x, std::max(width - 1, 0)));
        }
        refresh();
    }
};

#endif