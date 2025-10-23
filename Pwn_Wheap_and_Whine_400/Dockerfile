# Build: docker build -t wheapnwhine .
# Run: docker run -dt -p 1337:1337 wheapnwhine --restart always
FROM ubuntu:22.04

RUN apt update && apt -y upgrade && apt install -y socat wine
RUN wineboot

WORKDIR /chal
COPY HeapNoteTaker.exe flag.txt /chal/

ENTRYPOINT ["socat", "TCP-LISTEN:1337,reuseaddr,fork", "EXEC:wine /chal/HeapNoteTaker.exe"]
