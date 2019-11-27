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
    bool filechange = false;
    std::string filename;
    std::vector<std::string> lines;
    std::string cmdstr = "";
    std::string numcmd = "";
    std::string savedchange = "";
    std::vector<std::string> buffer;
    int repeats = 0;
    int offset = 0;
    int cursor_x = 0;
    int cursor_y = 0;
    std::vector<std::pair <std::pair<int, int>, int>> prevloc;
    std::vector<std::pair<std::pair <int, int>, std::vector<std::string>>> undostack; 
    std::vector<std::string> comparable;
    int backmovecount = 0;

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
                if(cursor_y != views[0]->getHeight() + offset && offset + views[0]->getHeight() != static_cast<int>(lines.size()) - 1) scrl(1);
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
            if(ch == KEY_BACKSPACE) { // delete last element
                lines[cur_line]  = lines[cur_line].substr(0, cursor_x - 1);
                --cursor_x;
                clearline();
            } else if(ch == 10) {
                lines.insert(lines.begin() + cur_line + 1, "");
                cursor_x = 0;
                if(cursor_y == views[0]->getHeight()) ++offset;
                else ++cursor_y;
                clear();
            } else if(ch == KEY_DC) {
                if(cursor_y + offset == static_cast<int>(lines.size() - 1)) return; // do nothing if at end of file
                ++cursor_y;
                cursor_x = 0;
                interpret_input(KEY_BACKSPACE);
                return;
            } else { // just insert character
                lines[cur_line] += ch;
                ++cursor_x;
            }
        }
        else if(ch == KEY_DC) { // delete key == move cursor foward and backspace
            ++cursor_x;
            interpret_input(KEY_BACKSPACE);
        }
        else if(ch == KEY_BACKSPACE) { // Backspace key
            if(cursor_x == 0) {
                if(cur_line) { 
                    cursor_x = static_cast<int>(lines[cur_line - 1].size());
                    lines[cur_line - 1] = lines[cur_line - 1].append(lines[cur_line]);
                    lines.erase(lines.begin() + cur_line);
                    debug();
                    if((cursor_y > 5 && offset != static_cast<int>(lines.size()) - views[0]->getHeight()) || offset == 0) --cursor_y;
                    else --offset;
                    clear();
                    --backmovecount;
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
                lines[cur_line] += ch;
                cursor_x = 1;
            } else {
                lines[cur_line] = lines[cur_line].substr(0,cursor_x) + static_cast<char>(ch) + lines[cur_line].substr(cursor_x, static_cast<int>(lines[cur_line].size()) - cursor_x);
                ++cursor_x;
            }
        }
        if(cur_line || cursor_x || ch != KEY_BACKSPACE) filechange = true;
    }

    void addBotCharacter(int ch) {
        if (ch == KEY_BACKSPACE) {
            cmdstr = cmdstr.substr(0, cmdstr.size() - 1);
            --cursor_x;
        }
        else {
            cmdstr += static_cast<char>(ch);
            ++cursor_x;
        }
    }

    void botCommand(std::string cmd) {
        std::string c = cmd.substr(1, cmd.size() - 1);
        if(cmd == ":wq") {
            botinsert_mode = false;
            complete = true;
            cmdstr = "";
            save_file();                    
        } 
        else if(cmd == ":q!") {
            botinsert_mode = false;
            complete = true;
            cmdstr = "";
        }
        else if(cmd == ":w") {
            botinsert_mode = false;
            cmdstr = "";
            save_file();
        }
        else if(cmd == ":q") {
            botinsert_mode = false;
            if (filechange == true) {
                cmdstr = "E37: No write since last change (add ! to override)";
            }
            else {
                complete = true;
                cmdstr = "";
            }
        }
        else if(isnum(c) && cmd[0] == ':') {
            botinsert_mode = false;
            cmdstr = "";
            prevloc.back().first.first = 0;
            prevloc.back().second = std::min(stoi(c), std::max(static_cast<int>(lines.size() - 1), 0));
        }
        else if(cmd == ":$"){
            botinsert_mode = false;
            cmdstr = "";
            prevloc.back().first.first = 0;
            prevloc.back().second = std::max(static_cast<int>(lines.size() - 1), 0);
        }
    }

    void updateViews() {
        for(auto &i : views) i->updateView();
    }
    void displayViews() {
        updateViews();
        if (botinsert_mode) {
            for(auto &i : views) i->displayView(lines, cursor_y, cursor_x, offset, cmdstr, numcmd);
        }
        else {
            for(int i = views.size() - 1; i >= 0; --i) {
                views[i]->displayView(lines, cursor_y, cursor_x, offset, cmdstr, numcmd);
            }
        }
    }

    void goinsert() {
        insert_mode = true;
        cmdstr = "-- INSERT --";
        comparable = lines;
        backmovecount = 0;
    }

    void savecursor() {
        std::pair <std::pair<int, int>, int> cursave;
        cursave.first.first = cursor_x;
        cursave.first.second = cursor_y;
        cursave.second = cursor_y + offset;
        prevloc.push_back(cursave);
    }

    void returncursor() {
        cursor_y = prevloc.back().first.second;
        cursor_x = prevloc.back().first.first;
        int line = prevloc.back().second;
        while (cursor_y + offset < line) {
            cursor_down();
        }
        while (cursor_y + offset > line) {
            cursor_up();
        }
        prevloc.pop_back();
    }

    void comparesaves() {
        int mxs = std::min(static_cast<int>(comparable.size()), static_cast<int>(lines.size()));
        std::pair<std::pair<int, int>, std::vector<std::string>> save;
        int linestart = prevloc.back().second + backmovecount; 
        int i = linestart;
        if(comparable.size() == lines.size()) {
            while(comparable[i] == lines[i]) {
                if(i == mxs - 1) {
                    save.first.first = -1;
                    save.first.second = -1;
                    undostack.push_back(save);
                    return;
                }
                ++i;
            } 
        }
        else {
            while(comparable[i] == lines[i]) {
                if(i == mxs - 1) break;
                ++i;
            } 
        }
        save.first.first = i;
        while (i < mxs) {
            if(comparable[i] != lines[i]) {
                save.first.second = i;
            }
            save.second.push_back(comparable[i]);
            ++i;
        }
        // if comparable is longer
        while (i < static_cast<int>(comparable.size())) {
            save.first.second = i;
            save.second.push_back(comparable[i]);
            ++i;
        }
        undostack.push_back(save);
    }

    void undo() {
        int start = undostack[undostack.size()-1].first.first;
        int end = undostack[undostack.size()-1].first.second;
        std::vector<std::string> change = undostack[undostack.size()-1].second;
        if (start == end) {
            lines[start] = change[0];
        }
        else {
            std::vector<std::string> tmp;
            for (int i = 0; i < static_cast<int>(change.size()) + start; ++i) {
                if(i < start) {
                    tmp.push_back(lines[i]);
                }
                else {
                    tmp.push_back(change[i-start]);
                }
            }
            lines = tmp;
        }
        undostack.pop_back();
    }

    void debug() {
        std::ofstream myfile;
        myfile.open("out.txt");
        // int start = undostack[undostack.size() - 1].first.first;
        // int end = undostack[undostack.size() - 1].first.second;
        // std::vector<std::string> change = undostack[undostack.size()-1].second;
        myfile << offset << std::endl;
        myfile << static_cast<int>(lines.size()) - 1 << std::endl;
        myfile << views[0]->getHeight() << std::endl;
        // myfile << start << " " << end << std::endl;
        // for (int i = 0; i < static_cast<int>(change.size()) + start; ++i) {
        //         if(i < start) {
        //             myfile << lines[i] << std::endl;
        //         }
        //         else {
        //             myfile << change[i-start] << std::endl;
        //         }
        //     }
        myfile.close();
    }

    void repeatsave() {
        for(int i = 1; i < repeats; ++i) {
            for(int j = 0; j < static_cast<int>(savedchange.size()); ++j) {
                addCharacter(savedchange[j]);
            }
        }
        savedchange = "";
        repeats = 0;
    }

    void undocommand() {
        if (!undostack.empty()) {
            undo();
            returncursor();
            if(undostack.empty()) {
                filechange = false;
            }
        }
        else {
            cmdstr = "Already at oldest change";
        }
    }

    void interpret_input(int ch = 0) { // make it possible to do command not from keyboard
        if(ch == 0) {
            cntrl->genAction();
            ch = cntrl->getAction()->getchar();
        } 
        if(isdigit(ch) && !botinsert_mode && !insert_mode) {
            numcmd += ch;
        }
        else {
            if(numcmd != "") {
                repeats = stoi(numcmd);
            }
            numcmd = "";
        }

        if(ch == 27) { // escape key
            cmdstr = "";
            if (botinsert_mode) {
                botinsert_mode = false;
                clearbottom(views[0]->getHeight());
                returncursor();
            }
            if(insert_mode) {
                repeatsave();
                if(cursor_x != static_cast<int>(lines[cursor_y + offset].size())) cursor_x = std::min(cursor_x, std::max(static_cast<int>(lines[cursor_y + offset].size()) - 1, 0));
                cursor_x = std::max(cursor_x - 1, 0);
                insert_mode = false;
                comparesaves();
            }
        }
        else if(insert_mode) {
            if(cursor_x != static_cast<int>(lines[cursor_y + offset].size())) cursor_x = std::min(cursor_x, std::max(static_cast<int>(lines[cursor_y + offset].size() - 1), 0)); // Fix cursor_x constant
            savedchange += ch;
            addCharacter(ch);
        }
        else if(botinsert_mode) {
            if(ch == 10) { // Pressed enter, do command
                botCommand(cmdstr);
                clearbottom(views[0]->getHeight());
                returncursor();
            } else if(cmdstr.size() == 1 && ch == KEY_BACKSPACE) { // backspace out of command
                cmdstr = "";
                botinsert_mode = false;
                clearbottom(views[0]->getHeight());
                returncursor();
            } else {
                addBotCharacter(ch);
            }
        }
        else if(ch == 'i') {
            cursor_x = std::min(cursor_x, std::max(static_cast<int>(lines[cursor_y + offset].size()) - 1, 0));
            goinsert();
            savecursor();
        }
        else if(ch == 'I') {
            cursor_x = 0;
            goinsert();
            savecursor();
        }
        else if(ch == ':') {
            cmdstr = ":";
            savecursor();
            cursor_y = views[0]->getHeight() + 1;
            cursor_x = 1;
            botinsert_mode = true;
        }
        else if(ch == 'h') { 
            cursor_left();
            for(int i = 1; i < repeats; ++i) {
                cursor_left();
            }
            repeats = 0;
        }
        else if(ch == 'j') {
            cursor_down();
            for(int i = 1; i < repeats; ++i) {
                cursor_down();
            }
            repeats = 0;
        }
        else if(ch == 'k') {
            cursor_up();
            for(int i = 1; i < repeats; ++i) {
                cursor_up();
            }
            repeats = 0;
        }
        else if(ch == 'l') {
            cursor_right();
            for(int i = 1; i < repeats; ++i) {
                cursor_right();
            }
            repeats = 0;
        }
        else if(ch == 'u') {
            undocommand();
            for(int i = 1; i < repeats; ++i) {
                undocommand();
            }
            repeats = 0;
        }
        else if(ch == 'a') {
            if(cursor_x < static_cast<int>(lines[cursor_y + offset].size())) ++cursor_x;
            goinsert();
            savecursor();
        }
        else if(ch == 'A') {
            cursor_x = lines[cursor_y + offset].size();
            goinsert();
            savecursor();
        }
    }
};

#endif
