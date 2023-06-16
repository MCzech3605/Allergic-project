#include <cpr/cpr.h>
#include <iostream>
#include <ctime>

std::string saved_data = "data.txt";
std::string url = "https://www.medicover.pl/kalendarz-pylenia/";

bool same_str(const std::string &str1, const std::string &str2, unsigned long char_num, unsigned long be1 = 0,
              unsigned long be2 = 0) {
    for (int i = 0; i < char_num; i++) {
        if (str1[be1 + i] != str2[be2 + i])
            return false;
    }
    return true;
} // checks if string [be1, ..., be1+char_num-1] and string [be2, ..., be2+char_num-1] are the same

std::string get_word_between(unsigned long be, unsigned long en, const std::string &target) {
    std::string res;
    while (be < en) {
        res += target[be];
        be++;
    }
    return res;
} // returns substring between be and en indexes in target string

void go_to_data(unsigned long &current_index, const std::string &destination, const std::string &target) {
    unsigned long t_len = target.length(), d_len = destination.length();
    bool flag = false;
    while (current_index < t_len && !flag) {
        if (same_str(destination, target, d_len, 0, current_index)) {
            flag = true;
        } else {
            current_index++;
        }
    }
} // moves current_index pointer until it finds destination string in target. If destination not found - returns target length

void get_allergens(unsigned long &current_index, const std::string &target, std::vector<std::string> &allergens,
                   int month, int part) {
    // find end of the table
    unsigned long finish_index = current_index;
    std::string to_go = "</table>";
    go_to_data(finish_index, to_go, target);
    // skip months sections
    std::string allergen_id = "<td colspan=3>";
    go_to_data(current_index, allergen_id + "Grud", target);
    current_index++;
    // get all allergens
    while (current_index < finish_index) {
        go_to_data(current_index, allergen_id, target);
        if (finish_index <= current_index)
            break;
        go_to_data(current_index, ">", target);
        current_index++;
        unsigned long word_end = current_index;
        go_to_data(word_end, "<", target);
        std::string allergen_description = get_word_between(current_index, word_end, target);
        allergen_description += ":\nZnaczenie kliniczne: ";
        go_to_data(current_index, "<td>", target);
        go_to_data(current_index, ">", target);
        current_index++;
        word_end = current_index;
        go_to_data(word_end, "<", target);
        allergen_description += get_word_between(current_index, word_end, target);
        allergen_description += "\nStezenie pylku: ";
        for (int i = 0; i < 3 * (month - 1) + part + 1; i++) {
            go_to_data(current_index, "<td class=pylenie__sqr", target);
            current_index++;
        }
        bool add_to_vec = false;
        std::string small = "td class=pylenie__sqr style=background:#f7d019>";
        std::string big = "td class=pylenie__sqr style=background:#a3231d>";
        unsigned long s_len = small.length(), b_len = big.length();
        if (same_str(big, target, b_len, 0, current_index)) {
            add_to_vec = true;
            allergen_description += "wysokie\n\n";
        } else if (same_str(small, target, s_len, 0, current_index)) {
            add_to_vec = true;
            allergen_description += "niskie\n\n";
        }
        if (add_to_vec) {
            allergens.push_back(allergen_description);
        }
    }
} // saves today's allergens to the allergens vector

int get_region(bool data_exists) {
    std::cout << "Wybierz swoj region (mapa dostepna na " << url << " )\n>> ";
    int x;
    std::cin >> x;
    while (x < 1 || x > 4) {
        std::cout << "Region nie jest w prawidlowym formacie. Wybierz liczbe od 1 do 4\n>> ";
        std::cin >> x;
    }
    if (data_exists) {
        std::remove(saved_data.c_str());
    }
    std::ofstream data(saved_data);
    data << x;
    data.close();
    return x;
} // inserts new region into the data.txt file and returns it as int

void print_allergens(int month, int part, int region) {
    cpr::Response r = cpr::Get(cpr::Url{url});
    std::string s = r.text;                         // JSON text string
    std::string table_line = R"(<table id=table)" + std::to_string(region);
    unsigned long ind = 0;
    go_to_data(ind, table_line, s);
    // works up to here
    std::vector<std::string> allergens;
    get_allergens(ind, s, allergens, month, part);
    std::cout << "Dzisiaj w wybranym regionie (" << region << ") pyla:\n\n";
    for (const auto &allergen: allergens) {
        std::cout << allergen;
    }
    char x;
    std::cout << "Jesli chcesz zmienic region wpisz \"r\". Jesli nie, wpisz cokolwiek innego\n>> ";
    std::cin >> x;
    if (x == 'r') {
        int reg = get_region(true);
        print_allergens(month, part, reg);
    }
} // prints all the allergens and asks about region change

int main(int argc, char **argv) {
    std::time_t t = std::time(nullptr);   // get time now
    std::tm *now = std::localtime(&t);
    int month = now->tm_mon + 1;
    int part = (now->tm_mday - 1) / 10;
    if (part > 2)
        part = 2;
    // get region
    int region;
    if (access(saved_data.c_str(), F_OK) == -1) {
        region = get_region(false);
    } else {
        std::ifstream data(saved_data);
        std::string tmp;
        getline(data, tmp);
        data.close();
        region = std::stoi(tmp);
        if (region < 1 || region > 4)
            region = get_region(true);
    }

    print_allergens(month, part, region);
    return 0;
}