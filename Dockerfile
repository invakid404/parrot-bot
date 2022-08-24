FROM ubuntu

ENV DEBIAN_FRONTEND noninteractive

RUN apt update && apt install -y build-essential cmake git libcurl4-openssl-dev

WORKDIR /opt/app

COPY . .
RUN cmake -DCMAKE_BUILD_TYPE=Release . && cmake --build . --config Release

ENTRYPOINT ["/opt/app/parrot_bot"]