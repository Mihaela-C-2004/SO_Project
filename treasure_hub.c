/*
//  update fara duplicate de cod 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>

#define TMP_REQUEST "/tmp/monitor_request"
#define TMP_RESPONSE "/tmp/monitor_response"

pid_t monitor_pid = -1;
int monitor_running = 0;

//monitor
void process_request() {
    FILE *fp = fopen(TMP_REQUEST, "r");
    if (!fp) return;

    char command[256];
    if (!fgets(command, sizeof(command), fp)) {
        fclose(fp);
        return;
    }
    fclose(fp);

    char *newline = strchr(command, '\n');
    if (newline) *newline = '\0';

    FILE *resp = fopen(TMP_RESPONSE, "w");
    if (!resp) {
        perror("Could not open response file");
        return;
    }
    fclose(resp);

    pid_t pid = fork();
    if (pid == 0) {
        // Proces copil
        freopen(TMP_RESPONSE, "w", stdout);

        if (strcmp(command, "list_hunts") == 0) {
            execl("./treasure_manager", "treasure_manager", "list_hunts", "space", NULL);
        } else if (strncmp(command, "list_treasures ", 15) == 0) {
            char *hunt_id = command + 15;
            execl("./treasure_manager", "treasure_manager", "list", hunt_id, NULL);
        } else if (strncmp(command, "view_treasure ", 14) == 0) {
            char *args = command + 14;
            char *hunt_id = strtok(args, " ");
            char *treasure_id = strtok(NULL, " ");
            if (hunt_id && treasure_id) {
                execl("./treasure_manager", "treasure_manager", "view", hunt_id, treasure_id, NULL);
            } else {
                execlp("echo", "echo", "Invalid arguments for view_treasure", NULL);
            }
        } else if (strcmp(command, "stop_monitor") == 0) {
            execlp("echo", "echo", "Monitor stops", NULL);
        } else {
            execlp("echo", "echo", "Unknown command", NULL);
        }

        perror("exec failed");
        exit(1);
    } else if (pid > 0) {
        wait(NULL);
        if (strcmp(command, "stop_monitor") == 0) {
            remove(TMP_REQUEST);
            exit(0);
        }
    }
}

void sigusr1_handler(int sig) {
    process_request();
}

//hub
void send_request(const char *message) {
    FILE *fp = fopen(TMP_REQUEST, "w");
    if (!fp) {
        perror("Failed to write to request file");
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
        perror("Failed to read response");
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
    // Dacă e apelat ca monitor
    if (argc == 2 && strcmp(argv[1], "--monitor") == 0) {
        struct sigaction sa;
        sa.sa_handler = sigusr1_handler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = SA_RESTART;
        sigaction(SIGUSR1, &sa, NULL);

        while (1) pause();
    }

    struct sigaction sa;
    sa.sa_handler = handle_monitor_exit;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGCHLD, &sa, NULL);

    char command[256];

    while (1) {
        printf("> ");
        fflush(stdout);

        if (!fgets(command, sizeof(command), stdin))
            break;

        char *newline = strchr(command, '\n');
        if (newline) *newline = '\0';

        if (strcmp(command, "start_monitor") == 0) {
            if (monitor_running) {
                printf("Monitor already running.\n");
                continue;
            }

            monitor_pid = fork();
            if (monitor_pid == 0) {
                execl("./treasure_hub", "./treasure_hub", "--monitor", NULL);
                perror("Failed to start monitor");
                exit(1);
            } else if (monitor_pid > 0) {
                monitor_running = 1;
                printf("Monitor started with PID %d\n", monitor_pid);
            } else {
                perror("Fork failed");
            }

        } else if (strncmp(command, "list_hunts", 10) == 0 ||
                   strncmp(command, "list_treasures", 14) == 0 ||
                   strncmp(command, "view_treasure", 13) == 0) {

            if (!monitor_running) {
                printf("Monitor is not running. Start it first.\n");
                continue;
            }

            send_request(command);
            wait_for_response();

        } else if (strcmp(command, "stop_monitor") == 0) {
            if (!monitor_running) {
                printf("Monitor is not running.\n");
                continue;
            }

            send_request("stop_monitor");
            printf("Sent stop request. Waiting for monitor to terminate...\n");

        } else if (strcmp(command, "exit") == 0) {
            if (monitor_running) {
                printf("Error: Monitor is still running. Stop it first.\n");
            } else {
                break;
            }

        } else {
            printf("Unknown or invalid command.\n");
        }
    }

    return 0;
}

*/
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

#define MAX_CMD 128

int to_monitor[2], from_monitor[2];
pid_t monitor_pid = -1;
int monitor_running = 0;



void start_monitor() {
    pipe(to_monitor);
    pipe(from_monitor);

    monitor_pid = fork();
    if (monitor_pid == 0) {
        dup2(to_monitor[0], STDIN_FILENO);
        dup2(from_monitor[1], STDOUT_FILENO);
        close(to_monitor[1]);
        close(from_monitor[0]);
        execl("./monitor_hub", "./monitor_hub", "--monitor", NULL);
        perror("exec failed");
        exit(1);
    }

    close(to_monitor[0]);
    close(from_monitor[1]);
    monitor_running = 1;
    printf("Monitor started with PID %d.\n", monitor_pid);
}

void send_command(const char *cmd) {
    write(to_monitor[1], cmd, strlen(cmd));
    write(to_monitor[1], "\n", 1);
}

void read_response() {
    char buf[256];
    int n;
    while ((n = read(from_monitor[0], buf, sizeof(buf) - 1)) > 0) {
        buf[n] = '\0';
        printf("%s", buf);
        if (strstr(buf, "Monitor stops")) break;
    }
}

void calculate_score() {
    DIR *dir = opendir(".");
    if (!dir) {
        perror("Could not open directory");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;

        struct stat st;
        if (stat(entry->d_name, &st) != 0 || !S_ISDIR(st.st_mode)) continue;

        pid_t pid = fork();
        if (pid == 0) {
            int pipefd[2];
            pipe(pipefd);
            pid_t calc_pid = fork();
            if (calc_pid == 0) {
                dup2(pipefd[1], STDOUT_FILENO);
                close(pipefd[0]);
                execl("./score_calculator", "./score_calculator", entry->d_name, NULL);
                perror("exec score_calculator");
                exit(1);
            } else {
                close(pipefd[1]);
                char buffer[256];
                int n;
                printf("\n--- Scores for hunt %s ---\n", entry->d_name);
                while ((n = read(pipefd[0], buffer, sizeof(buffer)-1)) > 0) {
                    buffer[n] = '\0';
                    printf("%s", buffer);
                }
                close(pipefd[0]);
                wait(NULL);
                exit(0);
            }
        }
        wait(NULL);
    }

    closedir(dir);
}

int main(int argc, char *argv[]) {
    if (argc == 2 && strcmp(argv[1], "--monitor") == 0) {
        char command[MAX_CMD];
        while (fgets(command, sizeof(command), stdin)) {
            command[strcspn(command, "\n")] = 0;

            if (strcmp(command, "stop_monitor") == 0) {
                printf("Monitor stops\n");
                fflush(stdout);
                break;
            }

            char *args[5] = {0};
            int i = 0;
            char *token = strtok(command, " ");
            while (token && i < 4) {
                args[i++] = token;
                token = strtok(NULL, " ");
            }

            pid_t pid = fork();
            if (pid == 0) {
                if (strcmp(args[0], "list_hunts") == 0) {
                    execl("./treasure_manager", "treasure_manager", "list_hunts", NULL);
                } else if (strcmp(args[0], "list_treasures") == 0 && args[1]) {
                    execl("./treasure_manager", "treasure_manager", "list", args[1], NULL);
                } else if (strcmp(args[0], "view_treasure") == 0 && args[1] && args[2]) {
                    execl("./treasure_manager", "treasure_manager", "view", args[1], args[2], NULL);
                } else {
                    printf("Unknown or invalid command\n");
                    exit(1);
                }
                perror("exec treasure_manager");
                exit(1);
            } else {
                waitpid(pid, NULL, 0);
            }

            fflush(stdout);
        }

        return 0;
    }

 
    char input[MAX_CMD];
    while (1) {
        printf("> ");
        fflush(stdout);
        if (!fgets(input, sizeof(input), stdin)) break;
        input[strcspn(input, "\n")] = 0;

        if (strcmp(input, "start_monitor") == 0) {
            if (!monitor_running)
                start_monitor();
            else
                printf("Monitor already running.\n");

        } else if (strncmp(input, "list_hunts", 10) == 0 ||
                   strncmp(input, "list_treasures", 14) == 0 ||
                   strncmp(input, "view_treasure", 13) == 0 ||
                   strcmp(input, "stop_monitor") == 0) {

            if (!monitor_running) {
                printf("Monitor not running.\n");
                continue;
            }

            send_command(input);
            read_response();

            if (strcmp(input, "stop_monitor") == 0) {
                waitpid(monitor_pid, NULL, 0);
                monitor_running = 0;
                printf("Monitor stopped.\n");
            }

        } else if (strcmp(input, "calculate_score") == 0) {
            calculate_score();

        } else if (strcmp(input, "exit") == 0) {
            if (monitor_running) {
                printf("Monitor is still running. Please stop it first.\n");
            } else {
                break;
            }

        } else {
            printf("Unknown command.\n");
        }
    }

    return 0;
}

