#!/usr/bin/python
from flask import Flask, request, abort
import os

server = Flask(__name__)  # HTTP server


@server.route('/', methods=['GET'])
def do_nothing():
    server.logger.info('GET')
    return ""


@server.route('/', methods=['POST'])
def fetch_secrect():
    token = request.values.get('token')
    if(token == os.getenv('TOKEN')):
        # os.system(os.getenv('SHELL_SCRIPT'))
        print("Test")
        return "Success"
    else:
        abort(403, {'token': token})


if __name__ == '__main__':
    server.run()  # default port localhost:5000
