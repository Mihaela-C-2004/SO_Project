#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

typedef struct {
    char treasure_id[30];
    char username[50];
    float latitude;
    float longitude;
    char clue[100];
    int value;
} Treasure;

void log_operation(const char *hunt_id, const char *dir_path, const char *message) {
    char log_path[150];
    FILE *log_file;

    strcpy(log_path, dir_path);
    strcat(log_path, "/logged_hunt");

    log_file = fopen(log_path, "a");
    if (!log_file) return;

    fprintf(log_file, "%s\n", message);
    fclose(log_file);

    char link_name[100];
    strcpy(link_name, "logged_hunt-");
    strcat(link_name, hunt_id);

    if (access(link_name, F_OK) == -1) {
        symlink(log_path, link_name);
    }
}

void build_paths(const char *dir_name, char *dir_path, char *file_path) {
    strcpy(dir_path, "./");
    strcat(dir_path, dir_name);

    strcpy(file_path, dir_path);
    strcat(file_path, "/treasures.bin");
}

int treasure_id_exists(const char *file_path, const char *id) {
    int fd = open(file_path, O_RDONLY);
    if (fd < 0)
      return 0;

    Treasure t;
    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
        if (strcmp(t.treasure_id, id) == 0) {
            close(fd);
            return 1;
        }
    }

    close(fd);
    return 0;
}

void add_treasure(const char *dir_path, const char *file_path, Treasure treasure) {
    mkdir(dir_path, 0755);

    int fd = open(file_path, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd < 0) {
        perror("Error opening treasure file");
        return;
    }

    write(fd, &treasure, sizeof(Treasure));
    close(fd);
    printf("Treasure with ID %s added successfully in %s.\n", treasure.treasure_id, dir_path);
}

void list_treasures(const char *file_path) {
    int fd = open(file_path, O_RDONLY);
    if (fd < 0) {
        perror("Error opening treasure file");
        return;
    }

    Treasure t;
    printf("Treasure List:\n");
    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
        printf("ID: %s | User: %s | Location: (%.2f, %.2f)\nClue: %s | Value: %d\n\n",
               t.treasure_id, t.username, t.latitude, t.longitude, t.clue, t.value);
    }

    close(fd);
}

void view_treasure(const char *file_path, const char *id) {
    int fd = open(file_path, O_RDONLY);
    if (fd < 0) {
        perror("Error opening treasure file");
        return;
    }

    Treasure t;
    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
        if (strcmp(t.treasure_id, id) == 0) {
            printf("Treasure Found:\n");
            printf("ID: %s\nUser: %s\nLatitude: %.2f\nLongitude: %.2f\nClue: %s\nValue: %d\n",
                   t.treasure_id, t.username, t.latitude, t.longitude, t.clue, t.value);
            close(fd);
            return;
        }
    }

    printf("Treasure with ID %s not found.\n", id);
    close(fd);
}

void remove_treasure(const char *dir_path, const char *file_path, const char *id) {
    char temp_path[200];
    strcpy(temp_path, dir_path);
    strcat(temp_path, "/temp.bin");

    FILE *fp = fopen(file_path, "rb");
    FILE *temp = fopen(temp_path, "wb");

    if (!fp || !temp) {
        perror("Error opening files");
        return;
    }

    Treasure t;
    int found = 0;

    while (fread(&t, sizeof(Treasure), 1, fp)) {
        if (strcmp(t.treasure_id, id) != 0) {
            fwrite(&t, sizeof(Treasure), 1, temp);
        } else {
            found = 1;
        }
    }

    fclose(fp);
    fclose(temp);

    if (found) {
        remove(file_path);
        rename(temp_path, file_path);
        printf("Treasure with ID %s removed from %s.\n", id, dir_path);
    } else {
        remove(temp_path);
        printf("Treasure with ID %s not found.\n", id);
    }
}

void remove_hunt(const char *dir_path) {
    char file_path[150];

    strcpy(file_path, dir_path);
    strcat(file_path, "/treasures.bin");
    remove(file_path);

    strcpy(file_path, dir_path);
    strcat(file_path, "/temp.bin");
    remove(file_path);

    strcpy(file_path, dir_path);
    strcat(file_path, "/logged_hunt");
    remove(file_path);

    if (rmdir(dir_path) == 0) {
        printf("Hunt directory '%s' removed successfully.\n", dir_path);
    } else 
        perror("Error removing hunt directory");

    char link_name[100];
    strcpy(link_name, "logged_hunt-");
    strcat(link_name, dir_path + 2);
    remove(link_name);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: %s [add <hunt_id> | list <hunt_id> | view <hunt_id> <id> | remove_treasure <hunt_id> <id> | remove_hunt <hunt_id>]\n", argv[0]);
        return 1;
    }

    char dir_path[100];
    char file_path[120];
    build_paths(argv[2], dir_path, file_path);

    if (strcmp(argv[1], "add") == 0) {
        Treasure t;
        printf("Enter treasure ID: ");
        scanf("%s", t.treasure_id);

        if (treasure_id_exists(file_path, t.treasure_id)) {
            printf("Error: A treasure with ID %s already exists in %s.\n", t.treasure_id, argv[2]);
            return 1;
        }

        printf("Enter username: ");
        scanf("%s", t.username);
        printf("Enter latitude: ");
        scanf("%f", &t.latitude);
        printf("Enter longitude: ");
        scanf("%f", &t.longitude);
        printf("Enter clue: ");
        getchar();
        fgets(t.clue, sizeof(t.clue), stdin);
        char *newline = strchr(t.clue, '\n');
        if (newline)
	  *newline = '\0';
        printf("Enter value: ");
        scanf("%d", &t.value);

        add_treasure(dir_path, file_path, t);

        char msg[100];
        strcpy(msg, "Added treasure ID ");
        strcat(msg, t.treasure_id);
        log_operation(argv[2], dir_path, msg);

    } else if (strcmp(argv[1], "list") == 0) {
        list_treasures(file_path);
        log_operation(argv[2], dir_path, "Listed treasures");

    } else if (strcmp(argv[1], "view") == 0 && argc >= 4) {
        view_treasure(file_path, argv[3]);

        char msg[100];
        strcpy(msg, "Viewed treasure ID ");
        strcat(msg, argv[3]);
        log_operation(argv[2], dir_path, msg);

    } else if (strcmp(argv[1], "remove_treasure") == 0 && argc >= 4) {
        remove_treasure(dir_path, file_path, argv[3]);

        char msg[100];
        strcpy(msg, "Removed treasure ID ");
        strcat(msg, argv[3]);
        log_operation(argv[2], dir_path, msg);

    } else if (strcmp(argv[1], "remove_hunt") == 0) {
        remove_hunt(dir_path);

        char msg[100];
        strcpy(msg, "Removed hunt ");
        strcat(msg, argv[2]);
        log_operation(argv[2], dir_path, msg);

    } else
        printf("Invalid command or missing arguments.\n");

    return 0;
}
