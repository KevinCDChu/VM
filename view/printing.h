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


bool contains_match(std::string str, std::regex re) {
    std::smatch match;
    return std::regex_search(str, match, re);
}


void myprintw(std::string line);
// int,

void strip_unnecessary_characters(std::string &first_string, std::string &middle, std::string &end_string, std::string &str) {
    if(middle[middle.length() - 1] == ')' || middle[middle.length() - 1] == ',' || middle[middle.length() - 1] == '(') {
        end_string = middle[middle.length() - 1];
        middle = middle.substr(0, middle.length() - 1);
    }
    size_t start_idx = str.find(middle);
    end_string += str.substr(start_idx + middle.length(), str.length() - (start_idx + middle.length()));
    first_string = str.substr(0, start_idx);
    if(middle[0] == '(' || middle[0] == ',' || middle[0] == ';') {
        first_string += middle[0];
        middle = middle.substr(1, middle.length() - 1);
    }

}


void myprintw_only_colour_match(std::string str, std::regex re, int colour) { // function assume that match exists!!! 
    std::smatch match;
    std::regex_search(str, match, re);
    std::string first_string;
    std::string middle = match[0];
    std::string end_string;
    strip_unnecessary_characters(first_string, middle, end_string, str); // in case (int or something
    myprintw(first_string);
    attron(COLOR_PAIR(colour));
    printw(middle);
    attroff(COLOR_PAIR(colour));
    myprintw(end_string);
}



std::regex init_data_types() {
    std::string str;
    str += "^int |[,( ]int[,) ]|[,( ]int$|";
    str += "^bool |[,( ]bool | bool$|";
    str += "^char |[,( ]char[,) ]|[,( ]char$|";
    str += "^void |[,( ]void[,) ]|[,( ]void$|";
    str += "^const |[,( ]const[,) ]|[,( ]const$|";
    str += "^virtual |[,( ]virtual[,) ]|[,( ]virtual$|";
    str += " override |";
    str += "^class |[,( ]class[,) ]|[,( ]class$";
    std::regex re(str);
    return re;
}

std::regex init_keywords() {
    std::string str;
    str += "return|";
    str += ";?if( )*\\(|";
    str += ";?for( )*\\(|";
    str += ":?public";
    std::regex re(str);
    return re;
}

std::regex init_numbers() {
    std::string str;
    str += "1|2|3|4|5|6|7|8|9|0";
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

std::regex data_types = init_data_types();
std::regex keywords = init_keywords();
std::regex numbers = init_numbers();
std::regex string = init_string();


void myprintw(std::string line) {
    if(line == "") return;
    std::regex comment("([^\"])*//([^\"])*");
    std::smatch comment_match;
    std::regex preprocessor("^( )*#( )*include( )*<.+>|^( )*#( )*include( )*\".+\"|#( )*ifndef( )+([^ ])+|#( )*define( )+([^ ])+|#( )*endif");
    std::smatch match_preprocessor;
    if(std::regex_search(line, comment_match, comment)) { //contains comment
        int comment_idx = line.find("//");
        myprintw(line.substr(0, comment_idx));
        attron(COLOR_PAIR(3));
        printw(line.substr(comment_idx, static_cast<int>(line.size()) - comment_idx));
        attroff(COLOR_PAIR(3));
    } else if(std::regex_search(line, match_preprocessor, preprocessor)) { // preprocessor directive
        std::regex include_part("^( )*#( )*include");
        std::smatch include_part_match;
        std::regex_search(line, include_part_match, include_part);
        attron(COLOR_PAIR(5));
        printw(include_part_match[0]);
        attroff(COLOR_PAIR(5));
        size_t include_start = line.find(include_part_match[0]);
        attron(COLOR_PAIR(4));
        printw(line.substr(include_start + include_part_match[0].length(), match_preprocessor[0].length() - (include_start + include_part_match[0].length())));
        attroff(COLOR_PAIR(4));
        size_t match_start = line.find(match_preprocessor[0]);
        myprintw(line.substr(match_start + match_preprocessor[0].length(), line.length() - (match_start + match_preprocessor[0].length())));
    } 
    else if(contains_match(line, string)) {myprintw_only_colour_match(line, string, 3); return;}
    else if(contains_match(line, data_types)) {myprintw_only_colour_match(line, data_types, 6); return;}
    else if(contains_match(line, keywords)) {myprintw_only_colour_match(line, keywords, 5); return;}
    else if(contains_match(line, numbers)) {myprintw_only_colour_match(line, numbers, 4); return;}
    else {
        printw(line);
    } 
}




#endif