#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>

void exec(char *args[]) {
    /* 内建命令 */
    char *p, *name, *value;
    if (strcmp(args[0], "cd") == 0) {
        if (args[1])
            chdir(args[1]);
    }
    else if (strcmp(args[0], "pwd") == 0) {
        char wd[4096];
        puts(getcwd(wd, 4096));
    }
    else if (strcmp(args[0], "exit") == 0)
        return;
    /*else if (strcmp(args[0], "env") == 0) {
        p = getenv("PATH");
        printf("PATH=%s\n", p);
    }*/
    else if (strcmp(args[0], "export") == 0){
        int flag = 0;
        for (name = args[1], value = args[1]; *value; value++) {
            if (*value == '=') {
                flag = 1;
                *value = '\0';
                value++;
                break;
            }
        }
        if (flag == 0) {
            printf("bad assigment\n");
            return;
        }
        setenv(name, value, 1);
    }
        /* 外部命令 */
    else {
        pid_t pid = fork();
        if (pid < 0) {
            perror("exec fork error");
            exit(-1);
        }
        else if (pid == 0) {
            /* 子进程 */
            execvp(args[0], args);
            /* execvp失败 */
            return;
        }
        /* 父进程 */
        int status;
        waitpid(pid, &status, 0);
    }
}

int expipe(char *arg[20][128], int pipenum) {
    int fd[2];
    if (pipe(fd) != 0) {
        perror("pipe error");
        exit(-1);
    }

    int status;
    int save;

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork error");
        exit(-1);
    }
    if (pid == 0) {
        close(fd[0]);
        dup2(fd[1], STDOUT_FILENO);
        close(fd[1]);
        if (pipenum == 1)
            exec(arg[0]);
        if (pipenum > 1) {
            expipe(arg, pipenum - 1);
        }
        exit(0);
    }
    else {
        save = dup(STDIN_FILENO);
        close(fd[1]);
        dup2(fd[0], STDIN_FILENO);
        exec(arg[pipenum]);
        close(fd[0]);
        dup2(save, STDIN_FILENO);
        waitpid(pid, &status, 0);
        return 0;
    }
}

int main() {
    /* 输入的命令行 */
    char cmd[256];
    /* 命令行拆解成的各部分，以空指针结尾 */
    char *args[128];
    char *arg[20][128];
    while (1) {
        /* 提示符 */
        printf("# ");
        fflush(stdin);
        fgets(cmd, 256, stdin);
        /* 清理结尾的换行符 */
        int i;
        for (i = 0; cmd[i] != '\n'; i++)
            ;
        cmd[i] = '\0';
        /* 拆解命令行 */
        args[0] = cmd;
        for (i = 0; *args[i]; i++)
            for (args[i+1] = args[i] + 1; *args[i+1]; args[i+1]++)
                if (*args[i+1] == ' ') {
                    *args[i+1] = '\0';
                    for (args[i+1]++; *args[i+1] == ' '; args[i+1]++)
                        ;
                    break;
                }
        args[i] = NULL;
        /* 清空命令 */
        for (int j = 0; j < 20; j++)
            for (int k = 0; k < 128; k++)
                arg[j][k] = NULL;

        /* 没有输入命令 */
        if (!args[0])
            continue;

        if (strcmp(args[0], "exit") == 0)
            return 0;

        /* 将管道符与命令分开，命令存入 arg */
        int pipenum = 0, pipeflag = 0;
        int k = 0, m = 0;
        for (int j = 0; j < i; j++) {
            if ((strcmp(args[j], "|") == 0) || (j == i - 1 && pipenum != 0)) {
                for (k = 0, m = pipeflag; k + pipeflag < j; k++) {
                    arg[pipenum][k] = args[m];
                    m++;
                }
                pipenum++;
                pipeflag = j + 1;
            }
        }
        arg[pipenum - 1][k] = args[m];
        arg[pipenum][k] = NULL;
        pipenum--;

        if (pipenum <= 0)
            exec(args);
        else {
            expipe(arg, pipenum);
        }
        /*
        for (int k = 0; k < pipenum; k++) {
            pid_t pid = fork();
            if (pid == -1)
                exit(-1);
            if (pid == 0)
        }
        */
    }
}