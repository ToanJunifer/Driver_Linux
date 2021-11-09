#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>  // related to O_RDWR
#include <unistd.h> // for open, read, write, close function
#include <sys/types.h>
#include <sys/stat.h>
#define buff_size   1024
int8_t write_buff[buff_size];
int8_t read_buff[buff_size];
int main(){
    int fd;
    char option;
    printf("Test device driver application onf user space.\n");

    fd = open("/dev/my_device2",O_RDWR);
    if (fd < 0){
        printf("Can not open device file.\n");
        return 0;
    }

    while(1){
        printf("*******Please choose your option*******\n");
        printf("        1. Write        \n");
        printf("        2. Read         \n");
        printf("        3. Exit         \n");
        scanf(" %c",&option);
        printf("You just choose: %c\n",option);
        switch (option){
        case '1':
            // printf("Type the string what you want to write into device file:\n");
            scanf(" %[^\t\n]s", write_buff); // not working properly
            // strcpy(write_buff,"Dao Van Toan");
            write(fd,write_buff,strlen(write_buff));
            // flush(fd);   // to save data in file directly, dont need to wait close file
            printf("Write done\n");
            break;
        case '2':
            printf("Data is reading...\n");
            read(fd,read_buff,buff_size);
            printf("Data: \"%s\"\n\n",read_buff);
            break;
        case '3':
            close(fd);
            exit(1);
            break;
        default:
            printf("Type invalid option\n");
            break;
        }
    }
    close(fd);
}