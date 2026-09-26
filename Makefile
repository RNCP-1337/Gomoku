NAME     = Gomoku
CXX      = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++17 -O2
SRC      = src/main.cpp src/Board.cpp src/AI.cpp
OBJ      = $(SRC:src/%.cpp=obj/%.o)
HDR      = src/Board.hpp src/AI.hpp
LIBS     = -lncursesw
ifeq ($(shell uname), Darwin)
	LIBS = -lncurses
endif

all: $(NAME)

$(NAME): $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -o $(NAME) $(LIBS)

obj/%.o: src/%.cpp $(HDR) | obj
	$(CXX) $(CXXFLAGS) -c $< -o $@

obj:
	mkdir -p obj

clean:
	rm -rf obj

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
