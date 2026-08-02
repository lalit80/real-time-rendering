#include<stdio.h>
#include<string.h>
#include<stdlib.h>

int main(void) {
    char date_str[] = "13/11/2025";
    char vertex_entry[] = "v 1.1 2.2 3.3";

    char *token = NULL;
    char *date_separator = "/";
    char *vertex_separator = " ";

    int day, month, year;
    float x, y, z;

    token  = strtok(date_str, date_separator);
    printf("token: %s\n", token);
    day = atoi(token);
    printf("Day: %d\n", day);

    token = strtok(NULL, date_separator);
    month = atoi(token);
    printf("token: %s\n", token);
    printf("Month: %d\n", month);

    token = strtok(NULL, date_separator);
    year = atoi(token);
    printf("token: %s\n", token);
    printf("Year: %d\n", year);

    token = strtok(vertex_entry, vertex_separator);
    token = strtok(NULL, vertex_separator);    
    x = atof(token);
    token = strtok(NULL, vertex_separator);    
    y = atof(token);
    token = strtok(NULL, vertex_separator);    
    z = atof(token);
    printf("Vertex: (%.2f, %.2f, %.2f)\n", x, y, z);
    return 0;
}