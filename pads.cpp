#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <pthread.h>  // 스레드를 사용하기 위한 라이브러리

// 시리얼 통신 스레드용 데이터 구조
struct SerialThreadData {
    int ttyfd;
    int active;
};

void* serialThread(void* data) {
    struct SerialThreadData* threadData = (struct SerialThreadData*)data;

    char buffer[256];
    while (threadData->active) {
        // 데이터를 읽고 즉시 표시
        ssize_t bytes_read = read(threadData->ttyfd, buffer, sizeof(buffer));
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            printf("Received data: %s\n", buffer);
        }
    }

    return NULL;
}

int main(void) {
    struct termios newtio;
    int ttyfd;
    char *ttyname = "/dev/ttyS11";  // 시리얼 포트 이름
    ttyfd = open(ttyname, O_RDWR | O_NOCTTY);

    if (ttyfd < 0) {
        perror("Error opening serial port");
        return -1;
    }

    memset(&newtio, 0, sizeof(newtio));

    int baud_rate, data_bits, parity, stop_bits;
    printf("baud rate (9600 or 115200): ");
    scanf("%d", &baud_rate);

    printf("data bits (7 or 8): ");
    scanf("%d", &data_bits);

    printf("parity (0 for none, 1 for odd, 2 for even): ");
    scanf("%d", &parity);

    printf("number of stop bits (1 or 2): ");
    scanf("%d", &stop_bits);

    speed_t speed;
    switch (baud_rate) {
        case 9600:
            speed = B9600;
            break;
        case 115200:
            speed = B115200;
            break;
        default:
            printf("Unsupported baud rate.\n");
            close(ttyfd);
            return -1;
    }

    cfsetispeed(&newtio, speed);
    cfsetospeed(&newtio, speed);

    if (data_bits == 7) {
        newtio.c_cflag |= CS7;
    } else if (data_bits == 8) {
        newtio.c_cflag |= CS8;
    } else {
        printf("Unsupported number of data bits.\n");
        close(ttyfd);
        return -1;
    }

    if (parity == 0) {
        newtio.c_cflag &= ~PARENB;
    } else if (parity == 1) {
        newtio.c_cflag |= PARODD;
    } else if (parity == 2) {
        newtio.c_cflag |= PARENB;
        newtio.c_cflag &= ~PARODD;
    } else {
        printf("Unsupported parity setting.\n");
        close(ttyfd);
        return -1;
    }

    if (stop_bits == 1) {
        newtio.c_cflag &= ~CSTOPB;
    } else if (stop_bits == 2) {
        newtio.c_cflag |= CSTOPB;
    } else {
        printf("Unsupported number of stop bits.\n");
        close(ttyfd);
        return -1;
    }

    newtio.c_lflag = 0;
    newtio.c_cc[VTIME] = 0;
    newtio.c_cc[VMIN] = 0;

    tcflush(ttyfd, TCIFLUSH);
    tcsetattr(ttyfd, TCSANOW, &newtio);

    printf("Serial port opened: %s\n", ttyname);

    pthread_t tid;
    struct SerialThreadData threadData;
    threadData.ttyfd = ttyfd;
    threadData.active = 1;
    
    if (pthread_create(&tid, NULL, serialThread, (void*)&threadData) != 0) {
        perror("Error creating thread");
        close(ttyfd);
        return -1;
    }

    char data[256];

    while (1) {
        if (fgets(data, sizeof(data), stdin) != NULL) {
            data[strcspn(data, "\n")] = '\0';

            if (strcmp(data, "exit") == 0) {
                threadData.active = 0;  // 시리얼 스레드 종료
                break;
            }

            ssize_t bytes_written = write(ttyfd, data, strlen(data));
            if (bytes_written < 0) {
                perror("Error writing to the serial port");
                close(ttyfd);
                return -1;
            }

            printf("Data sent: %s\n", data);
        } else {
            printf("Error reading user input.\n");
        }
    }

    // 시리얼 스레드의 종료를 기다림
    pthread_join(tid, NULL);

    close(ttyfd);
    return 0;
}