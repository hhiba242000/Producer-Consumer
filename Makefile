all:
	g++ -o consumer consumer.cpp -lpthread
	g++ -o producer producer.cpp -lpthread
	