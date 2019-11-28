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
    std::vector<std::string> entire_file_buffer;

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
        std::string c;
        if(cmd.size() > 1) c = cmd.substr(1, cmd.size() - 1);
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
        else if(cmd.size() > 2 && cmd.substr(0,2) == ":r") {
            botinsert_mode = false;
            //filechange = true;
            cmdstr = "";
            read_into_file_buffer(cmd.substr(2, cmd.size() - 2));
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
            cmdstr = "E492: Not an editor command: " + c;
        }
    }

    void read_into_file_buffer(std::string file_name) {
        if(file_exists(file_name)) {

        } else {
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
            if (lines[curline].rfind(ch, cursor_x - 1) != std::string::npos) {
                cursor_x = lines[curline].rfind(ch, cursor_x - 1);
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
                if(!cmdf(curline, end, ch)) {
                    returncursor();
                    return;
                }
                for(int i = 1; i < repeats; ++i) {
                    if(!cmdf(curline, end, ch)) {
                        returncursor();
                        return;
                    }
                }
                repeats = 0;
            }
            else if (cmd == 'F') {
                savecursor();
                if(!cmdF(curline, end, ch)) {
                    returncursor();
                    return;
                }
                for(int i = 1; i < repeats; ++i) {
                    if(!cmdF(curline, end, ch)) {
                        returncursor();
                        return;
                    }
                }
                repeats = 0;
            }
        }
    }

    void interpret_input(int ch = 0) { // make it possible to do command not from keyboard
        if(ch == 0) {
            cntrl->genAction();
            ch = cntrl->getAction()->getchar();
        } 

        if((isdigit(ch) || (!containsletter(numcmd) && matchshowcmd(ch))) && !botinsert_mode && !insert_mode) {
            numcmd += ch;
            if (numcmd == "0") {
                numcmd = "";
            }
        }
        else {
            if(containsletter(numcmd)) {
                interpret_showcmd(numcmd.substr(0, numcmd.size()-1), numcmd[numcmd.size()-1], ch);
                numcmd = "";
                return;
            }
            else if (!numcmd.empty()) {
                interpret_showcmd(numcmd, 0, ch);
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
                comparesaves();
            }
            repeats = 0;
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
            cursor_x = std::min(cursor_x, static_cast<int>(lines[cursor_y + offset].size()) - 1);
            wordback();
            for(int i = 1; i < repeats; ++i) {
                wordback();
            }
            repeats = 0;
        }
        else if(ch == 'w') {
            cursor_x = std::min(cursor_x, static_cast<int>(lines[cursor_y + offset].size()) - 1);
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
        
    }
};

#endif