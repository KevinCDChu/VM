#ifndef MODEL_H
#define MODEL_H
#include "../view/view.h"
#include "../controller/controller.h"
#include "undo.h"
#include <string>
#include <vector>


class Model {
  protected:
    std::vector<std::unique_ptr<View>> views;
    std::unique_ptr<Controller> cntrl;
    virtual ~Model() {}
  public:
    virtual void addView(std::unique_ptr<View> &&v) = 0;
    virtual void addController(std::unique_ptr<Controller> &&c) = 0;
    virtual void updateViews() = 0;
    virtual void displayViews() = 0;
    virtual void interpret_input(int ch = 0) = 0;
};


class Logic : public Model {
    bool complete = false;
    bool insert_mode = false; // True if insert mode, false if command mode
    bool botinsert_mode = false;
    bool replace_mode = false; // will affect how backspace and adding characters work in insert mode
    bool filechange = false;
    std::string filename = "";
    std::vector<std::string> lines;
    std::string cmdstr = "";
    std::string numcmd = "";
    std::string savedchange = "";
    std::vector<std::string> buffer = {""};
    int repeats = 0;
    int offset = 0;
    int cursor_x = 0;
    int cursor_y = 0;
    int linechangeamount = -1;
    std::vector<std::pair <std::pair<int, int>, int>> prevloc;
    std::vector<Undo> undostack; 
    std::vector<std::string> comparable;
    std::vector<std::string> temp_comparable; // needed for replace mode
    std::string prevpattern = "";
    std::pair<int, int> prevchar;
    int backmovecount = 0;
    std::vector<std::string> entire_file_buffer;
    bool insert_did_something = false; // true if insert actually did something
    std::vector<size_t> double_undo_indices;
    bool currently_macro = false;
    bool linewise_paste= true;
    std::map<char, std::string> macros;
    std::string prevcommand = "";
    std::vector<int> rlines; // lines added by replace mode

    

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
                if((cur_line < static_cast<int>(temp_comparable.size()) && cursor_x > static_cast<int>(temp_comparable[cur_line].size())) || cur_line >= static_cast<int>(temp_comparable.size())) { // do normal backspace in this case
                    lines[cur_line] = lines[cur_line].substr(0, cursor_x - 1);
                    --cursor_x;
                    clearline();
                } else { // decrement cursor and just add back character from 
                    lines[cur_line][cursor_x - 1] = temp_comparable[cur_line][cursor_x - 1];
                    --cursor_x;
                    clearline(); 
                }
            } else if(ch == 10) {
                lines.insert(lines.begin() + cur_line + 1, "");
                if(replace_mode) {
                    rlines.push_back(cur_line + 1);
                    temp_comparable.insert(temp_comparable.begin() + cur_line + 1, "");
                }
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
                if(replace_mode && cur_line && !in(rlines, cur_line)) {
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
                    if(replace_mode) {
                        rem(rlines, cur_line);
                        temp_comparable[cur_line - 1] = temp_comparable[cur_line - 1].append(temp_comparable[cur_line]);
                        temp_comparable.erase(temp_comparable.begin() + cur_line);
                    }
                }
            } else {
                if(replace_mode) { // just add back charcter that was there
                    if((cur_line < static_cast<int>(temp_comparable.size()) && cursor_x > static_cast<int>(temp_comparable[cur_line].size())) || cur_line >= static_cast<int>(temp_comparable.size())) { // do normal backspace in this case
                    lines[cur_line] = lines[cur_line].substr(0, cursor_x - 1);
                    --cursor_x;
                    clearline();
                    } else { // decrement cursor and just add back character from 
                    lines[cur_line][cursor_x - 1] = temp_comparable[cur_line][cursor_x - 1];
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
            if(replace_mode) {
                rlines.push_back(cur_line + 1);
                temp_comparable.insert(temp_comparable.begin() + cur_line + 1, temp_comparable[cur_line].substr(cursor_x, temp_comparable[cur_line].size() - cursor_x));
                temp_comparable[cur_line] = temp_comparable[cur_line].substr(0, cursor_x);
            }
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
            filechange = false;
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
                if(!prevloc.empty()) {
                    cursor_y = prevloc.back().first.second;
                    cursor_x = prevloc.back().first.first;
                    offset = prevloc.back().second - cursor_y;
                }
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
        int linestart = std::max(prevloc.back().second + backmovecount, 0);
        int i = linestart;
        Undo save;
        if(comparable.size() == lines.size()) {
            while(comparable[i] == lines[i]) {
                if(i == mxs - 1) {
                    save.setStart(i+1);
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
        save.setStart(i);
        while (i < mxs) {
            save.pushChange(comparable[i]);
            ++i;
        }
        while (i < static_cast<int>(comparable.size())) {
            save.pushChange(comparable[i]);
            ++i;
        }
        undostack.push_back(save);
        if(currently_macro) double_undo_indices.push_back(undostack.size());
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
        if(linewise_paste) {
            for(size_t i = 0; i < buffer.size(); ++i) {
                lines.insert(lines.begin() + cur_line  + i + 1, buffer[i]);
            }
            return;
        }
        std::string last_part;
        bool was_empty_start = lines[cur_line] == "";
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
        } else if(was_empty_start) {
            cursor_x = 0; // behaviour that vim does - cursor ends at start of line for multiline empty start paste
        } else {
            ++cursor_x;
        }
    }

    void undo() {
        int start = undostack.back().getStart();
        std::vector<std::string> change = undostack.back().getChange();
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
            if(!double_undo_indices.empty() && double_undo_indices.back() == undostack.size()) {
                double_undo_indices.pop_back();
                undocommand();
            }
        }
        else {
            cmdstr = "Already at oldest change";
        }
    }

    void pagedown() {
        offset += views[0]->getHeight() - 1;
        offset = std::min(offset, std::max(0, static_cast<int>(lines.size()) - 6));
        cursor_y = std::min(static_cast<int>(lines.size())-1, 5);
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

    void linedown(int x) {
        offset += x;
        int maxoff = std::max(static_cast<int>(lines.size()) - views[0]->getHeight() - 1, 0);
        if(offset > maxoff) {
            cursor_y += offset - maxoff;
            offset = std::min(offset, maxoff);
            cursor_y = std::min(static_cast<int>(lines.size()) - 1, std::min(cursor_y, views[0]->getHeight()));
        }
        if(offset < maxoff) {
            cursor_y = std::max(5, cursor_y);
        }
        cursor_x = 0;
        while(cursor_x < static_cast<int>(lines[cursor_y+offset].size()) && isspace(lines[cursor_y+offset][cursor_x])) {
            ++cursor_x;
        }
        if(cursor_x == static_cast<int>(lines[cursor_y+offset].size())) {
            cursor_x = 0;
        }
    }

    void lineup(int x) {
        offset -= x;
        if(offset < 0) {
            cursor_y -= 0 - offset;
            offset = std::max(offset, 0);
            cursor_y = std::max(cursor_y, 0);
        }
        if(offset > 0) {
            cursor_y = std::min(cursor_y, std::max(views[0]->getHeight() - 5, 0));
        }
        cursor_x = 0;
        while(cursor_x < static_cast<int>(lines[cursor_y+offset].size()) && isspace(lines[cursor_y+offset][cursor_x])) {
            ++cursor_x;
        }
        if(cursor_x == static_cast<int>(lines[cursor_y+offset].size())) {
            cursor_x = 0;
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
            if (lines[curline].rfind(ch, std::max(cursor_x - 1, 0)) != std::string::npos) {
                cursor_x = lines[curline].rfind(ch, std::max(cursor_x - 1, 0));
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
        else if(ch == 'd') {
            return true;
        }
        else if(ch == 'c') {
            return true;
        }
        else if(ch == 'y') {
            return true;
        }
        else if(ch == 'q') {
            return true;
        }
        return false;
    }


    void interpret_showcmd(std::string num, int cmd, int ch) {
        if (!num.empty()) {
            try { 
                repeats = stoi(num);
            } catch(...) {
                repeats = 0;
            }
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
                prevcommand = num + static_cast<char>(cmd) + static_cast<char>(ch);
                if(num.empty()) repeats = 1;
                int j = std::min(cursor_x, std::max(0, static_cast<int>(lines[curline].size() - 1)));
                savecursor();
                comparable = lines;
                if(j + repeats > end + 1 || ch == KEY_BACKSPACE || ch == KEY_DC ||  ch == 27 || lines[cursor_y + offset] == "") {
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
            else if (cmd == 'd' || cmd == 'c' || cmd == 'y') {
                if(!valid_movement(ch)) return;
                numcmd = "";
                if(num.empty()) repeats = 1;
                if(ch == 'l') { // hardcode these in as they have weird behaviour at the end of lines
                    if(cmd == 'c') {
                        comparable = lines;
                        currently_macro = true;
                        do_command_sequence("x");
                        interpret_input('i');
                        currently_macro = false;
                        filechange = true;
                        if (cmd == 'd' || cmd == 'c') prevcommand = num + static_cast<char>(cmd) + static_cast<char>(ch);
                        return;
                    } else {
                        filechange = true;
                        interpret_input('x');
                        if(cmd == 'y') interpret_input('u');
                        if (cmd == 'd' || cmd == 'c') prevcommand = num + static_cast<char>(cmd) + static_cast<char>(ch);
                        return;
                    }
                } else if(ch == 'h') {
                    if(cmd == 'c') {
                        comparable = lines;
                        currently_macro = true;
                        do_command_sequence("X");
                        interpret_input('i');
                        filechange = true;
                        currently_macro = false;
                        if (cmd == 'd' || cmd == 'c') prevcommand = num + static_cast<char>(cmd) + static_cast<char>(ch);
                        return;
                    } else {
                        filechange = true;
                        interpret_input('X');
                        if(cmd == 'y') interpret_input('u');
                        if (cmd == 'd' || cmd == 'c') prevcommand = num + static_cast<char>(cmd) + static_cast<char>(ch);
                        return;
                    }
                }
                cursor_x = std::min(cursor_x, std::max(0, static_cast<int>(lines[cursor_y + offset].size() - 1)));
                int old_cursor_x = cursor_x;
                int old_cursor_y = cursor_y;
                int old_offset = offset;
                comparable = lines;
                if(cmd == 'c') currently_macro = true;
                savecursor();
                if(cmd == 'c') currently_macro = false;
                std::string movement_command = "";
                movement_command += static_cast<char>(ch);
                if(ch != cmd) interpret_input(ch);
                if (cmd == 'd' || cmd == 'c') prevcommand = num + static_cast<char>(cmd) + static_cast<char>(ch);
                if(ch == ':' || ch == '/' || ch == '?') { // interpret the command if it is a colon command
                    displayViews();
                    int last_input = 0;
                    while(last_input != 10) {
                        last_input = getch();
                        interpret_input(last_input);
                        if (cmd == 'd' || cmd == 'c') prevcommand += last_input;
                        displayViews();
                    }
                }
                if(ch == 'f') {
                    displayViews();
                    int last_input = 0;
                    last_input = getch();
                    if(last_input == 27) return; // idk about this
                    numcmd = "f";
                    repeats = 0;
                    interpret_input(last_input);
                    if (cmd == 'd' || cmd == 'c') prevcommand += last_input;
                    displayViews();
                }
                if(ch == 'j') {
                    cursor_x = static_cast<int>(lines[cursor_y + offset].size());
                    old_cursor_x = 0;   
                }
                if(ch == 'k') {
                    old_cursor_x = static_cast<int>(lines[old_cursor_y + old_offset].size());
                    cursor_x = 0; 
                }
                if(ch == cmd) {
                    if(repeats != 1) {
                    old_cursor_x = 0;
                    repeats = std::min(repeats, static_cast<int>(lines.size() - cursor_y - offset));
                    cursor_x = static_cast<int>(lines[old_cursor_y + old_offset + repeats - 1].size());
                    cursor_y += repeats - 1;
                    } else {
                        old_cursor_x = 0;
                       cursor_x = static_cast<int>(lines[old_cursor_y + old_offset].size());
                    }
                }
                bool moved_backwards = false;
                bool inclusive_command = is_inclusive(ch);
                if(ch == cmd) inclusive_command = true;
                if(cursor_y + offset == old_cursor_y + old_offset && cursor_x == old_cursor_x && (!inclusive_command || ch == '%')) return;
                cursor_x = std::min(cursor_x, std::max(0, static_cast<int>(lines[cursor_y + offset].size() - 1)));
                if(cursor_y < old_cursor_y || (cursor_y == old_cursor_y && cursor_x < old_cursor_x)) moved_backwards = true;
                delete_and_store(old_cursor_x, old_cursor_y, old_offset, inclusive_command, moved_backwards, ch, cmd);
                if(!moved_backwards) {
                    cursor_x = old_cursor_x;
                    cursor_y = old_cursor_y;
                    offset = old_offset;
                }
                if(moved_backwards) { // cursor should end in a slighly weird spot on the undo command
                    prevloc.back().first.second = cursor_y;
                    prevloc.back().first.first = cursor_x;
                    prevloc.back().second = cursor_y + offset;
                }
                if(!currently_macro) repeats = 0;
                if(cmd == ch) {
                    //buffer.insert(buffer.begin(), "");
                }
                if(cmd == 'c') currently_macro = true;
                comparesaves();
                if(cmd == 'c') currently_macro = false;
                if(cmd == 'c') {
                    comparable = lines;
                    currently_macro = true;
                    interpret_input('i');
                    currently_macro = false;
                }
                if(cmd == 'y') undocommand(); // undo all the changes
                if(cmd == ch || ch == 'j' || ch == 'k') {
                    linewise_paste = true;
                } else {
                    linewise_paste = false;
                }
                filechange = true;
            }
        }
    }

    void delete_and_store(int old_cursor_x, int old_cursor_y, int old_offset, bool inclusive, bool moved_backwards, int ch, int cmd) { // assume everything is exclusive (deletes newline)
        buffer.clear();
        backmovecount = 0;
        if(old_cursor_y + old_offset == cursor_y + offset) {
            if(inclusive && moved_backwards) {
                old_cursor_x = std::min(old_cursor_x + 1, static_cast<int>(lines[old_cursor_y + old_offset].size() - 1));
                //cursor_x = std::max(cursor_x - 1, 0);
            } else if(inclusive && !moved_backwards) {
                cursor_x = std::min(cursor_x + 1, static_cast<int>(lines[old_cursor_y + old_offset].size()));
            }
            int x_start = std::min(old_cursor_x, cursor_x);
            int x_end = std::max(old_cursor_x, cursor_x);
            buffer.push_back(lines[old_cursor_y + old_offset].substr(x_start, x_end - x_start));
            lines[old_cursor_y + old_offset] = lines[old_cursor_y + old_offset].substr(0, x_start) + lines[old_cursor_y + old_offset].substr(x_end, static_cast<int>(lines[old_cursor_y + old_offset].size()) - x_end);
            if(ch == cmd && cmd == 'd') lines.erase(lines.begin() + old_cursor_y + old_offset); // erase line if dd/cc/yy
            return;
        }
        int x_start;
        int x_end;
        if(moved_backwards) {
            x_start = 0;
            x_end = old_cursor_x;
        } else {
            x_start = old_cursor_x;
            x_end = static_cast<int>(lines[old_cursor_y].size());
        }
        std::string thing_to_push_back = lines[old_cursor_y + old_offset].substr(0, old_cursor_x + 1);
        buffer.push_back(lines[old_cursor_y + old_offset].substr(x_start, x_end - x_start));
        if(moved_backwards) lines[old_cursor_y + old_offset] = lines[old_cursor_y + old_offset].substr(old_cursor_x, static_cast<int>(lines[old_cursor_y + old_offset].size() - old_cursor_x));
        else lines[old_cursor_y + old_offset] = lines[old_cursor_y + old_offset].substr(0, old_cursor_x);        
        int y_start;
        int y_end;
        if(moved_backwards) {
            y_start = cursor_y + offset + 1;
            y_end = old_cursor_y + old_offset;
        } else {
            y_end = cursor_y + offset;
            y_start = old_cursor_y + old_offset + 1;
        }
        for(int i = y_start; i < y_end; ++i) { // delete lines
            buffer.push_back(lines[y_start]);
            lines.erase(lines.begin() + y_start); // delete the same line again as we just deleted the last one
            if(moved_backwards) --backmovecount;
        }
        buffer.push_back("");
        if(moved_backwards) {
            x_start = cursor_x;
            x_end = static_cast<int>(lines[y_start].size() - 1);
        } else {
            x_start = 0;
            x_end = cursor_x;
        }
        int size_of_line_before;
        if(moved_backwards) {
            size_of_line_before = lines[cursor_y + offset].size();
            buffer[buffer.size() - 1] += lines[cursor_y + offset].substr(cursor_x, static_cast<int>(lines[cursor_y + offset].size()) - cursor_x);
            lines[cursor_y + offset + 1] = lines[cursor_y + offset].substr(0, cursor_x) + lines[cursor_y + offset + 1];
            lines.erase(lines.begin() + cursor_y + offset);
            if(inclusive) {
                lines[cursor_y + offset] = lines[cursor_y + offset].substr(0, size_of_line_before - 1) + lines[cursor_y + offset].substr(size_of_line_before, static_cast<int>(lines[cursor_y + offset].size() - size_of_line_before));
            }
        } else {
            if(lines[old_cursor_y + old_offset + 1] != "") {
            size_of_line_before = lines[old_cursor_y + old_offset].size();
            buffer[buffer.size() - 1] += lines[old_cursor_y + old_offset + 1].substr(0, cursor_x);
            lines[old_cursor_y + old_offset] += lines[old_cursor_y + old_offset + 1].substr(cursor_x, static_cast<int>(lines[old_cursor_y + old_offset + 1].size() - cursor_x));
            if(inclusive) {
                buffer[buffer.size() - 1] += lines[old_cursor_y + old_offset][size_of_line_before];
                lines[old_cursor_y + old_offset] = lines[old_cursor_y + old_offset].substr(0, size_of_line_before) + lines[old_cursor_y + old_offset].substr(size_of_line_before + 1, static_cast<int>(lines[old_cursor_y + old_offset].size() - size_of_line_before - 1));
            }
            }
            lines.erase(lines.begin() + old_cursor_y + old_offset + 1);
        }
        if(moved_backwards && !(cmd == ch || is_linewise(ch))) {
            if(inclusive) buffer.erase(buffer.begin());
            buffer.insert(buffer.begin() , buffer.back());
            buffer.pop_back();
            if(inclusive) {
                buffer.push_back(thing_to_push_back);
            }
        }
        if(is_linewise(ch)) {
           if(moved_backwards) lines.erase(lines.begin() + cursor_y + offset);
           else lines.erase(lines.begin() + old_cursor_y + old_offset);
           if(moved_backwards) std::swap(buffer[0], buffer[buffer.size() - 1]); // swap first and last for some reason
        }
    }




    void do_command_sequence(std::string str) {
        for(int i = 0; i < static_cast<int>(str.size()); ++i) {
            if(str[i] == 7) {
                interpret_input(KEY_BACKSPACE);
            }
            else {
                interpret_input(str[i]);
            }
        }
    }

  public:

    void setLines(std::vector<std::string> v) {
        lines = v;
    }
    void setFilename(std::string s) {
        filename = s;
    }
    bool completed() {
        return complete;
    }
    void addView(std::unique_ptr<View> &&v) override {
        views.push_back(std::move(v));
    }

    void addController(std::unique_ptr<Controller> &&c) override {
        cntrl = std::move(c);
    }
    void updateViews() override {
        for(auto &i : views) i->updateView();
    }
    void displayViews() override {
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
    void interpret_input(int ch = 0) override { // make it possible to do command not from keyboard
        if(ch == 0) {
            cntrl->genAction();
            std::unique_ptr<Action> new_action = cntrl->getAction();
            ch = new_action->getchar();
        } 
        if(ch == KEY_MOUSE) {
            MEVENT event;
            if(getmouse(&event) == OK) {
                cursor_y = std::max(std::min(event.y, std::min(views[0]->getHeight(), static_cast<int>(lines.size() - 1))), 0);
                cursor_x = std::max(std::min(event.x, static_cast<int>(lines[cursor_y + offset].size() - 1)), 0);
            }
		return;
        }

        if (!cmdstr.empty() && (cmdstr[0] == 'E' || cmdstr[0] == 's')) {
            cmdstr = "";
        }
        if(ch == 27) numcmd = "";
        if((isdigit(ch) || (!containsletter(numcmd) && matchshowcmd(ch))) && !botinsert_mode && !insert_mode) {
            numcmd += ch;
            if (numcmd == "0") {
                numcmd = "";
            }
        }
        else {
            if(containsletter(numcmd)) {
                if(containscd(numcmd)) prevcommand = numcmd + static_cast<char>(ch);
                if(valid_movement(ch) && containscdy(numcmd)) reformat_command(numcmd); // needed in case of double multipliers (like 3d4l)
                if(containscdy(numcmd) && !valid_movement(ch)) {numcmd = ""; return;}
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
                prevcommand += (savedchange + static_cast<char>(27));
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
            return;
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
            if(repeats == 0) prevcommand = "i";
            else prevcommand = std::to_string(repeats) + "i";
            cursor_x = std::min(cursor_x, std::max(static_cast<int>(lines[cursor_y + offset].size()) - 1, 0));
            goinsert();
            savecursor();
        }
        else if(ch == 'I') {
            if(repeats == 0) prevcommand = "I";
            else prevcommand = std::to_string(repeats) + "I";
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
            cursor_x = std::min(cursor_x, std::max(static_cast<int>(lines[cursor_y + offset].size()) - 1, 0));
            if(repeats == 0) prevcommand = "a";
            else prevcommand = std::to_string(repeats) + "a";
            if(cursor_x < static_cast<int>(lines[cursor_y + offset].size())) ++cursor_x;
            goinsert();
            savecursor();
        }
        else if(ch == 'A') {
            if(repeats == 0) prevcommand = "A";
            else prevcommand = std::to_string(repeats) + "A";
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
            if(repeats == 0) prevcommand = "x";
            else prevcommand = std::to_string(repeats) + "x";
            linewise_paste = false;
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
            filechange = true;
            repeats = 0;
        }
        else if(ch == 'X') {
            if(repeats == 0) prevcommand = "X";
            else prevcommand = std::to_string(repeats) + "X";
            linewise_paste = false;
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
            filechange = true;
            repeats = 0;
        }
        else if(ch == 'p') {
            if(repeats == 0) prevcommand = "p";
            else prevcommand = std::to_string(repeats) + "p";
            savecursor();
            comparable = lines;
            paste();
            for(int i = 1; i < repeats; ++i) {
                paste();
            } 
            comparesaves();
            filechange = true;
            repeats = 0;
        }
        else if(ch == 'P') {
            if(repeats == 0) prevcommand = "P";
            else prevcommand = std::to_string(repeats) + "P";
            if(linewise_paste) {
                savecursor();
                comparable = lines;
                for(int i = 0; i < static_cast<int>(buffer.size()); ++i) {
                    lines.insert(lines.begin() + cursor_y + offset + i, buffer[i]);
                }
                comparesaves();
                repeats = 0;
            }
            else if(cursor_x > 0) {
                cursor_left();
                interpret_input('p');
            } 
            else {
                int curline = cursor_y + offset;
                savecursor();
                comparable = lines;
                lines[curline] = " " + lines[curline];
                cursor_x = 0;
                paste();
                for(int i = 1; i < repeats; ++i) {
                    paste();
                } 
                comparesaves();
                repeats = 0;
                lines[curline] = lines[curline].substr(1, lines[curline].size() - 1);
            }
            filechange = true;
        }
        else if(ch == 'R') {
            if(repeats == 0) prevcommand = "R";
            else prevcommand = std::to_string(repeats) + "R";
            replace_mode = true;
            temp_comparable = lines;
            rlines.clear();
            goinsert();
            savecursor();
        }
        else if(ch == 'S') {
            comparable = lines;
            currently_macro = true;
            numcmd = "";
            if(repeats > 1) do_command_sequence(std::to_string(repeats) + "cc");
            else {
                do_command_sequence("cc");
            }
            currently_macro = false;
            repeats = 0;
            filechange = true;
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
            curline = offset + cursor_y;
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
        else if(ch == 'J') {
            if(repeats == 0) prevcommand = "J";
            else prevcommand = std::to_string(repeats) + "J";
            if(cursor_y + offset >= static_cast<int>(lines.size() - 1)) return;
            comparable = lines;
            savecursor();
            if(lines[cursor_y + offset] == "") lines.erase(lines.begin() + cursor_y + offset);
            else lines[cursor_y + offset] += " ";
            int move_to = static_cast<int>(lines[cursor_y + offset].size() - 1);
            cursor_x = 0;
            while(cursor_x < static_cast<int>(lines[cursor_y+offset + 1].size()) && isspace(lines[cursor_y+offset + 1][cursor_x])) {
                ++cursor_x;
            }
            lines[cursor_y + offset] += lines[cursor_y + offset + 1].substr(cursor_x, static_cast<int>(lines[cursor_y + offset + 1].size()) - cursor_x);
            lines.erase(lines.begin() + cursor_y + offset + 1);
            repeats -= 1;
            cursor_x = move_to;
            for(int i = 1; i < repeats; ++i) {
                if(cursor_y + offset >= static_cast<int>(lines.size() - 1)) return;
                if(lines[cursor_y + offset] == "") lines.erase(lines.begin() + cursor_y + offset);
                else lines[cursor_y + offset] += " ";
                int move_to = static_cast<int>(lines[cursor_y + offset].size() - 1);
                cursor_x = 0;
                while(cursor_x < static_cast<int>(lines[cursor_y+offset + 1].size()) && isspace(lines[cursor_y+offset + 1][cursor_x])) {
                    ++cursor_x;
                }
                lines[cursor_y + offset] += lines[cursor_y + offset + 1].substr(cursor_x, static_cast<int>(lines[cursor_y + offset + 1].size()) - cursor_x);
                lines.erase(lines.begin() + cursor_y + offset + 1);
                cursor_x = move_to;
            }
            comparesaves();
            repeats = 0;
            filechange = true;
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
        else if(ch == 'o') {
            comparable = lines;
            currently_macro = true;
            std::string command = "A\n";
            do_command_sequence(command);
            currently_macro = false;
            filechange = true;
        }
        else if(ch == 'O') {
            std::string tmpcmd = "";
            if(repeats != 0) tmpcmd = std::to_string(repeats) + "O";
            std::string command = "";
            numcmd = "";
            int savere = repeats;
            repeats = 0;
            std::pair<std::string, std::string> repeatsaves("", "");
            comparable = lines;
            currently_macro = true;
            if(cursor_y + offset > 0) {
                command = "k" + std::to_string(savere) + "A\n";
                do_command_sequence(command);
                prevcommand = "k" + prevcommand;
            }
            else {
                command = "0i\n";
                std::string esc(1, 27);
                std::string k(1, 'k');
                command.append(esc);
                command.append(k);
                do_command_sequence(command);
                interpret_input('i');
                if(savere != 0) {
                    while(insert_mode) {
                        displayViews();
                        interpret_input();
                        repeatsaves.first = repeatsaves.second;
                        repeatsaves.second = savedchange;
                    }
                    if(!repeatsaves.first.empty()) {
                        while(savere > 1) {
                            cursor_down();
                            do_command_sequence(command);
                            interpret_input('i');
                            savedchange = repeatsaves.first;
                            repeats = 2;
                            repeatsave();
                            interpret_input(27);
                            --savere;
                        }
                    }
                }
                if(savere == 1) {
                double_undo_indices.pop_back();
                prevcommand = tmpcmd;
                    }
                else if (savere == 0) {
                    prevcommand = command + static_cast<char>(KEY_A1) + prevcommand;
                }
                repeats = 0;
                filechange = true;
            }
            currently_macro = false;
        }
        else if(ch == 's') {
            currently_macro = true;
            if(repeats != 0) do_command_sequence(std::to_string(repeats) + "cl");
            else do_command_sequence("cl");
            currently_macro = false;
            filechange = true;
        }
        else if(ch == '.') {
            std::string tmp = prevcommand;
            int tmprepeats = repeats;
            if(!prevcommand.empty()) {
                if(repeats != 0) {
                    repeats = 0;
                    do_command_sequence(replaceforrepeat(prevcommand, tmprepeats));
                }
                else {
                    do_command_sequence(prevcommand);
                }
            }
            prevcommand = tmp;
            repeats = 0;
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
        else if(ch == 4) { // ^d
            if(linechangeamount == -1) {
                linechangeamount = views[0]->getHeight()/2;
            }
            if(repeats != 0) {
                linechangeamount = repeats;
            }
            linedown(linechangeamount);
            repeats = 0;
        }
        else if(ch == 21) { // ^u
            if(linechangeamount == -1) {
                linechangeamount = views[0]->getHeight()/2;
            }
            if(repeats != 0) {
                linechangeamount = repeats;
            }
            lineup(linechangeamount);
            repeats = 0;
        }
    }
};

#endif
