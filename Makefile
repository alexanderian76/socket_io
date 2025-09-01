all:
	g++ -std=c++20 -o game_server game_server.cpp db.cpp -lboost_system -lboost_json -lpthread -lboost_coroutine -lboost_context -lboost_regex -lsqlite3
run:
	./game_server