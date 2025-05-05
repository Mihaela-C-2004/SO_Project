#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>

#define TMP_REQUEST "/tmp/monitor_request"
#define TMP_RESPONSE "/tmp/monitor_response"

pid_t monitor_pid = -1;
int monitor_running = 0;

void process_request() {
    FILE *fp = fopen(TMP_REQUEST, "r");
    if (!fp) {
        perror("Monitor failed to open request file");
        return;
    }

    char command[128];
    if (!fgets(command, sizeof(command), fp)) {
        fclose(fp);
        return;
    }
    command[strcspn(command, "\n")] = 0;
    fclose(fp);

    FILE *resp = fopen(TMP_RESPONSE, "w");
    if (!resp) {
        perror("Monitor failed to open response file");
        return;
    }

    if (strcmp(command, "list_hunts") == 0) {
        DIR *dir = opendir(".");
        if (!dir) {
            fprintf(resp, "Couldn't open current directory\n");
            fclose(resp);
            return;
        }

        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_name[0] == '.')
                continue;

            struct stat st;
            if (stat(entry->d_name, &st) != 0 || !S_ISDIR(st.st_mode))
                continue;

	    char treasure_file[256];
	    strcpy(treasure_file, entry->d_name);
	    strcat(treasure_file, "/treasures.bin");

            FILE *fp = fopen(treasure_file, "rb");
            if (!fp)
                continue;

            int count = 0;
            struct {
                char treasure_id[30];
                char username[50];
                float latitude;
                float longitude;
                char clue[100];
                int value;
            } t;

            while (fread(&t, sizeof(t), 1, fp) == 1)
                count++;

            fclose(fp);
            fprintf(resp, "Hunt: %s | Total treasures: %d\n", entry->d_name, count);
        }

        closedir(dir);

    } else if (strncmp(command, "list_treasures ", 15) == 0) {
        char *hunt_id = command + 15;
        char filepath[100];
        snprintf(filepath, sizeof(filepath), "./%s/treasures.bin", hunt_id);

        FILE *fp = fopen(filepath, "rb");
        if (!fp) {
            fprintf(resp, "Error: Could not open file for hunt '%s'\n", hunt_id);
            fclose(resp);
            return;
        }

        struct {
            char treasure_id[30];
            char username[50];
            float latitude;
            float longitude;
            char clue[100];
            int value;
        } t;

        while (fread(&t, sizeof(t), 1, fp) == 1) {
            fprintf(resp, "ID: %s | User: %s | Location: (%.2f, %.2f)\nClue: %s | Value: %d\n\n",
                    t.treasure_id, t.username, t.latitude, t.longitude, t.clue, t.value);
        }

        fclose(fp);

    } else if (strncmp(command, "view_treasure ", 14) == 0) {
        char *args = command + 14;
        char *hunt_id = strtok(args, " ");
        char *treasure_id = strtok(NULL, " ");

        if (!hunt_id || !treasure_id) {
            fprintf(resp, "Invalid view_treasure command\n");
            fclose(resp);
            return;
        }

        char filepath[100];
        snprintf(filepath, sizeof(filepath), "./%s/treasures.bin", hunt_id);

        FILE *fp = fopen(filepath, "rb");
        if (!fp) {
            fprintf(resp, "Error: Could not open file for hunt '%s'\n", hunt_id);
            fclose(resp);
            return;
        }

        struct {
            char treasure_id[30];
            char username[50];
            float latitude;
            float longitude;
            char clue[100];
            int value;
        } t;

        int found = 0;
        while (fread(&t, sizeof(t), 1, fp) == 1) {
            if (strcmp(t.treasure_id, treasure_id) == 0) {
                fprintf(resp, "Treasure Found:\n");
                fprintf(resp, "ID: %s\nUser: %s\nLatitude: %.2f\nLongitude: %.2f\nClue: %s\nValue: %d\n",
                        t.treasure_id, t.username, t.latitude, t.longitude, t.clue, t.value);
                found = 1;
                break;
            }
        }

        if (!found)
            fprintf(resp, "Treasure with ID %s not found in hunt %s.\n", treasure_id, hunt_id);

        fclose(fp);

    } else if (strcmp(command, "stop_monitor") == 0) {
        fprintf(resp, "Monitor stops\n");
        fclose(resp);
        remove(TMP_REQUEST);
        exit(0);
    } else {
        fprintf(resp, "Unknown command: %s\n", command);
    }

    fclose(resp);
}

void sigusr1_handler(int sig) {
    process_request();
}

//interfata

void send_request(const char *message) {
    FILE *fp = fopen(TMP_REQUEST, "w");
    if (!fp) {
        perror("Error writing request");
        return;
    }
    fprintf(fp, "%s\n", message);
    fclose(fp);

    kill(monitor_pid, SIGUSR1);
}

void wait_for_response() {
    while (access(TMP_RESPONSE, F_OK) != 0) {
        usleep(100000); // 100ms
    }

    FILE *fp = fopen(TMP_RESPONSE, "r");
    if (!fp) {
        perror("Error reading response");
        return;
    }

    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        printf("%s", line);
    }
    fclose(fp);
    remove(TMP_RESPONSE);
}

void handle_monitor_exit(int sig) {
    monitor_running = 0;
    printf("Monitor has exited.\n");
}

int main(int argc, char *argv[]) {
    if (getppid() != 1 && argc == 2 && strcmp(argv[1], "--monitor") == 0) {
        signal(SIGUSR1, sigusr1_handler);
        while (1) pause();
    }

    struct sigaction sa;
    sa.sa_handler = handle_monitor_exit;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGCHLD, &sa, NULL);

    char command[128];

    while (1) {
        printf("> ");
        fflush(stdout);

        if (!fgets(command, sizeof(command), stdin)) break;
        command[strcspn(command, "\n")] = 0;

        if (strcmp(command, "start_monitor") == 0) {
            if (monitor_running) {
                printf("Monitor already running\n");
                continue;
            }

            monitor_pid = fork();
            if (monitor_pid == 0) {
                execl("./treasure_hub", "./treasure_hub", "--monitor", NULL);
                perror("Failed to start monitor");
                exit(1);
            } else if (monitor_pid > 0) {
                monitor_running = 1;
                printf("Monitor started with PID %d.\n", monitor_pid);
            } else {
                perror("Fork failed");
            }

        } else if (strncmp(command, "list_hunts", 10) == 0 ||
                   strncmp(command, "list_treasures", 14) == 0 ||
                   strncmp(command, "view_treasure", 13) == 0) {

            if (!monitor_running) {
                printf("Monitor is not running. Start it first\n");
                continue;
            }

            send_request(command);
            wait_for_response();

        } else if (strcmp(command, "stop_monitor") == 0) {
            if (!monitor_running) {
                printf("Monitor is not running\n");
                continue;
            }

            send_request("stop_monitor");
            printf("Sent stop request to monitor. Waiting to terminate\n");

        } else if (strcmp(command, "exit") == 0) {
            if (monitor_running) {
                printf("Error: Can't exit while monitor is still running\n");
            } else {
                break;
            }

        } else {
            printf("Unknown/invalid command\n");
        }
    }

    return 0;
}
