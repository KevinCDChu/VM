#ifndef UTILS_H
#define UTILS_H
#include <ncurses.h>


void printw(std::string str) {
    printw(str.c_str());
}

void printw(char c) {
    std::string str;
    str += c;
    printw(str);
}

void clearbottom(int height) {
    move(height + 1, 0);
    clrtoeol();
}

void clearline() {
    clrtoeol();
}

bool isnum(const std::string& s)
{
    for(int i = 0; i < static_cast<int>(s.size()); ++i) {
        if (!isdigit(s[i])) return false;
    }
    return true;
}


void count_parentheses(std::vector<std::string> &lines, int &offset, int &brackets, int &braces, int &parentheses) {
    for(int i = 0; i < offset; ++i) {
        for(int j = 0; j < static_cast<int>(lines[i].size()); ++j) {
            if(i >= static_cast<int>(lines.size())) break;
            if(lines[i][j] == '(') ++parentheses;
            if(lines[i][j] == '{') ++braces;
            if(lines[i][j] == '[') ++brackets;
            if(lines[i][j] == ')') parentheses = std::max(parentheses - 1, 0);
            if(lines[i][j] == '}') braces = std::max(braces - 1, 0);
            if(lines[i][j] == ']') brackets = std::max(brackets - 1, 0);
        }
    }
}




#endif