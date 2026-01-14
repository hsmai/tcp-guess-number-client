

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUF_SIZE 1024

int main(int argc, char *argv[]) {
    int sock;
    struct sockaddr_in serv_addr;
    char send_buf[BUF_SIZE], recv_buf[BUF_SIZE];

    // -------------------------------
    // 1. 명령행 인자 확인
    // -------------------------------

    if (argc != 4) {
        printf("Usage: %s <server_ip> <port> <username>\n", argv[0]);
        exit(1);
    }

    const char *server_ip = argv[1];
    int port = atoi(argv[2]);
    const char *username = argv[3];

    // -------------------------------
    // 2️. 소켓 생성
    // -------------------------------
    // socket() 함수를 사용하여 소켓을 생성

    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("socket() error");
        exit(1);
    }

    // -------------------------------
    // 3️. 서버 주소 설정
    // -------------------------------

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;

    if (inet_aton(server_ip, &serv_addr.sin_addr) == 0) {
        fprintf(stderr, "Your address is wrong\n");
        close(sock);
        exit(1);
    }

    serv_addr.sin_port = htons(port);



    // -------------------------------
    // 4️. 서버에 연결
    // -------------------------------
    // connect() 함수를 사용하여 서버에 연결
    // printf("Connected to server %s:%d\n", server_ip, port);

    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
        perror("connect() error");
        close(sock);
        exit(1);
    }

    printf("Server Connected %s:%d\n", server_ip, port);
    printf("Connected user : [%s]\n", username);



    // -------------------------------
    // 5️. 사용자 이름 전송
    // -------------------------------
    // send() 함수를 사용하여 username을 서버로 전송

    snprintf(send_buf, sizeof(send_buf), "%s\n", username);

    if (send(sock, send_buf, strlen(send_buf), 0) <=0) {
        perror("send() error");
        close(sock);
        exit(1);
    }



    // -------------------------------
    // 6️. 범위 수신 (예: "RANGE 1 100")
    // -------------------------------
    // recv() 함수를 사용하여 서버 메시지를 수신
    // sscanf()를 사용하여 범위 정보를 파싱

    int len;
    len = recv(sock, recv_buf, sizeof(recv_buf) -1, 0);

    if (len <= 0) {
        perror("recv() error");
        close(sock);
        exit(1);
    }

    recv_buf[len] = '\0';

    int minimum_num, maximum_num;

    if (sscanf(recv_buf, "RANGE %d %d", &minimum_num, &maximum_num) !=2) {
        fprintf(stderr, "Your RANGE msg is wrong\n");
        close(sock);
        exit(1);
    }

    printf("Server : RANGE %d ~ %d\n", minimum_num, maximum_num);


    // -------------------------------
    // 7️. 추측 반복문
    // -------------------------------
    //   - 서버에 숫자 추측값을 전송
    //   - 서버로부터 피드백("Higher", "Lower", "Correct") 수신
    //   - 피드백에 따라 다음 추측값 조정
    //   - 정답을 맞출 때까지 반복

    int low;
    int mid;
    int high;
    int attempt;

    low = minimum_num;
    high = maximum_num;
    attempt = 0;

    while (low <= high) {
        mid = low + (high-low)/2;
        attempt++;

        snprintf(send_buf, sizeof(send_buf), "%d\n", mid);
        if (send(sock, send_buf, strlen(send_buf), 0) <= 0) {
            perror("send() error");
            break;
        }

        len = recv(sock, recv_buf, sizeof(recv_buf)-1, 0);
        if (len <=0) {
            perror("recv() error");
            break;
        }

        recv_buf[len]='\0';

        printf("Client guess : %d\n", mid);
        printf("Server : %s", recv_buf);


        if (strncmp(recv_buf, "Higher", 6)==0) {
            low = mid +1;
        } else if (strncmp(recv_buf, "Lower", 5)==0) {
            high = mid -1;
        } else if (strncmp(recv_buf, "Correct", 7)==0) {
            break;
        } else {
            break;
        }

    }


    // -------------------------------
    // 8️. 연결 종료
    // -------------------------------
    // close() 함수를 사용하여 소켓 닫기

    close(sock);
    return 0;
}
