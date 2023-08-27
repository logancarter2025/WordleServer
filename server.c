
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

char** valid_words;

typedef struct {
    char letter;
    int* positions;
    int count;
} MapEntry;

 struct playGameArgs {
    int numValidWords;
    int socketDescriptor;
};

pthread_mutex_t mutex_lock_guesses = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t total_wins_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t total_losses_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t add_word = PTHREAD_MUTEX_INITIALIZER;

char* getHint(char* secret_word, char* my_guess);


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

            return true;
        }
    }
    return false;
}

/*
Socket descriptor in fd table is like a pipe, pass that into wordle_server
do receive on that socket descriptor to get the buffer
to send in back, use send(), both send and receive are in play game\
*/


void* playGame(void* x){
    int num_valid_words = ((struct playGameArgs*)  x)->numValidWords;
    int socketDescriptor = ((struct playGameArgs*) x)->socketDescriptor;

    // Generate secret word
    int secret_word_index = rand() % num_valid_words;
    char* secret_word = *(valid_words + secret_word_index);

    *(secret_word + 5) = '\0';
    
    

    int num_guesses_left = 6;
    char* my_guess = calloc(6, sizeof(char));
    
    while(num_guesses_left > 0){
       
        //receive buffer from client
        printf("THREAD %ld: waiting for guess\n", pthread_self());
        
        char* buffer = calloc(6, sizeof(char));
        int n = recv(socketDescriptor, buffer, 5, 0);
        if (n == -1){
            printf("recv() failed\n");
            exit(0);
        }
        if(n == 0){

            printf("Client closed connection\n");
            return EXIT_SUCCESS;
        }
        

        for(int i = 0; i < 5; i++){
            *(my_guess + i) = tolower(*(buffer + i));
        }
        printf("THREAD %ld: rcvd guess: %s\n", pthread_self(), my_guess);
            
        if(isInDict(my_guess, valid_words, num_valid_words)  ){
            
            *(my_guess + 5) = '\0';
            num_guesses_left--;
            pthread_mutex_lock( &mutex_lock_guesses );
            {
               total_guesses += 1;   /* CRITICAL SECTION */
            }
            pthread_mutex_unlock( &mutex_lock_guesses );


            
            char* hint = getHint(secret_word, my_guess);

            char* buffer_send = calloc(9, sizeof(char));
            *(buffer_send + 0) = 'Y';

            short* guesses = (short*)(buffer_send+1);
            *guesses = htons((short)num_guesses_left);
            strcpy(buffer_send + 3, hint);

            if (num_guesses_left != 1){
                printf("THREAD %ld: sending reply: %s (%d guesses left)\n", pthread_self(), hint, num_guesses_left);
            }
            else{
                printf("THREAD %ld: sending reply: %s (1 guess left)\n", pthread_self(), hint);
            }
            
            int rc = send(socketDescriptor, buffer_send, 9, 0);
            if(rc == -1){
                printf("send() failed\n");
                exit(0);
            }

            
            if(strcmp(secret_word, my_guess) == 0){
      
                pthread_mutex_lock( &total_wins_lock );
                {
                    total_wins += 1;   /* CRITICAL SECTION */
                }
                pthread_mutex_unlock( &total_wins_lock );
                close(socketDescriptor);
                break;
            }
             
        }
        else{

            char* buffer_send = calloc(9, sizeof(char));
            
            short* guesses = (short*)(buffer_send+1);
            *guesses = htons((short)num_guesses_left);
            
            *(buffer_send + 0) = 'N';
            strcpy(buffer_send + 3, "?????");
            printf("invalid guess; sending reply: ");
            printf("invalid guess; sending reply: ????? (%d guesses left)\n", num_guesses_left);
            int rc = send(socketDescriptor, buffer_send, 9, 0);
            if(rc == -1){
                printf("send() failed\n");
                exit(0);
            }
            
        }   
    }
 
    if(num_guesses_left == 0){
        printf("out of guesses");
        pthread_mutex_lock(&total_losses_lock);
        {
        total_losses += 1;
        }
        pthread_mutex_unlock(&total_losses_lock);
    }
    close(socketDescriptor);
    free(my_guess);   
    pthread_detach(pthread_self());

  
    printf(" game over; word was ");
    for(int i = 0; i < 5; i++){
        printf("%c", toupper(*(secret_word + i)));
    }
    printf("!\n");
    printf("MAIN: SIGUSR1 rcvd; Wordle server shutting down...\n");
    printf("MAIN:MAIN: valid guesses: %d\n", total_guesses);
    printf("MAIN:MAIN: win/loss: %d/%d\n", total_wins, total_losses);
    printf("MAIN:MAIN: word #1: ");
    for(int i = 0; i < 5; i++){
        printf("%c", toupper(*(secret_word + i)));
    }
    printf("\n");
    
    return NULL;
}

bool contains(int* array, int size, int value) {
    for (int i = 0; i < size; ++i) {
        if (*(array+i) == value) {
            return true;
        }
 
    }
    return false;
}

char* getHint(char* secret_word, char* my_guess) {   
    
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

    for (int i = 0; i < 5; i++) {
        free((s + i)->positions);
        free((g + i)->positions);
    }
    
    free(s);
    free(g);
    free(times_guessed);
    return hints;
}

char** read_words(char* filename, int words) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        perror("ERROR: Could not open file");
        exit(EXIT_FAILURE);
    }

    char* buffer = (char*) calloc(8, sizeof(char)); // Allocate space for 6 characters + newline
   

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
    int num_words_in_file = atoi(*(argv + 4));

    printf("MAIN: opened %s (%d words)\n",word_file, num_words_in_file);
    printf("MAIN: seeded pseudo-random number generator with %d\n", seed);
    printf("MAIN: Wordle server listening on port {%d}\n",port_num);
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

    valid_words = read_words(word_file, num_words_in_file);
    
    int listener = socket(AF_INET, SOCK_STREAM, 0);
    if (listener < 0) {
        perror("ERROR: Could not create socket");
        return EXIT_FAILURE;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_num); // port_num is the port number you want to listen on
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(listener, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("ERROR: Could not bind socket");
        return EXIT_FAILURE;
    }


    if (listen(listener, 5) < 0) { // 5 is the maximum len to which the queue of pending connections may grow.
        perror("ERROR: Could not listen on socket");
        return EXIT_FAILURE;
    }
    if (false){ //prints contents of word file
        for(char** ptr = valid_words; *ptr; ptr++){
            printf("%s\n", *ptr);
        }
    }


    while (true) {
        struct sockaddr_in remote_client;
        int addrlen = sizeof(remote_client);

       
        int newsd = accept(listener, (struct sockaddr *)&remote_client, (socklen_t *)&addrlen);
        //printf("SERVER: Accepted new client connection on newsd %d\n", newsd);

        pthread_t tid;
        struct playGameArgs *x = calloc(1, sizeof(int) + sizeof(int));

        x->numValidWords = num_words_in_file;
        x->socketDescriptor = newsd;
       
        if (pthread_create(&tid, NULL, playGame, (void*)x) != 0) {
            perror("pthread_create() failed");
            close(newsd);

        }

    }
    close(listener);

    for(int i = 0; i < num_words_in_file; i++){
        free(*(valid_words + i));
    }
        
    free(valid_words);
   

   
    return EXIT_SUCCESS;
}

