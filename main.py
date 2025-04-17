import socket
import struct
import random
import math

from commands import *

PORT = 8080
SERVER_IP = "192.168.0.101"
MIN = 1
MAX = 100


class MatrixData:
    def __init__(self, matrix_size=0, threads_count=0):
        self.matrix_size = matrix_size
        self.threads_count = threads_count
        self.matrix = [[0] * matrix_size for _ in range(matrix_size)]
        if matrix_size:
            self.fill_matrix()

    def fill_matrix(self):
        for i in range(self.matrix_size):
            for j in range(self.matrix_size):
                self.matrix[i][j] = random.randint(MIN, MAX)


def sendTLV(sock, tlv_type, data):
    length = struct.pack('!I', len(data))
    sock.sendall(struct.pack('!B', tlv_type) + length + data)


def sendCommand(sock, command):
    data = struct.pack('!I', command)
    sendTLV(sock, COMMAND, data)


def sendData(sock, data):
    matrixArray = [element for row in data.matrix for element in row]
    matrixArray_bytes = b''.join([struct.pack('!I', val) for val in matrixArray])
    sendTLV(sock, MATRIX, matrixArray_bytes)

    thread_N = struct.pack('!I', data.threads_count)
    sendTLV(sock, THREADS, thread_N)


def recv_all(sock, n):
    data = b''
    while len(data) < n:
        part = sock.recv(n - len(data))
        if not part:
            return None
        data += part
    return data


def receiveTLV(sock):
    type_byte = recv_all(sock, 1)
    if not type_byte:
        return None, None

    type = struct.unpack('!B', type_byte)[0]

    length_bytes = recv_all(sock, 4)
    if not length_bytes:
        return None, None

    length = struct.unpack('!I', length_bytes)[0]

    value = recv_all(sock, length)
    if not value or len(value) != length:
        return None, None

    return type, value


def receiveResponse(sock):
    type, value = receiveTLV(sock)
    if not value:
        return -1
    return struct.unpack('!I', value)[0]


def receiveData(sock, data):
    type, buffer = receiveTLV(sock)
    if not buffer:
        print("Failed to receive TLV segment.")
        return

    total_elements = len(buffer) // 4
    data.matrix_size = int(math.sqrt(total_elements))
    data.matrix = [[0] * data.matrix_size for _ in range(data.matrix_size)]

    values = [struct.unpack('!I', buffer[i:i + 4])[0] for i in range(0, len(buffer), 4)]

    index = 0
    for i in range(data.matrix_size):
        for j in range(data.matrix_size):
            data.matrix[i][j] = values[index]
            index += 1

    print("Matrix data received from the server.")


def main():
    random.seed()

    clientSock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        clientSock.connect((SERVER_IP, PORT))
    except:
        print("Connection to server failed")
        return

    sendCommand(clientSock, IS_BUSY)
    message = receiveResponse(clientSock)
    if message == BUSY:
        print("Server is busy. Please try again later.")
        clientSock.close()
        return

    client_ip, client_port = clientSock.getsockname()
    print(f"Client Address: {client_ip} | Port: {client_port}")

    matrix_size = int(input("Enter matrix size: "))
    threads_count = int(input("Enter number of threads: "))

    data = MatrixData(matrix_size, threads_count)
    row_test = data.matrix_size - 1
    col_test = 0
    initial_value = data.matrix[row_test][col_test]

    sendCommand(clientSock, CONFIG)
    sendData(clientSock, data)

    print("Matrix data sent to the server.")

    message = receiveResponse(clientSock)
    if message != CONFIG_OK:
        print("Config error.")
        clientSock.close()
        return

    sendCommand(clientSock, CALCULATE)
    message = receiveResponse(clientSock)
    if message != CALCULATION_STARTED:
        print("Calculation start error.")
        clientSock.close()
        return

    while True:
        sendCommand(clientSock, GET_RESULT)
        message = receiveResponse(clientSock)
        if message == CURRENT_PROGRESS:
            progress_bytes = clientSock.recv(4)
            progress = struct.unpack('!I', progress_bytes)[0]
            print(f"Progress: {progress}% of the data processed")
        elif message == COMPLETED:
            receiveData(clientSock, data)
            print("All data has been processed. Final matrix received.")

            new_value = data.matrix[row_test][col_test]
            if new_value != initial_value:
                print("Matrix was updated correctly.")
            else:
                print("Matrix value did not change.")
            break
        else:
            print("Error receiving result.")
            break

    clientSock.close()


if __name__ == '__main__':
    main()