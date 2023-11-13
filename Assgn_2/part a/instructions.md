## Part-1
- Building Docker images
    - docker build -f Dockerfile.client1 -t cli-img:1.0 .
    - docker build -f Dockerfile.server1 -t ser-img:1.0 .

- Create a Docker Network with name *mynet*
    - docker network create --driver bridge mynet

- Setting Sever Docker Container
    - docker run --rm -it --net=mynet --privileged ser-img:1.0
    - Go to ebpf folder in the interactive terminal of container using cd ebpf
    - Do make and this will attach the ebpf in the kernel
    - Do cd ..
    - Do ./server

- Getting server ip in docker network
    - Open a new terminal and do docker network inspect mynet | grep IPv4Address
    - Copy the IP address without mask, say serv-ip

- Tracking ebpf output
    - Open a new terminal
    - Do sudo cat /sys/kernel/debug/tracing/trace_pipe

- Running Client Docker Container
    - Replace the serv-ip with the copied value in the next command
    - docker run -it --rm -e IP={serv-ip} --net=mynet cli-img:1.0

- Go on and check resutls.

- After completion of checking results,
    - Do ctrl + C in client and the container will be closed
    - Do ctrl + C in server and the server is closed
    - Go to ebpf folder i.e., cd ebpf
    - Do make clean and this will detach ebpf
    - Do exit in the server container
    - Do docker network rm mynet

## Part-2