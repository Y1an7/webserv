NAME        = webserv
CXX         = c++
CXXFLAGS    = -Wall -Wextra -Werror -std=c++98 -I. -Iincs -Iincs/config -Iincs/network -Iincs/http -Iincs/cgi -g3 -MMD -MP

# 1. Define files WITHOUT directory prefixes
FILES       = main.cpp \
              config/ConfigParser.cpp \
              config/ServerConfig.cpp \
              config/Location.cpp \
              network/ServerSocket.cpp \
              network/Server.cpp \
              network/Client.cpp \
              http/HttpRequest.cpp \
              http/HttpParser.cpp \
              http/HttpResponse.cpp \
              http/RequestHandler.cpp \
              cgi/CgiHandler.cpp

SRC_DIR     = srcs/
OBJ_DIR     = objs/

# 2. Use addprefix to securely build the paths (Foolproof)
SRCS        = $(addprefix $(SRC_DIR), $(FILES))
OBJS        = $(addprefix $(OBJ_DIR), $(FILES:.cpp=.o))

DEPS		= $(OBJS:.o=.d)

all: $(NAME)

$(NAME): $(OBJS)
	@echo "Linking $(NAME)..."
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)
	@echo "Build successful!"

# The compilation rule remains the same
$(OBJ_DIR)%.o: $(SRC_DIR)%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@


clean:
	rm -rf $(OBJ_DIR)
	@echo "Object files cleaned."

fclean: clean
	rm -f $(NAME)
	@echo "Executable cleaned."

re: fclean all

-include &(DEPS)

.PHONY: all clean fclean re