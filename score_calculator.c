#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char treasure_id[30];
    char username[50];
    float latitude;
    float longitude;
    char clue[100];
    int value;
} Treasure;

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <hunt_id>\n", argv[0]);
        return 1;
    }

    char path[128];
    snprintf(path, sizeof(path), "./%s/treasures.bin", argv[1]);

    FILE *fp = fopen(path, "rb");
    if (!fp) {
        perror("Error opening file");
        return 1;
    }

    Treasure t;
    struct {
        char username[50];
        int score;
    } users[100];
    int count = 0;

    while (fread(&t, sizeof(t), 1, fp) == 1) {
        int found = 0;
        for (int i = 0; i < count; i++) {
            if (strcmp(users[i].username, t.username) == 0) {
                users[i].score = users[i].score + t.value;
                found = 1;
                break;
            }
        }
        if (!found) {
            strcpy(users[count].username, t.username);
            users[count].score = t.value;
            count++;
        }
    }
    fclose(fp);

    for (int i = 0; i < count; i++) {
        printf("User: %s | Score: %d\n", users[i].username, users[i].score);
    }

    return 0;
}
