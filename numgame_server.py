
import argparse
import random
import math
import socket

BUF_SIZE = 1024
#random.seed(42)

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--host", type=str, default="", help="Host to bind")
    parser.add_argument("--port", type=int, required=True)
    parser.add_argument("--min_num", type=int, default=1)
    parser.add_argument("--max_num", type=int, required=True)
    args = parser.parse_args()

    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.bind((args.host, args.port))
    server.listen(5)

    print(f"Number Guessing Game Server started on {args.host or '0.0.0.0'}:{args.port}")

    while True:
        conn, addr = server.accept()
        print(f"Client connected: {addr}")

        # 1️. username 요청 및 수신
        username = conn.recv(BUF_SIZE).decode().strip()
        if not username:
            conn.close()
            continue

        print(f"[{username}] joined the game.")

        # 2️. 랜덤 타깃 생성
        target = random.randint(args.min_num, args.max_num)
        attempts = 0
        conn.send(f"RANGE {args.min_num} {args.max_num}\n".encode())

        # 3️. 게임 루프
        while True:
            data = conn.recv(BUF_SIZE)
            if not data:
                break

            msg = data.decode().strip().lower()
            if msg == "quit":
                conn.send(b"Bye!\n")
                break

            if not msg.isdigit():
                conn.send(b"Invalid input. Please send a number.\n")
                continue

            guess = int(msg)
            attempts += 1

            if guess < target:
                conn.send(b"Higher!\n")
            elif guess > target:
                conn.send(b"Lower!\n")
            else:
                feedback = f"Correct! The answer is {target}. You guessed it in {attempts} attempts.\n"
                conn.send(feedback.encode())
                print(f"RESULT:CORRECT USER={username} ATTEMPTS={attempts}\n")

                break

        conn.close()
        print(f"[{username}] disconnected after {attempts} attempts.\n")

if __name__ == "__main__":
    main()
