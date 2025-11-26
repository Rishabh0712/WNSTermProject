CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -g
LDFLAGS = -lssl -lcrypto

# Source files
SOURCES = shamir_secret_sharing.cpp tls_multiparty.cpp test_tls_multiparty.cpp
OBJECTS = $(SOURCES:.cpp=.o)
TARGET = test_tls_multiparty

# Header files
HEADERS = shamir_secret_sharing.hpp tls_multiparty.hpp

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

run: $(TARGET)
	./$(TARGET)

# Individual object file dependencies
shamir_secret_sharing.o: shamir_secret_sharing.cpp shamir_secret_sharing.hpp
tls_multiparty.o: tls_multiparty.cpp tls_multiparty.hpp shamir_secret_sharing.hpp
test_tls_multiparty.o: test_tls_multiparty.cpp tls_multiparty.hpp shamir_secret_sharing.hpp
