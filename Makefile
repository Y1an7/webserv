NAME        = webserv
CXX         = c++
CXXFLAGS    = -Wall -Wextra -Werror -std=c++98 -I. -Iincs -Iincs/config -Iincs/network -Iincs/http -Iincs/cgi -g3
# 1. Define files WITHOUT directory prefixes
# 1. 定义不带目录前缀的文件
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
# 2. 使用 addprefix 安全地构建路径 (万无一失)
SRCS        = $(addprefix $(SRC_DIR), $(FILES))
OBJS        = $(addprefix $(OBJ_DIR), $(FILES:.cpp=.o))

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

.PHONY: all clean fclean re