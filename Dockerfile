FROM debian:latest as build
LABEL Author="zxc25077667@protonmail.com"
WORKDIR /app

RUN apt-get update -y && \
    apt-get install -y libsqlite3-dev \
    build-essential
COPY ["Makefile", "/app/"]
COPY ["src", "/app/src/"]
COPY ["include", "/app/include/"]
RUN make release

FROM debian:latest
LABEL Author="zxc25077667@protonmail.com"
ENV port 8080
ENV DB_PATH "/data/db.sqlite3"
WORKDIR /app

RUN apt-get update -y && apt-get install -y libsqlite3-dev && \
    mkdir /data && touch ${DB_PATH} && chmod 600 ${DB_PATH} && chown -R 1000:1000 /data
COPY ["static", "/app/static/"]
COPY ["templates", "/app/templates/"]

COPY --from=build --chown=1000:1000 /app/bin/facebooc /app/facebooc
USER 1000
CMD /app/facebooc ${port}