#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdbool.h>
#include <ctype.h>

#define MAX_KERNELS 100
#define PATH_MAX 4096
#define BUFFER_SIZE 1024

typedef struct {
    char version[256];
    char path[PATH_MAX];
    bool running;
} KernelInfo;

KernelInfo kernels[MAX_KERNELS];
int kernel_count = 0;
char running_kernel[256] = {0};

void print_help() {
    printf("kcleaner - A tool for managing Linux kernel installations\n\n");
    printf("Usage: kcleaner [OPTION]\n");
    printf("Options:\n");
    printf("  -l, --list      List all installed kernels\n");
    printf("  -d, --delete    Delete kernels by their numbers (e.g., -d 1,3,5-7)\n");
    printf("  -a, --auto      Auto-clean: removes all kernels except running and latest\n");
    printf("  -h, --help      Display this help message\n");
    printf("\nExamples:\n");
    printf("  kcleaner -l\n");
    printf("  kcleaner -d 2,4,7\n");
    printf("  kcleaner -d 1-3,5,8-10\n");
    printf("  kcleaner -a\n");
    printf("\nNote: Root privileges required for kernel deletion\n");
}

bool check_root_privileges() {
    if (geteuid() != 0) {
        printf("Error: Root privileges required for kernel deletion.\n");
        printf("Please run with sudo: sudo kcleaner -d ...\n");
        return false;
    }
    return true;
}

bool check_command_exists(const char *cmd) {
    char which_cmd[PATH_MAX];
    snprintf(which_cmd, sizeof(which_cmd), "which %s >/dev/null 2>&1", cmd);
    return (system(which_cmd) == 0);
}

void update_grub() {
    if (check_command_exists("update-grub")) {
        printf("Updating GRUB bootloader configuration...\n");
        int result = system("update-grub");
        if (result == 0) {
            printf("GRUB configuration updated successfully.\n");
        } else {
            printf("Warning: Failed to update GRUB configuration (exit code: %d).\n", result);
        }
    } else {
        printf("Note: GRUB update-grub command not found, skipping bootloader update.\n");
    }
}

void get_running_kernel() {
    FILE *fp = fopen("/proc/version", "r");
    if (fp == NULL) {
        perror("Failed to open /proc/version");
        return;
    }

    char buffer[BUFFER_SIZE];
    if (fgets(buffer, BUFFER_SIZE, fp) != NULL) {
        char *version_start = strstr(buffer, "Linux version ");
        if (version_start != NULL) {
            version_start += strlen("Linux version ");
            char *version_end = strchr(version_start, ' ');
            if (version_end != NULL) {
                size_t len = version_end - version_start;
                strncpy(running_kernel, version_start, len);
                running_kernel[len] = '\0';
            }
        }
    }
    fclose(fp);
}

int compare_kernel_versions(const void *a, const void *b) {
    KernelInfo *ka = (KernelInfo *)a;
    KernelInfo *kb = (KernelInfo *)b;
    
    return strcmp(kb->version, ka->version);
}

void find_kernels() {
    DIR *dir = opendir("/boot");
    if (dir == NULL) {
        perror("Failed to open /boot directory");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL && kernel_count < MAX_KERNELS) {
        if (strncmp(entry->d_name, "vmlinuz-", 8) == 0) {
            strcpy(kernels[kernel_count].version, entry->d_name + 8);
            snprintf(kernels[kernel_count].path, PATH_MAX, "/boot/%s", entry->d_name);
            
            kernels[kernel_count].running = (strcmp(kernels[kernel_count].version, running_kernel) == 0);
            
            kernel_count++;
        }
    }
    closedir(dir);
    
    qsort(kernels, kernel_count, sizeof(KernelInfo), compare_kernel_versions);
}

void list_kernels() {
    if (kernel_count == 0) {
        printf("No kernels found in /boot\n");
        return;
    }

    printf("Found %d kernel(s):\n\n", kernel_count);
    printf("  # | Version                       | Size      | Running\n");
    printf("----+-------------------------------+-----------+---------\n");

    for (int i = 0; i < kernel_count; i++) {
        char size_cmd[PATH_MAX + 50];
        snprintf(size_cmd, sizeof(size_cmd), "du -sh /boot/*%s* /lib/modules/%s 2>/dev/null | awk '{sum += $1} END {print sum}'", 
                 kernels[i].version, kernels[i].version);
        
        FILE *fp = popen(size_cmd, "r");
        char size_str[64] = "Unknown";
        if (fp != NULL) {
            if (fgets(size_str, sizeof(size_str), fp) != NULL) {
                size_str[strcspn(size_str, "\n")] = 0;
            }
            pclose(fp);
        }
        
        printf("%3d | %-29s | %-9s | %s\n", 
               i + 1, 
               kernels[i].version, 
               size_str,
               kernels[i].running ? "Yes" : "No");
    }
    printf("\n");
}

bool is_digit_or_separator(char c) {
    return isdigit(c) || c == ',' || c == '-';
}

bool confirm_deletion(const char *message) {
    printf("%s (y/N): ", message);
    fflush(stdout);
    
    char response[10];
    if (fgets(response, sizeof(response), stdin) == NULL) {
        printf("\nError reading input.\n");
        return false;
    }
    
    return (response[0] == 'y' || response[0] == 'Y');
}

void delete_kernel(int index) {
    if (index < 0 || index >= kernel_count) {
        printf("Invalid kernel index: %d\n", index);
        return;
    }

    if (kernels[index].running) {
        if (!confirm_deletion("WARNING: You are about to delete your RUNNING kernel! This may cause your system to be unbootable. Are you ABSOLUTELY sure?")) {
            printf("Skipping deletion of running kernel %s (index %d)\n", kernels[index].version, index + 1);
            return;
        }
    }

    printf("Deleting kernel %s (index %d)...\n", kernels[index].version, index + 1);

    char cmd[PATH_MAX * 2];
    snprintf(cmd, sizeof(cmd), 
             "rm -vf /boot/*%s* && "
             "rm -vrf /lib/modules/%s",
             kernels[index].version, kernels[index].version);

    int result = system(cmd);
    if (result == 0) {
        printf("Successfully deleted kernel %s\n", kernels[index].version);
    } else {
        printf("Error deleting kernel %s (exit code: %d)\n", 
               kernels[index].version, result);
    }
}

void delete_kernels(const char *selection) {
    if (!check_root_privileges()) {
        return;
    }

    char *sel_copy = strdup(selection);
    if (!sel_copy) {
        perror("Memory allocation failed");
        return;
    }

    for (char *c = sel_copy; *c; c++) {
        if (!is_digit_or_separator(*c)) {
            printf("Invalid character in selection: '%c'\n", *c);
            free(sel_copy);
            return;
        }
    }

    bool to_delete[MAX_KERNELS] = {false};
    int delete_count = 0;
    bool deleting_running = false;
    int remaining_count = kernel_count;

    char *token = strtok(sel_copy, ",");
    while (token != NULL) {
        char *range_sep = strchr(token, '-');
        if (range_sep) {
            *range_sep = '\0';
            int start = atoi(token);
            int end = atoi(range_sep + 1);
            
            if (start < 1 || end > kernel_count || start > end) {
                printf("Invalid range: %s-%s\n", token, range_sep + 1);
                free(sel_copy);
                return;
            }
            
            for (int i = start; i <= end; i++) {
                if (!to_delete[i-1]) {
                    to_delete[i-1] = true;
                    delete_count++;
                    remaining_count--;
                    
                    if (kernels[i-1].running) {
                        deleting_running = true;
                    }
                }
            }
        } else {
            int num = atoi(token);
            if (num < 1 || num > kernel_count) {
                printf("Invalid kernel number: %s\n", token);
                free(sel_copy);
                return;
            }
            
            if (!to_delete[num-1]) {
                to_delete[num-1] = true;
                delete_count++;
                remaining_count--;
                
                if (kernels[num-1].running) {
                    deleting_running = true;
                }
            }
        }
        token = strtok(NULL, ",");
    }
    free(sel_copy);

    if (delete_count == 0) {
        printf("No kernels selected for deletion.\n");
        return;
    }

    printf("You are about to delete the following kernels:\n\n");
    for (int i = 0; i < kernel_count; i++) {
        if (to_delete[i]) {
            printf("  %d. %s %s\n", i + 1, kernels[i].version, kernels[i].running ? "(RUNNING)" : "");
        }
    }
    
    if (deleting_running) {
        printf("\nWARNING: This includes your RUNNING kernel! System may become unbootable!\n");
    }
    
    if (remaining_count == 0) {
        printf("\nCRITICAL WARNING: This will delete ALL kernels! Your system will NOT BOOT!\n");
    }
    
    if (!confirm_deletion("\nProceed with deletion?")) {
        printf("Deletion cancelled.\n");
        return;
    }
    
    for (int i = 0; i < kernel_count; i++) {
        if (to_delete[i]) {
            delete_kernel(i);
        }
    }
    
    printf("\nDeletion completed.\n");
    
    if (delete_count > 0) {
        update_grub();
    }
}

void auto_clean() {
    if (!check_root_privileges()) {
        return;
    }

    if (kernel_count <= 2) {
        printf("Not enough kernels to clean. You have %d kernel(s) installed.\n", kernel_count);
        printf("Auto-clean keeps at least the running kernel and the latest kernel.\n");
        return;
    }

    bool to_delete[MAX_KERNELS] = {false};
    int delete_count = 0;
    int latest_index = 0;
    int running_index = -1;

    for (int i = 0; i < kernel_count; i++) {
        if (kernels[i].running) {
            running_index = i;
            break;
        }
    }

    for (int i = 0; i < kernel_count; i++) {
        if (i != 0 && i != running_index) {
            to_delete[i] = true;
            delete_count++;
        }
    }

    if (delete_count == 0) {
        printf("No kernels to clean. You only have the latest and running kernel installed.\n");
        return;
    }

    printf("Auto-clean will delete the following kernels:\n\n");
    for (int i = 0; i < kernel_count; i++) {
        if (to_delete[i]) {
            printf("  %d. %s\n", i + 1, kernels[i].version);
        }
    }
    
    printf("\nKeeping: \n");
    printf("  - %s (latest kernel)\n", kernels[0].version);
    if (running_index != 0) {
        printf("  - %s (running kernel)\n", kernels[running_index].version);
    }
    
    if (!confirm_deletion("\nProceed with deletion?")) {
        printf("Auto-clean cancelled.\n");
        return;
    }
    
    for (int i = 0; i < kernel_count; i++) {
        if (to_delete[i]) {
            delete_kernel(i);
        }
    }
    
    printf("\nAuto-clean completed.\n");
    
    update_grub();
}

int main(int argc, char *argv[]) {
    get_running_kernel();
    find_kernels();
    
    if (argc == 2 && (strcmp(argv[1], "-l") == 0 || strcmp(argv[1], "--list") == 0)) {
        list_kernels();
    } else if (argc == 3 && (strcmp(argv[1], "-d") == 0 || strcmp(argv[1], "--delete") == 0)) {
        list_kernels();
        delete_kernels(argv[2]);
    } else if (argc == 2 && (strcmp(argv[1], "-a") == 0 || strcmp(argv[1], "--auto") == 0)) {
        list_kernels();
        auto_clean();
    } else {
        print_help();
    }
    
    return 0;
}
