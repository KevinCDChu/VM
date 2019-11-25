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
        clear();
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


class Model {
  public:
    std::vector<View*> views;
    //Controller cntrl;
    //virtual void addView(View *v) = 0;
    //virtual void addController(Controller);
    virtual void updateViews() = 0;
    virtual void displayViews() = 0;
};


class Logic : public Model {
    public:
    bool insert_mode = false; // True if insert mode, false if command mode
    bool botinsert_mode = false;
    std::vector<std::string> lines;
    std::string cmdstr = "";
    int offset = 0;
    int cursor_x = 0;
    int cursor_y = 0;

    void cursor_up() { // move cursor up
    if(cursor_y > 0 && cursor_y <= 5) { 
            if(offset > 0) {
                offset -= 1;
                scrl(-1);
            }
        if(offset == 0) --cursor_y;
        } else cursor_y = std::max(cursor_y - 1, 0);
    }

    void cursor_down() { // move cursor down
            if(views[0]->getHeight() - cursor_y <= 5) {
                scrl(1);
                if(offset + views[0]->getHeight() < static_cast<int>(lines.size()) - 1) offset += 1;
                if(offset + views[0]->getHeight() == static_cast<int>(lines.size()) - 1) cursor_y = std::min(cursor_y + 1, views[0]->getHeight());
            }
            else cursor_y = std::min(std::min(cursor_y + 1, views[0]->getHeight()), static_cast<int>(lines.size()) - 1 - offset);
        }

    void cursor_left() { // move cursor left
        if(cursor_x > 0) {
            cursor_x = std::min(cursor_x - 1, std::max(static_cast<int>(lines[cursor_y + offset].size() - 2), 0)); // Take min as we might still be greater from previous line
        }
    }

    void cursor_right()  { // move cursor right
        if(cursor_x < static_cast<int>(lines[cursor_y + offset].size()) - 1) ++cursor_x;
    }


    void addCharacter(int ch) {
        int cur_line = cursor_y + offset;
        if(ch == 127) {
            if(cursor_x == 0) {
                if(cur_line) { 
                    cursor_x = static_cast<int>(lines[cur_line - 1].size());
                    lines[cur_line - 1] = lines[cur_line - 1].append(lines[cur_line]);
                    lines.erase(lines.begin() + cur_line);
                    if(cursor_y > 0) --cursor_y;
                    else --offset;
                }
            } else {
                lines[cur_line]  = lines[cur_line].substr(0, cursor_x - 1) + lines[cur_line].substr(cursor_x, static_cast<int>(lines[cur_line].size() - cursor_x));
                --cursor_x;
            }
        clear();
        } else if(ch == 10) {
            lines.insert(lines.begin() + cur_line + 1, lines[cur_line].substr(cursor_x, lines[cur_line].size() - cursor_x));
            lines[cur_line] = lines[cur_line].substr(0, cursor_x);
            cursor_x = 0;
            ++cursor_y;
            clear();
        } else { 
            if(lines[cur_line].size() == 0) lines[cur_line] = std::to_string(ch);
            else lines[cur_line] = lines[cur_line].substr(0,cursor_x) + static_cast<char>(ch) + lines[cur_line].substr(cursor_x, static_cast<int>(lines[cur_line].size()) - cursor_x);
            ++cursor_x;
        }
    }

    void addBotCharacter(int ch) {
        if (ch == 127) {
            cmdstr = cmdstr.substr(0, cmdstr.size() - 1);
        }
        else {
            cmdstr += static_cast<char>(ch);
        }
    }

    void addView(View *v) { views.push_back(v); }
    void updateViews() {
        for(auto &i : views) i->updateView();
    }
    void displayViews() {
        updateViews();
        for(auto &i : views) i->displayView(lines, cursor_y, cursor_x, offset, cmdstr);
    }
    void interpret_input(int ch) {
        if(ch == 27) {
            insert_mode = false; // escape key
            cmdstr = "";
            botinsert_mode = false;
            clear();
        }
        else if(insert_mode) {
            cursor_x = std::min(cursor_x, std::max(static_cast<int>(lines[cursor_y + offset].size() - 1), 0));
            addCharacter(ch);
        }
        else if(botinsert_mode) {
            if(ch == '\n') {
                cmdstr = "";
                botinsert_mode = false;
                clear();
            }
            else {
                addBotCharacter(ch);
            }
        }
        else if(ch == 'i') insert_mode = true;
        else if(ch == ':') {
            cmdstr = ":";
            botinsert_mode = true;
        }
        else if(ch == 'h') cursor_left();
        else if(ch == 'j') cursor_down();
        else if(ch == 'k') cursor_up();
        else if(ch == 'l') cursor_right();
    }
};






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
