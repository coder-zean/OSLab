#include <stdio.h>
#include <dirent.h>
#include <sys/types.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>


/**
 * pstree命令默认情况下会以init进程为根（一般情况下pid为1）
 * 如果pstree命令后跟上pid，则会以该pid为根，显示树状
*/

typedef struct {
    const char* proc_name;
    unsigned int pid;
    unsigned int ppid;
    int thread_num;
    int thread_id_arr[128];
}ProcMsg;

typedef struct proc_tree_node_t {
    struct proc_tree_node_t* parent;
    ProcMsg* self;
    unsigned int child_num;
    struct proc_tree_node_t* child[128];
}ProcTreeNode;

ProcTreeNode* proc_tree[1024] = {0};
int proc_count = 0;

int IsDigit(const char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        if (!isdigit(str[i])) {
            return 0;
        }
    }
    return 1;
}

ProcMsg* NewProcMsg(int pid) {
    ProcMsg* proc = (ProcMsg*)malloc(sizeof(ProcMsg));
    proc->pid = pid;
    proc->thread_num = 0;
    return proc;
}

ProcTreeNode* NewProcTreeNode(ProcMsg* self) {
    ProcTreeNode* proc_node = malloc(sizeof(ProcTreeNode));
    proc_node->child_num = 0;
    proc_node->self = self;
    proc_node->parent = 0;
    return proc_node;
}

ProcTreeNode* BinarySearch(int end, int dst_pid) {
    int left = 0, right = end;
    while (left < right) {
        int mid = (left + right) / 2;
        if (proc_tree[mid]->self->pid == dst_pid) {
            return proc_tree[mid];
        }
        if (proc_tree[mid]->self->pid > dst_pid) {
            right = mid;
        } else {
            left = mid + 1;
        }
    }
    return 0;
}

void RecursPrint(ProcTreeNode* node, int tab_num) {
    for (int i = tab_num; i > 0; i--) {
        printf("  ");
    }
    printf("%s\n", node->self->proc_name);
    if (node->self->thread_num > 1) {
        for (int i = tab_num + 1; i > 0; i--) {
            printf("  ");
        }
        printf("%d*[{%s}]\n", node->self->thread_num - 1, node->self->proc_name);
    }
    for (int i = 0; i < node->child_num; i++) {
        RecursPrint(node->child[i], tab_num + 1);
    }
}

void ReadThreadMsg(ProcMsg* proc, char* dir_name) {
    DIR* proc_task_dir = opendir(dir_name);
    struct dirent* entry1;
    while ((entry1 = readdir(proc_task_dir)) != 0) {
        if (IsDigit(entry1->d_name)) {
            proc->thread_id_arr[proc->thread_num++] = atoi(entry1->d_name);
        }
    }
    closedir(proc_task_dir);
}

ProcMsg* ReadProcessMsg(char* file_name) {
    FILE* stat_file = fopen(file_name, "r");
    char record[256];
    char temp;
    int i = 0;
    while ((temp = fgetc(stat_file)) != ' ') {
        record[i++] = temp;
    }
    record[i] = '\0';
    ProcMsg* proc_msg = NewProcMsg(atoi(record));
    i = 0;
    while ((temp = fgetc(stat_file)) != ' ') {
        if (temp == '(' || temp == ')')  continue;
        record[i++] = temp;
    }  
    record[i] = '\0';
    char* process_name = malloc(sizeof(char) * strlen(record));
    memcpy(process_name, record, strlen(record));
    proc_msg->proc_name = process_name;
    i = 0;
    while ((temp = fgetc(stat_file)) != ' ') ;
    while ((temp = fgetc(stat_file)) != ' ') {
        record[i++] = temp;
    }
    record[i] = '\0';
    proc_msg->ppid = atoi(record);
    fclose(stat_file);
    ProcTreeNode* proc_node = NewProcTreeNode(proc_msg);
    ProcTreeNode* parent_node = BinarySearch(proc_count, proc_msg->ppid);
    parent_node->child[parent_node->child_num++] = proc_node;
    proc_node->parent = parent_node;
    proc_tree[proc_count++] = proc_node;
    return proc_msg;
}

void ReadProcDir() {
    DIR* proc_dir = opendir("/proc");
    struct dirent* entry;
    while ((entry = readdir(proc_dir)) != 0) {
        if (IsDigit(entry->d_name)) {
            char file_name[256] = {'\0'};
            strcat(file_name, "/proc/");
            strcat(file_name, entry->d_name);
            strcat(file_name, "/stat");
            ProcMsg* proc_msg = ReadProcessMsg(file_name);
            file_name[0] = '\0';
            strcat(file_name, "/proc/");
            strcat(file_name, entry->d_name);
            strcat(file_name, "/task");
            ReadThreadMsg(proc_msg, file_name);
        }
    }
    closedir(proc_dir);
}

void PrintVesionMsg() {
    printf("pstree (PSmisc) UNKNOWN\n");
    printf("Copyright (C) 1993-2019 Werner Almesberger and Craig Small\n");
    printf("\n");
    printf("PSmisc comes with ABSOLUTELY NO WARRANTY.\n");
    printf("This is free software, and you are welcome to redistribute it under\n");
    printf("the terms of the GNU General Public License.\n");
    printf("For more information about these matters, see the files named COPYING.)\n");
}

void RecursPrintWithPid(ProcTreeNode* node, int tab_num) {
    for (int i = tab_num; i > 0; i--) {
        printf("  ");
    }
    printf("%s(%d)\n", node->self->proc_name, node->self->pid);
    if (node->self->thread_num > 1) {
        for (int j = 1; j < node->self->thread_num; j++) {
            for (int i = tab_num + 1; i > 0; i--) {
                printf("  ");
            }
            printf("{%s}(%d)\n", node->self->proc_name, node->self->thread_id_arr[j]);
        }
    }
    for (int i = 0; i < node->child_num; i++) {
        RecursPrintWithPid(node->child[i], tab_num + 1);
    }
}

void ParseParam(char* param, ProcTreeNode* node) {
    if (!strcmp("-V", param) || !strcmp("--version", param)) {
        PrintVesionMsg();
    } else if (!strcmp("-p", param) || !strcmp("--show-pids", param)) {
        RecursPrintWithPid(node, 0);
    } else if (!strcmp("-n", param) || !strcmp("--numeric-sort", param)) {
        RecursPrint(node, 0);
    } else {
        printf("param error!!!\n");
        printf("cmd format: ./pstree [pid] [-V|--version|-p|--show-pids|-n|--numeric-sort]\n");
        exit(1);
    }
}

int main(int argv, char** argc) {
    if (argv > 3) return 1;
    ProcMsg* pid0 = NewProcMsg(0);
    ProcTreeNode* pid0_tree_node = NewProcTreeNode(pid0);
    proc_tree[proc_count++] = pid0_tree_node;
    ReadProcDir();
    if (argv == 2) {
        if (IsDigit(argc[1])) {
            ProcTreeNode* node = BinarySearch(proc_count, atoi(argc[1]));
            if (!node) {
                printf("pid isn't exist!!!\n");
                return 1;
            }
            RecursPrint(node, 0);
        } else {
            ParseParam(argc[1], proc_tree[1]);
        }
    } else if (argv == 3) {
        if (IsDigit(argc[1])) {
            ProcTreeNode* node = BinarySearch(proc_count, atoi(argc[1]));
            if (!node) {
                printf("pid isn't exist!!!\n");
                return 1;
            }
            ParseParam(argc[2], node);
        } else if (IsDigit(argc[2])) {
            ProcTreeNode* node = BinarySearch(proc_count, atoi(argc[2]));
            if (!node) {
                printf("pid isn't exist!!!\n");
                return 1;
            }
            ParseParam(argc[1], node);
        }
    } else {
        printf("param error!!!\n");
        printf("cmd format: ./pstree [pid] [-V|--version|-p|--show-pids|-n|--numeric-sort]\n");
        exit(1);
    }
    
    return 0;
}

// int main() {
//     printf("systemd─┬─systemd-journal───init\n");
//     printf("        ├─systemd-udevd\n");
//     printf("        ├─systemd-udevd\n");
//     printf("        └─systemd-udevd\n");
//     return 0;
// }