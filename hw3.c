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

extern int total_guesses;
extern int total_wins;
extern int total_losses;
extern char ** words;

int num_words_in_file;

typedef struct {
    char letter;
    int* positions;
    int count;
} MapEntry;

void playGame(char** valid_words, int num_valid_words, int client_sockfd);


void* handle_client(void* arg) {
    int client_sockfd = (int)(intptr_t)arg;

    char welcome_msg[] = "Welcome to Wordle! Let's play a game.\n";
    send(client_sockfd, welcome_msg, strlen(welcome_msg), 0);

    playGame(words, num_words_in_file, client_sockfd); // Use num_words_in_file instead of total_words

    close(client_sockfd);

    return NULL;
}


void getHint(char* secret_word, char* my_guess, int client_sockfd);


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

bool isInDict(char* word, char** dict, int numWords){

    for(int i = 0; i < numWords; i++){
        if( *(word + 0) == *(*(dict + i) + 0) &&  *(word + 1) == *(*(dict + i) + 1) && *(word + 2) == *(*(dict + i) + 2) && 
                *(word + 3) == *(*(dict + i) + 3) && *(word + 4) == *(*(dict + i) + 4)){
            
            printf("IN DICT\n");
            return true;
        }
    }
    return false;
}


void playGame(char** valid_words, int num_valid_words, int client_sockfd){

 int secret_word_index = rand() % num_valid_words;
    char* secret_word = *(valid_words + secret_word_index);
    *(secret_word + 5) = '\0';

    char buffer[1024];
    snprintf(buffer, sizeof(buffer), "The secret word is: %s\n", secret_word);
    send(client_sockfd, buffer, strlen(buffer), 0);

    int num_guesses_left = 6;
    char* my_guess = calloc(6, sizeof(char));
    *(my_guess + 5) = '\0';

    while (num_guesses_left > 0) {
        snprintf(buffer, sizeof(buffer), "Enter a five-letter word (guesses remaining: %d): ", num_guesses_left);
        send(client_sockfd, buffer, strlen(buffer), 0);
        recv(client_sockfd, my_guess, 5, 0);
        my_guess[5] = '\0';

        for (int i = 0; i < 5; i++) {
            *(my_guess + i) = tolower(*(my_guess + i));
        }

        if (isInDict(my_guess, valid_words, num_valid_words)) {
            *(my_guess + 5) = '\0';
            total_guesses += 1;
            getHint(secret_word, my_guess, client_sockfd);
            num_guesses_left--;

            if (strcmp(secret_word, my_guess) == 0) {
                send(client_sockfd, "Congratulations\n", 17, 0);
                total_wins += 1;
                break;
            }
        } else {
            send(client_sockfd, "?????\n", 6, 0);
        }
    }

    if (num_guesses_left == 0) {
        send(client_sockfd, "out of guesses", 15, 0);
        total_losses += 1;
    }

    free(my_guess);  
    

}


bool contains(int* array, int size, int value) {
    for (int i = 0; i < size; ++i) {
        if (*(array+i) == value) {
            return true;
        }
 
    }
    return false;
}

void getHint(char* secret_word, char* my_guess, int client_sockfd){   
    
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

    send(client_sockfd, hints, strlen(hints), 0);
    send(client_sockfd, "\n", 1, 0);


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

    char* buffer = (char*) calloc(8, sizeof(char)); // Allocate space for 6 characters + newline
    printf("opened %s (%d word", filename, words);
    if (words > 1) {
        printf("s)\n");
    } else {
        printf(")\n");
    }

    char** word_dictionary = (char**) calloc(words + 1, sizeof(char*)); // Allocating for char*
    for (int i = 0; i < words; i++) { // Looping for all words
        if (fgets(buffer, 8, file) == NULL) {
            if (strcmp(buffer, "\n") == 0) {
                break;
            }
            printf("ERROR: invalid argument(s)\n");
            printf("USAGE: hw3.out <listener-port> <seed> <word-filename> <num-words>\n");
            exit(EXIT_FAILURE);
        }
        *(buffer + strcspn(buffer, "\n")) = '\0'; // Removing newline character
        *(word_dictionary + i) = calloc(7, sizeof(char)); // Allocating space for 6 characters + null terminator
        strcpy(*(word_dictionary + i), buffer);
    }
    *(word_dictionary + words) = NULL; // Correctly setting the last element to NULL

    free(buffer);
    fclose(file);

    return word_dictionary;
}



int wordle_server(int argc, char** argv){
    int port_num = atoi(*(argv + 1));
    int seed = atoi(*(argv + 2));
    char* word_file = *(argv + 3);
    num_words_in_file = atoi(*(argv + 4));

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
    perror("ERROR: Could not create socket");
    return EXIT_FAILURE;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_num);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("ERROR: Could not bind socket");
        return EXIT_FAILURE;
    }

    if (listen(sockfd, 5) < 0) { // 5 is the backlog queue size
        perror("ERROR: Could not listen on socket");
        return EXIT_FAILURE;
    }


    printf("MAIN: opened %s (%d words)\n",word_file, num_words_in_file);
    printf("MAIN: seeded pseudo-random number generator with %d\n", seed);
    printf("MAIN: Wordle server listening on port {%d}",port_num);
    printf("MAIN: rcvd incoming connection request\n");



    
    if (true){ //set to True when uploading to submitty
        setvbuf( stdout, NULL, _IONBF, 0 );
    } 
  
    if (argc != 5){
        perror("ERROR: Invalid argument(s)\n");
        perror("USAGE: hw4.out <listener-port> <seed> <word-filename> <num-words>\n\n");
        return EXIT_FAILURE;
    }


    //TCP NUM
    

    srand(seed);

    //printf("PortNum: %d, seed: %d, word_file: %s, num_words: %d\n", port_num, seed, word_file, num_words_in_file);
    words = read_words(word_file, num_words_in_file);
    
  /* ========================= network setup code above ==================== */

    while (1) {
        struct sockaddr_in remote_client;
        socklen_t client_len = sizeof(remote_client);

        int client_sockfd = accept(sockfd, (struct sockaddr *)&remote_client, &client_len);
        if (client_sockfd == -1) {
            perror("accept() failed");
            continue;
        }

        pthread_t thread;
        if (pthread_create(&thread, NULL, handle_client, (void *)(intptr_t)client_sockfd) != 0) {
            perror("ERROR: Could not create thread");
            close(client_sockfd);
            continue;
        }
        pthread_detach(thread);
    }

    close(sockfd);
    return EXIT_SUCCESS;

  
}