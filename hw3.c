#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>
#include <ctype.h>
#include <signal.h>
#include <stdbool.h>

typedef struct {
    char letter;
    int* positions;
    int count;
} MapEntry;

int get_or_add_entry(MapEntry *map, char letter, int size) {
    for (int i = 0; i < size; ++i) {
        if ((*(map + i)).letter == letter) {
            return i;
        }
        if ((*(map + i)).letter == 0) {
            (*(map + i)).letter = letter;
            (*(map + i)).count = 0;
            return i;
        }
    }
    return -1;
    
}

bool contains(int* array, int size, int value) {
    for (int i = 0; i < size; ++i) {
        if (*(array+i) == value) {
            return true;
        }
    }
    return false;
}

void getHint(const char* secret_word, char* my_guess) {
    for (int i = 0; *(my_guess + i); ++i) {
        *(my_guess + i) = tolower(*(my_guess + i));
    }

    MapEntry *s = (MapEntry *) calloc(5, sizeof(MapEntry));
    MapEntry *g = (MapEntry *) calloc(5, sizeof(MapEntry));
    for (int i = 0; i < 5; i++) {
        (*(s + i)).positions = (int*)calloc(5, sizeof(int));
        (*(g + i)).positions = (int*)calloc(5, sizeof(int));
    }
    
    MapEntry *times_guessed = (MapEntry *) calloc(5, sizeof(MapEntry));
    
    for (int i = 0; *(secret_word + i); ++i) {
        int index = get_or_add_entry(s, *(secret_word + i), 5);
        *((s + index)->positions + (s + index)->count++) = i;
    }
    
    for (int i = 0; *(my_guess + i); ++i) {
        int index = get_or_add_entry(g, *(my_guess + i), 5);
        if ((*(g + index)).count == 0) {
            (*(times_guessed + index)).letter = *(my_guess + i);
            (*(times_guessed + index)).count = 0;
        }
        *((g + index)->positions + (g + index)->count++) = i;
    }

    char* hints = calloc(6, sizeof(char));
    for(int i = 0; i < 5; i++){
        *(hints + i) = '-';
    }
    *(hints + 5) = '\0';

    for (int i = 0; *(my_guess + i); ++i) {
        int s_index = get_or_add_entry(s, *(my_guess + i), 5);
        int g_index = get_or_add_entry(g, *(my_guess + i), 5);
        if (s_index != -1 && contains((*(s + s_index)).positions, (*(s + s_index)).count, i)) {
            *(hints + i) = toupper(*(my_guess + i));
            (*(times_guessed + g_index)).count++;
        }
    }

    for (int i = 0; *(my_guess + i); ++i) {
        int s_index = get_or_add_entry(s, *(my_guess + i), 5);
        int g_index = get_or_add_entry(g, *(my_guess + i), 5);
        if (s_index != -1 && !contains((*(s + s_index)).positions, (*(s + s_index)).count, i) &&
            (*(times_guessed + g_index)).count < (*(s + s_index)).count) {
            *(hints + i) = *(my_guess + i);
            (*(times_guessed + g_index)).count++;
        }
    }

    printf("%s\n", hints);

    for (int i = 0; i < 5; i++) {
        free((s + i)->positions);
        free((g + i)->positions);
    }
    
    free(s);
    free(g);
    free(times_guessed);
    free(hints);
    return;
}

char** read_words(char* filename, int words) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        perror("ERROR: Could not open file");
        exit(EXIT_FAILURE);
    }

    char* buffer = (char*) calloc(6, sizeof(char));
    printf("opened %s (%d word", filename, words);
    if (words > 1) {
        printf("s)\n");
    } else {
        printf(")\n");
    }
    
    char** word_dictionary = (char**) calloc(words + 1, sizeof(char*)); // Allocating for char*
    for (int i = 0; i < words; i++) { // Looping for all words
        if (fgets(buffer, 6, file) == NULL) { // Reading 6 characters + newline
          printf("ERROR: invalid argument(s)\n");
          printf("USAGE: hw3.out <listener-port> <seed> <word-filename> <num-words>\n");
        }
        *(buffer + strcspn(buffer, "\n")) = '\0'; // Removing newline character
        *(buffer + strcspn(buffer, "\n")) = '\0';
        *(word_dictionary + i) = calloc(5, sizeof(char));
        strcpy(*( word_dictionary + i), buffer);
    }
    *(word_dictionary + words) = NULL;

    free(buffer);
    fclose(file);

    return word_dictionary;
}

int wordle_server(int argc, char** argv){
    const char* secret_word = "udder";
    char* my_guess = calloc(6, sizeof(char));

    printf("Enter a five letter word: ");
    scanf("%5s", my_guess);
    getHint(secret_word, my_guess);

    
    if (true){ //set to True when uploading to submitty
        setvbuf( stdout, NULL, _IONBF, 0 );
    } 
  
    if (argc != 5){
        perror("ERROR: Invalid argument(s)\n");
        perror("USAGE: hw4.out <listener-port> <seed> <word-filename> <num-words>\n\n");
        return EXIT_FAILURE;
    }


    //TCP NUM
    int port_num = atoi(*(argv + 1));
    
    int seed = atoi(*(argv + 2));
    char* word_file = *(argv + 3);
    int num_words_in_file = atoi(*(argv + 4));

    printf("PortNum: %d, seed: %d, word_file: %s, num_words: %d\n", port_num, seed, word_file, num_words_in_file);
    char** valid_words = read_words(word_file, num_words_in_file);
    
    if (true){ //prints contents of word file
        for(char** ptr = valid_words; *ptr; ptr++){
            printf("%s\n", *ptr);
        }
    }
    
    free(my_guess);
    for(int i = 0; i < num_words_in_file; i++){
        free(*(valid_words + i));
    }
        
    free(valid_words);

    return EXIT_SUCCESS;
}