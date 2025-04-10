#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <spawn.h>
#include <errno.h>

#define DEFAULT_SYSFS_PATH "/sys/kernel/night_monitor/message"
#define BUF_LEN 256

extern char **environ;

void send_notification(const char *message) {
    pid_t pid;
    char *argv[] = {"notify-send", "Ядро", (char *)message, NULL};
    
    int ret = posix_spawnp(&pid, "notify-send", NULL, NULL, argv, environ);
    
    if (ret != 0) {
        fprintf(stderr, "posix_spawnp failed: %s\n", strerror(ret));
        return;
    }
    int status;
    waitpid(pid, &status, 0);
}

int main(int argc, char *argv[]) {
    const char *sysfs_path = DEFAULT_SYSFS_PATH;
    int interval = 10; 
    
    if (argc > 1) sysfs_path = argv[1];
    if (argc > 2) interval = atoi(argv[2]);

    char last_message[BUF_LEN] = {0};
    
    while (1) {
        FILE *f = fopen(sysfs_path, "r");
        if (!f) {
            perror("fopen failed");
            sleep(5);
            continue;
        }

        char current_message[BUF_LEN] = {0};
        if (fgets(current_message, BUF_LEN, f)) {
            current_message[strcspn(current_message, "\n")] = '\0';
            
            if (strlen(current_message) > 0 && 
                strcmp(last_message, current_message) != 0) {
                strncpy(last_message, current_message, BUF_LEN-1);
                
                printf("New alert: %s\n", current_message);
                send_notification(current_message);
            }
        }
        
        fclose(f);
        sleep(interval);
    }
    
    return 0;
}