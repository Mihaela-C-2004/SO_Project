#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#define FILE_PATH "./hunt001/treasures.bin"
#define DIR_PATH "./hunt001"

typedef struct {
    int treasure_id;
    char username[50];
    float latitude;
    float longitude;
    char clue[100];
    int value;
} Treasure;

void directory_exists() {
  mkdir(DIR_PATH, 0777); //toate drepturile
}

void add_treasure(Treasure treasure) {
    directory_exists();

    int fd = open(FILE_PATH, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd < 0) {
        perror("Error opening treasure file");
        return;
    }

    write(fd, &treasure, sizeof(Treasure));
    close(fd);
    printf("Treasure with ID %d added successfully.\n", treasure.treasure_id);
}

void list_treasures() {
    int fd = open(FILE_PATH, O_RDONLY);
    if (fd < 0) {
        perror("Error opening treasure file");
        return;
    }

    Treasure t;
    printf("Treasure List:\n");
    
    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
        printf("ID: %d, User: %s, Location: (%f, %f), Clue: %s, Value: %d\n",
               t.treasure_id, t.username, t.latitude, t.longitude, t.clue, t.value);
    }

    close(fd);
}

void view_treasure(int id) {
    int fd = open(FILE_PATH, O_RDONLY);
    if (fd < 0) {
        perror("Error opening treasure file");
        return;
    }

    Treasure t;
    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
        if (t.treasure_id == id) {
            printf("Treasure Found:\n");
            printf("ID: %d\nUser: %s\nLatitude: %f\nLongitude: %f\nClue: %s\nValue: %d\n",
                   t.treasure_id, t.username, t.latitude, t.longitude, t.clue, t.value);
            close(fd);
            return;
        }
    }

    printf("Treasure with ID %d not found.\n", id);
    close(fd);
}

void remove_treasure(int id) {
    FILE *fp = fopen(FILE_PATH, "rb");
    FILE *temp = fopen("./hunt001/temp.bin", "wb");

    if (!fp || !temp) {
        perror("Error opening files");
        return;
    }

    Treasure t;
    int found = 0;

    while (fread(&t, sizeof(Treasure), 1, fp)) {
        if (t.treasure_id != id) {
            fwrite(&t, sizeof(Treasure), 1, temp);
        } else {
            found = 1;
        }
    }

    fclose(fp);
    fclose(temp);

    if (found) {
        remove(FILE_PATH);
        rename("./hunt001/temp.bin", FILE_PATH);
        printf("Treasure with ID %d removed.\n", id);
    } else {
        remove("./hunt001/temp.bin");
        printf("Treasure with ID %d not found.\n", id);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        return 1;
    }

    if (strcmp(argv[1], "add") == 0) {
        Treasure t;
        printf("Enter treasure ID (int): ");
        scanf("%d", &t.treasure_id);
        printf("Enter username: ");
        scanf("%s", t.username);
        printf("Enter latitude: ");
        scanf("%f", &t.latitude);
        printf("Enter longitude: ");
        scanf("%f", &t.longitude);
        printf("Enter clue: ");
        getchar(); 
        fgets(t.clue, sizeof(t.clue), stdin);
        printf("Enter value: ");
        scanf("%d", &t.value);

        add_treasure(t);

    } else if (strcmp(argv[1], "list") == 0) {
        list_treasures();

    } else if (strcmp(argv[1], "view") == 0 && argc >= 3) {
        int id = atoi(argv[2]);
        view_treasure(id);

    } else if (strcmp(argv[1], "remove") == 0 && argc >= 3) {
        int id = atoi(argv[2]);
        remove_treasure(id);

    } else {
        printf("Invalid command or missing arguments.\n");
    }

    return 0;
}
