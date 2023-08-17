#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

typedef struct {
    char letter;
    int positions[5];
    int count;
} MapEntry;

int get_or_add_entry(MapEntry map[], char letter, int size) {
    for (int i = 0; i < size; ++i) {
        if (map[i].letter == letter) {
            return i;
        }
        if (map[i].letter == 0) {
            map[i].letter = letter;
            map[i].count = 0;
            return i;
        }
    }
    return -1;
}

bool contains(int array[], int size, int value) {
    for (int i = 0; i < size; ++i) {
        if (array[i] == value) {
            return true;
        }
    }
    return false;
}

int main() {
    const char secret_word[] = "udder";
    char my_guess[6];

    printf("Enter a five letter word: ");
    scanf("%5s", my_guess);

    for (int i = 0; my_guess[i]; ++i) {
        my_guess[i] = tolower(my_guess[i]);
    }

    MapEntry s[5] = {0};
    MapEntry g[5] = {0};
    MapEntry times_guessed[5] = {0};
    
    for (int i = 0; secret_word[i]; ++i) {
        int index = get_or_add_entry(s, secret_word[i], 5);
        s[index].positions[s[index].count++] = i;
    }
    
    for (int i = 0; my_guess[i]; ++i) {
        int index = get_or_add_entry(g, my_guess[i], 5);
        if (g[index].count == 0) {
            times_guessed[index].letter = my_guess[i];
            times_guessed[index].count = 0;
        }
        g[index].positions[g[index].count++] = i;
    }

    char hints[6] = {'-', '-', '-', '-', '-', '\0'};

    for (int i = 0; my_guess[i]; ++i) {
        int s_index = get_or_add_entry(s, my_guess[i], 5);
        int g_index = get_or_add_entry(g, my_guess[i], 5);
        if (s_index != -1 && contains(s[s_index].positions, s[s_index].count, i)) {
            hints[i] = toupper(my_guess[i]);
            times_guessed[g_index].count++;
        }
    }

    for (int i = 0; my_guess[i]; ++i) {
        int s_index = get_or_add_entry(s, my_guess[i], 5);
        int g_index = get_or_add_entry(g, my_guess[i], 5);
        if (s_index != -1 && !contains(s[s_index].positions, s[s_index].count, i) &&
            times_guessed[g_index].count < s[s_index].count) {
            hints[i] = my_guess[i];
            times_guessed[g_index].count++;
        }
    }

    printf("%s\n", hints);
    return 0;
}
