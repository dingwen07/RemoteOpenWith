import socket
import os
import sys
import threading
import platform
import subprocess

SERVER_IP_V4 = "0.0.0.0"
SERVER_IP_V6 = "::"
SERVER_PORT = 2335
BUFFER_SIZE = 1024
FILE_FOLDER = './files'
PASSWORD_AUTH = False
PASSWORD = 'VerySecurePassword'
W32_PROHIBITED_EXTENSIONS = ['exe', 'bat', 'cmd', 'com', 'msi', 'ps1', 'scr', 'vbs']
W32_PROHIBITED_EXTENSIONS += ['lnk', 'py', 'pyw']

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
    print(f'Connected by {addr}')

    # Step 1: Handshake
    conn.sendall(b'RemoteOpenWith Server 0.1\r\n')
    data = conn.recv(BUFFER_SIZE)
    print('Received handshake:', data.decode().strip())

    # Step 2: Verify
    conn.sendall(b'VERIFY\r\n')
    data = conn.recv(BUFFER_SIZE)
    data = data.decode().strip()
    print('Received verification:', data)
    if data != PASSWORD:
        if PASSWORD_AUTH:
            conn.sendall(b'ERROR: Password incorrect (AUTH_FAILED_PWD)\r\n')
            conn.close()
            print('Password incorrect. Connection closed.')
            return
        else:
            # do port verification
            ports = data.split(',')
            print('Ports:', ports)
            if False:
                conn.sendall(b'ERROR: No known port (AUTH_FAILED_PORT)\r\n')
                conn.close()
                print('Port incorrect. Connection closed.')
                return
    conn.sendall(b'OK\r\n')

    # Step 3: Receive file
    file_metadata_raw = conn.recv(BUFFER_SIZE)
    print('Received file metadata:', file_metadata_raw)
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

    file_metadata_decoded = file_metadata_decoded.strip()
    file_metadata = file_metadata_decoded.split(':')
    request_id, filename, file_size, file_hash = file_metadata
    file_size = int(file_size)
    print(f'Received file metadata: request_id={request_id}, filename={filename}, file_size={file_size}, file_hash={file_hash}')
    # check if `./files/request_id` exists, if so, duplicate request, reject
    if os.path.exists(os.path.join(FILE_FOLDER, request_id)):
        print(f'Duplicate request: {request_id}, reject.')
        conn.sendall(b'ERROR: Duplicate request (DUPLICATE_REQUEST)\r\n')
        conn.close()
        return
    # check if file extension is executable for Microsoft Windows
    if platform.system() == 'Windows':
        if filename.split('.')[-1].lower() in W32_PROHIBITED_EXTENSIONS:
            print('File extenshion {} is prohibited for Microsoft Windows'.format(filename.split('.')[-1].lower()))
            conn.sendall(b'ERROR: File Rejected (W32_PROHIBITED)\r\n')
            conn.close()
            return
    # else for POSIX
    elif sys.platform == 'posix':
        # remove executable permission for POSIX
        # get current file permission
        file_permission = os.stat(filename).st_mode
        # remove executable permission
        file_permission = file_permission & ~stat.S_IXUSR
        # set new file permission
        os.chmod(filename, file_permission)

    conn.sendall(b'OK\r\n')

    folder_path = os.path.join(FILE_FOLDER, request_id)
    os.makedirs(folder_path, exist_ok=True)
    file_path = os.path.join(folder_path, filename)

    with open(file_path, 'wb') as f:
        while file_size > 0:
            chunk = conn.recv(min(BUFFER_SIZE, file_size))
            f.write(chunk)
            file_size -= len(chunk)
        print(f'Received file: {file_path}')
    
    conn.sendall(b'OK\r\n')

    # Step 4: Exchange File Update Server Port
    data = conn.recv(BUFFER_SIZE)
    try:
        data = data.decode()
    except UnicodeDecodeError:
        data = '0'
    
    try:
        file_update_server_port = int(data)
    except ValueError:
        file_update_server_port = 0
    print('Received File Update Server Port:', str(file_update_server_port))
    
    # Open the file
    open_file_platform(file_path)

def serve_forever(s):
    print(f'Server is listening on {s.getsockname()}')
    while True:
        conn, addr = s.accept()
        try:
            handle_client(conn, addr)
        except Exception as e:
            print('Error: ', e)
        finally:
            conn.close()

def open_file_platform(file_name):
    if platform.system() == 'Windows':
        os.startfile(file_name)
    elif platform.system() == 'Darwin':
        subprocess.Popen(["open", file_name])
    else:
        subprocess.Popen(["xdg-open", file_name])

def main():
    response_socket_v4 = create_response_socket(4)
    response_socket_v6 = create_response_socket(6)

    threading.Thread(target=serve_forever, args=(response_socket_v4,)).start()
    threading.Thread(target=serve_forever, args=(response_socket_v6,)).start()

if __name__ == '__main__':
    main()
