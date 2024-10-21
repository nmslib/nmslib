FROM ubuntu:22.04

WORKDIR /code
COPY . . 

# Install dependencies
RUN apt update
RUN apt install -y cmake build-essential
RUN apt install -y python3 python3-pip
RUN pip3 install pybind11

WORKDIR /code/python_bindings
RUN pip3 install -r requirements.txt
