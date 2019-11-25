#ifndef MODEL_H
#define MODEL_H
#include "../view/view.h"
#include "../controller/controller.h"
#include <string>
#include <vector>

class Model {
  public:
    std::vector<View*> views;
    Controller *cntrl;
    virtual void addView(View *v) = 0;
    virtual void addController(Controller *c) = 0;
    virtual void updateViews() = 0;
    virtual void displayViews() = 0;
};


class Logic : public Model {
    public:
    bool complete = false;
    bool insert_mode = false; // True if insert mode, false if command mode
    bool botinsert_mode = false;
    std::string filename;
    std::vector<std::string> lines;
    std::string cmdstr = "";
    int offset = 0;
    int cursor_x = 0;
    int cursor_y = 0;
    std::pair<int, int> prevloc;

    void addView(View *v) override {
        views.push_back(v);
    }

    void addController(Controller *c) override {
        cntrl = c;
    }

    void save_file() {
        std::ofstream myfile;
        myfile.open(filename);
        for(size_t i = 0; i < lines.size(); ++i) {
            myfile << lines[i];
            if(i != lines.size() - 1) myfile << "\n";
        }
        myfile.close();
    }

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
                if(cursor_y != views[0]->getHeight() + offset) scrl(1);
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
        if((cursor_x == static_cast<int>(lines[cur_line].size())) && !lines[cur_line].empty()) { // insert mode at end of string
            if(ch == 127) { // delete last element
                lines[cur_line]  = lines[cur_line].substr(0, cursor_x - 1);
                --cursor_x;
                clearline();
            } else if(ch == 10) {
                lines.insert(lines.begin() + cur_line + 1, "");
                cursor_x = 0;
                if(cursor_y == views[0]->getHeight()) ++offset;
                else ++cursor_y;
                clear();
            } else { // just insert character
                lines[cur_line] += ch;
                ++cursor_x;
            }
        }
        else if(ch == 127) { // Backspace key
            if(cursor_x == 0) {
                if(cur_line) { 
                    cursor_x = static_cast<int>(lines[cur_line - 1].size());
                    lines[cur_line - 1] = lines[cur_line - 1].append(lines[cur_line]);
                    lines.erase(lines.begin() + cur_line);
                    if(cursor_y > 0) --cursor_y;
                    else --offset;
                    clear();
                }
            } else {
                lines[cur_line]  = lines[cur_line].substr(0, cursor_x - 1) + lines[cur_line].substr(cursor_x, static_cast<int>(lines[cur_line].size() - cursor_x));
                --cursor_x;
                clearline();
            }
        } else if(ch == 10) { // Enter key
            lines.insert(lines.begin() + cur_line + 1, lines[cur_line].substr(cursor_x, lines[cur_line].size() - cursor_x));
            lines[cur_line] = lines[cur_line].substr(0, cursor_x);
            cursor_x = 0;
            if(cursor_y == views[0]->getHeight()) ++offset;
            else ++cursor_y;
            clear();
        } else { 
            if(lines[cur_line].size() == 0) {
                std::string new_line = "";
                new_line += ch;
                new_line += " ";
                lines[cur_line] = new_line;
                cursor_x = 1;
            } else {
                lines[cur_line] = lines[cur_line].substr(0,cursor_x) + static_cast<char>(ch) + lines[cur_line].substr(cursor_x, static_cast<int>(lines[cur_line].size()) - cursor_x);
                ++cursor_x;
            }
        }
    }

    void addBotCharacter(int ch) {
        if (ch == 127) {
            cmdstr = cmdstr.substr(0, cmdstr.size() - 1);
            --cursor_x;
        }
        else {
            cmdstr += static_cast<char>(ch);
            ++cursor_x;
        }
    }

    void updateViews() {
        for(auto &i : views) i->updateView();
    }
    void displayViews() {
        updateViews();
        for(auto &i : views) i->displayView(lines, cursor_y, cursor_x, offset, cmdstr);
    }
    void interpret_input() {
        cntrl->genAction();
        int ch = cntrl->getAction()->getchar();
        if(ch == 27) {
            insert_mode = false; // escape key
            cmdstr = "";
            botinsert_mode = false;
            cursor_x = std::min(cursor_x, std::max(static_cast<int>(lines[cursor_y + offset].size()) - 1, 0));
            clearbottom(views[0]->getHeight());
        }
        else if(insert_mode) {
            cursor_x = std::min(cursor_x, std::max(static_cast<int>(lines[cursor_y + offset].size()), 0)); // Fix cursor_x constant
            addCharacter(ch);
        }
        else if(botinsert_mode) {
            if(ch == 10) { // Pressed enter, do command
                if(cmdstr == ":wq") {
                    botinsert_mode = false;
                    complete = true;
                    cmdstr = "";
                    save_file();                    
                } else if(cmdstr == ":q!") {
                    botinsert_mode = false;
                    complete = true;
                    cmdstr = "";
                }
                cursor_y = prevloc.second;
                cursor_x = prevloc.first;
            } else if(cmdstr.size() == 1 && ch == 127) { // backspace out of command
                cmdstr = "";
                botinsert_mode = false;
                clearbottom(views[0]->getHeight());
                cursor_y = prevloc.second;
                cursor_x = prevloc.first;
            } else {
                addBotCharacter(ch);
            }
        }
        else if(ch == 'i') {
            insert_mode = true;
            cmdstr = "-- INSERT --";
        }
        else if(ch == ':') {
            cmdstr = ":";
            prevloc.first = cursor_x;
            prevloc.second = cursor_y;
            cursor_y = views[0]->getHeight() + 1;
            cursor_x = 1;
            botinsert_mode = true;
        }
        else if(ch == 'h') cursor_left();
        else if(ch == 'j') cursor_down();
        else if(ch == 'k') cursor_up();
        else if(ch == 'l') cursor_right();
    }
};

#endif
