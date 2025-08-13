all:
	g++ -std=c++20 -o game_server game_server.cpp -lboost_system -lboost_json -lpthread -lboost_coroutine -lboost_context -lboost_regex
run:
	./game_server