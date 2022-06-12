FROM debian:latest as build
LABEL Author="zxc25077667@protonmail.com"
WORKDIR /app

RUN apt-get update -y && \
    apt-get install -y libsqlite3-dev \
    build-essential sassc
COPY ["Makefile", "main.c", "/app/"]
COPY ["src", "/app/src/"]
COPY ["include", "/app/include/"]
COPY ["scripts","/app/scripts"]
COPY ["templates", "/app/templates/"]
COPY ["static/scss/", "/app/static/scss"]
RUN make release

FROM debian:latest
LABEL Author="zxc25077667@protonmail.com"
ENV port 8080
ENV DB_PATH "/app/data/db.sqlite3"
WORKDIR /app

RUN apt-get update -y && apt-get install -y libsqlite3-dev && \
    mkdir /data && touch ${DB_PATH} && chmod 600 ${DB_PATH} && chown -R 1000:1000 /data
COPY ["static/image", "/app/static/image/"]
COPY ["static/css_old", "/app/static/css_old/"]
COPY ["templates", "/app/templates/"]

COPY --from=build --chown=1000:1000 /app/bin/facebooc /app/facebooc
COPY --from=build --chown=1000:1000 /app/templates/version.html /app/templates/
COPY --from=build --chown=1000:1000 /app/static/css /app/static/css

USER 1000
CMD /app/facebooc ${port}