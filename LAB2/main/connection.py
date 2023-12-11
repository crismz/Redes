# encoding: utf-8
# Revisión 2019 (a Python 3 y base64): Pablo Ventura
# Copyright 2014 Carlos Bederián
# $Id: connection.py 455 2011-05-01 00:32:09Z carlos $

import os
from constants import *
from base64 import b64encode

def create_error_msg(msg_code):
    """
    Crea y devuelve el mensaje de error según el msg_code que recibe.
    Incluye el caso del CODE_OK.
    """

    assert valid_status(msg_code)
    buf = f"{msg_code} {error_messages[msg_code]}" + EOL
    return buf

def check_filename(filename):
    """
    Chequea si el nombre del archivo es válido.
    Si es inválido devuelve True, sino False.
    """
    
    for c in filename:
        if c not in VALID_CHARS:
            return True
    return False

def check_path(path):
        """
        Chequea si el archivo existe.
        Si no existe devuelve True, sino False.
        """
       
        if not os.path.isfile(path):
            return True
        return False

class Connection(object):
    """
    Conexión punto a punto entre el servidor y un cliente.
    Se encarga de satisfacer los pedidos del cliente hasta
    que termina la conexión.
    """

    def __init__(self, socket, directory):
        self.socket = socket
        self.directory = directory
        self.connected = True   # Flag para estado de conexión

    def send(self, message):
        message = message.encode('ascii')
        while len(message) > 0:
            sent = self.socket.send(message)
            assert sent > 0
            message = message[sent:]

    def get_file_listing(self):
        buf = create_error_msg(CODE_OK)
        for file in os.listdir(self.directory):
            buf += file  + " " + EOL
        buf += EOL

        self.send(buf)

    def get_metadata(self, filename):

        error = check_filename(filename)
        if error: 
            self.send(create_error_msg(INVALID_ARGUMENTS))
            return

        path = os.path.join(self.directory, filename)

        error = check_path(path)
        if error: 
            self.send(create_error_msg(FILE_NOT_FOUND))
            return

        buf = create_error_msg(CODE_OK)
        filesize = os.stat(path).st_size
        buf += str(filesize) + EOL
        self.send(buf)

    def get_slice(self, filename, offset, size):
        # Convertimos en int porque vienen como str
        offset = int(offset)   
        size = int(size)

        error = check_filename(filename)
        if error: 
            self.send(create_error_msg(INVALID_ARGUMENTS))
            return

        path = os.path.join(self.directory, filename)

        error = check_path(path)
        if error: 
            self.send(create_error_msg(FILE_NOT_FOUND))
            return

        filesize = os.stat(path).st_size

        if offset < 0 and size < 0:
            self.send(create_error_msg(INVALID_ARGUMENTS))
            return

        if offset + size > filesize:
            self.send(create_error_msg(BAD_OFFSET))
            return

        with open(path, "rb") as file:
            file.seek(offset)
            buf = file.read(size)

        buf64_bytes = b64encode(buf)
        buf64 = buf64_bytes.decode('ascii')
        buf = create_error_msg(CODE_OK) + buf64 + EOL

        self.send(buf)

    def quit(self):
        self.send(create_error_msg(CODE_OK))
        self.socket.close()
        self.connected = False


    def execute_command(self, argv):
        """
        Ejecuta el comando correspondiente
        """
        match argv:
            case ['get_file_listing']:
                self.get_file_listing()

            case ['get_metadata', filename]:
                self.get_metadata(filename)

            case ['get_slice', filename, offset, size]:
                if not (offset.isdigit() and size.isdigit()):
                    self.send(create_error_msg(INVALID_ARGUMENTS))
                else:
                    self.get_slice(filename, offset, size)

            case ['quit']:
                self.quit()

            case (
                    ['get_file_listing', *_] | ['get_metadata', *_] |
                    ['get_slice', *_] | ['quit', *_]):
                self.send(create_error_msg(INVALID_ARGUMENTS))

            case _:
                self.send(create_error_msg(INVALID_COMMAND))

    def handle(self):
        """
        Atiende eventos de la conexión hasta que termina.
        """
        while self.connected:
            # El mensaje deberia ser un comando
            message = self.socket.recv(1024).decode('ascii')

            # Recibimos data hasta que haya un EOL
            # Chequeamos que el comando no sea muy grande con MAX_BYTES
            while not EOL in message:
                message += self.socket.recv(1024).decode('ascii')
                if len(message) > MAX_BYTES:
                    self.send(create_error_msg(BAD_REQUEST))
                    self.quit()
                    break

            # Si hubo un quit se sale del ciclo
            if not self.connected:
                break

            # Se hace una lista separada por comandos
            message_list = message.split(EOL)

            # Chequea que el mensaje termine con un EOL
            if message_list[-1] != '':
                self.send(create_error_msg(BAD_EOL))
                self.quit()
                break
            else:
                message_list = message_list[:-1]

            # Chequea que no haya un \n dentro de los comandos
            for command in message_list:
                if '\n' in command:
                    self.send(create_error_msg(BAD_EOL))
                    self.quit()
                    break

            if not self.connected:
                break

            # Ejecuta cada comando y levanta una excepción en caso
            # de algun error para que el servidor no se corte 
            for command in message_list:
                argv = command.split()
                try:
                    self.execute_command(argv)
                except Exception:
                    self.send(create_error_msg(INTERNAL_ERROR))
                    self.quit()
