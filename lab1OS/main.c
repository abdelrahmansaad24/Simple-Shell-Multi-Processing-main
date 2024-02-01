#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>

typedef struct{
    char key[100];
    char value[100];
}data;

data array[500];
char working_directory[300];

static void handler(int sig) {
    int status;
    int pid;
    FILE *fptr;
    
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        fptr = fopen("/home/hassan/Documents/lab1OS/process.txt","a+");
        fprintf(fptr,"Child terminated\n");
        fclose(fptr);
    }
}

char* parse_input(char *command){
    char str[200];
    char *token;
    strcpy(str, command);
    token = strtok(str, " ");
    if(strcmp(token, "echo") == 0 || strcmp(token, "export") == 0 || strcmp(token, "cd") == 0){
        return "shell_builtin";
    }else{
        return "executable_or_error";
    }
}

void evaluate_expression(char *command){
    int index;
    char key[20] = "";
    char value[20];
    char new_str[100];
    int size = atoi(array[0].value);
    int found = -1;
    int flag =0, counter = 0;

    for (int i = 0; i < strlen(command); i++){
            if (command[i] == '$'){
                flag = 1;
                index = i;
                i++;
                while(i < strlen(command) && command[i] != ' '){
                    strncat(key, &command[i], 1);
                    i++;
                }
                for (int m = 1; m < size; m++){
                    if(strcmp(array[m].key, key) == 0){
                        found = 1;
                        strcpy(value, array[m].value);
                        break;
                    }
                }
                if(found != 1){
                    printf("Error\n");
                    exit(1);
                }
                found = -1;
                for (int x = 0; x < (strlen(command) + strlen(value) - 1); x++){
                    if (x == index){
                        int c;
                        for(c = 0; c < strlen(value); c++){
                            new_str[x + c] = value[c];
                            counter++;
                        }
                        x = (x + c);
                        continue;
                    }
                    new_str[x] = command[x];
                    counter++;
                }
                strcpy(key, "");
            }
    }
    
    if(flag){
        int i;
        for(i  = 0; i < counter; i++){
            command[i] = new_str[i];
        }
        command[i] = '\0';
    }
   
}


void execute_command(char *command){
    int status;
    pid_t child_id = fork();
    int i = 0;
    int foreground = 1;
    if (command[strlen(command) - 1] == '&'){
        foreground = 0;
    }
    if (child_id < 0){
        printf("creation of fork faild\n");
        exit(1);
    }else if(child_id == 0){
        evaluate_expression(command);
        //child
        char* argument_list[200];
        char *token = strtok(command, " ");
        while(token != NULL){
            argument_list[i++] = token;
            token = strtok(NULL, " ");
        }
        if(foreground == 0){
            argument_list[i - 1] = NULL;
        }else{
            argument_list[i] = NULL;
        }
        execvp(argument_list[0], argument_list);
        printf("Error\n");
        exit(1);
    }else if (foreground) {
        waitpid(child_id, &status, 0);
    }
}


void execute_export(char *command){
    int fd1[2]; // Used to store two ends of first pipe
    int fd2[2]; // Used to store two ends of second pipe
    if (pipe(fd1) == -1) {
        fprintf(stderr, "Pipe Failed");
        return;
    }
    if (pipe(fd2) == -1) {
        fprintf(stderr, "Pipe Failed");
        return;
    }

    int status;
    pid_t child_id = fork();

    if (child_id < 0){
        printf("creation of fork faild\n");
        exit(1);
    }else if(child_id == 0){
        //child
        data arr[600];
        int size;
        char *export_input;
        char *key;
        char *value;

        close(fd1[1]);
        read(fd1[0], arr, 4700);
        close(fd1[0]);
        close(fd2[0]);
        size = atoi(arr[0].value);

        export_input = strtok(command, " ");
        export_input = strtok(NULL, "");
        key = strtok(export_input, "=");
        value = strtok(NULL, "=");
        if ((value[0] != '"' ^ value[strlen(value) - 1] != '"')){
            printf("Error\n");
            exit(1);
        }
        if(value[0] == '"' && value[strlen(value) - 1] == '"'){
            value = value + 1;
            value[strlen(value) - 1] = '\0';
        }
        for(int i = 1; i < size; i++){
            if(strcmp(arr[i].key, key) == 0){
                strcpy(arr[i].value, value);
                write(fd2[1], arr, 4700);
                close(fd2[1]);
                exit(0);
            }
        }
        strcpy(arr[size].key, key);
        strcpy(arr[size].value, value);
        size++;
        sprintf(arr[0].value, "%d", size);
        write(fd2[1], arr, 4700);
        close(fd2[1]);
        
        exit(0);
    }else{
        //parent
        close(fd1[0]);
        write(fd1[1], array, 4700);
        close(fd1[1]);
        waitpid(child_id, &status, 0);
        close(fd2[1]);
        read(fd2[0], array, 4700);
        close(fd2[0]);

    }
}


void execute_cd(char *command){
    char str[200];
    char *path;
    strcpy(str, command);
    path = strtok(str, " ");
    path = strtok(NULL, "");
    if (chdir(path) != 0){
        printf("Error\n");
        return;
    }
    getcwd(working_directory, sizeof(working_directory));
    
}

void execute_echo(char *command){
    int fd1[2]; // Used to store two ends of first pipe
    if (pipe(fd1) == -1) {
        fprintf(stderr, "Pipe Failed");
        return;
    }
    int status;
    pid_t child_id = fork();

    if (child_id < 0){
        printf("creation of fork faild\n");
        exit(1);
    }else if(child_id == 0){
        //child
        data arr[600];
        int size, index, j;
        char *echo_input;
        char key[20] = "";
        char value[20];
        char new_str[100];
        int found = -1;
        int flag = 0;

        echo_input = strtok(command, " ");
        echo_input = strtok(NULL, "");
        close(fd1[1]);
        read(fd1[0], arr, 4700);
        close(fd1[0]);
        size = atoi(arr[0].value);

        if ((echo_input[0] != '"' || echo_input[strlen(echo_input) - 1] != '"')){
            printf("Error1\n");
            exit(1);
        }
        echo_input = echo_input + 1;
        echo_input[strlen(echo_input) - 1] = '\0';

        for (int i = 0; i < strlen(echo_input); i++){
            if (echo_input[i] == '$'){
                flag = 1;
                index = i;
                i++;
                while(i < strlen(echo_input) && echo_input[i] != ' '){
                    strncat(key, &echo_input[i], 1);
                    i++;
                }
                for (int m = 1; m < size; m++){
                    if(strcmp(arr[m].key, key) == 0){
                        found = 1;
                        strcpy(value, arr[m].value);
                        break;
                    }
                }
                if(found != 1){
                    printf("Error\n");
                    exit(1);
                }
                found = -1;
                for (int x = 0; x < (strlen(echo_input) + strlen(value)); x++){
                    if (x == index){
                        int c;
                        for(c = 0; c < strlen(value); c++){
                            new_str[x + c] = value[c];
                        }
                        x = (x + c);
                        continue;
                    }
                    new_str[x] = echo_input[x];
                }
                strcpy(key, "");
            }
        }
        if (flag){
            printf("%s\n", new_str);
        }else{
            printf("%s\n", echo_input);
        }
       
    }else{
        //parent
        close(fd1[0]);
        write(fd1[1], array, 4700);
        close(fd1[1]);
        waitpid(child_id, &status, 0);
    }
}


void execute_shell_bultin(char *command){
    char str[200];
    char *token;
    
    strcpy(str, command);
    token = strtok(str, " ");
    if(strcmp(token, "echo") == 0){
        execute_echo(command);
    }else if(strcmp(token, "export") == 0){
        execute_export(command);
    }else if(strcmp(token, "cd") == 0){
        execute_cd(command);
    }else{
        printf("Error\n");
    }
}



void shell(){
    strcpy(array[0].key, "size");
    strcpy(array[0].value, "1");
    char command[200];
    char *input_type;
    printf("%s => ", working_directory);
    scanf("%[^\n]%*c", command);
    while(strcmp(command, "exit") != 0){
        input_type = parse_input(command);
        if (strcmp(input_type, "shell_builtin") == 0){
            execute_shell_bultin(command);
        }else if (strcmp(input_type, "executable_or_error") == 0){
            execute_command(command);
        }
        printf("%s => ", working_directory);
        scanf("%[^\n]%*c", command);
    }
}

void setup_environment(){
    getcwd(working_directory, sizeof(working_directory));
}

void main(int argc, char *argv[]) {
    signal (SIGCHLD, handler);
    setup_environment();
    shell();
}

