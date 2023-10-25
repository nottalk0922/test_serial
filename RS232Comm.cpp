#include <stdio.h>
#include <iostream>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <future>
#include <functional>

const char PORT1[] = "/dev/ttyUSB0";
const char PORT2[] = "/dev/ttyS11";

bool m_break = false;

int readin, readinTot = 0;
char buffer[140];
char* bufptr;
int written;
int messageSize = 0;

int openPort(const char* port)
{
    int fd;
    fd = open(port, O_RDWR | O_NOCTTY);
    if (fd == -1)
    {
        printf("error : could not open port");
    }
    else {
        fcntl(fd, F_SETFL, 0);
    }
    return fd;
}
void configPort(int fd1, int fd2)
{
    struct termios options;
    __uint32_t bps;
    char data='8';
    char stopbits;
    char parity[6];

    memset(&options, 0, sizeof(options));
    tcgetattr(fd1, &options);
    tcgetattr(fd2, &options);
    
    printf("\n");
    printf("Bps : ");
    scanf("%d", &bps);
    printf("\n");

    printf("parity : ");
    scanf("%s", parity);
    printf("\n");

    printf("Data : ");
    std::cin >> data;
    printf("\n");

    printf("StopBits : ");
    std::cin >> stopbits;
    printf("\n");

    switch (bps)
    {
    case 57600:
        cfsetispeed(&options, B57600);
        cfsetospeed(&options, B57600);
        break;
    case 115200:
        cfsetispeed(&options, B115200);
        cfsetospeed(&options, B115200);
        break;
    case 1200:
        cfsetispeed(&options, B1200);
        cfsetospeed(&options, B1200);
        break;
    case 2400:
        cfsetispeed(&options, B2400);
        cfsetospeed(&options, B2400);
        break;
    case 4800:
        cfsetispeed(&options, B4800);
        cfsetospeed(&options, B4800);
        break;
    default:
        cfsetispeed(&options, B9600);
        cfsetospeed(&options, B9600);
        break;
    }
    
    
    options.c_cflag |= (CLOCAL | CREAD);

    switch (*parity)
    {
    case 'N':
        options.c_cflag &= ~PARENB;
        break;
    case 'E':
        options.c_cflag |= PARENB;
        options.c_cflag &= ~PARODD;
        break;
    case 'O':
        options.c_cflag |= PARENB;
        options.c_cflag |= PARODD;
    }

    if (stopbits == '1') options.c_cflag &= ~CSTOPB;
    else options.c_cflag &= CSTOPB;

    options.c_cflag &= ~CSIZE;

    switch (data)
    {
    case '5':
        options.c_cflag |= CS5;
        break;
    case '6':
        options.c_cflag |= CS6;
        break;
    case '7':
        options.c_cflag |= CS7;
        break;
    default:
        options.c_cflag |= CS8;
        break;
    }
    options.c_lflag = 0;
    options.c_cc[VMIN] = 0;
    options.c_cc[VTIME] = 240;
    
    tcsetattr(fd1, TCSANOW, &options);
    tcsetattr(fd2, TCSANOW, &options);
}

void *readData(void *arg) {
    int tty_fd = *((int *)arg);
    char buffer[256];
    ssize_t bytes_read;

    while (1) {
        bytes_read = read(tty_fd, buffer, sizeof(buffer));
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            printf("Received data: %s\n", buffer);
        }
    }
    return NULL;
}

void writeData(int fd, char * msg){
    ssize_t bytes_written = write(fd, msg, strlen(msg));
    if (bytes_written < 0) {
        perror("Error writing to the serial port");
        close(fd);
        return ;
    }

}

int main() {
    int fd1,fd2;
    char port;
    char data[140] = "";

    printf("port( 1 : /dev/ttyS10, 2 : /dev/ttyS11) : ");
    scanf("%c", &port);
    
    if (port == '1')
    {
        fd1 = openPort(PORT1);
        fd2 = openPort(PORT1);
    }
    else
    {
        fd1 = openPort(PORT2);
        fd2 = openPort(PORT2);
    }
    configPort(fd1, fd2);

    pthread_t thread;
    if (pthread_create(&thread, NULL, readData, &fd1)) {
        perror("Error creating thread");
        return -1;
    }

    while (1) {
        if (fgets(data, sizeof(data), stdin) != NULL) {
            data[strcspn(data, "\n")] = '\0';

            if (strcmp(data, "exit") == 0) {
                break;
            }

            writeData(fd1, data);
            printf("Data send: %s\n", data);
        } else {
            printf("Error reading user input.\n");
        }
    }

    pthread_join(thread, NULL);
    return 0;  

}