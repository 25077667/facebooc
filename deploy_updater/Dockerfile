FROM python:alpine
LABEL author="zxc25077667@protonmail.com"
WORKDIR /app

COPY ["requirements.txt", "updater.py", "/app/"]
RUN pip3 install -r /app/requirements.txt

VOLUME [ "/base" ]
CMD python updater.py