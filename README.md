Lab 3 RCOM João Melo Rui Sousa

Inicializar socat:
    sudo apt install socat
    sudo socat -d -d PTY,link=/dev/ttyS10,mode=777 PTY,link=/dev/ttyS11,mode=777

transmitter (writenoncanonical):
    gcc writenoncanonical.c -o write
    ./write /dev/ttyS10

receiver(readnoncanonical):
    gcc readnoncanonical.c -o read
    ./read /dev/ttyS11