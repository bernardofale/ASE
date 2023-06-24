import os
import socket
import threading
import signal
import time
from flask import Flask, request, render_template

app = Flask(__name__)
tcp_socket = None
data = "{ \"distance\":\"0.00\", \"samplerate\":\"0.00\"}"  # Global variable to store the received data

@app.route('/', methods=['GET'])
def root():
    return render_template('index.html', data=data)

@app.route('/get_data', methods=['GET'])
def get_data():
    global data
    return data

def start_server():
    app.run(host='0.0.0.0', port=3000)

def start_socket():
    while True:
        # Create and bind the TCP socket
        global tcp_socket 
        if(tcp_socket is None or not isinstance(tcp_socket, socket.socket)):
            tcp_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            try:
                tcp_socket.bind(('0.0.0.0', 5000))
            except OSError as e:
                print("Failed to bind socket:", e)
                time.sleep(10)
                continue
            except Exception as e:
                print("An error occurred:", e)
                time.sleep(10)
                continue

        tcp_socket.listen(1)
        print("Server Socket -> Waiting for connection...")

        client_socket, client_address = tcp_socket.accept()
        client_socket.settimeout(10)
        print("Server Socket -> Connected to:", client_address)

        # Receive and process packets from the ESP32
        global data
        while True:
            try:
                data = client_socket.recv(1024).decode('utf-8')
                if not data:
                    break
                print("Server Socket -> Received from", client_address, ":", data)
                # Process the received data and prepare a response
                # In this example, we'll just send back an acknowledgement message
                response = "Hello from PC"

                # Send the response back to the ESP32
                client_socket.sendall(response.encode('utf-8'))
            except socket.timeout:
                print("Timeout occurred. No response received within", 10, "seconds.")
                client_socket.close()
                break

            except socket.error as e:
                print("Socket error occurred:", e)
                client_socket.close()
                break

            except Exception as e:
                print("An error occurred:", e)
                client_socket.close()
                break

    # Close the client socket
    client_socket.close()
    print("Server Socket -> Connection closed.")

def signal_handler(signal, frame):
    # Handle Ctrl+C
    print("Ctrl+C detected. Exiting...")
    tcp_socket.close()
    os._exit(0)  # Terminate the program forcefully

if __name__ == '__main__':
    # Register the signal handler for Ctrl+C
    signal.signal(signal.SIGINT, signal_handler)

    # Create threads for server and socket
    server_thread = threading.Thread(target=start_server)
    socket_thread = threading.Thread(target=start_socket)

    # Start the threads
    server_thread.start()
    socket_thread.start()

    # Wait for the threads to complete
    server_thread.join()
    socket_thread.join()
