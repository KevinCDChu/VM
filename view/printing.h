#ifndef PRINTING_H
#define PRINTING_H
#include <ncurses.h>
#include <string>
#include <regex>


void init_colours(bool code_file) {
    init_pair(1, COLOR_BLUE, COLOR_BLACK);
    init_pair(2, COLOR_WHITE, COLOR_RED);
    if(code_file) {
        init_pair(3, COLOR_GREEN, COLOR_BLACK);
        init_pair(4, COLOR_CYAN, COLOR_BLACK);
        init_pair(5, COLOR_YELLOW, COLOR_BLACK);
        init_pair(6, COLOR_MAGENTA, COLOR_BLACK);
    } else {
        init_pair(3, COLOR_WHITE, COLOR_BLACK);
        init_pair(4, COLOR_WHITE, COLOR_BLACK);
        init_pair(5, COLOR_WHITE, COLOR_BLACK);
        init_pair(6, COLOR_WHITE, COLOR_BLACK);
    }
}


bool contains_match(std::string &str, std::regex &re, std::smatch &match) {
    return std::regex_search(str, match, re);
}


void myprintw(std::string &line);

void strip_unnecessary_characters(std::string &first_string, std::string &middle, std::string &end_string, std::string &str, std::smatch &match) {
    int length = middle.length();
    if(middle[middle.length() - 1] == ')' || middle[middle.length() - 1] == ',' || middle[middle.length() - 1] == '(' || middle[middle.length() - 1] == '>') {
        end_string = middle[middle.length() - 1];
        middle = middle.substr(0, length - 1);
    }
    size_t start_idx = match.position(0);
    end_string += str.substr(start_idx + length, str.length() - (start_idx + length));
    first_string = str.substr(0, start_idx);
    if(middle[0] == '(' || middle[0] == ',' || middle[0] == ';' || middle[0] == '<') {
        first_string += middle[0];
        middle = middle.substr(1, middle.length() - 1);
    }

}


void myprintw_helper(std::string &line, int checker);

void myprintw_only_colour_match(std::string &str, std::regex &re, int colour, int checker, std::smatch &match) { // function assume that match exists!!! 
    std::regex_search(str, match, re);
    std::string first_string;
    std::string middle = match[0];
    std::string end_string;
    strip_unnecessary_characters(first_string, middle, end_string, str, match); // in case (int or something
    myprintw_helper(first_string, checker - 1);
    attron(COLOR_PAIR(colour));
    printw(middle);
    attroff(COLOR_PAIR(colour));
    myprintw_helper(end_string, checker);
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
    str += "return|";
    str += ";?if( )*\\(?|";
    str += ";?for( )*\\(?|";
    str += ";?while( )*\\(?|";
    str += "}?else*\\(?|";
    str += " new|";
    str += ":?public|";
    str += ":?protected|";
    str += ":?private";


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
    str += "\\\".*\\\"|";
    str += "'.*'";
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

std::regex data_types = init_data_types();
std::regex keywords = init_keywords();
std::regex numbers = init_numbers();
std::regex string = init_string();
std::regex comment = init_comment();
std::regex preprocessor = init_preprocessor();



void myprintw_helper(std::string &line, int checker) {
    if(line == "") return;
    std::smatch match;
    std::smatch comment_match;
    std::smatch match_preprocessor;
    if(std::regex_search(line, comment_match, comment) && checker >= 8) { //contains comment
        int comment_idx = comment_match.position(0);
        std::string before = line.substr(0, comment_idx);
        myprintw_helper(before, checker - 1);
        attron(COLOR_PAIR(3));
        printw(line.substr(comment_idx, static_cast<int>(line.size()) - comment_idx));
        attroff(COLOR_PAIR(3));
    } else if(std::regex_search(line, match_preprocessor, preprocessor)) { // preprocessor directive
        std::regex include_part("^( )*#( )*include|#( )*define( )+([^ ])+|#( )*endif|#( )*ifndef( )+([^ ])+");
        std::smatch include_part_match;
        std::regex_search(line, include_part_match, include_part);
        attron(COLOR_PAIR(5));
        printw(include_part_match[0]);
        attroff(COLOR_PAIR(5));
        size_t include_start = include_part_match.position(0);
        attron(COLOR_PAIR(4));
        printw(line.substr(include_start + include_part_match[0].length(), match_preprocessor[0].length() - (include_start + include_part_match[0].length())));
        attroff(COLOR_PAIR(4));
        size_t match_start = match_preprocessor.position(0);
        std::string rest = line.substr(match_start + match_preprocessor[0].length(), line.length() - (match_start + match_preprocessor[0].length()));
        myprintw_helper(rest, checker);
    } 
    else if(contains_match(line, string, match) && checker >= 6) {myprintw_only_colour_match(line, string, 3, checker, match); return;}
    else if(contains_match(line, data_types, match) && checker >= 5) {myprintw_only_colour_match(line, data_types, 6, checker, match); return;}
    else if(contains_match(line, keywords, match) && checker >= 4) {myprintw_only_colour_match(line, keywords, 5, checker, match); return;}
    else if(contains_match(line, numbers, match) && checker >= 3) {myprintw_only_colour_match(line, numbers, 4, checker, match); return;}
    else {
        printw(line);
    } 
}


void myprintw(std::string &line) {
    myprintw_helper(line, 8);
}




#endif