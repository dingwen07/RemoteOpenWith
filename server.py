import socket
import os
import threading

SERVER_IP_V4 = "0.0.0.0"
SERVER_IP_V6 = "::"
SERVER_PORT = 2335
BUFFER_SIZE = 1024
FILE_FOLDER = './files'

def create_response_socket(ip_version):
    if ip_version == 4:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.bind((SERVER_IP_V4, SERVER_PORT))
    elif ip_version == 6:
        s = socket.socket(socket.AF_INET6, socket.SOCK_STREAM)
        s.bind((SERVER_IP_V6, SERVER_PORT))
    else:
        print("Invalid IP version. Choose 4 or 6.")
        return None

    s.listen(1)
    return s

def handle_client(conn, addr):
    print(f"Connected by {addr}")

    # Step 1: Handshake
    conn.sendall(b'RemoteOpenWith Server 0.1\r\n')
    data = conn.recv(BUFFER_SIZE)
    print("Received handshake: ", data.decode())

    # Step 2: Verify
    conn.sendall(b'VERIFY\r\n')
    data = conn.recv(BUFFER_SIZE)
    print("Received verification: ", data.decode())
    conn.sendall(b'OK\r\n')

    # Step 3: Receive file
    file_metadata_raw = conn.recv(BUFFER_SIZE)
    print("Received file metadata: ", file_metadata_raw)
    try:
        file_metadata_decoded = file_metadata_raw.decode()
    except UnicodeDecodeError:
        # use GB2312 to decode
        try:
            file_metadata_decoded = file_metadata_raw.decode('gb2312')
        except Exception as e:
            # try another way of decoding
            file_metadata_bytes = file_metadata_raw.split(b':')
            file_metadata_bytes[1] = 'filename failed to decode'.encode()
            file_metadata_decoded = ':'.join(file_metadata_bytes)

    file_metadata = file_metadata_decoded.split(':')
    request_id, filename, file_size, file_hash = file_metadata
    file_size = int(file_size)
    print(f"Received file metadata: request_id={request_id}, filename={filename}, file_size={file_size}, file_hash={file_hash}")
    conn.sendall(b'OK\r\n')

    folder_path = os.path.join(FILE_FOLDER, request_id)
    os.makedirs(folder_path, exist_ok=True)
    file_path = os.path.join(folder_path, filename)

    with open(file_path, 'wb') as f:
        while file_size > 0:
            chunk = conn.recv(min(BUFFER_SIZE, file_size))
            f.write(chunk)
            file_size -= len(chunk)
        print(f"Received file: {file_path}")
    
    conn.sendall(b'OK\r\n')

    # Step 4: Exchange File Update Server Port
    data = conn.recv(BUFFER_SIZE)
    print("Received File Update Server Port: ", data.decode())
    
    # Open the file using OS GUI desktop environment
    os.startfile(file_path)

def serve_forever(s):
    print(f"Server is listening on {s.getsockname()}")
    while True:
        conn, addr = s.accept()
        try:
            handle_client(conn, addr)
        except Exception as e:
            print("Error: ", e)
        finally:
            conn.close()

def main():
    response_socket_v4 = create_response_socket(4)
    response_socket_v6 = create_response_socket(6)

    threading.Thread(target=serve_forever, args=(response_socket_v4,)).start()
    threading.Thread(target=serve_forever, args=(response_socket_v6,)).start()

if __name__ == "__main__":
    main()
