// ========================================================================================>
// ZHRXXgroup Project 🚀 - ZLib
// File: zlib.h
// Author(s): ZHRXXgroup
// Version: 1
// ========================================================================================>

#ifndef STR_H
#define STR_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <stdbool.h>
#include "int.h"

char* from(const char* src, int init, int finish) {
    if (init < 0 || finish < 0 || init >= finish || finish > strlen(src)) {
        return NULL;
    }
    int length = finish - init;
    char* result = (char*)malloc(length + 1);
    if (result == NULL) {
        return NULL;
    }
    strncpy(result, src + init, length);
    result[length] = '\0';
    return result;
}

char** split(const char* src, const char* delimiter, int* count) {
    char* src_copy = strdup(src);
    char* token;
    int size = 10;
    char** result = (char**)malloc(size * sizeof(char*));
    *count = 0;

    token = strtok(src_copy, delimiter);
    while (token != NULL) {
        if (*count >= size) {
            size *= 2;
            result = (char**)realloc(result, size * sizeof(char*));
        }
        result[*count] = strdup(token);
        (*count)++;
        token = strtok(NULL, delimiter);
    }

    free(src_copy);
    return result;
}

int compare(const char* str1, const char* str2) {
    while (*str1 && *str2) {
        while (isspace(*str1)) str1++;
        while (isspace(*str2)) str2++;

        if (isdigit(*str1) && isdigit(*str2)) {
            long num1 = strtol(str1, (char**)&str1, 10);
            long num2 = strtol(str2, (char**)&str2, 10);
            if (num1 != num2) {
                return (num1 < num2) ? -1 : 1;
            }
        } else {
            if (*str1 != *str2) {
                return (*str1 < *str2) ? -1 : 1;
            }
            str1++;
            str2++;
        }
    }
    return (*str1 == '\0' && *str2 == '\0') ? 0 : (*str1 == '\0') ? -1 : 1;
}

bool starts_with(const char *prefix, const char *string)
{
    return strncmp(prefix, string, strlen(prefix)) == 0;
}

bool ends_with(const char *suffix, const char *string)
{
    size_t string_len = strlen(string);
    size_t suffix_len = strlen(suffix);

    if (suffix_len > string_len) return false;

    return strncmp(string + string_len - suffix_len, suffix, suffix_len) == 0;
}

// Code taken from https://stackoverflow.com/a/779960 with some change
char *replace(char *original, const char *search, const char *replace_with) {
    if (!original || !search || strlen(search) == 0) return NULL;
    if (!replace_with) replace_with = "";

    int search_len = strlen(search), replace_len = strlen(replace_with);
    int occurrences = 0;
    char *current_pos = original;

    while ((current_pos = strstr(current_pos, search))) {
        occurrences++;
        current_pos += search_len;
    }

    if (occurrences == 0) return original;

    char *result = malloc(strlen(original) + (replace_len - search_len) * occurrences + 1);
    if (!result) return NULL;

    char *temp = result;
    current_pos = original;
    while (occurrences--) {
        char *match_pos = strstr(current_pos, search);
        temp = strncpy(temp, current_pos, match_pos - current_pos) + (match_pos - current_pos);
        temp = strcpy(temp, replace_with) + replace_len;
        current_pos = match_pos + search_len;
    }

    strcpy(temp, current_pos);
    return result;
}


char* strip(const char* str, char ch) {
    if (!str) return NULL;

    const char* start = str;
    while (*start == ch) start++;

    const char* end = str + strlen(str) - 1;
    while (end > start && *end == ch) end--;

    size_t new_len = end - start + 1;

    char* result = (char*)malloc(new_len + 1);
    if (!result) return NULL;

    memcpy(result, start, new_len);
    result[new_len] = '\0';

    return result;
}



static const char* base64_chars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

char* base64_encode(const unsigned char* data, size_t input_length) {
    size_t output_length = 4 * ((input_length + 2) / 3);
    char* encoded_data = (char*)malloc(output_length + 1);
    if (!encoded_data) return NULL;

    for (size_t i = 0, j = 0; i < input_length;) {
        uint32_t octet_a = i < input_length ? data[i++] : 0;
        uint32_t octet_b = i < input_length ? data[i++] : 0;
        uint32_t octet_c = i < input_length ? data[i++] : 0;

        uint32_t triple = (octet_a << 16) + (octet_b << 8) + octet_c;

        encoded_data[j++] = base64_chars[(triple >> 18) & 0x3F];
        encoded_data[j++] = base64_chars[(triple >> 12) & 0x3F];
        encoded_data[j++] = (i > input_length + 1) ? '=' : base64_chars[(triple >> 6) & 0x3F];
        encoded_data[j++] = (i > input_length) ? '=' : base64_chars[triple & 0x3F];
    }

    encoded_data[output_length] = '\0';
    return encoded_data;
}

unsigned char* base64_decode(const char* encoded_data, size_t* output_length) {
    size_t input_length = strlen(encoded_data);
    if (input_length % 4 != 0) return NULL;

    size_t padding = 0;
    if (encoded_data[input_length - 1] == '=') padding++;
    if (encoded_data[input_length - 2] == '=') padding++;

    *output_length = (input_length / 4) * 3 - padding;
    unsigned char* decoded_data = (unsigned char*)malloc(*output_length);
    if (!decoded_data) return NULL;

    for (size_t i = 0, j = 0; i < input_length;) {
        uint32_t sextet_a = encoded_data[i] == '=' ? 0 : strchr(base64_chars, encoded_data[i]) - base64_chars;
        uint32_t sextet_b = encoded_data[i + 1] == '=' ? 0 : strchr(base64_chars, encoded_data[i + 1]) - base64_chars;
        uint32_t sextet_c = encoded_data[i + 2] == '=' ? 0 : strchr(base64_chars, encoded_data[i + 2]) - base64_chars;
        uint32_t sextet_d = encoded_data[i + 3] == '=' ? 0 : strchr(base64_chars, encoded_data[i + 3]) - base64_chars;

        uint32_t triple = (sextet_a << 18) + (sextet_b << 12) + (sextet_c << 6) + sextet_d;

        if (j < *output_length) decoded_data[j++] = (triple >> 16) & 0xFF;
        if (j < *output_length) decoded_data[j++] = (triple >> 8) & 0xFF;
        if (j < *output_length) decoded_data[j++] = triple & 0xFF;

        i += 4;
    }

    return decoded_data;
}

char *generate_random_string(int length) {
    char *random_string = (char *)malloc(length + 1);
    if (random_string == NULL) return NULL;

    const char *charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    size_t charset_length = strlen(charset);
    for (int i = 0; i < length; i++) {
        random_string[i] = charset[random_int(0, charset_length)];
    }
    random_string[length] = '\0';
    return random_string;
}


int contains(const char *str, const char *sub) {
    return strstr(str, sub) != NULL;
}



#endif
