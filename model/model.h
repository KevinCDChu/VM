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
    bool replace_mode = false; // will affect how backspace and adding characters work in insert mode
    bool filechange = false;
    std::string filename;
    std::vector<std::string> lines;
    std::string cmdstr = "";
    std::string numcmd = "";
    std::string savedchange = "";
    std::vector<std::string> buffer = {"hello there"};
    int repeats = 0;
    int offset = 0;
    int cursor_x = 0;
    int cursor_y = 0;
    std::vector<std::pair <std::pair<int, int>, int>> prevloc;
    std::vector<std::pair<std::pair <int, int>, std::vector<std::string>>> undostack; 
    std::vector<std::string> comparable;
    std::string prevpattern = "";
    std::pair<int, int> prevchar;
    int backmovecount = 0;
    std::vector<std::string> entire_file_buffer;
    bool insert_did_something = false; // true if insert actually did something


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
            if(ch == KEY_BACKSPACE && !replace_mode) { // delete last element
                lines[cur_line]  = lines[cur_line].substr(0, cursor_x - 1);
                --cursor_x;
                clearline();
            } else if(ch == KEY_BACKSPACE && replace_mode) {
                if((cur_line < static_cast<int>(comparable.size()) && cursor_x > static_cast<int>(comparable[cur_line].size())) || cur_line >= static_cast<int>(comparable.size())) { // do normal backspace in this case
                    lines[cur_line] = lines[cur_line].substr(0, cursor_x - 1);
                    --cursor_x;
                    clearline();
                } else { // decrement cursor and just add back character from 
                    lines[cur_line][cursor_x] = comparable[cur_line][cursor_x];
                    --cursor_x;
                    clearline(); 
                }
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
            if(lines[cur_line].empty()) {
                if(cursor_y + offset == static_cast<int>(lines.size() - 1)) return; // do nothing if at end of file
                ++cursor_y;
                cursor_x = 0;
                interpret_input(KEY_BACKSPACE);
            } else {
            ++cursor_x;
            interpret_input(KEY_BACKSPACE);
            }
        }
        else if(ch == KEY_BACKSPACE) { // Backspace key
            if(cursor_x == 0) {
                if(replace_mode && cur_line) {
                    --cursor_y;
                    cursor_x = static_cast<int>(lines[cursor_y + offset].size());
                    --backmovecount;
                }
                else if(cur_line) { 
                    cursor_x = static_cast<int>(lines[cur_line - 1].size());
                    lines[cur_line - 1] = lines[cur_line - 1].append(lines[cur_line]);
                    lines.erase(lines.begin() + cur_line);
                    if((cursor_y > 5 && offset != static_cast<int>(lines.size()) - views[0]->getHeight()) || offset == 0) --cursor_y;
                    else --offset;
                    clear();
                    --backmovecount;
                }
            } else {
                if(replace_mode) { // just add back charcter that was there
                    if((cur_line < static_cast<int>(comparable.size()) && cursor_x >= static_cast<int>(comparable[cur_line].size())) || cur_line >= static_cast<int>(comparable.size())) { // do normal backspace in this case
                    lines[cur_line] = lines[cur_line].substr(0, cursor_x - 1);
                    --cursor_x;
                    clearline();
                    } else { // decrement cursor and just add back character from 
                    lines[cur_line][cursor_x - 1] = comparable[cur_line][cursor_x - 1];
                    --cursor_x;
                    clearline(); 
                }
            } else {
                lines[cur_line]  = lines[cur_line].substr(0, cursor_x - 1) + lines[cur_line].substr(cursor_x, static_cast<int>(lines[cur_line].size() - cursor_x));
                --cursor_x;
                clearline();
            } }
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
                if(replace_mode) {
                    if(cursor_x == static_cast<int>(lines[cur_line].size() - 1)) lines[cur_line][cursor_x] = ch;
                    else lines[cur_line] = lines[cur_line].substr(0, cursor_x) + static_cast<char>(ch) + lines[cur_line].substr(cursor_x + 1, static_cast<int>(lines[cur_line].size() - cursor_x - 1));
                    cursor_x += 1;
                } else {
                lines[cur_line] = lines[cur_line].substr(0,cursor_x) + static_cast<char>(ch) + lines[cur_line].substr(cursor_x, static_cast<int>(lines[cur_line].size()) - cursor_x);
                ++cursor_x;
                }
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

    void forwardsearch(std::string s) {
        int curline = prevloc.back().first.second + offset;
        int prevline = curline;
        if(!lines[curline].empty()) {
            if(lines[curline].find(s, prevloc.back().first.first + 1) != std::string::npos) {
                prevloc.back().first.first = lines[curline].find(s, prevloc.back().first.first + 1);
                return;
            }
        }
        if(curline < static_cast<int>(lines.size())) {
            ++curline;
        }
        while(curline < static_cast<int>(lines.size())) {
            if(lines[curline].find(s) != std::string::npos) {
                prevloc.back().first.first = lines[curline].find(s);
                prevloc.back().second = curline;
                return;
            }
            ++curline;
        }
        curline = 0;
        while(curline != prevline) {
            if(lines[curline].find(s) != std::string::npos) {
                prevloc.back().first.first = lines[curline].find(s);
                prevloc.back().second = curline;
                cmdstr = "search hit BOTTOM, continuing at TOP";
                return;
            }
            ++curline;
        }
        if(!lines[curline].empty()) {
            if((lines[curline].substr(0, prevloc.back().first.first + 1)).find(s)  != std::string::npos) {
                prevloc.back().first.first = (lines[curline].substr(0, prevloc.back().first.first + 1)).find(s);
                cmdstr = "search hit BOTTOM, continuing at TOP";
                return;
            }
        }
        cmdstr = "E486: Pattern not found: " + s;
    }

    void backwardsearch(std::string s) {
       int curline = prevloc.back().first.second + offset;
        int prevline = curline;
        if(!lines[curline].empty()) {
            if(prevloc.back().first.first != 0 && lines[curline].rfind(s, prevloc.back().first.first - 1) != std::string::npos) {
                prevloc.back().first.first = lines[curline].rfind(s, prevloc.back().first.first - 1);
                return;
            }
        }
        if(curline > -1) {
            --curline;
        }
        while(curline > -1) {
            if(lines[curline].rfind(s) != std::string::npos) {
                prevloc.back().first.first = lines[curline].rfind(s);
                prevloc.back().second = curline;
                return;
            }
            --curline;
        }
        curline = static_cast<int>(lines.size()) - 1;
        while(curline != prevline) {
            if(lines[curline].rfind(s) != std::string::npos) {
                prevloc.back().first.first = lines[curline].rfind(s);
                prevloc.back().second = curline;
                cmdstr = "search hit TOP, continuing at BOTTOM";
                return;
            }
            --curline;
        }
        if(!lines[curline].empty()) {
            if((lines[curline].substr(prevloc.back().first.first + 1, lines[curline].size() - prevloc.back().first.first - 1)).find(s) != std::string::npos) {
                prevloc.back().first.first = (lines[curline].substr(prevloc.back().first.first + 1, lines[curline].size() - prevloc.back().first.first - 1)).find(s);
                cmdstr = "search hit TOP, continuing at BOTTOM";
                return;
            }
        }
        cmdstr = "E486: Pattern not found: " + s;
    } 

    void botCommand(std::string cmd) {
        std::string c;
        if(cmd.size() > 1) c = cmd.substr(1, cmd.size() - 1);
        if (cmd.size() > 1 && cmd[0] == '/') {
            forwardsearch(c);
            for(int i = 1; i < repeats; ++i) {
                returncursor();
                savecursor();
                forwardsearch(c);
            }
            if(cmdstr[0] == '/') {
                cmdstr = "";
            }
            prevpattern = cmd;
            repeats = 0;
        }
        else if (cmd.size() > 1 && cmd[0] == '?'){
            backwardsearch(c);
            for(int i = 1; i < repeats; ++i) {
                returncursor();
                savecursor();
                backwardsearch(c);
            }
            if(cmdstr[0] == '?') {
                cmdstr = "";
            }
            prevpattern = cmd;
            repeats = 0;
        }
        else if(cmd == ":wq") {
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
        else if(cmd.size() > 1 && cmd.substr(0,2) == ":r") {
            bool read_in_current_file = true; // if no file specified, then just copy current filename
            for(size_t i = 2; i < cmd.size(); ++i) {
                if(cmd[i] != ' ') {read_in_current_file = false; break;}
            }
            botinsert_mode = false;
            filechange = true;
            cmdstr = "";
            std::string file = cmd.substr(2, cmd.size() - 2);
            if(read_in_current_file) file = " " + filename;
            bool worked = true;
            read_into_file_buffer(file, worked);
            if(worked) {
                std::vector<std::string> temp_buffer = buffer;
                buffer = entire_file_buffer;
                comparable = lines;
                cursor_y = prevloc.back().first.second;
                cursor_x = prevloc.back().first.first;
                offset = prevloc.back().second - cursor_y;
                savecursor();
                for(int i = 0; i < static_cast<int>(entire_file_buffer.size()); ++i) {
                    lines.insert(lines.begin() + cursor_y + offset + i + 1, entire_file_buffer[i]);
                }
                comparesaves();
                prevloc.back().first.second = cursor_y + 1;
                prevloc.back().first.first = 0;
                prevloc.back().second = offset + cursor_y + 1;
                buffer = temp_buffer;
                cmdstr = ""; 
            }       
            botinsert_mode = false;
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
        else if(c != "" && isnum(c) && cmd[0] == ':') {
            botinsert_mode = false;
            cmdstr = "";
            prevloc.back().first.first = 0;
            prevloc.back().second = std::min(std::max(stoi(c) - 1, 0), std::max(static_cast<int>(lines.size() - 1), 0));
        }
        else if(cmd == ":$"){
            botinsert_mode = false;
            cmdstr = "";
            prevloc.back().first.first = 0;
            prevloc.back().second = std::max(static_cast<int>(lines.size() - 1), 0);
        }
        else {
            botinsert_mode = false;
            if(cmdstr[0] == '/' || cmdstr[0] == '?') {
                cmdstr = "E486: Pattern not found: " + c;
            }
            cmdstr = "E492: Not an editor command: " + c;
            
        }
    }

    void read_into_file_buffer(std::string file_name, bool &worked) {
        if(file_name != "") file_name = file_name = file_name.substr(1, file_name.size());
        if(file_exists(file_name)) {
                std::ifstream f{file_name};
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
        entire_file_buffer = lines; 
        } else {
            worked = false;
            cmdstr = "E484: Can't open file " + file_name;
        }
    }

    void wordback() {
        int i = cursor_x;
        int curline = cursor_y + offset;
        if (i != 0) {
            --i;
            cursor_x = i;
            while(i != 0 && isspace(lines[curline][i])) {
                --i;
            }
            if (i == 0 && isspace(lines[curline][i])) {
                cursor_up();
                if (static_cast<int>(lines[curline - 1].size()) == 0) {
                    cursor_x = 0;
                }
                else if (curline) {
                    cursor_x = static_cast<int>(lines[curline - 1].size());
                    wordback();
                }
            }
            else {
                if(isWord(lines[curline][i])) {
                    while(i != 0 && isWord(lines[curline][i-1])) {
                        --i;
                    }
                cursor_x = i;
                }
                else {
                    while(i != 0 && !isWord(lines[curline][i-1]) && !isspace(lines[curline][i-1])) {
                        --i;
                    }
                    cursor_x = i;
                }
            }
        }
        else if (curline) {
            cursor_up();
            if (static_cast<int>(lines[curline - 1].size()) == 0) {
                cursor_x = 0;
            }
            else {
                cursor_x = static_cast<int>(lines[curline - 1].size());
                wordback();
            }
        }
    }

    void wordforward() {
        int i = cursor_x;
        int curline = cursor_y + offset;
        int end = std::max(static_cast<int>(lines[curline].size()) - 1, 0);
        if (i != end) {
            int curchar;
            if (i == -1) curchar = '\n';
            else curchar = lines[curline][i];
            ++i;
            if(isspace(curchar)) {
                while(i != end && isspace(lines[curline][i])) {
                    ++i;
                }
                if(curline != std::max(static_cast<int>(lines.size()) - 1, 0) && i == end && isspace(lines[curline][i])) {
                    cursor_down();
                    cursor_x = 0;
                    if (static_cast<int>(lines[curline + 1].size()) != 0 && isspace(lines[curline + 1][0])) {
                        wordforward();
                    }
                }
                else {
                    cursor_x = i;
                }
            }
            else if(isWord(curchar)) {
                bool space = false;
                while(i != end && isWord(lines[curline][i])) {
                    ++i;
                }
                while(i != end && isspace(lines[curline][i])) {
                    ++i;
                    space = true;
                }
                if(isPunc(lines[curline][i])) {
                    space = true;
                }
                if(curline != std::max(static_cast<int>(lines.size()) - 1, 0) && i == end && (!space || isspace(lines[curline][i]))) {
                    cursor_down();
                    cursor_x = 0;
                    if (static_cast<int>(lines[curline + 1].size()) != 0 && isspace(lines[curline + 1][0])) {
                        wordforward();
                    }
                }
                else {
                    cursor_x = i;
                }
            }
            else {
                bool space = false;
                while(i != end && isPunc(lines[curline][i])) {
                    ++i;
                }
                while(i != end && isspace(lines[curline][i])) {
                    ++i;
                    space = true;
                }
                if(isWord(lines[curline][i])) {
                    space = true;
                }
                if(curline != std::max(static_cast<int>(lines.size()) - 1, 0) && i == end && (!space || isspace(lines[curline][i]))) {
                    cursor_down();
                    cursor_x = 0;
                    if (static_cast<int>(lines[curline + 1].size()) != 0 && isspace(lines[curline + 1][0])) {
                        wordforward();
                    }
                }
                else {
                    cursor_x = i;
                }
            }
        }
        else if (curline != std::max(static_cast<int>(lines.size()) - 1, 0)) {
            cursor_down();
            cursor_x = 0;
            if (static_cast<int>(lines[curline + 1].size()) != 0) {
                cursor_x = -1;
                wordforward();
            }
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
        if(replace_mode) cmdstr = "-- REPLACE --";
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
        int linestart = std::max(prevloc.back().second + backmovecount, 0);
        int i = linestart;
        if(comparable.size() == lines.size()) {
            while(comparable[i] == lines[i]) {
                if(i == mxs - 1) {
                    //return; will delete this so that empty changes still are undoable
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

    void get_bracket_pairs(std::vector<std::pair<std::pair<int,int>,std::pair<int,int>>> &parentheses, std::vector<std::pair<std::pair<int,int>,std::pair<int,int>>> &brackets, std::vector<std::pair<std::pair<int,int>, std::pair<int,int>>> &braces) {
        std::vector<std::pair<std::pair<int,int>,std::pair<int,int>>> temp_parentheses;
        std::vector<std::pair<std::pair<int,int>,std::pair<int,int>>> temp_brackets;
        std::vector<std::pair<std::pair<int,int>,std::pair<int,int>>> temp_braces;
        for(size_t i = 0; i < lines.size(); ++i) {
            for(size_t j = 0; j < lines[i].size(); ++j) {
                if(lines[i][j] == '(') {
                    std::pair<int,int> start = std::make_pair(i,j);
                    std::pair<int,int> end = std::make_pair(-1,-1);
                    temp_parentheses.push_back(std::make_pair(start, end));
                } else if(lines[i][j] == '[') {
                    std::pair<int,int> start = std::make_pair(i,j);
                    std::pair<int,int> end = std::make_pair(-1,-1);
                    temp_brackets.push_back(std::make_pair(start, end));
                } else if(lines[i][j] == '{') {
                    std::pair<int,int> start = std::make_pair(i,j);
                    std::pair<int,int> end = std::make_pair(-1,-1);
                    temp_braces.push_back(std::make_pair(start, end));
                } else if(lines[i][j] == '}' && !temp_braces.empty()) {
                    temp_braces[temp_braces.size() - 1].second.first = i;
                    temp_braces[temp_braces.size() - 1].second.second = j;
                    braces.push_back(temp_braces.back());
                    temp_braces.pop_back();
                } else if(lines[i][j] == ']' && !temp_brackets.empty()) {
                    temp_brackets[temp_brackets.size() - 1].second.first = i;
                    temp_brackets[temp_brackets.size() - 1].second.second = j;
                    brackets.push_back(temp_brackets.back());
                    temp_brackets.pop_back();
                } else if(lines[i][j] == ')' && !temp_parentheses.empty()) {
                    temp_parentheses[temp_parentheses.size() - 1].second.first = i;
                    temp_parentheses[temp_parentheses.size() - 1].second.second = j;
                    parentheses.push_back(temp_parentheses.back());
                    temp_parentheses.pop_back();
                } 
            }
        }
    }


    int get_best_from_pair(std::pair<std::pair<int,int>,std::pair<int,int>> &pair, int &line) {
        int best_start = 9999;
        int best_end = 9999;
        if(pair.first.first == line && pair.first.second >= cursor_x) best_start = pair.first.second;
        if(pair.second.first == line && pair.second.second >= cursor_x) best_end = pair.second.second;
        return(std::min(best_start, best_end));
    }


    bool better_pair(std::pair<std::pair<int,int>,std::pair<int,int>> &new_pair, std::pair<std::pair<int,int>,std::pair<int,int>> &old_pair, int line) {
        int best_new = get_best_from_pair(new_pair, line);
        int best_old = get_best_from_pair(old_pair, line);
        return best_new < best_old;        
   }

    void move_cursor_to_best_pair(std::vector<std::pair<std::pair<int,int>,std::pair<int,int>>> &parentheses, std::vector<std::pair<std::pair<int,int>,std::pair<int,int>>> &brackets, std::vector<std::pair<std::pair<int,int>, std::pair<int,int>>> &braces, int &cur_line) {
            std::pair<std::pair<int,int>,std::pair<int,int>> best_pair = std::make_pair(std::make_pair(9999,9999), std::make_pair(9999, 9999));
            for(auto &i : parentheses) {
                if(i.first.first == cur_line && better_pair(i, best_pair, i.first.first)) best_pair = i;
                if(i.second.first == cur_line && better_pair(i, best_pair, i.second.first)) best_pair = i;
            }
            for(auto &i : braces) {
                if(i.first.first == cur_line && better_pair(i, best_pair, i.first.first)) best_pair = i;
                if(i.second.first == cur_line && better_pair(i, best_pair, i.second.first)) best_pair = i;
            }
            for(auto &i : brackets) {
                if(i.first.first == cur_line && better_pair(i, best_pair, i.first.first)) best_pair = i;
                if(i.second.first == cur_line && better_pair(i, best_pair, i.second.first)) best_pair = i;
            }
            int x;
            int y;
            bool first_is_the_pair;
            if(best_pair.first.first != 9999) {
                if(best_pair.first.first != cur_line) {
                    x = best_pair.first.second; // open is the one on the line for sure
                    y = best_pair.first.first;
                    first_is_the_pair = true;
                }
                else if(best_pair.second.first != cur_line) {
                    x = best_pair.second.second;
                    y = best_pair.second.first;
                    first_is_the_pair = false;
                }
                else { // they are on the same line
                    if(best_pair.first.second >= cursor_x) {
                        x = best_pair.second.second;
                        y = best_pair.second.first;
                        first_is_the_pair = false;
                    } else {
                        x = best_pair.first.second;
                        y = best_pair.first.first;
                        first_is_the_pair = true;
                    }
                }
                size_t check_x;
                if(first_is_the_pair) check_x = best_pair.second.second;
                else check_x = best_pair.first.second;
                for(size_t i = cursor_x + 1; i < check_x; ++i) {
                    if(lines[cur_line][i] == '{' ||
                       lines[cur_line][i] == '}' ||
                       lines[cur_line][i] == '(' ||
                       lines[cur_line][i] == ')' ||
                       lines[cur_line][i] == '[' ||
                       lines[cur_line][i] == ']') return;
                }
                while(offset + cursor_y != y) {
                if(offset + cursor_y < y) cursor_down();
                else cursor_up();
            }
            cursor_x = x;
            }
            
    }





    void paste() {
        int cur_line = cursor_y + offset;
        int j = std::min(cursor_x, static_cast<int>(lines[cur_line].size()) - 1);
        std::string last_part;
        if(lines[cur_line] != "" && j != static_cast<int>(lines[cur_line].size() - 1)) {
                last_part = lines[cur_line].substr(j + 1, lines[cur_line].size() - j - 1);
                lines[cur_line] = lines[cur_line].substr(0, j + 1) + buffer[0];
            } else {
                lines[cur_line] += buffer[0];
        }
        for(int i = 1; i < static_cast<int>(buffer.size()); ++i) {
            lines.insert(lines.begin() + cur_line + i, buffer[i]);
        }
        lines[cur_line + buffer.size() - 1] += last_part;
        if(buffer.size() == 1) {
            cursor_x += static_cast<int>(buffer[0].size());
            //cursor_x = std::min(cursor_x, static_cast<int>(lines[cur_line].size()) - 1);
        } 
    }

    void undo() {
        int start = undostack[undostack.size()-1].first.first;
        //int end = undostack[undostack.size()-1].first.second;
        std::vector<std::string> change = undostack[undostack.size()-1].second;
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
        undostack.pop_back();
    }

    void debug(int i, int j, int k) {
        std::ofstream myfile;
        myfile.open("out.txt");
        myfile << "true" << i << std::endl;
        myfile << "true" << j << std::endl;
        myfile << "true" << k << std::endl;
        myfile.close();
    }

    void repeatsave() {
        for(int i = 1; i < repeats; ++i) {
            for(int j = 0; j < static_cast<int>(savedchange.size()); ++j) {
                if(savedchange[j] == 7) {
                    addCharacter(KEY_BACKSPACE);
                }
                else {
                    addCharacter(savedchange[j]);
                }
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

    void pagedown() {
        offset += views[0]->getHeight() - 1;
        offset = std::min(offset, std::max(0, static_cast<int>(lines.size()) - 6));
        cursor_y = 5;
        cursor_x = 0;
        while(cursor_x < static_cast<int>(lines[cursor_y+offset].size()) && isspace(lines[cursor_y+offset][cursor_x])) {
            ++cursor_x;
        }
        if(cursor_x == static_cast<int>(lines[cursor_y+offset].size())) {
            cursor_x = 0;
        }
    }

    void pageup() {
        if(offset != 0) {
            if(offset + 1 < std::max(views[0]->getHeight() - 5, 0)) {
                cursor_y = offset + 1;
                offset = 0;
            }
            else {
                offset -= views[0]->getHeight() - 1;
                offset = std::max(offset, 0);
                cursor_y = std::max(views[0]->getHeight() - 5, 0);
            }
            cursor_x = 0;
            while(cursor_x < static_cast<int>(lines[cursor_y+offset].size()) && isspace(lines[cursor_y+offset][cursor_x])) {
                ++cursor_x;
            }
            if(cursor_x == static_cast<int>(lines[cursor_y+offset].size())) {
                cursor_x = 0;
            }
        }
    }

    bool cmdf(int curline, int end, int ch) {
        if (cursor_x != end) {
            if (lines[curline].find(ch, cursor_x + 1) != std::string::npos) {
                cursor_x = lines[curline].find(ch, cursor_x + 1);
                return true;
            }
            return false;
        }
        return false;
    }

    bool cmdF(int curline, int end, int ch) {
        if (cursor_x != 0) {
            if (lines[curline].find(ch, cursor_x + 1) != std::string::npos) {
                cursor_x = lines[curline].find(ch, cursor_x + 1);
                return true;
            }
            return false;
        }
        return false;
    }

    bool cmdt (int curline, int end, int ch) {
        if (cursor_x != end) {
            if (lines[curline].find(ch, cursor_x + 1) != std::string::npos) {
                cursor_x = lines[curline].find(ch, cursor_x + 1) - 1;
                return true;
            }
            return false;
        }
        return false;
    }

    bool matchshowcmd(int ch) {
        if(ch == 'f') {
            return true;
        }
        else if(ch == 'F') {
            return true;
        }
        else if(ch == 't') {
            return true;
        } 
        else if(ch == 'r') {
            return true;
        }
        return false;
    }

    void interpret_showcmd(std::string num, int cmd, int ch) {
        if (!num.empty()) {
            repeats = stoi(num);
        }
        if (cmd != 0) {
            int curline = cursor_y + offset;
            int end = std::max(static_cast<int>(lines[curline].size()) - 1, 0);
            if (cmd == 'f') {
                savecursor();
                prevchar.first = 'f';
                prevchar.second = ch;
                if(!cmdf(curline, end, ch)) {
                    returncursor();
                    repeats = 0;
                    return;
                }
                for(int i = 1; i < repeats; ++i) {
                    if(!cmdf(curline, end, ch)) {
                        returncursor();
                        repeats = 0;
                        return;
                    }
                }
                repeats = 0;    
            }
            else if (cmd == 'F') {
                savecursor();
                prevchar.first = 'F';
                prevchar.second = ch;
                if(!cmdF(curline, end, ch)) {
                    returncursor();
                    repeats = 0;
                    return;
                }
                for(int i = 1; i < repeats; ++i) {
                    if(!cmdF(curline, end, ch)) {
                        returncursor();
                        repeats = 0;
                        return;
                    }
                }
                repeats = 0;
            }
            else if (cmd == 't') {
                savecursor();
                prevchar.first = 't';
                prevchar.second = ch;
                if(!cmdt(curline, end, ch)) {
                    returncursor();
                    repeats = 0;
                    return;
                }
                for(int i = 1; i < repeats; ++i) {
                    cursor_right();
                    if(!cmdt(curline, end, ch)) {
                        returncursor();
                        repeats = 0;
                        return;
                    }
                }
                repeats = 0;
            }
            else if (cmd == 'r') {
                if(num.empty()) repeats = 1;
                int j = std::min(cursor_x, std::max(0, static_cast<int>(lines[curline].size() - 1)));
                savecursor();
                comparable = lines;
                if(j + repeats > end + 1 || ch == KEY_BACKSPACE || ch == KEY_DC) {
                    returncursor();
                    repeats = 0;
                    return;
                }
                for(int i = 0; i < repeats; ++i) {
                    lines[curline][j + i] = ch;
                    if(ch == 10) {
                        std::string new_line = lines[cursor_y + offset].substr(j + repeats, static_cast<int>(lines[cursor_y + offset].size()) - j - repeats);
                        lines[cursor_y + offset] = lines[cursor_y + offset].substr(0, j);
                        lines.insert(lines.begin() + cursor_y + offset + 1, new_line);
                        cursor_down();
                        cursor_x = 0;
                        break;
                    }
                    if(i != 0) cursor_right();
                }
                repeats = 0;
                comparesaves();
            }
        }
    }

    void interpret_input(int ch = 0) { // make it possible to do command not from keyboard
        if(ch == 0) {
            cntrl->genAction();
            ch = cntrl->getAction()->getchar();
        } 

        if (!cmdstr.empty() && (cmdstr[0] == 'E' || cmdstr[0] == 's')) {
            cmdstr = "";
        }

        if((isdigit(ch) || (!containsletter(numcmd) && matchshowcmd(ch))) && !botinsert_mode && !insert_mode) {
            numcmd += ch;
            if (numcmd == "0") {
                numcmd = "";
            }
        }
        else {
            if(containsletter(numcmd)) {
                reformat_command(numcmd); // needed in case of double multipliers (like 3d4l)
                interpret_showcmd(numcmd.substr(0, numcmd.size()-1), numcmd[numcmd.size()-1], ch);
                numcmd = "";
                return;
            }
            else if (!numcmd.empty()) {
                interpret_showcmd(numcmd, 0, ch);
                if(ch == 10) {
                    repeats = 0;
                }
                numcmd = "";
            }
            numcmd = "";
            
        }

        if(ch == 27) { // escape key
            cmdstr = "";
            numcmd = "";
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
                replace_mode = false;
                if(insert_did_something) comparesaves();
                insert_did_something = false;
            }
            repeats = 0;
        }
        else if(insert_mode) {
            if(cursor_x != static_cast<int>(lines[cursor_y + offset].size())) cursor_x = std::min(cursor_x, std::max(static_cast<int>(lines[cursor_y + offset].size() - 1), 0)); // Fix cursor_x constant
            savedchange += ch;
            insert_did_something = true;
            addCharacter(ch);
        }
        else if(botinsert_mode) {
            if(ch == 10) { // Pressed enter, do command
                botCommand(cmdstr);
                clearbottom(views[0]->getHeight());
                returncursor();
                botinsert_mode = false;
                // if :r command we need to display file information
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
        else if(ch == '/') {
            cmdstr = "/";
            savecursor();
            cursor_y = views[0]->getHeight() + 1;
            cursor_x = 1;
            botinsert_mode = true;
        }
        else if(ch == '?') {
            cmdstr = "?";
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
        else if(ch == 'b') {
            cursor_x = std::min(cursor_x, std::max(static_cast<int>(lines[cursor_y + offset].size()) - 1, 0));
            wordback();
            for(int i = 1; i < repeats; ++i) {
                wordback();
            }
            repeats = 0;
        }
        else if(ch == 'w') {
            cursor_x = std::min(cursor_x, std::max(static_cast<int>(lines[cursor_y + offset].size()) - 1, 0));
            wordforward();
            for(int i = 1; i < repeats; ++i) {
                wordforward();
            }
            repeats = 0;
        }
        else if(ch == 'x') {
            comparable = lines;
            buffer.clear();
            buffer.push_back("");
            savecursor();
            int cur_line = cursor_y + offset;
            int k = std::min(cursor_x, static_cast<int>(lines[cur_line].size()) - 1);
            int j = std::min(cursor_x, static_cast<int>(lines[cur_line].size()) - 1);
            if(lines[cur_line] != "") {
                buffer[0] += lines[cur_line][j];
                if(j != static_cast<int>(lines[cur_line].size()) - 1) lines[cur_line] = lines[cur_line].substr(0, j) + lines[cur_line].substr(j + 1, lines[cur_line].length() - j - 1);
                else lines[cur_line] = lines[cur_line].substr(0, j);
            }
            for(int i = 1; i < repeats; ++i) {
                if(lines[cur_line] != "") {
                j = std::min(cursor_x, static_cast<int>(lines[cur_line].size()) - 1);   
                if(j < k) break;
                buffer[0] += lines[cur_line][j];
                if(j != static_cast<int>(lines[cur_line].size()) - 1) lines[cur_line] = lines[cur_line].substr(0, j) + lines[cur_line].substr(j + 1, lines[cur_line].length() - j - 1);
                else lines[cur_line] = lines[cur_line].substr(0, j);
                }
            }
            comparesaves();
            repeats = 0;
        }
        else if(ch == 'X') {
            comparable = lines;
            buffer.clear();
            buffer.push_back("");
            int cur_line = cursor_y + offset;
            int k = std::min(cursor_x, static_cast<int>(lines[cur_line].size()) - 1);
            int j = std::min(cursor_x, static_cast<int>(lines[cur_line].size()) - 1);
            if(k != 0 && lines[cur_line] != "") {
                buffer[0] += lines[cur_line][k - 1];
                lines[cur_line] = lines[cur_line].substr(0, j - 1) + lines[cur_line].substr(j, static_cast<int>(lines[cur_line].size()));
                cursor_x = std::max(0, cursor_x - 1);
            }
            for(int i = 1; i < repeats; ++i) {
                if(lines[cur_line] != "") {
                k = cursor_x;  
                if(k != 0) {
                buffer[0] += lines[cur_line][k - 1];
                lines[cur_line] = lines[cur_line].substr(0, k - 1) + lines[cur_line].substr(k, static_cast<int>(lines[cur_line].size()));
                cursor_x = std::max(0, cursor_x - 1);
                }
            } }
            reverse(buffer[0].begin(), buffer[0].end());
            savecursor(); // cursor does not return, so save here
            comparesaves();
            repeats = 0;
        }
        else if(ch == 'p') {
            savecursor();
            comparable = lines;
            paste();
            for(int i = 1; i < repeats; ++i) {
                paste();
            } 
            comparesaves();
            repeats = 0;
        }
        else if(ch == 'R') {
            replace_mode = true;
            goinsert();
            savecursor();
        }
        else if(ch == '0') {
            if(numcmd == "") {
                cursor_x = 0;
                repeats = 0;
            }
        }
        else if(ch == '^') {
            cursor_x = 0;
            int curline = offset + cursor_y;
            while(cursor_x != std::max(static_cast<int>(lines[curline].size()) - 1, 0) && isspace(lines[curline][cursor_x])) {
                cursor_right();
            }
            repeats = 0;
        }
        else if(ch == '$') {
            int curline = offset + cursor_y;
            for(int i = 1; i < repeats; ++i) {
                cursor_down();
            }
            cursor_x = std::max(static_cast<int>(lines[curline].size()) - 1, 0);
        }
        else if(ch == '%') {
            cursor_x = std::min(cursor_x, std::max(0, static_cast<int>(lines[cursor_y + offset].size() - 1)));
            std::vector<std::pair<std::pair<int,int>,std::pair<int,int>>> parentheses;
            std::vector<std::pair<std::pair<int,int>,std::pair<int,int>>> brackets;
            std::vector<std::pair<std::pair<int,int>,std::pair<int,int>>> braces;
            get_bracket_pairs(parentheses, brackets, braces);
            int cur_line = cursor_y + offset;
            move_cursor_to_best_pair(parentheses, brackets, braces, cur_line);
        }
        else if(ch == 'n') {
            if(!prevpattern.empty()) {
                std::string tmp = prevpattern;
                cmdstr = prevpattern;
                savecursor();
                botCommand(cmdstr);
                clearbottom(views[0]->getHeight());
                returncursor();
                prevpattern = tmp;
            }
        }
        else if(ch == 'N') {
            if(!prevpattern.empty()) {
                std::string tmp = prevpattern;
                cmdstr = prevpattern;
                if(cmdstr[0] == '/') {
                    cmdstr[0] = '?';
                }
                else if(cmdstr[0] == '?') {
                    cmdstr[0] = '/';
                }
                savecursor();
                botCommand(cmdstr);
                clearbottom(views[0]->getHeight());
                returncursor();
                prevpattern = tmp;
            }
        }
        else if(ch == ';') {
            if(prevchar.first != 0) {
                numcmd += prevchar.first;
                reformat_command(numcmd);
                interpret_showcmd(numcmd.substr(0, numcmd.size()-1), numcmd[numcmd.size()-1], prevchar.second);
                numcmd = "";
                return;
            }
        }
        else if(ch == 6) { // ^f
            pagedown();
            for(int i = 1; i < repeats; ++i) {
                pagedown();
            } 
            repeats = 0;
        }
        else if(ch == 2) { // ^b
            pageup();
            for(int i = 1; i < repeats; ++i) {
                pageup();
            } 
            repeats = 0;
        }
        else if(ch == 7) { // ^g
            int perc = (100*(offset + cursor_y + 1))/(static_cast<int>(lines.size()));
            std::string percs = "--" + std::to_string(perc) + "%%--";
            cmdstr = "\"" + filename + "\" " + (filechange ? "[Modified] " : "") + std::to_string(lines.size()) + " lines " + percs;
            repeats = 0;
        }
        else {
            debug(ch, 0, 0);
        }
    }
};

#endif