.PHONY: all clean


all: server client

server: server.cpp
	g++ -std=c++17 -Wall -o server server.cpp ./utils/tool.cpp ./utils/base64.cpp httpRequest.cpp httpResponse.cpp
	
client: client.cpp
	g++ -std=c++17 -Wall -o client client.cpp ./utils/tool.cpp ./utils/base64.cpp httpRequest.cpp httpResponse.cpp

test:
	g++ -std=c++17 -DDEBUG -Wall -o server server.cpp ./utils/tool.cpp ./utils/base64.cpp httpRequest.cpp httpResponse.cpp -fsanitize=address -fsanitize=undefined
	g++ -std=c++17 -DDEBUG -Wall -o client client.cpp ./utils/tool.cpp ./utils/base64.cpp httpRequest.cpp httpResponse.cpp -fsanitize=address -fsanitize=undefined

clean:
	@rm server client 
