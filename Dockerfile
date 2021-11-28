FROM silkeh/clang:11 AS builder

RUN mkdir -p auction_house
COPY include /home/user/auction_house/include
COPY src /home/user/auction_house/src
COPY tests /home/user/auction_house/tests
COPY CMakeLists.txt /home/user/auction_house/CMakeLists.txt
COPY main.cpp /home/user/auction_house/main.cpp
WORKDIR /home/user/auction_house

RUN mkdir cmake-build-release
WORKDIR /home/user/auction_house/cmake-build-release
RUN cmake -DCMAKE_BUILD_TYPE=Release /home/user/auction_house
RUN make

FROM builder as test
WORKDIR /home/user/
COPY --from=builder /home/user/auction_house/cmake-build-release/tests tests
RUN ./tests

#TODO: this image could be smaller
FROM ubuntu:20.04
COPY --from=builder /home/user/auction_house/cmake-build-release/auction_house /bin/auction_house
CMD ["auction_house"]
ENTRYPOINT ["sh", "-c"]