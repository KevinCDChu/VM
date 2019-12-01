#ifndef UTILS_H
#define UTILS_H
#include <ncurses.h>
#include <string>
#include <algorithm>

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

bool is_inclusive(int ch) {
    return (ch == '$' || ch == 'f' || ch == '%');
}

bool valid_register(int ch) {
    return (isdigit(ch) || isalpha(ch));
}

bool valid_movement(int ch) {
    return (ch == 'h' || ch == 'j' || ch == 'k' || ch == 'l' || ch == ':' || ch == '/' || ch == '0' || ch == '$' || ch == '%' || ch == ';' || ch == 'f'
    || ch == 'w' || ch == 'b' || ch == 'n' || ch == 'N' || ch == 'c' || ch == 'd' || ch == 'y');
}

bool is_linewise(int ch) {
    return (ch == 'j' || ch == 'k');
}

bool isnum(const std::string& s)
{
    for(int i = 0; i < static_cast<int>(s.size()); ++i) {
        if (!isdigit(s[i])) return false;
    }
    return true;
}

bool in(std::vector<int> &v, int n) {
    return std::find(v.begin(), v.end(), n) != v.end();
}

void rem(std::vector<int> &v, int n) {
    v.erase(std::find(v.begin(), v.end(), n));
}


bool isWord(size_t c) {
    if(isdigit(c) || isalpha(c) || c == '_') {
        return true;
    }
    return false;
} 

bool isPunc(size_t c) {
    if(isWord(c) || isspace(c)) {
        return false;
    }
    return true;
}

bool containsletter(std::string s) {
    for(int i = 0; i < static_cast<int>(s.size()); ++i) {
        if(isalpha(s[i])) {
            return true;
        }
    }
    return false;
}


std::string get_product_of_digits(std::string &command, char &og_command) {
    std::string first_digits;
    std::string second_digits;
    bool in_first = true;
    for(auto i : command) {
        if(isalpha(i)) {
            in_first = false;
            og_command = i;
        } else {
            if(in_first) first_digits += i;
            else second_digits += i;
        }
    }
    if(first_digits == "") first_digits = "1";
    if(second_digits == "") second_digits = "1";
    return std::to_string(stoi(first_digits) * stoi(second_digits));
}


bool movement_command(int ch) {
    return(ch == 'h' || ch == 'j' || ch == 'k' || ch == 'l' || ch == 'w' || ch == 'b' || ch == '$');
}

void reformat_command(std::string &command) {
    char og_command; // remember which command we are trying to do (c, y, d)
    std::string multiplier = get_product_of_digits(command, og_command);
    command = "";
    command += multiplier;
    command += og_command;
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


bool file_exists(std::string file_name) {
	std::ifstream f{file_name};
	return f.is_open();
}



#endif