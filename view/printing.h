#ifndef PRINTING_H
#define PRINTING_H
#include <ncurses.h>
#include <string>
#include <regex>
#include <utility>

void init_colours(bool code_file) {
    init_pair(1, COLOR_BLUE, COLOR_BLACK);
    init_pair(2, COLOR_WHITE, COLOR_RED);
    if(code_file) {
        init_pair(3, COLOR_GREEN, COLOR_BLACK);
        init_pair(4, COLOR_CYAN, COLOR_BLACK);
        init_pair(5, COLOR_YELLOW, COLOR_BLACK);
        init_pair(6, COLOR_MAGENTA, COLOR_BLACK);
        init_pair(7, COLOR_BLUE, COLOR_BLACK);
        init_pair(8, COLOR_RED, COLOR_BLACK);
        init_pair(9, COLOR_GREEN, COLOR_BLACK); // need a seperate comment colour for multiline as comment will end it
        init_pair(10, COLOR_WHITE, COLOR_RED);
    } else {
        init_pair(3, COLOR_WHITE, COLOR_BLACK);
        init_pair(4, COLOR_WHITE, COLOR_BLACK);
        init_pair(5, COLOR_WHITE, COLOR_BLACK);
        init_pair(6, COLOR_WHITE, COLOR_BLACK);
        init_pair(7, COLOR_WHITE, COLOR_BLACK);
        init_pair(8, COLOR_WHITE, COLOR_BLACK);
        init_pair(9, COLOR_WHITE, COLOR_BLACK);
        init_pair(10, COLOR_WHITE, COLOR_BLACK);

    }
}


bool contains_match(std::string &str, std::regex &re, std::smatch &match) {
    return std::regex_search(str, match, re);
}


void myprintw(std::string &line, std::vector<int> &cur_line, std::vector<std::string> &lines, int &brackets, int &braces, int &parentheses);

void strip_unnecessary_characters(std::string &first_string, std::string &middle, std::string &end_string, std::string &str, std::smatch &match) {
    int length = middle.length();
    if(middle[middle.length() - 1] == ')' || middle[middle.length() - 1] == ',' || middle[middle.length() - 1] == '(' || middle[middle.length() - 1] == '>' || middle[middle.length() - 1] == '}') {
        end_string = middle[middle.length() - 1];
        middle = middle.substr(0, length - 1);
    }
    size_t start_idx = match.position(0);
    end_string += str.substr(start_idx + length, str.length() - (start_idx + length));
    first_string = str.substr(0, start_idx);
    if(middle[0] == '(' || middle[0] == ',' || middle[0] == ';' || middle[0] == '<' || middle[0] == '{') {
        first_string += middle[0];
        middle = middle.substr(1, middle.length() - 1);
    }

}


void myprintw_helper(std::string &line, int checker, int &cur_line, std::vector<std::string> &lines, int &brackets, int &braces, int &parentheses);

void myprintw_only_colour_match(std::string &str, std::regex &re, int colour, int checker, std::smatch &match, int &cur_line, std::vector<std::string> &lines, int &brackets, int &braces, int &parentheses) { // function assume that match exists!!! 
    std::string first_string;
    std::string middle = match[0];
    std::string end_string;
    strip_unnecessary_characters(first_string, middle, end_string, str, match); // in case (int or something
    myprintw_helper(first_string, checker - 1, cur_line, lines, brackets, braces, parentheses);
    attron(COLOR_PAIR(colour));
    printw(middle);
    attroff(COLOR_PAIR(colour));
    myprintw_helper(end_string, checker, cur_line, lines, brackets, braces, parentheses);
}



std::regex init_data_types() {
    std::string str;
    str += "^int |[,(< ]int[,)> ]|[,(< ]int$|";
    str += "^bool |[,(< ]bool | bool$|";
    str += "^char |[,(< ]char[,)> ]|[,(< ]char$|";
    str += "^void |[,(< ]void[,)> ]|[,(< ]void$|";
    str += "^const |[,(< ]const[,)> ]|[,(< ]const$|";
    str += "^virtual |[,(< ]virtual[,)> ]|[,(< ]virtual$|";
    str += " override |";
    str += "^class |[,(< ]class[,)> ]|[,(< ]class$";
    std::regex re(str);
    return re;
}

std::regex init_keywords() {
    std::string str;
    str += "[^a-zA-Z0-9:]return|";
    str += "[^a-zA-Z0-9:]if( )*\\(?|^if( )*\\(?|";
    str += "[^a-zA-Z0-9:]for( )*\\(?|^for( )*\\(?|";
    str += "[^a-zA-Z0-9:]while( )*\\(?|^while( )*\\(?|";
    str += "[^a-zA-Z0-9:]else( )*\\(?|^else( )*\\(?|";
    str += " new|";
    str += "[^a-zA-Z0-9:]public|";
    str += "[^a-zA-Z0-9:]protected|";
    str += "static_cast|";
    str += "dynamic_cast|";
    str += "[^a-zA-Z0-9:]private";


    std::regex re(str);
    return re;
}

std::regex init_numbers() {
    std::string str;
    str += "1|2|3|4|5|6|7|8|9|0|";
    str += "true|false";
    std::regex re(str);
    return re;
}

std::regex init_string() {
    std::string str;
    str += "([\"'])(?:\\\\1|.)*?\\1";
    //str += "\'[^\"]+\'";
    std::regex re(str);
    return re;
}

std::regex init_comment() {
    std::regex comment("//");
    return comment;
}

std::regex init_preprocessor() {
    std::regex preprocessor("^( )*#( )*include( )*<.+>|^( )*#( )*include( )*\".+\"|#( )*ifndef( )+([^ ])+|#( )*define( )+([^ ])+|#( )*endif");
    return preprocessor;
}

std::regex init_multiline_comment_start() {
    std::regex multiline_comment_start("/\\*");
    return multiline_comment_start;
}

std::regex init_multiline_comment_end() {
    std::regex multiline_comment_end("\\*/");
    return multiline_comment_end;
}

std::regex data_types = init_data_types();
std::regex keywords = init_keywords();
std::regex numbers = init_numbers();
std::regex string = init_string();
std::regex comment = init_comment();
std::regex preprocessor = init_preprocessor();
std::regex multiline_comment_start = init_multiline_comment_start();
std::regex multiline_comment_end = init_multiline_comment_end();




int end_comment = -1;
int start_comment = -1;
int first_end_comment = -1;

bool in_comment(int &cur_line) {
    if(end_comment != -1 && end_comment > cur_line && cur_line > start_comment) return true;
    else if((first_end_comment >= cur_line) && (first_end_comment < start_comment)) return true;
    else return false;
}


void get_end_comment(int &cur_line, std::vector<std::string> &lines) {
    std::smatch match;
    for(int i = cur_line; i < static_cast<int>(lines.size()); ++i) {
        if(std::regex_search(lines[i], match, multiline_comment_end)) {end_comment =  i; return;}
    }
    end_comment = -1; // set to large number if no end comment exists
}


void get_first_end_comment(int &cur_line, std::vector<std::string> &lines) {
    std::smatch match;
    for(int i = cur_line; i < static_cast<int>(lines.size()); ++i) {
        if(std::regex_search(lines[i], match, multiline_comment_end)) {first_end_comment =  i; return;}
    }
    first_end_comment = -1; 
}


void get_first_comment(int &cur_line, std::vector<std::string> &lines) {
    std::smatch match;
    for(int i = cur_line; i < static_cast<int>(lines.size()); ++i) {
        if(std::regex_search(lines[i], match, multiline_comment_start)) {start_comment =  i; return;}
    }
    start_comment = -1; 
}


void print_mismatched(std::string &line, int &brackets, int &braces, int &parentheses) {
    for(int j = 0; j < static_cast<int>(line.size()); ++j) {
            if(line[j] == '(') {++parentheses; printw(line[j]);}
            else if(line[j] == '{') {++braces; printw(line[j]);}
            else if(line[j] == '[') {++brackets; printw(line[j]);}
            else if(line[j] == ')') {
                if(parentheses == 0) {
                    attron(COLOR_PAIR(10));
                    printw(line[j]);
                    attroff(COLOR_PAIR(10));
                }
                else {--parentheses; printw(line[j]);}
            }
            else if(line[j] == '}') {
                if(braces == 0) {
                    attron(COLOR_PAIR(10));
                    printw(line[j]);
                    attroff(COLOR_PAIR(10));
                }
                else {--braces; printw(line[j]);}

            }
            else if(line[j] == ']') {
                if(brackets == 0) {
                    attron(COLOR_PAIR(10));
                    printw(line[j]);
                    attroff(COLOR_PAIR(10));
                }
                else {--brackets; printw(line[j]);}
            }
            else {
                printw(line[j]);
            }

        }
}



void myprintw_helper(std::string &line, int checker, int &cur_line, std::vector<std::string> &lines,int &brackets, int &braces, int &parentheses) {
    if(line == "") return;
    std::smatch match;
    std::smatch comment_match;
    std::smatch match_preprocessor;
    if(std::regex_search(line, match, multiline_comment_start) && checker >= 10) {
        int comment_idx = match.position(0);
        std::string before = line.substr(0, comment_idx);
        myprintw_helper(before, checker - 1, cur_line, lines, brackets, braces, parentheses);
        attron(COLOR_PAIR(9));
        printw(line.substr(comment_idx, static_cast<int>(line.size()) - comment_idx));
        attroff(COLOR_PAIR(9));
        get_end_comment(cur_line, lines);
    } else if(std::regex_search(line, match, multiline_comment_end) && checker >= 9) {
        int comment_idx = match.position(0) + 2;
        std::string before = line.substr(0, comment_idx);
        attron(COLOR_PAIR(9));
        printw(before);
        attroff(COLOR_PAIR(9));
        std::string after = line.substr(comment_idx, static_cast<int>(line.size()) - comment_idx);
        myprintw_helper(after,  checker - 1, cur_line, lines, brackets, braces, parentheses);
    } else if(in_comment(cur_line)) { // currently in comment
        attron(COLOR_PAIR(9));
        printw(line);
        attroff(COLOR_PAIR(9));
    } else if(std::regex_search(line, comment_match, comment) && checker >= 8) { //contains comment
        int comment_idx = comment_match.position(0);
        std::string before = line.substr(0, comment_idx);
        myprintw_helper(before, checker - 1, cur_line, lines, brackets, braces, parentheses);
        attron(COLOR_PAIR(3));
        printw(line.substr(comment_idx, static_cast<int>(line.size()) - comment_idx));
        attroff(COLOR_PAIR(3));
    } else if(std::regex_search(line, match_preprocessor, preprocessor) && checker >= 7) { // preprocessor directive
        std::regex include_part("^( )*#( )*include|#( )*define( )+([^ ])+|#( )*endif|#( )*ifndef( )+([^ ])+");
        std::smatch include_part_match;
        std::regex_search(line, include_part_match, include_part);
        attron(COLOR_PAIR(8));
        printw(include_part_match[0]);
        attroff(COLOR_PAIR(8));
        size_t include_start = include_part_match.position(0);
        attron(COLOR_PAIR(7));
        printw(line.substr(include_start + include_part_match[0].length(), match_preprocessor[0].length() - (include_start + include_part_match[0].length())));
        attroff(COLOR_PAIR(7));
        size_t match_start = match_preprocessor.position(0);
        std::string rest = line.substr(match_start + match_preprocessor[0].length(), line.length() - (match_start + match_preprocessor[0].length()));
        myprintw_helper(rest, checker, cur_line, lines, brackets, braces, parentheses);
    } 
    else if(contains_match(line, string, match) && checker >= 6) {myprintw_only_colour_match(line, string, 3, checker, match, cur_line, lines, brackets, braces, parentheses); return;}
    else if(contains_match(line, data_types, match) && checker >= 5) {myprintw_only_colour_match(line, data_types, 6, checker, match, cur_line, lines, brackets, braces, parentheses); return;}
    else if(contains_match(line, keywords, match) && checker >= 4) {myprintw_only_colour_match(line, keywords, 5, checker, match, cur_line, lines, brackets, braces, parentheses); return;}
    else if(contains_match(line, numbers, match) && checker >= 3) {myprintw_only_colour_match(line, numbers, 4, checker, match, cur_line, lines, brackets, braces, parentheses); return;}
    else {
        print_mismatched(line, brackets, braces, parentheses);
    } 
}


void myprintw(std::string &line, int &cur_line, std::vector<std::string> &lines, int &brackets, int &braces, int &parentheses) {
    myprintw_helper(line, 10, cur_line, lines, brackets, braces, parentheses);
}




#endif