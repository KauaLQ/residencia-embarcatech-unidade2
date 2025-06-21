import socket

HOST = "0.0.0.0"
PORT = 5005

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.bind((HOST, PORT))
    s.listen(1)
    print(f"Servidor escutando em {HOST}:{PORT}")

    conn, addr = s.accept()
    with conn:
        print(f"Conectado por {addr}")
        buffer = b""
        while True:
            data = conn.recv(1024)
            if not data:
                break
            buffer += data
            if b"\n" in buffer:
                lines = buffer.split(b"\n")
                for line in lines[:-1]:
                    try:
                        text = line.decode("utf-8").strip()
                        print(f"Recebido: {text}")
                    except UnicodeDecodeError:
                        print("Erro ao decodificar mensagem.")
                buffer = lines[-1]