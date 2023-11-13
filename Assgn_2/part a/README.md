## Instructions
- Building Docker images
    - docker build -f Dockerfile.client -t cli-img:1.0 .
    - docker build -f Dockerfile.server -t ser-img:1.0 .

- Create a Docker Network with name *aos*
    - docker network create --driver bridge aos

- Setting Sever Docker Container
    - docker run --rm -it --net=aos --privileged ser-img:1.0
	- Do make load
    - Do ./server

- Getting server ip in docker network
    - Open a new terminal and do docker network inspect aos | grep IPv4Address
    - Copy the IP address without mask, say server-ip

- Tracking ebpf output
    - Open a new terminal
    - Do sudo cat /sys/kernel/debug/tracing/trace_pipe

- Running Client Docker Container
    - Replace the server-ip with the copied value in the next command
    - docker run -it --rm -e IP={server-ip} --net=aos cli-img:1.0

- Go on and check resutls.

- After completion of checking results,
    - Do ctrl + C in client and the client is closed
    - Do ctrl + C in server and the server is closed
    - Do make unload and this will detach ebpf
    - Do exit in the server and client container
    - Do docker network rm aos
